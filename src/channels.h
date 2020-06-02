#pragma once

#include <set>

class Player;

enum ChannelType_t : uint16_t
{
	CHANNEL_GUILD,
	CHANNEL_GAMEMASTER,
	CHANNEL_TUTOR,
	CHANNEL_RULEVIOLATIONS,
	CHANNEL_GAMECHAT,
	CHANNEL_TRADE,
	CHANNEL_RLCHAT,
	CHANNEL_HELP,
	CHANNEL_PRIVATE = 0xFFFF,
};

class Channel
{
public:
	virtual ~Channel() = default;

	Channel(uint16_t channel_id, const std::string& channel_name) :
		id(channel_id), name(channel_name) {
		//
	}

	uint16_t getId() const {
		return id;
	}

	const std::string& getName() const {
		return name;
	}

	const std::vector<Player*>& getSubscribers() const {
		return subscribers;
	}

	bool isSubscribed(Player* player) const;
	virtual bool isOwnChannel(Player* player) const {
		return false;
	}

	virtual bool mayJoin(Player* player) const {
		return true;
	}

	void addSubscriber(Player* player);
	void removeSubscriber(Player* player);

	virtual void closeChannel();
private:
	uint16_t id = 0;

	std::string name;
	std::vector<Player*> subscribers{};
};

class PrivateChannel : public Channel
{
public:
	PrivateChannel(Player* player, uint16_t channel_id, const std::string& channel_name) : Channel(channel_id, channel_name) {
		owner = player;
	}

	Player* getOwner() const {
		return owner;
	}

	bool mayJoin(Player* player) const final;
	bool isInvited(uint32_t player_id) {
		return invited_players.find(player_id) != invited_players.end();
	}
	bool isOwnChannel(Player* player) const final {
		return player == owner;
	}

	void addInvitation(uint32_t player_id);
	void revokeInvitation(uint32_t player_id);

	void closeChannel() final;
private:
	Player* owner = nullptr;

	std::set<uint32_t> invited_players{};
};

class Channels
{
public:
	Channels();
	~Channels();

	void leaveChannels(Player* player);

	PrivateChannel* getPrivateChannel(Player* player);
	void closePrivateChannel(Player* player);

	Channel* getChannelById(uint16_t channel_id) const;

	std::vector<const Channel*> getChannelsForPlayer(Player* player);
private:
	void createChannel(uint16_t channel_id, const std::string& name);

	std::vector<Channel*> channels{};
	std::map<uint32_t, PrivateChannel*> private_channels{};
};

extern Channels g_channels;