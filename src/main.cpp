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

std::map<std::string, std::shared_ptr<session_t>> session_t::manager;

uint64_t get_unix_time()
{
    uint64_t time_last;
    time_last = time(nullptr);
    return time_last;
}
using namespace std;

using io_context_t = boost::asio::io_context;
using acceptor_t = boost::asio::ip::tcp::acceptor;
using endpoint_t = boost::asio::ip::tcp::endpoint;
using address_t = boost::asio::ip::address;
using error_code_t = boost::system::error_code;
using steady_timer_t = boost::asio::steady_timer;

void on_accept(shared_ptr<session_t> session, error_code_t ec);
void on_read(shared_ptr<session_t> session, error_code_t ec);
void on_write(shared_ptr<session_t> session, error_code_t ec);

void on_write(shared_ptr<session_t> session, error_code_t ec)
{
    LOG(info);
    if (ec)
    {
        LOG(info, ec.message());
        return;
    }
    session->unix_time = get_unix_time();
    auto& write_buffer = session->write_buffer;
    //LOG(info, "offset:{0} size:{1}", write_buffer.offset(), write_buffer.size());
    //assert(write_buffer.offset() <= write_buffer.offset());
    if (write_buffer.offset() == write_buffer.size())
    {
        write_buffer.clear();
        async_read(session, 1, on_read);
    }
    else
    {
        LOG(info);
        async_write(session, 1, on_write);
    }
}

void on_read(shared_ptr<session_t> session, error_code_t ec)
{
    LOG(info);
    if (ec)
    {
        LOG(info, ec.message());
        //session->io_context->run();
        return;
    }
    session->unix_time = get_unix_time();
    auto& read_buffer = session->read_buffer;
    //string hex = to_hex(read_buffer);
    //boost::algorithm::hex(
    //    read_buffer.data(),
    //    read_buffer.data() + read_buffer.offset(),
    //    back_inserter(hex));
    //LOG(info, "hex:{0}", hex);
    auto last = read_buffer.data() + read_buffer.offset() - 1;
    //LOG(info, "last:{0}", *last);
    if (*last == '\n')
    {
        auto& write_buffer = session->write_buffer;
        write_buffer.resize(read_buffer.offset());
        copy_n(read_buffer.data(), read_buffer.offset(), write_buffer.data());
        //string hex;
        //copy_n(read_buffer.data(), read_buffer.offset(), back_inserter(write_buffer));
        //boost::algorithm::hex(
        //    write_buffer.data(),
        //    write_buffer.data() + write_buffer.offset(),
        //    back_inserter(hex));
        //LOG(info, "hex:{0} {1} {2}", hex, write_buffer.offset(), write_buffer.size());
        //assert(false);
        read_buffer.clear();
        async_write(session, 1, on_write);
    }
    else
    {
        async_read(session, 1, on_read);
    }
}

void on_accept(shared_ptr<session_t> session, error_code_t ec)
{
    LOG(info);
    if (ec)
    {
        LOG(info, ec.message());
        return;
    }
    session_t::manager[session->id] = session;
    session->unix_time = get_unix_time();
    async_read(session, 1, on_read);
    auto new_session = make_shared<session_t>(session->io_context, session->acceptor);
    async_accept(new_session, on_accept);
}

void on_wait(error_code_t ec, shared_ptr<steady_timer_t> timer)
{
    LOG(info, "time out");
    for (auto i = session_t::manager.begin(); i != session_t::manager.end();)
    {
        auto session = i->second;
        auto unix_time = session->unix_time;
        LOG(info, "session:{0}", i->first);
        LOG(info, "unix_time:{0}", unix_time);
        auto now_time = get_unix_time();
        LOG(info, now_time - unix_time);
        if (now_time - unix_time > 5)
        {
            session->socket.close();
            auto current = i;
            i++;
            session_t::manager.erase(current);
        }
        else
        {
            i++;
        }
    }

    timer->expires_from_now(chrono::seconds(3));
    timer->async_wait(
        [timer](error_code_t ec)
    {
        on_wait(ec, timer);
    });
}

int main()
{
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
    timer->expires_from_now(chrono::seconds(3));
    timer->async_wait(
        [timer](error_code_t ec)
    {
        on_wait(ec, timer);
    });
    io_context->run();
    return 0;
}
