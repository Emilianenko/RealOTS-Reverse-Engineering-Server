#include "pch.h"

#include "Protocol.h"
#include "game.h"
#include "player.h"
#include "config.h"
#include "map.h"
#include "item.h"
#include "tools.h"
#include "channels.h"

void Protocol::parseCharacterList(Connection_ptr connection, NetworkMessage& msg)
{
	msg.skipBytes(16);

	if (!msg.rsaDecrypt()) {
		connection->close();
		return;
	}

	uint32_t symmetric_key[4];
	symmetric_key[0] = msg.readQuad();
	symmetric_key[1] = msg.readQuad();
	symmetric_key[2] = msg.readQuad();
	symmetric_key[3] = msg.readQuad();
	connection->setSymmetricKey(symmetric_key);

	if (g_game.getGameState() == GAME_OFFLINE) {
		sendLoginDisconnect(connection, 0x0A, "The server is not online.\nPlease try again later.");
		return;
	}

	if (g_game.getGameState() == GAME_STARTING) {
		sendLoginDisconnect(connection, 0x0A, "The g_game is just starting.\nPlease try again later.");
		return;
	}

	if (g_game.getGameState() == GAME_SAVING) {
		sendLoginDisconnect(connection, 0x0A, "The g_game is just going down.\nPlease try again later.");
		return;
	}

	msg.readQuad();
	msg.readString();

	sendCharacterList(connection);
}

void Protocol::parseCharacterLogin(Connection_ptr connection, NetworkMessage& msg)
{
	msg.skipBytes(4);

	if (!msg.rsaDecrypt()) {
		connection->close();
		return;
	}

	uint32_t symmetric_key[4];
	symmetric_key[0] = msg.readQuad();
	symmetric_key[1] = msg.readQuad();
	symmetric_key[2] = msg.readQuad();
	symmetric_key[3] = msg.readQuad();
	connection->setSymmetricKey(symmetric_key);

	msg.readByte();
	const uint32_t account_number = msg.readQuad();
	msg.readString();
	msg.readString();

	if (g_game.getGameState() == GAME_OFFLINE) {
		sendLoginDisconnect(connection, 0x14, "The server is not online.\nPlease try again later.");
		return;
	}

	if (g_game.getGameState() == GAME_STARTING) {
		sendLoginDisconnect(connection, 0x14, "The g_game is just starting.\nPlease try again later.");
		return;
	}

	if (g_game.getGameState() == GAME_SAVING) {
		sendLoginDisconnect(connection, 0x14, "The g_game is just going down.\nPlease try again later.");
		return;
	}

	Player* player = g_game.getPlayerByUserId(account_number);
	if (player == nullptr) {
		player = g_game.getStoredPlayer(account_number);
		if (player == nullptr) {
			player = new Player();

			if (!player->loadData(account_number)) {
				delete player;
				sendLoginDisconnect(connection, 0x14, "Invalid player data.");
				return;
			}

			g_game.storePlayer(player, account_number);
		}

		Position pos = player->getPosition();

		player->setConnection(connection);
		connection->setPlayer(player);

		if (!g_map.searchFreeField(player, pos, 1)) {
			pos = g_game.getNewbieStart();
		}

		if (g_game.setCreatureOnMap(player, pos.x, pos.y, pos.z) != ALLGOOD) {
			sendLoginDisconnect(connection, 0x14, "Invalid position for player.");
		}
	} else {
		player->takeOver(connection);
	}
}

void Protocol::parseCommand(Connection_ptr connection, NetworkMessage& msg)
{
	Player* player = connection->getPlayer();
	if (!player) {
		fmt::printf("ERROR - Protocol::ParseCommand: player is null.\n");
		return;
	}

	const uint8_t command = msg.readByte();

	if (player->is_dead || player->removed || player->getHitpoints() <= 0) {
		if (command == 20) {
			connection->closeSocket();
		}

		// cannot execute actions
		return;
	}

	if (command != 30 && command != 105 && command != 190 && command != 202) {
		player->timestamp_action = g_game.getRoundNr();
	}

	switch (command) {
		case 20: parseLogout(connection, player); break;
		case 30: parsePing(connection, player); break;
		case 100: parseGoPath(connection, player, msg); break;
		case 101: parseGoDirection(connection, player, DIRECTION_NORTH); break;
		case 102: parseGoDirection(connection, player, DIRECTION_EAST); break;
		case 103: parseGoDirection(connection, player, DIRECTION_SOUTH); break;
		case 104: parseGoDirection(connection, player, DIRECTION_WEST); break;
		case 105: parseStop(connection, player); break;
		case 106: parseGoDirection(connection, player, DIRECTION_NORTHEAST); break;
		case 107: parseGoDirection(connection, player, DIRECTION_SOUTHEAST); break;
		case 108: parseGoDirection(connection, player, DIRECTION_SOUTHWEST); break;
		case 109: parseGoDirection(connection, player, DIRECTION_NORTHWEST); break;
		case 111: parseRotate(connection, player, DIRECTION_NORTH); break;
		case 112: parseRotate(connection, player, DIRECTION_EAST); break;
		case 113: parseRotate(connection, player, DIRECTION_SOUTH); break;
		case 114: parseRotate(connection, player, DIRECTION_WEST); break;
		case 120: parseMoveObject(connection, player, msg); break;
		case 125: parseRequestTrade(connection, player, msg); break;
		case 126: parseInspectTrade(connection, player, msg); break;
		case 127: parseAcceptTrade(connection, player, msg); break;
		case 128: parseCloseTrade(connection, player, msg); break;
		case 130: parseUseObject(connection, player, msg); break;
		case 131: parseUseTwoObjects(connection, player, msg); break;
		case 132: parseUseOnCreature(connection, player, msg); break;
		case 133: parseTurnOject(connection, player, msg); break;
		case 135: parseCloseContainer(connection, player, msg); break;
		case 136: parseUpContainer(connection, player, msg); break;
		case 137: parseEditText(connection, player, msg); break;
		// 138 ParseEditList
		case 140: parseLookAtPoint(connection, player, msg); break;
		case 150: parseTalk(connection, player, msg); break;
		case 151: parseRequestChannels(connection, player); break;
		case 152: parseJoinChannel(connection, player, msg); break;
		case 153: parseLeaveChannel(connection, player, msg); break;
		case 154: parseOpenPrivateChannel(connection, player, msg); break;
		case 155: parseProcessRuleViolationReport(connection, player, msg); break;
		case 156: parseCloseRuleViolationReport(connection, player, msg); break;
		case 157: parseCancelRuleViolationReport(connection, player); break;
		case 160: parseTactics(connection, player, msg); break;
		case 161: parseAttack(connection, player, msg); break;
		case 162: parseFollow(connection, player, msg); break;
		case 163: parseInviteToParty(connection, player, msg); break;
		case 164: parseJoinParty(connection, player, msg); break;
		case 165: parseRevokePartyInvitation(connection, player, msg); break;
		case 166: parsePassPartyLeadership(connection, player, msg); break;
		case 167: parseLeaveParty(connection, player, msg); break;
		case 170: parseOpenChannel(connection, player, msg); break;
		case 171: parseInviteToChannel(connection, player, msg); break;
		case 172: parseExcludeFromChannel(connection, player, msg); break;
		case 190: parseCancel(connection, player, msg); break;
		case 201: parseRefreshTile(connection, player, msg); break;
		// 0xCA ParseRefreshContainer
		case 210: parseRequestOutfits(connection, player); break;
		case 211: parseSetOutfit(connection, player, msg); break;
		case 230: parseBugReport(connection, player, msg); break;
		case 232: parseErrorFileEntry(connection, player, msg); break;
		default:
			fmt::printf("INFO - Protocol::ParseCommand: %s sent unknown command (%d)\n", player->getName(), command);
			break;
	}
}

