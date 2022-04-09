#ifndef INCLUDE_GUARD_AUX_FILTER_DEFLATE_HPP
#define INCLUDE_GUARD_AUX_FILTER_DEFLATE_HPP

#include <process_return.hpp>

#include <cstddef>
#include <memory>
#include <span>

struct FilterDeflate {
    std::span<std::byte> in;
    std::span<std::byte> out;

    ProcessReturn process();

    FilterDeflate();
    ~FilterDeflate();

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

#endif
