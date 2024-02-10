#pragma once

#include <cassert>
#include <chrono>
#include <cstdint>
#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "grpcpp/client_context.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "grpcpp/server_context.h"
#include "registration_apis.grpc.pb.h"
#include "registration_apis.pb.h"
#include "types.h"
#include <functional>
#include <grpcpp/support/status.h>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include "segment.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#define PORT 50053
static uint cnt = 0;
class RPC_helper {
    protected:
        using add_cb = std::function<void(grpc::Status, registration_apis::db_rsp&)>;
        using async_lookup_cb = std::function<void(grpc::Status, registration_apis::db_rsp*)>;
        using del_cb = std::function<void(grpc::Status, registration_apis::db_rsp&)>;
    public:
        bool register_push_cbs(std::function<bool(db_snapshot_t&, action_t&)>&& _func)
        {
            return _register_push_cbs(std::forward<std::function<bool(db_snapshot_t&, action_t&)>>(_func));
        }
        bool add(std::unique_ptr<segment<char>>&& _segment, const db_snapshot_t& _db, add_cb&& cb_) {
            return _async_add(std::move(_segment), _db, std::move(cb_));
        }
        bool del(std::string_view _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db, del_cb&& cb)
        {
            return _async_del(_file_name, _segment_id, _db, std::move(cb));
        }
        bool lookup(std::string_view _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db, async_lookup_cb&& cb_)
        {
            return _async_lookup(_file_name, _segment_id, _db, std::move(cb_));
        }

    private:
        virtual bool _register_push_cbs(std::function<bool(db_snapshot_t&, action_t&)>&&) = 0;
        virtual bool _add(std::unique_ptr<segment<char>>& _segment, const db_snapshot_t& _db, add_cb&& cb_) = 0;
        virtual bool _async_add(std::unique_ptr<segment<char>>&& _segment, const db_snapshot_t& _db, add_cb&& cb_) = 0;
        virtual bool _del(std::string& _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db) = 0;
        virtual bool _async_del(std::string_view _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db, del_cb&& cb) = 0;
        virtual std::unique_ptr<segment<registration_apis::db_rsp>> _lookup(std::string& _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db) = 0;
        virtual bool _async_lookup(std::string_view _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db, async_lookup_cb&& cb) = 0;
};

