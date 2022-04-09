#ifndef INCLUDE_GUARD_AUX_FILTER_ID_HPP
#define INCLUDE_GUARD_AUX_FILTER_ID_HPP

#include <process_return.hpp>

#include <cstddef>
#include <cstring>
#include <span>

struct FilterId {
    std::span<std::byte> in;
    std::span<std::byte> out;

    ProcessReturn process()
    {
        if (in.empty()) {
            return ProcessReturn::Done;
        }
        if (out.empty()) {
            return ProcessReturn::Error;
        }

        std::size_t const bytes_to_copy = std::min(in.size(), out.size());
        std::memcpy(out.data(), in.data(), bytes_to_copy);
        in = in.subspan(bytes_to_copy);
        out = out.subspan(bytes_to_copy);

        return ProcessReturn::Pending;
    }
};

#endif
