
#include <iostream>
#include <cstdio>
#include <syncstream>

#include <vector>
#include <iostream>
#include <fstream>
#include <thread>
#include <filesystem>

#include <sys/time.h>
#include <sys/resource.h>

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/awaitable.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <cstdio>

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
namespace this_coro = boost::asio::this_coro;

//#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
//# define use_awaitable \
//  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
//#endif


awaitable<void> echo(tcp::socket socket)
{
  {
     osyncstream sout(std::clog);
     sout << "Waiting for message from " << socket.remote_endpoint() << endl;
  }
  try
  {
    char data[1024];
    for (;;)
    {
      osyncstream sout(std::clog);
      std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), use_awaitable);
      sout << "* Read message '" << string_view{data, n}
           << "' from " << socket.remote_endpoint() << endl;
      co_await async_write(socket, boost::asio::buffer(data, n), use_awaitable);
    }
  }
  catch (std::exception& e)
  {
      osyncstream sout(std::clog);
      sout << "echo Exception: " << e.what() << endl;
  }

  osyncstream sout(std::clog);
  sout << "Done with " << socket.remote_endpoint() << endl;
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), 55555});
  {
      osyncstream sout(std::clog);
      sout << "Entering accept loop" << endl;
  }
  for (;;)
  {
    tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
    osyncstream sout(std::clog);
    sout << "Accepted connection from " << socket.remote_endpoint() << endl;
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    osyncstream sout(std::clog);
    sout << "Exception: " << e.what() << endl;
  }
}
