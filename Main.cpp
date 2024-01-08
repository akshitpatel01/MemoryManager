#include <functional>
#include <memory>
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "rpc.h"
#include "node_manager.h"

#define NUMBER 8000

int main(int argc, char** argv)
{
    /*
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
    */
    /*node_manager test{};
    std::string db1 = "test2";
    test.add_db(db1);
    cleanup(test);
    //test1(test);
    //test2(test);
    //*/
    //const char *data = "hehehehhehe\n";
    node_manager manager{};
    std::string db1 = "test5";
    std::string db2 = "test4";
    manager.add_db(db1);
    manager.add_db(db2);
    manager.del_all(1); 
    manager.del_all(2); 
    RPC_helper* m_rpc = new gRPC(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()), std::bind(&node_manager::insert, &manager, std::placeholders::_1, std::placeholders::_2),
                                                                                                                std::bind(&node_manager::lookup, &manager, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                                                                                                                std::bind(&node_manager::remove, &manager, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    auto n = m_rpc->register_node();
    m_rpc->register_db(n);
    m_rpc->register_db(n);
    m_rpc->wait();
}
