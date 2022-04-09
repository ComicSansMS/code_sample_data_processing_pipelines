#ifndef INCLUDE_GUARD_AUX_PIPE_SYNTAX_OPERATOR_HPP
#define INCLUDE_GUARD_AUX_PIPE_SYNTAX_OPERATOR_HPP

#include <pipeline_concepts.hpp>
#include <execute_step_co.hpp>

template<typename CoStep, PipelineBuffer B>
struct StepHelperSource {
    CoStep co;
    B* out;
};

auto step(PipelineSource auto&& src, PipelineBuffer auto& b)
{
    return StepHelperSource{ .co = co_executeStep(src, b), .out = &b };
}

template<PipelineFilter F, PipelineBuffer B>
struct StepHelperFilter {
    F* filter;
    B* out;
};

auto step(PipelineFilter auto&& filter, PipelineBuffer auto& b_out)
{
    return StepHelperFilter{ .filter = &filter, .out = &b_out };
}

template<PipelineSink S>
struct StepHelperSink {
    S* sink;
};

auto step(PipelineSink auto&& sink)
{
    return StepHelperSink{ .sink = &sink };
}

template<typename CoStep, PipelineBuffer B, PipelineSink Sink>
auto operator|(StepHelperSource<CoStep, B>&& src, StepHelperSink<Sink> const& snk)
{
    return co_executeStep(*snk.sink, *src.out, src.co);
}

template<typename CoStep, PipelineBuffer B1, PipelineBuffer B2, PipelineFilter Filter>
auto operator|(StepHelperSource<CoStep, B1> const& src, StepHelperFilter<Filter, B2> const& f)
    requires(std::same_as<CoStep, CoFilter> || std::same_as<CoStep, CoSource>)
{
    return StepHelperSource{ .co = co_executeStep(*f.filter, *src.out, *f.out, src.co), .out = f.out };
}


#endif
