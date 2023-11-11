#pragma once
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <ratio>
#include <string>
#include <chrono>
#include <ctime>

#define PROFILE_SCOPE() Scope_profiler profiler(__FUNCTION__)

class Scope_profiler {
    private:
        std::string cur_function;
        std::chrono::time_point<std::chrono::high_resolution_clock> start_timestamp;
        std::chrono::time_point<std::chrono::high_resolution_clock> end_timestamp;

    public:
        Scope_profiler(std::string function)
            :cur_function(function), start_timestamp(std::chrono::high_resolution_clock::now())
        {
        }
        ~Scope_profiler()
        {
            end_timestamp = std::chrono::high_resolution_clock::now();
            auto start = std::chrono::time_point_cast<std::chrono::microseconds>(start_timestamp).time_since_epoch();
            auto end = std::chrono::time_point_cast<std::chrono::microseconds>(end_timestamp).time_since_epoch();


            auto duration = end - start;
            double ms = duration.count();
            
            std::cout << "Time taken by " << cur_function << " is: ";
            std::cout << ms << std::endl;
        }
};
