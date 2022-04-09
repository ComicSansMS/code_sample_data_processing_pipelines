#define _CRT_SECURE_NO_WARNINGS
#include <file_source.hpp>

#include <string>

FileSource::FileSource()
    :out(), fin(nullptr), total_bytes_read(0)
{}

FileSource::~FileSource()
{
    if (fin) {
        fclose(fin);
    }
}

void FileSource::open(std::string_view fname)
{
    total_bytes_read = 0;
    if (fin) { fclose(fin); }
    std::string fname_str(begin(fname), end(fname));
    fin = std::fopen(fname_str.c_str(), "rb");
    std::fseek(fin, 0, SEEK_SET);
    //char buf[128];
    //fread(&buf, 1, 128, fin);
    //int i = buf[1];
}

ProcessReturn FileSource::process()
{
    if (!fin) {
        return ProcessReturn::Error;
    }
    if (out.empty()) {
        return ProcessReturn::Pending;
    }

    std::size_t const bytes_read = fread(out.data(), 1, out.size(), fin);
    out = out.subspan(bytes_read);
    total_bytes_read += bytes_read;
    if (feof(fin)) {
        fclose(fin);
        fin = nullptr;
        return ProcessReturn::Done;
    }
    if (ferror(fin)) {
        perror("Error reading file");
        return ProcessReturn::Error;
    }
    return ProcessReturn::Pending;
}
