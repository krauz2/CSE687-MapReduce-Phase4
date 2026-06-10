#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifdef MAPPER_DLL_EXPORTS
#define MAPPER_API __declspec(dllexport)
#else
#define MAPPER_API __declspec(dllimport)
#endif
#else
#define MAPPER_API
#endif

#include <cstddef>

extern "C" {

    MAPPER_API void* CreateMapper(
        void* fileManager,
        const char* temp_dir,
        int mapper_id,
        int reducer_count,
        std::size_t max_buffer_size
    );

    MAPPER_API void DestroyMapper(void* mapper);

    MAPPER_API bool MapperMap(
        void* mapper,
        const char* file_name,
        const char* raw_line
    );

    MAPPER_API bool MapperFlush(void* mapper);
}