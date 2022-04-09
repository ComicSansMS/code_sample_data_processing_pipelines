#ifndef INCLUDE_GUARD_AUX_OP_STATE_HPP
#define INCLUDE_GUARD_AUX_OP_STATE_HPP

enum class OpState {
    Run,
    Abort,
    Error,
    Finalize,
    Done
};

#endif
