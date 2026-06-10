// reducer_dll.cpp

#include "reducer_dll.h"
#include "reducer.h"
#include "file_manager.h"

#include <filesystem>
#include <vector>
#include <string>

extern "C" {

    // =========================================================
    // Create Reducer
    // =========================================================
    void* CreateReducer(
        void* fileManager,
        const char* output_directory,
        const char* output_file_name,
        std::size_t max_buffer_size) {

        if (!fileManager || !output_directory || !output_file_name) {
            return nullptr;
        }

        FileManager* fm = static_cast<FileManager*>(fileManager);

        Reducer* reducer = new Reducer(
            *fm,
            std::filesystem::path(output_directory),
            std::string(output_file_name),
            max_buffer_size
        );

        return static_cast<void*>(reducer);
    }

    // =========================================================
    // Destroy Reducer
    // =========================================================
    void DestroyReducer(void* reducer) {
        if (!reducer) {
            return;
        }

        Reducer* reducerPtr = static_cast<Reducer*>(reducer);
        delete reducerPtr;
    }

    // =========================================================
    // Reduce wrapper
    // =========================================================
    bool ReducerReduce(
        void* reducer,
        const char* key,
        const int* values,
        std::size_t value_count) {

        if (!reducer || !key || !values) {
            return false;
        }

        Reducer* reducerPtr = static_cast<Reducer*>(reducer);

        std::vector<int> valueVector(values, values + value_count);

        return reducerPtr->reduce(
            std::string(key),
            valueVector
        );
    }

    // =========================================================
    // Finish wrapper
    // =========================================================
    bool ReducerFinish(void* reducer) {
        if (!reducer) {
            return false;
        }

        Reducer* reducerPtr = static_cast<Reducer*>(reducer);

        return reducerPtr->finish();
    }
}