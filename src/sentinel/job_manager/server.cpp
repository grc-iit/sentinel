#include <sentinel/job_manager/Server.h>


void sentinel::job_manager::Server::Run(std::future<void> futureObj,common::Daemon<Server> * obj) {
    daemon = obj;
    RunInternal(std::move(futureObj));
}

void sentinel::job_manager::Server::RunInternal(std::future<void> futureObj) {
    while(futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout){
        usleep(10000);
    }
}

bool sentinel::job_manager::Server::SubmitJob(JobId jobId, TaskId num_sources){
    AUTO_TRACER("sentinel::job_manager::Server::SubmitJob ", jobId, num_sources);
    auto classLoader = ClassLoader();
    std::shared_ptr<Job<Event>> job = classLoader.LoadClass<Job<Event>>(jobId);
    std::unique_lock job_lock(job_mutex_);
    jobs.insert(std::make_pair(jobId, job));
    job_lock.unlock();

    ResourceAllocation defaultResourceAllocation = SENTINEL_CONF->DEFAULT_RESOURCE_ALLOCATION;
    defaultResourceAllocation.job_id_ = jobId;

    auto vect = std::vector<WorkerManagerResource>();
    std::unique_lock resource_lock(resources_mutex_);
    used_resources.insert(std::make_pair(jobId, vect));
    resource_lock.unlock();

    auto threads = defaultResourceAllocation.num_nodes_ * defaultResourceAllocation.num_procs_per_node * defaultResourceAllocation.num_threads_per_proc;
    SpawnWorkerManagers(threads, jobId);

    usleep(40000);

    auto collector = job->GetTask();
    auto current_worker_index = 0;

    std::shared_lock resource_lock_shared(job_mutex_);
    auto workers = used_resources.find(jobId);
    resource_lock_shared.unlock();
    auto current_worker_thread = workers->second[current_worker_index].threads_.begin();
    for(int i=0;i<num_sources;i++){
        WorkerManagerId workermanager = workers->second[current_worker_index].id_;

        Event event;
        event.id_ = std::to_string(i);
        auto available_threads = std::set<ThreadId>();
        for(const auto& thread:workers->second[current_worker_index].threads_){
            auto iter =  workers->second[current_worker_index].excluded_threads_.find(thread);
            if(iter == workers->second[current_worker_index].excluded_threads_.end()) available_threads.emplace(thread);
        }
        auto assigned_thread_id = workermanager_client->AssignTask(workermanager, available_threads, jobId, collector->id_,event);
        workers->second[current_worker_index].excluded_threads_.emplace(assigned_thread_id);
        //Lets ensure that load map is not empty
        WorkerManagerStats wms = WorkerManagerStats();
        UpdateWorkerManagerStats(workermanager, wms);
        current_worker_thread++;
        if(current_worker_thread == workers->second[current_worker_index].threads_.end()){
            current_worker_index++;
            if(workers->second.size() == current_worker_index && i != num_sources){
                //TODO: throw error.
                break;
            }else current_worker_thread = workers->second[current_worker_index].threads_.begin();
        }
    }
}

bool sentinel::job_manager::Server::TerminateJob(JobId jobId){
    AUTO_TRACER("sentinel::job_manager::Server::TerminateJob",jobId);
    auto kill_worker_managers = std::vector<WorkerManagerId>();
    std::unique_lock resource_lock_ex(resources_mutex_);
    auto possible_job = used_resources.find(jobId);
    if (possible_job == used_resources.end()) return false;
    auto workermanagers_used = used_resources.find(jobId);
    for(const auto& worker_manager_used: workermanagers_used->second){
        auto iter = available_workermanagers.find(worker_manager_used.id_);
        if(available_workermanagers.end() == iter){
            WorkerManagerResource resource;
            resource.id_=worker_manager_used.id_;
            resource.node_name_=worker_manager_used.node_name_;
            available_workermanagers.insert({worker_manager_used.id_,resource});
        }
        iter = available_workermanagers.find(worker_manager_used.id_);
        for(auto thread:worker_manager_used.threads_){
            iter->second.threads_.emplace(thread);
        }
        auto num_thread_avail = iter->second.threads_.size();
        if(num_thread_avail == SENTINEL_CONF->WORKERTHREAD_COUNT) {
            kill_worker_managers.emplace_back(worker_manager_used.id_);
        }
    }
    used_resources.erase(jobId);
    resource_lock_ex.unlock();
    std::unique_lock load_lock(load_mutex_);
    for(const auto& worker_id:kill_worker_managers){
        auto reverse_lookup = loadMap.find(worker_id);
        if(loadMap.end() != reverse_lookup){
            auto iter = reversed_loadMap.find(reverse_lookup->second);
            if(iter != reversed_loadMap.end()){
                if(iter->second == worker_id){
                    reversed_loadMap.erase(iter);
                }
                iter++;
            }
            loadMap.erase(reverse_lookup);
        }
    }
    load_lock.unlock();
    for(const auto& worker_id:kill_worker_managers) {
        workermanager_client->FinalizeWorkerManager(worker_id);
    }
    return true;
}

