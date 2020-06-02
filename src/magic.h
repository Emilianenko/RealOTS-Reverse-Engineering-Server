#pragma once

enum SpellType_t
{
	NORMAL_SPELL,
	RUNE_SPELL,
	CHARACTER_RIGHT_SPELL,
};

struct Spell
{
	uint16_t id = 0;
	SpellType_t spell_type = NORMAL_SPELL;
	std::string words;
	std::string name;
	uint16_t level = 0;
	uint16_t rune_level = 0;
	uint16_t mana = 0;
	uint16_t soulpoints = 0;
	uint16_t amount = 0;
	uint16_t rune_nr = 0;
	bool aggressive = false;
	bool has_param = false;
	bool need_target = false;
};

class Player;
class Creature;

class Magic
{
public:
	Magic(const Magic&) = delete; // non construction-copyable
	Magic& operator=(const Magic&) = delete; // non copyable

	explicit Magic() = default;
	~Magic();

	const std::map<std::string, Spell*>& getSpells() const {
		return spells;
	}
	
	void initSpells();

	Spell* castSpell(std::string& text, std::string& params);

	void getMagicItemDescription(uint16_t type_id, std::string& spell_name, uint16_t& rune_level);
	uint16_t getMagicItemCharges(uint16_t type_id);

	bool checkSpellReq(Player* player, Spell* spell) const;
	bool checkMana(Player* player, int32_t mana, int32_t soulpoints, int32_t spell_delay);
	void takeMana(Player* player, uint16_t mana) const;
	void takeSoulpoints(Player* player, uint16_t soulpoints) const;

	void heal(Creature* creature, int32_t value) const;
	void enlight(Creature* creature, uint8_t radius, uint32_t duration) const;
	void magicGoStrength(Creature* creature, Creature* dest_creature, int32_t percent, uint32_t duration) const;
	void negatePoison(Creature* creature);
	void invisibility(Creature* creature, uint32_t duration);
	void magicShield(Player* player, uint32_t duration);
	bool magicRope(Player* player);
	bool magicClimbing(Player* player, const std::string& param, int32_t mana, int32_t soulpoints);
	void createFood(Player* player);
	void createGold(Player* player, uint32_t amount);
private:
	Spell* createSpell(uint16_t id, SpellType_t spell_type, const std::string& name, const std::string& syllables);

	std::map<std::string, Spell*> spells;
};

extern Magic g_magic;