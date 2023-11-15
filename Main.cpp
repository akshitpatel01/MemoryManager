#include "blob_manager.h"
#include <cstdint>
#include <iostream>
int main()
{
    Blob_manager bm;
    uint32_t node_id = bm.register_node();
    uint32_t db_id;
    db_id = bm.add_node_db(node_id);
    std::cout << db_id << "\n";
    db_id = bm.add_node_db(node_id);
    std::cout << db_id << "\n";
    db_id = bm.add_node_db(node_id);
    std::cout << db_id << "\n";
    db_id = bm.add_node_db(node_id);
    std::cout << db_id << "\n";

    bm.add("file.txt", nullptr);        

    bm.lookup("file.txt");
}
