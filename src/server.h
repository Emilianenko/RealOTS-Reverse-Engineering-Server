#pragma once

#include "Connection.h"
#include "config.h"

class Server
{
public:
	explicit Server(boost::asio::io_service& ioservice) :
		io_service(ioservice),
		acceptor(ioservice, boost::asio::ip::tcp::endpoint(boost::asio::ip::address(boost::asio::ip::address_v4::from_string(g_config.IP)), g_config.Port)) {
		acceptor.set_option(boost::asio::ip::tcp::no_delay(true));
	}

	void open();
	void close();
private:
	boost::asio::io_service& io_service;
	boost::asio::ip::tcp::acceptor acceptor;

	void accept();
	void onAccept(Connection_ptr Connection, const boost::system::error_code& error);
};