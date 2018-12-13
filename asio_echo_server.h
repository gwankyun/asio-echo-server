#pragma once
#include "base.h"

class session_t : public session_base_t, public std::enable_shared_from_this<session_t>
{
public:
	session_t() = default;
	session_t(shared_ptr<socket_t> socket_);
	~session_t() override;

	void run() override;

private:

};

session_t::session_t(shared_ptr<socket_t> socket_)
{
	socket = socket_;
}

session_t::~session_t()
{
}

void write_handler(const error_code_t &ec,
	std::size_t size,
	shared_ptr<session_t> session);

void read_handler(const error_code_t &ec,
	std::size_t size,
	shared_ptr<session_t> session);

void accept_handler(const error_code_t &ec,
	shared_ptr<socket_t> socket,
	acceptor_t &acceptor);
