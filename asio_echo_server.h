#pragma once
#include "base.h"

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
