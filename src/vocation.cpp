#include "pch.h"

#include "vocation.h"

uint64_t Vocation::getExperienceForSkill(SkillType_t skill, int32_t level) const
{
	uint64_t experience;
	if (skill == SKILL_LEVEL) {
		level--;
		experience = ((50LL * level * level * level) - (150LL * level * level) + (400LL * level)) / 3LL;
	} else if (skill == SKILL_MAGIC) {
		experience = static_cast<uint64_t>(1600 * std::pow<double>(skill_multipler[skill], level - 1));
	} else {
		experience = static_cast<uint64_t>(skill_base[skill] * std::pow(static_cast<double>(skill_multipler[skill]), level - 10));
	}
	return experience;
}

Vocations::~Vocations()
{
	for (auto it = vocations.begin(); it != vocations.end();) {
		delete it->second;
		it = vocations.erase(it);
	}
}

void Vocations::loadVocations()
{
	Vocation* novoc = new Vocation();
	novoc->id = 0;
	novoc->secs_per_hp = 6;
	novoc->secs_per_mp = 6;
	novoc->gain_hp_secs = 1;
	novoc->gain_mp_secs = 2;
	novoc->gain_hp = 5;
	novoc->gain_mp = 5;
	novoc->gain_capacity = 5;
	novoc->skill_base[SKILL_FISTFIGHTING] = 50;
	novoc->skill_base[SKILL_SWORDFIGHTING] = 50;
	novoc->skill_base[SKILL_CLUBFIGHTING] = 50;
	novoc->skill_base[SKILL_AXEFIGHTING] = 50;
	novoc->skill_base[SKILL_DISTANCEFIGHTING] = 50;
	novoc->skill_base[SKILL_SHIELDING] = 100;
	novoc->skill_base[SKILL_FISHING] = 20;
	novoc->skill_multipler[SKILL_FISTFIGHTING] = 2.0f;
	novoc->skill_multipler[SKILL_SWORDFIGHTING] = 2.0f;
	novoc->skill_multipler[SKILL_CLUBFIGHTING] = 2.0f;
	novoc->skill_multipler[SKILL_AXEFIGHTING] = 2.0f;
	novoc->skill_multipler[SKILL_DISTANCEFIGHTING] = 2.0f;
	novoc->skill_multipler[SKILL_SHIELDING] = 2.0f;
	novoc->skill_multipler[SKILL_FISHING] = 1.1f;
	novoc->skill_multipler[SKILL_MAGIC] = 1.0f;
	novoc->name = "no vocation";
	novoc->description = "no vocation";
	vocations[novoc->id] = novoc;

	Vocation* knight = new Vocation();
	knight->id = 1;
	knight->secs_per_hp = 6;
	knight->secs_per_mp = 6;
	knight->gain_hp_secs = 1;
	knight->gain_mp_secs = 2;
	knight->gain_hp = 15;
	knight->gain_mp = 5;
	knight->gain_capacity = 25;
	knight->skill_base[SKILL_FISTFIGHTING] = 50;
	knight->skill_base[SKILL_SWORDFIGHTING] = 50;
	knight->skill_base[SKILL_CLUBFIGHTING] = 50;
	knight->skill_base[SKILL_AXEFIGHTING] = 50;
	knight->skill_base[SKILL_DISTANCEFIGHTING] = 50;
	knight->skill_base[SKILL_SHIELDING] = 100;
	knight->skill_base[SKILL_FISHING] = 20;
	knight->skill_multipler[SKILL_FISTFIGHTING] = 1.1f;
	knight->skill_multipler[SKILL_SWORDFIGHTING] = 1.1f;
	knight->skill_multipler[SKILL_CLUBFIGHTING] = 1.1f;
	knight->skill_multipler[SKILL_AXEFIGHTING] = 1.1f;
	knight->skill_multipler[SKILL_DISTANCEFIGHTING] = 1.1f;
	knight->skill_multipler[SKILL_SHIELDING] = 1.1f;
	knight->skill_multipler[SKILL_FISHING] = 1.1f;
	knight->skill_multipler[SKILL_MAGIC] = 3.0f;
	knight->name = "knight";
	knight->description = "a knight";
	vocations[knight->id] = knight;

	Vocation* paladin = new Vocation();
	paladin->id = 2;
	paladin->secs_per_hp = 8;
	paladin->secs_per_mp = 4;
	paladin->gain_hp_secs = 1;
	paladin->gain_mp_secs = 2;
	paladin->gain_hp = 10;
	paladin->gain_mp = 15;
	paladin->gain_capacity = 20;
	paladin->skill_base[SKILL_FISTFIGHTING] = 50;
	paladin->skill_base[SKILL_SWORDFIGHTING] = 50;
	paladin->skill_base[SKILL_CLUBFIGHTING] = 50;
	paladin->skill_base[SKILL_AXEFIGHTING] = 50;
	paladin->skill_base[SKILL_DISTANCEFIGHTING] = 50;
	paladin->skill_base[SKILL_SHIELDING] = 100;
	paladin->skill_base[SKILL_FISHING] = 20;
	paladin->skill_multipler[SKILL_FISTFIGHTING] = 1.2f;
	paladin->skill_multipler[SKILL_SWORDFIGHTING] = 1.2f;
	paladin->skill_multipler[SKILL_CLUBFIGHTING] = 1.2f;
	paladin->skill_multipler[SKILL_AXEFIGHTING] = 1.2f;
	paladin->skill_multipler[SKILL_DISTANCEFIGHTING] = 1.2f;
	paladin->skill_multipler[SKILL_SHIELDING] = 1.2f;
	paladin->skill_multipler[SKILL_FISHING] = 1.1f;
	paladin->skill_multipler[SKILL_MAGIC] = 1.4f;
	paladin->name = "paladin";
	paladin->description = "a paladin";
	vocations[paladin->id] = paladin;

	Vocation* druid = new Vocation();
	druid->id = 3;
	druid->secs_per_hp = 12;
	druid->secs_per_mp = 3;
	druid->gain_hp_secs = 1;
	druid->gain_mp_secs = 2;
	druid->gain_hp = 5;
	druid->gain_mp = 30;
	druid->gain_capacity = 10;
	druid->skill_base[SKILL_FISTFIGHTING] = 50;
	druid->skill_base[SKILL_SWORDFIGHTING] = 50;
	druid->skill_base[SKILL_CLUBFIGHTING] = 50;
	druid->skill_base[SKILL_AXEFIGHTING] = 50;
	druid->skill_base[SKILL_DISTANCEFIGHTING] = 50;
	druid->skill_base[SKILL_SHIELDING] = 100;
	druid->skill_base[SKILL_FISHING] = 20;
	druid->skill_multipler[SKILL_FISTFIGHTING] = 1.8f;
	druid->skill_multipler[SKILL_SWORDFIGHTING] = 1.8f;
	druid->skill_multipler[SKILL_CLUBFIGHTING] = 1.8f;
	druid->skill_multipler[SKILL_AXEFIGHTING] = 1.8f;
	druid->skill_multipler[SKILL_DISTANCEFIGHTING] = 1.8f;
	druid->skill_multipler[SKILL_SHIELDING] = 1.8f;
	druid->skill_multipler[SKILL_FISHING] = 1.1f;
	druid->skill_multipler[SKILL_MAGIC] = 1.1f;
	druid->name = "druid";
	druid->description = "a druid";
	vocations[druid->id] = druid;

	Vocation* sorcerer = new Vocation();
	sorcerer->id = 4;
	sorcerer->secs_per_hp = 12;
	sorcerer->secs_per_mp = 3;
	sorcerer->gain_hp_secs = 1;
	sorcerer->gain_mp_secs = 2;
	sorcerer->gain_hp = 5;
	sorcerer->gain_mp = 30;
	sorcerer->gain_capacity = 10;
	sorcerer->skill_base[SKILL_FISTFIGHTING] = 50;
	sorcerer->skill_base[SKILL_SWORDFIGHTING] = 50;
	sorcerer->skill_base[SKILL_CLUBFIGHTING] = 50;
	sorcerer->skill_base[SKILL_AXEFIGHTING] = 50;
	sorcerer->skill_base[SKILL_DISTANCEFIGHTING] = 50;
	sorcerer->skill_base[SKILL_SHIELDING] = 100;
	sorcerer->skill_base[SKILL_FISHING] = 20;
	sorcerer->skill_multipler[SKILL_FISTFIGHTING] = 2.0f;
	sorcerer->skill_multipler[SKILL_SWORDFIGHTING] = 2.0f;
	sorcerer->skill_multipler[SKILL_CLUBFIGHTING] = 2.0f;
	sorcerer->skill_multipler[SKILL_AXEFIGHTING] = 2.0f;
	sorcerer->skill_multipler[SKILL_DISTANCEFIGHTING] = 2.0f;
	sorcerer->skill_multipler[SKILL_SHIELDING] = 2.0f;
	sorcerer->skill_multipler[SKILL_FISHING] = 1.1f;
	sorcerer->skill_multipler[SKILL_MAGIC] = 1.1f;
	sorcerer->name = "sorcerer";
	sorcerer->description = "a sorcerer";
	vocations[sorcerer->id] = sorcerer;

	Vocation* elite_knight = new Vocation();
	elite_knight->id = 11;
	elite_knight->secs_per_hp = 4;
	elite_knight->secs_per_mp = 6;
	elite_knight->gain_hp_secs = 1;
	elite_knight->gain_mp_secs = 2;
	elite_knight->gain_hp = 15;
	elite_knight->gain_mp = 5;
	elite_knight->gain_capacity = 25;
	elite_knight->skill_base[SKILL_FISTFIGHTING] = 50;
	elite_knight->skill_base[SKILL_SWORDFIGHTING] = 50;
	elite_knight->skill_base[SKILL_CLUBFIGHTING] = 50;
	elite_knight->skill_base[SKILL_AXEFIGHTING] = 50;
	elite_knight->skill_base[SKILL_DISTANCEFIGHTING] = 50;
	elite_knight->skill_base[SKILL_SHIELDING] = 100;
	elite_knight->skill_base[SKILL_FISHING] = 20;
	elite_knight->skill_multipler[SKILL_FISTFIGHTING] = 1.1f;
	elite_knight->skill_multipler[SKILL_SWORDFIGHTING] = 1.1f;
	elite_knight->skill_multipler[SKILL_CLUBFIGHTING] = 1.1f;
	elite_knight->skill_multipler[SKILL_AXEFIGHTING] = 1.1f;
	elite_knight->skill_multipler[SKILL_DISTANCEFIGHTING] = 1.1f;
	elite_knight->skill_multipler[SKILL_SHIELDING] = 1.1f;
	elite_knight->skill_multipler[SKILL_FISHING] = 1.1f;
	elite_knight->skill_multipler[SKILL_MAGIC] = 3.0f;
	elite_knight->name = "elite knight";
	elite_knight->description = "an elite knight";
	vocations[elite_knight->id] = elite_knight;

	Vocation* royal_paladin = new Vocation();
	royal_paladin->id = 12;
	royal_paladin->secs_per_hp = 6;
	royal_paladin->secs_per_mp = 3;
	royal_paladin->gain_hp_secs = 1;
	royal_paladin->gain_mp_secs = 2;
	royal_paladin->gain_hp = 10;
	royal_paladin->gain_mp = 15;
	royal_paladin->gain_capacity = 20;
	royal_paladin->skill_base[SKILL_FISTFIGHTING] = 50;
	royal_paladin->skill_base[SKILL_SWORDFIGHTING] = 50;
	royal_paladin->skill_base[SKILL_CLUBFIGHTING] = 50;
	royal_paladin->skill_base[SKILL_AXEFIGHTING] = 50;
	royal_paladin->skill_base[SKILL_DISTANCEFIGHTING] = 50;
	royal_paladin->skill_base[SKILL_SHIELDING] = 100;
	royal_paladin->skill_base[SKILL_FISHING] = 20;
	royal_paladin->skill_multipler[SKILL_FISTFIGHTING] = 1.2f;
	royal_paladin->skill_multipler[SKILL_SWORDFIGHTING] = 1.2f;
	royal_paladin->skill_multipler[SKILL_CLUBFIGHTING] = 1.2f;
	royal_paladin->skill_multipler[SKILL_AXEFIGHTING] = 1.2f;
	royal_paladin->skill_multipler[SKILL_DISTANCEFIGHTING] = 1.2f;
	royal_paladin->skill_multipler[SKILL_SHIELDING] = 1.2f;
	royal_paladin->skill_multipler[SKILL_FISHING] = 1.1f;
	royal_paladin->skill_multipler[SKILL_MAGIC] = 1.4f;
	royal_paladin->name = "royal paladin";
	royal_paladin->description = "a royal paladin";
	vocations[royal_paladin->id] = royal_paladin;

	Vocation* elder_druid = new Vocation();
	elder_druid->id = 13;
	elder_druid->secs_per_hp = 12;
	elder_druid->secs_per_mp = 2;
	elder_druid->gain_hp_secs = 1;
	elder_druid->gain_mp_secs = 2;
	elder_druid->gain_hp = 5;
	elder_druid->gain_mp = 30;
	elder_druid->gain_capacity = 10;
	elder_druid->skill_base[SKILL_FISTFIGHTING] = 50;
	elder_druid->skill_base[SKILL_SWORDFIGHTING] = 50;
	elder_druid->skill_base[SKILL_CLUBFIGHTING] = 50;
	elder_druid->skill_base[SKILL_AXEFIGHTING] = 50;
	elder_druid->skill_base[SKILL_DISTANCEFIGHTING] = 50;
	elder_druid->skill_base[SKILL_SHIELDING] = 100;
	elder_druid->skill_base[SKILL_FISHING] = 20;
	elder_druid->skill_multipler[SKILL_FISTFIGHTING] = 1.8f;
	elder_druid->skill_multipler[SKILL_SWORDFIGHTING] = 1.8f;
	elder_druid->skill_multipler[SKILL_CLUBFIGHTING] = 1.8f;
	elder_druid->skill_multipler[SKILL_AXEFIGHTING] = 1.8f;
	elder_druid->skill_multipler[SKILL_DISTANCEFIGHTING] = 1.8f;
	elder_druid->skill_multipler[SKILL_SHIELDING] = 1.8f;
	elder_druid->skill_multipler[SKILL_FISHING] = 1.1f;
	elder_druid->skill_multipler[SKILL_MAGIC] = 1.1f;
	elder_druid->name = "elder druid";
	elder_druid->description = "an elder druid";
	vocations[elder_druid->id] = elder_druid;

	Vocation* master_sorcerer = new Vocation();
	master_sorcerer->id = 14;
	master_sorcerer->secs_per_hp = 12;
	master_sorcerer->secs_per_mp = 2;
	master_sorcerer->gain_hp_secs = 1;
	master_sorcerer->gain_mp_secs = 2;
	master_sorcerer->gain_hp = 5;
	master_sorcerer->gain_mp = 30;
	master_sorcerer->gain_capacity = 10;
	master_sorcerer->skill_base[SKILL_FISTFIGHTING] = 50;
	master_sorcerer->skill_base[SKILL_SWORDFIGHTING] = 50;
	master_sorcerer->skill_base[SKILL_CLUBFIGHTING] = 50;
	master_sorcerer->skill_base[SKILL_AXEFIGHTING] = 50;
	master_sorcerer->skill_base[SKILL_DISTANCEFIGHTING] = 50;
	master_sorcerer->skill_base[SKILL_SHIELDING] = 100;
	master_sorcerer->skill_base[SKILL_FISHING] = 20;
	master_sorcerer->skill_multipler[SKILL_FISTFIGHTING] = 2.0f;
	master_sorcerer->skill_multipler[SKILL_SWORDFIGHTING] = 2.0f;
	master_sorcerer->skill_multipler[SKILL_CLUBFIGHTING] = 2.0f;
	master_sorcerer->skill_multipler[SKILL_AXEFIGHTING] = 2.0f;
	master_sorcerer->skill_multipler[SKILL_DISTANCEFIGHTING] = 2.0f;
	master_sorcerer->skill_multipler[SKILL_SHIELDING] = 2.0f;
	master_sorcerer->skill_multipler[SKILL_FISHING] = 1.1f;
	master_sorcerer->skill_multipler[SKILL_MAGIC] = 1.1f;
	master_sorcerer->name = "master sorcerer";
	master_sorcerer->description = "a master sorcerer";
	vocations[master_sorcerer->id] = master_sorcerer;
}

const Vocation* Vocations::getVocationById(uint16_t id) const
{
	const auto it = vocations.find(id);
	if (it == vocations.end()) {
		return nullptr;
	}

	return it->second;
}
