#include "pch.h"

#include "object.h"
#include "cylinder.h"
#include "map.h"

int32_t Position::getOffsetX(const Position& from_pos, const Position& to_pos)
{
	return from_pos.x - to_pos.x;
}

int32_t Position::getOffsetY(const Position& from_pos, const Position& to_pos)
{
	return from_pos.y - to_pos.y;
}

int32_t Position::getOffsetZ(const Position& from_pos, const Position& to_pos)
{
	return from_pos.z - to_pos.z;
}

Position Position::getNextPosition(const Position& pos, Direction_t direction)
{
	Position result = pos;

	switch (direction) {
		case DIRECTION_NORTH: result.y--; break;
		case DIRECTION_EAST: result.x++; break;
		case DIRECTION_SOUTH: result.y++; break;
		case DIRECTION_WEST: result.x--; break;
		case DIRECTION_NORTHEAST: result.y--; result.x++; break;
		case DIRECTION_NORTHWEST: result.y--; result.x--; break;
		case DIRECTION_SOUTHEAST: result.y++; result.x++; break;
		case DIRECTION_SOUTHWEST: result.y++; result.x--; break;
		default: break;
	}

	return result;
}

bool Position::isAccessible(const Position& from_pos, const Position& to_pos, uint8_t range)
{
	const Tile* to_tile = g_map.getTile(to_pos);
	if (to_tile && to_tile->getFlag(HOOKEAST) || to_tile->getFlag(HOOKSOUTH)) {
		if (to_tile->getFlag(HOOKSOUTH)) {
			if (!(from_pos.y >= to_pos.y && from_pos.y <= 1 + to_pos.y || from_pos.x >= to_pos.x - 1 && from_pos.x > to_pos.x + 1)) {
				return false;
			}
		}

		if (to_tile->getFlag(HOOKEAST)) {
			if (!(from_pos.x >= to_pos.x && from_pos.x <= 1 + to_pos.x || from_pos.x >= to_pos.x - 1 && from_pos.x > to_pos.x + 1)) {
				return false;
			}
		}
	}

	return std::abs(from_pos.x - to_pos.x) <= range && std::abs(from_pos.y - to_pos.y) <= range;
}

uint8_t Object::getObjectPriority() const
{
	return -1;
}

int32_t Object::getObjectIndex() const
{
	return parent->getObjectIndex(this);
}

const Position& Object::getPosition() const {
	if (parent) {
		return parent->getPosition();
	}
	
	static Position static_pos;
	return static_pos;
}

Cylinder* Object::getTopParent()
{
	Cylinder* aux = getParent();
	Cylinder* prevaux = dynamic_cast<Cylinder*>(this);
	if (!aux) {
		return prevaux;
	}

	while (aux->getParent() != nullptr) {
		prevaux = aux;
		aux = aux->getParent();
	}

	if (prevaux) {
		return prevaux;
	}
	return aux;
}

const Cylinder* Object::getTopParent() const
{
	const Cylinder* aux = getParent();
	const Cylinder* prevaux = dynamic_cast<const Cylinder*>(this);
	if (!aux) {
		return prevaux;
	}

	while (aux->getParent() != nullptr) {
		prevaux = aux;
		aux = aux->getParent();
	}

	if (prevaux) {
		return prevaux;
	}
	return aux;
}

Player* Object::getHoldingPlayer()
{
	Cylinder* p = getParent();
	while (p) {
		if (p->getCreature()) {
			return p->getPlayer();
		}

		p = p->getParent();
	}
	return nullptr;
}

const Player* Object::getHoldingPlayer() const
{
	const Cylinder* p = getParent();
	while (p) {
		if (p->getCreature()) {
			return p->getPlayer();
		}

		p = p->getParent();
	}
	return nullptr;
}
