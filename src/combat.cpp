#include "pch.h"

#include "combat.h"
#include "tools.h"
#include "creature.h"
#include "map.h"
#include "protocol.h"
#include "game.h"
#include "player.h"
#include "vocation.h"
#include "magic.h"

#include <random>

void AreaCombat::clear()
{
	for (const auto& it : areas) {
		delete it.second;
	}
	areas.clear();
}

void DamageImpact::handleCreature(Creature* victim)
{
	if (victim == actor) {
		return;
	}

	int32_t victim_damage = power;
	if (damage_type == DAMAGE_PHYSICAL && allow_defense) {
		victim_damage -= victim->getDefendDamage();
	}
	
	victim->damage(actor, victim_damage, damage_type);
}

void FieldImpact::handleField(Tile* tile)
{
	g_game.createField(tile->getPosition(), field_type, actor->getId(), false);
}

void HealingImpact::handleCreature(Creature* victim)
{
	if (victim->isDead() || victim->isRemoved() || victim->getHitpoints() == 0) {
		return;
	}

	g_magic.heal(victim, power);
}

void CancelInvisibleImpact::handleCreature(Creature* victim)
{
	victim->skill_illusion->setTiming(0, 0, 0, -1);
}

AreaCombat::AreaCombat(const AreaCombat& rhs)
{
	has_ext_area = rhs.has_ext_area;
	for (const auto& it : rhs.areas) {
		areas[it.first] = new MatrixArea(*it.second);
	}
}

void AreaCombat::getList(const Position& center_pos, const Position& target_pos, std::forward_list<Tile*>& list) const
{
	const MatrixArea* area = getArea(center_pos, target_pos);
	if (!area) {
		return;
	}

	uint32_t center_y, center_x;
	area->getCenter(center_y, center_x);

	Position tmp_pos(target_pos.x - center_x, target_pos.y - center_y, target_pos.z);
	for (uint32_t y = 0, rows = area->getRows(); y < rows; ++y) {
		for (uint32_t x = 0; x < area->getCols(); ++x) {
			if (area->getValue(y, x) != 0) {
				if (g_map.throwPossible(target_pos, tmp_pos)) {
					Tile* tile = g_map.getTile(tmp_pos);
					if (tile) {
						list.push_front(tile);
					}
				}
			}
			tmp_pos.x++;
		}
		tmp_pos.x -= area->getCols();
		tmp_pos.y++;
	}
}

void AreaCombat::copyArea(const MatrixArea* input, MatrixArea* output, MatrixOperation_t op) const
{
	uint32_t center_y, center_x;
	input->getCenter(center_y, center_x);

	if (op == MATRIXOPERATION_COPY) {
		for (uint32_t y = 0; y < input->getRows(); ++y) {
			for (uint32_t x = 0; x < input->getCols(); ++x) {
				(*output)[y][x] = (*input)[y][x];
			}
		}

		output->setCenter(center_y, center_x);
	}
	else if (op == MATRIXOPERATION_MIRROR) {
		for (uint32_t y = 0; y < input->getRows(); ++y) {
			uint32_t rx = 0;
			for (int32_t x = input->getCols(); --x >= 0;) {
				(*output)[y][rx++] = (*input)[y][x];
			}
		}

		output->setCenter(center_y, (input->getRows() - 1) - center_x);
	}
	else if (op == MATRIXOPERATION_FLIP) {
		for (uint32_t x = 0; x < input->getCols(); ++x) {
			uint32_t ry = 0;
			for (int32_t y = input->getRows(); --y >= 0;) {
				(*output)[ry++][x] = (*input)[y][x];
			}
		}

		output->setCenter((input->getCols() - 1) - center_y, center_x);
	}
	else {
		// rotation
		const int32_t rotate_center_x = (output->getCols() / 2) - 1;
		const int32_t rotate_center_y = (output->getRows() / 2) - 1;
		int32_t angle;

		switch (op) {
		case MATRIXOPERATION_ROTATE90:
			angle = 90;
			break;

		case MATRIXOPERATION_ROTATE180:
			angle = 180;
			break;

		case MATRIXOPERATION_ROTATE270:
			angle = 270;
			break;

		default:
			angle = 0;
			break;
		}

		const double angle_rad = M_PI * angle / 180.0;

		const double a = std::cos(angle_rad);
		const double b = -std::sin(angle_rad);
		const double c = std::sin(angle_rad);
		const double d = std::cos(angle_rad);

		const uint32_t rows = input->getRows();
		for (uint32_t x = 0, cols = input->getCols(); x < cols; ++x) {
			for (uint32_t y = 0; y < rows; ++y) {
				//calculate new coordinates using rotation center
				const int32_t new_x = x - center_x;
				const int32_t new_y = y - center_y;

				//perform rotation
				const int32_t rotated_x = static_cast<int32_t>(round(new_x * a + new_y * b));
				const int32_t rotated_y = static_cast<int32_t>(round(new_x * c + new_y * d));

				//write in the output matrix using rotated coordinates
				(*output)[rotated_y + rotate_center_y][rotated_x + rotate_center_x] = (*input)[y][x];
			}
		}

		output->setCenter(rotate_center_y, rotate_center_x);
	}
}

