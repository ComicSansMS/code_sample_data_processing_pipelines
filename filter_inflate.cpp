#include <filter_inflate.hpp>

#define ZLIB_CONST
#include <zlib.h>

#include <cassert>
#include <cstring>

struct FilterInflate::Impl {
    z_stream zs;
};

FilterInflate::FilterInflate()
    :pimpl(std::make_unique<Impl>())
{
    std::memset(&pimpl->zs, 0, sizeof(z_stream));
    auto const res = inflateInit(&pimpl->zs);
    assert(res == Z_OK);
}

FilterInflate::~FilterInflate()
{
    auto const res = inflateEnd(&pimpl->zs);
    assert(res == Z_OK);
}

ProcessReturn FilterInflate::process()
{
    z_stream& zs = pimpl->zs;
    if (zs.avail_in == 0) {
        zs.next_in = reinterpret_cast<Bytef const*>(in.data());
        zs.avail_in = static_cast<uInt>(in.size());
    }
    if (zs.avail_out == 0) {
        if (out.empty()) { return ProcessReturn::Error; }
        zs.next_out = reinterpret_cast<Bytef*>(out.data());
        zs.avail_out = static_cast<uInt>(out.size());
    }
    auto const res = inflate(&zs, Z_NO_FLUSH);
    if ((res != Z_OK) && (res != Z_STREAM_END)) { return ProcessReturn::Error; }
    std::size_t const in_consumed = in.size() - zs.avail_in;
    in = in.subspan(in_consumed);
    std::size_t const out_consumed = out.size() - zs.avail_out;
    out = out.subspan(out_consumed);
    if (res == Z_STREAM_END) { return ProcessReturn::Done; }
    return ProcessReturn::Pending;
}
