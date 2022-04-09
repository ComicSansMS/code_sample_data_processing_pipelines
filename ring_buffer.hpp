#ifndef INCLUDE_GUARD_AUX_RING_BUFFER_HPP
#define INCLUDE_GUARD_AUX_RING_BUFFER_HPP

#include <op_state.hpp>

#include <cassert>
#include <span>
#include <utility>
#include <vector>

struct RingBuffer;

struct [[nodiscard]] AcquiredBuffer {
    std::span<std::byte> buffer;
    RingBuffer* parent;
    void (RingBuffer::* releaseFunc)(std::size_t);

    AcquiredBuffer() noexcept
        :buffer(), parent(nullptr), releaseFunc(nullptr)
    {}

    AcquiredBuffer(RingBuffer* parent, std::span<std::byte> buffer, void (RingBuffer::* releaseFunc)(std::size_t)) noexcept
        :buffer(buffer), parent(parent), releaseFunc(releaseFunc)
    {}

    ~AcquiredBuffer()
    {
        release();
    }

    AcquiredBuffer(AcquiredBuffer const&) = delete;
    AcquiredBuffer& operator=(AcquiredBuffer const&) = delete;
    AcquiredBuffer(AcquiredBuffer&& rhs) noexcept
        :buffer(std::exchange(rhs.buffer, std::span<std::byte>{})), parent(std::exchange(rhs.parent, nullptr)),
        releaseFunc(std::exchange(rhs.releaseFunc, nullptr))
    {}
    AcquiredBuffer& operator=(AcquiredBuffer&& rhs) noexcept {
        if (this != &rhs) {
            release();
            buffer = std::exchange(rhs.buffer, std::span<std::byte>{});
            parent = std::exchange(rhs.parent, nullptr);
            releaseFunc = std::exchange(rhs.releaseFunc, nullptr);
        }
        return *this;
    }

    std::span<std::byte> getBuffer() & noexcept {
        return buffer;
    }

    operator bool() const noexcept {
        return parent != nullptr;
    }

    void commitBytes(std::size_t n)
    {
        assert(parent);
        assert(n <= buffer.size());
        (parent->*releaseFunc)(n);
        parent = nullptr;
        buffer = std::span<std::byte>{};
        releaseFunc = nullptr;
    }

    friend void commitBytes(AcquiredBuffer&& b, std::size_t n) noexcept {
        assert(b.parent);
        assert(n <= b.buffer.size());
        (b.parent->*b.releaseFunc)(n);
        b.parent = nullptr;
        b.buffer = std::span<std::byte>{};
        b.releaseFunc = nullptr;
    }

    void release() noexcept
    {
        if (parent) {
            (parent->*releaseFunc)(0);
        }
        parent = nullptr;
        buffer = std::span<std::byte>{};
        releaseFunc = nullptr;
    }
};

struct RingBuffer {
    using BufferType = AcquiredBuffer;

    std::vector<std::byte> storage;
    std::size_t buffer_size;
    std::vector<std::span<std::byte>> buffers;
    std::vector<std::size_t> committed_bytes;
    std::size_t begin_free_index;
    bool free_is_acquired;
    std::size_t begin_filled_index;
    bool filled_is_acquired;
    std::size_t filled_count;
    OpState operation_state;

    RingBuffer(std::size_t n_buffers, std::size_t buffer_size)
        :storage(n_buffers*buffer_size), buffers(n_buffers, std::span<std::byte>{}),
        committed_bytes(n_buffers, buffer_size),
        begin_free_index(0), free_is_acquired(false),
        begin_filled_index(0), filled_is_acquired(false), filled_count(0),
        operation_state(OpState::Run)
    {
        assert(n_buffers > 1);
        
        for (std::size_t i = 0; i < n_buffers; ++i) {
            buffers[i] = std::span<std::byte>(&storage[i * buffer_size], buffer_size);
        }
    }

    bool hasFreeBuffer() const noexcept {
        return (filled_count == 0) || (begin_free_index != begin_filled_index);
    }

    bool hasFilledBuffer() const noexcept {
        return (filled_count == buffers.size()) || (begin_filled_index != begin_free_index);
    }

    AcquiredBuffer acquireFreeBuffer()
    {
        assert(!free_is_acquired);
        bool const running = (operation_state == OpState::Run);
        if (running && hasFreeBuffer()) {
            std::span<std::byte> ret = buffers[begin_free_index];
            free_is_acquired = true;
            return AcquiredBuffer{ this, ret, &RingBuffer::releaseFreeBuffer };
        } else {
            return {};
        }
    }

    AcquiredBuffer acquireFilledBuffer()
    {
        assert(!filled_is_acquired);
        bool const running = (operation_state == OpState::Run || operation_state == OpState::Finalize);
        if (running && hasFilledBuffer()) {
            std::span<std::byte> ret = buffers[begin_filled_index];
            ret = ret.subspan(0, committed_bytes[begin_filled_index]);
            filled_is_acquired = true;
            return AcquiredBuffer{ this, ret, &RingBuffer::releaseFilledBuffer };
        } else {
            return {};
        }
    }

    OpState getOperationState() const noexcept {
        return operation_state;
    }

    void setOperationState(OpState new_state) noexcept {
        operation_state = new_state;
    }

private:
    void releaseFreeBuffer(std::size_t committed) noexcept
    {
        committed_bytes[begin_free_index] = committed;
        ++begin_free_index;
        if (begin_free_index == buffers.size()) { begin_free_index = 0; }
        ++filled_count;
        free_is_acquired = false;
    }

    void releaseFilledBuffer(std::size_t) noexcept
    {
        ++begin_filled_index;
        if (begin_filled_index == buffers.size()) { begin_filled_index = 0; }
        --filled_count;
        filled_is_acquired = false;
    }
};

#endif
