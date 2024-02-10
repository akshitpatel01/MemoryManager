#include "blob_manager.h"
#include "central_manager.h"
#include "hash_helper.h"
#include "rpc.h"
#include <chrono>
#include <cstring>
#include <thread>
void test_add(Blob_manager* _obj)
{
    std::this_thread::sleep_for(std::chrono::seconds(10));
    File::iter ptr;

    {
        std::string fname{"test.txt"};
        auto file = _obj->async_add(fname);
        file->wait();
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    {
        std::string fname{"test.txt"};
        auto file = _obj->async_get(fname);
        while ((file->next(ptr))) {
            char c[ptr.len+1];
            memcpy(c, ptr.data, ptr.len);
            c[ptr.len] = '\0';
            std::cout << c << "\n";
        }
    }
#if 0
    for (auto& seg: seg_vector) {
        std::cout << "Got something!!! " << seg->get_len() << "\n";
        char c[seg->get_len()+1];
        memcpy(c, seg->get_data(), seg->get_len());
        c[seg->get_len()] = '\0';
        std::cout << c << "\n";
    }
#endif
    std::this_thread::sleep_for(std::chrono::seconds(10));

    {
        std::string fname{"test.txt"};
        auto file = _obj->async_remove(fname);
        file->wait();
    }
    
    {
        std::string fname{"test.txt"};
        auto file = _obj->async_get(fname);
        while (file && (file->next(ptr))) {
            char c[ptr.len+1];
            memcpy(c, ptr.data, ptr.len);
            c[ptr.len] = '\0';
            std::cout << c << "\n";
        }
    }
    std::cout << "Done\n";
}
int main()
{
    gRPC m_rpc{};
    Central_manager m_central_manager{m_rpc, true};
    Hash_helper hash_helper{m_central_manager};
    Blob_manager blob{hash_helper, m_rpc, m_central_manager};

    std::thread t1(test_add, &blob);
    t1.join();
    m_rpc.wait();
}
