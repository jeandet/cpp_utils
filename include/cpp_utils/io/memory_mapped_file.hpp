/*------------------------------------------------------------------------------
-- The MIT License (MIT)
--
-- Copyright © 2024, Laboratory of Plasma Physics- CNRS
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the “Software”), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
-- of the Software, and to permit persons to whom the Software is furnished to do
-- so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
-- INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
-- PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
-- HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
-- OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
-- SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------------*/
/*-- Author : Alexis Jeandet
-- Mail : alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/
#pragma once

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <string>
#include <type_traits>
#if __has_include(<sys/mman.h>)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#define USE_MMAP
#endif
#if __has_include(<windows.h>)
#define NOMINMAX
#include <windows.h>
#define USE_MapViewOfFile
#endif

namespace cpp_utils::io
{

struct memory_mapped_file
{
#ifdef USE_MMAP
    int fd = -1;
#endif
    char* mapped_file = nullptr;
    std::size_t f_size = 0UL;
    using implements_view = std::true_type;
#ifdef USE_MapViewOfFile
    HANDLE hMapFile = NULL;
    HANDLE hFile = NULL;
#endif

    memory_mapped_file(const std::string& path)
    {

        if (std::filesystem::exists(path))
        {
            this->f_size = std::filesystem::file_size(path);
            if (this->f_size)
            {
#ifdef USE_MMAP
                if (fd = open(path.c_str(), O_RDONLY, static_cast<mode_t>(0600)); fd != -1)
                {

                    {
                        mapped_file = static_cast<char*>(mmap(
                            nullptr, this->f_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0UL));
                    }
                }
#endif
#ifdef USE_MapViewOfFile
                hFile = ::CreateFile(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    hMapFile = ::CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
                    if (hMapFile != NULL)
                    {
                        mapped_file = static_cast<char*>(
                            ::MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, this->f_size));
                    }
                }
#endif
            }
        }
    }
    ~memory_mapped_file()
    {
        if (mapped_file)
        {
#ifdef USE_MMAP
            munmap(mapped_file, f_size);
            close(fd);
#endif
#ifdef USE_MapViewOfFile
            ::UnmapViewOfFile(mapped_file);
            ::CloseHandle(hFile);
            ::CloseHandle(hMapFile);
#endif
        }
    }

    auto read(const std::size_t offset, const std::size_t) { return mapped_file + offset; }

    template <std::size_t size>
    auto read(const std::size_t offset)
    {
        return mapped_file + offset;
    }

    void read(char* dest, const std::size_t offset, const std::size_t size)
    {
        std::memcpy(dest, mapped_file + offset, size);
    }

    template <std::size_t size>
    auto view(const std::size_t offset) const
    {
        return mapped_file + offset;
    }

    auto view(const std::size_t offset) const { return mapped_file + offset; }

    auto size() const { return this->f_size; }
    auto data() const { return view(0); }

    bool is_valid() const
    {
#ifdef USE_MMAP
        return fd != -1 && mapped_file != nullptr;
#endif
#ifdef USE_MapViewOfFile
        return hFile != NULL && hMapFile != NULL && mapped_file != nullptr;
#endif
    }
};

}

namespace std
{
std::size_t size(const cpp_utils::io::memory_mapped_file& arr)
{
    return arr.size();
}

char* data(cpp_utils::io::memory_mapped_file& arr)
{
    return arr.view(0);
}
}
