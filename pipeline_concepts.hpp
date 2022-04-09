#ifndef INCLUDE_GUARD_AUX_PIPELINE_CONCEPTS_HPP
#define INCLUDE_GUARD_AUX_PIPELINE_CONCEPTS_HPP

#include <op_state.hpp>
#include <process_return.hpp>

#include <concepts>
#include <span>

template<typename T>
concept PipelineStage = requires(T a) {
    { a.process() } -> std::same_as<ProcessReturn>;
};

template<typename T>
concept PipelineSource = PipelineStage<T> && requires(T a) {
    { a.out } -> std::same_as<std::span<std::byte>&>;
};

template<typename T>
concept PipelineSink = PipelineStage<T> && requires(T a) {
    { a.in } -> std::same_as<std::span<std::byte>&>;
};

template<typename T>
concept PipelineFilter = PipelineSource<T> && PipelineSink<T>;

template<typename T>
concept PipelineAcquiredBuffer = requires(T a) {
    { a.getBuffer() } -> std::same_as<std::span<std::byte>>;
    { a.commitBytes(std::size_t{}) };
    { commitBytes(std::move(a), std::size_t{}) };
};

template<typename T>
concept PipelineBuffer = requires(T a) {
    { a.acquireFreeBuffer() } -> PipelineAcquiredBuffer;
    { a.acquireFilledBuffer() }  -> PipelineAcquiredBuffer;
    { a.getOperationState() } -> std::same_as<OpState>;
    { a.setOperationState(OpState{}) };
} && PipelineAcquiredBuffer<typename T::BufferType>;

#endif
