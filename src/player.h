#pragma once

#include "creature.h"
#include "cylinder.h"

class Combat;
class Game;
class Protocol;
class Party;
class Vocation;

class SkillManapoints : public Skill
{
public:
	explicit SkillManapoints(Creature* cr) : Skill(cr) {
		//
	}

	void setValue(int32_t value) {
		manapoints = value;
	}
	int32_t getValue() const {
		return manapoints;
	}

	void setMax(int32_t value) {
		max_manapoints = value;
	}
	int32_t getMax() const {
		return max_manapoints;
	}

	void change(int16_t value);
protected:
	int32_t manapoints = 0;
	int32_t max_manapoints = 0;
};

class SkillSoulpoints : public Skill
{
public:
	explicit SkillSoulpoints(Creature* cr) : Skill(cr) {
		//
	}

	void setSoulpoints(int32_t value) {
		soulpoints = value;
	}
	uint16_t getSoulpoints() const {
		return soulpoints;
	}

	void setMaxSoulpoints(int32_t value) {
		max_soulpoints = value;
	}
	uint16_t getMaxSoulpoints() const {
		return max_soulpoints;
	}

	void change(int16_t value);
protected:
	uint16_t soulpoints = 0;
	uint16_t max_soulpoints = 0;
};

class SkillFed : public Skill
{
public:
	explicit SkillFed(Creature* cr) : Skill(cr) {
		//
	}
protected:
	void event(int32_t) final;
};

class SkillMagicShield : public Skill
{
public:
	explicit SkillMagicShield(Creature* cr) : Skill(cr) {
		//
	}

	void setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value) final;
};

class Player final : public Creature, public Cylinder
{
public:
	Player(const Player&) = delete; // non construction-copyable
	Player& operator=(const Player&) = delete; // non copyable

	Player();
	~Player();

	Player* getPlayer() final {
		return this;
	}

	const Player* getPlayer() const final {
		return this;
	}

	bool loadData(uint32_t user_id, bool set_inventory = false);
	void takeOver(Connection_ptr new_connection);

	void changeManaPoints(int32_t value) const;
	uint16_t getManaPoints() const {
		return skill_manapoints->getValue();
	}
	uint16_t getMaxManaPoints() const {
		return skill_manapoints->getMax();
	}

	SexType_t getSex() const {
		return sex_type;
	}

	Item* getInventoryItem(InventorySlot_t slot) const;
	Item* getWeapon() const;
	Item* getShield() const;

	Skull_t getKillingMark(const Player* observer) const;
	PartyShields_t getPartyMark(const Player* observer) const;

	uint32_t getFreeCapacity() const;
	uint32_t getInventoryWeight() const;

	std::string getObjectDescription(const Creature* creature) const final;

	ReturnValue_t queryAdd(int32_t index, const Object* object, uint32_t amount, uint32_t flags, Creature* actor) const final;
	ReturnValue_t queryMaxCount(int32_t index, const Object* object, uint32_t amount, uint32_t& max_amount, uint32_t flags) const final;
	Cylinder* queryDestination(int32_t& index, const Object* object, Item** to_item, uint32_t& flags) final;

	void addObject(Object* object, int32_t index) final;
	void removeObject(Object* object) final;

	int32_t getObjectIndex(const Object* object) const final;
	Object* getObjectIndex(uint32_t index) const final;

	const Position& getPosition() const final;

	bool isAttacker(const Player* victim, bool check_former) const;
	bool isAttackJustified(const Player* victim) const;

	void recordMurder(Player* victim);
	void recordAttack(Player* victim);
	void clearAttacker(Player* victim);
	void clearKillingMarks();
	void addExperience(SkillType_t skill, int64_t value);
	void removeExperience(SkillType_t skill, int64_t value);

	int32_t checkPlayerKilling();
	int32_t getAttackDamage() const final;
	int32_t getDefense() const final;
	int32_t getArmor() const final;
	int32_t probeSkill(SkillType_t skill, int32_t value, int32_t prob);

	void addContainer(uint8_t container_id, Item* item);
	void closeContainer(uint8_t container_id);
	Item* getContainerById(uint8_t container_id);
	int8_t getContainerId(const Item* item) const;

	int64_t checkForMuting() const;
	int64_t recordTalk();
	int64_t recordMessage(uint32_t address);

	void blockLogout(uint32_t delay, bool block_pz);
	void checkState();

	void notifyTrades();

	static uint64_t getExperienceForLevel(uint16_t level);
	static uint64_t getExperienceForSkill(uint16_t level);
	static uint64_t getManaForMagicLevel(uint16_t level);
protected:
	void onCreate() final;
	void onDelete() final;
	void onDeath() final;
	void onLoseTarget() final;
	void onWalk(const Position& from_pos, int32_t from_index, const Position& to_pos, int32_t to_index) final;
	void onKilledCreature(Creature* target) final;
	void onAttackCreature(Creature* target) final;
	void onAttacked(Creature* attacker) final;
	int32_t onDamaged(Creature* attacker, int32_t value, DamageType_t damage_type) final;

	SexType_t sex_type = SEX_MALE;

	Position start_pos;
	Position current_pos;

	bool aggressor = false;
	bool former_aggressor = false;
	bool stored = false;

	uint8_t number_of_mutings = 0;
	uint8_t old_state = 0;
	uint8_t skill_percent[SKILL_SIZE];
	uint16_t learning_points = 30;
	uint32_t user_id = 0;
	uint32_t capacity = 400;
	uint32_t edit_text_id = 0;
	uint64_t last_ping = 0;
	uint64_t last_pong = 0;
	uint64_t muting_end = 0;
	uint64_t player_killer_end = 0;
	uint64_t talk_buffer_full_time = 0;
	uint64_t earliest_trade_channel_round = 0;
	uint64_t earliest_yell_round = 0;
	uint64_t earliest_logout_round = 0;
	uint64_t former_logout_round = 0;
	uint64_t earliest_protection_zone_round = 0;
	uint64_t timestamp = 0;
	uint64_t timestamp_action = 0;
	int64_t skill_level[SKILL_SIZE]{ 1, 10, 10, 10, 10, 10, 10, 10, 0 };
	int64_t skill_experience[SKILL_SIZE];
	int64_t skill_next_level[SKILL_SIZE];

	Item* edit_item = nullptr;
	Item* trading_item = nullptr;

	Player* trade_partner = nullptr;
	TradeState_t trade_state = TRADE_NONE;

	Party* party = nullptr;
	const Vocation* vocation = nullptr;

	time_t last_login_time = 0;

	std::array<uint32_t, 20> addresses;
	std::array<uint64_t, 20> addresses_times;

	std::unordered_set<uint32_t> former_attacked_players;
	std::unordered_set<uint32_t> attacked_players;
	std::unordered_set<uint32_t> murder_timestamps;

	SkillManapoints* skill_manapoints = new SkillManapoints(this);
	SkillSoulpoints* skill_soulpoints = new SkillSoulpoints(this);
	SkillFed* skill_fed = new SkillFed(this);
	SkillMagicShield* skill_magic_shield = new SkillMagicShield(this);

	std::map<uint8_t, Item*> open_containers{};

	friend class Game;
	friend class Protocol;
	friend class Combat;
	friend class Magic;
	friend class Creature;
	friend class SkillFed;
	friend class Party;
	friend class Tile;
};