class gRPC: public RPC_helper, registration_apis::Registeration::Service {
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
    /* grpc Registration Server APIs
     */
    private:
        grpc::Status register_node_db(grpc::ServerContext* _context,
                                        const registration_apis::request* _request,
                                        registration_apis::response* _response) override
        {
            db_snapshot_t _db_snap{};
#ifdef LOGS
            std::cout << "got req\n";
#endif
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
        bool _register_push_cbs(std::function<bool(db_snapshot_t&, action_t&)>&& _cb) override
        {
            m_registration_cbs.push_back(std::move(_cb));
            return true;
        }
        void wait()
        {
            server_thread.join();
        }

    /* GRPC client add/del/lookup APIs
     */
    public:
        /* Sync APIs
         */
        bool _add(std::unique_ptr<segment<char>>& _segment, const db_snapshot_t& _db, add_cb&& cb_) override
        {
            grpc::ClientContext context;
            std::unique_ptr<registration_apis::add_meta> _add_meta = make_add_meta(_segment->get_id(), _segment->get_len(), _segment->get_file_name(), _segment->get_data(), _db.m_db.m_id);
            registration_apis::db_rsp _rsp{};
            auto stub_ = get_stub(_db.m_node.m_ip_addr);
            assert(stub_);

            grpc::Status status = stub_->add(&context, *_add_meta, &_rsp);
            
            if (status.ok()) {
#ifdef LOGS
                std:: cout << "Added segment ID: " << _segment->get_id() << " dbID: " << _db.m_db.m_id << "\n";
#endif
            } else {
                std::cout << status.error_message() << "\n";
                std:: cout << "Failed Added segment ID: %lu" << _segment->get_id() << " dbID: " << _db.m_db.m_id << "\n";
            }
            return true; 
        }
        bool _del(std::string& _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db) override
        {
            grpc::ClientContext context;
            std::unique_ptr<registration_apis::lookup_meta> _meta = make_lookup_meta(_file_name, _segment_id, _db.m_db.m_id);
            auto _rsp = new registration_apis::db_rsp{};
            auto stub_ = get_stub(_db.m_node.m_ip_addr);
            assert(stub_);

            grpc::Status status = stub_->del(&context, *_meta, _rsp);

            if (status.ok()) {
#ifdef LOGS
                std::cout << "yeah\n";
#endif
            }
            return true; 
        }
        std::unique_ptr<segment<registration_apis::db_rsp>> _lookup(std::string& _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db) override
        {
            grpc::ClientContext context;
            std::unique_ptr<registration_apis::lookup_meta> _meta = make_lookup_meta(_file_name, _segment_id, _db.m_db.m_id);
            auto _rsp = new registration_apis::db_rsp{};
            auto stub_ = get_stub(_db.m_node.m_ip_addr);
            assert(stub_);
            grpc::Status status = stub_->lookup(&context, *_meta, _rsp);

            if (status.ok()) {
#ifdef LOGS
                std::cout << "DBID: " << _db.m_db.m_id << " yeah\n";
#endif
            }
            
            if (_rsp->data().length() == 0) {
                return nullptr;
            }

            return segment<registration_apis::db_rsp>::create_segment(_rsp, _rsp->data().length(), _file_name);
        }

        /* Client Async APIs */
        using add_cb = typename RPC_helper::add_cb;
        bool _async_add(std::unique_ptr<segment<char>>&& _segment, const db_snapshot_t& _db, add_cb&& cb_) override
        {
            /* Maintain state throughout the lifetime of this request
             */
            class Impl {
                public:
                    Impl(gRPC& grpc_, std::unique_ptr<segment<char>>&& _segment, const db_snapshot_t& _db, add_cb&& cb_, uint8_t max_retry_cnt = 3)
                        :m_segment(std::move(_segment)), m_cb(std::move(cb_)), m_stub(grpc_.get_stub(_db.m_node.m_ip_addr)), m_max_retry_cnt(max_retry_cnt)
                    {
                        assert(m_stub);
                        _add_meta = grpc_.make_add_meta(m_segment->get_id(),
                                m_segment->get_len(),
                                m_segment->get_file_name(),
                                m_segment->get_data(),
                                _db.m_db.m_id);
                        send_add_request();
                        //std:: cout << "Adding segment ID: " << m_segment->get_id() << " dbID: " << m_db.m_db.m_id << "\n";
                    }
                private:
                    inline void send_add_request()
                    {
                        ctx_p = std::unique_ptr<grpc::ClientContext>(new grpc::ClientContext{});
                        m_stub->async()->add(ctx_p.get(), _add_meta.get(), &m_rsp,
                                [this](grpc::Status status_) {
                                        if (status_.ok() && m_rsp.rsp()) {
                                            m_cb(status_, m_rsp);
                                        } else {
                                            if (retry_cnt < m_max_retry_cnt) {
                                                retry_cnt++;
                                                std::cout << "Addition of segment " << m_segment->get_id() << " failed. Retrying...\n";
                                                std::this_thread::sleep_for(std::chrono::seconds(1));
                                                send_add_request();
                                            } else {
                                                std::cout << "Addition of segment " << m_segment->get_id() << " failed.\n";
                                                m_cb(status_, m_rsp);
                                            }
                                        }
                                });

                    }

                private:
                    std::unique_ptr<grpc::ClientContext> ctx_p{};
                    std::unique_ptr<segment<char>> m_segment;
                    add_cb m_cb;
                    registration_apis::db_rsp m_rsp{};
                    registration_apis::db_update::Stub* m_stub; /* Not owned */
                    std::unique_ptr<registration_apis::add_meta> _add_meta{};
                    uint8_t retry_cnt{};
                    uint8_t m_max_retry_cnt;
            };

            new Impl{*this, std::forward<std::unique_ptr<segment<char>>>(_segment), _db, std::forward<add_cb>(cb_)};
            return true;
        }

        using async_lookup_cb = typename RPC_helper::async_lookup_cb;
        bool _async_lookup(std::string_view _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db, async_lookup_cb&& cb_) override
        {
            class Impl {
                public:
                    Impl(gRPC& grpc, std::string_view _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db, async_lookup_cb&& cb_, uint8_t max_retry_cnt = 3)
                        :m_cb(std::move(cb_)), m_rsp(new registration_apis::db_rsp()), m_stub(grpc.get_stub(_db.m_node.m_ip_addr)), m_max_retry_cnt(max_retry_cnt)
                    {
                        assert(m_stub);
                        m_meta = grpc.make_lookup_meta(_file_name, _segment_id, _db.m_db.m_id);
                        send_lookup_request();
                    }
                private:
                    inline void send_lookup_request()
                    {
                        ctx_p = std::unique_ptr<grpc::ClientContext>(new grpc::ClientContext{});
                        m_stub->async()->lookup(ctx_p.get(), m_meta.get(), m_rsp, 
                                [this](grpc::Status status) {
                                    if (status.ok() && m_rsp->rsp()) {
#ifdef LOGS
                                        std::cout << "got Lookup rsp\n";
#endif
                                        m_cb(status, m_rsp);
                                    } else {
                                        if (retry_cnt < m_max_retry_cnt) {
                                            retry_cnt++;
                                            std::cout << "Lookup of segment " << m_meta->seg_id() << " failed. Retrying...\n";
                                            std::this_thread::sleep_for(std::chrono::seconds(1));
                                            send_lookup_request();
                                        } else {
                                            std::cout << "Lookup of segment " << m_meta->seg_id() << " failed.\n";
                                            m_cb(status, m_rsp);
                                        }
                                    }
                                });
                    }

                private:
                    std::unique_ptr<grpc::ClientContext> ctx_p{};
                    async_lookup_cb m_cb;
                    registration_apis::db_rsp* m_rsp;
                    registration_apis::db_update::Stub* m_stub; /* Not owned */
                    std::unique_ptr<registration_apis::lookup_meta> m_meta{};
                    uint8_t retry_cnt{};
                    uint8_t m_max_retry_cnt;
            };

            new Impl{*this, _file_name, _segment_id, _db, std::move(cb_)};
            return true;
        }

        using async_del_cb = typename RPC_helper::del_cb;
        bool _async_del(std::string_view _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db, async_del_cb&& cb) override
        {
            class Impl {
                public:
                    Impl(gRPC& grpc, std::string_view _file_name, pType::segment_ID _segment_id, const db_snapshot_t& _db, async_del_cb&& cb_, uint8_t max_retry_cnt = 3)
                        :m_cb(std::move(cb_)), m_stub(grpc.get_stub(_db.m_node.m_ip_addr)), m_max_retry_cnt(max_retry_cnt)
                    {
                        assert(m_stub);
                        m_meta = grpc.make_lookup_meta(_file_name, _segment_id, _db.m_db.m_id);
                        send_del_request();
                    }
                private:
                    inline void send_del_request()
                    {
                        ctx_p = std::unique_ptr<grpc::ClientContext>(new grpc::ClientContext{});
                        m_stub->async()->del(ctx_p.get(), m_meta.get(), &m_rsp, 
                                [this](grpc::Status status) {
                                if (status.ok() && m_rsp.rsp()) {
                                    m_cb(status, m_rsp);
                                } else {
                                    if (retry_cnt < m_max_retry_cnt) {
                                        retry_cnt++;
                                        std::cout << "Delete of segment " << m_meta->seg_id() << " failed. Retrying...\n";
                                        std::this_thread::sleep_for(std::chrono::seconds(1));
                                        send_del_request();
                                    } else {
                                        std::cout << "Delete of segment " << m_meta->seg_id() << " failed.\n";
                                        m_cb(status, m_rsp);
                                    }
                                }
                                });
                    }

                private:
                    std::unique_ptr<grpc::ClientContext> ctx_p;
                    async_del_cb m_cb;
                    registration_apis::db_rsp m_rsp{};
                    registration_apis::db_update::Stub* m_stub; /* Not owned */
                    std::unique_ptr<registration_apis::lookup_meta> m_meta{};
                    uint8_t retry_cnt{};
                    uint8_t m_max_retry_cnt;
            };

            new Impl{*this, _file_name, _segment_id, _db, std::move(cb)};
            return true;
        }
    
    private:
        /* Helper functions
         */
        std::unique_ptr<registration_apis::add_meta> make_add_meta(uint32_t _id, uint32_t _size, std::string_view _file_name, void const* _data, uint32_t _db_ID)
        {
            std::unique_ptr<registration_apis::add_meta> ptr = std::make_unique<registration_apis::add_meta>();

            ptr->set_id(_id);
            ptr->set_data_size(_size);
            ptr->set_file_name(_file_name);
            ptr->set_data(_data, _size);
            ptr->set_db_id(_db_ID);

            return ptr;
        }
        
        std::unique_ptr<registration_apis::lookup_meta> make_lookup_meta(std::string_view _file_name, uint32_t _id, uint32_t _db_ID)
        {
            std::unique_ptr<registration_apis::lookup_meta> ptr = std::make_unique<registration_apis::lookup_meta>();

            ptr->set_file_name(_file_name);
            ptr->set_seg_id(_id);
            ptr->set_db_id(_db_ID);

            return ptr;
        }

        registration_apis::db_update::Stub* get_stub(uint32_t ip_)
        {
            auto iter = m_stubs_map.find(ip_);
            std::string channel_string_;
            char string_ip[INET_ADDRSTRLEN];
            if (iter == m_stubs_map.end()) {
                /* FIXME: frame string better way */
                inet_ntop(AF_INET, &ip_, string_ip, INET_ADDRSTRLEN);
                std::string temp(string_ip);
                channel_string_ = temp + ":" + std::to_string(PORT);
#ifdef LOGS
                std::cout << "Channel string: " << channel_string_ << "\n";
#endif
                auto channel = grpc::CreateChannel(channel_string_, grpc::InsecureChannelCredentials());
                auto stub = registration_apis::db_update::NewStub(channel);

                m_stubs_map[ip_] = std::move(stub);
            }

            return m_stubs_map[ip_].get();
        }
    private:
        std::vector<std::function<bool(db_snapshot_t&, action_t&)>> m_registration_cbs;
        std::thread server_thread;
        std::unordered_map<uint32_t, std::unique_ptr<registration_apis::db_update::Stub>> m_stubs_map;

};
