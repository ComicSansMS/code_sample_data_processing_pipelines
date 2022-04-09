#include <file_sink.hpp>
#include <file_source.hpp>
#include <filter_inflate.hpp>
#include <mt_ring_buffer.hpp>
#include <execute_step.hpp>

static_assert(PipelineAcquiredBuffer<MTAcquiredBuffer>);
static_assert(PipelineBuffer<MTRingBuffer>);

#include <fmt/format.h>

#include <cstdio>
#include <chrono>
#include <thread>

ProcessReturn do_copy(FileSource& src, FileSink& dst, MTRingBuffer& buffer)
{
    std::thread t1{ [&src, &buffer] { executeStep(src, buffer); } };
    MTRingBuffer b2(10, 1024*4);
    FilterInflate f;
    std::thread tcp{ [&f, &buffer, &b2] { executeStep(f, buffer, b2); } };
    std::thread t2{ [&dst, &b2] { executeStep(dst, b2); } };
    t1.join();
    tcp.join();
    t2.join();
    return ProcessReturn::Done;
}


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
    std::size_t const buffer_size = 1 << 20;
    MTRingBuffer buffer(3, buffer_size);

    auto const t0 = std::chrono::steady_clock::now();
    ProcessReturn const res = do_copy(src, dst, buffer);
    auto const t1 = std::chrono::steady_clock::now();
    if (res == ProcessReturn::Error) {
        fmt::print("An error occured.\n");
        return 5;
    }
    fmt::print("Done. Took {}ms.\n", std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
}
