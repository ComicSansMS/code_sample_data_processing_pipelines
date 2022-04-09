#ifndef INCLUDE_GUARD_AUX_EXECUTE_STEP_CO_HPP
#define INCLUDE_GUARD_AUX_EXECUTE_STEP_CO_HPP

#include <pipeline_concepts.hpp>

#include <coroutine>
#include <optional>
#include <span>

struct CoSource {
    struct promise_type {
        promise_type()
            :final_return(ProcessReturn::Pending)
        {
        }

        CoSource get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_value(ProcessReturn r) { final_return = r; downstream_continuation.resume(); }
        void unhandled_exception() {}

        std::coroutine_handle<> downstream_continuation;
        ProcessReturn final_return;
    };

    std::coroutine_handle<promise_type> handle;
    CoSource(std::coroutine_handle<promise_type> h)
        :handle(h)
    {}
};

struct CoSink {
    struct promise_type {
        promise_type()
            :final_return(ProcessReturn::Pending)
        {
        }

        CoSink get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_value(ProcessReturn r) { final_return = r; }
        void unhandled_exception() {}

        std::coroutine_handle<> upstream_continuation;
        ProcessReturn final_return;
    };

    std::coroutine_handle<promise_type> handle;
    CoSink(std::coroutine_handle<promise_type> h)
        :handle(h)
    {}

};

struct CoFilter {
    struct promise_type {
        promise_type()
            :final_return(ProcessReturn::Pending)
        {
        }

        CoFilter get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_value(ProcessReturn r) { final_return = r; downstream_continuation.resume(); }
        void unhandled_exception() {}

        std::coroutine_handle<> upstream_continuation;
        std::coroutine_handle<> downstream_continuation;
        ProcessReturn final_return;
    };

    std::coroutine_handle<promise_type> handle;
    CoFilter(std::coroutine_handle<promise_type> h)
        :handle(h)
    {}

    ProcessReturn getFinalReturn() const {
        return handle.promise().final_return;
    }
};

struct AcquireFree {
    RingBuffer* buffer;
    AcquireFree(RingBuffer& b)
        :buffer(&b)
    {}

    bool await_ready() { return buffer->hasFreeBuffer(); }
    template<typename P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> h) {
        P& promise = h.promise();
        return promise.downstream_continuation;
    }
    AcquiredBuffer await_resume() {
        return buffer->acquireFreeBuffer();
    }
};

template<typename CoSource_T>
struct AcquireFilled {
    RingBuffer* buffer;
    CoSource_T const* upstream;

    AcquireFilled(RingBuffer& b, CoSource_T const& upstream)
        :buffer(&b), upstream(&upstream)
    {}

    bool await_ready() { return buffer->hasFilledBuffer() || buffer->getOperationState() != OpState::Run; }
    template<typename P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> h) {
        upstream->handle.promise().downstream_continuation = h;
        return upstream->handle;
    }
    AcquiredBuffer await_resume() {
        return buffer->acquireFilledBuffer();
    }
};

inline CoSource co_executeStep(PipelineSource auto&& step, PipelineBuffer auto& out_buffer)
{
    ProcessReturn ret = ProcessReturn::Pending;
    while (ret == ProcessReturn::Pending) {
        auto acquired_buffer = co_await AcquireFree{ out_buffer };
        step.out = acquired_buffer.getBuffer();
        if (step.out.empty()) {
            // ring buffer quit requested
            assert(out_buffer.getOperationState() == OpState::Abort);
            ret = ProcessReturn::Error;
            break;
        }
        ret = step.process();
        if (ret == ProcessReturn::Error) { out_buffer.setOperationState(OpState::Error); break; }
        commitBytes(std::move(acquired_buffer), acquired_buffer.getBuffer().size() - step.out.size());
    }
    if (ret == ProcessReturn::Done) {
        out_buffer.setOperationState(OpState::Finalize);
    }
    co_return ret;
}

template<typename Producer_T>
inline CoSink co_executeStep(PipelineSink auto&& step, PipelineBuffer auto& in_buffer, Producer_T const& p)
{
    ProcessReturn ret = ProcessReturn::Pending;
    while (ret == ProcessReturn::Pending) {
        auto acquired_buffer = co_await AcquireFilled{ in_buffer, p };
        step.in = acquired_buffer.getBuffer();
        if (step.in.empty()) { /* ring buffer quit requested */ co_return ProcessReturn::Error; }
        ret = step.process();
        if (ret != ProcessReturn::Error) {
            commitBytes(std::move(acquired_buffer), acquired_buffer.getBuffer().size() - step.in.size());
        }
    }
    if (ret == ProcessReturn::Done) {
        in_buffer.setOperationState(OpState::Done);
    }
    co_return ret;
}


template<PipelineBuffer InBuffer_T, PipelineBuffer OutBuffer_T, typename CoStep>
inline CoFilter co_executeStep(PipelineFilter auto&& step,
    InBuffer_T& in_buffer,
    OutBuffer_T& out_buffer,
    CoStep const& p)
requires(std::same_as<CoStep, CoFilter> || std::same_as<CoStep, CoSource>)
{
    ProcessReturn ret = ProcessReturn::Pending;
    std::optional<typename InBuffer_T::BufferType> opt_acquired_in;
    std::optional<typename OutBuffer_T::BufferType> opt_acquired_out;
    while (ret == ProcessReturn::Pending) {
        if (step.in.empty()) {
            assert(!opt_acquired_in);
            opt_acquired_in = co_await AcquireFilled{ in_buffer, p };
            if (opt_acquired_in->getBuffer().empty()) {
                if (in_buffer.getOperationState() != OpState::Finalize) {
                    assert((in_buffer.getOperationState() == OpState::Error) ||
                        (in_buffer.getOperationState() == OpState::Abort));
                    out_buffer.setOperationState(in_buffer.getOperationState());
                    co_return ProcessReturn::Error;
                }
            }
            step.in = opt_acquired_in->getBuffer();
        }
        if (step.out.empty()) {
            assert(!opt_acquired_out);
            opt_acquired_out = co_await AcquireFree{ out_buffer };
            if (opt_acquired_out->getBuffer().empty()) {
                // ring buffer quit requested
                assert(out_buffer.getOperationState() == OpState::Abort);
                co_return ProcessReturn::Error;
            }
            step.out = opt_acquired_out->getBuffer();
        }
        ret = step.process();
        if (ret == ProcessReturn::Error) { out_buffer.setOperationState(OpState::Error); break; }
        if (step.in.empty()) {
            opt_acquired_in = std::nullopt;
        }
        if (step.out.empty()) {
            commitBytes(std::move(*opt_acquired_out), opt_acquired_out->getBuffer().size());
            opt_acquired_out = std::nullopt;
        }
        if (ret == ProcessReturn::Done) {
            commitBytes(std::move(*opt_acquired_out), opt_acquired_out->getBuffer().size() - step.out.size());
            in_buffer.setOperationState(OpState::Done);
            out_buffer.setOperationState(OpState::Finalize);
            co_return ProcessReturn::Done;
        }
    }
    co_return ProcessReturn::Error;
}


#endif
