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
	INFO("log", "address:{0} port:{1}", address, port);
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
		io_context.run();
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
				async_read(session,
					[session, &io_context](const error_code_t &ec, std::size_t size)
				{
					read_handler(ec, size, session, io_context);
				});
				return;
			}
		}

		async_write(session,
			[session, &io_context](const error_code_t &ec, std::size_t size)
		{
			write_handler(ec, size, session, io_context);
		});
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
		io_context.run();
	}
	else
	{
		auto &buffer = session->buffer;
		auto &socket = session->socket;
		auto &read_offset = session->read_offset;
		auto &write_queue = session->write_queue;
		vector<char> buff;
		copy_n(buffer.begin(), buffer.size(), back_inserter(buff));
		buff.push_back('\0');
		INFO("log", "async_read_some:{0}", buff.data());

		read_offset += size;

		INFO("log", "session->size:{0} g_one_size:{1} size:{2}", read_offset, session_t::read_size, size);

		INFO("log", "buffer size:{0}", buffer.size());

		if (buffer[read_offset - 1] == '\0')
		{
			INFO("log");
			INFO("log", "read all:{0}", buffer.data());
			vector<char> vec;
			copy_n(buffer.begin(), read_offset, back_inserter(vec));

			write_queue.push(std::move(vec));

			session->clear();

			async_write(session,
				[session, &io_context](const error_code_t &ec, std::size_t size)
			{
				write_handler(ec, size, session, io_context);
			});
		}
		else
		{
			INFO("log");
			async_read(session,
				[session, &io_context](const error_code_t &ec, std::size_t size)
			{
				read_handler(ec, size, session, io_context);
			});
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
		io_context.run();
	}
	else
	{
		auto &socket = session->socket;
		auto &buffer = session->buffer;
		auto re = socket.remote_endpoint();
		auto address = re.address().to_string();
		auto port = re.port();
		INFO("log", "address:{0} port:{1}", address, port);
		session->address = address;
		session->port = port;

		async_read(session,
			[session, &io_context](const error_code_t &ec, std::size_t size)
		{
			read_handler(ec, size, session, io_context);
		});

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
