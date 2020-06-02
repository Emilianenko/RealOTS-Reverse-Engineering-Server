#pragma once

#include "object.h"
#include "enums.h"

enum CylinderFlags_t : uint8_t
{
	FLAG_NOLIMIT = 1 << 0, // bypass all checkings
	FLAG_CHILDISOWNER = 1 << 1, // used by containers to query capacity of the carrier (player)
	FLAG_PATHFINDING = 1 << 2, // used to query when searching for available path
};

static constexpr int32_t INDEX_ANYWHERE = -1;

class Cylinder : public virtual Object
{
public:
	virtual ReturnValue_t queryAdd(int32_t index, const Object* object, uint32_t amount, uint32_t flags, Creature* actor = nullptr) const = 0;
	virtual ReturnValue_t queryMaxCount(int32_t index, const Object* object, uint32_t amount, uint32_t& max_amount, uint32_t flags) const = 0;
	virtual Cylinder* queryDestination(int32_t& index, const Object* object, Item** to_item, uint32_t& flags) = 0;
	virtual void addObject(Object* object, int32_t index) = 0;
	virtual void removeObject(Object* object) = 0;
	virtual int32_t getObjectIndex(const Object* object) const = 0;
	virtual Object* getObjectIndex(uint32_t index) const = 0;
};