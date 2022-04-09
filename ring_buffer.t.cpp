#include <ring_buffer.hpp>

#include <catch.hpp>

TEST_CASE("Ring Buffer")
{
    RingBuffer buffer(3, 1024);

    SECTION("Acquire")
    {

        {
            auto b1 = buffer.acquireFreeBuffer();
            REQUIRE(b1);
            CHECK(b1.parent == &buffer);
            CHECK(b1.buffer.data() == buffer.buffers[0].data());
        }
        {
            auto b2 = buffer.acquireFreeBuffer();
            REQUIRE(b2);
            CHECK(b2.parent == &buffer);
            CHECK(b2.buffer.data() == buffer.buffers[1].data());
        }
        {
            auto b3 = buffer.acquireFreeBuffer();
            REQUIRE(b3);
            CHECK(b3.parent == &buffer);
            CHECK(b3.buffer.data() == buffer.buffers[2].data());
        }
        {
            auto no_more = buffer.acquireFreeBuffer();
            CHECK(!no_more);
        }
        {
            auto b1 = buffer.acquireFilledBuffer();
            REQUIRE(b1);
            CHECK(b1.parent == &buffer);
            CHECK(b1.buffer.data() == buffer.buffers[0].data());
        }
        {
            auto b1 = buffer.acquireFreeBuffer();
            REQUIRE(b1);
            CHECK(b1.parent == &buffer);
            CHECK(b1.buffer.data() == buffer.buffers[0].data());
        }
        {
            auto no_more = buffer.acquireFreeBuffer();
            CHECK(!no_more);
        }
        {
            auto b2 = buffer.acquireFilledBuffer();
            REQUIRE(b2);
            CHECK(b2.parent == &buffer);
            CHECK(b2.buffer.data() == buffer.buffers[1].data());
        }
        {
            auto b3 = buffer.acquireFilledBuffer();
            REQUIRE(b3);
            CHECK(b3.parent == &buffer);
            CHECK(b3.buffer.data() == buffer.buffers[2].data());
        }
        {
            auto b1 = buffer.acquireFilledBuffer();
            REQUIRE(b1);
            CHECK(b1.parent == &buffer);
            CHECK(b1.buffer.data() == buffer.buffers[0].data());
        }
        {
            auto no_more = buffer.acquireFilledBuffer();
            CHECK(!no_more);
        }
    }

    SECTION("Commit size")
    {
        {
            auto b1 = buffer.acquireFreeBuffer();
            REQUIRE(b1);
            CHECK(b1.getBuffer().size() == 1024);
            b1.commitBytes(42);
        }
        {
            auto b1 = buffer.acquireFilledBuffer();
            REQUIRE(b1);
            CHECK(b1.getBuffer().size() == 42);
        }
        {
            auto b2 = buffer.acquireFreeBuffer();
            REQUIRE(b2);
            CHECK(b2.getBuffer().size() == 1024);
        }
        {
            auto b2 = buffer.acquireFilledBuffer();
            REQUIRE(b2);
            CHECK(b2.getBuffer().size() == 0);
        }
    }

    SECTION("OpState")
    {
        CHECK(buffer.getOperationState() == OpState::Run);
        {
            auto b1 = buffer.acquireFreeBuffer();
            CHECK(!b1.getBuffer().empty());
            b1.commitBytes(422);
        }
        CHECK(buffer.getOperationState() == OpState::Run);
        buffer.setOperationState(OpState::Done);
        CHECK(buffer.getOperationState() == OpState::Done);
        {
            auto b1 = buffer.acquireFilledBuffer();
            CHECK(b1.getBuffer().empty());
        }
        {
            auto b2 = buffer.acquireFreeBuffer();
            CHECK(b2.getBuffer().empty());
        }
        buffer.setOperationState(OpState::Run);
        CHECK(buffer.getOperationState() == OpState::Run);
        {
            auto b1 = buffer.acquireFilledBuffer();
            CHECK(!b1.getBuffer().empty());
            CHECK(b1.getBuffer().size() == 422);
        }
        {
            auto b2 = buffer.acquireFreeBuffer();
            CHECK(!b2.getBuffer().empty());
        }
    }
}
