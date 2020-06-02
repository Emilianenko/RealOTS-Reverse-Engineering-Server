#include "pch.h"

#include "player.h"
#include "protocol.h"
#include "script.h"
#include "item.h"
#include "game.h"
#include "tools.h"
#include "map.h"
#include "party.h"
#include "vocation.h"
#include "itempool.h"

void SkillManapoints::change(int16_t value)
{
	manapoints += value;

	if (manapoints < 0) {
		manapoints = 0;
	}

	if (manapoints > max_manapoints) {
		manapoints = max_manapoints;
	}

	if (Player* player = creature->getPlayer()) {
		Protocol::sendStats(player->getConnection());
	}
}

void SkillSoulpoints::change(int16_t value)
{
	soulpoints = std::min<uint16_t>(max_soulpoints, soulpoints + value);
	
	if (Player* player = creature->getPlayer()) {
		Protocol::sendStats(player->getConnection());
	}
}

void SkillFed::event(int32_t)
{
	if (Player* player = creature->getPlayer()) {
		const Vocation* vocation = player->vocation;

		if (!(getTiming() % vocation->getSecsPerHp())) {
			creature->skill_hitpoints->change(vocation->getGainHpSecs());
		}

		if (!(getTiming() % vocation->getSecsPerMp())) {
			player->skill_manapoints->change(vocation->getGainMpSecs());
		}
	}
}

void SkillMagicShield::setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value)
{
	Skill::setTiming(new_cycle, new_count, new_max_count, additional_value);

	if (Player* player = creature->getPlayer()) {
		player->checkState();
	}
}

Player::Player()
{
	creature_type = CREATURE_PLAYER;
	vocation = g_vocations.getVocationById(0);

	for (int32_t i = 0; i <= SKILL_SIZE; i++) {
		skill_next_level[i] = vocation->getExperienceForSkill(static_cast<SkillType_t>(i), skill_level[i]);
	}
}

Player::~Player()
{
	delete skill_manapoints;
	delete skill_soulpoints;
	delete skill_fed;
	delete skill_magic_shield;

	connection_ptr = nullptr;
}

