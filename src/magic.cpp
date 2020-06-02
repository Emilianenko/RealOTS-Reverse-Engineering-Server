#include "pch.h"

#include "magic.h"

#include "creature.h"
#include "game.h"
#include "protocol.h"
#include "player.h"
#include "item.h"
#include "tile.h"
#include "map.h"
#include "tools.h"

#include <boost/algorithm/string/erase.hpp>

Magic::~Magic()
{
	for (auto it : spells) {
		delete it.second;
	}
}

void Magic::initSpells()
{
	Spell* spell = createSpell(1, NORMAL_SPELL, "Light Healing", "exura");
	spell->level = 9;
	spell->mana = 25;

	spell = createSpell(2, NORMAL_SPELL, "Intense Healing", "exura gran");
	spell->level = 11;
	spell->mana = 40;

	spell = createSpell(3, NORMAL_SPELL, "Ultimate Healing", "exura vita");
	spell->level = 20;
	spell->mana = 160;

	spell = createSpell(4, RUNE_SPELL, "Intense Healing Rune", "adura gran");
	spell->level = 15;
	spell->mana = 240;
	spell->rune_nr = 3152;
	spell->amount = 1;
	spell->rune_level = 1;
	spell->soulpoints = 2;
	spell->need_target = true;

	spell = createSpell(5, RUNE_SPELL, "Ultimate Healing Rune", "adura vita");
	spell->level = 24;
	spell->mana = 400;
	spell->rune_nr = 3160;
	spell->amount = 1;
	spell->rune_level = 4;
	spell->soulpoints = 3;
	spell->need_target = true;

	spell = createSpell(6, NORMAL_SPELL, "Haste", "utani hur");
	spell->level = 14;
	spell->mana = 14;
	
	spell = createSpell(7, RUNE_SPELL, "Light Magic Missile", "adori");
	spell->level = 15;
	spell->mana = 120;
	spell->rune_nr = 3174;
	spell->amount = 5;
	spell->rune_level = 0;
	spell->soulpoints = 1;
	spell->aggressive = true;
	spell->need_target = true;
	
	spell = createSpell(8, RUNE_SPELL, "Heavy Magic Missile", "adori gran");
	spell->level = 25;
	spell->mana = 280;
	spell->rune_nr = 3198;
	spell->amount = 5;
	spell->rune_level = 4;
	spell->soulpoints = 2;
	spell->aggressive = true;
	spell->need_target = true;

	spell = createSpell(9, NORMAL_SPELL, "Summon Creature", "utevo res");
	spell->level = 25;
	spell->mana = 0;

	spell = createSpell(10, NORMAL_SPELL, "Light", "utevo lux");
	spell->level = 8;
	spell->mana = 20;

	spell = createSpell(11, NORMAL_SPELL, "Great Light", "utevo gran lux");
	spell->level = 13;
	spell->mana = 60;

	spell = createSpell(12, RUNE_SPELL, "Convince Creature", "adeta sio");
	spell->level = 16;
	spell->mana = 200;
	spell->rune_nr = 3177;
	spell->amount = 1;
	spell->rune_level = 5;
	spell->soulpoints = 3;

	spell = createSpell(13, NORMAL_SPELL, "Energy Wave", "exevo mort hur");
	spell->level = 38;
	spell->mana = 250;
	spell->aggressive = true;

	spell = createSpell(14, RUNE_SPELL, "Chameleon", "adevo ina");
	spell->level = 27;
	spell->mana = 600;
	spell->rune_nr = 3178;
	spell->amount = 1;
	spell->rune_level = 4;
	spell->soulpoints = 2;

	spell = createSpell(15, RUNE_SPELL, "Fireball", "adori flam");
	spell->level = 17;
	spell->mana = 160;
	spell->rune_nr = 3189;
	spell->amount = 2;
	spell->rune_level = 2;
	spell->soulpoints = 2;
	spell->aggressive = true;
	
	spell = createSpell(16, RUNE_SPELL, "Great Fireball", "adori gran flam");
	spell->level = 23;
	spell->mana = 480;
	spell->rune_nr = 3191;
	spell->amount = 2;
	spell->rune_level = 4;
	spell->soulpoints = 2;
	spell->aggressive = true;
		
	spell = createSpell(17, RUNE_SPELL, "Firebomb", "adevo mas flam");
	spell->level = 27;
	spell->mana = 600;
	spell->rune_nr = 3192;
	spell->amount = 2;
	spell->rune_level = 5;
	spell->soulpoints = 4;
	spell->aggressive = true;
		
	spell = createSpell(18, RUNE_SPELL, "Explosion", "adevo mas hur");
	spell->level = 31;
	spell->mana = 720;
	spell->rune_nr = 3200;
	spell->amount = 3;
	spell->rune_level = 6;
	spell->soulpoints = 4;
	spell->aggressive = true;

	spell = createSpell(19, NORMAL_SPELL, "Fire Wave", "exevo flam hur");
	spell->level = 18;
	spell->mana = 80;
	spell->aggressive = true;
	
	spell = createSpell(20, NORMAL_SPELL, "Find Person", "exiva");
	spell->level = 8;
	spell->mana = 20;
	spell->has_param = true;
			
	spell = createSpell(21, RUNE_SPELL, "Sudden Death", "adori vita vis");
	spell->level = 45;
	spell->mana = 880;
	spell->rune_nr = 3155;
	spell->amount = 1;
	spell->rune_level = 15;
	spell->soulpoints = 5;
	spell->need_target = true;
	spell->aggressive = true;
		
	spell = createSpell(22, NORMAL_SPELL, "Energy Beam", "exevo vis lux");
	spell->level = 23;
	spell->mana = 100;
	spell->aggressive = true;
			
	spell = createSpell(23, NORMAL_SPELL, "Great Energy Beam", "exevo gran vis lux");
	spell->level = 29;
	spell->mana = 200;
	spell->aggressive = true;
				
	spell = createSpell(24, NORMAL_SPELL, "Ultimate Explosion", "exevo gran mas vis");
	spell->level = 60;
	spell->mana = 1200;
	spell->aggressive = true;
				
	spell = createSpell(25, RUNE_SPELL, "Fire Field", "adevo grav flam");
	spell->level = 15;
	spell->mana = 240;
	spell->rune_nr = 3188;
	spell->amount = 3;
	spell->rune_level = 1;
	spell->soulpoints = 1;
	spell->aggressive = true;
						
	spell = createSpell(26, RUNE_SPELL, "Poison Field", "adevo grav pox");
	spell->level = 14;
	spell->mana = 200;
	spell->rune_nr = 3172;
	spell->amount = 3;
	spell->rune_level = 0;
	spell->soulpoints = 1;
	spell->aggressive = true;
								
	spell = createSpell(27, RUNE_SPELL, "Energy Field", "adevo grav vis");
	spell->level = 18;
	spell->mana = 320;
	spell->rune_nr = 3164;
	spell->amount = 3;
	spell->rune_level = 3;
	spell->soulpoints = 2;
	spell->aggressive = true;
										
	spell = createSpell(28, RUNE_SPELL, "Fire Wall", "adevo mas grav flam");
	spell->level = 33;
	spell->mana = 780;
	spell->rune_nr = 3190;
	spell->amount = 4;
	spell->rune_level = 6;
	spell->soulpoints = 4;
	spell->aggressive = true;
			
	spell = createSpell(29, NORMAL_SPELL, "Antidote", "exana pox");
	spell->level = 10;
	spell->mana = 30;
											
	spell = createSpell(30, RUNE_SPELL, "Destroy Field", "adito grav");
	spell->level = 17;
	spell->mana = 120;
	spell->rune_nr = 3148;
	spell->amount = 3;
	spell->rune_level = 3;
	spell->soulpoints = 2;
														
	spell = createSpell(31, RUNE_SPELL, "Antidote Rune", "adana pox");
	spell->level = 15;
	spell->mana = 200;
	spell->rune_nr = 3153;
	spell->amount = 1;
	spell->rune_level = 0;
	spell->soulpoints = 1;
	spell->need_target = true;

	spell = createSpell(32, RUNE_SPELL, "Poison Wall", "adevo mas grav pox");
	spell->level = 29;
	spell->mana = 640;
	spell->rune_nr = 3176;
	spell->amount = 4;
	spell->rune_level = 5;
	spell->soulpoints = 3;
	spell->aggressive = true;
			
	spell = createSpell(33, RUNE_SPELL, "Energy Wall", "adevo mas grav vis");
	spell->level = 41;
	spell->mana = 1000;
	spell->rune_nr = 3166;
	spell->amount = 4;
	spell->rune_level = 9;
	spell->soulpoints = 5;
	spell->aggressive = true;
		
	spell = createSpell(34, CHARACTER_RIGHT_SPELL, "Get Item", "/i");
	spell->has_param = true;

	spell = createSpell(37, CHARACTER_RIGHT_SPELL, "Teleport", "/a");
	spell->has_param = true;
	
	spell = createSpell(38, NORMAL_SPELL, "Creature Illusion", "utevo res ina");
	spell->level = 23;
	spell->mana = 100;
		
	spell = createSpell(39, NORMAL_SPELL, "Great Haste", "utani gran hur");
	spell->level = 20;
	spell->mana = 100;

	spell = createSpell(42, NORMAL_SPELL, "Food", "exevo pan");
	spell->level = 14;
	spell->mana = 120;	
	spell->soulpoints = 1;

	spell = createSpell(44, NORMAL_SPELL, "Magic Shield", "utamo vita");
	spell->level = 14;
	spell->mana = 50;	
			
	spell = createSpell(45, NORMAL_SPELL, "Invisible", "utana vid");
	spell->level = 35;
	spell->mana = 440;	

	spell = createSpell(47, CHARACTER_RIGHT_SPELL, "Teleport to Friend", "/goto");
	spell->has_param = true;

	spell = createSpell(48, NORMAL_SPELL, "Poisoned Arrow", "exevo con pox");
	spell->level = 16;
	spell->mana = 130;	
	spell->soulpoints = 2;	
	
	spell = createSpell(49, NORMAL_SPELL, "Explosive Arrow", "exevo con flam");
	spell->level = 25;
	spell->mana = 290;	
	spell->soulpoints = 2;	
				
	spell = createSpell(50, RUNE_SPELL, "Soulfire", "adevo res flam");
	spell->level = 27;
	spell->mana = 600;
	spell->rune_nr = 3195;
	spell->amount = 2;
	spell->rune_level = 7;
	spell->soulpoints = 3;
	spell->aggressive = true;
	spell->need_target = true;
			
	spell = createSpell(51, NORMAL_SPELL, "Conjure Arrow", "exevo con");
	spell->level = 13;
	spell->mana = 100;	
	spell->soulpoints = 2;	
				
	spell = createSpell(52, CHARACTER_RIGHT_SPELL, "Retrieve Friend", "/retrievefriend");
	spell->has_param = true;

	createSpell(53, NORMAL_SPELL, "Summon Wild Creature", "alevo res");
	
	spell = createSpell(54, RUNE_SPELL, "Paralyze", "adana ani");
	spell->level = 54;
	spell->mana = 1400;
	spell->rune_nr = 3165;
	spell->amount = 1;
	spell->rune_level = 18;
	spell->soulpoints = 3;
	spell->aggressive = true;
	spell->need_target = true;
					
	spell = createSpell(55, RUNE_SPELL, "Energybomb", "adevo mas vis");
	spell->level = 37;
	spell->mana = 880;
	spell->rune_nr = 3149;
	spell->amount = 2;
	spell->rune_level = 10;
	spell->soulpoints = 5;
	spell->aggressive = true;
	
	spell = createSpell(56, NORMAL_SPELL, "Poison Storm", "exevo gran mas pox");
	spell->level = 50;
	spell->mana = 600;
	spell->aggressive = true;
	
	createSpell(58, CHARACTER_RIGHT_SPELL, "Get Position", "/getposition");
	
	spell = createSpell(60, CHARACTER_RIGHT_SPELL, "Temple Teleport", "/temple");
	spell->has_param = true;

	spell = createSpell(63, CHARACTER_RIGHT_SPELL, "Create Gold", "/creategold");
	spell->has_param = true;

	spell = createSpell(64, CHARACTER_RIGHT_SPELL, "Change Profession or Sex", "/change");
	spell->has_param = true;

	spell = createSpell(67, CHARACTER_RIGHT_SPELL, "Kick Player", "/kick");
	spell->has_param = true;

	createSpell(71, NORMAL_SPELL, "Invite Guests", "aleta sio");
	createSpell(72, NORMAL_SPELL, "Invite Subowners", "aleta som");
	createSpell(73, NORMAL_SPELL, "Kick Guest", "alana sio");
	createSpell(74, NORMAL_SPELL, "Edit Door", "aleta grav");
			
	spell = createSpell(75, NORMAL_SPELL, "Ultimate Light", "utevo vis lux");
	spell->level = 26;
	spell->mana = 140;
	spell->aggressive = true;
			
	spell = createSpell(76, NORMAL_SPELL, "Magic Rope", "exani tera");
	spell->level = 23;
	spell->mana = 20;
					
	spell = createSpell(77, RUNE_SPELL, "Envenom", "adevo res pox");
	spell->level = 21;
	spell->mana = 400;
	spell->rune_nr = 3179;
	spell->amount = 1;
	spell->rune_level = 4;
	spell->soulpoints = 2;
	spell->aggressive = true;
	spell->need_target = true;
						
	spell = createSpell(78, RUNE_SPELL, "Desintegrate", "adito tera");
	spell->level = 21;
	spell->mana = 200;
	spell->rune_nr = 3197;
	spell->amount = 3;
	spell->rune_level = 4;
	spell->soulpoints = 3;
	spell->aggressive = true;
				
	spell = createSpell(79, NORMAL_SPELL, "Conjure Bolt", "exevo con mort");
	spell->level = 17;
	spell->mana = 140;
	spell->soulpoints = 2;	
					
	spell = createSpell(80, NORMAL_SPELL, "Berserk", "exori");
	spell->level = 35;
	spell->mana = 0;
	spell->aggressive = true;

	spell = createSpell(81, NORMAL_SPELL, "Levitate", "exani hur");
	spell->level = 12;
	spell->mana = 50;
	spell->has_param = true;
	
	spell = createSpell(82, NORMAL_SPELL, "Mass Healing", "exura gran mas res");
	spell->level = 36;
	spell->mana = 150;
							
	spell = createSpell(83, RUNE_SPELL, "Animate Dead", "adana mort");
	spell->level = 27;
	spell->mana = 600;
	spell->rune_nr = 3203;
	spell->amount = 1;
	spell->rune_level = 4;
	spell->soulpoints = 5;
	spell->aggressive = true;
	
	spell = createSpell(84, NORMAL_SPELL, "Heal Friend", "exura sio");
	spell->level = 12;
	spell->mana = 50;
	spell->has_param = true;
			
	spell = createSpell(85, NORMAL_SPELL, "Undead Legion", "exana mas mort");
	spell->level = 30;
	spell->mana = 500;
	spell->aggressive = true;
							
	spell = createSpell(86, RUNE_SPELL, "Magic Wall", "adevo grav tera");
	spell->level = 32;
	spell->mana = 750;
	spell->rune_nr = 3180;
	spell->amount = 3;
	spell->rune_level = 9;
	spell->soulpoints = 5;
	spell->aggressive = true;
				
	spell = createSpell(87, NORMAL_SPELL, "Force Strike", "exori mort");
	spell->level = 11;
	spell->mana = 20;
	spell->aggressive = true;
						
	spell = createSpell(88, NORMAL_SPELL, "Energy Strike", "exori vis");
	spell->level = 12;
	spell->mana = 20;
	spell->aggressive = true;
							
	spell = createSpell(89, NORMAL_SPELL, "Flame Strike", "exori flam");
	spell->level = 12;
	spell->mana = 20;
	spell->aggressive = true;
							
	spell = createSpell(90, NORMAL_SPELL, "Cancel Invisibility", "exana ina");
	spell->level = 26;
	spell->mana = 200;
								
	spell = createSpell(91, RUNE_SPELL, "Poisonbomb", "adevo mas pox");
	spell->level = 25;
	spell->mana = 520;
	spell->rune_nr = 3173;
	spell->amount = 2;
	spell->rune_level = 4;
	spell->soulpoints = 2;
	spell->aggressive = true;
							
	spell = createSpell(92, NORMAL_SPELL, "Enchant Staff", "exeta vis");
	spell->level = 41;
	spell->mana = 80;
							
	spell = createSpell(93, NORMAL_SPELL, "Challenge", "exeta res");
	spell->level = 20;
	spell->mana = 30;
							
	spell = createSpell(94, NORMAL_SPELL, "Wild Growth", "exevo grav vita");
	spell->level = 27;
	spell->mana = 220;
								
	spell = createSpell(95, NORMAL_SPELL, "Power Bolt", "exevo con vis");
	spell->level = 59;
	spell->mana = 800;
	spell->soulpoints = 3;

	spell = createSpell(103, CHARACTER_RIGHT_SPELL, "Add Mana", "/addmana");
	spell->has_param = true;
	spell = createSpell(104, CHARACTER_RIGHT_SPELL, "Remove Experience", "/removeexperience");
	spell->has_param = true;
	spell = createSpell(105, CHARACTER_RIGHT_SPELL, "Add Experience", "/addexperience");
	spell->has_param = true;
}

