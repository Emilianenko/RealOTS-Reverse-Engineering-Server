#pragma once

#include "cylinder.h"
#include "enums.h"

class Map;

class Tile : public Cylinder
{
public:
	Tile(const Tile&) = delete; // non construction-copyable
	Tile& operator=(const Tile&) = delete; // non copyable

	explicit Tile() = default;
	~Tile();

	Tile* getTile() final {
		return this;
	}

	const Tile* getTile() const final {
		return this;
	}

	bool isProtectionZone() const {
		return protection_zone;
	}

	bool isRefreshZone() const {
		return refresh_zone;
	}

	bool isNoLogoutZone() const {
		return nologout_zone;
	}

	const std::list<Object*>& getObjects() const {
		return objects;
	}

	ReturnValue_t queryAdd(int32_t index, const Object* object, uint32_t amount, uint32_t flags, Creature* actor) const final;
	ReturnValue_t queryMaxCount(int32_t index, const Object* object, uint32_t amount, uint32_t& max_amount, uint32_t flags) const final;
	Cylinder* queryDestination(int32_t& index, const Object* object, Item** to_item, uint32_t& flags) final;
	
	void addObject(Object* object, int32_t index) final;
	void removeObject(Object* object) final;
	
	int32_t getObjectIndex(const Object* object) const final;
	Object* getObjectIndex(uint32_t index) const final;

	const Position& getPosition() const final;

	bool getFlag(ItemFlags_t flag) const;

	Object* getTopObject(bool move) const;
	Item* getBankItem() const;
	Item* getTopMoveItem() const;
	Item* getTopItem() const;
	Item* getMagicFieldItem() const;
	Item* getLiquidPoolItem() const;

	Creature* getTopCreature() const;
protected: 
	Position current_position;

	bool protection_zone = false;
	bool refresh_zone = false;
	bool nologout_zone = false;

	std::list<Object*> objects;

	friend class Map;
};
