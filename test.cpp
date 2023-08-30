#include <shared_mutex>
int main() 
{
    std::shared_mutex lock;
    lock.unlock_shared();
}
