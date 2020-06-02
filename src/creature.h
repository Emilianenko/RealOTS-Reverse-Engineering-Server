#pragma once

#include "object.h"
#include "enums.h"
#include "connection.h"

enum ToDoType_t : uint8_t
{
	TODO_NONE,
	TODO_ATTACK,
	TODO_WAIT,
	TODO_ROTATE,
	TODO_GO,
	TODO_MOVE_OBJECT,
	TODO_USE_OBJECT,
	TODO_USE_TWO_OBJECTS,
	TODO_TURNOBJECT,
	TODO_TALK,
	TODO_TRADE,
};

struct ToDoEntry
{
	ToDoType_t code = TODO_NONE;
	TalkType_t type = TALKTYPE_SAY;
	Direction_t look_direction = DIRECTION_NORTH;
	Position to_pos;
	Position from_pos;
	uint16_t type_id = 0;
	uint16_t to_type_id = 0;
	uint8_t from_index = 0;
	uint8_t to_index = 0;
	uint64_t interval = 0;
	uint8_t amount = 0;
	uint8_t container_id = 0;
	uint16_t channel_id = 0;
	uint32_t player_id = 0;
	uint32_t creature_id = 0;
	std::string text;
	std::string address;
	bool check_spamming = false;
	Item* item = nullptr;
};

struct Outfit
{
	uint16_t type_id = 0;
	uint16_t look_id = 0;
	uint8_t head = 0;
	uint8_t body = 0;
	uint8_t legs = 0;
	uint8_t feet = 0;
};

class Combat;
class Magic;
class Creature;
class Skill;
class Player;

class Skill
{
public:
	explicit Skill(Creature* cr) : creature(cr) {
		//
	}
	virtual ~Skill() = default;

	int32_t getTiming() const {
		return cycle;
	}

	virtual void setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value);
	virtual bool process();
protected:
	virtual void event(int32_t) {
		//
	};

	Creature* creature = nullptr;

	int32_t cycle = 0;
	int32_t count = 0;
	int32_t max_count = 0;
};

class SkillHitpoints : public Skill
{
public:
	explicit SkillHitpoints(Creature* cr) : Skill(cr) {
		//
	}

	void setValue(int32_t value) {
		hitpoints = value;
	}
	int32_t getValue() const {
		return hitpoints;
	}
	void setMax(int32_t value) {
		max_hitpoints = value;
	}
	int32_t getMax() const {
		return max_hitpoints;
	}

	void change(int16_t value);
private:
	int32_t hitpoints = 0;
	int32_t max_hitpoints = 0;
};

class SkillGoStrength : public Skill
{
public:
	explicit SkillGoStrength(Creature* cr) : Skill(cr) {
		//
	}

	int32_t getBaseSpeed() const {
		return speed;
	}
	int32_t getSpeed() const {
		return delta + getBaseSpeed();
	}
	int32_t getDelta() const {
		return delta;
	}

	void setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value) final;
	void setBaseSpeed(int32_t value);
	void setDelta(int32_t value);
protected:
	void event(int32_t value) final;

	int32_t speed = 0;
	int32_t delta = 0;
};

class SkillLight : public Skill
{
public:
	explicit SkillLight(Creature* cr) : Skill(cr) {
		//
	}

	void setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value) final;
protected:
	void event(int32_t value) final;
};

class SkillIllusion : public Skill
{
public:
	explicit SkillIllusion(Creature* cr) : Skill(cr) {
		//
	}

	void setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value) final;
protected:
	void event(int32_t value) final;
};

class SkillBurning : public Skill
{
public:
	explicit SkillBurning(Creature* cr) : Skill(cr) {
		//
	}

protected:
	void event(int32_t value) final;
};

class SkillEnergy : public Skill
{
public:
	explicit SkillEnergy(Creature* cr) : Skill(cr) {
		//
	}

protected:
	void event(int32_t value) final;
};

class SkillPoison : public Skill
{
public:
	explicit SkillPoison(Creature* cr) : Skill(cr) {
		//
	}

	bool process() final;
	void setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value) final;
protected:
	void event(int32_t value) final;

	int32_t factor_percent = 0;
};

struct CombatDamage
{
	uint32_t creature_id = 0;
	uint32_t damage = 0;
	uint32_t timestamp = 0;
};

class Game;
struct CancelInvisibleImpact;

class Creature : public virtual Object
{
public:
	Creature(const Creature&) = delete; // non construction-copyable
	Creature& operator=(const Creature&) = delete; // non copyable

	Creature();
	virtual ~Creature();

	Creature* getCreature() final {
		return this;
	}

	const Creature* getCreature() const final {
		return this;
	}

	const std::string& getName() const {
		return name;
	}

	uint32_t getId() const {
		return id;
	}
	uint16_t getHitpoints() const {
		return skill_hitpoints->getValue();
	}
	uint16_t getMaxHitpoints() const {
		return skill_hitpoints->getMax();
	}
	uint16_t getSpeed() const {
		return static_cast<uint16_t>(2 * skill_go_strength->getSpeed() + 80);
	}
	uint64_t getNextWakeup() const {
		return next_wakeup;
	}

	bool isDead() const {
		return is_dead;
	}

	bool isRemoved() const final {
		return removed;
	}

	uint8_t getObjectPriority() const final {
		return 4;
	}

	void setCurrentOutfit(const Outfit& outfit) {
		current_outfit = outfit;
	}
	const Outfit& getCurrentOutfit() const {
		return current_outfit;
	}

	void setOriginalOutfit(const Outfit& outfit) {
		original_outfit = outfit;
	}
	const Outfit& getOriginalOutfit() const {
		return original_outfit;
	}

