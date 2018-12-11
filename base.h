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

class session_t
{
public:
	session_t(io_context_t &io_context);
	~session_t();

	socket_t socket;
	vector<char> buffer;
	std::size_t read_offset = 0;
	std::size_t write_offset = 0;
	queue<vector<char>> write_queue;
	string address;
	uint16_t port = 0;

	void clear();

	static std::size_t read_size;
	static std::size_t write_size;

private:

};

template<typename H>
bool async_write(shared_ptr<session_t> session, io_context_t &io_context, H handler)
{
	INFO("log");
	auto &socket = session->socket;
	auto &write_queue = session->write_queue;
	if (!write_queue.empty())
	{
		auto &front = write_queue.front();
		auto &write_offset = session->write_offset;
		auto size = std::min(session_t::write_size, front.size() - write_offset);
		socket.async_write_some(
			asio::buffer(front.data() + write_offset, size),
			[session, &io_context, handler](const error_code_t &ec, std::size_t size)
		{
			handler(ec, size, session, io_context);
		});
		return true;
	}
	else
	{
		return false;
	}
}

template<typename H>
void async_read(shared_ptr<session_t> session, io_context_t &io_context, H handler)
{
	INFO("log");
	auto &buffer = session->buffer;
	auto &socket = session->socket;
	auto &read_offset = session->read_offset;
	buffer.resize(read_offset + session_t::read_size);
	socket.async_read_some(
		asio::buffer(buffer.data() + read_offset, session_t::read_size),
		[session, &io_context, handler](const error_code_t &ec, std::size_t size)
	{
		handler(ec, size, session, io_context);
	});
}
