#include "pch.h"

#include "creature.h"
#include "game.h"
#include "protocol.h"
#include "tile.h"
#include "item.h"
#include "config.h"
#include "player.h"
#include "combat.h"
#include "tools.h"
#include "vocation.h"

void Skill::setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value)
{
	cycle = new_cycle;
	count = new_count;
	max_count = new_max_count;

	g_game.addCreatureSkillCheck(creature);
}

bool Skill::process()
{
	bool result;

	const int32_t r_cycle = cycle;
	if (r_cycle) {
		if (count <= 0) {
			count = max_count;
			cycle = r_cycle + 2 * (r_cycle <= 0) - 1;
			event(2 * (r_cycle <= 0) - 1);
		} else {
			--count;
		}
		result = true;
	} else {
		if (Player* player = creature->getPlayer()) {
			player->checkState();
		}

		result = false;
	}
	return result;
}

void SkillHitpoints::change(int16_t value)
{
	hitpoints += value;

	if (hitpoints <= 0) {
		hitpoints = 0;
	}

	if (hitpoints > max_hitpoints) {
		hitpoints = max_hitpoints;
	}

	g_game.announceChangedCreature(creature, CREATURE_HEALTH);

	if (Player* player = creature->getPlayer()) {
		Protocol::sendStats(player->getConnection());
	}
}

void SkillGoStrength::setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value)
{
	Skill::setTiming(new_cycle, new_count, new_max_count, additional_value);

	if (new_cycle == 0) {
		delta = 0;
	}

	if (Player* player = creature->getPlayer()) {
		player->checkState();
	}

	g_game.announceChangedCreature(creature, CREATURE_SPEED);
}

void SkillGoStrength::setBaseSpeed(int32_t value)
{
	speed = value;
	g_game.announceChangedCreature(creature, CREATURE_SPEED);
}

void SkillGoStrength::setDelta(int32_t value)
{
	delta = value;

	if (Player* player = creature->getPlayer()) {
		player->checkState();
	}

	g_game.announceChangedCreature(creature, CREATURE_SPEED);
}

void SkillGoStrength::event(int32_t value)
{
	Skill::event(value);

	if (!cycle) {
		delta = 0;

		if (Player* player = creature->getPlayer()) {
			player->checkState();
		}

		g_game.announceChangedCreature(creature, CREATURE_SPEED);
	}
}

void SkillLight::setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value)
{
	Skill::setTiming(new_cycle, new_count, new_max_count, additional_value);
	event(0);
}

void SkillLight::event(int32_t value)
{
	g_game.announceChangedCreature(creature, CREATURE_LIGHT);
}

void SkillIllusion::setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value)
{
	Skill::setTiming(new_cycle, new_count, new_max_count, additional_value);

	if (Player* player = creature->getPlayer()) {
		player->checkState();
	}

	if (!cycle) {
		creature->setCurrentOutfit(creature->getOriginalOutfit());
		g_game.announceChangedCreature(creature, CREATURE_OUTFIT);
	}

	g_game.announceChangedCreature(creature, CREATURE_OUTFIT);
}

void SkillIllusion::event(int32_t value)
{
	if (!cycle) {
		creature->setCurrentOutfit(creature->getOriginalOutfit());
		g_game.announceChangedCreature(creature, CREATURE_OUTFIT);
	}
}

void SkillBurning::event(int32_t value)
{
	creature->damage(g_game.getCreatureById(creature->fire_damage_origin), 10, DAMAGE_FIRE);
}

void SkillEnergy::event(int32_t value)
{
	creature->damage(g_game.getCreatureById(creature->energy_damage_origin), 25, DAMAGE_ENERGY);
}

bool SkillPoison::process()
{
	bool result;

	const int32_t r_cycle = cycle;
	if (r_cycle) {
		if (count <= 0) {
			count = max_count;
			int32_t f = factor_percent * r_cycle / 1000;
			if (!f) {
				f = 2 * (r_cycle > 0) - 1;
			}

			cycle = r_cycle - f;
			event(f);
		} else {
			--count;
		}
		result = true;
	} else {
		if (Player* player = creature->getPlayer()) {
			player->checkState();
		}

		result = false;
	}
	return result;
}

