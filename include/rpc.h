#pragma once

#include <cstdint>
#include <functional>
#include <grpcpp/support/status.h>
#include <memory>
#include <vector>
#include "grpcpp/server_builder.h"
#include "grpcpp/server_context.h"
#include "registration_apis.grpc.pb.h"
#include "registration_apis.pb.h"
#include "segment.h"
#include <thread>
class RPC_helper {
    public:
        uint32_t register_db(uint32_t _id)
        {
            return _register_db(_id);
        }
        uint32_t register_node()
        {
            return _register_node();
        }
        /* Callbacks to be invoked on receiving add/del/lookup notifs
         */
        bool register_cbs();
        virtual void wait(){}
    private:
        virtual uint32_t _register_db(uint32_t) = 0;
        virtual uint32_t _register_node() = 0;
        virtual bool _register_cb(std::function<bool(std::shared_ptr<segment>)>&& _cb) = 0;
};

class gRPC: public RPC_helper, registration_apis::Db_update::Service {
    private:
        std::unique_ptr<registration_apis::Registeration::Stub> stub_;
        std::vector<std::function<bool(std::shared_ptr<segment>)>> m_cbs;
        std::thread server_thread;
        
    public:
        gRPC(std::shared_ptr<grpc::ChannelInterface> channel)
            :stub_(registration_apis::Registeration::NewStub(channel)), m_cbs(), server_thread(&gRPC::start_server, this)
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
            std::unique_ptr<registration_apis::request> req = make_req(5, 0, registration_apis::REG_NODE);
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
            std::string server_address("0.0.0.0:50052");

            grpc::ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(this);
            std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
            std::cout << "Server listening on " << server_address << std::endl;
            server->Wait();
        }

    private:
        bool _register_cb(std::function<bool(std::shared_ptr<segment>)>&& _cb) override
        {
            m_cbs.push_back(_cb);
            return true;
        }
        bool notify_cbs(std::shared_ptr<segment> _segment)
        {
            bool _ret = true;
            std::cout << "triggered all db cbs\n";
            
            for (auto& _func: m_cbs) {
                _ret &= _func(_segment);
            }

            return _ret;
        }
        grpc::Status add(grpc::ServerContext* _context, const registration_apis::add_meta* _add_req, registration_apis::db_rsp* _rsp) override
        {
            bool _ret;
            _ret = notify_cbs(segment::create_segment(_add_req->data().data(), _add_req->data().length(), _add_req->file_name().c_str()));
            _rsp->set_rsp(_ret);
            return grpc::Status::OK; 
        }
};
