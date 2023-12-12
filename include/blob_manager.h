#pragma once

#include "segment.h"
#include <memory>
#include <vector>

class Blob_manager {
    using segment_t = segment<char>;

    public:
       bool add(const char* _file_path);

       bool remove(const char* _file_path);

       std::vector<std::unique_ptr<segment_t>> get(const char* _file_path);
};