Spell* Magic::castSpell(std::string& text, std::string& params)
{
	Spell* result = nullptr;
	for (const auto it : spells) {
		const std::string& spell_words = it.second->words;
		const size_t spell_length = spell_words.length();

		if (strncasecmp(spell_words.c_str(), text.c_str(), spell_length) == 0) {
			if (result == nullptr) {
				result = it.second;
				if (text.length() == spell_length) {
					break;
				}
			}
		}

		if (result) {
			if (text.length() > spell_length) {
				const size_t param_len = spell_length + 1;
				if (param_len < text.length()) {
					if (text[spell_length] != ' ' || (!it.second->has_param && text[spell_length + 1] != '"')) {
						result = nullptr;
					} else {
						params = text.substr(param_len, text.length());
					}
				} else {
					result = nullptr;
				}
			} else if (it.second->has_param) {
				// spell has params and no param was given
				result = nullptr;
			}
		}

		if (result) {
			break;
		}
	}

	return result;
}

void Magic::getMagicItemDescription(uint16_t type_id, std::string& spell_name, uint16_t& rune_level)
{
	for (const auto it : spells) {
		const Spell* spell = it.second;
		if (spell->rune_nr == type_id) {
			rune_level = spell->rune_level;
			spell_name = spell->words;
			break;
		}
	}
}

