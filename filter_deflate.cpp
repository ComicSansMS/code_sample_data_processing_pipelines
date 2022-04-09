#include <filter_deflate.hpp>

#define ZLIB_CONST
#include <zlib.h>

#include <cassert>
#include <cstring>

struct FilterDeflate::Impl {
    z_stream zs;
};

FilterDeflate::FilterDeflate()
    :pimpl(std::make_unique<Impl>())
{
    std::memset(&pimpl->zs, 0, sizeof(z_stream));
    auto const res = deflateInit(&pimpl->zs, Z_BEST_COMPRESSION);
    assert(res == Z_OK);
}

FilterDeflate::~FilterDeflate()
{
    auto const res = deflateEnd(&pimpl->zs);
    assert(res == Z_OK);
}

ProcessReturn FilterDeflate::process()
{
    z_stream& zs = pimpl->zs;
    bool do_flush = false;
    if (zs.avail_in == 0) {
        if (in.empty()) {
            do_flush = true;
        } else {
            zs.next_in = reinterpret_cast<Bytef const*>(in.data());
            zs.avail_in = static_cast<uInt>(in.size());
            do_flush = false;
        }
    }
    if (zs.avail_out == 0) {
        if (out.empty()) { return ProcessReturn::Error; }
        zs.next_out = reinterpret_cast<Bytef*>(out.data());
        zs.avail_out = static_cast<uInt>(out.size());
    }
    if (do_flush) {
        auto const res = deflate(&zs, Z_FINISH);
        if ((res != Z_OK) && (res != Z_STREAM_END)) { return ProcessReturn::Error; }
        std::size_t out_consumed = out.size() - zs.avail_out;
        out = out.subspan(out_consumed);
        if (res == Z_STREAM_END) {
            auto const res2 = deflateReset(&zs);
            assert(res2 == Z_OK);
            return ProcessReturn::Done;
        }
    } else {
        auto const res = deflate(&zs, Z_NO_FLUSH);
        if (res != Z_OK) { return ProcessReturn::Error; }
        std::size_t const in_consumed = in.size() - zs.avail_in;
        in = in.subspan(in_consumed);
        std::size_t const out_consumed = out.size() - zs.avail_out;
        out = out.subspan(out_consumed);
    }
    return ProcessReturn::Pending;
}
