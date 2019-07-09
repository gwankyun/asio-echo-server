#include <iostream>
#include <boost/asio.hpp>
#include <memory>
#include <algorithm>
#include <utility>
#include "session.hpp"
#include <boost/system/error_code.hpp>
#include <spdlog_easy.hpp>
#include "session.hpp"
#include <ctime>

#include "handler.h"

using namespace std;

using io_context_t = boost::asio::io_context;
using acceptor_t = boost::asio::ip::tcp::acceptor;
using endpoint_t = boost::asio::ip::tcp::endpoint;
using address_t = boost::asio::ip::address;
using error_code_t = boost::system::error_code;
using steady_timer_t = boost::asio::steady_timer;

int main()
{
    auto& config = get_config();
    config.read_size = 2;
    config.write_size = 2;
    config.timer_interval = 3;
    config.timeout = 10;
    spdlog::easy::init();
    auto io_context = make_shared<io_context_t>();
    auto acceptor = make_shared<acceptor_t>(
        *io_context,
        endpoint_t(address_t::from_string("0.0.0.0"), 12345));
    {
        auto session = make_shared<session_t>(io_context, acceptor);
        async_accept(session, on_accept);
    }
    auto timer = make_shared<steady_timer_t>(*io_context);
    timer->expires_from_now(chrono::seconds(config.timer_interval));
    timer->async_wait(
        [timer](error_code_t ec)
    {
        on_wait(ec, timer);
    });
    io_context->run();
    return 0;
}
