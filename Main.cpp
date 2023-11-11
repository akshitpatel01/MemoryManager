#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <thread>
#include "scope_profiler.h"
#include <future>
#include <vector>
#include "node_manager.h"

#define NUMBER 8000
void test2(node_manager& test)
{
    PROFILE_SCOPE();
    for(int i = 0; i < NUMBER*10; i++) {
        if (test.lookup(1, (std::string("file")+ std::to_string(i)).c_str(), i) == nullptr) {
            //std::cout << "lookup failed\n";
        }
    }
}
void test1(node_manager& test)
{
    PROFILE_SCOPE();
    for(int i = 0; i < NUMBER; i++) {
        test.insert(1, (std::string("file")+ std::to_string(i)).c_str(), i, (void*)"heheheiiii", 11);
    }
}
void cleanup(node_manager& test)
{

    std::string db1 = "test1";
    test.del_all(1);
}

int main(int argc, char** argv)
{
    int i = 0;
    node_manager node_mgr = node_manager();
    char data[] = "hehehe";
    argv++;

    for (i = 1; i < argc; i++) {
        std::string db_name{*argv};
        node_mgr.test();
        node_mgr.add_db(db_name);
        node_mgr.insert(i, "file1", 5, (void*)data, strlen(data));
        //node_mgr.remove(i, "file1", 5);
        std::shared_ptr<segment> s1 = node_mgr.lookup(i, "file1", 5);
        if (s1) {
            std::cout << "Id: " << s1->get_id() << " ";
            std::cout << "Len: " << s1->get_len() << " ";
            std::cout << std::endl;
        }
        argv++;
    }
    /*node_manager test{};
    std::string db1 = "test2";
    test.add_db(db1);
    cleanup(test);
    //test1(test);
    //test2(test);
    //*/
    //const char *data = "hehehehhehe\n";
}
