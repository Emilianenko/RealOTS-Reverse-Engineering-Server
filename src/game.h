#pragma once

#include "enums.h"
#include "connection.h"
#include "object.h"

#include <queue>
#include <set>
#include <boost/algorithm/string.hpp>

class Vocation;
class Party;
struct ItemType;
static constexpr int32_t UPDATE_AMBIENTE_INTERVAL = 2000;
static constexpr int32_t RELEASE_MEMORY_INTERVAL = 2000;
static constexpr int32_t OTHER_COUNTER_INTERVAL = 1000;

static constexpr int32_t DECAY_SIZE = 4;
static constexpr int32_t CREATURE_SKILL_SIZE = 10;

enum GameState_t
{
	GAME_OFFLINE,
	GAME_STARTING,
	GAME_RUNNING,
	GAME_SAVING,
};

enum AnnounceType_t
{
	ANNOUNCE_CREATE,
	ANNOUNCE_CHANGE,
	ANNOUNCE_DELETE,
};

enum CreatureChangeType_t
{
	CREATURE_HEALTH,
	CREATURE_LIGHT,
	CREATURE_OUTFIT,
	CREATURE_SPEED,
	CREATURE_SKULL,
	CREATURE_PARTY,
};

enum SearchObjectType_t
{
	SEARCH_MOVE,
	SEARCH_LOOK,
	SEARCH_USEITEM
};

enum SpellCastResult_t : uint8_t
{
	SPELL_SUCCESS = 1,
	SPELL_FAILED = 2,
	SPELL_NONE = 3,
};

struct Spell;
class Config;
class Object;
class Creature;
class Player;
class Cylinder;
class Protocol;
class Tile;
class Item;
struct Position;

struct PlayerStatement
{
	uint32_t statement_id = 0;
	uint64_t timestamp = 0;
	uint16_t channel_id = 0;
	TalkType_t type = TALKTYPE_SAY;
	Player* player = nullptr;
	std::string text;
	bool reported = false;
};

struct RuleViolationEntry
{
	Player* player = nullptr;
	Player* gamemaster = nullptr;
	std::string text;
	uint64_t timestamp = 0;
	bool available = true;
};

struct CreatureEntry
{
	Creature* creature = nullptr;
	uint64_t interval = 0;
};

struct CreatureEntryComparator
{
	bool operator()(const CreatureEntry& l, const CreatureEntry& r) const;
};

struct InsensitiveStringCompare {
	bool operator() (const std::string& a, const std::string& b) const {
		return _stricmp(a.c_str(), b.c_str()) < 0;
	}
};

class Game
{
public:
	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;

	explicit Game() = default;
	~Game();

	void launchGame();
	void shutdown();

	void setGameState(GameState_t new_state);
	GameState_t getGameState() const {
		return game_state;
	}

	const Position& getNewbieStart() const {
		return newbie_start_pos;
	}

	uint32_t getRoundNr() const {
		return round_number;
	}

	const std::vector<Player*>& getPlayers() const {
		return players;
	}

	void announceMissileEffect(const Position& from_pos, const Position& to_pos, uint8_t type);
	void announceGraphicalEffect(Object* object, uint8_t type);
	void announceGraphicalEffect(int32_t x, int32_t y, int32_t z, uint8_t type);
	void announceAnimatedText(const Position& pos, uint8_t color, const std::string& text);
	void announceChangedObject(Object* object, AnnounceType_t type);
	void announceChangedField(Object* object, AnnounceType_t type);
	void announceChangedInventory(Object* object, AnnounceType_t type) const;
	void announceChangedContainer(Object* object, AnnounceType_t type);
	void announceMovingCreature(Creature* creature, const Position& from_pos, int32_t from_index, const Position& to_pos, int32_t to_index);
	void announceChangedCreature(Creature* creature, CreatureChangeType_t type);

	void getSpectators(std::unordered_set<Creature*>& spectator_list, int32_t centerx, int32_t centery, int32_t rangex, int32_t rangey, bool only_players);
	void getAmbiente(uint8_t& brightness, uint8_t& color) const;

	void closeContainers(Item* item, bool force);

	inline uint64_t serverMilliseconds() const;

	void addConnection(Connection_ptr connection);
	void removeConnection(Connection_ptr connection);

	void decayItem(Item* item);
	void stopDecay(Item* item) const;

	void moveAllObjects(Tile* tile, Tile* to_tile, Object* ignore_object, bool move_unmovable);
	void clearTile(const Position& pos, Object* ignore_object, ItemFlags_t item_flag);
	void changeItem(Item* item, uint16_t type_id, uint32_t value);

	void queueCreature(Creature* creature);
	void addCreatureSkillCheck(Creature* creature);
	void addPlayerList(Player* player);
	void addCreatureList(Creature* creature);
	void storePlayer(Player* player, uint32_t user_id);