bool Player::loadData(uint32_t user_id, bool set_inventory)
{
	char filename[4096];
	sprintf(filename, "%s/%02ld/%ld.%s", "usr", user_id % 100, user_id, "usr");
	std::ostringstream ss;
	ss << filename;

	ScriptReader script;
	if (!script.loadScript(ss.str())) {
		if (set_inventory) {
			fmt::printf("ERROR - Player::loadData: default user file is invalid (%d)\n", user_id);
		} else {
			fmt::printf("INFO - Player::loadData: no data file found for user (%d) loading default.\n", user_id);

			if (!set_inventory) {
				return loadData(0, true);
			}
		}
		
		return false;
	}

	while (script.canRead()) {
		script.nextToken();
		if (script.getToken() == TOKEN_IDENTIFIER) {
			const std::string identifier = script.getIdentifier();
			script.readSymbol('=');
			if (identifier == "id") {
				user_id = id = script.readNumber();
			} else if (identifier == "name") {
				name = script.readString();
			} else if (identifier == "race") {
				sex_type = static_cast<SexType_t>(script.readNumber());
			} else if (identifier == "profession") {
				vocation = g_vocations.getVocationById(script.readNumber());
				if (vocation == nullptr) {
					script.error("unknown profession");
					return false;
				}
			} else if (identifier == "originaloutfit") {
				script.readSymbol('(');
				original_outfit.look_id = script.readNumber();
				script.readSymbol(',');
				original_outfit.head = script.readNumber();
				script.readSymbol('-');
				original_outfit.body = script.readNumber();
				script.readSymbol('-');
				original_outfit.legs = script.readNumber();
				script.readSymbol('-');
				original_outfit.feet = script.readNumber();
				script.readSymbol(')');
			} else if (identifier == "currentoutfit") {
				script.readSymbol('(');
				current_outfit.look_id = script.readNumber();
				script.readSymbol(',');
				current_outfit.head = script.readNumber();
				script.readSymbol('-');
				current_outfit.body = script.readNumber();
				script.readSymbol('-');
				current_outfit.legs = script.readNumber();
				script.readSymbol('-');
				current_outfit.feet = script.readNumber();
				script.readSymbol(')');
			} else if (identifier == "lastlogin") {
				last_login_time = script.readNumber();
			} else if (identifier == "lastlogout") {
				script.readNumber();
			} else if (identifier == "startposition") {
				script.readSymbol('[');
				start_pos.x = script.readNumber();
				script.readSymbol(',');
				start_pos.y = script.readNumber();
				script.readSymbol(',');
				start_pos.z = script.readNumber();
				script.readSymbol(']');
			} else if (identifier == "currentposition") {
				script.readSymbol('[');
				current_pos.x = script.readNumber();
				script.readSymbol(',');
				current_pos.y = script.readNumber();
				script.readSymbol(',');
				current_pos.z = script.readNumber();
				script.readSymbol(']');
			} else if (identifier == "playerkillerend") {
				script.readNumber();
			} else if (identifier == "skill") {
				script.readSymbol('(');
				const uint8_t skill_nr = script.readNumber();
				if (skill_nr < 0 || skill_nr > 49) {
					script.error("illegal skill number");
					return false;
				}

				script.readSymbol(',');
				const int32_t actual = script.readNumber();
				script.readSymbol(',');
				int32_t maximum = script.readNumber();
				script.readSymbol(',');
				int32_t minimum = script.readNumber();
				script.readSymbol(',');
				int32_t delta_act = script.readNumber();
				script.readSymbol(',');
				int32_t magidelta_act = script.readNumber();
				script.readSymbol(',');
				int32_t cycle = script.readNumber();
				script.readSymbol(',');
				int32_t max_cycle = script.readNumber();
				script.readSymbol(',');
				int32_t count = script.readNumber();
				script.readSymbol(',');
				int32_t max_count = script.readNumber();
				script.readSymbol(',');
				int32_t add_level = script.readNumber();
				script.readSymbol(',');
				int32_t exp = script.readNumber();
				script.readSymbol(',');
				int32_t factor_percent = script.readNumber();
				script.readSymbol(',');
				int32_t next_level = script.readNumber();
				script.readSymbol(',');
				int32_t delta = script.readNumber();
				script.readSymbol(')');

				if (skill_nr == 0) {
					skill_level[SKILL_LEVEL] = actual;
					skill_experience[SKILL_LEVEL] = exp;
					skill_next_level[SKILL_LEVEL] = next_level;
				}

				if (skill_nr == 1) {
					skill_level[SKILL_MAGIC] = actual;
					skill_experience[SKILL_MAGIC] = exp;
					skill_next_level[SKILL_MAGIC] = vocation->getExperienceForSkill(SKILL_MAGIC, actual + 1);

				}

				if (skill_nr == 2) {
					skill_hitpoints->setValue(actual);
					skill_hitpoints->setMax(maximum);
				}

				if (skill_nr == 3) {
					skill_manapoints->setValue(actual);
					skill_manapoints->setMax(maximum);
				}
				
				if (skill_nr == 4) {
					skill_go_strength->setBaseSpeed(actual);
				}

				if (skill_nr == 5) {
					capacity = actual;
				}

				if (skill_nr == 6) {
					skill_level[SKILL_SHIELDING] = actual;
					skill_experience[SKILL_SHIELDING] = 0;
					skill_next_level[SKILL_SHIELDING] = vocation->getExperienceForSkill(SKILL_SHIELDING, actual + 1);
				}

				if (skill_nr == 7) {
					skill_level[SKILL_DISTANCEFIGHTING] = actual;
					skill_experience[SKILL_DISTANCEFIGHTING] = 0;
					skill_next_level[SKILL_DISTANCEFIGHTING] = vocation->getExperienceForSkill(SKILL_DISTANCEFIGHTING, actual + 1);
				}

				if (skill_nr == 8) {
					skill_level[SKILL_SWORDFIGHTING] = actual;
					skill_experience[SKILL_SWORDFIGHTING] = 0;
					skill_next_level[SKILL_SWORDFIGHTING] = vocation->getExperienceForSkill(SKILL_SWORDFIGHTING, actual + 1);
				}

				if (skill_nr == 9) {
					skill_level[SKILL_CLUBFIGHTING] = actual;
					skill_experience[SKILL_CLUBFIGHTING] = 0;
					skill_next_level[SKILL_CLUBFIGHTING] = vocation->getExperienceForSkill(SKILL_CLUBFIGHTING, actual + 1);
				}

				if (skill_nr == 10) {
					skill_level[SKILL_AXEFIGHTING] = actual;
					skill_experience[SKILL_AXEFIGHTING] = 0;
					skill_next_level[SKILL_AXEFIGHTING] = vocation->getExperienceForSkill(SKILL_AXEFIGHTING, actual + 1);
				}

				if (skill_nr == 11) {
					skill_level[SKILL_FISTFIGHTING] = actual;
					skill_experience[SKILL_FISTFIGHTING] = 0;
					skill_next_level[SKILL_FISTFIGHTING] = vocation->getExperienceForSkill(SKILL_FISTFIGHTING, actual + 1);
				}

				if (skill_nr == 13) {
					skill_level[SKILL_FISHING] = actual;
					skill_experience[SKILL_FISHING] = 0;
					skill_next_level[SKILL_FISHING] = vocation->getExperienceForSkill(SKILL_FISHING, actual + 1);
				}

				if (skill_nr == 14) {
					skill_fed->setTiming(cycle, count, max_count, -1);
				}

				if (skill_nr == 15) {
					skill_light->setTiming(cycle, count, max_count, -1);
				}

				if (skill_nr == 17) {
					skill_poison->setTiming(cycle, count, max_count, -1);
				}

				if (skill_nr == 18) {
					skill_burning->setTiming(cycle, count, max_count, -1);
				}

				if (skill_nr == 19) {
					skill_energy->setTiming(cycle, count, max_count, -1);
				}

				if (skill_nr == 20) {
					skill_drunken->setTiming(cycle, count, max_count, -1);
				}

				if (skill_nr == 21) {
					skill_magic_shield->setTiming(cycle, count, max_count, -1);
				}

				if (skill_nr == 22) {
					skill_soulpoints->setSoulpoints(actual);
					skill_soulpoints->setMaxSoulpoints(maximum);
				}

				for (int32_t i = 0; i <= SKILL_SIZE; i++) {
					skill_percent[i] = getPercentToGo(skill_experience[i], skill_next_level[i]);
				}
			} else if (identifier == "spells") {
				script.readSymbol('{');
				while (script.canRead()) {
					script.nextToken();
					if (script.getToken() == TOKEN_NUMBER) {
						// ignored
					} else {
						if (script.getSpecial() == ',') {
							continue;
						} if (script.getSpecial() != ',' && script.getSpecial() != '}') {
							script.error("'}' expected");
							return false;
						}

						break;
					}
				}
			} else if (identifier == "questvalues") {
				script.readSymbol('{');
				while (script.canRead()) {
					script.nextToken();
					if (script.getToken() == TOKEN_SPECIAL) {
						if (script.getSpecial() == '(') {
							script.readNumber(); // quest number
							script.readSymbol(',');
							script.readNumber(); // quest value
							script.readSymbol(')');
						} else if (script.getSpecial() == ',') {
							continue;
						} else if (script.getSpecial() != '}') {
							script.error("'}' expected");
							return false;
						}
						
						break;
					}
				}
			} else if (identifier == "murders") {
				script.readSymbol('{');
				while (script.canRead()) {
					script.nextToken();
					if (script.getToken() == TOKEN_NUMBER) {
						// ignored
					} else {
						if (script.getSpecial() == ',') {
							continue;
						} 
						
						if (script.getSpecial() != '}') {
							script.error("'}' expected");
							return false;
						}
						
						break;
					}
				}
			} else if (identifier == "depots") {
				// skip, depots come last
				return true;
			} else if (identifier == "inventory") {
				script.readSymbol('{');
				script.nextToken();
				while (script.canRead()) {
					if (script.getToken() == TOKEN_NUMBER) {
						const InventorySlot_t slot = static_cast<InventorySlot_t>(script.getNumber());

						if (script.readIdentifier() != "content") {
							script.error("content expected");
							return false;
						}

						script.readSymbol('=');
						script.readSymbol('{');
						script.nextToken();

						if (script.getToken() == TOKEN_NUMBER) {
							const uint16_t type_id = script.getNumber();
							if (type_id != 0) {
								Item* item = g_itempool.createItem(type_id);
								if (!item) {
									fmt::printf("ERROR - Player::loadData: invalid item (typeid:%d)\n", type_id);
									return false;
								}

								addObject(item, slot);

								if (!item->loadData(script)) {
									fmt::printf("ERROR - Player::loadData: failed to load item data (typeid:%d)\n", type_id);
									return false;
								}
							}
						}

						if (script.getSpecial() != '}') {
							script.error("'}' expected");
							return false;
						}

						script.nextToken();
					} else if (script.getToken() == TOKEN_SPECIAL) {
						if (script.getSpecial() == '}') {
							break;
						}
						
						if (script.getSpecial() == ',') {
							script.nextToken();
						} 
					} else {
						script.error("'}' expected");
						return false;
					}
				}
			} else {
				ss.clear();
				ss << "unknown identifier '" << identifier << '\'';
				script.error(ss.str());
				return false;
			}
		}
	}
	return true;
}

void Player::takeOver(Connection_ptr new_connection)
{
	if (connection_ptr) {
		connection_ptr->close();
		connection_ptr = nullptr;
	}

	setConnection(new_connection);
	new_connection->setPlayer(this);

	open_containers.clear();

	Protocol::sendInitGame(new_connection);
	Protocol::sendAmbiente(new_connection);
	Protocol::sendStats(new_connection);
}

void Player::changeManaPoints(int32_t value) const
{
	skill_manapoints->change(value);
}

Item* Player::getInventoryItem(InventorySlot_t slot) const
{
	return inventory[slot];
}