void SkillPoison::setTiming(int32_t new_cycle, int32_t new_count, int32_t new_max_count, int32_t additional_value)
{
	Skill::setTiming(new_cycle, new_count, new_max_count, additional_value);

	if (Player* player = creature->getPlayer()) {
		player->checkState();
	}

	if (additional_value == -1) {
		additional_value = 50;
	}

	if (additional_value <= 9) {
		additional_value = 10;
	}

	if (additional_value >= 1001) {
		additional_value = 1000;
	}

	factor_percent = additional_value;
}

void SkillPoison::event(int32_t value)
{
	Skill::event(value);

	value = std::abs(value);
	creature->damage(g_game.getCreatureById(creature->poison_damage_origin), value, DAMAGE_POISON);
}

Creature::Creature()
{
	setId();
}

Creature::~Creature()
{
	connection_ptr = nullptr;

	delete skill_hitpoints;
	delete skill_go_strength;
	delete skill_light;
	delete skill_poison;
	delete skill_burning;
	delete skill_energy;
	delete skill_drunken;
	delete skill_illusion;
}

void Creature::getLight(uint8_t& brightness, uint8_t& color) const
{
	brightness = skill_light->getTiming();

	int32_t red = 5 * brightness;
	int32_t green = 5 * brightness;
	int32_t blue = 5 * brightness;

	for (const Item* item : inventory) {
		if (item == nullptr) {
			continue;
		}

		const int32_t item_brightness = item->getAttribute(BRIGHTNESS);
		const int32_t item_color = item->getAttribute(LIGHTCOLOR);

		const int32_t item_red = item_brightness * (item_color / 36);
		const int32_t item_green = item_brightness * (item_color / 6 - 6 * (item_color / 36));
		const int32_t item_blue = item_brightness * (item_color % 6);

		if (brightness < item_brightness) {
			brightness = item_brightness;
		}

		if (red < item_red) {
			red = item_red;
		}

		if (green < item_green) {
			green = item_green;
		}

		if (blue < item_blue) {
			blue = item_blue;
		}
	}

	if (brightness) {
		color = blue / brightness + 6 * (green / brightness) + 36 * (red / brightness);
	}
	else {
		color = 0;
	}
}

bool Creature::canSeePosition(const Position& pos) const
{
	return canSeePosition(pos.x, pos.y, pos.z);
}

bool Creature::canSeePosition(int32_t x, int32_t y, int32_t z) const
{
	const Position& pos = getPosition();
	if (pos.z <= 7) {
		if (z > 7) {
			return false;
		}
	} else if (pos.z >= 8) {
		if (std::abs(pos.z - z) > 2) {
			return false;
		}
	}

	const int32_t ry = 6 + y - pos.y - (pos.z - z);
	const int32_t rx = 8 + x - pos.x - (pos.z - z);
	return rx >= 0 && rx < 18 && ry >= 0 && ry < 14;
}

bool Creature::canSeeCreature(const Creature* creature) const
{
	return canSeePosition(creature->getPosition());
}

int32_t Creature::getDefendDamage()
{
	if (g_game.serverMilliseconds() < earliest_defend_time) {
		return 0;
	}

	earliest_defend_time = last_defend_time + 2000;
	last_defend_time = g_game.serverMilliseconds();

	int32_t defense = getDefense() + 1;

	FightMode_t s_fight_mode = fight_mode;
	if ((following || !attacked_creature) && earliest_attack_time <= g_game.serverMilliseconds()) {
		s_fight_mode = FIGHT_DEFENSIVE;
	}

	if (s_fight_mode == FIGHT_ATTACK) {
		defense -= 4 * defense / 10;
	} else if (s_fight_mode == FIGHT_DEFENSIVE) {
		defense += 8 * defense / 10;
	}

	if (Player* player = getPlayer()) {
		if (player->getShield()) {
			defense = player->probeSkill(SKILL_SHIELDING, defense, 0);

			if (player->learning_points > 0) {
				player->learning_points--;

				Protocol::sendSkills(connection_ptr);
			}
		}
	}

	return defense;
}