	Direction_t getLookDirection() const {
		return look_direction;
	}

	void setFightMode(FightMode_t new_fightmode) {
		fight_mode = new_fightmode;
	}
	void setChaseMode(ChaseMode_t new_chasemode) {
		chase_mode = new_chasemode;
	}
	void setSecureMode(bool new_securemode) {
		secure_mode = new_securemode;
	}

	Connection_ptr getConnection() const {
		return connection_ptr;
	}
	void setConnection(Connection_ptr new_connection) {
		connection_ptr = new_connection;
	}

	virtual void getLight(uint8_t& brightness, uint8_t& color) const;

	bool canSeePosition(const Position& pos) const;
	bool canSeePosition(int32_t x, int32_t y, int32_t z) const;
	bool canSeeCreature(const Creature* creature) const;

	virtual int32_t getDefendDamage();
	virtual int32_t getAttackDamage() const {
		return 0;
	}
	virtual int32_t getDefense() const {
		return 0;
	}
	virtual int32_t getArmor() const {
		return 0;
	}

	virtual void process();

	int32_t damage(Creature* attacker, int32_t value, DamageType_t damage_type);

	int64_t calculateDelay();
	bool toDoClear();
	void toDoYield();
	void toDoAttack();
	void toDoWait(uint16_t interval);
	void toDoTrade(const Position& pos, uint16_t type_id, uint8_t index, uint32_t player_id);
	void toDoRotate(Direction_t new_look_direction);
	void toDoMoveItem(Item* item, uint16_t amount, const Position& to_pos);
	void toDoMoveObject(const Position& from_pos, uint16_t type_id, uint8_t from_index, const Position& to_pos, uint8_t amount);
	void toDoTurnObject(const Position& pos, uint16_t type_id, uint8_t index);
	void toDoUse(const Position& pos, uint16_t type_id, uint8_t index, uint8_t container_id);
	void toDoUseTwoObjects(const Position& pos, uint16_t type_id, uint8_t index, uint32_t creature_id);
	void toDoUseTwoObjects(Item* item, const Position& to_pos, uint16_t to_type_id, uint8_t to_index);
	void toDoUseTwoObjects(const Position& pos, uint16_t type_id, uint8_t index, const Position& to_pos, uint16_t to_type_id, uint8_t to_index);
	void toDoTalk(const std::string& text, const std::string& address, uint16_t channel_id, TalkType_t type, bool check_spamming);
	void toDoAdd(ToDoEntry& new_entry);
	void toDoStop();
	void toDoStart();
	void execute();
protected:
	void setId();

	virtual void onCreate();
	virtual void onDelete();
	virtual void onDeath();
	virtual void onLoseTarget();
	virtual void onWalk(const Position& from_pos, int32_t from_index, const Position& to_pos, int32_t to_index);
	virtual void onCreatureMove(Creature* creature);
	virtual void onIdleStimulus();
	virtual void onKilledCreature(Creature* target) {
		//
	}
	virtual void onAttackCreature(Creature* target) {
		//
	}
	virtual void onAttacked(Creature* attacker) {
		//
	}
	virtual int32_t onDamaged(Creature* attacker, int32_t value, DamageType_t damage_type) {
		return value;
	}

	bool removed = false;
	bool is_dead = false;
	bool skill_processing = false;
	bool secure_mode = false;
	bool following = false;
	bool stop = false;
	bool lock_todo = false;

	uint8_t current_combat_entry = 0;
	uint8_t total_todo = 0;
	uint8_t current_todo = 0;
	uint32_t id = 0;
	uint32_t combat_damage = 0;
	uint32_t fire_damage_origin = 0;
	uint32_t poison_damage_origin = 0;
	uint32_t energy_damage_origin = 0;
	uint64_t next_wakeup = 0;
	uint64_t earliest_walk_time = 0;
	uint64_t earliest_attack_time = 0;
	uint64_t earliest_spell_time = 0;
	uint64_t earliest_defend_time = 0;
	uint64_t last_defend_time = 0;

	std::string name = "Player";
	std::string murderer;

	CreatureType_t creature_type = CREATURE_NONE;
	RaceType_t race_type = RACE_BLOOD;
	ChaseMode_t chase_mode = CHASE_STAND;
	FightMode_t fight_mode = FIGHT_BALANCED;
	Direction_t look_direction = DIRECTION_SOUTH;
	Outfit current_outfit;
	Outfit original_outfit;

	std::array<Item*, INVENTORY_SIZE> inventory;
	std::array<ToDoEntry, 64> todo_list;
	std::array<CombatDamage, 20> combat_list;

	Creature* next_chain_creature = nullptr;
	Creature* attacked_creature = nullptr;
	Connection_ptr connection_ptr = nullptr;

	SkillHitpoints* skill_hitpoints = new SkillHitpoints(this);
	SkillGoStrength* skill_go_strength = new SkillGoStrength(this);
	SkillLight* skill_light = new SkillLight(this);
	SkillBurning* skill_burning = new SkillBurning(this);
	SkillPoison* skill_poison = new SkillPoison(this);
	SkillEnergy* skill_energy = new SkillEnergy(this);
	SkillIllusion* skill_illusion = new SkillIllusion(this);
	Skill* skill_drunken = new Skill(this);

	friend class Skill;
	friend class SkillFed;
	friend class Magic;
	friend class Game;
	friend class Combat;
	friend class Player;
	friend class SkillBurning;
	friend class SkillPoison;
	friend class SkillEnergy;
	friend struct CancelInvisibleImpact;
};
