#include <fmt/format.h>

#include <coroutine>
#include <optional>


struct promise;
struct coroutine {
    struct promise {
        coroutine get_return_object()
        {
            return coroutine{ std::coroutine_handle<promise>::from_promise(*this) };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {
            if (other) {
                other->promise().other = std::nullopt;
                other->resume();
            }
        }
        void unhandled_exception() { std::abort(); }

        std::optional<std::coroutine_handle<promise>> other;
    };
    using promise_type = promise;

    coroutine(std::coroutine_handle<promise_type> h)
        :m_handle(h)
    {
    }

    void setOther(coroutine const& other)
    {
        m_handle.promise().other = other.m_handle;
    }

    void run()
    {
        m_handle.resume();
    }

    std::coroutine_handle<promise_type> m_handle;
};

struct Transfer {
    bool await_ready() { return false; }

    bool await_resume() {
        return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<coroutine::promise> me)
    {
        coroutine::promise& p = me.promise();
        if (p.other) {
            std::coroutine_handle<coroutine::promise> other_handle = *me.promise().other;
            other_handle.promise().other = me;
        }
        return me.promise().other.value_or(me);
    }
};

coroutine stride2_from(int start, int stop)
{
    for (int i = start; i <= stop; i += 2) {
        fmt::print("{}\n", i);
        co_await Transfer{};
    }
}

int main()
{
    coroutine even = stride2_from(0, 10);
    coroutine odd = stride2_from(1, 25);
    even.setOther(odd);
    even.run();
}
