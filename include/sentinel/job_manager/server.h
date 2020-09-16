//
// Created by mani on 9/14/2020.
//

#ifndef RHEA_JOB_MANAGER_H
#define RHEA_JOB_MANAGER_H

namespace sentinel::job_manager{
    class server {
    private:
        std::shared_ptr<RPC> rpc;

        void RunInternal(std::future<void> futureObj);
    public:
        void Run(std::future<void> futureObj);
        explicit Server();
        int Store(Data &source, Data &destination);
        Data Locate(Data &source, Data &destination);
        size_t Size(Data &request);
        bool Delete(Data &request);
    };
}


#endif //RHEA_JOB_MANAGER_H
