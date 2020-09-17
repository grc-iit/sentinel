//
// Created by yejie on 9/16/20.
//

#include <basket/common/singleton.h>
#include <common/data_structure.h>
#include <sentinel/common/data_structures.h>
#include <stdio.h>
#include <memory>

extern "C" std::shared_ptr<Job> create_job_1() {
    printf("Begin to create object.....\n");
    return basket::Singleton<Job>::GetInstance();
}
extern "C" void free_job(Job* p) { delete p; }


