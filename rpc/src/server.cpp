#include "grpcpp/server_context.h"
#include "registration_apis.pb.h"
#include "registration_apis.grpc.pb.h"
#include <grpcpp/support/status.h>

using registration_apis::Registeration;

class Impl: public Registeration::Service {
    public:
        grpc::Status register_node_db(grpc::ServerContext* _context, const registration_apis::request* _request, registration_apis::response* _response) override
        {
            return grpc::Status::OK;
        }
};
