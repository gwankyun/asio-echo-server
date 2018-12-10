#pragma once
#include <cmath>

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

	void clear();

	static std::size_t read_size;
	static std::size_t write_size;

private:

};

void write_handler(const error_code_t &ec,
	std::size_t size,
	shared_ptr<session_t> session,
	io_context_t &io_context);

bool async_write(shared_ptr<session_t> session, io_context_t &io_context);

void read_handler(const error_code_t &ec,
	std::size_t size,
	shared_ptr<session_t> session,
	io_context_t &io_context);

void accept_handler(const error_code_t &ec,
	shared_ptr<session_t> session,
	io_context_t &io_context,
	acceptor_t &acceptor);

void async_read(shared_ptr<session_t> session, io_context_t &io_context);
