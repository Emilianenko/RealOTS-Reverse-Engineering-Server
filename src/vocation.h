#pragma once
#include "enums.h"

class Vocations;

class Vocation
{
public:
	explicit Vocation() = default;

	// non-copyable
	Vocation(const Vocation&) = delete;
	Vocation& operator=(const Vocation&) = delete;

	uint8_t getId() const {
		return id;
	}
	uint8_t getMaxSoul() const {
		return max_soul;
	}
	uint16_t getSecsPerHp() const {
		return secs_per_hp;
	}
	uint16_t getSecsPerMp() const {
		return secs_per_mp;
	}
	uint16_t getGainHpSecs() const {
		return gain_hp_secs;
	}
	uint16_t getGainMpSecs() const {
		return gain_mp_secs;
	}
	uint16_t getGainHp() const {
		return gain_hp;
	}
	uint16_t getGainMp() const {
		return gain_mp;
	}
	uint16_t getGainCapacity() const {
		return gain_capacity;
	}

	float getMagicFormula() const {
		return magic_multipler;
	}
	float getFistFightingFormula() const {
		return fist_multipler;
	}
	float getSwordFightingFormula() const {
		return sword_multipler;
	}
	float getClubFightingFormula() const {
		return club_multipler;
	}
	float getAxeFightingFormula() const {
		return axe_multipler;
	}
	float getDistanceFightingFormula() const {
		return distance_multipler;
	}
	float getFishingFormula() const {
		return fishing_multipler;
	}

	const std::string& getName() const {
		return name;
	}
	const std::string& getDescription() const {
		return description;
	}

	uint64_t getExperienceForSkill(SkillType_t skill, int32_t level) const;
private:
	uint8_t max_soul = 100;
	uint8_t id = 0;
	uint16_t secs_per_hp = 0;
	uint16_t secs_per_mp = 0;
	uint16_t gain_hp_secs = 0;
	uint16_t gain_mp_secs = 0;
	uint16_t gain_hp = 0;
	uint16_t gain_mp = 0;
	uint16_t gain_capacity = 0;
	uint32_t skill_base[SKILL_SIZE];
	float skill_multipler[SKILL_SIZE];

	float magic_multipler = 4.0f;
	float fist_multipler = 1.5f;
	float sword_multipler = 2.0f;
	float club_multipler = 2.0f;
	float axe_multipler = 2.0f;
	float distance_multipler = 1.5f;
	float fishing_multipler = 1.1f;
	float shielding_multipler = 1.5f;

	std::string name;
	std::string description;

	friend class Vocations;
};

class Vocations
{
public:
	explicit Vocations() = default;
	~Vocations();

	// non-copyable
	Vocations(const Vocations&) = delete;
	Vocations& operator=(const Vocations&) = delete;

	void loadVocations();

	const Vocation* getVocationById(uint16_t id) const;
private:
	std::map<uint16_t, Vocation*> vocations;
};

extern Vocations g_vocations;