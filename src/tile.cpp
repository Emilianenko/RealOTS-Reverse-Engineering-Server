#include "pch.h"

#include "tile.h"
#include "item.h"
#include "game.h"
#include "tools.h"
#include "player.h"

Tile::~Tile()
{
	for (Object* object : objects) {
		if (object != nullptr && object->getPlayer() == nullptr && object->getItem() == nullptr) {
			delete object;
		}
	}
}

ReturnValue_t Tile::queryAdd(int32_t index, const Object* object, uint32_t amount, uint32_t flags, Creature* actor) const
{
	if (hasBitSet(FLAG_NOLIMIT, flags)) {
		return ALLGOOD;
	}

	if (object->getCreature()) {
		if (!getFlag(BANK)) {
			return NOTPOSSIBLE;
		}

		if (getTopCreature() != nullptr) {
			return NOTENOUGHROOM;
		}

		if (getFlag(UNPASS)) {
			return NOTENOUGHROOM;
		}

		if (hasBitSet(FLAG_PATHFINDING, flags)) {
			if (getFlag(AVOID)) {
				return NOTPOSSIBLE;
			}
		} else {
			for (const Object* tile_object : objects) {
				if (const Item* item = tile_object->getItem()) {
					if (item->getFlag(UNPASS) && item->getFlag(UNMOVE)) {
						return NOTENOUGHROOM;
					}
				}
			}
		}

		if (const Player* player = object->getPlayer()) {
			if (protection_zone && g_game.getRoundNr() < player->earliest_protection_zone_round) {
				return PLAYERISPZLOCKED;
			}
		}
	} else if (const Item* item = object->getItem()) {
		if (objects.size() >= 1000) {
			return NOTENOUGHROOM;
		}

		if (getBankItem() == nullptr && item->getFlag(HANG)) {
			return NOTPOSSIBLE;
		}

		if (item->getFlag(UNPASS) && getTopCreature() != nullptr) {
			return NOTENOUGHROOM;
		}

		if (getFlag(UNPASS)) {
			if (getFlag(UNLAY)) {
				if (!item->getFlag(HANG) || !getFlag(HOOKEAST) && !getFlag(HOOKSOUTH)) {
					return NOTENOUGHROOM;
				}
			}

			if (item->getFlag(UNPASS)) {
				return NOTENOUGHROOM;
			}
		}
	}

	return ALLGOOD;
}

ReturnValue_t Tile::queryMaxCount(int32_t index, const Object* object, uint32_t amount, uint32_t& max_amount, uint32_t flags) const
{
	max_amount = std::max<int32_t>(1, amount);
	return ALLGOOD;
}

Cylinder* Tile::queryDestination(int32_t& index, const Object* object, Item** to_item, uint32_t& flags)
{
	*to_item = getTopItem();
	return this;
}

void Tile::addObject(Object* object, int32_t index)
{
	object->parent = this;

	if (Item* item = object->getItem()) {
		// only allow 1 magic field 
		if (item->getFlag(MAGICFIELD)) {
			Item* previous_field = getMagicFieldItem();
			if (previous_field) {
				g_game.removeItem(previous_field, 1);
			}
		}

		// only allow 1 liquid pool
		if (item->getFlag(LIQUIDPOOL)) {
			Item* previous_pool = getLiquidPoolItem();
			if (previous_pool) {
				g_game.removeItem(previous_pool, 1);
			}
		}
	}

	if (g_game.getGameState() == GAME_STARTING) {
		const auto it = std::lower_bound(objects.begin(), objects.end(), object, [](const Object* l, const Object* r) {
			return l->getObjectPriority() <= r->getObjectPriority();
		});
		objects.insert(it, object);
	} else {
		const auto it = std::upper_bound(objects.begin(), objects.end(), object, [](const Object* l, const Object* r) {
			return l->getObjectPriority() <= r->getObjectPriority();
		});
		objects.insert(it, object);
	}
}

void Tile::removeObject(Object* object)
{
	const auto it = std::find(objects.begin(), objects.end(), object);
	if (it == objects.end()) {
		fmt::printf("ERROR - Tile::removeObject: object not found.\n");
		return;
	}

	object->parent = nullptr;
	objects.erase(it);
}

int32_t Tile::getObjectIndex(const Object* object) const
{
	int32_t index = 0;
	for (Object* help_object : objects) {
		if (help_object == object) {
			break;
		}

		index++;
	}
	return index;
}

Object* Tile::getObjectIndex(uint32_t index) const
{
	return nullptr;
}

const Position& Tile::getPosition() const
{
	return current_position;
}

bool Tile::getFlag(ItemFlags_t flag) const
{
	for (const Object* object : objects) {
		if (const Item* item = object->getItem()) {
			if (item->getFlag(flag)) {
				return true;
			}
		}
	}

	return false;
}

Object* Tile::getTopObject(bool move) const
{
	Object* top_object = nullptr;
	for (Object* object : objects) {
		top_object = object;

		if (Item* item = object->getItem()) {
			if (!item->getFlag(BANK)) {
				if (!item->getFlag(CLIP)) {
					if (!item->getFlag(BOTTOM)) {
						if (!item->getFlag(TOP)) {
							if (move) {
								break;
							}
						}
					}
				}
			}
		} else if (object->getCreature()) {
			if (!move) {
				break;
			}
		}
	}
	return top_object;
}

Item* Tile::getBankItem() const
{
	for (Object* object : objects) {
		if (Item* item = object->getItem()) {
			if (item->getFlag(BANK)) {
				return item;
			}
		}
	}

	return nullptr;
}

Item* Tile::getTopMoveItem() const
{
	Item* top_item = nullptr;
	for (Object* object : objects) {
		if (Item* item = object->getItem()) {
			if (!item->getFlag(UNMOVE)) {
				top_item = item;
			}
		}
	}

	return top_item;
}

Item* Tile::getTopItem() const
{
	Item* top_item = nullptr;
	for (Object* object : objects) {
		if (Item* item = object->getItem()) {
			top_item = item;
			if (!item->getFlag(BANK)) {
				if (!item->getFlag(CLIP)) {
					if (!item->getFlag(BOTTOM)) {
						if (!item->getFlag(TOP)) {
							break;
						}
					}
				}
			}
		}
	}

	return top_item;
}

Item* Tile::getMagicFieldItem() const
{
	for (Object* object : objects) {
		if (Item* item = object->getItem()) {
			if (item->getFlag(MAGICFIELD)) {
				return item;
			}
		}
	}
	return nullptr;
}

Item* Tile::getLiquidPoolItem() const
{
	for (Object* object : objects) {
		if (Item* item = object->getItem()) {
			if (item->getFlag(LIQUIDPOOL)) {
				return item;
			}
		}
	}
	return nullptr;
}

Creature* Tile::getTopCreature() const
{
	Creature* top_creature = nullptr;
	for (Object* object : objects) {
		if (Creature* creature = object->getCreature()) {
			top_creature = creature;
		}
	}

	return top_creature;
}
