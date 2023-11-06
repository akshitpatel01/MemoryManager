#include <cstring>
#include <filesystem>
#include <shared_mutex>
#include <sys/mount.h>
#include <iostream>
int main(int argc, char** argv) 
{
    if (argc != 3) {
        std::cout << ""
    }
    const char *dstPath = "/var/lib/mongodb/test3_db";
    const char *srcPath = "/media/akshit/MemoryManag/test3_db";
    std::string fstype = "vfat";
    /*std::filesystem::create_directory(dstPath);
    if (mount(srcPath, dstPath, fstype.c_str(), MS_BIND, "username=mongodb,password=mongodb") == -1) {
        std::cout << "mount failed " << strerror(errno) << std::endl; 
    } else {
        std::cout << "mount success\n";
    }*/
}
