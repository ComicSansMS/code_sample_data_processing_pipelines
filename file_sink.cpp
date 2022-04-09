#define _CRT_SECURE_NO_WARNINGS
#include <file_sink.hpp>

#include <string>

FileSink::FileSink()
    :in(), fout(nullptr), total_bytes_written(0)
{}

FileSink::~FileSink()
{
    if (fout) {
        fclose(fout);
    }
}

void FileSink::open(std::string_view fname)
{
    total_bytes_written = 0;
    if (fout) { fclose(fout); }
    std::string fname_str(begin(fname), end(fname));
    fout = std::fopen(fname_str.c_str(), "wb");
}

ProcessReturn FileSink::process()
{
    if (!fout) {
        return ProcessReturn::Error;
    }
    if (in.empty()) {
        return ProcessReturn::Pending;
    }

    std::size_t const bytes_written = fwrite(in.data(), 1, in.size(), fout);
    in = in.subspan(0, in.size() - bytes_written);
    total_bytes_written += bytes_written;
    if (ferror(fout)) {
        perror("Error writing file");
        return ProcessReturn::Error;
    }
    return ProcessReturn::Pending;
}