void Creature::process()
{
	if (is_dead || removed) {
		return;
	}

	if (skill_processing) {
		uint8_t processed_skills = 0;
		if (skill_go_strength->getTiming()) {
			if (skill_go_strength->process()) {
				processed_skills++;
			}
		}

		if (skill_burning->getTiming()) {
			if (skill_burning->process()) {
				processed_skills++;
			}
		}

		if (skill_energy->getTiming()) {
			if (skill_energy->process()) {
				processed_skills++;
			}
		}

		if (skill_light->getTiming()) {
			if (skill_light->process()) {
				processed_skills++;
			}
		}

		if (skill_poison->getTiming()) {
			if (skill_poison->process()) {
				processed_skills++;
			}
		}

		if (skill_illusion->getTiming()) {
			if (skill_illusion->process()) {
				processed_skills++;
			}
		}

		if (Player* player = getPlayer()) {
			if (player->skill_fed->getTiming()) {
				if (player->skill_fed->process()) {
					processed_skills++;
				}
			}

			if (player->skill_magic_shield->getTiming()) {
				if (player->skill_magic_shield->process()) {
					processed_skills++;
				}
			}
		}

		if (processed_skills == 0) {
			skill_processing = false;
		}
	}
}

int32_t Creature::damage(Creature* attacker, int32_t value, DamageType_t damage_type)
{
	if (is_dead) {
		return 0;
	}

	onAttacked(attacker);
	value = onDamaged(attacker, value, damage_type);

	if (value <= 0) {
		g_game.announceGraphicalEffect(this, 3);
		return 0;
	}

	uint32_t attacker_id = 0;
	if (attacker) {
		attacker_id = attacker->getId();
	}

	switch (damage_type) {
		case DAMAGE_POISONING: {
			poison_damage_origin = attacker_id;
			skill_poison->setTiming(value, 3, 3, -1);
			return value;
		}
		case DAMAGE_BURNING: {
			fire_damage_origin = attacker_id;
			skill_burning->setTiming(value / 10, 8, 8, -1);
			return value;
		}
		case DAMAGE_ELECTRIFY: {
			energy_damage_origin = attacker_id;
			skill_energy->setTiming(value / 20, 10, 10, -1);
			return value;
		}
		default: break;
	}

	if (damage_type == DAMAGE_PHYSICAL) {
		value -= getArmor();	
	}

	if (damage_type == DAMAGE_PHYSICAL && value <= 0) {
		g_game.announceGraphicalEffect(this, 4);
		return 0;
	}

	if (current_outfit.look_id == 0 && current_outfit.type_id == 0) {
		// remove invisible
		skill_illusion->setTiming(0, 0, 0, -1);
		current_outfit = original_outfit;
		g_game.announceChangedCreature(this, CREATURE_OUTFIT);
	}

	if (attacker) {
		Combat::addDamageToCombatList(this, attacker, value);
	}

	if (damage_type != DAMAGE_MANADRAIN) {
		if (Player* player = getPlayer()) {
			if (player->skill_magic_shield->getTiming()) {
				if (player->getManaPoints() >= value) {
					player->changeManaPoints(-value);
					g_game.announceGraphicalEffect(this, 2);
					g_game.announceAnimatedText(getPosition(), 5, fmt::sprintf("%d", value));

					if (attacker) {
						Protocol::sendTextMessage(connection_ptr, 21, fmt::sprintf("You lose %d mana blocking due to an attack by %s.", value, attacker->getName()));
					} else {
						Protocol::sendTextMessage(connection_ptr, 21, fmt::sprintf("You lose %d mana.", value));
					}

					return value;
				}

				value -= player->getManaPoints();
				player->skill_manapoints->setValue(0);
			}
		}
	}

	if (creature_type == CREATURE_PLAYER) {
		if (damage_type != DAMAGE_POISONING && damage_type != DAMAGE_BURNING && damage_type != DAMAGE_ELECTRIFY) {
			if (attacker) {
				Protocol::sendTextMessage(connection_ptr, 21, fmt::sprintf("You lose %d hitpoints due to an attack by %s.", std::min<int32_t>(getHitpoints(), value), attacker->getName()));
			} else {
				Protocol::sendTextMessage(connection_ptr, 21, fmt::sprintf("You lose %d hitpoints.", std::min<int32_t>(getHitpoints(), value)));
			}
		}
	}

	if (damage_type == DAMAGE_PHYSICAL) {
		if (race_type == RACE_BLOOD) {
			g_game.announceGraphicalEffect(this, 1);
			g_game.announceAnimatedText(getPosition(), 180, fmt::sprintf("%d", std::min<int32_t>(getHitpoints(), value)));
			g_game.createLiquidPool(getPosition(), g_items.getSpecialItem(SPECIAL_BLOOD_SPLASH), 5);
		} else if (race_type == RACE_UNDEAD) {
			g_game.announceGraphicalEffect(this, 17);
			g_game.announceAnimatedText(getPosition(), 30, fmt::sprintf("%d", std::min<int32_t>(getHitpoints(), value)));
			g_game.createLiquidPool(getPosition(), g_items.getSpecialItem(SPECIAL_BLOOD_SPLASH), 6);
		} else if (race_type == RACE_POISON) {
			g_game.announceGraphicalEffect(this, 10);
			g_game.announceAnimatedText(getPosition(), 129, fmt::sprintf("%d", std::min<int32_t>(getHitpoints(), value)));
			g_game.createLiquidPool(getPosition(), g_items.getSpecialItem(SPECIAL_BLOOD_SPLASH), 6);
		}
	} else if (damage_type == DAMAGE_POISON) {
		g_game.announceGraphicalEffect(this, 9);
		g_game.announceAnimatedText(getPosition(), 30, fmt::sprintf("%d", std::min<int32_t>(getHitpoints(), value)));
	} else if (damage_type == DAMAGE_FIRE) {
		g_game.announceGraphicalEffect(this, 16);
		g_game.announceAnimatedText(getPosition(), 198, fmt::sprintf("%d", std::min<int32_t>(getHitpoints(), value)));
	} else if (damage_type == DAMAGE_ENERGY) {
		g_game.announceGraphicalEffect(this, 12);
		g_game.announceAnimatedText(getPosition(), 35, fmt::sprintf("%d", std::min<int32_t>(getHitpoints(), value)));
	} else if (damage_type == DAMAGE_LIFEDRAIN) {
		g_game.announceGraphicalEffect(this, 14);
		g_game.announceAnimatedText(getPosition(), 180, fmt::sprintf("%d", std::min<int32_t>(getHitpoints(), value)));
	}

	skill_hitpoints->change(-value);
	if (getHitpoints() <= 0) {
		is_dead = true;

		if (attacker != this) {
			if (attacker) {
				murderer = attacker->getName();
			}

			attacker->onKilledCreature(this);
		}
	}

	return value;
}

