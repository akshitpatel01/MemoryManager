#include "central_manager.h"
#include "rpc.h"
int main()
{
    gRPC m_rpc;
    Central_manager m_central_manager{m_rpc, true};

    m_rpc.wait();
}
