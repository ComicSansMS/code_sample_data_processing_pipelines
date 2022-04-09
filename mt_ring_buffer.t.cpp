#include <mt_ring_buffer.hpp>

#include <catch.hpp>

TEST_CASE("MT Ring Buffer")
{
    MTRingBuffer buffer(3, 1024);
    std::span<std::byte> span1;

    std::thread t1{ [&buffer, &span1]() {
        {
            auto b1 = buffer.acquireFreeBuffer();
            REQUIRE(b1);
            span1 = b1.getBuffer();
            CHECK(b1.getBuffer().data() == buffer.getRingBuffer().buffers[0].data());
        }
        {
            auto b2 = buffer.acquireFreeBuffer();
            REQUIRE(b2);
            CHECK(b2.getBuffer().data() != span1.data());
            CHECK(b2.getBuffer().data() == buffer.getRingBuffer().buffers[1].data());
        }
        {
            auto b3 = buffer.acquireFreeBuffer();
            REQUIRE(b3);
            CHECK(b3.getBuffer().data() != span1.data());
            CHECK(b3.getBuffer().data() == buffer.getRingBuffer().buffers[2].data());
        }
        {
            auto b1 = buffer.acquireFreeBuffer();
            REQUIRE(b1);
            CHECK(b1.getBuffer().data() == span1.data());
        }
    } };
    std::span<std::byte> outer_span1;
    {
        auto b1 = buffer.acquireFilledBuffer();
        outer_span1 = b1.getBuffer();
    }
    t1.join();
    CHECK(outer_span1.data() == span1.data());
}
