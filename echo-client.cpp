
#include <iostream>

#include <iostream>
#include <string_view>

#include <sys/time.h>
#include <sys/resource.h>

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/awaitable.hpp>

using namespace std;

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/signal_set.hpp>

using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
using boost::asio::async_connect;
namespace this_coro = boost::asio::this_coro;

void echo(const std::string& message,
          const std::string& server,
          const std::string& port) {

    co_spawn(boost::asio::system_executor(), [&]() ->awaitable<void>  {
        auto executor = get_associated_executor(use_awaitable);

        // Initialize a TCP socket
        tcp::socket s{executor};

        // Resolve the hostname and port
        tcp::resolver resolver{executor};
        auto endpoints =
            co_await resolver.async_resolve(server, port, use_awaitable);

        // Connect asynchronously to the echo server (tries all endpoints)
        co_await async_connect(s, endpoints, use_awaitable);

        // Asynchronously send the message
        co_await boost::asio::async_write(s, boost::asio::buffer(message), use_awaitable);

        // Prepare a buffer for the reply
        std::array<char, 1024> b;
        boost::asio::mutable_buffer buffer{b.data(), b.size()};

        // Asynchronously wait for and receive the reply
        const auto bytes = co_await s.async_receive(buffer, use_awaitable);

        // Check that we got the same message as we sent
        assert(bytes == message.size());
        assert(message == string_view(b.data(), bytes));

        // Exit the C++ 20 coroutine
        co_return;
    }, detached);
}

template <typename CompletionToken, typename Executor>
auto async_echox(
        Executor&& executor,
        const std::string& message,
        const std::string& server,
        const std::string& port,
        CompletionToken&& token) {

    auto initiation = [](auto && completionHandler,
            const std::string& message,
            const std::string& server,
            const std::string& port) mutable {

        co_spawn(boost::asio::system_executor(),
                 [completed=std::move(completionHandler), message, server, port]() mutable ->awaitable<void>  {
            // Initialize a TCP socket

            auto executor = get_associated_executor(use_awaitable);

            tcp::socket s{executor};

            // Resolve the hostname and port
            tcp::resolver resolver{executor};
            auto endpoints =
                co_await resolver.async_resolve(server, port, use_awaitable);

            // Connect asynchronously to the echo server (tries all endpoints)
            co_await async_connect(s, endpoints, use_awaitable);

            // Asynchronously send the message
            co_await boost::asio::async_write(s, boost::asio::buffer(message), use_awaitable);

            // Prepare a buffer for the reply
            std::array<char, 1024> b;
            boost::asio::mutable_buffer buffer{b.data(), b.size()};

            // Asynchronously wait for and receive the reply
            const auto bytes = co_await s.async_receive(buffer, use_awaitable);

            // Check that we got the same message as we sent
            assert(bytes == message.size());
            assert(message == string_view(b.data(), bytes));

            // Do the continuation magic
            completed();

            // Exit the C++ 20 coroutine
            co_return;
        }, detached);
    };

    return boost::asio::async_initiate<CompletionToken, void()> (
                initiation, token,
                message,
                server,
                port);
}

template <typename CompletionToken, typename Executor>
auto async_echo(
        Executor&& executor,
        const std::string& message,
        const std::string& server,
        const std::string& port,
        CompletionToken&& token) {

    return boost::asio::async_compose<CompletionToken, void()>(
                [&message, &server, &port, &executor](auto&& self) {

            co_spawn(executor, [message, server, port, self=std::move(self)]() mutable ->awaitable<void>  {
            // Initialize a TCP socket

            auto executor = get_associated_executor(use_awaitable);

            tcp::socket s{executor};

            // Resolve the hostname and port
            tcp::resolver resolver{executor};
            auto endpoints =
                co_await resolver.async_resolve(server, port, use_awaitable);

            // Connect asynchronously to the echo server
            co_await async_connect(s, endpoints, use_awaitable);

            // Asynchronously send the message
            co_await boost::asio::async_write(s, boost::asio::buffer(message), use_awaitable);

            // Prepare a buffer for the reply
            std::array<char, 1024> b;
            boost::asio::mutable_buffer buffer{b.data(), b.size()};

            // Asynchronously wait for and receive the reply
            const auto bytes = co_await s.async_receive(buffer, use_awaitable);

            // Check that we got the same message as we sent
            assert(bytes == message.size());
            assert(message == string_view(b.data(), bytes));

            // Do the continuation magic
            self.complete();

            // Exit the C++ 20 coroutine
            co_return;
        }, detached);
    }, token, executor);
}


int main()
{
  try
  {
    boost::asio::io_context ioCtx;

    // Call the "normal" echo()
    echo("test 1", "127.0.0.1", "55555");
    echo("test 2", "127.0.0.1", "55555");
    echo("test 3", "127.0.0.1", "55555");

    // Call async_echo() from a stackful coroutine
    const auto res = spawn(ioCtx, [](boost::asio::yield_context yield) {
        auto executor = get_associated_executor(yield);
        async_echo(executor, "async message 1", "127.0.0.1", "55555", yield);
        async_echo(executor, "async message 2", "127.0.0.1", "55555", yield);
    });

    // Call async_echo() using a future
    auto f = async_echo(ioCtx, "future message", "127.0.0.1", "55555", boost::asio::use_future);

    // Call async_echo() using a completion callback
    async_echo(ioCtx, "callback message", "127.0.0.1", "55555", []{
       ; // We could have done something now...
    });

    // Call async_echo() using C++ 20 stackless coroutine
    co_spawn(ioCtx, []() ->awaitable<void> {
        auto executor = get_associated_executor(use_awaitable);
        co_await async_echo(executor, "C++20 coro message 1", "127.0.0.1", "55555", use_awaitable);
        co_await async_echo(executor, "C++20 coro message 2", "127.0.0.1", "55555", use_awaitable);
        co_return;
    }, detached);

    // Run the pending asio tasks, until all are done.
    ioCtx.run();

    // If we use the same thread as the io_service, as here, we must call `get()` after
    // `ioCtx.run()`, so asio gets a chance to execute `async_echo()`.
    f.get();
  }
  catch (std::exception& e)
  {
    clog << "echo Exception: " << e.what() << endl;
  }
}
