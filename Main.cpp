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

    std::string db1 = "segment_db";
    test.del_all(1);
}
int main()
{
    node_manager test{};
    std::string db1 = "segment_db";
    test.add_db(db1);
    cleanup(test);
    test1(test);
    test2(test);
#if 0
    std::string db1 = "segment_db";
    const char *data = "hehehehhehe\n";
    node_manager test_obj = node_manager();
    test_obj.test();
    test_obj.add_db(db1);
    test_obj.insert(1, "file1", 5, (void*)data, strlen(data));
    std::shared_ptr<segment> s1 = test_obj.lookup(1, "file1", 5);
    if (s1) {
        std::cout << "Id: " << s1->get_id() << " ";
        std::cout << "Len: " << s1->get_len() << " ";
        std::cout << std::endl;
    }
#endif
}