void Protocol::parseLogout(Connection_ptr connection, Player* player)
{
	if (player->earliest_logout_round > g_game.getRoundNr()) {
		Protocol::sendResult(connection, YOUMAYNOTLOGOUTDURINGAFIGHT);
		return;
	}

	g_game.removeCreature(player);
	connection->close(false);
}

void Protocol::parsePing(Connection_ptr connection, Player* player)
{
	player->last_pong = g_game.serverMilliseconds();
}

void Protocol::parseGoPath(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const uint8_t total_directions = msg.readByte();
	if (total_directions == 0 || (msg.getPosition() + total_directions) - 2 != msg.getLength() || total_directions > 20) {
		fmt::printf("INFO - Protocol::ParseGoPath: %s sent too many or invalid directions (directions:%d).\n", player->getName(), total_directions);
		return;
	}

	if (player->toDoClear()) {
		sendSnapback(connection);
	}

	const Position& start_pos = player->getPosition();
	int32_t x = start_pos.x;
	int32_t y = start_pos.y;
	const int32_t z = start_pos.z;

	for (uint8_t i = 0; i < total_directions; i++) {
		const uint8_t rawdir = msg.readByte();

		ToDoEntry entry;
		entry.code = TODO_GO;

		switch (rawdir) {
			case 1:
				x++;
				break;
			case 2: {
				x++;
				y--;
				break;
			}
			case 3:
				y--;
				break;
			case 4: {
				x--;
				y--;
				break;
			}
			case 5:
				x--;
				break;
			case 6:
				y++;
				x--;
				break;
			case 7:
				y++;
				break;
			case 8: {
				x++;
				y++;
				break;
			}
			default: break;
		}

		entry.to_pos.x = x;
		entry.to_pos.y = y;
		entry.to_pos.z = z;
		player->toDoAdd(entry);
	}

	player->toDoStart();
}

void Protocol::parseGoDirection(Connection_ptr connection, Player* player, Direction_t look_direction)
{
	const Position& pos = player->getPosition();
	const int32_t x = pos.x;
	const int32_t y = pos.y;
	const int32_t z = pos.z;

	if (player->toDoClear()) {
		sendSnapback(connection);
	}

	int32_t dx = 0;
	int32_t dy = 0;

	switch (look_direction)
	{
		case DIRECTION_NORTH:
			dy--;
			break;
		case DIRECTION_EAST:
			dx++;
			break;
		case DIRECTION_WEST:
			dx--;
			break;
		case DIRECTION_SOUTH:
			dy++;
			break;
		case DIRECTION_NORTHEAST: {
			dy--;
			dx++;
			break;
		}
		case DIRECTION_NORTHWEST: {
			dy--;
			dx--;
			break; }
		case DIRECTION_SOUTHEAST: {
			dy++;
			dx++;
			break;
		}
		case DIRECTION_SOUTHWEST: {
			dy++;
			dx--;
			break;
		}
		default: break;
	}

	ToDoEntry entry;
	entry.code = TODO_GO;
	entry.to_pos.x = x + dx;
	entry.to_pos.y = y + dy;
	entry.to_pos.z = z;
	player->toDoAdd(entry);
	player->toDoStart();
}

void Protocol::parseStop(Connection_ptr connection, Player* player)
{
	player->toDoStop();
}

void Protocol::parseRotate(Connection_ptr connection, Player* player, Direction_t look_direction)
{
	if (player->toDoClear()) {
		sendSnapback(connection);
	}

	player->toDoRotate(look_direction);
	player->toDoStart();
}

