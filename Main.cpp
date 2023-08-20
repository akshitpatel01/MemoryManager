#include "queue"
#include "hash.h"
#include <chrono>
#include <future>
#include <iostream>
#include <ostream>
#include <thread>
#include "scope_profiler.h"
#include <future>

bool lookup_func(void *a, void *b)
{
    int *a1 = static_cast<int*>(a);
    int *b1 = static_cast<int*>(b);
    return (*a1 == *b1);
}

void insert_thread(Hash<Hash_linear, int, int>* h_obj)
{
    PROFILE_SCOPE();
    uint i;
    int *key, *val;

    for (i = 0; i < 100000; i++) {
        key = new int(i);
        val = new int(*key+105);
        h_obj->insert(key, val);
    }
}

void lookup_thread(Hash<Hash_linear, int, int>* h_obj)
{
    PROFILE_SCOPE();
    uint i;
    int *key  = new int(5);

    for (i = 0; i < 100000; i++) {
        h_obj->lookup(key);
    }
}
void remove_thread(Hash<Hash_linear, int, int>* h_obj)
{
    PROFILE_SCOPE();
    uint i;
    int *key;

    for (i = 0; i < 100000; i++) {
        key = new int(i);
        h_obj->remove(key);
    }
}

int main()
{
    Hash<Hash_linear, int, int> *h_obj = new Hash<Hash_linear, int, int>(lookup_func);
    PROFILE_SCOPE();
#ifdef MULTI_THREAD
    //auto fut = std::async(std::launch::async, insert_thread, h_obj);
    auto fut1 = std::async(std::launch::async, lookup_thread, h_obj);
    auto fut2 = std::async(std::launch::async, remove_thread, h_obj);
    auto fut3 = std::async(std::launch::async, insert_thread, h_obj);
    auto fut4 = std::async(std::launch::async, remove_thread, h_obj);
    //std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto fut5 = std::async(std::launch::async, remove_thread, h_obj);

    //fut.wait();
    fut1.wait();
    fut2.wait();
    fut3.wait();
    fut4.wait();
    fut5.wait();
    
#else
    insert_thread(h_obj);
    lookup_thread(h_obj);
    remove_thread(h_obj);
    insert_thread(h_obj);
    remove_thread(h_obj);
//#define BASIC_TEST
#ifdef BASIC_TEST
    int *key = new int(10);
    int key1 = 10;
    int key2 = 11;
    int val = 50;
    int val2 = 100;
    int *test = nullptr;
    if ((test = h_obj->lookup(key)) != nullptr) {
        std::cout << "key " << *test << std::endl;
    }
    test = nullptr;
    if ((test = h_obj->lookup(&key1)) != nullptr) {
        std::cout << "key1 " << *test << std::endl;
    }
    test = nullptr;
    if ((test = h_obj->lookup(&key2)) != nullptr) {
        std::cout << "key2 " << *test << std::endl;
    }
    h_obj->insert(key, &val);
    if ((test = h_obj->lookup(key)) != nullptr) {
        std::cout << "key " << *test << std::endl;
    }
    test = nullptr;
    if ((test = h_obj->lookup(&key1)) != nullptr) {
        std::cout << "key1 " << *test << std::endl;
    }
    test = nullptr;
    if ((test = h_obj->lookup(&key2)) != nullptr) {
        std::cout << "key2 " << *test << std::endl;
    }
    h_obj->insert(key, &val2);
    test = nullptr;
    if ((test = h_obj->lookup(key)) != nullptr) {
        std::cout << "key " << *test << std::endl;
    }
    test = nullptr;
    if ((test = h_obj->lookup(&key1)) != nullptr) {
        std::cout << "key1 " << *test << std::endl;
    }
    test = nullptr;
    if ((test = h_obj->lookup(&key2)) != nullptr) {
        std::cout << "key2 " << *test << std::endl;
    }
    h_obj->remove(&key1);
    if ((test = h_obj->lookup(key)) != nullptr) {
        std::cout << "key " << *test << std::endl;
    }
    test = nullptr;
    if ((test = h_obj->lookup(&key1)) != nullptr) {
        std::cout << "key1 " << *test << std::endl;
    }
    test = nullptr;
    if ((test = h_obj->lookup(&key2)) != nullptr) {
        std::cout << "key2 " << *test << std::endl;
    }
#endif

#endif

    delete h_obj;
}
