#include "hash_linear.h"
#include "queue"
#include "hash.h"
#include <chrono>
#include <future>
#include <iostream>
#include <ostream>
#include <thread>
#include "scope_profiler.h"
#include <future>

#define NUMBER 8000000
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
    uint count = 0;

    for (i = 0; i < NUMBER; i++) {
        key = new int(i);
        val = new int(*key+105);
        if (h_obj->insert(key, val) == false) {
            std::cout << "Insert failed. Count: " << count << std::endl;
        }
        count++;
        //std::cout << "Total Count: " << count << std::endl;
        if( count % 1000 == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void lookup_thread(Hash<Hash_linear, int, int>* h_obj)
{
    PROFILE_SCOPE();
    uint i;
    int *key  = new int(5);

    for (i = 0; i < NUMBER; i++) {
        h_obj->lookup(key);
    }
}
void remove_thread(Hash<Hash_linear, int, int>* h_obj)
{
    PROFILE_SCOPE();
    uint i;
    int *key;
    //uint count = 0;

    for (i = 0; i < NUMBER; i++) {
        key = new int(i);
        h_obj->remove(key);
        //count++;
        //std::cout << "Total Remove Count: " << count << std::endl;
    }
}
#if 0
extern std::atomic_ullong count1;
extern std::atomic_ullong count2;
extern std::atomic_ullong count3;
extern std::atomic_ullong count4;
extern std::atomic_ullong count5;
extern std::atomic_ullong count6;
void print_debug(Hash<Hash_linear, int, int>* h_obj)
{
    while(1) {
        /*
        std::cout << "Count1: " << count1 << std::endl;
        std::cout << "Count2: " << count2 << std::endl;
        std::cout << "Count3: " << count3 << std::endl;
        std::cout << "Count4: " << count4 << std::endl;
        std::cout << "Count6: " << count6 << std::endl;
        std::cout << "Count5: " << count5 << std::endl;
        */
        if (count1 < count3 || count2 < count4) {
            std::cout << "Count1: " << count1 << std::endl;
            std::cout << "Count2: " << count2 << std::endl;
            std::cout << "Count3: " << count3 << std::endl;
            std::cout << "Count4: " << count4 << std::endl;
            std::cout << "Count6: " << count6 << std::endl;
            std::cout << "Count5: " << count5 << std::endl;
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
#endif

int main()
{
    Hash<Hash_linear, int, int> *h_obj = new Hash<Hash_linear, int, int>(lookup_func);
    PROFILE_SCOPE();
#ifdef MULTI_THREAD
#if 0
    void *__key = nullptr;
    void *__val = nullptr;
    Hash_simple test_obj(5, lookup_func, sizeof(int));
    Hash_simple::hash_iter_t_ *__hash_iter = nullptr;
    int key1 = 50;
    int val2 = 100;

    test_obj.insert(0, &key1, &val2);
    __hash_iter = test_obj.iter_init();
    test_obj.insert(0, &key1, &val2);
    test_obj.insert(1, &key1, &val2);


    while((__key = test_obj.iter_get_key(__hash_iter))) {
        // del && add
        __val = test_obj.iter_get_val(__hash_iter);
        std::cout << *(int*)__key << " " << *(int*)__val << std::endl;
        test_obj.remove(__hash_iter->__bucket_ind, __key);
        __hash_iter = test_obj.iter_inc(__hash_iter);
    }
    test_obj.iter_clear(__hash_iter);
#endif
    //auto fut0 = std::async(std::launch::async, print_debug, h_obj);
    auto fut = std::async(std::launch::async, insert_thread, h_obj);
    auto fut1 = std::async(std::launch::async, lookup_thread, h_obj);
    auto fut2 = std::async(std::launch::async, remove_thread, h_obj);
    auto fut3 = std::async(std::launch::async, insert_thread, h_obj);
    auto fut4 = std::async(std::launch::async, remove_thread, h_obj);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto fut5 = std::async(std::launch::async, remove_thread, h_obj);

    //fut0.wait();
    fut.wait();
    fut1.wait();
    fut2.wait();
    fut3.wait();
    fut4.wait();
    fut5.wait();

#define BASIC_TEST
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