void Protocol::parseMoveObject(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const Position from_pos = readPosition(msg);
	const uint16_t type_id = msg.readWord();
	const uint8_t index = msg.readByte();
	const Position to_pos = readPosition(msg);
	const uint8_t amount = msg.readByte();

	if (from_pos == to_pos) {
		fmt::printf("INFO - Protocol::ParseMoveObject: %s attempt to move object to it's same position (typeid:%d).\n", player->getName(), type_id);
		return;
	}

	if (player->toDoClear()) {
		sendSnapback(connection);
	}

	if (type_id == 99) {
		player->toDoWait(1000);
	}

	player->toDoMoveObject(from_pos, type_id, index, to_pos, amount);
	player->toDoStart();
}

void Protocol::parseRequestTrade(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const Position pos = readPosition(msg);
	const uint16_t type_id = msg.readWord();
	const uint8_t index = msg.readByte();
	const uint32_t player_id = msg.readQuad();

	player->toDoTrade(pos, type_id, index, player_id);
	player->toDoStart();
}

void Protocol::parseInspectTrade(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const bool counter_offer = msg.readByte() == 0x01;
	const uint8_t index = msg.readByte();
	g_game.playerInspectTrade(player, counter_offer, index);
}

void Protocol::parseAcceptTrade(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	g_game.playerAcceptTrade(player);
}

void Protocol::parseCloseTrade(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	g_game.closeTrade(player);
}

void Protocol::parseUseObject(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const Position pos = readPosition(msg);
	const uint16_t type_id = msg.readWord();
	const uint8_t index = msg.readByte();
	const uint8_t container_id = msg.readByte();

	player->toDoWait(100);
	player->toDoUse(pos, type_id, index, container_id);
	player->toDoStart();
}

void Protocol::parseUseTwoObjects(Connection_ptr connection, Player * player, NetworkMessage & msg)
{
	const Position pos = readPosition(msg);
	const uint16_t type_id = msg.readWord();
	const uint8_t index = msg.readByte();
	const Position to_pos = readPosition(msg);
	const uint16_t to_type_id = msg.readWord();
	const uint8_t to_index = msg.readByte();

	player->toDoWait(100);
	player->toDoUseTwoObjects(pos, type_id, index, to_pos, to_type_id, to_index);
	player->toDoStart();
}

void Protocol::parseUseOnCreature(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const Position pos = readPosition(msg);
	const uint16_t type_id = msg.readWord();
	const uint8_t index = msg.readByte();
	const uint32_t creature_id = msg.readQuad();

	player->toDoWait(100);
	player->toDoUseTwoObjects(pos, type_id, index, creature_id);
	player->toDoStart();
}

void Protocol::parseTurnOject(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const Position pos = readPosition(msg);
	const uint16_t type_id = msg.readWord();
	const uint8_t index = msg.readByte();

	player->toDoWait(100);
	player->toDoTurnObject(pos, type_id, index);
	player->toDoStart();
}

void Protocol::parseCloseContainer(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const uint8_t container_id = msg.readByte();
	g_game.playerCloseContainer(player, container_id);
}

void Protocol::parseUpContainer(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const uint8_t container_id = msg.readByte();
	g_game.playerUpContainer(player, container_id);
}

void Protocol::parseEditText(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const uint32_t edit_text_id = msg.readQuad();
	const std::string text = msg.readString();

	g_game.playerEditText(player, edit_text_id, text);
}

void Protocol::parseLookAtPoint(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const Position pos = readPosition(msg);
	const uint16_t type_id = msg.readWord();
	const uint8_t index = msg.readByte();
	g_game.playerLookAtObject(player, pos, type_id, index);
}

void Protocol::parseTalk(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	std::string address;
	uint16_t channel_id;

	const TalkType_t type = static_cast<TalkType_t>(msg.readByte());
	switch (type) {
		case TALKTYPE_PRIVATE:
		case TALKTYPE_PRIVATE_RED:
		case TALKTYPE_RVR_ANSWER:
			address = msg.readString();
			channel_id = 0;
			break;

		case TALKTYPE_CHANNEL_Y:
		case TALKTYPE_CHANNEL_R1:
		case TALKTYPE_CHANNEL_R2:
			channel_id = msg.readWord();
			break;

		default:
			channel_id = 0;
			break;
	}

	const std::string text = msg.readString();
	if (text.length() > 255) {
		fmt::printf("INFO - Protocol::ParseTalk: %s sent a big text message: '%s'.\n", text);
		return;
	}

	player->toDoTalk(text, address, channel_id, type, true);
	player->toDoStart();
}

void Protocol::parseRequestChannels(Connection_ptr connection, Player* player)
{
	g_game.playerRequestChannels(player);
}

void Protocol::parseJoinChannel(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	g_game.playerJoinChannel(player, msg.readWord());
}

void Protocol::parseLeaveChannel(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	g_game.playerCloseChannel(player, msg.readWord());
}

void Protocol::parseOpenPrivateChannel(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	g_game.playerOpenPrivateChannel(player, msg.readString());
}

void Protocol::parseProcessRuleViolationReport(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const std::string reporter = msg.readString();
	g_game.playerProcessRuleViolationReport(player, reporter);
}

void Protocol::parseCloseRuleViolationReport(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const std::string reporter = msg.readString();
	g_game.playerCloseRuleViolationReport(player, reporter);
}

void Protocol::parseCancelRuleViolationReport(Connection_ptr connection, Player* player)
{
	g_game.playerCancelRuleViolationReport(player);
}

