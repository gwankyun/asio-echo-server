// asio_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

//#include "pch.h"
#include "asio_echo_server.h"

std::size_t session_t::read_size = 2;
std::size_t session_t::write_size = 2;

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
	read_offset = 0;
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
		auto &write_offset = session->write_offset;
		auto &socket = session->socket;
		auto &write_queue = session->write_queue;
		auto &front = write_queue.front();
		auto &buffer = session->buffer;

		INFO("log", "write size:{0}", size);

		write_offset += size;

		if (write_offset == front.size())
		{
			write_offset = 0;
			write_queue.pop();
			if (write_queue.empty())
			{
				session->clear();
				async_read(session, io_context, read_handler);
				return;
			}
		}
		async_write(session, io_context, write_handler);
	}
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

		session->read_offset += size;

		INFO("log", "session->size:{0} g_one_size:{1} size:{2}", session->read_offset, session_t::read_size, size);

		if (size == session_t::read_size)
		{
			INFO("log");
			async_read(session, io_context, read_handler);
		}
		else if (size < session_t::read_size)
		{
			INFO("log");
			vector<char> vec;
			copy_n(buffer.begin(), session->read_offset, back_inserter(vec));

			session->write_queue.push(std::move(vec));

			async_write(session, io_context, write_handler);

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

		async_read(session, io_context, read_handler);

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