bool sentinel::job_manager::Server::UpdateWorkerManagerStats(WorkerManagerId workerManagerId, WorkerManagerStats &stats){
    AUTO_TRACER("sentinel::job_manager::Server::UpdateWorkerManagerStats",workerManagerId,stats);
    std::unique_lock load_lock(load_mutex_);
    auto possible_load = loadMap.find(workerManagerId);
    if (possible_load == loadMap.end()){
        loadMap.insert(std::pair<WorkerManagerId, WorkerManagerStats>(workerManagerId, stats));
    }
    else {
        possible_load->second = stats;
        reversed_loadMap.erase(stats);
    }
    reversed_loadMap.insert(std::pair<WorkerManagerStats, WorkerManagerId>(stats, workerManagerId));
    load_lock.unlock();
    return true;
}

std::pair<bool, WorkerManagerStats> sentinel::job_manager::Server::GetWorkerManagerStats(WorkerManagerId workerManagerId){
    AUTO_TRACER("sentinel::job_manager::Server::UpdateWorkerManagerStats",workerManagerId);
    auto possible_load = loadMap.find(workerManagerId);
    if (possible_load == loadMap.end()) return std::pair<bool, WorkerManagerStats>(false, WorkerManagerStats());
    return std::pair<bool, WorkerManagerStats>(true, possible_load->second);
}


std::vector<std::tuple<JobId , std::set<ThreadId>, TaskId>> sentinel::job_manager::Server::GetNextNode(JobId job_id, TaskId currentTaskId, Event &event){
    AUTO_TRACER("job_manager::GetNextNode::resources", job_id, currentTaskId,event);
    auto newTasks = jobs.at(job_id)->GetTask(currentTaskId)->links;
    auto next_tasks = std::vector<std::tuple<JobId , std::set<ThreadId>, TaskId>>();
    for(const auto& task:newTasks){
        switch(task->type_){
            case TaskType::SOURCE:{
                break;
            }
            case TaskType::KEYBY:{
                std::shared_lock resourced_lock(resources_mutex_);
                auto iter = used_resources.find(job_id);
                size_t total_workers = iter->second.size();
                std::hash<size_t> worker_thread_hash;
                for(const auto& child_task:task->links){
                    auto child_event=event;
                    child_event.id_ += std::to_string(child_task->id_);
                    auto hash_event = task->Execute(event);
                    auto hash = atoi(hash_event.id_.c_str());
                    auto worker_index = hash % total_workers;
                    auto worker_resource = iter->second[worker_index];
                    auto available_threads = std::set<ThreadId>();
                    for(const auto& thread:worker_resource.threads_){
                        auto iter =  worker_resource.excluded_threads_.find(thread);
                        if(iter == worker_resource.excluded_threads_.end()) available_threads.emplace(thread);
                    }
                    auto num_threads = available_threads.size();
                    uint16_t worker_thread_index = worker_thread_hash(hash) % num_threads;
                    auto iter = available_threads.begin();
                    std::advance(iter, worker_thread_index);
                    uint16_t worker_thread_id = *iter;
                    auto selected_threads = std::set<ThreadId>();
                    selected_threads.emplace(worker_thread_id);
                    next_tasks.emplace_back(worker_resource.id_, selected_threads, child_task->id_);
                }
                resourced_lock.unlock();
                break;
            }
            case TaskType::SINK:{
                std::shared_lock load_lock(load_mutex_);
                WorkerManagerId newWorkermanager = reversed_loadMap.begin()->second;
                load_lock.unlock();
                std::shared_lock resourced_lock(resources_mutex_);
                auto iter = used_resources.find(job_id);
                for(const auto& worker_resource:iter->second){
                    if(newWorkermanager == worker_resource.id_){
                        auto available_threads = std::set<ThreadId>();
                        for(const auto& thread:worker_resource.threads_){
                            auto iter =  worker_resource.excluded_threads_.find(thread);
                            if(iter == worker_resource.excluded_threads_.end()) available_threads.emplace(thread);
                        }

                        next_tasks.emplace_back(newWorkermanager, available_threads , task->id_);
                        break;
                    }
                }
                resourced_lock.unlock();

                break;
            }
        }
    }
    /*
     * TODO: appen used reosurce info
     */
    return next_tasks;
}

bool sentinel::job_manager::Server::ChangeResourceAllocation(ResourceAllocation &resourceAllocation){
    AUTO_TRACER("job_manager::Server::ChangeResourceAllocation", resourceAllocation);
    auto threads = resourceAllocation.num_nodes_ * resourceAllocation.num_procs_per_node * resourceAllocation.num_threads_per_proc;
    if( resourceAllocation.num_nodes_ > 0) return SpawnWorkerManagers(threads,resourceAllocation.job_id_);
    else if( resourceAllocation.num_nodes_ == 0) return TerminateWorkerManagers(resourceAllocation);
    return true;
}

