#ifndef INCLUDE_GUARD_AUX_EXECUTE_STEP_HPP
#define INCLUDE_GUARD_AUX_EXECUTE_STEP_HPP

#include <pipeline_concepts.hpp>

#include <optional>
#include <span>

inline ProcessReturn executeStep(PipelineSource auto& step, PipelineBuffer auto& out_buffer)
{
    ProcessReturn ret = ProcessReturn::Pending;
    while(ret == ProcessReturn::Pending) {
        auto acquired_buffer = out_buffer.acquireFreeBuffer();
        step.out = acquired_buffer.getBuffer();
        if (step.out.empty()) {
            // ring buffer quit requested
            assert(out_buffer.getOperationState() == OpState::Abort);
            return ProcessReturn::Error;
        }
        ret = step.process();
        if (ret == ProcessReturn::Error) { out_buffer.setOperationState(OpState::Error); break; }
        commitBytes(std::move(acquired_buffer), acquired_buffer.getBuffer().size() - step.out.size());
    }
    if (ret == ProcessReturn::Done) {
        out_buffer.setOperationState(OpState::Finalize);
    }
    return ret;
}

inline ProcessReturn executeStep(PipelineSink auto& step, PipelineBuffer auto& in_buffer)
{
    ProcessReturn ret = ProcessReturn::Pending;
    while (ret == ProcessReturn::Pending) {
        auto acquired_buffer = in_buffer.acquireFilledBuffer();
        step.in = acquired_buffer.getBuffer();
        if (step.in.empty()) { /* ring buffer quit requested */ return ProcessReturn::Error; }
        ret = step.process();
        if (ret != ProcessReturn::Error) {
            commitBytes(std::move(acquired_buffer), acquired_buffer.getBuffer().size() - step.in.size());
        }
    }
    if (ret == ProcessReturn::Done) {
        in_buffer.setOperationState(OpState::Done);
    }
    return ret;
}

template<PipelineBuffer InBuffer_T, PipelineBuffer OutBuffer_T>
inline ProcessReturn executeStep(PipelineFilter auto& step,
                                 InBuffer_T& in_buffer,
                                 OutBuffer_T& out_buffer)
{
    ProcessReturn ret = ProcessReturn::Pending;
    std::optional<typename InBuffer_T::BufferType> opt_acquired_in;
    std::optional<typename OutBuffer_T::BufferType> opt_acquired_out;
    while (ret == ProcessReturn::Pending) {
        if (step.in.empty()) {
            assert(!opt_acquired_in);
            opt_acquired_in = in_buffer.acquireFilledBuffer();
            if (opt_acquired_in->getBuffer().empty()) {
                if (in_buffer.getOperationState() != OpState::Finalize) {
                    assert((in_buffer.getOperationState() == OpState::Error) ||
                           (in_buffer.getOperationState() == OpState::Abort));
                    out_buffer.setOperationState(in_buffer.getOperationState());
                    return ProcessReturn::Error;
                }
            }
            step.in = opt_acquired_in->getBuffer();
        }
        if (step.out.empty()) {
            assert(!opt_acquired_out);
            opt_acquired_out = out_buffer.acquireFreeBuffer();;
            if (opt_acquired_out->getBuffer().empty()) {
                // ring buffer quit requested
                assert(out_buffer.getOperationState() == OpState::Abort);
                return ProcessReturn::Error;
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
        }
    }
    return ProcessReturn::Error;
}

#endif
