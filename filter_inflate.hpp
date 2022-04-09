#ifndef INCLUDE_GUARD_AUX_FILTER_INFLATE_HPP
#define INCLUDE_GUARD_AUX_FILTER_INFLATE_HPP

#include <process_return.hpp>

#include <cstddef>
#include <memory>
#include <span>

struct FilterInflate {
    std::span<std::byte> in;
    std::span<std::byte> out;

    ProcessReturn process();

    FilterInflate();
    ~FilterInflate();

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

#endif
