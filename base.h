#pragma once
#include <cmath>
#include <cstdint>

#include <iostream>
#include <memory>
#include <utility>
#include <string>
#include <vector>
#include <algorithm>
#include <queue>
#include <functional>
#include <type_traits>

#include <boost/asio.hpp>

#include <boost/system/error_code.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog_easy.hpp>

using namespace std;

namespace asio = boost::asio;

using io_context_t = asio::io_context;
using tcp_t = asio::ip::tcp;
using socket_t = tcp_t::socket;
using address_t = asio::ip::address;
using endpoint_t = tcp_t::endpoint;
using acceptor_t = tcp_t::acceptor;
using resolver_t = tcp_t::resolver;
using error_code_t = boost::system::error_code;

class session_base_t
{
public:
	session_base_t() = default;
	session_base_t(shared_ptr<socket_t> socket_);
	virtual ~session_base_t();

	shared_ptr<socket_t> socket;
	vector<char> buffer;
	std::size_t read_offset = 0;
	std::size_t write_offset = 0;
	queue<vector<char>> write_queue;
	string address;
	uint16_t port = 0;
	bool is_write = false;
	bool is_read = false;

	void clear();
	virtual void run() = 0;
	error_code_t close();

private:

};

session_base_t::session_base_t(shared_ptr<socket_t> socket_)
{
	socket = socket_;
}

session_base_t::~session_base_t()
{
	INFO("log", "address:{0} port:{1}", address, port);
}

void session_base_t::clear()
{
	INFO("log");
	buffer.clear();
	read_offset = 0;
}

error_code_t session_base_t::close()
{
	INFO("log");
	error_code_t ec;
	socket->close(ec);
	return ec;
}

template<typename S, typename H>
bool async_write(shared_ptr<S> session, std::size_t size, H handler)
{
	using namespace std;
	INFO("log");
	static_assert(is_base_of<session_base_t, S>::value);
	auto &socket = session->socket;
	auto &write_queue = session->write_queue;

	if (!write_queue.empty())
	{
		auto &front = write_queue.front();
		auto &write_offset = session->write_offset;
		auto &is_write = session->is_write;
		if (!is_write)
		{
			auto wsize = std::min(size, front.size() - write_offset);
			is_write = true;
			socket->async_write_some(
				asio::buffer(front.data() + write_offset, wsize),
				handler);
			return true;
		}
	}
	return false;
}

template<typename S, typename H>
bool async_read(shared_ptr<S> session, std::size_t size,  H handler)
{
	using namespace std;
	INFO("log");
	static_assert(is_base_of<session_base_t, S>::value);
	auto &buffer = session->buffer;
	auto &socket = session->socket;
	auto &read_offset = session->read_offset;
	auto &is_read = session->is_read;

	if (!is_read)
	{
		buffer.resize(read_offset + size);
		is_read = true;
		socket->async_read_some(
			asio::buffer(buffer.data() + read_offset, size),
			handler);
		return true;
	}
	return false;
}