Item* Player::getWeapon() const
{
	Item* left = inventory[INVENTORY_LEFT];
	if (left && (left->getFlag(WEAPON) || left->getFlag(WAND) || left->getFlag(BOW) || left->getFlag(THROWABLE))) {
		return left;
	}

	Item* right = inventory[INVENTORY_RIGHT];
	if (right && (right->getFlag(WEAPON) || right->getFlag(WAND) || right->getFlag(BOW) || right->getFlag(THROWABLE))) {
		return right;
	}

	return nullptr;
}

Item* Player::getShield() const
{
	Item* left = inventory[INVENTORY_LEFT];
	if (left && left->getFlag(SHIELD)) {
		return left;
	}

	Item* right = inventory[INVENTORY_RIGHT];
	if (right && right->getFlag(SHIELD)) {
		return right;
	}

	return nullptr;
}

Skull_t Player::getKillingMark(const Player* observer) const
{
	if (player_killer_end) {
		return SKULL_RED;
	}

	Skull_t skull = SKULL_WHITE;
	if (!aggressor) {
		if (party && party == observer->party) {
			skull = SKULL_GREEN;
		} else if (isAttacker(observer, false)) {
			skull = SKULL_YELLOW;
		} else {
			skull = SKULL_NONE;
		}
	}

	return skull;
}

PartyShields_t Player::getPartyMark(const Player* observer) const
{
	if (party) {
		if (party->getHost() == observer) {
			return SHIELD_YELLOW;
		}

		if (party == observer->party) {
			return SHIELD_BLUE;
		}

		if (party->getHost() == this && party->isInvited(observer)) {
			return SHIELD_WHITEBLUE;
		}
	}

	if (Party* other_party = observer->party) {
		if (other_party->getHost() == observer && other_party->isInvited(this)) {
			return SHIELD_WHITEYELLOW;
		}
	}

	return SHIELD_NONE;
}

uint32_t Player::getFreeCapacity() const
{
	const uint32_t inventory_weight = getInventoryWeight();
	const uint32_t carry_strength = 100 * capacity;
	return std::max<uint32_t>(0, carry_strength - inventory_weight);
}

uint32_t Player::getInventoryWeight() const
{
	uint32_t weight = 0;
	for (const Item* item : inventory) {
		if (item == nullptr) {
			continue;
		}

		weight += item->getWeight();
	}

	return weight;
}

std::string Player::getObjectDescription(const Creature* creature) const
{
	std::ostringstream ss;
	if (creature == this) {
		ss << "yourself. You are ";

		if (vocation->getId() == 0) {
			ss << "have no vocation";
		} else {
			ss << vocation->getDescription();
		}
	} else {
		ss << name << " (Level " << skill_level[SKILL_LEVEL] << "). ";
		if (sex_type == SEX_MALE) {
			ss << "He ";
		} else {
			ss << "She ";
		}


		if (vocation->getId() == 0) {
			ss << "has no vocation";
		} else {
			ss << "is " << vocation->getDescription();
		}
	}

	ss << '.';
	return ss.str();
}

ReturnValue_t Player::queryAdd(int32_t index, const Object* object, uint32_t amount, uint32_t flags, Creature* actor) const
{
	const Item* item = object->getItem();
	if (item == nullptr) {
		return NOTPOSSIBLE;
	}

	if (!item->getFlag(TAKE)) {
		return CANNOTPICKUP;
	}

	const bool child_is_owner = hasBitSet(FLAG_CHILDISOWNER, flags);
	if (child_is_owner) {
		//a child container is querying the player, just check if enough capacity
		const bool skip_limit = hasBitSet(FLAG_NOLIMIT, flags);
		if (skip_limit || item->getTopParent() == this || getFreeCapacity() >= item->getWeight()) {
			return ALLGOOD;
		}
		return NOTENOUGHCAPACITY;
	}

	const uint8_t body_pos = item->getAttribute(BODYPOSITION);
	ReturnValue_t ret = ALLGOOD;

	if (index == INDEX_ANYWHERE) {
		ret = NOTENOUGHROOM;
	} else if (index != INVENTORY_EXTRA && index != INVENTORY_LEFT && index != INVENTORY_RIGHT) {
		if (body_pos != index) {
			ret = CANNOTBEDRESSED;
		}
	} else if (index == INVENTORY_LEFT) {
		if (body_pos == 0) {
			if (inventory[INVENTORY_RIGHT] && inventory[INVENTORY_RIGHT] != item) {
				ret = BOTHHANDSNEEDTOBEFREE;
			}
		} else if (inventory[INVENTORY_RIGHT]) {
			const Item* right_item = inventory[INVENTORY_RIGHT];
			const int32_t right_item_pos = right_item->getAttribute(BODYPOSITION);

			if (right_item_pos == 0) {
				ret = DROPTWOHANDEDITEM;
			} else if (item == right_item && amount == item->getAttribute(ITEM_AMOUNT)) {
				ret = ALLGOOD;
			} else if (right_item->getFlag(SHIELD) && item->getFlag(SHIELD)) {
				ret = CANONLYUSEONESHIELD;
			} else if (right_item->getFlag(WEAPON) && item->getFlag(WEAPON)) {
				ret = CANONLYUSEONEWEAPON;
			}
		}
	} else if (index == INVENTORY_RIGHT) {
		if (body_pos == 0) {
			if (inventory[INVENTORY_LEFT] && inventory[INVENTORY_LEFT] != item) {
				ret = BOTHHANDSNEEDTOBEFREE;
			}
		} else if (inventory[INVENTORY_LEFT]) {
			const Item* right_item = inventory[INVENTORY_LEFT];
			const int32_t right_item_pos = right_item->getAttribute(BODYPOSITION);

			if (right_item_pos == 0) {
				ret = DROPTWOHANDEDITEM;
			} else if (item == right_item && amount == item->getAttribute(ITEM_AMOUNT)) {
				ret = ALLGOOD;
			} else if (right_item->getFlag(SHIELD) && item->getFlag(SHIELD)) {
				ret = CANONLYUSEONESHIELD;
			} else if (right_item->getFlag(WEAPON) && item->getFlag(WEAPON)) {
				ret = CANONLYUSEONEWEAPON;
			}
		}
	}

	if (ret == ALLGOOD || ret == NOTENOUGHROOM) {
		const Item* inventory_item = nullptr;
		if (index >= INVENTORY_HEAD && index <= INVENTORY_EXTRA) {
			inventory_item = inventory[index];
		}

		if (inventory_item && (!inventory_item->getFlag(CUMULATIVE) || inventory_item->getId() != item->getId())) 			{
			return NEEDEXCHANGE;
		}

		if (item->getTopParent() != this && getFreeCapacity() < item->getWeight()) {
			return NOTENOUGHCAPACITY;
		}
	}

	return ret;
}