int64_t Creature::calculateDelay()
{
	ToDoEntry& top_entry = todo_list[current_todo];
	const uint64_t time_now = g_game.serverMilliseconds();
	if (top_entry.code == TODO_GO) {
		if (time_now < earliest_walk_time) {
			return earliest_walk_time - time_now;
		}
	} else if (top_entry.code == TODO_WAIT) {
		if (time_now >= earliest_walk_time && time_now >= top_entry.interval) {
			return 0;
		}

		uint64_t check_interval = top_entry.interval;
		if (earliest_walk_time >= check_interval) {
			check_interval = earliest_walk_time;
		}

		return check_interval - time_now;
	} else if (top_entry.code == TODO_ATTACK) {
		if (time_now < earliest_attack_time) {
			return earliest_attack_time - time_now;
		}
	}

	return 0;
}

bool Creature::toDoClear()
{
	bool snapback_needed = false;
	for (uint8_t index = 0; index < total_todo; index++) {
		const ToDoEntry& current_entry = todo_list[index];
		if (current_entry.code == TODO_GO) {
			snapback_needed = true;
		}
	}

	lock_todo = false;
	current_todo = 0;
	total_todo = 0;
	stop = false;
	return snapback_needed;
}

void Creature::toDoYield()
{
	if (lock_todo) {
		return;
	}

	ToDoEntry entry;
	entry.code = TODO_WAIT;
	entry.interval = g_game.serverMilliseconds();
	todo_list[total_todo++] = entry;
}

