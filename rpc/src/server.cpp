#include "grpcpp/server_context.h"
#include "registration_apis.pb.h"
#include "registration_apis.grpc.pb.h"
#include <grpcpp/support/status.h>

using registration_apis::Register;

class Impl: public Register::Service {
    public:
        grpc::Status reg_node (grpc::ServerContext* _context, const registration_apis::node_reg* _reg_struct, registration_apis::node_reg_rsp* _ret_struct)
        {
            return grpc::Status::OK;
        }
};
