#pragma once
#include "enums.h"

struct Position;
class Item;
class Creature;
class Player;
class Tile;
class Object;

struct ImpactDamage
{
	Creature* actor = nullptr;

	virtual void handleCreature(Creature* victim) {
		//
	};
	virtual void handleField(Tile* tile) {
		//
	};
};

struct DamageImpact : ImpactDamage
{
	bool allow_defense = false;
	uint32_t power = 0;
	DamageType_t damage_type = DAMAGE_PHYSICAL;

	void handleCreature(Creature* victim) final;
};

struct FieldImpact : ImpactDamage
{
	FieldType_t field_type = FIELD_NONE;

	void handleField(Tile* tile) final;
};

struct HealingImpact : ImpactDamage
{
	uint32_t power = 0;

	void handleCreature(Creature* victim) final;
};

struct CancelInvisibleImpact : ImpactDamage
{
	void handleCreature(Creature* victim) final;
};

class MatrixArea
{
public:
	MatrixArea(uint32_t rows, uint32_t cols) : center_x(0), center_y(0), rows(rows), cols(cols) {
		data_ = new bool*[rows];

		for (uint32_t row = 0; row < rows; ++row) {
			data_[row] = new bool[cols];

			for (uint32_t col = 0; col < cols; ++col) {
				data_[row][col] = false;
			}
		}
	}

	MatrixArea(const MatrixArea& rhs) {
		center_x = rhs.center_x;
		center_y = rhs.center_y;
		rows = rhs.rows;
		cols = rhs.cols;

		data_ = new bool*[rows];

		for (uint32_t row = 0; row < rows; ++row) {
			data_[row] = new bool[cols];

			for (uint32_t col = 0; col < cols; ++col) {
				data_[row][col] = rhs.data_[row][col];
			}
		}
	}

	~MatrixArea() {
		for (uint32_t row = 0; row < rows; ++row) {
			delete[] data_[row];
		}

		delete[] data_;
	}

	void setValue(uint32_t row, uint32_t col, bool value) const {
		data_[row][col] = value;
	}
	bool getValue(uint32_t row, uint32_t col) const {
		return data_[row][col];
	}

	void setCenter(uint32_t y, uint32_t x) {
		center_x = x;
		center_y = y;
	}
	void getCenter(uint32_t& y, uint32_t& x) const {
		x = center_x;
		y = center_y;
	}

	uint32_t getRows() const {
		return rows;
	}
	uint32_t getCols() const {
		return cols;
	}

	const bool* operator[](uint32_t i) const {
		return data_[i];
	}
	bool* operator[](uint32_t i) {
		return data_[i];
	}
private:
	uint32_t center_x;
	uint32_t center_y;

	uint32_t rows;
	uint32_t cols;
	bool** data_;
};

class AreaCombat
{
public:
	AreaCombat() = default;

	AreaCombat(const AreaCombat& rhs);
	~AreaCombat() {
		clear();
	}

	void getList(const Position& center_pos, const Position& target_pos, std::forward_list<Tile*>& list) const;

	void setupArea(const std::list<uint32_t>& list, uint32_t rows);
	void setupArea(int32_t length, int32_t spread);
	void setupArea(int32_t radius);
	void setupExtArea(const std::list<uint32_t>& list, uint32_t rows);
	void clear();
private:
	enum MatrixOperation_t {
		MATRIXOPERATION_COPY,
		MATRIXOPERATION_MIRROR,
		MATRIXOPERATION_FLIP,
		MATRIXOPERATION_ROTATE90,
		MATRIXOPERATION_ROTATE180,
		MATRIXOPERATION_ROTATE270,
	};

	MatrixArea* createArea(const std::list<uint32_t>& list, uint32_t rows) const;
	void copyArea(const MatrixArea* input, MatrixArea* output, MatrixOperation_t op) const;

	MatrixArea* getArea(const Position& center_pos, const Position& target_pos) const;

	std::map<Direction_t, MatrixArea*> areas;
	bool has_ext_area = false;
};

class Combat
{
public:
	static int32_t computeDamage(Creature* creature, int32_t damage, int32_t variation, bool apply_formulas);

	static bool canDoAttack(Creature* creature, Creature* target);

	static void setAttackDest(Creature* creature, Creature* target, bool follow);
	static void attack(Creature* creature, Creature* target);

	static void addDamageToCombatList(Creature* creature, Creature* attacker, uint32_t damage);

	static bool massCombat(Creature* creature, Object* target, uint32_t mana, uint32_t soulpoints, uint32_t damage, uint32_t effect_nr, uint32_t radius, DamageType_t damage_type, uint32_t missile);
	static bool angleCombat(Creature* creature, uint32_t mana, uint32_t soulpoints, uint32_t damage, uint32_t effect_nr, uint32_t length, uint32_t spread, DamageType_t damage_type);

	static void circleShapeSpell(const Position& from_pos, const Position& to_pos, uint8_t radius, uint8_t effect, uint8_t animation, ImpactDamage& impact);
	static void angleShapeSpell(const Position& pos, uint16_t length, uint16_t spread, uint8_t effect, ImpactDamage& impact);
	static void angleWallSpell(const Position& pos, const Position& to_pos, uint8_t effect, uint8_t animation, ImpactDamage& impact);
	static void customShapeSpell(const std::list<uint32_t>& list, uint32_t rows, const Position& pos, uint8_t effect, uint8_t animation, ImpactDamage& impact);
protected:
	static void meleeAttack(Creature* creature, Creature* target);
	static void closeAttack(Player* player, Item* weapon, Creature* target);
	static void rangeAttack(Player* player, Item* weapon, Creature* target);
	static void wandAttack(Player* player, Item* wand, Creature* target);
};