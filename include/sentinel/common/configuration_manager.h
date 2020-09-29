//
// Created by mani on 9/14/2020.
//

#ifndef SENTINEL_COMMON_CONFIGURATION_MANAGER_H
#define SENTINEL_COMMON_CONFIGURATION_MANAGER_H

#include <basket/common/singleton.h>
#include <basket/common/typedefs.h>
#include <sentinel/common/enumerations.h>
#include <sentinel/common/data_structures.h>
#include <basket/common/data_structures.h>
#include <basket/common/macros.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/reader.h>
#include <regex>
#include <boost/filesystem/operations.hpp>
#include <sentinel/common/error_codes.h>
#include <common/configuration_manager.h>

#define SENTINEL_CONF basket::Singleton<sentinel::ConfigurationManager>::GetInstance()
namespace sentinel {
    class ConfigurationManager: public common::ConfigurationManager {
    protected:

        void LoadChildConfigurations(void *doc_) override {
            rapidjson::Document* doc= (rapidjson::Document*)doc_;
            config(doc, "JOBMANAGER_HOST_FILE", JOBMANAGER_HOST_FILE);
            config(doc, "WORKERMANAGER_HOST_FILE", WORKERMANAGER_HOST_FILE);
            config(doc, "JOBMANAGER_PORT", JOBMANAGER_PORT);
            config(doc, "WORKERMANAGER_PORT_SERVER", WORKERMANAGER_PORT_SERVER);
            config(doc, "WORKERMANAGER_PORT_CLIENT", WORKERMANAGER_PORT_CLIENT);
            config(doc, "JOBMANAGER_RPC_THREADS", JOBMANAGER_RPC_THREADS);
            config(doc, "WORKERMANAGER_RPC_THREADS", WORKERMANAGER_RPC_THREADS);
            config(doc, "JOBMANAGER_DIR", JOBMANAGER_DIR);
            config(doc, "WORKERMANAGER_DIR", WORKERMANAGER_DIR);
            config(doc, "WORKERMANAGER_EXECUTABLE", WORKERMANAGER_EXECUTABLE);
            config(doc, "WORKERMANAGER_SCRIPT", WORKERMANAGER_SCRIPT);
            config(doc, "JOBMANAGER_COUNT", JOBMANAGER_COUNT);
            config(doc, "WORKERMANAGER_COUNT", WORKERMANAGER_COUNT);
            config(doc, "WORKERTHREAD_COUNT", WORKERTHREAD_COUNT);
            config(doc, "WORKERMANAGER_EPOCH_MS", WORKERMANAGER_EPOCH_MS);
            config(doc, "WORKERMANAGER_UPDATE_MIN_TASKS", WORKERMANAGER_UPDATE_MIN_TASKS);
            config(doc, "WORKERTHREAD_TIMOUT_MS", WORKERTHREAD_TIMOUT_MS);
            config(doc, "COLLECTORS_PER_SOURCE", COLLECTORS_PER_SOURCE);
            config(doc, "RANDOM_SEED", RANDOM_SEED);
            boost::filesystem::create_directories(JOBMANAGER_DIR.c_str());
            boost::filesystem::create_directories(WORKERMANAGER_DIR.c_str());
            WORKERMANAGER_LISTS=GetServers(WORKERMANAGER_HOST_FILE);
        }
    public:
        CharStruct JOBMANAGER_HOST_FILE, WORKERMANAGER_HOST_FILE;
        WorkerManagerId WORKERMANAGER_ID;
        uint16_t JOBMANAGER_PORT, WORKERMANAGER_PORT_CLIENT,WORKERMANAGER_PORT_SERVER;
        uint16_t JOBMANAGER_RPC_THREADS, WORKERMANAGER_RPC_THREADS;
        CharStruct JOBMANAGER_DIR, WORKERMANAGER_DIR;
        CharStruct WORKERMANAGER_EXECUTABLE,WORKERMANAGER_SCRIPT;
        uint16_t JOBMANAGER_COUNT, WORKERMANAGER_COUNT;
        uint16_t WORKERTHREAD_COUNT, WORKERMANAGER_EPOCH_MS, WORKERMANAGER_UPDATE_MIN_TASKS, WORKERTHREAD_TIMOUT_MS;
        uint16_t RANDOM_SEED;
        uint16_t MAX_LOAD;
        uint32_t COLLECTORS_PER_SOURCE;