MatrixArea* AreaCombat::createArea(const std::list<uint32_t>& list, uint32_t rows) const
{
	uint32_t cols;
	if (rows == 0) {
		cols = 0;
	}
	else {
		cols = list.size() / rows;
	}

	MatrixArea* area = new MatrixArea(rows, cols);

	uint32_t x = 0;
	uint32_t y = 0;

	for (uint32_t value : list) {
		if (value == 1 || value == 3) {
			area->setValue(y, x, true);
		}

		if (value == 2 || value == 3) {
			area->setCenter(y, x);
		}

		++x;

		if (cols == x) {
			x = 0;
			++y;
		}
	}
	return area;
}

void AreaCombat::setupArea(const std::list<uint32_t>& list, uint32_t rows)
{
	MatrixArea* area = createArea(list, rows);

	//NORTH
	areas[DIRECTION_NORTH] = area;

	const uint32_t max_output = std::max<uint32_t>(area->getCols(), area->getRows()) * 2;

	//SOUTH
	MatrixArea* south_area = new MatrixArea(max_output, max_output);
	copyArea(area, south_area, MATRIXOPERATION_ROTATE180);
	areas[DIRECTION_SOUTH] = south_area;

	//EAST
	MatrixArea* east_area = new MatrixArea(max_output, max_output);
	copyArea(area, east_area, MATRIXOPERATION_ROTATE90);
	areas[DIRECTION_EAST] = east_area;

	//WEST
	MatrixArea* west_area = new MatrixArea(max_output, max_output);
	copyArea(area, west_area, MATRIXOPERATION_ROTATE270);
	areas[DIRECTION_WEST] = west_area;
}

void AreaCombat::setupArea(int32_t length, int32_t spread)
{
	std::list<uint32_t> list;

	const uint32_t rows = length;
	int32_t cols = 1;

	if (spread != 0) {
		cols = ((length - (length % spread)) / spread) * 2 + 1;
	}

	int32_t col_spread = cols;

	for (uint32_t y = 1; y <= rows; ++y) {
		const int32_t mincol = cols - col_spread + 1;
		const int32_t maxcol = cols - (cols - col_spread);

		for (int32_t x = 1; x <= cols; ++x) {
			if (y == rows && x == ((cols - (cols % 2)) / 2) + 1) {
				list.push_back(3);
			} else if (x >= mincol && x <= maxcol) {
				list.push_back(1);
			} else {
				list.push_back(0);
			}
		}

		if (spread > 0 && y % spread == 0) {
			--col_spread;
		}
	}

	setupArea(list, rows);
}

