#pragma once

#include <cstdint>
#include <functional>
#include <grpcpp/support/status.h>
#include <ifaddrs.h>
#include <memory>
#include <sys/types.h>
#include "grpcpp/server_builder.h"
#include "grpcpp/server_context.h"
#include "registration_apis.grpc.pb.h"
#include "registration_apis.pb.h"
#include "segment.h"
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef enum ip_type_t_ {
    IPV4 = AF_INET,
    IPV6 = AF_INET6,
    IPV46 = AF_UNSPEC
} ip_type_t;

inline uint32_t get_ifc_info(std::string& ifc_name)
{
    struct ifaddrs *ifaddr, *ifa;
    char ip[INET_ADDRSTRLEN];
    uint32_t num_ip;

    if (getifaddrs(&ifaddr) == -1) 
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL)
            continue;  

        if(strcmp(ifc_name.c_str(), ifa->ifa_name) && ifa->ifa_addr->sa_family == IPV4)
        {
            num_ip = ((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr.s_addr;
            if (!inet_ntop(AF_INET, &num_ip, ip, INET_ADDRSTRLEN)) {
                printf("Error in IP conversion\n");
                return 0;
            }
            std::cout << ifa->ifa_name << ":" << ifa->ifa_addr << " " << ip << "\n";
            return num_ip;
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}
class RPC_helper {
    protected:
        using segment_t = segment<uint8_t*>;
        using add_cb = std::function<bool(const std::unique_ptr<segment_t>&, uint32_t)>;
        using lookup_cb = std::function<std::unique_ptr<segment_t>(const char*, uint32_t, uint32_t)>;
        using del_cb = std::function<bool(const char*, uint32_t, uint32_t)>;
    public:
        uint32_t register_db(uint32_t _id)
        {
            return _register_db(_id);
        }
        uint32_t register_node()
        {
            return _register_node();
        }
        virtual void wait(){}
    private:
        virtual uint32_t _register_db(uint32_t) = 0;
        virtual uint32_t _register_node() = 0;
};

class gRPC: public RPC_helper, registration_apis::db_update::Service {
    private:
        using add_cb = typename RPC_helper::add_cb;
        using lookup_cb = typename RPC_helper::lookup_cb;
        using del_cb = typename RPC_helper::del_cb;
        using segment_t = typename RPC_helper::segment_t;

        std::unique_ptr<registration_apis::Registeration::Stub> stub_;
        add_cb m_add_cb;
        lookup_cb m_lookup_cb;
        del_cb m_del_cb;
        std::thread server_thread;
        
    public:
        gRPC(std::shared_ptr<grpc::ChannelInterface> channel, add_cb&& _add_cb, lookup_cb&& _lookup_cb, del_cb&& _del_cb)
            :stub_(registration_apis::Registeration::NewStub(channel)),
             m_add_cb(std::move(_add_cb)),
             m_lookup_cb(std::move(_lookup_cb)),
             m_del_cb(std::move(_del_cb)),
             server_thread(&gRPC::start_server, this)
        {}
        ~gRPC()
        {
            server_thread.join();
        }
        void wait() override
        {
            server_thread.join();
        }

    private:
        std::unique_ptr<registration_apis::request> make_req(uint32_t _ip, uint32_t _id, registration_apis::Reg_action _action)
        {
            std::unique_ptr<registration_apis::request> ptr = std::make_unique<registration_apis::request>();

            ptr->set_ip(_ip);
            ptr->set_node_id(_id);
            ptr->set_action(_action);

            return ptr;
        }
        uint32_t _register_node() override
        {
            grpc::ClientContext context;
            registration_apis::response rsp{};
            std::string ifc = "en0";
            std::unique_ptr<registration_apis::request> req = make_req(get_ifc_info(ifc), 0, registration_apis::REG_NODE);
            grpc::Status status = stub_->register_node_db(&context, *req, &rsp); 

            if (status.ok()) {
                std::cout << rsp.id() << " Got it!!!\n";
                return rsp.id();
            }
            return 0;
        }
        uint32_t _register_db(uint32_t _nId) override
        {
            grpc::ClientContext context;
            registration_apis::response rsp{};
            std::unique_ptr<registration_apis::request> req = make_req(0, _nId, registration_apis::REG_DB);
            grpc::Status status = stub_->register_node_db(&context, *req, &rsp); 

            if (status.ok()) {
                std::cout << rsp.id() << " Got it!!!\n";
                return rsp.id();
            }
            return 0;

        }

        void start_server()
        {
            std::string server_address("localhost:50052");

            grpc::ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(this);
            std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
            std::cout << "Server listening on " << server_address << std::endl;
            server->Wait();
        }

    private:
        bool _notify_add_cbs(const std::unique_ptr<segment_t>& _segment, uint32_t _id)
        {
            std::cout << "triggered all db cbs\n";
            
            return m_add_cb(_segment, _id);
        }
        std::unique_ptr<segment_t> notify_lookup_cbs(const char* _file_name, uint32_t _seg_id, uint32_t _db_id)
        {
            return m_lookup_cb(_file_name, _seg_id, _db_id); 
        }
        bool notify_del_cbs(const char* _file_name, uint32_t _seg_id, uint32_t _db_id)
        {
            return m_del_cb(_file_name, _seg_id, _db_id);
        }
        grpc::Status add(grpc::ServerContext* _context, const registration_apis::add_meta* _add_req, registration_apis::db_rsp* _rsp) override
        {
            bool _ret;
            std::string s{_add_req->file_name()};
            const auto ptr = std::unique_ptr<segment<uint8_t*>>(new observer_segment<uint8_t>((uint8_t*)_add_req->data().data(),
                                                                                               _add_req->data().length(),
                                                                                               std::move(s)));
            _ret = _notify_add_cbs(ptr, _add_req->db_id());
            _rsp->set_rsp(_ret);
            return grpc::Status::OK; 
        }
        grpc::Status lookup(grpc::ServerContext* _context, const registration_apis::lookup_meta* _lookup_req, registration_apis::db_rsp* _rsp) override
        {
            bool _ret = false;
            auto seg_ptr = notify_lookup_cbs(_lookup_req->file_name().c_str(), _lookup_req->seg_id(), _lookup_req->db_id());

            if (seg_ptr) {
                _rsp->set_data(seg_ptr->get_data(), seg_ptr->get_len());
                _ret = true;
            }

            _rsp->set_rsp(_ret);
            return grpc::Status::OK;
        }
        grpc::Status del(grpc::ServerContext* _context, const registration_apis::lookup_meta* _lookup_req, registration_apis::db_rsp* _rsp) override
        {
            auto _ret = notify_del_cbs(_lookup_req->file_name().c_str(), _lookup_req->seg_id(), _lookup_req->db_id());

            _rsp->set_rsp(_ret);
            return grpc::Status::OK;
        }
};
