#include "pch.h"

#include "connection.h"
#include "protocol.h"
#include "game.h"
#include "player.h"

Connection::~Connection()
{
	closeSocket();
}

void Connection::receiveData()
{
	try {
		read_timer.expires_from_now(boost::posix_time::seconds(CONNECTION_READ_TIMEOUT));
		read_timer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(shared_from_this()), std::placeholders::_1));

		// Read size of the first packet
		boost::asio::async_read(socket,
			boost::asio::buffer(in_message.getBuffer(), NetworkMessage::HEADER_LENGTH),
			std::bind(&Connection::parseHeader, shared_from_this(), std::placeholders::_1));
	} catch (boost::system::system_error& e) {
		fmt::printf("ERROR - Connection::receiveData: %s.\n", e.what());
		close();
	}
}

void Connection::close(bool force)
{
	if (state != CONNECTION_STATE_OPEN) {
		return;
	}

	state = CONNECTION_STATE_CLOSED;

	if (message_queue.empty() || force) {
		closeSocket();
	}
}

void Connection::send(NetworkMessage& smsg)
{
	if (state != CONNECTION_STATE_OPEN) {
		return;
	}

	message_queue.emplace_back(smsg);
}

void Connection::checkCreatureID(uint32_t creature_id, bool& is_known, uint32_t& removed_id)
{
	const auto result = known_creatures.insert(creature_id);
	if (!result.second) {
		is_known = true;
		return;
	}

	is_known = false;

	if (known_creatures.size() > 150) {
		// Look for a creature to remove
		for (auto it = known_creatures.begin(), end = known_creatures.end(); it != end; ++it) {
			Creature* creature = g_game.getCreatureById(creature_id);
			if (!player->canSeeCreature(creature)) {
				removed_id = *it;
				known_creatures.erase(it);
				return;
			}
		}

		// Bad situation. Let's just remove anyone.
		auto it = known_creatures.begin();
		if (*it == creature_id) {
			++it;
		}

		removed_id = *it;
		known_creatures.erase(it);
	} else {
		removed_id = 0;
	}
}

void Connection::parseHeader(const boost::system::error_code& error)
{
	read_timer.cancel();

	if (error) {
		close();
		return;
	}

	if (state != CONNECTION_STATE_OPEN) {
		return;
	}

	const uint32_t time_passed = std::max<uint32_t>(1, (time(nullptr) - time_connected) + 1);
	if ((++packets_sent / time_passed) > static_cast<uint32_t>(15)) {
		fmt::printf("INFO - Connection::parseHeader: %s disconnected for exceeding packet per second limit.\n", socket.local_endpoint().address().to_string());
		close();
		return;
	}

	if (time_passed > 2) {
		time_connected = time(nullptr);
		packets_sent = 0;
	}

	const uint16_t size = in_message.getLengthHeader();
	if (size == 0 || size >= NETWORKMESSAGE_MAXSIZE - 16) {
		close();
		return;
	}

	try {
		read_timer.expires_from_now(boost::posix_time::seconds(CONNECTION_READ_TIMEOUT));
		read_timer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(shared_from_this()),
			std::placeholders::_1));

		// Read packet content
		in_message.setLength(size + NetworkMessage::HEADER_LENGTH);
		boost::asio::async_read(socket, boost::asio::buffer(in_message.getBuffer(), size),
			std::bind(&Connection::parsePacket, shared_from_this(), std::placeholders::_1));
	} catch (boost::system::system_error& e) {
		fmt::printf("ERROR - Connection::parseHeader: %s.\n", e.what());
		close();
	}
}

void Connection::parsePacket(const boost::system::error_code& error)
{
	read_timer.cancel();

	if (error) {
		close();
		return;
	}

	if (state != CONNECTION_STATE_OPEN) {
		return;
	}

	pending_data++;
	g_game.callGame();
}

void Connection::parseData()
{
	if (pending_data <= 0) {
		return;
	}

	pending_data--;

	in_message.setPosition(0);

	if (player) {
		if (!in_message.xteaDecrypt(symmetric_key)) {
			fmt::printf("INFO - Connection::parseData: %s sent a packet that failed to decrypt with RSA.\n", player->getName());
		}

		Protocol::parseCommand(shared_from_this(), in_message);
	} else {
		const uint8_t protocol_type = in_message.readByte();
		if (protocol_type == 0x01) {
			Protocol::parseCharacterList(shared_from_this(), in_message);
		} else if (protocol_type == 0x0A) {
			Protocol::parseCharacterLogin(shared_from_this(), in_message);
		} else {
			fmt::printf("INFO - Connection::parseData: unknown protocol %d.\n", protocol_type);
		}
	}

	receiveData();
}

void Connection::sendAll()
{
	std::lock_guard<std::recursive_mutex> lockClass(mutex_lock);

	for (NetworkMessage& msg : message_queue) {
		internalSend(msg);
	}
	message_queue.clear();
}

void Connection::onWriteOperation(const boost::system::error_code& error)
{
	std::lock_guard<std::recursive_mutex> lockClass(mutex_lock);

	write_timer.cancel();

	if (error) {
		message_queue.clear();
		close();
		return;
	}

	if (state == CONNECTION_STATE_CLOSED) {
		closeSocket();
	}
}

void Connection::closeSocket()
{
	if (socket.is_open()) {
		try {
			if (player) {
				player->setConnection(nullptr);
				player = nullptr;
			}

			message_queue.clear();
			read_timer.cancel();
			write_timer.cancel();
			boost::system::error_code error;
			socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			socket.close(error);
		} catch (boost::system::system_error& e) {
			fmt::printf("ERROR - Connection::closeSocket: %s.\n", e.what());
		}
	}
}

void Connection::internalSend(NetworkMessage& msg)
{
	std::lock_guard<std::recursive_mutex> lockClass(mutex_lock);

	// prepare packet
	msg.writeHeader();
	msg.xteaEncrypt(symmetric_key);
	msg.writeHeader();

	try {
		write_timer.expires_from_now(boost::posix_time::seconds(CONNECTION_WRITE_TIMEOUT));
		write_timer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(shared_from_this()),
			std::placeholders::_1));

		boost::asio::async_write(socket,
			boost::asio::buffer(msg.getBuffer(), msg.getLength()),
			std::bind(&Connection::onWriteOperation, shared_from_this(), std::placeholders::_1));
	} catch (boost::system::system_error& e) {
		fmt::printf("ERROR - Connection::internalSend: %s.\n", e.what());
		close();
	}
}

void Connection::handleTimeout(ConnectionWeak_ptr connection_weak, const boost::system::error_code& error)
{
	if (error == boost::asio::error::operation_aborted) {
		//The timer has been manually cancelled
		return;
	}

	if (auto Connection = connection_weak.lock()) {
		Connection->close();
	}
}