uint16_t Magic::getMagicItemCharges(uint16_t type_id)
{
	for (const auto it : spells) {
		const Spell* spell = it.second;
		if (spell->rune_nr == type_id) {
			return spell->amount;
		}
	}

	return 0;
}

bool Magic::checkSpellReq(Player* player, Spell* spell) const
{
	if (spell->mana) {
		if (player->skill_manapoints->getValue() < spell->mana) {
			Protocol::sendResult(player->connection_ptr, NOTENOUGHMANA);
			return false;
		}
	}

	if (spell->soulpoints) {
		if (player->skill_soulpoints->getSoulpoints() < spell->soulpoints) {
			Protocol::sendResult(player->connection_ptr, NOTENOUGHSOUL);
			return false;
		}
	}

	if (spell->level) {
		if (player->skill_level[SKILL_LEVEL] < spell->level) {
			Protocol::sendResult(player->connection_ptr, NOTENOUGHLEVEL);
			return false;
		}
	}

	if (spell->rune_level) {
		if (player->skill_level[SKILL_MAGIC] < spell->rune_level) {
			Protocol::sendResult(player->connection_ptr, NOTENOUGHMAGICLEVEL);
			return false;
		}
	}

	return true;
}

bool Magic::checkMana(Player* player, int32_t mana, int32_t soulpoints, int32_t spell_delay)
{
	if (player->skill_manapoints->getValue() < mana) {
		Protocol::sendResult(player->connection_ptr, NOTENOUGHMANA);
		return false;
	}

	if (player->skill_soulpoints->getSoulpoints() < soulpoints) {
		Protocol::sendResult(player->connection_ptr, NOTENOUGHSOUL);
		return false;
	}

	if (mana > 0) {
		player->addExperience(SKILL_MAGIC, mana);
	}

	player->skill_manapoints->change(-mana);
	player->skill_soulpoints->change(-soulpoints);

	player->earliest_spell_time = g_game.serverMilliseconds() + spell_delay;
	return true;
}