void AreaCombat::setupArea(int32_t radius)
{
	int32_t area[13][13] = {
		{ 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 8, 8, 7, 8, 8, 0, 0, 0, 0 },
		{ 0, 0, 0, 8, 7, 6, 6, 6, 7, 8, 0, 0, 0 },
		{ 0, 0, 8, 7, 6, 5, 5, 5, 6, 7, 8, 0, 0 },
		{ 0, 8, 7, 6, 5, 4, 4, 4, 5, 6, 7, 8, 0 },
		{ 0, 8, 6, 5, 4, 3, 2, 3, 4, 5, 6, 8, 0 },
		{ 8, 7, 6, 5, 4, 2, 1, 2, 4, 5, 6, 7, 8 },
		{ 0, 8, 6, 5, 4, 3, 2, 3, 4, 5, 6, 8, 0 },
		{ 0, 8, 7, 6, 5, 4, 4, 4, 5, 6, 7, 8, 0 },
		{ 0, 0, 8, 7, 6, 5, 5, 5, 6, 7, 8, 0, 0 },
		{ 0, 0, 0, 8, 7, 6, 6, 6, 7, 8, 0, 0, 0 },
		{ 0, 0, 0, 0, 8, 8, 7, 8, 8, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0 }
		};

	std::list<uint32_t> list;

	for (auto& row : area) {
		for (int cell : row) {
			if (cell == 1) {
				list.push_back(3);
			} else if (cell > 0 && cell <= radius) {
				list.push_back(1);
			} else {
				list.push_back(0);
			}
		}
	}

	setupArea(list, 13);
}

void AreaCombat::setupExtArea(const std::list<uint32_t>& list, uint32_t rows)
{
	if (list.empty()) {
		return;
	}

	has_ext_area = true;
	MatrixArea* area = createArea(list, rows);

	//NORTH-WEST
	areas[DIRECTION_NORTHWEST] = area;

	const uint32_t max_output = std::max<uint32_t>(area->getCols(), area->getRows()) * 2;

	//NORTH-EAST
	MatrixArea* ne_area = new MatrixArea(max_output, max_output);
	copyArea(area, ne_area, MATRIXOPERATION_MIRROR);
	areas[DIRECTION_NORTHEAST] = ne_area;

	//SOUTH-WEST
	MatrixArea* sw_area = new MatrixArea(max_output, max_output);
	copyArea(area, sw_area, MATRIXOPERATION_FLIP);
	areas[DIRECTION_SOUTHWEST] = sw_area;

	//SOUTH-EAST
	MatrixArea* se_area = new MatrixArea(max_output, max_output);
	copyArea(sw_area, se_area, MATRIXOPERATION_MIRROR);
	areas[DIRECTION_SOUTHEAST] = se_area;
}

MatrixArea* AreaCombat::getArea(const Position& center_pos, const Position& target_pos) const
{
	const int32_t dx = Position::getOffsetX(target_pos, center_pos);
	const int32_t dy = Position::getOffsetY(target_pos, center_pos);

	Direction_t dir;
	if (dx < 0) {
		dir = DIRECTION_WEST;
	} else if (dx > 0) {
		dir = DIRECTION_EAST;
	} else if (dy < 0) {
		dir = DIRECTION_NORTH;
	} else {
		dir = DIRECTION_SOUTH;
	}

	if (has_ext_area) {
		if (dx < 0 && dy < 0) {
			dir = DIRECTION_NORTHWEST;
		} else if (dx > 0 && dy < 0) {
			dir = DIRECTION_NORTHEAST;
		} else if (dx < 0 && dy > 0) {
			dir = DIRECTION_SOUTHWEST;
		} else if (dx > 0 && dy > 0) {
			dir = DIRECTION_SOUTHEAST;
		}
	}

	const auto it = areas.find(dir);
	if (it == areas.end()) {
		return nullptr;
	}
	return it->second;
}

int32_t Combat::computeDamage(Creature* creature, int32_t damage, int32_t variation, bool apply_formulas)
{
	int32_t result = damage;
	if (variation) {
		result = result + random(-variation, variation);
	}
	
	if (Player* player = creature->getPlayer()) {
		if (apply_formulas) {
			const int32_t level_formula = 2 * player->skill_level[SKILL_LEVEL];
			const int32_t magic_formula = 3 * player->skill_level[SKILL_MAGIC];

			result = (level_formula + magic_formula) * result / 100;
		}
	}

	return result;
}

bool Combat::canDoAttack(Creature* creature, Creature* target)
{
	if (target == nullptr) {
		return false;
	}

	const Position& target_pos = target->getPosition();
	if (!Position::isAccessible(creature->getPosition(), target_pos, 8)) {
		return false;
	}

	bool get_near = creature->following || creature->chase_mode == CHASE_FOLLOW;
	if (get_near && !Position::isAccessible(creature->getPosition(), target_pos, 1)) {
		creature->toDoWait(100);

		Shortway shortway(creature);
		if (!shortway.calculate(target_pos.x, target_pos.y, false)) {
			setAttackDest(creature, nullptr, false);
			Protocol::sendClearTarget(creature->connection_ptr);
			Protocol::sendResult(creature->connection_ptr, THEREISNOWAY);
			return false;
		}

		creature->toDoStart();
	}

	return true;
}