ReturnValue_t Player::queryMaxCount(int32_t index, const Object* object, uint32_t amount, uint32_t& max_amount, uint32_t flags) const
{
	const Item* item = object->getItem();
	if (item == nullptr) {
		return NOTPOSSIBLE;
	}

	if (index == INDEX_ANYWHERE) {
		uint32_t n = 0;
		for (int32_t slot_index = INVENTORY_HEAD; slot_index <= INVENTORY_EXTRA; ++slot_index) {
			Item* inventory_item = inventory[slot_index];
			if (inventory_item) {
				if (inventory_item->getFlag(CONTAINER)) {
					uint32_t query_count = 0;
					inventory_item->queryMaxCount(INDEX_ANYWHERE, item, item->getAttribute(ITEM_AMOUNT), query_count, flags);
					n += query_count;
				} else if (inventory_item->getFlag(CUMULATIVE) && item->getId() == inventory_item->getId() && inventory_item->getAttribute(ITEM_AMOUNT) < 100) {
					const uint32_t remainder = (100 - inventory_item->getAttribute(ITEM_AMOUNT));

					if (queryAdd(slot_index, item, remainder, flags, nullptr) == ALLGOOD) {
						n += remainder;
					}
				}
			} else if (queryAdd(slot_index, item, item->getAttribute(ITEM_AMOUNT), flags, nullptr) == ALLGOOD) { //empty slot
				if (item->getFlag(CUMULATIVE)) {
					n += 100;
				} else {
					++n;
				}
			}
		}

		max_amount = n;
	} else {
		const Item* dest_item = inventory[index];
		if (dest_item) {
			if (dest_item->getFlag(CUMULATIVE) && item->getId() == dest_item->getId() && dest_item->getAttribute(ITEM_AMOUNT) < 100) {
				max_amount = 100 - dest_item->getAttribute(ITEM_AMOUNT);
			} else {
				max_amount = 0;
			}
		} else if (queryAdd(index, item, amount, flags, nullptr) == ALLGOOD) { //empty slot
			if (item->getFlag(CUMULATIVE)) {
				max_amount = 100;
			} else {
				max_amount = 1;
			}

			return ALLGOOD;
		}
	}

	if (max_amount < amount) {
		return NOTENOUGHROOM;
	}
	
	return ALLGOOD;
}

Cylinder* Player::queryDestination(int32_t& index, const Object* object, Item ** to_item, uint32_t& flags)
{
	if (index == 0 /*drop to capacity window*/ || index == INDEX_ANYWHERE) {
		*to_item = nullptr;

		const Item* item = object->getItem();
		if (item == nullptr) {
			return this;
		}

		const bool is_stackable = item->getFlag(CUMULATIVE);

		std::vector<Item*> containers;

		for (uint32_t slot_index = INVENTORY_HEAD; slot_index <= INVENTORY_EXTRA; ++slot_index) {
			Item* inventory_item = inventory[slot_index];
			if (inventory_item) {
				if (inventory_item == trading_item) {
					continue;
				}

				if (inventory_item == item) {
					continue;
				}

				if (is_stackable) {
					//try find an already existing item to stack with
					if (queryAdd(slot_index, item, item->getAttribute(ITEM_AMOUNT), 0, nullptr) == ALLGOOD) {
						if (inventory_item->getId() == item->getId() && inventory_item->getAttribute(ITEM_AMOUNT) < 100) {
							index = slot_index;
							*to_item = inventory_item;
							return this;
						}
					}

					if (inventory_item->getFlag(CONTAINER)) {
						containers.push_back(inventory_item);
					}
				} else if (inventory_item->getFlag(CONTAINER)) {
					containers.push_back(inventory_item);
				}
			} else if (queryAdd(slot_index, item, item->getAttribute(ITEM_AMOUNT), flags, nullptr) == ALLGOOD) { //empty slot
				index = slot_index;
				*to_item = nullptr;
				return this;
			}
		}

		size_t i = 0;
		while (i < containers.size()) {
			Item* tmp_container = containers[i++];
			if (!is_stackable) {
				//we need to find first empty container as fast as we can for non-stackable items
				uint32_t n = tmp_container->getAttribute(CAPACITY) - tmp_container->getItems().size();
				while (n) {
					if (tmp_container->queryAdd(tmp_container->getAttribute(CAPACITY) - n, item, item->getAttribute(ITEM_AMOUNT), flags, nullptr) == ALLGOOD) {
						index = tmp_container->getAttribute(CAPACITY) - n;
						*to_item = nullptr;
						return tmp_container;
					}

					n--;
				}

				continue;
			}

			int32_t n = 0;

			for (Item* tmp_item : tmp_container->getItems()) {
				if (tmp_item == trading_item) {
					continue;
				}

				if (tmp_item == item) {
					continue;
				}

				//try find an already existing item to stack with
				if (tmp_item->getId() == item->getId() && tmp_item->getAttribute(ITEM_AMOUNT) < 100) {
					index = n;
					*to_item = tmp_item;
					return tmp_container;
				}

				n++;
			}

			if (n < tmp_container->getAttribute(CAPACITY) && tmp_container->queryAdd(n, item, item->getAttribute(ITEM_AMOUNT), flags, nullptr) == ALLGOOD) {
				index = n;
				*to_item = nullptr;
				return tmp_container;
			}
		}

		return this;
	}

	Item* dest_item = nullptr;
	if (index >= INVENTORY_HEAD && index <= INVENTORY_EXTRA) {
		dest_item = inventory[index];
		*to_item = dest_item;
	}

	if (dest_item && dest_item->getFlag(CONTAINER)) {
		index = INDEX_ANYWHERE;
		*to_item = nullptr;
		return dest_item;
	} else {
		return this;
	}
}

void Player::addObject(Object* object, int32_t index)
{
	if (index < INVENTORY_HEAD || index > INVENTORY_EXTRA) {
		fmt::printf("ERROR - Player::addObject: index outside of inventory (index:%d)\n", index);
		return;
	}

	Item* item = object->getItem();
	if (item == nullptr) {
		return;
	}

	item->setParent(this);
	inventory[index] = item;
}

void Player::removeObject(Object* object)
{
	Item* item = object->getItem();
	if (item == nullptr) {
		return;
	}

	for (uint8_t index = INVENTORY_HEAD; index <= INVENTORY_EXTRA; index++) {
		if (inventory[index] == object) {
			object->setParent(nullptr);
			inventory[index] = nullptr;
			return;
		}
	}

	fmt::printf("ERROR - Player::removeObject: item (typeid:%d) not found in player inventory.\n", item->getId());
}

int32_t Player::getObjectIndex(const Object* object) const
{
	for (int32_t index = INVENTORY_HEAD; index <= INVENTORY_EXTRA; index++) {
		if (inventory[index] == object) {
			return index;
		}
	}

	return -1;
}

Object* Player::getObjectIndex(uint32_t index) const
{
	if (index < INVENTORY_HEAD || index > INVENTORY_EXTRA) {
		fmt::printf("ERROR - Player::getObjectIndex: index outside of inventory (index:%d)\n", index);
		return nullptr;
	}

	return inventory[index];
}

const Position& Player::getPosition() const
{
	if (parent) {
		return parent->getPosition();
	}

	return current_pos;
}

bool Player::isAttacker(const Player* victim, bool check_former) const
{
	for (uint32_t player_id : attacked_players) {
		if (player_id == victim->id) {
			return true;
		}
	}

	if (!check_former) {
		return false;
	}

	if (g_game.getRoundNr() > former_logout_round) {
		return false;
	}

	for (uint32_t player_id : former_attacked_players) {
		if (player_id == victim->id) {
			return true;
		}
	}

	return false;
}