void Magic::takeMana(Player* player, uint16_t mana) const
{
	if (mana == 0) {
		return;
	}

	if (player->skill_manapoints->getValue() < mana) {
		return;
	}

	player->skill_manapoints->change(-mana);
	player->addExperience(SKILL_MAGIC, mana);
}

void Magic::takeSoulpoints(Player* player, uint16_t soulpoints) const
{
	if (soulpoints == 0) {
		return;
	}

	if (player->skill_soulpoints->getSoulpoints() < soulpoints) {
		return;
	}

	player->skill_soulpoints->change(-soulpoints);
}

void Magic::heal(Creature* creature, int32_t value) const
{
	if (creature == nullptr) {
		fmt::printf("ERROR - Magic::heal: creature is null.\n");
		return;
	}

	if (creature->skill_hitpoints->getValue() > 0) {
		creature->skill_hitpoints->change(value);
	}

	if (creature->skill_go_strength->getDelta() < 0) {
		creature->skill_go_strength->setTiming(0, 0, 0, 0);
	}

	g_game.announceGraphicalEffect(creature, 13);
}

void Magic::enlight(Creature* creature, uint8_t radius, uint32_t duration) const
{
	if (creature == nullptr) {
		fmt::printf("ERROR - Magic::enlight: creature is null.\n");
		return;
	}

	SkillLight* skill = creature->skill_light;
	if (radius >= skill->getTiming()) {
		skill->setTiming(radius, duration / radius, duration / radius, -1);
	}

	g_game.announceGraphicalEffect(creature, 13);
}

