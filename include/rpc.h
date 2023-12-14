#pragma once

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "grpcpp/server_context.h"
#include "registration_apis.grpc.pb.h"
#include "registration_apis.pb.h"
#include "types.h"
#include <functional>
#include <grpcpp/support/status.h>
#include <thread>
#include <utility>
#include <vector>

class RPC_helper {
   public:
        bool register_push_cbs(std::function<bool(db_snapshot_t&, action_t&)>&& _func)
        {
            return _register_push_cbs(std::forward<std::function<bool(db_snapshot_t&, action_t&)>>(_func));
        }
    private:
        virtual bool _register_push_cbs(std::function<bool(db_snapshot_t&, action_t&)>&&) = 0;
};

class gRPC: public RPC_helper, registration_apis::Registeration::Service {
    private:
        std::vector<std::function<bool(db_snapshot_t&, action_t&)>> m_registration_cbs;
        std::thread server_thread;

    private:
        grpc::Status register_node_db(grpc::ServerContext* _context, const registration_apis::request* _request, registration_apis::response* _response) override
        {
            db_snapshot_t _db_snap;
            std::cout << "got req\n";
            bool _ret = false;

            switch(_request->action()) {
                case registration_apis::REG_NODE:
                    _db_snap.m_node.m_ip_addr = _request->ip();
                    _ret = _call_reg_cbs(_db_snap, NODE_ADD);
                    _response->set_id(_db_snap.m_node.m_id);
                    break;
                case registration_apis::REG_DB:
                    _db_snap.m_node.m_id = _request->node_id();
                    _ret = _call_reg_cbs(_db_snap, DB_ADD);
                    _response->set_id(_db_snap.m_db.m_id);
                    break;
                case registration_apis::REM_NODE:
                    _db_snap.m_node.m_id = _request->node_id();
                    _ret = _call_reg_cbs(_db_snap, NODE_REM);
                    _response->set_rsp(_ret);
                    break;
                case registration_apis::REM_DB:
                    _db_snap.m_db.m_id = _request->node_id();
                    _ret = _call_reg_cbs(_db_snap, DB_REM);
                    _response->set_rsp(_ret);
                    break;
                default:
                    std::cout << "Invlaid states\n";
            }

            if (!_ret) {
                std::cout << "Client req errored\n";
            }

            return grpc::Status::OK;
        }
        bool _call_reg_cbs(db_snapshot_t& _db, action_t _action)
        {
            bool _ret = true;

            for (auto& func: m_registration_cbs) {
                _ret &= func(_db, _action);
            }

            return _ret;
        }
        void start_server()
        {
            std::string server_address("0.0.0.0:50051");

            grpc::ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(this);
            std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
            std::cout << "Server listening on " << server_address << std::endl;
            server->Wait();
        }
    public:
        gRPC()
            :server_thread(&gRPC::start_server, this)
        {
            m_registration_cbs.reserve(5);
        }
        ~gRPC()
        {
            std::cout << "Grpc destroyed\n";
        }
        bool _register_push_cbs(std::function<bool(db_snapshot_t&, action_t&)>&& _cb) override
        {
            m_registration_cbs.push_back(std::move(_cb));
            return true;
        }
        void wait()
        {
            server_thread.join();
        }
};