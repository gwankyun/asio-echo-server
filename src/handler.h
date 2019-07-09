#pragma once
#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include <memory>
#include "session.hpp"
#include <boost/system/error_code.hpp>

void on_accept(std::shared_ptr<session_t> session, boost::system::error_code ec);
void on_read(std::shared_ptr<session_t> session, boost::system::error_code ec);
void on_write(std::shared_ptr<session_t> session, boost::system::error_code ec);
void on_wait(boost::system::error_code ec, std::shared_ptr<boost::asio::steady_timer> timer);

struct config_t
{
    std::size_t read_size = 1;
    std::size_t write_size = 1;
    std::size_t timer_interval = 1;
    std::size_t timeout = 1;
    std::map<std::string, std::shared_ptr<session_t>> manager;
    uint16_t port;
};

config_t& get_config();
