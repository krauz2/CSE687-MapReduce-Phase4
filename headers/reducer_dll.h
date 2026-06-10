// reducer_dll.h
#pragma once

#include <cstddef>

#ifdef _WIN32
    #ifdef REDUCER_DLL_EXPORTS
        #define REDUCER_API __declspec(dllexport)
    #else
        #define REDUCER_API __declspec(dllimport)
    #endif
#else
    #define REDUCER_API
#endif

extern "C" {

    // Create a Reducer instance
    REDUCER_API void* CreateReducer(
        void* fileManager,
        const char* output_directory,
        const char* output_file_name,
        std::size_t max_buffer_size
    );

    // Destroy Reducer instance
    REDUCER_API void DestroyReducer(void* reducer);

    // Run reduce()
    REDUCER_API bool ReducerReduce(
        void* reducer,
        const char* key,
        const int* values,
        std::size_t value_count
    );

    // Run finish()
    REDUCER_API bool ReducerFinish(void* reducer);
}