bool Player::isAttackJustified(const Player* victim) const
{
	// player has skull
	if (victim->player_killer_end > 0) {
		return true;
	}

	if (victim->aggressor || g_game.getRoundNr() <= victim->former_logout_round + 5 && victim->former_aggressor) {
		return true;
	}

	if (party && party == victim->party) {
		return true;
	}

	return victim->isAttacker(this, true);
}

void Player::recordMurder(Player* victim)
{
	if (isAttackJustified(victim)) {
		return;
	}

	murder_timestamps.emplace(std::time(nullptr));

	Protocol::sendTextMessage(connection_ptr, MESSAGE_WARNING, fmt::sprintf("Warning! The murder of %s was not justified.", victim->getName()));
	
	const int32_t killing_state = checkPlayerKilling();
	if (killing_state) {
		const bool skull_change = player_killer_end == 0;
		player_killer_end = std::time(nullptr) + 2592000; // 1 month red skull
		// todo: banishment == killing_state 2
		if (skull_change) {
			g_game.announceChangedCreature(this, CREATURE_SKULL);
		}
	}
}

void Player::recordAttack(Player* victim)
{
	if (victim->id == id) {
		return;
	}

	if (!victim->isAttacker(this, true) && !isAttacker(victim, false)) {
		if (!party || party != victim->party) {
			attacked_players.emplace(victim->id);
			Protocol::sendCreatureSkull(victim->connection_ptr, this);
		}
	}

	if (!isAttackJustified(victim) && !aggressor) {
		aggressor = true;
		g_game.announceChangedCreature(this, CREATURE_SKULL);
	}
}

void Player::clearAttacker(Player* victim)
{
	if (attacked_players.empty()) {
		return;
	}

	const auto it = attacked_players.find(victim->id);
	if (it == attacked_players.end()) {
		return;
	}

	attacked_players.erase(it);
	Protocol::sendCreatureSkull(victim->connection_ptr, this);
}

void Player::clearKillingMarks()
{
	former_aggressor = aggressor;
	former_logout_round = g_game.getRoundNr();

	for (uint32_t player_id : attacked_players) {
		former_attacked_players.emplace(player_id);
	}

	if (aggressor) {
		aggressor = false;
		attacked_players.clear();
		g_game.announceChangedCreature(this, CREATURE_SKULL);
	}

	for (uint32_t player_id : attacked_players) {
		Player* player = g_game.getPlayerByUserId(player_id);
		if (player) {
			Protocol::sendCreatureSkull(player->connection_ptr, this);
		}
	}

	for (Player* player : g_game.getPlayers()) {
		player->clearAttacker(this);
	}
}

void Player::addExperience(SkillType_t skill, int64_t value)
{
	skill_experience[skill] += value;

	while (skill_experience[skill] >= skill_next_level[skill]) {
		skill_level[skill] += 1;
		skill_percent[skill] = 0;
		skill_next_level[skill] = vocation->getExperienceForSkill(skill, skill_level[skill] + 1);

		if (skill != SKILL_LEVEL) {
			skill_experience[skill] = 0;
		}

		switch (skill) {
			case SKILL_FISTFIGHTING: 
				Protocol::sendTextMessage(connection_ptr, MESSAGE_EVENT_ADVANCE, "You advanced in fist fighting.");
				break;
			case SKILL_SWORDFIGHTING:
				Protocol::sendTextMessage(connection_ptr, MESSAGE_EVENT_ADVANCE, "You advanced in sword fighting.");
				break;
			case SKILL_CLUBFIGHTING:
				Protocol::sendTextMessage(connection_ptr, MESSAGE_EVENT_ADVANCE, "You advanced in club fighting.");
				break;
			case SKILL_AXEFIGHTING:
				Protocol::sendTextMessage(connection_ptr, MESSAGE_EVENT_ADVANCE, "You advanced in axe fighting.");
				break;
			case SKILL_DISTANCEFIGHTING: 
				Protocol::sendTextMessage(connection_ptr, MESSAGE_EVENT_ADVANCE, "You advanced in distance fighting.");
				break;
			case SKILL_SHIELDING: 
				Protocol::sendTextMessage(connection_ptr, MESSAGE_EVENT_ADVANCE, "You advanced in shielding.");
				break;
			case SKILL_FISHING:
				Protocol::sendTextMessage(connection_ptr, MESSAGE_EVENT_ADVANCE, "You advanced in fishing.");
				break;
			case SKILL_MAGIC: 
				Protocol::sendTextMessage(connection_ptr, MESSAGE_EVENT_ADVANCE, fmt::sprintf("You advanced to magic level %d.", skill_level[skill]));
				break;
			case SKILL_LEVEL: {
				skill_go_strength->setBaseSpeed(std::max<int32_t>(0, skill_go_strength->getBaseSpeed() + 1));
				skill_hitpoints->setMax(skill_hitpoints->getMax() + vocation->getGainHp());
				skill_manapoints->setMax(skill_manapoints->getMax() + vocation->getGainMp());

				capacity += vocation->getGainCapacity();

				Protocol::sendTextMessage(connection_ptr, MESSAGE_EVENT_ADVANCE, fmt::sprintf("You advanced from level %d to level %d.", skill_level[skill] - 1, skill_level[skill]));
				break;
			}
			default:;
		}

		if (skill == SKILL_MAGIC || skill == SKILL_LEVEL) {
			Protocol::sendStats(connection_ptr);
		} else {
			Protocol::sendSkills(connection_ptr);
		}
	}

	const uint8_t old_percent = skill_percent[skill];
	if (skill == SKILL_LEVEL) {
		const uint64_t current_level_exp = vocation->getExperienceForSkill(skill, skill_level[skill]);
		skill_percent[skill] = getPercentToGo(skill_experience[skill] - current_level_exp, skill_next_level[skill] - current_level_exp);
	} else {
		skill_percent[skill] = getPercentToGo(skill_experience[skill], skill_next_level[skill]);
	}

	if (skill_percent[skill] != old_percent && skill != SKILL_LEVEL && skill != SKILL_MAGIC) {
		Protocol::sendSkills(connection_ptr);
	}

	if (skill == SKILL_LEVEL || skill == SKILL_MAGIC) {
		Protocol::sendStats(connection_ptr);
	}
}