	void removePlayerList(Player* player);
	void removeCreatureList(Creature* creature);
	void removeCreature(Creature* creature);
	void removeItem(Item* item, uint8_t amount);

	ReturnValue_t setCreatureOnMap(Creature* creature, const Position& pos) {
		return setCreatureOnMap(creature, pos.x, pos.y, pos.z);
	}
	ReturnValue_t setCreatureOnMap(Creature* creature, int32_t x, int32_t y, int32_t z);
	ReturnValue_t moveCreature(Creature* creature, Tile* to_tile, uint32_t flags);
	ReturnValue_t moveCreature(Creature* creature, int32_t x, int32_t y, int32_t z, uint32_t flags);
	ReturnValue_t moveItem(Item* item, uint8_t amount, Cylinder* from_cylinder, Cylinder* to_cylinder, int32_t to_index, Creature* actor, uint32_t flags);
	ReturnValue_t canPlayerCarryItem(Player* player, const Item* item, uint32_t flags) const;
	ReturnValue_t addItem(Item* item, Cylinder* to_cylinder);

	Item* createItemAtPlayer(Player* player, const ItemType* item_type, uint32_t value);
	Item* createLiquidPool(const Position& pos, const ItemType* item_type, uint8_t liquid_type);
	Item* createField(const Position& pos, FieldType_t field, uint32_t owner, bool peaceful);
	void massCreateField(Creature* creature, const Position& pos, uint8_t radius, FieldType_t type);
	void deleteField(const Position& pos);
	void cleanUpTile(Tile* tile);

	int32_t identifyPlayer(Player** player, bool exact_match, const std::string& name) const;

	Player* findPerson(Player* player, bool exact_match, const std::string& name);
	void creatureTalk(Creature* creature, TalkType_t type, const std::string& text);
	void creatureAttack(Creature* creature, uint32_t creature_id, bool follow);
	void playerUseObject(Player* player, const Position& pos, uint16_t type_id, uint8_t index, uint8_t container_id);
	void playerUseOnCreature(Player* player, const Position& pos, uint16_t type_id, uint8_t index, uint32_t creature_id);
	void playerUseTwoObjects(Player* player, const Position& pos, uint16_t type_id, uint8_t index, const Position& to_pos, uint16_t to_type_id, uint8_t to_index);
	void playerUseTwoObjects(Player* player, Item* item, const Position& to_pos, uint16_t to_type_id, uint8_t to_index);
	void playerMoveObject(Player* player, const Position& from_pos, uint8_t from_index, const Position& to_pos, uint16_t type_id, uint8_t amount);
	void playerMoveItem(Player* player, Item* item, uint8_t amount, uint16_t type_id, Cylinder* from_cylinder, Cylinder* to_cylinder, const Position& to_pos);
	void playerTurnObject(Player* player, const Position& pos, uint16_t type_id, uint8_t index);
	void playerCloseContainer(Player* player, uint8_t container_id) const;
	void playerUpContainer(Player* player, uint8_t container_id) const;
	void playerLookAtObject(Player* player, const Position& pos, uint16_t type_id, uint8_t index) const;
	void playerOpenChannel(Player* player) const;
	void playerInviteToChannel(Player* player, const std::string& name) const;
	void playerExcludeFromChannel(Player* player, const std::string& name) const;
	void playerRequestChannels(Player* player) const;
	void playerJoinChannel(Player* player, uint16_t channel_id) const;
	void playerCloseChannel(Player* player, uint16_t channel_id) const;
	void playerOpenPrivateChannel(Player* player, const std::string& address) const;
	void playerTalk(Player* player, const std::string& text, const std::string& address, uint16_t channel_id, TalkType_t type, bool check_spamming);
	void playerTalkChannel(Player* player, uint16_t channel_id, const std::string& text, TalkType_t type) const;
	void playerReportRuleViolation(Player* player, const std::string& text);
	void playerContinueRuleViolationReport(Player* player, const std::string& text);
	void playerTradeObject(Player* player, const Position& pos, uint16_t type_id, uint8_t index, uint32_t player_id);
	void playerInspectTrade(Player* player, bool own_offer, uint8_t index) const;
	void playerAcceptTrade(Player* player);
	void playerEditText(Player* player, uint32_t edit_text_id, const std::string& text) const;
	void playerProcessRuleViolationReport(Player* player, const std::string& reporter);
	void playerCloseRuleViolationReport(Player* player, const std::string& reporter);
	void playerCancelRuleViolationReport(Player* player);
	void playerInviteToParty(Player* player, uint32_t player_id) const;
	void playerJoinParty(Player* player, uint32_t player_id) const;
	void playerRevokePartyInvitation(Player* player, uint32_t player_id) const;
	void playerPassPartyLeadership(Player* player, uint32_t player_id) const;
	void playerLeaveParty(Player* player) const;
	void playerItemIllusion(Player* player, const Item* item) const;
	void playerTeleportToFriend(Player* player, const std::string& name);
	void playerRetrieveFriend(Player* player, const std::string& name);
	void playerHomeTeleport(Player* player, const std::string& name);
	void playerChangeSpecial(Player* player, const std::string& param);
	void playerChangeProfession(Player* player, const Vocation* vocation);
	void playerKickPlayer(Player* player, const std::string& name);
	bool playerEnchantItem(Player* player, const ItemType* origin, const ItemType* target, int32_t mana, int32_t soulpoints);