void Protocol::parseTactics(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const FightMode_t fight_mode = static_cast<FightMode_t>(msg.readByte());
	const ChaseMode_t chase_mode = static_cast<ChaseMode_t>(msg.readByte());
	const uint8_t secure_mode = msg.readByte();

	player->setFightMode(fight_mode);
	player->setChaseMode(chase_mode);
	player->setSecureMode(secure_mode);

	player->earliest_attack_time = g_game.serverMilliseconds() + 2000;
}

void Protocol::parseAttack(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const uint32_t creature_id = msg.readQuad();
	g_game.creatureAttack(player, creature_id, false);
}

void Protocol::parseFollow(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const uint32_t creature_id = msg.readQuad();
	g_game.creatureAttack(player, creature_id, true);
}

void Protocol::parseInviteToParty(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const uint32_t creature_id = msg.readQuad();
	g_game.playerInviteToParty(player, creature_id);
}

void Protocol::parseJoinParty(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const uint32_t creature_id = msg.readQuad();
	g_game.playerJoinParty(player, creature_id);
}

void Protocol::parseRevokePartyInvitation(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const uint32_t creature_id = msg.readQuad();
	g_game.playerRevokePartyInvitation(player, creature_id);
}

void Protocol::parsePassPartyLeadership(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const uint32_t creature_id = msg.readQuad();
	g_game.playerPassPartyLeadership(player, creature_id);
}

void Protocol::parseLeaveParty(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	g_game.playerLeaveParty(player);
}

void Protocol::parseOpenChannel(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	g_game.playerOpenChannel(player);
}

void Protocol::parseInviteToChannel(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const std::string name = msg.readString();
	g_game.playerInviteToChannel(player, name);
}

void Protocol::parseExcludeFromChannel(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const std::string name = msg.readString();
	g_game.playerExcludeFromChannel(player, name);
}

void Protocol::parseCancel(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	if (player->toDoClear()) {
		sendSnapback(connection);
	}

	player->attacked_creature = nullptr;
	sendClearTarget(connection);
	player->toDoYield();
	player->toDoStart();
}

void Protocol::parseRefreshTile(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const Position pos = readPosition(msg);
	if (!player->canSeePosition(pos)) {
		return;
	}

	sendRefreshField(connection, g_map.getTile(pos));
}

void Protocol::parseRequestOutfits(Connection_ptr connection, Player* player)
{
	sendOutfitWindow(connection);
}

void Protocol::parseSetOutfit(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	Outfit outfit;
	outfit.look_id = msg.readWord();
	outfit.head = msg.readByte();
	outfit.body = msg.readByte();
	outfit.legs = msg.readByte();
	outfit.feet = msg.readByte();

	if (player->skill_illusion->getTiming() > 0) {
		player->setOriginalOutfit(outfit);
	} else {
		player->setOriginalOutfit(outfit);
		player->setCurrentOutfit(outfit);
	}

	g_game.announceChangedCreature(player, CREATURE_OUTFIT);
}

void Protocol::parseBugReport(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const std::string text = msg.readString(1024);

	const Position& pos = player->getPosition();
	fmt::printf("bug-report: %s (%d,%d,%d): %s\n", player->getName(), pos.x, pos.y, pos.z, text);
}

void Protocol::parseErrorFileEntry(Connection_ptr connection, Player* player, NetworkMessage& msg)
{
	const std::string title = msg.readString(65);
	const std::string date = msg.readString(27);
	const std::string stack = msg.readString(2049);
	const std::string comment = msg.readString(513);

	fmt::printf("client-error: %s - %s - %s: %s\n", date, title, stack, comment);
}

void Protocol::addCreature(Connection_ptr connection, NetworkMessage& msg, const Creature* creature, bool is_known, uint32_t old_creature, bool update_follows)
{
	if (update_follows) {
		msg.writeWord(0x63);
		msg.writeQuad(creature->getId());
		msg.writeByte(creature->getLookDirection());
		return;
	} else if (is_known) {
		msg.writeWord(0x62);
		msg.writeQuad(creature->getId());
	} else {
		msg.writeWord(0x61);
		msg.writeQuad(old_creature);
		msg.writeQuad(creature->getId());
		msg.writeString(creature->getName());
	}

	msg.writeByte(100);
	msg.writeByte(creature->getLookDirection());

	addOutfit(msg, creature->getCurrentOutfit());

	uint8_t brightness, color;
	creature->getLight(brightness, color);
	msg.writeByte(brightness);
	msg.writeByte(color);

	msg.writeWord(creature->getSpeed());

	Player* player = connection->getPlayer();
	if (const Player* other_player = creature->getPlayer()) {
		msg.writeByte(other_player->getKillingMark(player));
		msg.writeByte(other_player->getPartyMark(player));
	} else {
		msg.writeByte(0x00);
		msg.writeByte(0x00);
	}
}

void Protocol::addOutfit(NetworkMessage& msg, const Outfit& outfit)
{
	msg.writeWord(outfit.look_id);

	if (outfit.look_id) {
		msg.writeByte(outfit.head);
		msg.writeByte(outfit.body);
		msg.writeByte(outfit.legs);
		msg.writeByte(outfit.feet);
	} else {
		msg.writeWord(outfit.type_id);
	}
}

void Protocol::addPosition(NetworkMessage& msg, const Position& position)
{
	addPosition(msg, position.x, position.y, position.z);
}

void Protocol::addPosition(NetworkMessage& msg, int32_t x, int32_t y, int32_t z)
{
	msg.writeWord(x);
	msg.writeWord(y);
	msg.writeByte(z);
}

void Protocol::addItem(NetworkMessage& msg, const Item* item)
{
	if (item->getFlag(DISGUISE)) {
		msg.writeWord(item->getAttribute(DISGUISETARGET));
	} else {
		msg.writeWord(item->getId());
	}

	if (item->getFlag(CUMULATIVE)) {
		msg.writeByte(item->getAttribute(ITEM_AMOUNT));
	} else if (item->getFlag(LIQUIDCONTAINER) || item->getFlag(LIQUIDPOOL)) {
		msg.writeByte(getLiquidColor(item->getAttribute(ITEM_LIQUID_TYPE)));
	}
}

