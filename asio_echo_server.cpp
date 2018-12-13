// asio_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

//#include "pch.h"
#include "asio_echo_server.h"

void session_t::run()
{
	INFO("log");
	auto self(session_t::shared_from_this());
	auto r = async_read(self, 2,
		[self](const error_code_t &ec, std::size_t size)
	{
		read_handler(ec, size, self);
	});
}

void write_handler(const error_code_t &ec,
	std::size_t size,
	shared_ptr<session_t> session)
{
	INFO("log");
	if (ec)
	{
		INFO("log", ec.message());
		session->close();
		return;
	}
	else
	{
		auto &write_offset = session->write_offset;
		auto &socket = session->socket;
		auto &write_queue = session->write_queue;
		auto &front = write_queue.front();
		auto &buffer = session->buffer;
		session->is_write = false;

		INFO("log", "write size:{0}", size);

		write_offset += size;

		if (write_offset == front.size())
		{
			write_offset = 0;
			write_queue.pop();
			if (write_queue.empty())
			{
				session->clear();
				auto r = async_read(session, 2,
					[session](const error_code_t &ec, std::size_t size)
				{
					read_handler(ec, size, session);
				});
				if (!r)
				{
					session->socket->get_io_context().run();
					return;
				}
				return;
			}
		}

		auto w = async_write(session, 2,
			[session](const error_code_t &ec, std::size_t size)
		{
			write_handler(ec, size, session);
		});
		if (!w)
		{
			session->socket->get_io_context().run();
			return;
		}
	}
}

void read_handler(const error_code_t &ec,
	std::size_t size,
	shared_ptr<session_t> session)
{
	INFO("log");
	if (ec)
	{
		INFO("log", ec.message());
		session->close();
		return;
	}
	else
	{
		auto &buffer = session->buffer;
		auto &socket = session->socket;
		auto &read_offset = session->read_offset;
		auto &write_queue = session->write_queue;
		session->is_read = false;

		vector<char> buff;
		copy_n(buffer.begin(), buffer.size(), back_inserter(buff));
		buff.push_back('\0');
		INFO("log", "async_read_some:{0}", buff.data());

		read_offset += size;

		//INFO("log", "session->size:{0} g_one_size:{1} size:{2}", read_offset, session_t::read_size, size);

		INFO("log", "buffer size:{0}", buffer.size());

		if (buffer[read_offset - 1] == '\0')
		{
			INFO("log");
			INFO("log", "read all:{0}", buffer.data());
			vector<char> vec;
			copy_n(buffer.begin(), read_offset, back_inserter(vec));

			write_queue.push(std::move(vec));

			session->clear();

			auto w = async_write(session, 2,
				[session](const error_code_t &ec, std::size_t size)
			{
				write_handler(ec, size, session);
			});
			if (!w)
			{
				session->socket->get_io_context().run();
				return;
			}
		}
		else
		{
			INFO("log");
			auto r = async_read(session, 2,
				[session](const error_code_t &ec, std::size_t size)
			{
				read_handler(ec, size, session);
			});
			if (!r)
			{
				session->socket->get_io_context().run();
				return;
			}
		}
	}
}

void accept_handler(const error_code_t &ec, 
	shared_ptr<socket_t> socket,
	acceptor_t &acceptor)
{
	INFO("log");
	if (ec)
	{
		INFO("log", ec.message());
		socket->close();
	}
	else
	{
		auto session = make_shared<session_t>(socket);
		auto &buffer = session->buffer;
		auto re = socket->remote_endpoint();
		auto address = re.address().to_string();
		auto port = re.port();
		INFO("log", "address:{0} port:{1}", address, port);
		session->address = address;
		session->port = port;

		session->run();
	}
	auto next_socket = make_shared<socket_t>(socket->get_io_context());

	acceptor.async_accept(*next_socket,
		[next_socket, &acceptor](const error_code_t &ec)
	{
		accept_handler(ec, next_socket, acceptor);
	});
}

int main()
{
	auto logger = spdlog::stdout_color_mt("log");
	io_context_t io_context;

	acceptor_t acceptor(io_context,
		endpoint_t(address_t::from_string("0.0.0.0"), 12500));

	{
		auto socket = make_shared<socket_t>(io_context);

		acceptor.async_accept(*socket,
			[socket, &acceptor](const error_code_t &ec)
		{
			accept_handler(ec, socket, acceptor);
		});
	}

	io_context.run();
	return 0;
}
