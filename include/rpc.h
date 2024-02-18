#pragma once

#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <grpcpp/support/status.h>
#include <ifaddrs.h>
#include <memory>
#include <mutex>
#include <sys/types.h>
#include "grpcpp/server_builder.h"
#include "grpcpp/server_context.h"
#include "grpcpp/support/server_callback.h"
#include "registration_apis.grpc.pb.h"
#include "registration_apis.pb.h"
#include "segment.h"
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>

        static std::atomic<uint> cnt = 0;
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

        if(strcmp(ifc_name.c_str(), ifa->ifa_name) == 0 && ifa->ifa_addr->sa_family == IPV4)
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
        class CallbackServiceImpl: public registration_apis::db_update::CallbackService {
            public:
                CallbackServiceImpl(gRPC &owner)
                    :owner_(owner)
                {}

#if 0
                grpc::ServerUnaryReactor* add(grpc::CallbackServerContext* ctx, const registration_apis::add_meta* _add_req, registration_apis::db_rsp* rsp) override
                {
                        auto* reactor = ctx->DefaultReactor();
                        auto f = std::async(std::launch::async, [reactor, _add_req, rsp, this]()
                                {
                                    bool _ret = false;
                                    std::string s{_add_req->file_name()};
                                    const auto ptr = std::unique_ptr<segment<uint8_t*>>(new observer_segment<uint8_t>((uint8_t*)_add_req->data().data(),
                                                _add_req->data().length(),
                                                std::move(s)));
                                    std::cout << "ID1: " << _add_req->id() << "\n";
                                    _ret = owner_._notify_add_cbs(ptr, _add_req->db_id());
                                    //std::cout << "Ret: " << _ret << "\n";
                                    std::cout << "ID4: " << _add_req->id() << "\n";
                                    rsp->set_rsp(_ret);

                                    reactor->Finish(grpc::Status::OK);
                                });
                        owner_.add_fut(std::move(f), _add_req->id());
                    return reactor;
                }
#endif
                grpc::ServerBidiReactor<registration_apis::add_meta, registration_apis::db_rsp>* add(grpc::CallbackServerContext* ctx) override
                {
                    class Impl: public grpc::ServerBidiReactor<registration_apis::add_meta,
                                                               registration_apis::db_rsp> {
                        public:
                            Impl(gRPC& owner)
                                :m_owner(owner)
                            {
                                read_internal();
                            }
                            void OnDone() override
                            {
                                delete this;
                            }
                            
                            void OnReadDone(bool ok) override
                            {
                                if (ok) {
                                    std::string s{m_req.file_name()};
                                    const auto ptr = std::unique_ptr<segment<uint8_t*>>(new observer_segment<uint8_t>((uint8_t*)m_req.data().data(),
                                                m_req.data().length(),
                                                std::move(s), m_req.id()));
                                    //std::cout << "ID1: " << _add_req->id() << "\n";
                                    m_owner._notify_add_cbs(ptr, m_req.db_id());
                                    //std::cout << "Ret: " << _ret << "\n";
                                    //std::cout << "ID4: " << _add_req->id() << "\n";
                                    write_internal(true, m_req.id());
                                    read_internal();
                                    return;
                                }

                                m_rsp.set_rsp(true);
                                std::cout << "Finish\n";
                                Finish(grpc::Status::OK);
                            }
                            void OnWriteDone(bool ok) override
                            {
                                if (ok) {
                                } else {
                                    std::cout << "Failed\n";
                                }
                            }
                        private:
                            void write_internal(bool ok, uint32_t idx)
                            {
                                m_rsp.Clear();
                                m_rsp.set_rsp(ok);
                                m_rsp.set_id(idx);
                                StartWrite(&m_rsp);
                            }
                            void read_internal()
                            {
                                m_req.Clear();
                                StartRead(&m_req);
                            }
                        private:
                            gRPC& m_owner;
                            registration_apis::add_meta m_req;
                            registration_apis::db_rsp m_rsp;
                
                    };
                    return new Impl{owner_};

                }
                grpc::ServerUnaryReactor* lookup(grpc::CallbackServerContext* ctx, const registration_apis::lookup_meta* _lookup_req, registration_apis::db_rsp* _rsp) override
                {
                    bool _ret = false;
                    auto seg_ptr = owner_.notify_lookup_cbs(_lookup_req->file_name().c_str(), _lookup_req->seg_id(), _lookup_req->db_id());

                    if (seg_ptr) {
                        _rsp->set_data(seg_ptr->get_data(), seg_ptr->get_len());
                        _ret = true;
                    }

                    _rsp->set_rsp(_ret);

                    auto* reactor = ctx->DefaultReactor();
                    reactor->Finish(grpc::Status::OK);
                    return reactor;
                }
                
                grpc::ServerUnaryReactor* del(grpc::CallbackServerContext* ctx, const registration_apis::lookup_meta* _lookup_req, registration_apis::db_rsp* _rsp) override
                {
                    auto _ret = owner_.notify_del_cbs(_lookup_req->file_name().c_str(), _lookup_req->seg_id(), _lookup_req->db_id());

                    _rsp->set_rsp(_ret);

                    auto* reactor = ctx->DefaultReactor();
                    reactor->Finish(grpc::Status::OK);
                    return reactor;
                }
            private:
                gRPC &owner_;

        };
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
        std::unique_ptr<CallbackServiceImpl> service_;
        std::vector<std::future<void>> m_fut;
        std::mutex m_lock;

    public:
        void add_fut(std::future<void>&& fut, uint idx)
        {
            //std::scoped_lock<std::mutex> lock_(m_lock);
            m_fut[idx] = (std::move(fut));
        }
        
    public:
        gRPC(std::shared_ptr<grpc::ChannelInterface> channel, add_cb&& _add_cb, lookup_cb&& _lookup_cb, del_cb&& _del_cb)
            :stub_(registration_apis::Registeration::NewStub(channel)),
             m_add_cb(std::move(_add_cb)),
             m_lookup_cb(std::move(_lookup_cb)),
             m_del_cb(std::move(_del_cb)),
             server_thread(&gRPC::start_server, this),
             m_fut{10487}
        {
        }
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
#ifdef LOGS
                std::cout << rsp.id() << " Got it!!!\n";
#endif
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
            std::string server_address("192.168.1.201:50053");

            grpc::ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            //builder.RegisterService(this);
            service_ = std::make_unique<CallbackServiceImpl>(*this);
            builder.RegisterService(service_.get());
            std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
            std::cout << "Server listening on " << server_address << std::endl;
            server->Wait();
        }

    private:
        bool _notify_add_cbs(const std::unique_ptr<segment_t>& _segment, uint32_t _id)
        {
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
#if 0
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
#endif
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
