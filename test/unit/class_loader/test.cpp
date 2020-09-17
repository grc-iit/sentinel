//
// Created by yejie on 9/16/20.
//

#include <common/class_loader.h>
#include <common/error_codes.h>
#include <sentinel/common/data_structures.h>

int main(int argc, char* argv[]){
    ClassLoader class_loader;
    // load so file
    class_loader.LoadClass<Job>(2);
}