Position Protocol::readPosition(NetworkMessage& msg)
{
	Position pos;
	pos.x = msg.readWord();
	pos.y = msg.readWord();
	pos.z = msg.readByte();
	return pos;
}

void Protocol::sendLoginDisconnect(Connection_ptr connection, uint8_t protocol, const std::string& text)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(protocol);
	msg.writeString(text);
	connection->send(msg);
	connection->close(false);
}

void Protocol::sendCharacterList(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x64);
	msg.writeByte(0x01);
	msg.writeString("User File");
	msg.writeString(g_config.World);
	msg.writeQuad(inet_addr("127.0.0.1"));
	msg.writeWord(7171);
	msg.writeWord(0x00);
	connection->send(msg);
	connection->close(false);
}

void Protocol::sendInitGame(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	const Player* player = connection->getPlayer();

	NetworkMessage msg;
	msg.writeByte(0x0A);
	msg.writeQuad(player->getId());
	msg.writeWord(0x32);
	msg.writeByte(0x01); // Can report bugs?
	addMapDescription(connection, msg, player->getPosition());
	connection->send(msg);

	if (Item* item = player->getInventoryItem(INVENTORY_HEAD)) {
		Protocol::sendSetInventory(connection, INVENTORY_HEAD, item);
	}

	if (Item* item = player->getInventoryItem(INVENTORY_AMULET)) {
		Protocol::sendSetInventory(connection, INVENTORY_AMULET, item);
	}

	if (Item* item = player->getInventoryItem(INVENTORY_CONTAINER)) {
		Protocol::sendSetInventory(connection, INVENTORY_CONTAINER, item);
	}
	if (Item* item = player->getInventoryItem(INVENTORY_BODY)) {
		Protocol::sendSetInventory(connection, INVENTORY_BODY, item);
	}

	if (Item* item = player->getInventoryItem(INVENTORY_RIGHT)) {
		Protocol::sendSetInventory(connection, INVENTORY_RIGHT, item);
	}

	if (Item* item = player->getInventoryItem(INVENTORY_LEFT)) {
		Protocol::sendSetInventory(connection, INVENTORY_LEFT, item);
	}

	if (Item* item = player->getInventoryItem(INVENTORY_LEGS)) {
		Protocol::sendSetInventory(connection, INVENTORY_LEGS, item);
	}

	if (Item* item = player->getInventoryItem(INVENTORY_FEET)) {
		Protocol::sendSetInventory(connection, INVENTORY_FEET, item);
	}

	if (Item* item = player->getInventoryItem(INVENTORY_RING)) {
		Protocol::sendSetInventory(connection, INVENTORY_RING, item);
	}

	if (Item* item = player->getInventoryItem(INVENTORY_EXTRA)) {
		Protocol::sendSetInventory(connection, INVENTORY_EXTRA, item);
	}

	Protocol::sendSkills(connection);
}

void Protocol::sendPing(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x1E);
	connection->send(msg);
}

void Protocol::sendStats(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	const Player* player = connection->getPlayer();

	NetworkMessage msg;
	msg.writeByte(0xA0);
	msg.writeWord(player->skill_hitpoints->getValue());
	msg.writeWord(player->skill_hitpoints->getMax());
	msg.writeWord(static_cast<uint16_t>(std::max<uint16_t>(0, player->getFreeCapacity() / 100)));
	if (player->skill_experience[SKILL_LEVEL] >= std::numeric_limits<uint32_t>::max() - 1) {
		msg.writeQuad(0);
	} else {
		msg.writeQuad(std::min<uint32_t>(std::numeric_limits<uint32_t>::max(), player->skill_experience[SKILL_LEVEL]));
	}
	msg.writeWord(player->skill_level[SKILL_LEVEL]);
	msg.writeByte(player->skill_percent[SKILL_LEVEL]);
	msg.writeWord(player->skill_manapoints->getValue());
	msg.writeWord(player->skill_manapoints->getMax());
	msg.writeByte(player->skill_level[SKILL_MAGIC]);
	msg.writeByte(player->skill_percent[SKILL_MAGIC]); 
	msg.writeByte(player->skill_soulpoints->getSoulpoints());
	connection->send(msg);
}

void Protocol::sendSkills(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	const Player* player = connection->getPlayer();

	NetworkMessage msg;
	msg.writeByte(0xA1);
	msg.writeByte(player->skill_level[SKILL_FISTFIGHTING]);
	msg.writeByte(player->skill_percent[SKILL_FISTFIGHTING]);
	msg.writeByte(player->skill_level[SKILL_CLUBFIGHTING]);
	msg.writeByte(player->skill_percent[SKILL_CLUBFIGHTING]);
	msg.writeByte(player->skill_level[SKILL_SWORDFIGHTING]);
	msg.writeByte(player->skill_percent[SKILL_SWORDFIGHTING]);
	msg.writeByte(player->skill_level[SKILL_AXEFIGHTING]);
	msg.writeByte(player->skill_percent[SKILL_AXEFIGHTING]);
	msg.writeByte(player->skill_level[SKILL_DISTANCEFIGHTING]);
	msg.writeByte(player->skill_percent[SKILL_DISTANCEFIGHTING]);
	msg.writeByte(player->skill_level[SKILL_SHIELDING]);
	msg.writeByte(player->skill_percent[SKILL_SHIELDING]);
	msg.writeByte(player->skill_level[SKILL_FISHING]);
	msg.writeByte(player->skill_percent[SKILL_FISHING]);
	connection->send(msg);
}