void Magic::magicGoStrength(Creature* creature, Creature* dest_creature, int32_t percent, uint32_t duration) const
{
	if (creature == nullptr || dest_creature == nullptr) {
		fmt::printf("ERROR - Magic::magicGoStrength: creature is null.\n");
		return;
	}

	int32_t formula;

	SkillGoStrength* skill = dest_creature->skill_go_strength;
	if (percent >= -100) {
		formula = percent * skill->getBaseSpeed() / 100;
	} else {
		formula = -20 - skill->getBaseSpeed();
	}

	skill->setDelta(formula);
	skill->setTiming(duration, 10, 10, -1);

	g_game.announceGraphicalEffect(creature, 15);
	g_game.announceGraphicalEffect(dest_creature, 14);

	if (percent < 0) {
		if (Player* player = creature->getPlayer()) {
			player->blockLogout(60, dest_creature->getPlayer() != nullptr);
		}
	}
}

void Magic::negatePoison(Creature* creature)
{
	creature->skill_poison->setTiming(0, 0, 0, -1);
	g_game.announceGraphicalEffect(creature, 13);
}

void Magic::invisibility(Creature* creature, uint32_t duration)
{
	Outfit outfit;
	creature->setCurrentOutfit(outfit);
	creature->skill_illusion->setTiming(1, duration, duration, -1);
	g_game.announceGraphicalEffect(creature, 13);
}