void Player::removeExperience(SkillType_t skill, int64_t value)
{
	if (skill == SKILL_LEVEL) {
		skill_experience[skill] = std::max<int64_t>(0, skill_experience[skill] - value);

		const uint16_t old_level = skill_level[skill];

		while (skill_level[skill] > 1 && skill_experience[skill] < vocation->getExperienceForSkill(skill, skill_level[skill])) {
			skill_level[skill]--;

			skill_go_strength->setBaseSpeed(std::max<int32_t>(0, skill_go_strength->getBaseSpeed() - 1));
			skill_hitpoints->setMax(std::max<int32_t>(150, skill_hitpoints->getMax() - vocation->getGainHp()));
			skill_manapoints->setMax(std::max<int32_t>(0, skill_manapoints->getMax() - vocation->getGainMp()));

			capacity = std::max<int32_t>(0, capacity - vocation->getGainCapacity());
		}

		if (old_level != skill_level[skill]) {
			Protocol::sendTextMessage(connection_ptr, MESSAGE_EVENT_ADVANCE, fmt::sprintf("You were downgraded from level %d to level %d.", old_level, skill_level[skill]));
		}

		const uint64_t current_level_exp = vocation->getExperienceForSkill(skill, skill_level[skill]);
		skill_next_level[skill] = vocation->getExperienceForSkill(skill, skill_level[skill] + 1);

		if (skill_next_level[skill] > current_level_exp) {
			skill_percent[skill] = getPercentToGo(skill_experience[skill] - current_level_exp, skill_next_level[skill] - current_level_exp);
		} else {
			skill_percent[skill] = 0;
		}

		Protocol::sendStats(connection_ptr);
	} else {
		uint16_t min_skill = 10;
		if (skill == SKILL_MAGIC) {
			min_skill = 0;
		}

		while (value > skill_experience[skill] && skill_level[skill] > min_skill) {
			value -= skill_experience[skill];
			skill_experience[skill] = vocation->getExperienceForSkill(skill, skill_level[skill]);
			skill_level[skill]--;
			skill_next_level[skill] = vocation->getExperienceForSkill(skill, skill_level[skill] + 1);
		}

		skill_experience[skill] = std::max<int64_t>(0, skill_experience[skill] - value);
		skill_percent[skill] = getPercentToGo(skill_experience[skill], skill_next_level[skill]);

		if (skill == SKILL_MAGIC) {
			Protocol::sendStats(connection_ptr);
		} else {
			Protocol::sendSkills(connection_ptr);
		}
	}
}

int32_t Player::checkPlayerKilling()
{
	const uint32_t today = std::time(nullptr);
	uint32_t last_day = 0;
	uint32_t last_week = 0;
	uint32_t last_month = 0;
	const uint32_t day_timestamp = today - 86400;
	const uint32_t week_timestamp = today - 604800;
	const uint32_t month_timestamp = today - 2592000;

	for (uint32_t timestamp : murder_timestamps) {
		if (timestamp > day_timestamp) {
			++last_day;
		}

		if (timestamp > week_timestamp) {
			++last_week;
		}

		uint32_t month_check = last_month + 1;

		if (timestamp <= month_timestamp) {
			month_check = last_month;
		}

		last_month = month_check;
	}

	if (last_week > 5 || last_day > 9 || last_month > 19) {
		return 2;
	}

	if (last_week > 2 || last_day > 4 || last_month > 9) {
		return 1;
	}

	return 0;
}

int32_t Player::getAttackDamage() const
{
	const Item* item = getWeapon();
	if (item == nullptr) {
		return 7; // fist fighting
	}

	if (item->getFlag(WEAPON)) {
		return item->getAttribute(WEAPONATTACKVALUE);
	}
	
	if (item->getFlag(THROWABLE)) {
		return item->getAttribute(THROWATTACKVALUE);
	}
	
	if (item->getFlag(BOW)) {
		const Item* ammo = inventory[INVENTORY_EXTRA];
		if (ammo && ammo->getAttribute(AMMOTYPE) == item->getAttribute(BOWAMMOTYPE)) {
			return ammo->getAttribute(AMMOATTACKVALUE);
		}
	}

	return 0;
}

int32_t Player::getDefense() const
{
	for (Item* item : inventory) {
		if (item == nullptr) {
			continue;
		}

		if (item->getFlag(SHIELD)) {
			return item->getAttribute(SHIELDDEFENDVALUE);
		}

		if (item->getFlag(WEAPON)) {
			return item->getAttribute(WEAPONDEFENDVALUE);
		}

		if (item->getFlag(THROWABLE)) {
			return item->getAttribute(THROWDEFENDVALUE);
		}
	}
	return 0;
}

int32_t Player::getArmor() const
{
	int32_t armor = 5;
	for (int32_t slot = 0; slot <= INVENTORY_EXTRA; slot++) {
		const Item* item = inventory[slot];
		if (item == nullptr) {
			continue;
		}

		if (item->getFlag(CLOTHES)) {
			if (item->getAttribute(BODYPOSITION) == slot) {
				armor += item->getAttribute(ARMORVALUE);
			}
		}
	}

	if (armor > 1) {
		armor = rand() % (armor >> 1) + (armor >> 1);	
	}

	return armor;
}

int32_t Player::probeSkill(SkillType_t skill, int32_t value, int32_t prob)
{
	if (learning_points > 0) {
		addExperience(skill, 1);
	}

	if (prob == 0) {
		const int32_t f = (5 * (skill_level[skill]) + 50) * value;
		const int32_t r = rand() % 100;
		return f * ((rand() % 100 + r) / 2) / 10000;
	}

	if (rand() % value <= skill_level[skill]) {
		return rand() % 100 <= prob;
	}

	return 0;
}

void Player::addContainer(uint8_t container_id, Item* item)
{
	if (container_id > 0x0F) {
		return;
	}

	const auto it = open_containers.find(container_id);
	if (it != open_containers.end()) {
		it->second = item;
	} else {
		open_containers[container_id] = item;
	}
}

void Player::closeContainer(uint8_t container_id)
{
	const auto it = open_containers.find(container_id);
	if (it == open_containers.end()) {
		return;
	}

	open_containers.erase(it);
}

Item* Player::getContainerById(uint8_t container_id)
{
	const auto it = open_containers.find(container_id);
	if (it == open_containers.end()) {
		return nullptr;
	}

	return it->second;
}

int8_t Player::getContainerId(const Item* item) const
{
	for (const auto& it : open_containers) {
		if (it.second == item) {
			return it.first;
		}
	}
	return -1;
}

int64_t Player::checkForMuting() const
{
	if (muting_end > g_game.serverMilliseconds()) {
		return muting_end - g_game.serverMilliseconds();
	}
	return 0;
}

int64_t Player::recordTalk()
{
	uint64_t interval = g_game.serverMilliseconds() + 2500;
	if (g_game.serverMilliseconds() < talk_buffer_full_time) {
		if (g_game.serverMilliseconds() < talk_buffer_full_time - 7500) {
			number_of_mutings++;
			const int32_t result = 5 * number_of_mutings * number_of_mutings;
			muting_end = (result * 1000) + g_game.serverMilliseconds();
			return result;
		}
		interval = talk_buffer_full_time + 2500;
	}
	talk_buffer_full_time = interval;
	return 0;
}

int64_t Player::recordMessage(uint32_t address)
{
	int32_t update_index = -1;
	for (int32_t i = 0; i <= 19; i++) {
		const uint32_t prev_address = addresses[i];
		if (prev_address == address) {
			addresses_times[i] = g_game.serverMilliseconds();
			return 0;
		}

		if (prev_address == 0 || addresses_times[i] < g_game.serverMilliseconds() - 10 * 60 * 1000) {
			update_index = i;
		}
	}

	if (update_index >= 0) {
		addresses[update_index] = address;
		addresses_times[update_index] = g_game.serverMilliseconds();
		return 0;
	}

	number_of_mutings++;
	const int64_t result = 5 * number_of_mutings * number_of_mutings;
	muting_end = (result * 1000) + g_game.serverMilliseconds();
	return result;
}