void Protocol::sendSnapback(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xB5);
	msg.writeByte(connection->getPlayer()->getLookDirection());
	connection->send(msg);
}

void Protocol::sendOutfitWindow(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	const Player* player = connection->getPlayer();
	NetworkMessage msg;
	msg.writeByte(0xC8);

	const Outfit current_outfit = player->getOriginalOutfit();
	addOutfit(msg, current_outfit);

	if (player->getSex() == SEX_MALE) {
		msg.writeWord(128);
		msg.writeWord(134);
	} else {
		msg.writeWord(136);
		msg.writeWord(142);
	}

	connection->send(msg);
}

void Protocol::sendSetInventory(Connection_ptr connection, uint8_t slot, const Item * item)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x78);
	msg.writeByte(slot);
	addItem(msg, item);
	connection->send(msg);
}

void Protocol::sendDeleteInventory(Connection_ptr connection, uint8_t slot)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x79);
	msg.writeByte(slot);
	connection->send(msg);
}

void Protocol::sendTextMessage(Connection_ptr connection, uint8_t type, const std::string& text)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xB4);
	msg.writeByte(type);
	msg.writeString(text);
	connection->send(msg);
}

void Protocol::sendResult(Connection_ptr connection, ReturnValue_t ret)
{
	if (!connection) {
		return;
	}

	sendTextMessage(connection, MESSAGE_STATUS_SMALL, getReturnMessage(ret));
}

void Protocol::sendEditText(Connection_ptr connection, const Item* item, uint32_t edit_text_id)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x96);
	msg.writeQuad(edit_text_id);
	addItem(msg, item);

	const std::string& text = item->getText();

	if (item->getFlag(WRITE) || item->getFlag(WRITEONCE)) {
		if (item->getFlag(WRITE)) {
			msg.writeWord(item->getAttribute(MAXLENGTH));
			msg.writeString(text);
		} else if (item->getFlag(WRITEONCE)) {
			if (text.empty()) {
				msg.writeWord(item->getAttribute(MAXLENGTHONCE));
				msg.writeString(text);
			} else {
				msg.writeWord(text.size());
				msg.writeString(text);
			}
		}
	} else if (!item->getFlag(INFORMATION) || item->getAttribute(INFORMATIONTYPE) != 4) {
		msg.writeWord(text.size());
		msg.writeString(text);
	}

	if (item->getEditor().empty()) {
		msg.writeWord(0x00);
	} else {
		msg.writeString(item->getEditor());
	}

	connection->send(msg);
}

void Protocol::sendPlayerState(Connection_ptr connection, uint8_t state)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xA2);
	msg.writeByte(state);
	connection->send(msg);
}

void Protocol::sendClearTarget(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xA3);
	connection->send(msg);
}

void Protocol::sendMarkCreature(Connection_ptr connection, uint32_t creature_id, uint8_t color)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x86);
	msg.writeQuad(creature_id);
	msg.writeByte(color);
	connection->send(msg);
}

void Protocol::sendTradeItemRequest(Connection_ptr connection, const std::string& name, const Item* item, bool acknowledge)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;

	if (acknowledge) {
		msg.writeByte(0x7D);
	} else {
		msg.writeByte(0x7E);
	}

	msg.writeString(name);

	if (item->getFlag(CONTAINER)) {
		std::vector<const Item*> containers{ item };
		std::vector<const Item*> items{ item };

		while (!containers.empty()) {
			const Item* container = containers.back();
			containers.pop_back();

			for (Item* container_item : container->getItems()) {
				if (container_item->getFlag(CONTAINER)) {
					containers.push_back(container_item);
				}
				items.push_back(container_item);
			}
		}

		msg.writeByte(std::min<int32_t>(100, items.size()));
		int32_t i = 0;
		for (const Item* ex_item : items) {
			if (++i >= 100) {
				break;
			}
			addItem(msg, ex_item);
		}
	} else {
		msg.writeByte(0x01);
		addItem(msg, item);
	}

	connection->send(msg);
}

void Protocol::sendTradeClose(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x7F);
	connection->send(msg);
}

void Protocol::sendChannels(Connection_ptr connection, const std::vector<const Channel*>& channels)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xAB);
	msg.writeByte(channels.size());
	for (const Channel* channel : channels) {
		if (channel->isOwnChannel(connection->getPlayer())) {
			msg.writeWord(CHANNEL_PRIVATE);
			msg.writeString("Private Chat Channel");
		} else {
			msg.writeWord(channel->getId());
			msg.writeString(channel->getName());
		}
	}
	connection->send(msg);
}

void Protocol::sendChannel(Connection_ptr connection, const Channel* channel)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xAC);
	msg.writeWord(channel->getId());
	msg.writeString(channel->getName());
	connection->send(msg);
}

void Protocol::sendPrivateChannel(Connection_ptr connection, const std::string& address)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xAD);
	msg.writeString(address);
	connection->send(msg);
}

void Protocol::sendOpenOwnChannel(Connection_ptr connection, const Channel* channel)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xB2);
	msg.writeWord(channel->getId());
	if (channel->isOwnChannel(connection->getPlayer())) {
		msg.writeString("Private Chat Channel");
	} else {
		msg.writeString(channel->getName());
	}
	connection->send(msg);
}

void Protocol::sendCloseChannel(Connection_ptr connection, const Channel* channel)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xB3);
	msg.writeWord(channel->getId());
	connection->send(msg);
}

void Protocol::sendLockRuleViolation(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xB1);
	connection->send(msg);
}

