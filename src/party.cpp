#include "pch.h"

#include "party.h"
#include "player.h"
#include "protocol.h"

bool Party::isInvited(const Player* player)
{
	return std::find(invited_players.begin(), invited_players.end(), player) != invited_players.end();
}

bool Party::isMember(const Player* player)
{
	return std::find(members.begin(), members.end(), player) != members.end();
}

void Party::addInvitation(Player* player)
{
	if (isInvited(player)) {
		fmt::printf("INFO - Party::addInvitation: player %s is already invited.\n", player->getName());
		return;
	}

	invited_players.push_back(player);
}

void Party::removeInvitation(Player* player)
{
	const auto it = std::find(invited_players.begin(), invited_players.end(), player);
	if (it == invited_players.end()) {
		fmt::printf("INFO - Party::removeInvitation: %s is not invited.\n", player->getName());
		return;
	}

	invited_players.erase(it);

	Protocol::sendCreatureParty(host->connection_ptr, player);
	Protocol::sendCreatureParty(player->connection_ptr, host);

	if (isEmpty()) {
		disband();
	}
}

void Party::addMember(Player* player)
{
	const auto it = std::find(members.begin(), members.end(), player);
	if (it != members.end()) {
		return;
	}

	members.push_back(player);
}

void Party::removeMember(Player* player)
{
	const auto it = std::find(members.begin(), members.end(), player);
	if (it == members.end()) {
		return;
	}

	members.erase(it);
}

void Party::passLeadership(Player* player)
{
	if (host == player || player->party != this) {
		return;
	}

	if (!isMember(player)) {
		Protocol::sendTextMessage(host->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s is not member of your party.", player->getName()));
		return;
	}

	Player* old_host = host;
	host = player;

	for (Player* member : members) {
		if (member == player) {
			Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, "You are now leader of your party.");
		} else {
			Protocol::sendTextMessage(member->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s is now leader of your party.", host->getName()));
		}

		Protocol::sendCreatureParty(member->connection_ptr, old_host);
		Protocol::sendCreatureParty(member->connection_ptr, player);
	}

	for (Player* invited_player : invited_players) {
		Protocol::sendCreatureParty(invited_player->connection_ptr, old_host);
		Protocol::sendCreatureParty(invited_player->connection_ptr, host);
		Protocol::sendCreatureParty(old_host->connection_ptr, invited_player);
		Protocol::sendCreatureParty(host->connection_ptr, invited_player);
	}
}

void Party::joinParty(Player* player)
{
	if (!isInvited(player)) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has not invited you.", host->getName()));
		return;
	}

	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("You have joined %s's party.", host->getName()));

	addMember(player);
	removeInvitation(player);

	Protocol::sendCreatureParty(player->connection_ptr, player);
	Protocol::sendCreatureSkull(player->connection_ptr, player);

	player->party = this;

	for (Player* member : members) {
		Protocol::sendCreatureParty(player->connection_ptr, member);
		Protocol::sendCreatureSkull(player->connection_ptr, member);

		if (member != player) {
			Protocol::sendCreatureParty(member->connection_ptr, player);
			Protocol::sendCreatureSkull(member->connection_ptr, player);

			Protocol::sendTextMessage(member->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has joined the party.", player->getName()));
		}
	}
}

void Party::leaveParty(Player* player)
{
	if (player->party != this && player != host) {
		return;
	}

	if (members.size() == 1 || members.size() == 2 && invited_players.empty()) {
		disband();
		return;
	}

	if (player == host) {
		passLeadership(members.front());
	}

	removeMember(player);
	player->party = nullptr;

	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, "You have left the party.");

	Protocol::sendCreatureSkull(player->connection_ptr, player);
	Protocol::sendCreatureParty(player->connection_ptr, player);

	for (Player* member : members) {
		Protocol::sendCreatureSkull(member->connection_ptr, player);
		Protocol::sendCreatureParty(member->connection_ptr, player);
		Protocol::sendCreatureSkull(player->connection_ptr, member);
		Protocol::sendCreatureParty(player->connection_ptr, member);
	}
}

void Party::disband()
{
	for (Player* member : members) {
		Protocol::sendTextMessage(member->connection_ptr, MESSAGE_OBJECT_INFO, "Your party has been disbanded.");
		member->party = nullptr;

		for (Player* other_member : members) {
			Protocol::sendCreatureSkull(member->connection_ptr, other_member);
			Protocol::sendCreatureParty(member->connection_ptr, other_member);
		}
	}
	members.clear();

	for (Player* invited_player : invited_players) {
		Protocol::sendCreatureParty(invited_player->connection_ptr, host);
		Protocol::sendCreatureParty(host->connection_ptr, invited_player);
	}
	invited_players.clear();

	host->party = nullptr;
	delete this;
}