void Combat::setAttackDest(Creature* creature, Creature* target, bool follow)
{
	if (creature->attacked_creature == target && creature->following == follow) {
		return;
	}

	if (target) {
		if (target->is_dead || target->removed || !Position::isAccessible(creature->getPosition(), target->getPosition(), 8)) {
			creature->onLoseTarget();
			return;
		}

		if (!follow) {
			if (g_map.isProtectionZone(creature->getPosition()) || g_map.isProtectionZone(target->getPosition())) {
				Protocol::sendResult(creature->connection_ptr, ACTIONNOTPERMITTEDINPROTECTIONZONE);
				Protocol::sendClearTarget(creature->connection_ptr);
				return;
			}

			if (Player* attacker_player = creature->getPlayer()) {
				if (Player* attacked_player = target->getPlayer()) {
					if (!attacker_player->isAttackJustified(attacked_player) && attacker_player->secure_mode) {
						Protocol::sendResult(creature->connection_ptr, TURNSECUREMODETOATTACKUNMARKEDPLAYERS);
						Protocol::sendClearTarget(creature->connection_ptr);
						return;
					}

					attacker_player->blockLogout(60, true);
					attacker_player->recordAttack(attacked_player);
					attacked_player->blockLogout(60, false);
				}
			}
		}
	}

	creature->attacked_creature = target;
	creature->following = follow;
}

void Combat::attack(Creature* creature, Creature* target)
{
	if (target == nullptr) {
		return;
	}

	if (target->is_dead || target->removed || !Position::isAccessible(creature->getPosition(), target->getPosition(), 8)) {
		creature->onLoseTarget();
		return;
	}

	Player* player = creature->getPlayer();
	Player* target_player = target->getPlayer();

	if (g_map.isProtectionZone(creature->getPosition()) || g_map.isProtectionZone(target->getPosition())) {
		Protocol::sendResult(creature->connection_ptr, ACTIONNOTPERMITTEDINPROTECTIONZONE);
		Protocol::sendClearTarget(creature->connection_ptr);
		setAttackDest(creature, nullptr, false);
		return;
	}

	if (player && target_player) {
		if (player->secure_mode && !player->isAttackJustified(target_player)) {
			player->onLoseTarget();
			Protocol::sendResult(player->connection_ptr, TURNSECUREMODETOATTACKUNMARKEDPLAYERS);
			return;
		}

		player->onAttackCreature(target_player);
		target_player->onAttacked(creature);
	} else if (player) {
		player->onAttackCreature(target_player);
	}

	const Position& target_pos = target->getPosition();

	if (!canDoAttack(creature, target)) {
		creature->onLoseTarget();
		return;
	}

	if (g_game.serverMilliseconds() < creature->earliest_attack_time) {
		return;
	}

	creature->earliest_attack_time = g_game.serverMilliseconds() + 200;

	if (!creature->following) {
		if (Player* player = creature->getPlayer()) {
			Item* weapon = player->getWeapon();
			if (weapon) {
				if (weapon->getFlag(RESTRICTPROFESSION)) {
					if (!hasBitSet(player->vocation->getId(), weapon->getAttribute(PROFESSIONS))) {
						meleeAttack(player, target);
						return;
					}
				}

				if (weapon->getFlag(RESTRICTLEVEL)) {
					if (player->skill_level[SKILL_LEVEL] < weapon->getAttribute(MINIMUMLEVEL)) {
						meleeAttack(player, target);
						return;
					}
				}

				if (weapon->getFlag(WAND)) {
					wandAttack(player, weapon, target);
				} else if (weapon->getFlag(BOW) || weapon->getFlag(THROWABLE)) {
					rangeAttack(player, weapon, target);
				} else {
					closeAttack(player, weapon, target);
				}
			} else {
				meleeAttack(player, target);
			}
		}
	}

	if (target->is_dead || target->isRemoved()) {
		creature->onLoseTarget();
	}
}