        ResourceAllocation DEFAULT_RESOURCE_ALLOCATION;
        std::vector<CharStruct> WORKERMANAGER_LISTS;


        ConfigurationManager() : common::ConfigurationManager("/home/user/sentinel/conf/base_rhea.conf"),
                                 JOBMANAGER_HOST_FILE("${HOME}/projects/rhea/scripts/local/job_manager"),
                                 WORKERMANAGER_HOST_FILE("${HOME}/projects/rhea/scripts/local/worker_manager"),
                                 JOBMANAGER_PORT(9000),
                                 WORKERMANAGER_PORT_CLIENT(10000),
                                 WORKERMANAGER_PORT_SERVER(10000),
                                 JOBMANAGER_RPC_THREADS(4),
                                 WORKERMANAGER_ID(0),
                                 WORKERMANAGER_RPC_THREADS(4),
                                 DEFAULT_RESOURCE_ALLOCATION(0, 1,1,15),
                                 JOBMANAGER_DIR("/dev/shm/hari/single_node_jobmanager_server"), //TODO: CHECK if they have to be different
                                 WORKERMANAGER_DIR("/dev/shm/hari/single_node_workermanager_server"),
                                 WORKERMANAGER_EXECUTABLE("${HOME}/projects/rhea/build/sentinel/sentinel_worker_manager"),
                                 WORKERMANAGER_SCRIPT("${HOME}/projects/rhea/scripts/execute_worker_manager.sh"),
                                 JOBMANAGER_COUNT(1),
                                 COLLECTORS_PER_SOURCE(1),
                                 WORKERMANAGER_COUNT(1),
                                 WORKERTHREAD_COUNT(15),
                                 WORKERMANAGER_EPOCH_MS(50),
                                 WORKERMANAGER_UPDATE_MIN_TASKS(256),
                                 WORKERTHREAD_TIMOUT_MS(100),
                                 MAX_LOAD(0.8),
                                 WORKERMANAGER_LISTS({"localhost"}),
                                 RANDOM_SEED(100){
            LoadConfiguration();
        }



        void ConfigureJobmanagerClient() {
            LoadConfiguration();
            BASKET_CONF->ConfigureDefaultClient(JOBMANAGER_HOST_FILE.c_str());
            BASKET_CONF->RPC_PORT = JOBMANAGER_PORT;
            JOBMANAGER_COUNT = BASKET_CONF->NUM_SERVERS;
        }

        void ConfigureJobManagerServer() {
            LoadConfiguration();
            BASKET_CONF->RPC_THREADS = JOBMANAGER_RPC_THREADS;
            BASKET_CONF->MEMORY_ALLOCATED = 1024ULL * 1024ULL * 1024ULL;
            BASKET_CONF->BACKED_FILE_DIR=JOBMANAGER_DIR;
            BASKET_CONF->ConfigureDefaultServer(JOBMANAGER_HOST_FILE.c_str());
            JOBMANAGER_COUNT = BASKET_CONF->NUM_SERVERS;
            BASKET_CONF->RPC_PORT = JOBMANAGER_PORT;
        }

        void ConfigureWorkermanagerClient() {
            LoadConfiguration();
            BASKET_CONF->ConfigureDefaultClient(WORKERMANAGER_HOST_FILE.c_str());
            BASKET_CONF->RPC_PORT = WORKERMANAGER_PORT_CLIENT;
            WORKERMANAGER_COUNT = BASKET_CONF->NUM_SERVERS;
        }

        void ConfigureWorkermanagerServer() {
            LoadConfiguration();
            BASKET_CONF->RPC_THREADS = WORKERMANAGER_RPC_THREADS;
            BASKET_CONF->MEMORY_ALLOCATED = 1024ULL * 1024ULL * 1024ULL;
            BASKET_CONF->BACKED_FILE_DIR=WORKERMANAGER_DIR;
            BASKET_CONF->ConfigureDefaultServer(WORKERMANAGER_HOST_FILE.c_str());
            WORKERMANAGER_COUNT = BASKET_CONF->NUM_SERVERS;
            BASKET_CONF->RPC_PORT = WORKERMANAGER_PORT_SERVER;
        }



    };
}
#endif //SENTINEL_COMMON_CONFIGURATION_MANAGER_H