void Player::blockLogout(uint32_t delay, bool block_pz)
{
	if (block_pz) {
		earliest_protection_zone_round = g_game.getRoundNr() + delay;
	}

	earliest_logout_round = g_game.getRoundNr() + delay;
	checkState();
}

void Player::checkState()
{
	int32_t states = 0;
	if (skill_poison->getTiming() >= 1) {
		states = PLAYER_STATE_POISONING;
	}

	if (skill_burning->getTiming() >= 1) {
		states |= PLAYER_STATE_BURNING;
	}

	if (skill_energy->getTiming() >= 1) {
		states |= PLAYER_STATE_ELECTRIFIED;
	}

	if (skill_drunken->getTiming() >= 1) {
		states |= PLAYER_STATE_DRUNKEN;
	}

	if (skill_magic_shield->getTiming() >= 1) {
		states |= PLAYER_STATE_MAGIC_SHIELD;
	}

	const int32_t speed_mdact = skill_go_strength->getDelta();
	if (speed_mdact <= -1) {
		states |= PLAYER_STATE_PARALYZED; // paralyze
	} else if (speed_mdact >= 1) {
		states |= PLAYER_STATE_HASTED; // haste
	}

	if (earliest_logout_round > g_game.getRoundNr()) {
		states |= PLAYER_STATE_INFIGHT; // in fight
	}

	if (states != old_state) {
		Protocol::sendPlayerState(connection_ptr, states);
		old_state = states;
	}
}

void Player::notifyTrades()
{
	if (trading_item && trade_partner) {
		const Position& pos = getPosition();
		const Position& trade_pos = trade_partner->getPosition();
		const Position& item_pos = trading_item->getPosition();

		if (pos.z != trade_pos.z || !Position::isAccessible(pos, trade_pos, 2)) {
			g_game.closeTrade(this);
		} else {
			if (pos.z != item_pos.z || !Position::isAccessible(pos, item_pos, 1)) {
				g_game.closeTrade(this);
			}
		}
	}
}

uint64_t Player::getExperienceForLevel(uint16_t level)
{
	level--;
	return ((50ULL * level * level * level) - (150ULL * level * level) + (400ULL * level)) / 3ULL;
}

uint64_t Player::getExperienceForSkill(uint16_t level)
{
	return static_cast<uint64_t>(50 * std::pow(static_cast<double>(1.1), level - 11));
}

uint64_t Player::getManaForMagicLevel(uint16_t level)
{
	uint64_t req_mana = static_cast<uint64_t>(400 * std::pow<double>(1.1, static_cast<int32_t>(level) - 1));
	const uint32_t mod_result = req_mana % 20;
	if (mod_result < 10) {
		req_mana -= mod_result;
	} else {
		req_mana -= mod_result + 20;
	}

	return req_mana;
}

void Player::onCreate()
{
	Creature::onCreate();

	if (is_dead) {
		is_dead = false;

		skill_hitpoints->setValue(skill_hitpoints->getMax());
		skill_manapoints->setValue(skill_manapoints->getMax());

		earliest_logout_round = 0;
		earliest_protection_zone_round = 0;
	}

	last_ping = g_game.serverMilliseconds();
	last_pong = g_game.serverMilliseconds();
	timestamp = g_game.getRoundNr();
	timestamp_action = g_game.getRoundNr();

	g_game.addPlayerList(this);
}

void Player::onDelete()
{
	Creature::onDelete();

	if (party != nullptr) {
		party->leaveParty(this);
	}

	if (trade_state != TRADE_NONE) {
		g_game.closeTrade(this);
	}

	Channel* channel = g_channels.getPrivateChannel(this);
	if (channel) {
		for (Player* subscriber : channel->getSubscribers()) {
			Protocol::sendCloseChannel(subscriber->connection_ptr, channel);
		}
	}

	g_channels.leaveChannels(this);

	if (connection_ptr) {
		if (!is_dead) {
			connection_ptr->close();
		}

		connection_ptr = nullptr;
	}

	for (int32_t i = INVENTORY_HEAD; i <= INVENTORY_EXTRA; i++) {
		Item* item = inventory[i];
		if (item && item->getFlag(EXPIRE)) {
			g_game.stopDecay(item);
		}
	}

	g_game.removePlayerList(this);
}

void Player::onDeath()
{
	Creature::onDeath();

	Item* item = g_itempool.createItem(4240);
	if (g_game.addItem(item, getParent()) != ALLGOOD) {
		delete item;
		return;
	}

	std::string dead_description = fmt::sprintf("You recognize %s", getName());
	dead_description += (sex_type == SEX_MALE ? ". He" : ". She");
	if (!murderer.empty()) {
		dead_description += fmt::sprintf(" was killed by %s.", murderer);
	}
	item->setText(dead_description);

	Protocol::sendTextMessage(connection_ptr, 19, "You are dead.\n");

	bool lose_inventory = true;

	Item* amulet = inventory[INVENTORY_AMULET];
	if (amulet && amulet->getId() == 3057) {
		lose_inventory = false;
		g_game.removeItem(amulet, 1);
	}

	if (lose_inventory) {
		for (Item* corpse_item : inventory) {
			if (corpse_item == nullptr) {
				continue;
			}

			if (g_game.moveItem(corpse_item, corpse_item->getAttribute(ITEM_AMOUNT), this, item, INDEX_ANYWHERE, nullptr, FLAG_NOLIMIT) != ALLGOOD) {
				fmt::printf("INFO - Player::onDeath: %s - failed to move item to corpse (%s)\n", getName(), corpse_item->getName(-1));
			}
		}
	}

	// skills loss part
	double loss_percent = 10;
	if (vocation->getId() > 4) {
		loss_percent -= 3;
	}

	loss_percent /= 100.;

	// level loss
	removeExperience(SKILL_LEVEL, static_cast<int64_t>(skill_experience[SKILL_LEVEL] * loss_percent));

	// skill loss
	for (int32_t skill = SKILL_FIRST; skill != SKILL_LAST; skill++) {
		int64_t total_experience = 0;
		int64_t start = 1;
		if (skill != SKILL_MAGIC) {
			start = 11;
		}

		for (int64_t i = start; i <= skill_level[skill]; i++) {
			total_experience += vocation->getExperienceForSkill(static_cast<SkillType_t>(skill), i);
		}

		total_experience += skill_level[skill];
		removeExperience(static_cast<SkillType_t>(skill), total_experience * loss_percent);
	}

	// reset data
	current_pos = start_pos;

	clearKillingMarks();
}

void Player::onLoseTarget()
{
	Creature::onLoseTarget();

	Protocol::sendResult(connection_ptr, TARGETLOST);
	Protocol::sendClearTarget(connection_ptr);
}

