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

	void clear();
	void run();
	error_code_t close();

	//static std::size_t read_size;
	//static std::size_t write_size;

private:

};

session_t::session_t(io_context_t &io_context)
	: socket(io_context)
{
}

session_t::~session_t()
{
	INFO("log", "address:{0} port:{1}", address, port);
}

void session_t::clear()
{
	INFO("log");
	buffer.clear();
	read_offset = 0;
}

error_code_t session_t::close()
{
	INFO("log");
	error_code_t ec;
	socket.close(ec);
	return ec;
}

template<typename H>
bool async_write(shared_ptr<session_t> session, std::size_t size, H handler)
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
			auto wsize = std::min(size, front.size() - write_offset);
			is_write = true;
			socket.async_write_some(
				asio::buffer(front.data() + write_offset, wsize),
				handler);
			return true;
		}
	}
	return false;
}

template<typename H>
bool async_read(shared_ptr<session_t> session, std::size_t size,  H handler)
{
	INFO("log");
	auto &buffer = session->buffer;
	auto &socket = session->socket;
	auto &read_offset = session->read_offset;
	auto &is_read = session->is_read;

	if (!is_read)
	{
		buffer.resize(read_offset + size);
		is_read = true;
		socket.async_read_some(
			asio::buffer(buffer.data() + read_offset, size),
			handler);
		return true;
	}
	return false;
}
