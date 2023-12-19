#include "blob_manager.h"
#include "central_manager.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "hash_helper.h"
#include "rpc.h"
#include <chrono>
#include <thread>
void test_add(Blob_manager* _obj)
{
    std::this_thread::sleep_for(std::chrono::seconds(10));

    _obj->add("test.txt");
}
int main()
{
    gRPC m_rpc{grpc::CreateChannel("localhost:50052", grpc::InsecureChannelCredentials())};
    Central_manager m_central_manager{m_rpc, true};
    Hash_helper hh{m_central_manager};
    Blob_manager blob{hh, m_rpc, m_central_manager};

    std::thread t1(test_add, &blob);
    t1.join();
    m_rpc.wait();
}