void Player::onWalk(const Position& from_pos, int32_t from_index, const Position& to_pos, int32_t to_index)
{
	Creature::onWalk(from_pos, from_index, to_pos, to_index);

	current_pos = to_pos;

	// close containers that are out of reach
	std::vector<uint32_t> closed_containers;
	for (const auto& it : open_containers) {
		Item* item = it.second;
		const Position& pos = item->getPosition();

		Player* holding_player = item->getHoldingPlayer();

		if ((holding_player && holding_player != this) || !Position::isAccessible(pos, to_pos, 1)) {
			closed_containers.push_back(it.first);
		}
	}

	for (uint32_t container_id : closed_containers) {
		closeContainer(container_id);
		Protocol::sendCloseContainer(connection_ptr, container_id);
	}

	notifyTrades();

	if (!connection_ptr) {
		return;
	}

	const int32_t dx = std::abs(from_pos.x - to_pos.x);
	const int32_t dy = std::abs(from_pos.y - to_pos.y);
	const int32_t dz = std::abs(from_pos.z - to_pos.z);

	if ((dx + 1) > 2 || (dy + 1) > 2 || (dz + 1) > 1) {
		NetworkMessage msg;
		Protocol::addMapDescription(connection_ptr, msg, to_pos);
		connection_ptr->send(msg);
		return;
	}

	NetworkMessage msg;

	// move self player
	msg.writeByte(0x6D);
	Protocol::addPosition(msg, from_pos);
	msg.writeByte(from_index);
	Protocol::addPosition(msg, to_pos);

	// player is going down a floor
	if (to_pos.z > from_pos.z) {
		msg.writeByte(0xBF);

		if (to_pos.z == 8) {
			// from ground floor to underground
			int32_t skip = -1;

			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, to_pos.z, 18, 14, -1, skip);
			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, to_pos.z + 1, 18, 14, -1, skip);
			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, to_pos.z + 2, 18, 14, -1, skip);

			if (skip >= 0) {
				msg.writeByte(skip);
				msg.writeByte(0xFF);
			}
		} else if (to_pos.z > from_pos.z && to_pos.z > 8 && to_pos.z < 14) {
			// going further down
			int32_t skip = -1;
			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, to_pos.z + 2, 18, 14, -3, skip);

			if (skip >= 0) {
				msg.writeByte(skip);
				msg.writeByte(0xFF);
			}
		}

		// send east floors
		msg.writeByte(0x66);
		Protocol::addMapFloors(connection_ptr, msg, from_pos.x + 9, from_pos.y - 7, to_pos.z, 1, 14);

		// send south floors
		msg.writeByte(0x67);
		Protocol::addMapFloors(connection_ptr, msg, from_pos.x - 8, from_pos.y + 7, to_pos.z, 18, 1);
	}
	// player is going up a floor
	else if (to_pos.z < from_pos.z) {
		msg.writeByte(0xBE);

		if (to_pos.z == 7) {
			int32_t skip = -1;
			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, 5, 18, 14, 3, skip);
			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, 4, 18, 14, 4, skip);
			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, 3, 18, 14, 5, skip);
			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, 2, 18, 14, 6, skip);
			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, 1, 18, 14, 7, skip);
			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, 0, 18, 14, 8, skip);

			if (skip >= 0) {
				msg.writeByte(skip);
				msg.writeByte(0xFF);
			}
		} else if (to_pos.z > 7) {
			int32_t skip = -1;
			Protocol::addMapRow(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, to_pos.z - 3, 18, 14, 3, skip);

			if (skip >= 0) {
				msg.writeByte(skip);
				msg.writeByte(0xFF);
			}
		}

		// send west floors
		msg.writeByte(0x68);
		Protocol::addMapFloors(connection_ptr, msg, from_pos.x - 8, from_pos.y - 5, to_pos.z, 1, 14);

		// send north floors
		msg.writeByte(0x65);
		Protocol::addMapFloors(connection_ptr, msg, from_pos.x - 8, from_pos.y - 6, to_pos.z, 18, 1);
	}

	if (from_pos.y > to_pos.y) {
		// walk north
		msg.writeByte(0x65);
		Protocol::addMapFloors(connection_ptr, msg, from_pos.x - 8, to_pos.y - 6, to_pos.z, 18, 1);
	} else if (from_pos.y < to_pos.y) {
		// walk south
		msg.writeByte(0x67);
		Protocol::addMapFloors(connection_ptr, msg, from_pos.x - 8, to_pos.y + 7, to_pos.z, 18, 1);
	}

	if (from_pos.x < to_pos.x) {
		// walk east
		msg.writeByte(0x66);
		Protocol::addMapFloors(connection_ptr, msg, to_pos.x + 9, to_pos.y - 6, to_pos.z, 1, 14);
	} else if (from_pos.x > to_pos.x) {
		// walk west
		msg.writeByte(0x68);
		Protocol::addMapFloors(connection_ptr, msg, to_pos.x - 8, to_pos.y - 6, to_pos.z, 1, 14);
	}

	connection_ptr->send(msg);
}

void Player::onKilledCreature(Creature* target)
{
	Creature::onKilledCreature(target);

	if (Player* player = target->getPlayer()) {
		blockLogout(900, true);
		recordMurder(player);
	}
}

void Player::onAttackCreature(Creature* target)
{
	Creature::onAttackCreature(target);

	bool block_pz = false;
	if (Player* victim_player = target->getPlayer()) {
		block_pz = true;
		recordAttack(victim_player);
	}

	blockLogout(60, block_pz);
}

void Player::onAttacked(Creature* attacker)
{
	Creature::onAttacked(attacker);

	blockLogout(60, false);
}

int32_t Player::onDamaged(Creature* attacker, int32_t value, DamageType_t damage_type)
{
	value = Creature::onDamaged(attacker, value, damage_type);

	attacker->onAttackCreature(this);

	Protocol::sendMarkCreature(connection_ptr, attacker->getId(), 0);

	if (Player* attacker_player = attacker->getPlayer()) {
		if (damage_type != DAMAGE_POISONING && damage_type != DAMAGE_BURNING && damage_type != DAMAGE_ELECTRIFY) {
			value = (value + 1) / 2;
		}
	}

	// damage reduction items
	for (int32_t slot = INVENTORY_HEAD; slot <= INVENTORY_EXTRA; slot++) {
		Item* item = inventory[slot];
		if (item == nullptr || !item->getFlag(PROTECTION)) {
			continue;
		}

		if (item->getFlag(CLOTHES) && item->getAttribute(BODYPOSITION) == slot) {
			if (hasBitSet(item->getAttribute(PROTECTIONDAMAGETYPES), damage_type)) {
				const int32_t damage_reduction = item->getAttribute(DAMAGEREDUCTION);
				value = (100 - damage_reduction) * value / 100;

				if (item->getFlag(WEAROUT)) {
					if (item->getAttribute(ITEM_REMAINING_USES) <= 1) {
						g_game.removeItem(item, 1);
					} else {
						item->setAttribute(ITEM_REMAINING_USES, item->getAttribute(ITEM_REMAINING_USES) - 1);
					}
				}
			}
		}
	}

	return value;
}
