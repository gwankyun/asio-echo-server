// asio_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

//#include "pch.h"
#include "asio_echo_server.h"

std::size_t session_t::read_size = 2;

session_t::session_t(io_context_t &io_context)
	: socket(io_context)
{
}

session_t::~session_t()
{
	INFO("log");
}

void session_t::clear()
{
	INFO("log");
	buffer.clear();
	size = 0;
}

void write_handler(const error_code_t &ec,
	std::size_t size,
	shared_ptr<session_t> session,
	io_context_t &io_context)
{
	INFO("log");
	if (ec)
	{
		INFO("log", ec.message());
	}
	else
	{
		auto &socket = session->socket;
		auto &write_queue = session->write_queue;
		auto &buffer = session->buffer;
		auto &front = write_queue.front();
		INFO("log", "write size:{0}", size);
		write_queue.pop();
		async_read(session, io_context);
	}
}

bool async_write(shared_ptr<session_t> session, io_context_t &io_context)
{
	INFO("log");
	auto &socket = session->socket;
	auto &write_queue = session->write_queue;
	if (!write_queue.empty())
	{
		auto &front = write_queue.front();
		INFO("log", "front size:{0}", front.size());
		socket.async_write_some(asio::buffer(front),
			[session, &io_context](const error_code_t &ec, std::size_t size)
		{
			write_handler(ec, size, session, io_context);
		});
		return true;
	}
	else
	{
		return false;
	}
}

void async_read(shared_ptr<session_t> session, io_context_t &io_context)
{
	INFO("log");
	auto &buffer = session->buffer;
	auto &socket = session->socket;
	buffer.resize(session->size + session_t::read_size);
	socket.async_read_some(asio::buffer(buffer.data() + session->size, session_t::read_size),
		[session, &io_context](const error_code_t &ec, std::size_t size)
	{
		read_handler(ec, size, session, io_context);
	});
}

void read_handler(const error_code_t &ec,
	std::size_t size,
	shared_ptr<session_t> session,
	io_context_t &io_context)
{
	INFO("log");
	if (ec)
	{
		INFO("log", ec.message());
	}
	else
	{
		auto &buffer = session->buffer;
		auto &socket = session->socket;
		vector<char> buff;
		copy_n(buffer.begin(), buffer.size(), back_inserter(buff));
		buff.push_back('\0');
		INFO("log", "async_read_some:{0}", buff.data());

		session->size += size;

		INFO("log", "session->size:{0} g_one_size:{1} size:{2}", session->size, session_t::read_size, size);

		if (size == session_t::read_size)
		{
			INFO("log");
			async_read(session, io_context);
		}
		else if (size < session_t::read_size)
		{
			INFO("log");
			vector<char> vec;
			copy_n(buffer.begin(), session->size, back_inserter(vec));

			session->write_queue.push(std::move(vec));

			async_write(session, io_context);

			session->clear();
		}
	}
}

void accept_handler(const error_code_t &ec, 
	shared_ptr<session_t> session,
	io_context_t &io_context,
	acceptor_t &acceptor)
{
	if (ec)
	{
		INFO("log", ec.message());
	}
	else
	{
		auto &socket = session->socket;
		auto &buffer = session->buffer;
		auto re = socket.remote_endpoint();
		auto address = re.address().to_string();
		auto port = re.port();
		INFO("log", "address:{0} port:{1}", address, port);

		async_read(session, io_context);

		auto next_session = make_shared<session_t>(io_context);

		acceptor.async_accept(next_session->socket, 
			[next_session, &io_context, &acceptor](const error_code_t &ec)
		{
			accept_handler(ec, next_session, io_context, acceptor);
		});
	}
}

int main()
{
	auto logger = spdlog::stdout_color_mt("log");
	io_context_t io_context;

	acceptor_t acceptor(io_context,
		endpoint_t(address_t::from_string("0.0.0.0"), 12500));

	auto session = make_shared<session_t>(io_context);

	acceptor.async_accept(session->socket, 
		[session, &io_context, &acceptor](const error_code_t &ec)
	{
		accept_handler(ec, session, io_context, acceptor);
	});

	io_context.run();
	return 0;
}