void Combat::addDamageToCombatList(Creature* creature, Creature* attacker, uint32_t damage)
{
	creature->combat_damage += damage;
	int32_t index = 0;
	while (index < creature->combat_list.size()) {
		CombatDamage& combat = creature->combat_list[index];
		if (combat.creature_id == attacker->id) {
			combat.damage += damage;
			combat.timestamp = g_game.getRoundNr();
			return;
		}

		index++;
	}

	CombatDamage& combat = creature->combat_list[creature->current_combat_entry];
	combat.creature_id = attacker->getId();
	combat.damage = damage;
	combat.timestamp = g_game.getRoundNr();

	uint8_t new_index = 0;
	if (creature->current_combat_entry != 19) {
		new_index = creature->current_combat_entry + 1;
	}

	creature->current_combat_entry = new_index;
}

bool Combat::massCombat(Creature* creature, Object* target, uint32_t mana, uint32_t soulpoints, uint32_t damage, uint32_t effect_nr, uint32_t radius, DamageType_t damage_type, uint32_t missile)
{
	if (creature == nullptr) {
		fmt::printf("ERROR - Combat::massCombat: creature is null.\n");
		return false;
	}

	if (target == nullptr) {
		fmt::printf("ERROR - Combat::massCombat: target is null.\n");
		return false;
	}

	if (!g_map.throwPossible(creature->getPosition(), target->getPosition())) {
		return false;
	}

	if (Player* player = creature->getPlayer()) {
		if (!g_magic.checkMana(player, mana, soulpoints, 2000)) {
			return false;
		}
	}

	DamageImpact impact;
	impact.actor = creature;
	impact.allow_defense = false;
	impact.damage_type = damage_type;
	impact.power = damage;
	circleShapeSpell(creature->getPosition(), target->getPosition(), radius, effect_nr, missile, impact);
	return true;
}

bool Combat::angleCombat(Creature* creature, uint32_t mana, uint32_t soulpoints, uint32_t damage, uint32_t effect_nr, uint32_t length, uint32_t spread, DamageType_t damage_type)
{
	if (!g_map.throwPossible(creature->getPosition(), Position::getNextPosition(creature->getPosition(), creature->getLookDirection()))) {
		Protocol::sendResult(creature->connection_ptr, NOTENOUGHROOM);
		return false;
	}

	if (Player* player = creature->getPlayer()) {
		if (!g_magic.checkMana(player, mana, soulpoints, (length != 1 || spread) ? 2000 : 1000)) {
			return false;
		}
	}

	DamageImpact impact;
	impact.actor = creature;
	impact.allow_defense = false;
	impact.damage_type = damage_type;
	impact.power = damage;
	angleShapeSpell(creature->getPosition(), length, spread, effect_nr, impact);
	return true;
}

void Combat::circleShapeSpell(const Position& from_pos, const Position& to_pos, uint8_t radius, uint8_t effect, uint8_t animation, ImpactDamage& impact)
{
	AreaCombat area_combat;
	area_combat.setupArea(radius);
	
	std::forward_list<Tile*> tiles;
	area_combat.getList(from_pos, to_pos, tiles);

	if (animation && from_pos != to_pos) {
		g_game.announceMissileEffect(from_pos, to_pos, animation);
	}

	for (Tile* tile : tiles) {
		if (tile->isProtectionZone() || !g_map.throwPossible(to_pos, tile->getPosition())) {
			continue;
		}

		if (effect) {
			g_game.announceGraphicalEffect(tile, effect);
		}

		impact.handleField(tile);

		for (Object* object : tile->getObjects()) {
			if (Creature* creature = object->getCreature()) {
				impact.handleCreature(creature);
			}
		}
	}
}

void Combat::angleShapeSpell(const Position& pos, uint16_t length, uint16_t spread,
	uint8_t effect, ImpactDamage& impact)
{
	AreaCombat area_combat;
	area_combat.setupArea(length, spread);

	Position to_pos = pos;
	switch (impact.actor->getLookDirection()) {
		case DIRECTION_NORTH: to_pos.y--; break;
		case DIRECTION_EAST: to_pos.x++; break;
		case DIRECTION_SOUTH: to_pos.y++; break;
		case DIRECTION_WEST: to_pos.x--; break;
		default: break;
	}

	std::forward_list<Tile*> tiles;
	area_combat.getList(pos, to_pos, tiles);

	for (Tile* tile : tiles) {
		if (tile->isProtectionZone() || !g_map.throwPossible(pos, tile->getPosition())) {
			continue;
		}

		if (effect) {
			g_game.announceGraphicalEffect(tile, effect);
		}

		impact.handleField(tile);

		for (Object* object : tile->getObjects()) {
			if (Creature* creature = object->getCreature()) {
				impact.handleCreature(creature);
			}
		}
	}
}