void Magic::magicShield(Player* player, uint32_t duration)
{
	player->skill_magic_shield->setTiming(1, duration, duration, -1);
	g_game.announceGraphicalEffect(player, 13);
}

bool Magic::magicRope(Player* player)
{
	Tile* tile = player->getParent()->getTile();
	if (!tile->getFlag(ROPESPOT)) {
		Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
		return false;
	}

	const Position& pos = tile->getPosition();
	Tile* to_tile = g_map.getTile(Position(pos.x, pos.y + 1, pos.z - 1));
	if (to_tile == nullptr) {
		Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
		return false;
	}

	g_game.moveCreature(player, to_tile, FLAG_NOLIMIT);
	g_game.announceGraphicalEffect(player, 11);
	return true;
}

bool Magic::magicClimbing(Player* player, const std::string& param, int32_t mana, int32_t soulpoints)
{
	Position to_pos = player->getPosition();
	switch (player->look_direction) {
		case DIRECTION_NORTH: to_pos.y--; break;
		case DIRECTION_EAST: to_pos.x++; break;
		case DIRECTION_WEST: to_pos.x--; break;
		case DIRECTION_SOUTH: to_pos.y++; break;
	}

	if (lowerString(param) == "down") {
		to_pos.z++;
	} else if (lowerString(param) == "up") {
		to_pos.z--;
	}

	if (to_pos.z == player->getPosition().z) {
		Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
		return false;
	}

	Tile* to_tile = g_map.getTile(to_pos);
	if (to_tile == nullptr) {
		return false;
	}

	ReturnValue_t ret = to_tile->queryAdd(INDEX_ANYWHERE, player, 1, 0, nullptr);
	if (ret != ALLGOOD) {
		return false;
	}

	if (!checkMana(player, mana, soulpoints, 1000)) {
		return false;
	}

	g_game.moveCreature(player, to_tile, FLAG_NOLIMIT);
	g_game.announceGraphicalEffect(player, 11);
	return true;
}

