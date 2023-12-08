#pragma once

#include "registration_apis.grpc.pb.h"
#include "registration_apis.pb.h"
#include "types.h"
#include <functional>
#include <vector>
class RPC_helper {
   public:
        bool register_push_cbs(std::function<bool(db_snapshot_t&, action_t&)>&&);
    private:
        virtual bool _register_push_cbs(std::function<bool(db_snapshot_t&, action_t&)>&&) = 0;
};

class gRPC: public RPC_helper, registration_apis::Register::Service {
    private:
        std::vector<std::function<bool(db_snapshot_t&, action_t&)>> m_registration_cbs;

    private:
      reg_rsp reg_node()  
    public:
        bool _register_push_cbs(std::function<bool(db_snapshot_t&, action_t&)>&&);
};
