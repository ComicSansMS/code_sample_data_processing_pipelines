#ifndef INCLUDE_GUARD_AUX_MT_RING_BUFFER_HPP
#define INCLUDE_GUARD_AUX_MT_RING_BUFFER_HPP

#include <ring_buffer.hpp>

#include <condition_variable>
#include <mutex>

class MTRingBuffer;

class [[nodiscard]] MTAcquiredBuffer {
private:
    friend class MTRingBuffer;
    MTRingBuffer* parent;
    AcquiredBuffer b;
    void (MTRingBuffer::* releaseFunc)(MTAcquiredBuffer&&, std::size_t);
public:
    MTAcquiredBuffer()
        :parent(nullptr), b(), releaseFunc(nullptr)
    {}

    MTAcquiredBuffer(MTRingBuffer* parent, AcquiredBuffer&& b, void (MTRingBuffer::* releaseFunc)(MTAcquiredBuffer&&, std::size_t))
        :parent(parent), b(std::move(b)), releaseFunc(releaseFunc)
    {}

    ~MTAcquiredBuffer()
    {
        release();
    }

    MTAcquiredBuffer(MTAcquiredBuffer const&) = delete;
    MTAcquiredBuffer& operator=(MTAcquiredBuffer const&) = delete;
    MTAcquiredBuffer(MTAcquiredBuffer&& rhs) noexcept
        :parent(std::exchange(rhs.parent, nullptr)), b(std::move(rhs.b)),
        releaseFunc(std::exchange(rhs.releaseFunc, nullptr))
    {}
    
    MTAcquiredBuffer& operator=(MTAcquiredBuffer&& rhs) noexcept
    {
        if (this != &rhs) {
            release();
            parent = std::exchange(rhs.parent, nullptr);
            b = std::move(rhs.b);
            releaseFunc = std::exchange(rhs.releaseFunc, nullptr);
        }
        return *this;
    }

    std::span<std::byte> getBuffer() & noexcept {
        return b.getBuffer();
    }

    operator bool() const noexcept {
        return static_cast<bool>(b);
    }

    void commitBytes(std::size_t n)
    {
        assert(parent);
        (parent->*releaseFunc)(std::move(*this), n);
        parent = nullptr;
        b = AcquiredBuffer{};
        releaseFunc = nullptr;
    }

    friend void commitBytes(MTAcquiredBuffer&& b, std::size_t n)
    {
        assert(b.parent);
        (b.parent->*b.releaseFunc)(std::move(b), n);
        b.parent = nullptr;
        b.b = AcquiredBuffer{};
        b.releaseFunc = nullptr;
    }

    void release() noexcept
    {
        if (parent) {
            (parent->*releaseFunc)(std::move(*this), 0);
        }
        parent = nullptr;
        b = AcquiredBuffer{};
        releaseFunc = nullptr;
    }
};

class MTRingBuffer {
public:
    using BufferType = MTAcquiredBuffer;
private:
    RingBuffer ring_buffer;
    mutable std::mutex mtx;
    std::condition_variable cv;

public:
    MTRingBuffer(std::size_t n_buffers, std::size_t buffer_size)
        :ring_buffer(n_buffers, buffer_size)
    {}

    ~MTRingBuffer() = default;
    MTRingBuffer(MTRingBuffer const&) = delete;
    MTRingBuffer& operator=(MTRingBuffer const&) = delete;
    MTRingBuffer(MTRingBuffer&&) = delete;
    MTRingBuffer& operator=(MTRingBuffer&&) = delete;

    MTAcquiredBuffer acquireFreeBuffer()
    {
        std::unique_lock lk{ mtx };
        AcquiredBuffer ret;
        cv.wait(lk, [this, &ret]() -> bool {
                ret = ring_buffer.acquireFreeBuffer();
                return static_cast<bool>(ret) || (ring_buffer.getOperationState() != OpState::Run);
            });
        return MTAcquiredBuffer{ this, std::move(ret), &MTRingBuffer::releaseFreeBuffer };
    }

    MTAcquiredBuffer acquireFilledBuffer()
    {
        std::unique_lock lk{ mtx };
        AcquiredBuffer ret;
        cv.wait(lk, [this, &ret]() -> bool {
                ret = ring_buffer.acquireFilledBuffer();
                return static_cast<bool>(ret) || (ring_buffer.getOperationState() != OpState::Run);
            });
        return MTAcquiredBuffer{ this, std::move(ret), &MTRingBuffer::releaseFilledBuffer };
    }

    void requestQuit()
    {
        std::scoped_lock lk{ mtx };
        ring_buffer.setOperationState(OpState::Abort);
        cv.notify_all();
    }

    RingBuffer& getRingBuffer() noexcept
    {
        return ring_buffer;
    }

    OpState getOperationState() const
    {
        std::scoped_lock lk{ mtx };
        return ring_buffer.getOperationState();
    }

    void setOperationState(OpState new_state) {
        std::scoped_lock lk{ mtx };
        ring_buffer.setOperationState(new_state);
    }

private:
    void releaseFreeBuffer(MTAcquiredBuffer&& bb, std::size_t n)
    {
        std::scoped_lock lk{ mtx };
        bb.b.commitBytes(n);
        cv.notify_one();
    }

    void releaseFilledBuffer(MTAcquiredBuffer&& bb, std::size_t)
    {
        std::scoped_lock lk{ mtx };
        bb.b.release();
        cv.notify_one();
    }
};


#endif