	Creature* getCreatureById(uint32_t creature_id);
	Player* getStoredPlayer(uint32_t user_id) const;
	Player* getPlayerByUserId(uint32_t user_id) const;
	Player* getPlayerByName(const std::string& name, bool exact_match = true) const;
	Cylinder* getCylinder(Player* player, const Position& pos) const;
	Object* getObject(Player* player, const Position& pos, uint8_t index, uint16_t type_id, SearchObjectType_t search_type) const;

	const std::map<uint32_t, RuleViolationEntry>& getRuleViolationReports() const {
		return player_requests;
	}

	bool playerExists(const std::string& name) const;

	const std::string& getPlayerName(const std::string& name) const;
	uint32_t getPlayerId(const std::string& name) const;

	void closeTrade(Player* player);
	void loginPlayers(int32_t count);
private:
	void playerUseChangeItem(Player* player, Item* item);
	void playerUseKeyDoor(Player* player, Item* item);
	void playerUseNameDoor(Player* player, Item* item);
	void playerUseFood(Player* player, Item* item);
	void playerUseTextItem(Player* player, Item* item) const;
	void playerUseRune(Player* player, Item* item, Object* target);

	SpellCastResult_t playerCastSpell(Player* player, std::string text);
	SpellCastResult_t playerCastRuneSpell(Player* player, Spell* spell, const std::string& text);
	void playerCastCharacterRightSpell(Player* player, Spell* spell, const std::string& text, const std::string& params);

	void playerCreateItem(Player* player, const std::string& text);
	void playerTeleportSpell(Player* player, const std::string& text);

	void notifyPlayer(Player* player, const Item* item, bool inventory);
	void notifyTrades(const Item* item);
	void closeRuleViolationReport(Player* player);

	void insertChainCreature(Creature* creature, int32_t x, int32_t y);
	void moveChainCreature(Creature* creature, int32_t x, int32_t y);
	void removeChainCreature(Creature* creature);

	void indexPlayer(const Player* player);

	uint32_t appendStatement(Player* player, const std::string& text, uint16_t channel_id, TalkType_t type) const;

	void advanceGame(int32_t delay);
	void processConnections();
	void moveCreatures();
	void processItems(int32_t interval);
	void processSkills();
	void processCreatures();

	void receiveData();
	void sendData();

	void releaseObjects();
	void releaseItem(Item* item);
	void releaseCreature(Creature* creature);

	std::priority_queue<CreatureEntry, std::vector<CreatureEntry>, CreatureEntryComparator> creature_queue{};
	std::array<std::vector<Creature*>, CREATURE_SKILL_SIZE> creature_skills{};

	std::array<std::vector<Item*>, DECAY_SIZE> decaying_list{};
	std::map<Item*, uint32_t> trading_items{};

	std::vector<Creature*> removed_creatures{};

	std::vector<Creature*> creatures{};
	std::vector<Player*> players{};

	std::map<uint32_t, PlayerStatement> player_statements{};
	std::map<uint32_t, RuleViolationEntry> player_requests{};
	std::unordered_map<uint32_t, Player*> stored_players{};
	std::map<std::string, Position, InsensitiveStringCompare> map_points{};
	std::map<std::string, uint32_t, InsensitiveStringCompare> player_ids{};
	std::set<std::string, InsensitiveStringCompare> player_names_index{};

	std::vector<Connection_ptr> connections{};

	std::recursive_mutex connection_mutex;

	Position newbie_start_pos;

	GameState_t game_state = GAME_STARTING;

	uint8_t old_ambiente = 0;
	uint8_t last_decay_bucket = 1;
	uint8_t last_creature_bucket = 1;

	int32_t decay_time_counter = 0;
	int32_t other_time_counter = 0;
	int32_t skill_time_counter = 0;
	int32_t creature_time_counter = 0;
	int32_t round_number = 0;

	uint64_t currentBeatMiliseconds = 0;

	friend class Protocol;
	friend class Config;
};

extern Game g_game;
