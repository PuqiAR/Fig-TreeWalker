#pragma once

#include <cassert>
#include <cstdint>
#include <fstream>
#include <limits>
#include <vector>

namespace Fig::CppLibrary
{
    using FileIDType = uint16_t;
    struct File
    {
        FileIDType id;
        std::fstream *fs;
    };
    class FileManager
    {
    private:
        std::vector<File *> handlers;
        std::vector<FileIDType> free_handlers;

        FileIDType allocated = 0;

    public:
        static constexpr FileIDType MAX_HANDLERS = std::numeric_limits<FileIDType>::max();
        static constexpr unsigned int MAX_FILE_BUF = 961200; // bytes

        FileIDType AllocFile(std::fstream *fs)
        {
            FileIDType id = allocated++;
            File *f = new File{.id = id, .fs = fs};
            handlers.push_back(f);
            return id;
        }

        void CloseFile(FileIDType id)
        {
            assert(id < allocated && "CloseHandler: id out of range");
            File *f = handlers[id];
            if (f == nullptr) { return; }
            f->fs->close();
            delete f->fs;
            delete f;
            free_handlers.push_back(id);
            handlers[id] = nullptr;
        }

        File *GetNextFreeFile()
        {
            // if there is no free handler, create a new one
            if (free_handlers.size() > 0)
            {
                FileIDType id = *free_handlers.begin();
                handlers[id] = new File{.id = id, .fs = new std::fstream};
                free_handlers.erase(free_handlers.begin());
                return handlers[id];
            }
            return handlers[AllocFile(new std::fstream)];
        }

        File *GetFile(FileIDType id)
        {
            assert(id < allocated && "GetFile: id out of range");
            return handlers[id];
        }

        static FileManager &getInstance()
        {
            static FileManager fm;
            return fm;
        }
    };
}; // namespace Fig::CppLibrary