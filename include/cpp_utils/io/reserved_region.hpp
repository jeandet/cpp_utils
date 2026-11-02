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
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#if __has_include(<windows.h>)
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#define CPP_UTILS_REGION_WIN32
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace cpp_utils::io
{

enum class region_backing
{
    memory,
    file
};

namespace details::region
{
    [[noreturn]] inline void throw_last_error(const char* what)
    {
#ifdef CPP_UTILS_REGION_WIN32
        throw std::system_error { static_cast<int>(::GetLastError()), std::system_category(), what };
#else
        throw std::system_error { errno, std::generic_category(), what };
#endif
    }

    inline std::filesystem::path env_path(const char* name)
    {
        const char* value = std::getenv(name);
        return value ? std::filesystem::path { value } : std::filesystem::path {};
    }

    inline std::filesystem::path user_cache_dir()
    {
#if defined(CPP_UTILS_REGION_WIN32)
        return env_path("LOCALAPPDATA");
#elif defined(__APPLE__)
        return env_path("HOME") / "Library" / "Caches";
#else
        if (auto xdg = env_path("XDG_CACHE_HOME"); !xdg.empty())
            return xdg;
        return env_path("HOME") / ".cache";
#endif
    }

    inline std::filesystem::path resolve_temp_dir(const std::filesystem::path& requested)
    {
        auto dir = requested.empty() ? user_cache_dir() : requested;
        if (dir.empty())
            throw std::runtime_error { "no directory for a file-backed region" };
        std::filesystem::create_directories(dir);
        return dir;
    }

#ifdef CPP_UTILS_REGION_WIN32
    struct mapping
    {
        char* data = nullptr;
        HANDLE file = INVALID_HANDLE_VALUE;
        HANDLE section = nullptr;
    };

    inline mapping reserve_memory(std::size_t length)
    {
        auto* p = ::VirtualAlloc(nullptr, length, MEM_RESERVE, PAGE_NOACCESS);
        if (!p)
            throw_last_error("VirtualAlloc(MEM_RESERVE)");
        return { static_cast<char*>(p) };
    }

    inline HANDLE create_sparse_temp_file(const std::filesystem::path& dir, std::size_t length)
    {
        wchar_t name[MAX_PATH];
        if (!::GetTempFileNameW(dir.c_str(), L"cur", 0, name))
            throw_last_error("GetTempFileNameW");
        HANDLE file = ::CreateFileW(name, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            throw_last_error("CreateFileW");
        DWORD returned = 0;
        ::DeviceIoControl(file, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &returned, nullptr);
        LARGE_INTEGER size;
        size.QuadPart = static_cast<LONGLONG>(length);
        if (!::SetFilePointerEx(file, size, nullptr, FILE_BEGIN) || !::SetEndOfFile(file))
        {
            ::CloseHandle(file);
            throw_last_error("SetEndOfFile");
        }
        return file;
    }

    inline mapping reserve_file(std::size_t length, const std::filesystem::path& dir)
    {
        mapping m;
        m.file = create_sparse_temp_file(dir, length);
        m.section = ::CreateFileMappingW(m.file, nullptr, PAGE_READWRITE, 0, 0, nullptr);
        if (m.section)
            m.data = static_cast<char*>(::MapViewOfFile(m.section, FILE_MAP_WRITE, 0, 0, length));
        DWORD old = 0;
        if (!m.data || !::VirtualProtect(m.data, length, PAGE_NOACCESS, &old))
        {
            const auto error = ::GetLastError();
            if (m.data)
                ::UnmapViewOfFile(m.data);
            if (m.section)
                ::CloseHandle(m.section);
            ::CloseHandle(m.file);
            ::SetLastError(error);
            throw_last_error("MapViewOfFile");
        }
        return m;
    }

    inline void release(mapping& m, std::size_t)
    {
        if (m.section)
        {
            ::UnmapViewOfFile(m.data);
            ::CloseHandle(m.section);
            ::CloseHandle(m.file);
        }
        else
            ::VirtualFree(m.data, 0, MEM_RELEASE);
    }

    inline void protect(char* p, std::size_t length, DWORD protection)
    {
        DWORD old = 0;
        if (!::VirtualProtect(p, length, protection, &old))
            throw_last_error("VirtualProtect");
    }

    inline void make_writable(const mapping& m, char* p, std::size_t length)
    {
        if (!m.section)
        {
            if (!::VirtualAlloc(p, length, MEM_COMMIT, PAGE_READWRITE))
                throw_last_error("VirtualAlloc(MEM_COMMIT)");
            return;
        }
        protect(p, length, PAGE_READWRITE);
    }

    inline void seal(const mapping&, char* p, std::size_t length)
    {
        protect(p, length, PAGE_READONLY);
    }

    inline void make_no_access(const mapping&, char* p, std::size_t length)
    {
        protect(p, length, PAGE_NOACCESS);
    }

    inline std::size_t page_size()
    {
        SYSTEM_INFO info;
        ::GetSystemInfo(&info);
        return info.dwPageSize;
    }
#else
    struct mapping
    {
        char* data = nullptr;
    };

    inline mapping checked_mmap(void* p, const char* what)
    {
        if (p == MAP_FAILED)
            throw_last_error(what);
        return { static_cast<char*>(p) };
    }

    inline mapping reserve_memory(std::size_t length)
    {
        return checked_mmap(::mmap(nullptr, length, PROT_NONE,
                                MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0),
            "mmap(anonymous)");
    }

    // The file is unlinked at once: it disappears with the mapping, even after a crash.
    inline int create_unlinked_temp_file(const std::filesystem::path& dir, std::size_t length)
    {
        auto name = (dir / "cpp_utils_region_XXXXXX").string();
        const int fd = ::mkstemp(name.data());
        if (fd == -1)
            throw_last_error("mkstemp");
        ::unlink(name.c_str());
        if (::ftruncate(fd, static_cast<off_t>(length)) != 0)
        {
            ::close(fd);
            throw_last_error("ftruncate");
        }
        return fd;
    }

    inline mapping reserve_file(std::size_t length, const std::filesystem::path& dir)
    {
        const int fd = create_unlinked_temp_file(dir, length);
        void* p = ::mmap(nullptr, length, PROT_NONE, MAP_SHARED, fd, 0);
        ::close(fd);
        return checked_mmap(p, "mmap(file)");
    }

    inline void release(mapping& m, std::size_t length) { ::munmap(m.data, length); }

    inline void protect(char* p, std::size_t length, int protection)
    {
        if (::mprotect(p, length, protection) != 0)
            throw_last_error("mprotect");
    }

    inline void make_writable(const mapping&, char* p, std::size_t length)
    {
        protect(p, length, PROT_READ | PROT_WRITE);
    }

    inline void seal(const mapping&, char* p, std::size_t length) { protect(p, length, PROT_READ); }

    inline void make_no_access(const mapping&, char* p, std::size_t length)
    {
        protect(p, length, PROT_NONE);
    }

    inline std::size_t page_size() { return static_cast<std::size_t>(::sysconf(_SC_PAGESIZE)); }
#endif
}

[[nodiscard]] inline std::size_t page_size()
{
    static const std::size_t size = details::region::page_size();
    return size;
}

// A region of address space the size of a file, whose pages are inaccessible until written and
// sealed. Reading a page that was never sealed crashes instead of returning zeros.
class reserved_region
{
    details::region::mapping p_mapping {};
    std::size_t p_size = 0;
    std::size_t p_length = 0;

    [[nodiscard]] char* page(std::size_t index) const { return p_mapping.data + index * page_size(); }
    [[nodiscard]] std::size_t pages_length(std::size_t first_page, std::size_t page_count) const
    {
        if (first_page + page_count > p_length / page_size())
            throw std::out_of_range { "page range outside the region" };
        return page_count * page_size();
    }

public:
    explicit reserved_region(std::size_t size, region_backing backing = region_backing::memory,
        const std::filesystem::path& temp_dir = {})
            : p_size { size }, p_length { (size + page_size() - 1) / page_size() * page_size() }
    {
        if (p_length == 0)
            return;
        p_mapping = backing == region_backing::memory
            ? details::region::reserve_memory(p_length)
            : details::region::reserve_file(p_length, details::region::resolve_temp_dir(temp_dir));
    }

    reserved_region(const reserved_region&) = delete;
    reserved_region& operator=(const reserved_region&) = delete;

    reserved_region(reserved_region&& other) noexcept
            : p_mapping { std::exchange(other.p_mapping, {}) }
            , p_size { std::exchange(other.p_size, 0) }
            , p_length { std::exchange(other.p_length, 0) }
    {
    }

    reserved_region& operator=(reserved_region&& other) noexcept
    {
        std::swap(p_mapping, other.p_mapping);
        std::swap(p_size, other.p_size);
        std::swap(p_length, other.p_length);
        return *this;
    }

    ~reserved_region()
    {
        if (p_mapping.data)
            details::region::release(p_mapping, p_length);
    }

    [[nodiscard]] char* data() const { return p_mapping.data; }
    [[nodiscard]] std::size_t size() const { return p_size; }

    char* make_writable(std::size_t first_page, std::size_t page_count)
    {
        details::region::make_writable(
            p_mapping, page(first_page), pages_length(first_page, page_count));
        return page(first_page);
    }

    void seal(std::size_t first_page, std::size_t page_count)
    {
        details::region::seal(p_mapping, page(first_page), pages_length(first_page, page_count));
    }

    void make_no_access(std::size_t first_page, std::size_t page_count)
    {
        details::region::make_no_access(
            p_mapping, page(first_page), pages_length(first_page, page_count));
    }
};

}