void Combat::angleWallSpell(const Position& pos, const Position& to_pos, uint8_t effect, uint8_t animation, ImpactDamage& impact)
{
	const std::list<uint32_t> normal_area = {
		1, 1, 3, 1, 1
	};

	const std::list<uint32_t> ex_area = {
		0, 0, 0, 0, 1,
		0, 0, 0, 1, 1,
		0, 1, 3, 1, 0,
		1, 1, 0, 0, 0,
		1, 0, 0, 0, 0,
	};

	AreaCombat area_combat;
	area_combat.setupArea(normal_area, 1);
	area_combat.setupExtArea(ex_area, 5);

	if (animation) {
		g_game.announceMissileEffect(pos, to_pos, animation);
	}

	std::forward_list<Tile*> tiles;
	area_combat.getList(pos, to_pos, tiles);

	for (Tile* tile : tiles) {
		if (tile->isProtectionZone() || !g_map.throwPossible(pos, tile->getPosition())) {
			continue;
		}

		if (effect) {
			g_game.announceGraphicalEffect(tile, effect);
		}

		impact.handleField(tile);

		for (Object* object : tile->getObjects()) {
			if (Creature* creature = object->getCreature()) {
				impact.handleCreature(creature);
			}
		}
	}
}

void Combat::customShapeSpell(const std::list<uint32_t>& list, uint32_t rows, const Position& pos, uint8_t effect, uint8_t animation, ImpactDamage& impact)
{
	AreaCombat area_combat;
	area_combat.setupArea(list, rows);

	Position to_pos = pos;
	switch (impact.actor->getLookDirection()) {
		case DIRECTION_NORTH: to_pos.y--; break;
		case DIRECTION_EAST: to_pos.x++; break;
		case DIRECTION_SOUTH: to_pos.y++; break;
		case DIRECTION_WEST: to_pos.x--; break;
		default: break;
	}

	if (animation) {
		g_game.announceMissileEffect(pos, to_pos, animation);
	}

	std::forward_list<Tile*> tiles;
	area_combat.getList(pos, to_pos, tiles);

	for (Tile* tile : tiles) {
		if (tile->isProtectionZone() || !g_map.throwPossible(pos, tile->getPosition())) {
			continue;
		}

		if (effect) {
			g_game.announceGraphicalEffect(tile, effect);
		}

		impact.handleField(tile);

		for (Object* object : tile->getObjects()) {
			if (Creature* creature = object->getCreature()) {
				impact.handleCreature(creature);
			}
		}
	}
}

void Combat::meleeAttack(Creature* creature, Creature* target)
{
	if (!Position::isAccessible(creature->getPosition(), target->getPosition(), 1)) {
		return;
	}

	int32_t total_damage = creature->getAttackDamage();

	if (creature->fight_mode == FIGHT_ATTACK) {
		total_damage += 2 * total_damage / 10;
	}
	else if (creature->fight_mode == FIGHT_DEFENSIVE) {
		total_damage -= 4 * total_damage / 10;
	}

	if (Player* player = creature->getPlayer()) {
		total_damage = player->probeSkill(SKILL_FISTFIGHTING, total_damage, 0);

		if (player->learning_points > 0) {
			Protocol::sendSkills(player->connection_ptr);
		}

		player->learning_points--;
	}

	total_damage -= target->getDefendDamage();
	if (total_damage < 0) {
		total_damage = 0;
	}

	total_damage = target->damage(creature, total_damage, DAMAGE_PHYSICAL);
	if (total_damage > 0) {
		if (Player* player = creature->getPlayer()) {
			player->learning_points = 30;
		}
	}

	creature->earliest_attack_time = g_game.serverMilliseconds() + 2000;
}

