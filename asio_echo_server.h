#pragma once
#include "base.h"

void write_handler(const error_code_t &ec,
	std::size_t size,
	shared_ptr<session_t> session);

void read_handler(const error_code_t &ec,
	std::size_t size,
	shared_ptr<session_t> session);

void accept_handler(const error_code_t &ec,
	shared_ptr<session_t> session,
	acceptor_t &acceptor);
