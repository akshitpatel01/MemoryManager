#pragma once

#include "segment.h"
#include <memory>
#include <vector>

class Blob_manager {
    using segment = segment<char>;

    public:
       bool add(const char* _file_path);

       bool remove(const char* _file_path);

       std::vector<std::unique_ptr<segment>> get(const char* _file_path);
};