void Combat::closeAttack(Player* player, Item* weapon, Creature* target)
{
	if (weapon == nullptr) {
		fmt::printf("ERROR - Combat::closeAttack: item is null.\n");
		return;
	}

	if (!weapon->getFlag(WEAPON)) {
		fmt::printf("ERROR - Combat::closeAttack: item is not a WEAPON.\n");
		return;
	}

	if (!Position::isAccessible(player->getPosition(), target->getPosition(), 1)) {
		return;
	}

	int32_t total_damage = player->getAttackDamage();

	if (player->fight_mode == FIGHT_ATTACK) {
		total_damage += 2 * total_damage / 10;
	} else if (player->fight_mode == FIGHT_DEFENSIVE) {
		total_damage -= 4 * total_damage / 10;
	}

	const SkillType_t type = static_cast<SkillType_t>(weapon->getAttribute(WEAPONTYPE));
	total_damage = player->probeSkill(type, total_damage, 0);

	if (player->learning_points > 0) {
		player->learning_points--;

		Protocol::sendSkills(player->connection_ptr);
	}

	total_damage -= target->getDefendDamage();
	if (total_damage < 0) {
		total_damage = 0;
	}

	total_damage = target->damage(player, total_damage, DAMAGE_PHYSICAL);
	if (total_damage > 0) {
		player->learning_points = 30;
	}

	if (weapon->getFlag(WEAROUT)) {
		if (weapon->getAttribute(ITEM_REMAINING_USES) <= 1) {
			g_game.removeItem(weapon, 1);
		} else {
			weapon->setAttribute(ITEM_REMAINING_USES, weapon->getAttribute(ITEM_REMAINING_USES) - 1);
		}
	}

	player->earliest_attack_time = g_game.serverMilliseconds() + 2000;
}

void Combat::rangeAttack(Player* player, Item* weapon, Creature* target)
{
	if (!weapon->getFlag(BOW) && !weapon->getFlag(THROWABLE)) {
		fmt::printf("ERROR - Combat::rangeAttack: item is not a BOW or THROWABLE type.\n");
		return;
	}

	bool move_projectile = true;

	uint8_t missile = 0;
	uint8_t special_effect = 0;
	uint16_t effect_strength = 0;
	uint8_t hit_chance = 0;
	Position missile_position = target->getPosition();

	int32_t total_damage = player->getAttackDamage();

	if (player->fight_mode == FIGHT_ATTACK) {
		total_damage += 2 * total_damage / 10;
	}
	else if (player->fight_mode == FIGHT_DEFENSIVE) {
		total_damage -= 4 * total_damage / 10;
	}

	if (weapon->getFlag(THROWABLE)) {
		if (!Position::isAccessible(player->getPosition(), target->getPosition(), weapon->getAttribute(THROWRANGE)) || !g_map.throwPossible(player->getPosition(), target->getPosition())) {
			return;
		}

		hit_chance = 75;
		missile = weapon->getAttribute(THROWMISSILE);
		special_effect = weapon->getAttribute(THROWSPECIALEFFECT);
		effect_strength = weapon->getAttribute(THROWEFFECTSTRENGTH);

		if (random(0, weapon->getAttribute(THROWFRAGILITY)) == 0) {
			g_game.removeItem(weapon, 1);
			move_projectile = false;
		}
	} else if (weapon->getFlag(BOW)) {
		if (!Position::isAccessible(player->getPosition(), target->getPosition(), weapon->getAttribute(BOWRANGE)) || !g_map.throwPossible(player->getPosition(), target->getPosition())) {
			return;
		}

		Item* ammo = player->inventory[INVENTORY_EXTRA];
		if (ammo == nullptr || ammo->getAttribute(AMMOTYPE) != weapon->getAttribute(BOWAMMOTYPE)) {
			// redirect fist attack
			meleeAttack(player, target);
			return;
		}

		hit_chance = 90;
		missile = ammo->getAttribute(AMMOMISSILE);
		special_effect = ammo->getAttribute(AMMOSPECIALEFFECT);
		effect_strength = ammo->getAttribute(AMMOEFFECTSTRENGTH);

		g_game.removeItem(ammo, 1);
		move_projectile = false;
	}

	int32_t distance = Position::getOffsetX(player->getPosition(), target->getPosition()) + Position::getOffsetY(player->getPosition(), target->getPosition());
	if (distance <= 1) {
		distance = 5;
	}

	// calculate damage increase
	total_damage = player->probeSkill(SKILL_DISTANCEFIGHTING, total_damage, 0);

	// calculate hitting probability
	const bool hit = player->probeSkill(SKILL_DISTANCEFIGHTING, 15 * distance, hit_chance);

	if (player->learning_points > 0) {
		player->learning_points--;

		Protocol::sendSkills(player->connection_ptr);
	}

	if (special_effect == 1 && hit) {
		target->damage(player, effect_strength, DAMAGE_POISON);
	} else if (special_effect == 2) {
		DamageImpact impact;
		impact.actor = player;
		impact.allow_defense = false;
		impact.damage_type = DAMAGE_PHYSICAL;
		impact.power = computeDamage(player, effect_strength, effect_strength, true);
		circleShapeSpell(player->getPosition(), target->getPosition(), 3, 7, 0, impact);
	}

	if (!hit) {
		static std::vector<std::pair<int32_t, int32_t>> dest_list{
			{ -1, -1 },{ 0, -1 },{ 1, -1 },
			{ -1,  0 },{ 0,  0 },{ 1,  0 },
			{ -1,  1 },{ 0,  1 },{ 1,  1 }
			};

		std::shuffle(dest_list.begin(), dest_list.end(), std::default_random_engine(g_game.serverMilliseconds()));

		for (const auto& dir : dest_list) {
			Position dest_pos = target->getPosition();
			dest_pos.x += dir.first;
			dest_pos.y += dir.second;

			const Tile* tile = g_map.getTile(dest_pos);
			if (tile == nullptr) {
				continue;
			}

			if (tile->getFlag(BANK) && !tile->getFlag(UNLAY) && g_map.throwPossible(player->getPosition(), dest_pos)) {
				missile_position = dest_pos;
				break;
			}
		}

		g_game.announceGraphicalEffect(missile_position.x, missile_position.y, missile_position.z, 3);
	} else {
		// only hit creature when successful hit
		if (target->damage(player, total_damage, DAMAGE_PHYSICAL)) {
			player->learning_points = 30;
		}
	}

	if (move_projectile) {
		g_game.moveItem(weapon, 1, weapon->getParent(), g_map.getTile(missile_position), INDEX_ANYWHERE, nullptr, FLAG_NOLIMIT);
	}

	g_game.announceMissileEffect(player->getPosition(), missile_position, missile);
	player->earliest_attack_time = g_game.serverMilliseconds() + 2000;
}

