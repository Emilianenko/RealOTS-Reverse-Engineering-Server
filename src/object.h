#pragma once

#include "enums.h"

struct Position
{
	int32_t x = 0;
	int32_t y = 0;
	int32_t z = 0;

	Position(int32_t _x = 0, int32_t _y = 0, int32_t _z = 0) {
		x = _x;
		y = _y;
		z = _z;
	}

	bool operator!=(const Position& l) const {
		return l.x != x || l.y != y || l.z != z;
	}

	bool operator==(const Position& l) const {
		return l.x == x && l.y == y && l.z == z;
	}

	static int32_t getOffsetX(const Position& from_pos, const Position& to_pos);
	static int32_t getOffsetY(const Position& from_pos, const Position& to_pos);
	static int32_t getOffsetZ(const Position& from_pos, const Position& to_pos);
	static Position getNextPosition(const Position& pos, Direction_t direction);

	static bool isAccessible(const Position& from_pos, const Position& to_pos, uint8_t range);
};

class Item;
class Creature;
class Player;
class Tile;
class Cylinder;
class Game;
class Map;

class Object
{
protected:
	constexpr Object() = default;
	virtual ~Object() = default;
public:
	// non-copyable
	//Object(const Object&) = delete;
	//Object& operator=(const Object&) = delete;

	virtual Item* getItem() {
		return nullptr;
	}

	virtual const Item* getItem() const {
		return nullptr;
	}

	virtual Creature* getCreature() {
		return nullptr;
	}

	virtual const Creature* getCreature() const {
		return nullptr;
	}

	virtual Player* getPlayer() {
		return nullptr;
	}

	virtual const Player* getPlayer() const {
		return nullptr;
	}

	virtual Tile* getTile() {
		return nullptr;
	}

	virtual const Tile* getTile() const {
		return nullptr;
	}

	virtual bool isRemoved() const {
		return false;
	}

	virtual uint8_t getObjectPriority() const;
	int32_t getObjectIndex() const;

	virtual std::string getObjectDescription(const Creature* creature) const {
		return std::string();
	}

	virtual const Position& getPosition() const;

	void setParent(Cylinder* new_parent) {
		parent = new_parent;
	}
	Cylinder* getParent() {
		return parent;
	}
	const Cylinder* getParent() const {
		return parent;
	}
	Cylinder* getTopParent();
	const Cylinder* getTopParent() const;
	Player* getHoldingPlayer();
	const Player* getHoldingPlayer() const;
protected:
	Cylinder* parent = nullptr;

	friend class Tile;
	friend class Game;
	friend class Map;
};