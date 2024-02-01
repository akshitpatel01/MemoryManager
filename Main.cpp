#include "blob_manager.h"
#include "central_manager.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "hash_helper.h"
#include "rpc.h"
#include <chrono>
#include <cstring>
#include <thread>
void test_add(Blob_manager* _obj)
{
    std::this_thread::sleep_for(std::chrono::seconds(10));

    _obj->add("test.txt");
    auto seg_vector = _obj->get("test.txt");
    std::cout << seg_vector.size() << "Size\n";
    for (auto& seg: seg_vector) {
        std::cout << "Got something!!! " << seg->get_len() << "\n";
        char c[seg->get_len()+1];
        memcpy(c, seg->get_data(), seg->get_len());
        c[seg->get_len()] = '\0';
        std::cout << c << "\n";
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));

    _obj->remove("test.txt");
    seg_vector = _obj->get("test.txt");
    std::cout << seg_vector.size() << "Size\n";
    if (seg_vector.size() == 0) {
        std::cout << "fine\n";
    } else {
        std::cout << "not fine\n";
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
