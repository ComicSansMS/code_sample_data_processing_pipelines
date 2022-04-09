#ifndef INCLUDE_GUARD_AUX_FILE_SOURCE_HPP
#define INCLUDE_GUARD_AUX_FILE_SOURCE_HPP

#include <process_return.hpp>

#include <cassert>
#include <span>
#include <cstdio>
#include <string_view>

struct FileSource {
    std::span<std::byte> out;
    FILE* fin;
    std::size_t total_bytes_read;

    FileSource();
    ~FileSource();

    void open(std::string_view fname);
    ProcessReturn process();
};

#endif
