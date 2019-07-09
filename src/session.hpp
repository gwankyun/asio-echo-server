#pragma once
#include <vector>
#include <memory>
#include <string>
#include <utility>
#include <map>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <spdlog_easy.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "buffer.h"

inline std::string make_uuid()
{
    auto uuid = boost::uuids::random_generator()();
    return std::move(boost::uuids::to_string(uuid));
}

template<typename Range>
std::string to_hex(const Range& range)
{
    using namespace std;
    string hex;
    boost::algorithm::hex(begin(range), end(range), back_inserter(hex));
    return std::move(hex);
}

class session_t
{
    using io_context_t = boost::asio::io_context;
    using acceptor_t = boost::asio::ip::tcp::acceptor;
    using socket_t = boost::asio::ip::tcp::socket;
public:

    session_t() = default;

    session_t(
        std::shared_ptr<io_context_t> _io_context,
        std::shared_ptr<acceptor_t> _acceptor)
        :io_context(_io_context), acceptor(_acceptor), socket(*io_context),
        id(make_uuid()), unix_time(0)
    {
        LOG(info, id);
    }

    ~session_t()
    {
        LOG(info, id);
    }

    std::shared_ptr<io_context_t> io_context;
    std::shared_ptr<acceptor_t> acceptor;
    socket_t socket;
    buffer_t read_buffer;
    buffer_t write_buffer;
    std::string id;
    uint64_t unix_time;
};

template<typename F>
void async_accept(std::shared_ptr<session_t> session, F f)
{
    using error_code_t = boost::system::error_code;
    auto acceptor = session->acceptor;
    auto& socket = session->socket;
    acceptor->async_accept(socket,
        [session, f](error_code_t ec) {
        f(session, ec);
    });
}

template<typename F>
void async_read(std::shared_ptr<session_t> session, std::size_t read_size, F f)
{
    using error_code_t = boost::system::error_code;
    auto& socket = session->socket;
    auto& read_buffer = session->read_buffer;
    auto newsize = read_buffer.offset() + read_size;
    if (newsize > read_buffer.size())
    {
        read_buffer.resize(newsize);
    }
    socket.async_read_some(
        boost::asio::buffer(read_buffer.data() + read_buffer.offset(), read_size),
        [f, session](error_code_t ec, std::size_t size) {
        if (ec)
        {
            f(session, ec);
            return;
        }
        session->read_buffer.offset() += size;
        f(session, ec);
    });
}

template<typename F>
void async_write(std::shared_ptr<session_t> session, std::size_t write_size, F f)
{
    using error_code_t = boost::system::error_code;
    auto& socket = session->socket;
    auto& write_buffer = session->write_buffer;
    socket.async_write_some(
        boost::asio::buffer(write_buffer.data() + write_buffer.offset(), write_size),
        [f, session](error_code_t ec, std::size_t size) {
        if (ec)
        {
            f(session, ec);
            return;
        }
        session->write_buffer.offset() += size;
        f(session, ec);
    });
}
