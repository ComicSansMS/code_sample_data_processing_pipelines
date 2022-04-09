#include <fmt/format.h>

#include <coroutine>

struct Producer {
    struct promise_type {
        promise_type(char const* producer_name)
            :producer_name(producer_name)
        {
            fmt::print("Spinning up producer {}\n", producer_name);
        }

        Producer get_return_object() { return {producer_name, this}; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() { produced_data = nullptr; continuation.resume(); }
        void unhandled_exception() {}

        char const* producer_name;
        char const* produced_data = nullptr;
        std::coroutine_handle<> continuation;
    };

    char const* producer_name;
    promise_type* promise;
    Producer(char const* name, promise_type* p)
        :producer_name(name), promise(p)
    {}
};

struct Sender {
    char const* m_data;
    Sender(char const* data)
        :m_data(data)
    {}

    bool await_ready() { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Producer::promise_type> h) {
        Producer::promise_type& promise = h.promise();
        promise.produced_data = m_data;
        fmt::print("producer suspend with \"{}\"\n", m_data);
        return promise.continuation;
    }
    void await_resume() {
        fmt::print("producer resume\n");
    }
};

struct Consumer {
    struct promise_type {
        promise_type(Producer const& p)
            :producer(&p), is_done(false)
        {}
        Consumer get_return_object() { return Consumer{ *producer, std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() { is_done = true; }
        void unhandled_exception() {}

        char const* received_data = nullptr;
        Producer const* producer;
        bool is_done;
    };

    Producer const* producer;
    std::coroutine_handle<promise_type> handle;

    Consumer(Producer const& p, std::coroutine_handle<promise_type> h)
        :producer(&p), handle(h)
    {}

};

struct Receiver {
    Producer const* m_producer;

    Receiver(Producer const& p)
        :m_producer(&p)
    {}

    bool await_ready() { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Consumer::promise_type> h) {
        fmt::print("receiver suspend\n");
        m_producer->promise->continuation = h;
        return std::coroutine_handle<Producer::promise_type>::from_promise(*m_producer->promise);
    }
    char const* await_resume() {
        fmt::print("receiver resume\n");
        return m_producer->promise->produced_data;
    }
};

Producer producer(char const* producer_name)
{
    char const* arr[] = { "mary", "had", "a", "little", "lamb" };
    for (auto const& str : arr) {
        fmt::print("producer: {}\n", str);
        co_await Sender{ str };
    }
    co_return;
}

Consumer consumer(Producer const& p)
{
    for (;;) {
        char const* str = co_await Receiver{ p };
        if (str == nullptr) {
            fmt::print(" # Done.\n", str);
            co_return;
        }
        fmt::print(" # {}\n", str);
    }
}


int main()
{
    consumer(producer("producer #1"));
    fmt::print("----\n");
}

