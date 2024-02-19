#include "blob_manager.h"
#include "central_manager.h"
#include "hash_helper.h"
#include "include/blob_manager.h"
#include "rpc.h"
#include "scope_profiler.h"
#include <chrono>
#include <thread>
void test(Blob_manager* _obj)
{
    std::this_thread::sleep_for(std::chrono::seconds(10));
    Blob_manager::File<registration_apis::db_rsp>::iter ptr;

    {
        PROFILE_SCOPE_FUNC("Add");
        std::string fname{"test.txt"};
        std::cout << "Add start\n";
        auto file_p = _obj->async_stream_add(fname);
        file_p->wait();
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    {
        PROFILE_SCOPE_FUNC("Lookup");
        std::string fname{"test.txt"};
        auto file_p = _obj->async_get(fname);
        auto cnt = 0;
        while ((file_p->next_segment(ptr) == Blob_manager::in_progress)) {
#ifdef LOGS
            char c[ptr.len+1];
            memcpy(c, ptr.data, ptr.len);
            c[ptr.len] = '\0';
            std::cout << c << "\n";
#endif
            cnt++;
        }
        //std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "Successfull segment lookup: " << cnt << "\n";
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    {
        PROFILE_SCOPE_FUNC("Delete");
        std::string fname{"test.txt"};
        auto file = _obj->async_remove(fname);
        file->wait();
    }
    
    {
        std::string fname{"test.txt"};
        auto file = _obj->async_get(fname);
        uint cnt = 0;
        while (file && (file->next_segment(ptr) == Blob_manager::in_progress)) {
#ifdef LOGS
            char c[ptr.len+1];
            memcpy(c, ptr.data, ptr.len);
            c[ptr.len] = '\0';
            std::cout << c << "\n";
#endif
            cnt++;
        }
        std::cout << "Successfull segment lookup: " << cnt << "\n";
    }
    std::cout << "Done\n";
}
int main()
{
    gRPC m_rpc{};
    Central_manager m_central_manager{m_rpc, false};
    Hash_helper hash_helper{m_central_manager};
    Blob_manager blob{hash_helper, m_rpc, m_central_manager};

    std::thread t1(test, &blob);
    t1.join();
    m_rpc.wait();
}
