#pragma once

#include "connection.h"
#include "channels.h"
#include "enums.h"

struct Outfit;
struct Position;
class Object;
class Creature;
class Item;
class Tile;
class Player;

class Protocol
{
public:
	// parse
	static void parseCharacterList(Connection_ptr connection, NetworkMessage& msg);
	static void parseCharacterLogin(Connection_ptr connection, NetworkMessage& msg);

	static void parseCommand(Connection_ptr connection, NetworkMessage& msg);

	static void parseLogout(Connection_ptr connection, Player* player);
	static void parsePing(Connection_ptr connection, Player* player);
	static void parseGoPath(Connection_ptr connection, Player* Player, NetworkMessage& msg);
	static void parseGoDirection(Connection_ptr connection, Player* Player, Direction_t look_direction);
	static void parseStop(Connection_ptr connection, Player* Player);
	static void parseRotate(Connection_ptr connection, Player* Player, Direction_t look_direction);
	static void parseMoveObject(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseRequestTrade(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseInspectTrade(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseAcceptTrade(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseCloseTrade(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseUseObject(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseUseTwoObjects(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseUseOnCreature(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseTurnOject(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseCloseContainer(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseUpContainer(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseEditText(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseLookAtPoint(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseTalk(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseRequestChannels(Connection_ptr connection, Player* player);
	static void parseJoinChannel(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseLeaveChannel(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseOpenPrivateChannel(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseProcessRuleViolationReport(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseCloseRuleViolationReport(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseCancelRuleViolationReport(Connection_ptr connection, Player* player);
	static void parseTactics(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseAttack(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseFollow(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseInviteToParty(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseJoinParty(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseRevokePartyInvitation(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parsePassPartyLeadership(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseLeaveParty(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseOpenChannel(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseInviteToChannel(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseExcludeFromChannel(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseCancel(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseRefreshTile(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseRequestOutfits(Connection_ptr connection, Player* player);
	static void parseSetOutfit(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseBugReport(Connection_ptr connection, Player* player, NetworkMessage& msg);
	static void parseErrorFileEntry(Connection_ptr connection, Player* player, NetworkMessage& msg);

	static void addCreature(Connection_ptr connection, NetworkMessage& msg, const Creature* creature, bool IsKnown, uint32_t OldCreature, bool UpdateFollows = false);
	static void addOutfit(NetworkMessage& msg, const Outfit& Outfit);
	static void addPosition(NetworkMessage& msg, const Position& Position);
	static void addPosition(NetworkMessage& msg, int32_t x, int32_t y, int32_t z);
	static void addItem(NetworkMessage& msg, const Item* Item);

	static Position readPosition(NetworkMessage& msg);

	static void sendLoginDisconnect(Connection_ptr connection, uint8_t Protocol, const std::string& Text);
	static void sendCharacterList(Connection_ptr connection);

	// player
	static void sendInitGame(Connection_ptr connection);
	static void sendPing(Connection_ptr connection);
	static void sendStats(Connection_ptr connection);
	static void sendSkills(Connection_ptr connection);
	static void sendSnapback(Connection_ptr connection);
	static void sendOutfitWindow(Connection_ptr connection);
	static void sendSetInventory(Connection_ptr connection, uint8_t slot, const Item* item);
	static void sendDeleteInventory(Connection_ptr connection, uint8_t slot);
	static void sendTextMessage(Connection_ptr connection, uint8_t type, const std::string& text);
	static void sendResult(Connection_ptr connection, ReturnValue_t ret);
	static void sendEditText(Connection_ptr connection, const Item* item, uint32_t edit_text_id);
	static void sendPlayerState(Connection_ptr connection, uint8_t state);
	static void sendClearTarget(Connection_ptr connection);
	static void sendMarkCreature(Connection_ptr connection, uint32_t creature_id, uint8_t color);

	static void sendTradeItemRequest(Connection_ptr connection, const std::string& name, const Item* item, bool acknowledge);
	static void sendTradeClose(Connection_ptr connection);

	static void sendChannels(Connection_ptr connection, const std::vector<const Channel*>& channels);
	static void sendChannel(Connection_ptr connection, const Channel* channel);
	static void sendPrivateChannel(Connection_ptr connection, const std::string& address);
	static void sendOpenOwnChannel(Connection_ptr connection, const Channel* channel);
	static void sendCloseChannel(Connection_ptr connection, const Channel* channel);

	static void sendLockRuleViolation(Connection_ptr connection);
	static void sendRuleViolationChannel(Connection_ptr connection, uint16_t channel_id);
	static void sendRemoveRuleViolationReport(Connection_ptr connection, const std::string& reporter);
	static void sendRuleViolationCancel(Connection_ptr connection, const std::string& reporter);

	static void sendTalk(Connection_ptr connection, uint32_t statement_id, const std::string& sender, TalkType_t type, uint16_t channel_id, const std::string& text);
	static void sendTalk(Connection_ptr connection, uint32_t statement_id, const std::string& sender, TalkType_t type, const std::string& text, uint32_t data);
	static void sendTalk(Connection_ptr connection, uint32_t statement_id, const std::string& sender, TalkType_t type, const std::string& text, const Position& pos);

	// containers
	static void sendContainer(Connection_ptr connection, uint8_t container_id, const Item* item);
	static void sendCloseContainer(Connection_ptr connection, uint8_t container_id);
	static void sendCreateInContainer(Connection_ptr connection, uint8_t container_id, const Item* item);
	static void sendChangeInContainer(Connection_ptr connection, uint8_t container_id, uint8_t index, const Item* item);
	static void sendDeleteInContainer(Connection_ptr connection, uint8_t container_id, uint8_t index);

	// tile - map
	static void addMapDescription(Connection_ptr connection, NetworkMessage& msg, const Position& Position);
	static void addMapFloors(Connection_ptr connection, NetworkMessage& msg, int32_t x, int32_t y, int32_t z, int32_t width, int32_t height);
	static void addMapRow(Connection_ptr connection, NetworkMessage& msg, int32_t x, int32_t y, int32_t z, int32_t width, int32_t height, int32_t offset, int32_t& skip);
	static void addMapPoint(Connection_ptr connection, NetworkMessage& msg, const Tile* Tile);
	static void addMapObject(Connection_ptr connection, NetworkMessage& msg, const Object* object, bool Update = false);

	static void sendMoveCreature(Connection_ptr connection, const Creature* creature, const Position& from_pos, int32_t from_index, const Position& to_pos, int32_t to_index);
	static void sendAmbiente(Connection_ptr connection);
	
	static void sendRefreshField(Connection_ptr connection, const Tile* tile);
	static void sendAddField(Connection_ptr connection, const Object* object);
	static void sendDeleteField(Connection_ptr connection, const Position& Position, int32_t index);
	static void sendChangeField(Connection_ptr connection, Object* object);
	
	static void sendCreatureOutfit(Connection_ptr connection, const Creature* creature);
	static void sendCreatureHealth(Connection_ptr connection, const Creature* creature);
	static void sendCreatureLight(Connection_ptr connection, const Creature* creature);
	static void sendCreatureSpeed(Connection_ptr connection, const Creature* creature);
	static void sendCreatureSkull(Connection_ptr connection, const Creature* creature);
	static void sendCreatureParty(Connection_ptr connection, const Creature* creature);

	static void sendGraphicalEffect(Connection_ptr connection, int32_t x, int32_t y, int32_t z, uint8_t type);
	static void sendMissile(Connection_ptr connection, const Position& from_pos, const Position& to_pos, uint8_t type);
	static void sendAnimatedText(Connection_ptr connection, const Position& pos, uint8_t color, const std::string& text);
};