void Magic::createFood(Player* player)
{
	int32_t max = rand() % 2 + 1;
	for (int32_t i = 0; i < max; i++) {
		const ItemType* item_type = nullptr;
		switch (rand() % 7) {
			case 0: item_type = g_items.getItemType(3600); break;
			case 1: item_type = g_items.getItemType(3577); break;
			case 2: item_type = g_items.getItemType(3607); break;
			case 3: item_type = g_items.getItemType(3582); break;
			case 4: item_type = g_items.getItemType(3585); break;
			case 5: item_type = g_items.getItemType(3592); break;
			case 6: item_type = g_items.getItemType(3601); break;
			default: break;
		}

		g_game.createItemAtPlayer(player, item_type, 1);
	}

	g_game.announceGraphicalEffect(player, 15);
}

void Magic::createGold(Player* player, uint32_t amount)
{
	if (amount > 1000000) {
		Protocol::sendTextMessage(player->connection_ptr, 23, "You may only create 1 to 1,000,000 gold.");
		return;
	}

	const uint32_t tenthousand = amount / 10000;
	if (tenthousand > 0) {
		g_game.createItemAtPlayer(player, g_items.getSpecialItem(SPECIAL_MONEY_TENTHOUSAND), tenthousand);
	}

	const uint32_t hundred = amount % 10000 / 100;
	if (hundred) {
		g_game.createItemAtPlayer(player, g_items.getSpecialItem(SPECIAL_MONEY_HUNDRED), hundred);
	}

	const uint32_t one = amount % 100;
	if (one) {
		g_game.createItemAtPlayer(player, g_items.getSpecialItem(SPECIAL_MONEY_ONE), one);
	}

	g_game.announceGraphicalEffect(player, 15);
}

Spell* Magic::createSpell(uint16_t id, SpellType_t spell_type, const std::string& name, const std::string& syllables)
{
	Spell* spell = new Spell();
	spell->id = id;
	spell->spell_type = spell_type;
	spell->words = syllables;
	spell->name = name;

	for (const auto it : spells) {
		if (it.second->words == syllables || it.second->id == id) {
			fmt::printf("ERROR - Magic::createSpell: spell already exists (%d - %s)\n", id, syllables);
			delete spell;
			return it.second;
		}
	}

	spells[syllables] = spell;
	return spell;
}
