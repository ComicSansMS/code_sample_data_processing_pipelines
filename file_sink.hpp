#ifndef INCLUDE_GUARD_AUX_FILE_SINK_HPP
#define INCLUDE_GUARD_AUX_FILE_SINK_HPP

#include <process_return.hpp>

#include <cassert>
#include <span>
#include <cstdio>
#include <string_view>

struct FileSink {
    std::span<std::byte> in;
    FILE* fout;
    std::size_t total_bytes_written;

    FileSink();
    ~FileSink();

    void open(std::string_view fname);
    ProcessReturn process();
};

#endif
