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

#define SENTINEL_CONF basket::Singleton<sentinel::ConfigurationManager>::GetInstance()
namespace sentinel {
    class ConfigurationManager {

    private:
        static std::string replaceEnvVariable(std::string temp_variable){

            std::string pattern("(\\$\\{.*?\\})");
            auto regexp = regex(pattern);
            smatch m;
            regex_search(temp_variable, m, regexp);
            auto variables=std::set<std::string>();
            for(unsigned i=0; i<m.size(); ++i) {
                auto extracted_val = m[i].str();
                //if(extracted_val.find("{") == std::string::npos) continue;
                auto val = m[i].str().substr(2, m[i].str().size() - 3);
                variables.insert(val);
            }
            for(auto variable:variables){
                auto unrolled = std::getenv(variable.c_str());
                if(unrolled==NULL) throw ErrorException(UNDEFINED_ENV_VARIABLE,variable.c_str());
                temp_variable = regex_replace(temp_variable, regexp, unrolled);
            }
            return temp_variable;
        }
        template <typename T>
        void config(T &doc, const char *member, uint16_t &variable) {
            if(!doc.HasMember(member)) return;
            assert(doc[member].IsInt());
            variable = atoi(replaceEnvVariable(std::to_string(doc[member].GetInt())).c_str());
        }
        template <typename T>
        void config(T &doc, const char *member, really_long &variable) {
            if(!doc.HasMember(member)) return;
            assert(doc[member].IsUint64());
            variable = atoll(replaceEnvVariable(std::to_string(doc[member].GetUint64())).c_str());
        }

        template <typename T>
        void config(T &doc, const char *member, std::string &variable) {
            if(!doc.HasMember(member)) return;
            assert(doc[member].IsString());
            std::string temp_variable = doc[member].GetString();
            variable = replaceEnvVariable(temp_variable);
        }
        template <typename T>
        void config(T &doc, const char *member, CharStruct &variable) {
            if(!doc.HasMember(member)) return;
            assert(doc[member].IsString());
            std::string temp_variable = doc[member].GetString();
            variable = CharStruct(replaceEnvVariable(temp_variable));
        }

        std::vector<CharStruct> LoadServers(std::vector<CharStruct> SERVER_LIST, CharStruct SERVER_LIST_PATH){
            SERVER_LIST=std::vector<CharStruct>();
            fstream file;
            file.open(SERVER_LIST_PATH.c_str(), ios::in);
            if (file.is_open()) {
                std::string file_line;

                int count;
                while (getline(file, file_line)) {
                    CharStruct server_node_name;
                    if (!file_line.empty()) {
                        int split_loc = file_line.find(':');  // split to node and net
                        if (split_loc != std::string::npos) {
                            server_node_name = file_line.substr(0, split_loc);
                            count = atoi(file_line.substr(split_loc+1, std::string::npos).c_str());
                        } else {
                            // no special network
                            server_node_name=file_line;
                            count = 1;
                        }
                        // server list is list of network interfaces
                        for(int i=0;i<count;++i){
                            SERVER_LIST.emplace_back(server_node_name);
                        }
                    }
                }
            } else {
                printf("Error: Can't open server list file %s\n", SERVER_LIST_PATH.c_str());
            }
            file.close();
            return SERVER_LIST;
        }

        int CountServers(CharStruct server_list_path) {
            fstream file;
            int total = 0;
            file.open(server_list_path.c_str(), ios::in);
            if (file.is_open()) {
                std::string file_line;
                std::string server_node_name;
                int count;
                while (getline(file, file_line)) {
                    if (!file_line.empty()) {
                        int split_loc = file_line.find(':');  // split to node and net
                        if (split_loc != std::string::npos) {
                            server_node_name = file_line.substr(0, split_loc);
                            count = atoi(file_line.substr(split_loc, std::string::npos).c_str());
                        } else {
                            // no special network
                            server_node_name = file_line;
                            count = 1;
                        }
                        // server list is list of network interfaces
                        for (int i = 0; i < count; i++) {
                            total++;
                        }
                    }
                }
            } else {
                printf("Error: Can't open server list file %s\n", server_list_path.c_str());
                exit(EXIT_FAILURE);
            }
            file.close();
            return total;
        }

    public:
        CharStruct JOBMANAGER_HOST_FILE, WORKERMANAGER_HOST_FILE;
        uint16_t JOBMANAGER_PORT, WORKERMANAGER_PORT;
        uint16_t JOBMANAGER_RPC_THREADS, WORKERMANAGER_RPC_THREADS;
        CharStruct JOBMANAGER_DIR, WORKERMANAGER_DIR;
        CharStruct CONFIGURATION_FILE;
        CharStruct WORKERMANAGER_DINAMIC_HOSTFILE;
        CharStruct WORKERMANAGER_EXECUTABLE;
        uint16_t JOBMANAGER_COUNT, WORKERMANAGER_COUNT;
        uint16_t RANDOM_SEED;
        uint16_t MAX_LOAD;
        ResourceAllocation DEFAULT_RESOURCE_ALLOCATION;
        std::vector<CharStruct> WORKERMANAGER_LISTS;