void Creature::toDoAttack()
{
	ToDoEntry entry;
	entry.code = TODO_ATTACK;
	todo_list[total_todo++] = entry;
}

void Creature::toDoWait(uint16_t interval)
{
	if (lock_todo && toDoClear() && creature_type == CREATURE_PLAYER) {
		Protocol::sendSnapback(connection_ptr);
	}

	ToDoEntry entry;
	entry.code = TODO_WAIT;
	entry.interval = g_game.serverMilliseconds() + interval;
	todo_list[total_todo++] = entry;
}

void Creature::toDoTrade(const Position& pos, uint16_t type_id, uint8_t index, uint32_t player_id)
{
	if (lock_todo && toDoClear() && creature_type == CREATURE_PLAYER) {
		Protocol::sendSnapback(connection_ptr);
	}

	ToDoEntry entry;
	entry.code = TODO_TRADE;
	entry.from_pos = pos;
	entry.type_id = type_id;
	entry.from_index = index;
	entry.player_id = player_id;
	todo_list[total_todo++] = entry;
}

void Creature::toDoRotate(Direction_t new_look_direction)
{
	if (lock_todo && toDoClear()) {
		Protocol::sendSnapback(connection_ptr);
	}

	ToDoEntry entry;
	entry.code = TODO_ROTATE;
	entry.look_direction = new_look_direction;
	todo_list[total_todo++] = entry;
}

void Creature::toDoMoveItem(Item* item, uint16_t amount, const Position& to_pos)
{
	ToDoEntry entry;
	entry.code = TODO_MOVE_OBJECT;
	entry.item = item;
	entry.amount = amount;
	entry.to_pos = to_pos;
	todo_list[total_todo++] = entry;
}

void Creature::toDoMoveObject(const Position& from_pos, uint16_t type_id, uint8_t from_index, const Position& to_pos, uint8_t amount)
{
	ToDoEntry entry;
	entry.code = TODO_MOVE_OBJECT;
	entry.from_index = from_index;
	entry.to_pos = to_pos;
	entry.from_pos = from_pos;
	entry.type_id = type_id;
	entry.amount = amount;
	todo_list[total_todo++] = entry;
}

void Creature::toDoTurnObject(const Position& pos, uint16_t type_id, uint8_t index)
{
	ToDoEntry entry;
	entry.code = TODO_TURNOBJECT;
	entry.from_pos = pos;
	entry.type_id = type_id;
	entry.from_index = index;
	todo_list[total_todo++] = entry;
}

void Creature::toDoUse(const Position& pos, uint16_t type_id, uint8_t index, uint8_t container_id)
{
	ToDoEntry entry;
	entry.code = TODO_USE_OBJECT;
	entry.from_pos = pos;
	entry.type_id = type_id;
	entry.from_index = index;
	entry.container_id = container_id;
	todo_list[total_todo++] = entry;
}

void Creature::toDoUseTwoObjects(const Position& pos, uint16_t type_id, uint8_t index, uint32_t creature_id)
{
	ToDoEntry entry;
	entry.code = TODO_USE_TWO_OBJECTS;
	entry.from_pos = pos;
	entry.type_id = type_id;
	entry.from_index = index;
	entry.creature_id = creature_id;
	todo_list[total_todo++] = entry;
}

void Creature::toDoUseTwoObjects(Item* item, const Position& to_pos, uint16_t to_type_id, uint8_t to_index)
{
	ToDoEntry entry;
	entry.code = TODO_USE_TWO_OBJECTS;
	entry.item = item;
	entry.to_pos = to_pos;
	entry.to_type_id = to_type_id;
	entry.to_index = to_index;
	todo_list[total_todo++] = entry;
}

