#pragma once

class blob_store {
    private:
    public:
        bool insert();
        bool remove();
        void* lookup();
};
