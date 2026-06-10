#define MAPPER_DLL_EXPORTS

#include "mapper_dll.h"
#include "mapper.h"
#include "file_manager.h"

#include <filesystem>

extern "C" {

    void* CreateMapper(
        void* fileManager,
        const char* temp_dir,
        int mapper_id,
        int reducer_count,
        std::size_t max_buffer_size) {

        if (!fileManager || !temp_dir || reducer_count <= 0) {
            return nullptr;
        }

        FileManager* fm = static_cast<FileManager*>(fileManager);

        return new Mapper(
            *fm,
            std::filesystem::path(temp_dir),
            mapper_id,
            reducer_count,
            max_buffer_size
        );
    }

    void DestroyMapper(void* mapper) {
        delete static_cast<Mapper*>(mapper);
    }

    bool MapperMap(void* mapper, const char* file_name, const char* raw_line) {
        if (!mapper || !file_name || !raw_line) {
            return false;
        }

        Mapper* m = static_cast<Mapper*>(mapper);
        return m->map(file_name, raw_line);
    }

    bool MapperFlush(void* mapper) {
        if (!mapper) {
            return false;
        }

        Mapper* m = static_cast<Mapper*>(mapper);
        return m->flush_buffer();
    }
}