void Creature::toDoUseTwoObjects(const Position& pos, uint16_t type_id, uint8_t index, const Position& to_pos, uint16_t to_type_id, uint8_t to_index)
{
	ToDoEntry entry;
	entry.code = TODO_USE_TWO_OBJECTS;
	entry.from_pos = pos;
	entry.type_id = type_id;
	entry.from_index = index;
	entry.to_pos = to_pos;
	entry.to_type_id = to_type_id;
	entry.to_index = to_index;
	todo_list[total_todo++] = entry;
}

void Creature::toDoTalk(const std::string& text, const std::string& address, uint16_t channel_id, TalkType_t type, bool check_spamming)
{
	if (lock_todo && toDoClear() && creature_type == CREATURE_PLAYER) {
		Protocol::sendSnapback(connection_ptr);
	}

	ToDoEntry entry;
	entry.code = TODO_TALK;
	entry.text = text;
	entry.address = address;
	entry.channel_id = channel_id;
	entry.check_spamming = check_spamming;
	entry.type = type;
	todo_list[total_todo++] = entry;
}

void Creature::toDoAdd(ToDoEntry& new_entry)
{
	if (lock_todo && toDoClear() && creature_type == CREATURE_PLAYER) {
		Protocol::sendSnapback(connection_ptr);
	}

	todo_list[total_todo++] = new_entry;
}

void Creature::toDoStop()
{
	if (lock_todo) {
		stop = true;
	} else if (creature_type == CREATURE_PLAYER) {
		Protocol::sendSnapback(connection_ptr);
	}
}

void Creature::toDoStart()
{
	if (total_todo == 0) {
		return;
	}

	lock_todo = true;
	current_todo = 0;
	const int64_t delay = calculateDelay();
	next_wakeup = delay + g_game.serverMilliseconds();
	g_game.queueCreature(this);
}

void Creature::execute()
{
	while (lock_todo && !is_dead && !removed && next_wakeup <= g_game.serverMilliseconds()) {
		if (current_todo >= total_todo) {
			toDoClear();
			onIdleStimulus();
			return;
		}

		const int64_t delay = calculateDelay();
		if (delay > 0) {
			if (stop) {
				toDoClear();
				if (creature_type == CREATURE_PLAYER) {
					Protocol::sendSnapback(connection_ptr);
				}
			} else {
				next_wakeup = delay + g_game.serverMilliseconds();
				g_game.queueCreature(this);
			}

			return;
		}

		const ToDoEntry current_entry = todo_list[current_todo];
		current_todo++;

		switch (current_entry.code) {
			case TODO_ROTATE: {
				look_direction = current_entry.look_direction;
				g_game.announceChangedObject(this, ANNOUNCE_CHANGE);
				break;
			}
			case TODO_GO: {
				ReturnValue_t ret = g_game.moveCreature(this, current_entry.to_pos.x, current_entry.to_pos.y, current_entry.to_pos.z, 0);
				if (ret != ALLGOOD) {
					toDoClear();
				}
				break;
			}
			case TODO_MOVE_OBJECT: {
				if (current_entry.item == nullptr) {
					g_game.playerMoveObject(getPlayer(), current_entry.from_pos, current_entry.from_index, current_entry.to_pos, current_entry.type_id, current_entry.amount);
				} else {
					g_game.playerMoveItem(getPlayer(), current_entry.item, current_entry.amount, current_entry.item->getId(), current_entry.item->getParent(), g_game.getCylinder(getPlayer(), current_entry.to_pos), current_entry.to_pos);
				}
				break;
			}
			case TODO_USE_TWO_OBJECTS: {
				if (current_entry.item == nullptr && current_entry.creature_id == 0) {
					g_game.playerUseTwoObjects(getPlayer(), current_entry.from_pos, current_entry.type_id, current_entry.from_index, current_entry.to_pos, current_entry.to_type_id, current_entry.to_index);
				} else if (current_entry.creature_id != 0) {
					g_game.playerUseOnCreature(getPlayer(), current_entry.from_pos, current_entry.type_id, current_entry.from_index, current_entry.creature_id);
				} else {
					g_game.playerUseTwoObjects(getPlayer(), current_entry.item, current_entry.to_pos, current_entry.to_type_id, current_entry.to_index);
				}
				break;
			}
			case TODO_USE_OBJECT:
				g_game.playerUseObject(getPlayer(), current_entry.from_pos, current_entry.type_id, current_entry.from_index, current_entry.container_id);
				break;
			case TODO_TALK:
				if (getPlayer()) {
					g_game.playerTalk(getPlayer(), current_entry.text, current_entry.address, current_entry.channel_id, current_entry.type, current_entry.check_spamming);
				}
				break;
			case TODO_TRADE:
				g_game.playerTradeObject(getPlayer(), current_entry.from_pos, current_entry.type_id, current_entry.from_index, current_entry.player_id);
				break;
			case TODO_TURNOBJECT:
				g_game.playerTurnObject(getPlayer(), current_entry.from_pos, current_entry.type_id, current_entry.from_index);
				break;
			case TODO_ATTACK:
				Combat::attack(this, attacked_creature);
				break;
			default: break;
		}

		if (stop) {
			toDoClear();
			Protocol::sendSnapback(connection_ptr);
			return;
		}
	}
}

