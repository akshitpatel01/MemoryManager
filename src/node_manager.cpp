#include "node_manager.h"
#include <filesystem>
#include <string>

bool
node_manager::m_file_hash_lookup(void* _file1, void *_file2)
{
    std::string* __file1 = static_cast<std::string*>(_file1);
    std::string* __file2 = static_cast<std::string*>(_file2);

    return (*__file1 == *__file2);
}


node_manager::node_manager()
    :m_file_hash(Hash<Hash_linear, std::string, std::string>(node_manager::m_file_hash_lookup))
{
    std::filesystem::path t = std::filesystem::current_path();
}

std::string node_manager::test()
{
    return "Test success!!\n";
}

node_manager::~node_manager()
{
    std::cout << "node manager destroyed\n";
}