        ConfigurationManager() : JOBMANAGER_HOST_FILE("/home/user/symbios/conf/server_lists/single_node_rhea_jobmanager"),
                                 WORKERMANAGER_HOST_FILE("/home/user/symbios/conf/server_lists/single_node_rhea_workermanager"),
//                                 WORKERMANAGER_LISTS(),
                                 JOBMANAGER_PORT(8000),
                                 WORKERMANAGER_PORT(9000),
                                 JOBMANAGER_RPC_THREADS(4),
                                 WORKERMANAGER_RPC_THREADS(4),
                                 DEFAULT_RESOURCE_ALLOCATION(1,1,1),
                                 JOBMANAGER_DIR("/dev/shm/hari/single_node_jobmanager_server"), //TODO: CHECK if they have to be different
                                 WORKERMANAGER_DIR("/dev/shm/hari/single_node_workermanager_server"),
                                 CONFIGURATION_FILE("/home/user/sentinel/conf/base_rhea.conf"),
                                 WORKERMANAGER_DINAMIC_HOSTFILE("/home/user/symbios/conf/server_lists/single_node_rhea_dyn_workermanager"),
                                 WORKERMANAGER_EXECUTABLE("/home/user/symbios/build/workermanager_server"),
                                 JOBMANAGER_COUNT(1),
                                 WORKERMANAGER_COUNT(1),
                                 MAX_LOAD(0.8),
                                 RANDOM_SEED(100){}

        void LoadConfiguration() {
            using namespace rapidjson;

            FILE *outfile = fopen(CONFIGURATION_FILE.c_str(), "r");
            if (outfile == NULL) {
                printf("Configuration not found %s \n",CONFIGURATION_FILE.c_str());
                exit(EXIT_FAILURE);
            }
            char buf[65536];
            FileReadStream instream(outfile, buf, sizeof(buf));
            Document doc;
            doc.ParseStream<kParseStopWhenDoneFlag>(instream);
            if (!doc.IsObject()) {
                std::cout << "Configuration JSON is invalid" << std::endl;
                fclose(outfile);
                exit(EXIT_FAILURE);
            }
            config(doc, "JOBMANAGER_HOST_FILE", JOBMANAGER_HOST_FILE);
            config(doc, "WORKERMANAGER_HOST_FILE", WORKERMANAGER_HOST_FILE);
            config(doc, "JOBMANAGER_PORT", JOBMANAGER_PORT);
            config(doc, "WORKERMANAGER_PORT", WORKERMANAGER_PORT);
            config(doc, "JOBMANAGER_RPC_THREADS", JOBMANAGER_RPC_THREADS);
            config(doc, "WORKERMANAGER_RPC_THREADS", WORKERMANAGER_RPC_THREADS);
            config(doc, "JOBMANAGER_DIR", JOBMANAGER_DIR);
            config(doc, "WORKERMANAGER_DIR", WORKERMANAGER_DIR);
            config(doc, "CONFIGURATION_FILE", CONFIGURATION_FILE);
            config(doc, "WORKERMANAGER_DINAMIC_HOSTFILE", WORKERMANAGER_DINAMIC_HOSTFILE);
            config(doc, "WORKERMANAGER_EXECUTABLE", WORKERMANAGER_EXECUTABLE);
            config(doc, "JOBMANAGER_COUNT", JOBMANAGER_COUNT);
            config(doc, "WORKERMANAGER_COUNT", WORKERMANAGER_COUNT);
            config(doc, "RANDOM_SEED", RANDOM_SEED);
            boost::filesystem::create_directories(JOBMANAGER_DIR.c_str());
            boost::filesystem::create_directories(WORKERMANAGER_DIR.c_str());

            fclose(outfile);
        }

        void ConfigureJobmanagerClient() {
            LoadConfiguration();
            BASKET_CONF->ConfigureDefaultClient(JOBMANAGER_HOST_FILE.c_str());
            BASKET_CONF->RPC_PORT = JOBMANAGER_PORT;
        }

        void ConfigureJobManagerServer() {
            LoadConfiguration();
            BASKET_CONF->RPC_THREADS = JOBMANAGER_RPC_THREADS;
            BASKET_CONF->MEMORY_ALLOCATED = 1024ULL * 1024ULL * 1ULL;
            BASKET_CONF->BACKED_FILE_DIR=JOBMANAGER_DIR;
            BASKET_CONF->ConfigureDefaultServer(JOBMANAGER_HOST_FILE.c_str());
            LoadServers(WORKERMANAGER_LISTS, WORKERMANAGER_HOST_FILE);
            JOBMANAGER_COUNT = BASKET_CONF->NUM_SERVERS;
            BASKET_CONF->RPC_PORT = JOBMANAGER_PORT;
        }

        void ConfigureWorkermanagerClient() {
            LoadConfiguration();
            BASKET_CONF->ConfigureDefaultClient(WORKERMANAGER_HOST_FILE.c_str());
            BASKET_CONF->RPC_PORT = WORKERMANAGER_PORT;
        }

        void ConfigureWorkermanagerServer() {
            LoadConfiguration();
            BASKET_CONF->RPC_THREADS = WORKERMANAGER_RPC_THREADS;
            BASKET_CONF->MEMORY_ALLOCATED = 1024ULL * 1024ULL * 1ULL;
            BASKET_CONF->BACKED_FILE_DIR=WORKERMANAGER_DIR;
            BASKET_CONF->ConfigureDefaultServer(WORKERMANAGER_HOST_FILE.c_str());
            WORKERMANAGER_COUNT = BASKET_CONF->NUM_SERVERS;
            BASKET_CONF->RPC_PORT = WORKERMANAGER_PORT;
        }
    };
}
#endif //SENTINEL_COMMON_CONFIGURATION_MANAGER_H
