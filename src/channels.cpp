#include "pch.h"

#include "channels.h"
#include "player.h"

bool Channel::isSubscribed(Player* player) const
{
	const auto it = std::find(subscribers.begin(), subscribers.end(), player);
	if (it != subscribers.end()) {
		return true;
	}

	return false;
}

void Channel::addSubscriber(Player* player) {
	const auto it = std::find(subscribers.begin(), subscribers.end(), player);
	if (it != subscribers.end()) {
		return;
	}

	subscribers.push_back(player);
}

void Channel::removeSubscriber(Player* player) {
	const auto it = std::find(subscribers.begin(), subscribers.end(), player);
	if (it != subscribers.end()) {
		subscribers.erase(it);
	}
}

void Channel::closeChannel()
{
	subscribers.clear();
}

bool PrivateChannel::mayJoin(Player* player) const
{
	if (player == owner) {
		return true;
	}

	return std::find(invited_players.begin(), invited_players.end(), player->getId()) != invited_players.end();
}

void PrivateChannel::addInvitation(uint32_t player_id)
{
	invited_players.emplace(player_id);
}

void PrivateChannel::revokeInvitation(uint32_t player_id)
{
	invited_players.erase(player_id);
}

void PrivateChannel::closeChannel()
{
	Channel::closeChannel();
	invited_players.clear();
}

Channels::Channels()
{
	createChannel(CHANNEL_HELP, "Help");
	createChannel(CHANNEL_TUTOR, "Tutor");
	createChannel(CHANNEL_GAMECHAT, "Game-Chat");
	createChannel(CHANNEL_RLCHAT, "RL-Chat");
	createChannel(CHANNEL_TRADE, "Trade");
	createChannel(CHANNEL_GAMEMASTER, "Gamemaster");
	createChannel(CHANNEL_RULEVIOLATIONS, "Rule Violations");
}

Channels::~Channels()
{
	for (Channel* channel : channels) {
		delete channel;
	}

	for (auto it : private_channels) {
		delete it.second;
	}
}

void Channels::leaveChannels(Player* player)
{
	for (Channel* channel : channels) {
		channel->removeSubscriber(player);
	}

	for (const auto it : private_channels) {
		PrivateChannel* channel = it.second;
		if (channel->getOwner() == player) {
			channel->closeChannel();
		} else {
			channel->removeSubscriber(player);
		}
	}
}

PrivateChannel* Channels::getPrivateChannel(Player* player)
{
	static uint16_t next_private_channel_id = CHANNEL_HELP + 1;

	for (const auto it : private_channels) {
		PrivateChannel* channel = it.second;
		if (channel->getOwner() == player) {
			return channel;
		}
	}

	if (next_private_channel_id >= std::numeric_limits<uint16_t>::max() - 1) {
		fmt::printf("ERROR - Channels::getPrivateChannel: no valid unique id available (%d).\n", next_private_channel_id);
		return nullptr;
	}

	PrivateChannel* channel = new PrivateChannel(player, next_private_channel_id++, fmt::sprintf("%s's Private Channel", player->getName()));
	private_channels[player->getId()] = channel;
	return channel;
}

void Channels::closePrivateChannel(Player* player)
{
	PrivateChannel* channel = getPrivateChannel(player);
	if (channel == nullptr) {
		return;
	}

	channel->closeChannel();
}

Channel* Channels::getChannelById(uint16_t channel_id) const
{
	for (Channel* channel : channels) {
		if (channel->getId() == channel_id) {
			return channel;
		}
	}

	for (const auto it : private_channels) {
		PrivateChannel* channel = it.second;
		if (channel->getId() == channel_id) {
			return channel;
		}
	}

	return nullptr;
}

std::vector<const Channel*> Channels::getChannelsForPlayer(Player* player)
{
	std::vector<const Channel*> player_channels;
	for (const Channel* channel : channels) {
		if (channel->mayJoin(player)) {
			player_channels.push_back(channel);
		}
	}

	PrivateChannel* private_channel = g_channels.getPrivateChannel(player);
	if (private_channel) {
		player_channels.push_back(private_channel);
	}

	for (const auto it : private_channels) {
		const PrivateChannel* channel = it.second;
		if (!channel->isOwnChannel(player) && channel->mayJoin(player)) {
			player_channels.push_back(channel);
		}
	}

	return player_channels;
}

void Channels::createChannel(uint16_t channel_id, const std::string & name)
{
	Channel* channel = new Channel(channel_id, name);
	channels.push_back(channel);
}