bool sentinel::job_manager::Server::SpawnWorkerManagers(ThreadId required_threads, JobId job_id) {
    AUTO_TRACER("job_manager::Server::SpawnWorkerManagers", required_threads,job_id);
    char* cmd = SENTINEL_CONF->WORKERMANAGER_EXECUTABLE.data();
    MPI_Info info;
    MPI_Info_create(&info);
    auto left_threads = required_threads;
    std::unique_lock resourced_lock(resources_mutex_);
    auto available_worker_iter = available_workermanagers.begin();
    //TODO: throw error if no available workers.
    auto new_worker_spawn = std::vector<uint32_t>();
    while(left_threads > 0){
        auto worker_index = available_worker_iter->first;
        auto host = available_worker_iter->second.node_name_;
        ThreadId start_thread = *available_worker_iter->second.threads_.begin();
        if(start_thread == 0){
            new_worker_spawn.push_back(worker_index);
        }
        auto can_use_threads = available_worker_iter->second.threads_.size() < left_threads ? available_worker_iter->second.threads_.size() : left_threads;

        auto end_thread = start_thread + can_use_threads - 1;
        auto left_thread_in_worker = available_worker_iter->second.threads_.size() - can_use_threads;
        auto used_threads = std::set<ThreadId>();
        if(left_thread_in_worker == 0){
            used_threads = available_worker_iter->second.threads_;
            auto worker_id = available_worker_iter->first;
            available_worker_iter++;
            available_workermanagers.erase(worker_id);

        }else{
            auto current = available_worker_iter->second.threads_.begin();
            auto remove_threads = 0;
            while(can_use_threads != remove_threads && current != available_worker_iter->second.threads_.end()){
                used_threads.insert(*current);
                current++;
                remove_threads++;
            }
            current = available_worker_iter->second.threads_.begin();
            for(auto thread:used_threads)
                available_worker_iter->second.threads_.erase(thread);
        }
        auto used_resources_iter = used_resources.find(job_id);
        if(used_resources_iter != used_resources.end()){
            WorkerManagerResource resource;
            resource.id_=worker_index;
            resource.node_name_=host;
            resource.threads_=used_threads;
            used_resources_iter->second.emplace_back(resource);
        }

        left_threads -= can_use_threads;

    }
    resourced_lock.unlock();
    for(const auto& worker_index:new_worker_spawn){
        auto worker_resource = worker_managers[worker_index];
        MPI_Info_set(info,"host", worker_resource.node_name_.data());
        char * mpi_argv[4];
        mpi_argv[0] = SENTINEL_CONF->CONFIGURATION_FILE.data();
        mpi_argv[1] = std::to_string(worker_resource.port_).data();
        mpi_argv[2] = std::to_string(worker_resource.id_).data();
        mpi_argv[3] = (char *)0;
        MPI_Comm workerManagerComm=MPI_Comm();
        int errcodes[1];
        MPI_Comm_spawn(cmd, mpi_argv, 1, info, 0, MPI_COMM_WORLD, &workerManagerComm, errcodes );
        if( errcodes[0] != MPI_SUCCESS) throw ErrorException(SPAWN_WORKERMANAGER_FAILED);
    }
    return true;
}

bool sentinel::job_manager::Server::TerminateWorkerManagers(ResourceAllocation &resourceAllocation){
    AUTO_TRACER("job_manager::Server::TerminateWorkerManagers", resourceAllocation);
    auto workermanager_client = basket::Singleton<sentinel::worker_manager::Client>::GetInstance();
    for(int i=resourceAllocation.num_nodes_; i < 0; i++){
        std::unique_lock load_lock(load_mutex_);
        WorkerManagerId workermanager_killed = reversed_loadMap.begin()->second;
        reversed_loadMap.erase(reversed_loadMap.begin());
        loadMap.erase(workermanager_killed);
        load_lock.unlock();
        std::unique_lock resourced_lock(resources_mutex_);
        WorkerManagerResource resource;
        resource.id_=i;
        resource.node_name_=SENTINEL_CONF->WORKERMANAGER_LISTS[workermanager_killed];
        for(int i=0;i<SENTINEL_CONF->WORKERTHREAD_COUNT;++i)
            resource.threads_.insert(i);
        available_workermanagers.insert({workermanager_killed, resource});
        used_resources.erase(resourceAllocation.job_id_);
        resourced_lock.unlock();
        if(!workermanager_client->FinalizeWorkerManager(workermanager_killed)) throw ErrorException(TERMINATE_WORKERMANAGER_FAILED);;
    }
    return true;
}

