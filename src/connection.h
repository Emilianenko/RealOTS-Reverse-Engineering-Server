#pragma once

#include "networkmessage.h"

#include <unordered_set>

static constexpr int32_t CONNECTION_WRITE_TIMEOUT = 30;
static constexpr int32_t CONNECTION_READ_TIMEOUT = 30;

class Game;
class Player;
class Server;
class Connection;

using Connection_ptr = std::shared_ptr<Connection>;
using ConnectionWeak_ptr = std::weak_ptr<Connection>;

enum ConnectionState_t
{
	CONNECTION_STATE_OPEN,
	CONNECTION_STATE_CLOSED,
};

class Connection : public std::enable_shared_from_this<Connection>
{
public:
	explicit Connection(boost::asio::io_service& io_service) :
		read_timer(io_service),
		write_timer(io_service),
		socket(io_service) {
		//
	}
	~Connection();

	void receiveData();
	void close(bool force = true);
	void closeSocket();

	void send(NetworkMessage& smsg);

	Player* getPlayer() const {
		return player;
	}
	void setPlayer(Player* new_player) {
		player = new_player;
	}

	ConnectionState_t getState() const {
		return state;
	}

	boost::asio::ip::tcp::socket& getSocket() {
		return socket;
	}

	void setSymmetricKey(uint32_t* key) {
		memcpy(symmetric_key, key, sizeof(uint32_t) * 4);
	}
	void checkCreatureID(uint32_t creature_id, bool& is_known, uint32_t& removed_id);
private:

	void parseHeader(const boost::system::error_code& error);
	void parsePacket(const boost::system::error_code& error);
	void parseData();

	void sendAll();

	void internalSend(NetworkMessage& msg);
	void onWriteOperation(const boost::system::error_code& error);

	static void handleTimeout(ConnectionWeak_ptr connection_weak, const boost::system::error_code& error);

	Player* player = nullptr;

	NetworkMessage in_message{};
	int32_t pending_data = 0;

	std::unordered_set<uint32_t> known_creatures{};
	std::list<NetworkMessage> message_queue{};
	std::recursive_mutex mutex_lock;

	boost::asio::deadline_timer read_timer;
	boost::asio::deadline_timer write_timer;
	boost::asio::ip::tcp::socket socket;

	time_t time_connected = time(nullptr);

	uint32_t packets_sent = 0;
	uint32_t symmetric_key[4]{};

	ConnectionState_t state = CONNECTION_STATE_OPEN;

	friend class Game;
};