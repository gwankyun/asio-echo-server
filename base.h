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

class application_t;

class session_t : public std::enable_shared_from_this<session_t>
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
	bool is_write = false;
	bool is_read = false;
	shared_ptr<application_t> app;

	void clear();
	void run();

	static std::size_t read_size;
	static std::size_t write_size;

private:

};

class application_t
{
public:
	application_t();
	~application_t();

	io_context_t io_context;

private:

};

application_t::application_t()
{
}

application_t::~application_t()
{
}

template<typename H>
bool async_write(shared_ptr<session_t> session, H handler)
{
	INFO("log");
	auto &socket = session->socket;
	auto &write_queue = session->write_queue;

	if (!write_queue.empty())
	{
		auto &front = write_queue.front();
		auto &write_offset = session->write_offset;
		auto &is_write = session->is_write;
		if (!is_write)
		{
			auto size = std::min(session_t::write_size, front.size() - write_offset);
			is_write = true;
			socket.async_write_some(
				asio::buffer(front.data() + write_offset, size),
				handler);
			return true;
		}
	}
	return false;
}

template<typename H>
bool async_read(shared_ptr<session_t> session, H handler)
{
	INFO("log");
	auto &buffer = session->buffer;
	auto &socket = session->socket;
	auto &read_offset = session->read_offset;
	auto &is_read = session->is_read;

	if (!is_read)
	{
		buffer.resize(read_offset + session_t::read_size);
		is_read = true;
		socket.async_read_some(
			asio::buffer(buffer.data() + read_offset, session_t::read_size),
			handler);
		return true;
	}
	return false;
}