void Combat::wandAttack(Player* player, Item* wand, Creature* target)
{
	if (!wand->getFlag(WAND)) {
		fmt::printf("ERROR - Combat::wandAttack: item is not WAND type.\n");
		return;
	}

	if (!Position::isAccessible(player->getPosition(), target->getPosition(), wand->getAttribute(WANDRANGE)) || !g_map.throwPossible(player->getPosition(), target->getPosition())) {
		return;
	}

	if (wand->getFlag(RESTRICTPROFESSION)) {
		if (!hasBitSet(player->vocation->getId(), wand->getAttribute(PROFESSIONS))) {
			meleeAttack(player, target);
			return;
		}
	}

	const int32_t mana_consumption = wand->getAttribute(WANDMANACONSUMPTION);
	if (mana_consumption) {
		if (player->getManaPoints() < mana_consumption) {
			return;
		}
	}

	// take mana from player
	player->changeManaPoints(-mana_consumption);
	player->addExperience(SKILL_MAGIC, mana_consumption);

	const DamageType_t damage_type = static_cast<DamageType_t>(wand->getAttribute(WANDDAMAGETYPE));
	const uint8_t missile = wand->getAttribute(WANDMISSILE);
	const int32_t wand_attack_strength = wand->getAttribute(WANDATTACKSTRENGTH);
	const int32_t wand_attack_variation = wand->getAttribute(WANDATTACKVARIATION);

	const int32_t total_damage = computeDamage(player, wand_attack_strength, wand_attack_variation, false);

	g_game.announceMissileEffect(player->getPosition(), target->getPosition(), missile);

	if (target->damage(player, total_damage, damage_type)) {
		player->learning_points = 30;
	}

	player->earliest_attack_time = g_game.serverMilliseconds() + 2000;
}