void Protocol::sendRuleViolationChannel(Connection_ptr connection, uint16_t channel_id)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xAE);
	msg.writeWord(channel_id);
	for (const auto it : g_game.getRuleViolationReports()) {
		const RuleViolationEntry& entry = it.second;
		if (entry.available) {
			msg.writeByte(0xAA);
			msg.writeQuad(it.first);
			msg.writeString(entry.player->getName());
			msg.writeByte(TALKTYPE_RVR_CHANNEL);
			msg.writeQuad(entry.timestamp);
			msg.writeString(entry.text);
		}
	}
	connection->send(msg);
}

void Protocol::sendRemoveRuleViolationReport(Connection_ptr connection, const std::string& reporter)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xAF);
	msg.writeString(reporter);
	connection->send(msg);
}

void Protocol::sendRuleViolationCancel(Connection_ptr connection, const std::string& reporter)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xB0);
	msg.writeString(reporter);
	connection->send(msg);
}

void Protocol::sendTalk(Connection_ptr connection, uint32_t statement_id, const std::string& sender, TalkType_t type, uint16_t channel_id, const std::string& text)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xAA);
	msg.writeQuad(statement_id);
	msg.writeString(sender);
	msg.writeByte(type);
	msg.writeWord(channel_id);
	msg.writeString(text);
	connection->send(msg);
}

void Protocol::sendTalk(Connection_ptr connection, uint32_t statement_id, const std::string& sender, TalkType_t type, const std::string& text, uint32_t data)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xAA);
	msg.writeQuad(statement_id);
	msg.writeString(sender);
	msg.writeByte(type);

	if (type == TALKTYPE_RVR_CHANNEL) {
		msg.writeQuad(data);
	}

	msg.writeString(text);
	connection->send(msg);
}

void Protocol::sendTalk(Connection_ptr connection, uint32_t statement_id, const std::string& sender, TalkType_t type, const std::string& text, const Position& pos)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0xAA);
	msg.writeQuad(statement_id);
	msg.writeString(sender);
	msg.writeByte(type);
	addPosition(msg, pos);
	msg.writeString(text);
	connection->send(msg);
}

void Protocol::sendContainer(Connection_ptr connection, uint8_t container_id, const Item* item)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x6E);
	msg.writeByte(container_id);
	addItem(msg, item);
	msg.writeString(item->getName(-1));
	msg.writeByte(item->getAttribute(CAPACITY));
	msg.writeByte(item->hasParentContainer());
	
	const std::deque<Item*> items = item->getItems();

	const int32_t total_items = std::min<uint8_t>(item->getAttribute(CAPACITY), items.size());
	msg.writeByte(total_items);

	int32_t id = 0;
	for (Item* container_item : items) {
		addItem(msg, container_item);
		if (id >= total_items) {
			break;
		}

		id++;
	}

	connection->send(msg);
}

void Protocol::sendCloseContainer(Connection_ptr connection, uint8_t container_id)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x6F);
	msg.writeByte(container_id);
	connection->send(msg);
}

void Protocol::sendCreateInContainer(Connection_ptr connection, uint8_t container_id, const Item* item)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x70);
	msg.writeByte(container_id);
	addItem(msg, item);
	connection->send(msg);
}

void Protocol::sendChangeInContainer(Connection_ptr connection, uint8_t container_id, uint8_t index, const Item* item)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x71);
	msg.writeByte(container_id);
	msg.writeByte(index);
	addItem(msg, item);
	connection->send(msg);
}

void Protocol::sendDeleteInContainer(Connection_ptr connection, uint8_t container_id, uint8_t index)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x72);
	msg.writeByte(container_id);
	msg.writeByte(index);
	connection->send(msg);
}

void Protocol::addMapDescription(Connection_ptr connection, NetworkMessage& msg, const Position& position)
{
	msg.writeByte(0x64);
	addPosition(msg, position);
	addMapFloors(connection, msg, position.x - 8, position.y - 6, position.z, 18, 14);
}

void Protocol::addMapFloors(Connection_ptr connection, NetworkMessage& msg, int32_t x, int32_t y, int32_t z, int32_t width, int32_t height)
{
	int32_t skip = -1;
	int32_t startz, endz, zstep;

	if (z > 7) {
		startz = z - 2;
		endz = std::min<int32_t>(15, z + 2);
		zstep = 1;
	} else {
		startz = 7;
		endz = 0;
		zstep = -1;
	}

	for (int32_t nz = startz; nz != endz + zstep; nz += zstep) {
		addMapRow(connection, msg, x, y, nz, width, height, z - nz, skip);
	}

	if (skip >= 0) {
		msg.writeByte(skip);
		msg.writeByte(0xFF);
	}
}

void Protocol::addMapRow(Connection_ptr connection, NetworkMessage& msg, int32_t x, int32_t y, int32_t z, int32_t width, int32_t height, int32_t offset, int32_t& skip)
{
	for (int32_t nx = 0; nx < width; nx++) {
		for (int32_t ny = 0; ny < height; ny++) {
			Tile* tile = g_map.getTile(x + nx + offset, y + ny + offset, z);
			if (tile) {
				if (skip >= 0) {
					msg.writeByte(skip);
					msg.writeByte(0xFF);
				}

				skip = 0;
				addMapPoint(connection, msg, tile);
			} else if (skip == 0xFE) {
				msg.writeByte(0xFF);
				msg.writeByte(0xFF);
				skip = -1;
			} else {
				++skip;
			}
		}
	}
}

void Protocol::addMapPoint(Connection_ptr connection, NetworkMessage& msg, const Tile* tile)
{
	uint8_t count = 0;
	for (Object* object : tile->getObjects()) {
		count++;
		addMapObject(connection, msg, object);
		if (count == 10) {
			break;
		}
	}
}

