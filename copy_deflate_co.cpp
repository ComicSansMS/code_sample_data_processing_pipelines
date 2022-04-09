#include <file_sink.hpp>
#include <file_source.hpp>
#include <filter_deflate.hpp>
#include <filter_inflate.hpp>
#include <ring_buffer.hpp>
#include <execute_step_co.hpp>
#include <pipe_syntax_operator.hpp>

#include <fmt/format.h>

#include <cstdio>
#include <chrono>
#include <thread>

ProcessReturn do_copy(FileSource& file_in, FileSink& file_out)
{
    RingBuffer b1(3, 1 << 20);
    RingBuffer b2(10, 4 << 10);
    RingBuffer b3(2, 4 << 10);
    fmt::print("+++ Start processing.\n");
    /*
    co_executeStep(file_out, b2,
        co_executeStep(FilterDeflate{}, b1, b2,
            co_executeStep(file_in, b1)));
    /*/
    step(file_in, b1) | step(FilterDeflate{}, b2) | step(file_out);
    //*/
    fmt::print("+++ Finished processing.\n");

    fmt::print("Bytes read:    {}\n", file_in.total_bytes_read);
    fmt::print("Bytes written: {}\n", file_out.total_bytes_written);
    return ProcessReturn::Done;
}

//step(file_in, b1) | step(FilterDeflate{}, b2) | step(FilterInflate{}, b3) | step(file_out);

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fmt::print(stderr, "Usage: {} [source-file] [destination-file]\n", argv[0]);
        return 1;
    }
    char const* source_file = argv[1];
    char const* destination_file = argv[2];

    fmt::print("Copying {} -> {}\n", source_file, destination_file);
    FileSource src;
    src.open(source_file);
    if (!src.fin) {
        fmt::print("Error: Could not open source file {} for reading.\n", source_file);
        return 2;
    }
    FileSink dst;
    dst.open(destination_file);
    if (!dst.fout) {
        fmt::print("Error: Could not open destination file {} for writing.\n", destination_file);
        return 2;
    }

    auto const t0 = std::chrono::steady_clock::now();
    ProcessReturn const res = do_copy(src, dst);
    auto const t1 = std::chrono::steady_clock::now();
    if (res == ProcessReturn::Error) {
        fmt::print("An error occured.\n");
        return 5;
    }
    fmt::print("Done. Took {}ms.\n", std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
}
