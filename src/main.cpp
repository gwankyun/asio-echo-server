#include <iostream>
#include <boost/asio.hpp>
#include <memory>
#include <algorithm>
#include <utility>
#include "session.hpp"
#include <boost/system/error_code.hpp>
#include <spdlog_easy.hpp>
#include "session.hpp"
using namespace std;

using io_context_t = boost::asio::io_context;
using acceptor_t = boost::asio::ip::tcp::acceptor;
using endpoint_t = boost::asio::ip::tcp::endpoint;
using address_t = boost::asio::ip::address;
using error_code_t = boost::system::error_code;

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
    async_read(session, 1, on_read);
    auto new_session = make_shared<session_t>(session->io_context, session->acceptor);
    async_accept(new_session, on_accept);
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
    io_context->run();
    return 0;
}