void Protocol::addMapObject(Connection_ptr connection, NetworkMessage& msg, const Object* object, bool update)
{
	if (const Item* item = object->getItem()) {
		addItem(msg, item);
	} else if (const Creature* creature = object->getCreature()) {
		bool is_known = false;
		uint32_t removed_id = 0;
		connection->checkCreatureID(creature->getId(), is_known, removed_id);
		addCreature(connection, msg, creature, is_known, removed_id, update);
	}
}

void Protocol::sendMoveCreature(Connection_ptr connection, const Creature* creature, const Position& from_pos, int32_t from_index, const Position& to_pos, int32_t to_index)
{
	if (!connection) {
		return;
	}

	Player* player = connection->getPlayer();
	if (player->canSeePosition(from_pos) && player->canSeePosition(to_pos)) {
		const int32_t dx = from_pos.x - to_pos.x;
		const int32_t dy = from_pos.y - to_pos.y;
		const int32_t dz = from_pos.z - to_pos.z;

		if ((dx + 1) > 2 || (dy + 1) > 2 || (dz + 1) > 1 || from_index >= 10) {
			sendDeleteField(connection, from_pos, from_index);
			sendAddField(connection, creature);
		} else {
			NetworkMessage msg;
			msg.writeByte(0x6D);
			addPosition(msg, from_pos);
			msg.writeByte(from_index);
			addPosition(msg, to_pos);
			connection->send(msg);
		}
	} else if (player->canSeePosition(from_pos)) {
		sendDeleteField(connection, from_pos, from_index);
	} else if (player->canSeePosition(to_pos)) {
		sendAddField(connection, creature);
	}
}

void Protocol::sendAmbiente(Connection_ptr connection)
{
	if (!connection) {
		return;
	}

	uint8_t brightness, color;
	g_game.getAmbiente(brightness, color);

	NetworkMessage msg;
	msg.writeByte(0x82);
	msg.writeByte(brightness);
	msg.writeByte(color);
	connection->send(msg);
}

void Protocol::sendRefreshField(Connection_ptr connection, const Tile * tile)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x69);
	addPosition(msg, tile->getPosition());

	if (tile) {
		addMapPoint(connection, msg, tile);
		msg.writeByte(0x00);
		msg.writeByte(0xFF);
	} else {
		msg.writeByte(0x01);
		msg.writeByte(0xFF);
	}

	connection->send(msg);
}

void Protocol::sendAddField(Connection_ptr connection, const Object* object)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x6A);
	addPosition(msg, object->getPosition());
	addMapObject(connection, msg, object);
	connection->send(msg);
}

void Protocol::sendDeleteField(Connection_ptr connection, const Position& position, int32_t index)
{
	if (index >= 10 || !connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x6C);
	addPosition(msg, position);
	msg.writeByte(index);
	connection->send(msg);
}

void Protocol::sendChangeField(Connection_ptr connection, Object* object)
{
	if (!connection) {
		return;
	}

	const int32_t index = object->getObjectIndex();
	if (index >= 10) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x6B);
	addPosition(msg, object->getPosition());
	msg.writeByte(index);
	addMapObject(connection, msg, object, true);
	connection->send(msg);
}

void Protocol::sendCreatureOutfit(Connection_ptr connection, const Creature * creature)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x8E);
	msg.writeQuad(creature->getId());
	addOutfit(msg, creature->getCurrentOutfit());
	connection->send(msg);
}

void Protocol::sendCreatureHealth(Connection_ptr connection, const Creature* creature)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x8C);
	msg.writeQuad(creature->getId());
	msg.writeByte(getPercentToGo(creature->getHitpoints(), creature->getMaxHitpoints()));
	connection->send(msg);
}

void Protocol::sendCreatureLight(Connection_ptr connection, const Creature* creature)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x8D);
	msg.writeQuad(creature->getId());
	uint8_t brightness, color;
	creature->getLight(brightness, color);
	msg.writeByte(brightness);
	msg.writeByte(color);
	connection->send(msg);
}

void Protocol::sendCreatureSpeed(Connection_ptr connection, const Creature* creature)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x8F);
	msg.writeQuad(creature->getId());
	msg.writeWord(creature->getSpeed());
	connection->send(msg);
}

void Protocol::sendCreatureSkull(Connection_ptr connection, const Creature* creature)
{
	if (!connection) {
		return;
	}

	const Player* player = creature->getPlayer();

	NetworkMessage msg;
	msg.writeByte(0x90);
	msg.writeQuad(creature->getId());
	msg.writeByte(player->getKillingMark(connection->getPlayer()));
	connection->send(msg);
}

void Protocol::sendCreatureParty(Connection_ptr connection, const Creature* creature)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x91);
	msg.writeQuad(creature->getId());
	msg.writeByte(connection->getPlayer()->getPartyMark(creature->getPlayer()));
	connection->send(msg);
}

void Protocol::sendGraphicalEffect(Connection_ptr connection, int32_t x, int32_t y, int32_t z, uint8_t type)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x83);
	addPosition(msg, x, y, z);
	msg.writeByte(type);
	connection->send(msg);
}

void Protocol::sendMissile(Connection_ptr connection, const Position& from_pos, const Position& to_pos, uint8_t type)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x85);
	addPosition(msg, from_pos);
	addPosition(msg, to_pos);
	msg.writeByte(type);
	connection->send(msg);
}

void Protocol::sendAnimatedText(Connection_ptr connection, const Position& pos, uint8_t color, const std::string& text)
{
	if (!connection) {
		return;
	}

	NetworkMessage msg;
	msg.writeByte(0x84);
	addPosition(msg, pos);
	msg.writeByte(color);
	msg.writeString(text);
	connection->send(msg);
}
