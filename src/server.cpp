#include "pch.h"

#include "server.h"
#include "game.h"

void Server::open()
{
	accept();
}

void Server::close()
{
	acceptor.close();
}

void Server::accept()
{
	auto connection = std::make_shared<Connection>(io_service);
	acceptor.async_accept(connection->getSocket(), std::bind(&Server::onAccept, this, connection, std::placeholders::_1));
}

void Server::onAccept(Connection_ptr Connection, const boost::system::error_code& error)
{
	if (error == boost::asio::error::operation_aborted) {
		return;
	}

	g_game.addConnection(Connection);
	Connection->receiveData();

	// accept next incoming Connection
	accept();
}