void Creature::setId()
{
	static uint32_t next_creature_id = 0x40000000;
	id = next_creature_id++;
}

void Creature::onCreate() 
{
	removed = false;

	look_direction = DIRECTION_SOUTH;

	g_game.addCreatureList(this);
}

void Creature::onDelete()
{
	removed = true;

	toDoClear();

	Combat::setAttackDest(this, nullptr, false);

	g_game.removeCreatureList(this);
}

void Creature::onDeath()
{
	is_dead = true;

	switch (race_type) {
		case RACE_BLOOD: {
			g_game.createLiquidPool(getPosition(), g_items.getSpecialItem(SPECIAL_BLOOD_POOL), 5);
			break;
		}
		case RACE_POISON: {
			g_game.createLiquidPool(getPosition(), g_items.getSpecialItem(SPECIAL_BLOOD_POOL), 6);
			break;
		}
		default: break;
	}
}

void Creature::onLoseTarget()
{
	toDoClear();
	Combat::setAttackDest(this, nullptr, false);
}

void Creature::onWalk(const Position& from_pos, int32_t from_index, const Position& to_pos, int32_t to_index)
{
	if (Position::getOffsetX(from_pos, to_pos) > 1 || Position::getOffsetY(from_pos, to_pos) > 1 || Position::getOffsetZ(from_pos, to_pos)) {
		toDoClear();
	}

	const bool diagonal_move = (from_pos.z == to_pos.z && to_pos.x != from_pos.x && to_pos.y != from_pos.y);

	Tile* tile = parent->getTile();
	Item* bank_item = tile->getBankItem();

	if (from_pos.y > to_pos.y) {
		look_direction = DIRECTION_NORTH;
	} else if (from_pos.y < to_pos.y) {
		look_direction = DIRECTION_SOUTH;
	}

	if (from_pos.x < to_pos.x) {
		look_direction = DIRECTION_EAST;
	} else if (from_pos.x > to_pos.x) {
		look_direction = DIRECTION_WEST;
	}

	int32_t waypoints_cost = 0;

	if (bank_item) {
		waypoints_cost = bank_item->getAttribute(WAYPOINTS);
	}

	if (diagonal_move) {
		waypoints_cost *= 3;
	}

	earliest_walk_time = g_game.serverMilliseconds() + g_config.Beat * ((g_config.Beat + 1000 * waypoints_cost / getSpeed() - 1) / g_config.Beat);
}

void Creature::onCreatureMove(Creature* creature)
{
	if (attacked_creature == creature) {
		if (!Position::isAccessible(getPosition(), attacked_creature->getPosition(), 8)) {
			onLoseTarget();
		}
	}
}

void Creature::onIdleStimulus()
{
	if (attacked_creature) {
		toDoAttack();
		toDoStart();
	}
}
