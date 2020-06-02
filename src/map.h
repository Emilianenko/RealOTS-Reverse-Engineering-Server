#pragma once

#include "tile.h"
#include "item.h"

#include <queue>

template<typename T>
struct Matrix
{
	virtual ~Matrix() {
		delete entry;
	}

	int32_t xmin;
	int32_t ymin;
	int32_t zmin;
	int32_t xmax;
	int32_t ymax;
	int32_t dx;
	int32_t dy;

	T* entry;

	void init(int32_t xmin, int32_t xmax, int32_t ymin, int32_t ymax);
};

struct ShortwayPoint
{
	int32_t x = 0;
	int32_t y = 0;
	int32_t waypoints = -1;
	int32_t waylength = -1;
	int32_t heuristic = -1;
	bool open = false;
	ShortwayPoint* parent = nullptr;
};

struct ShortwayPointEntryComparator
{
	bool operator()(const ShortwayPoint* l, const ShortwayPoint* r) const;
};

class Shortway
{
public:
	explicit Shortway(Creature* creature);
	~Shortway() = default;

	bool calculate(int32_t destx, int32_t desty, bool must_reach);
private:
	void fillMap();

	int32_t startx = 0;
	int32_t starty = 0;
	int32_t startz = 0;

	int32_t destx = 0;
	int32_t desty = 0;

	int32_t minwaypoints = 1000;

	Creature* creature = nullptr;
	ShortwayPoint* start_point = nullptr;
	ShortwayPoint* end_point = nullptr;

	Matrix<ShortwayPoint> points;

	std::priority_queue<ShortwayPoint*, std::deque<ShortwayPoint*>, ShortwayPointEntryComparator> open_points{};
};

struct Sector
{
	constexpr Sector() = default;
	~Sector();

	// non-copyable
	Sector(const Sector&) = delete;
	Sector& operator=(const Sector&) = delete;

	Tile* tile[32][32] = {};
};

struct SectorMap
{
	SectorMap() = default;
	~SectorMap() {
		delete[] entry;
	}

	int32_t xmin;
	int32_t ymin;
	int32_t zmin;
	int32_t xmax;
	int32_t ymax;
	int32_t zmax;
	int32_t dx;
	int32_t dy;
	int32_t dz;

	Sector* entry;

	void init(int32_t xmin, int32_t xmax, int32_t ymin, int32_t ymax, int32_t zmin, int32_t zmax);
};

class Game;
class ScriptReader;

class Map
{
public:
	explicit Map() = default;

	bool loadMap();

	Tile* getTile(const Position& pos) const {
		return getTile(pos.x, pos.y, pos.z);
	}
	Tile* getTile(int32_t x, int32_t y, int32_t z) const;

	bool isProtectionZone(const Position& pos) const;
	bool getCoordinateFlag(const Position& pos, ItemFlags_t flag) const;
	bool searchFreeField(const Creature* creature, int32_t x, int32_t y, int32_t z, int32_t distance) const;
	bool searchFreeField(const Creature* creature, Position& pos, uint8_t distance) const;
	bool throwPossible(const Position& from_pos, const Position& to_pos) const;
	bool fieldPossible(const Position& pos, FieldType_t field_type) const;
private:
	Tile* createTile(int32_t x, int32_t y, int32_t z) const;
	Sector* getSector(int32_t x, int32_t y, int32_t z) const;

	bool loadSector(const std::string& filename) const;
	bool loadContents(Tile* tile, ScriptReader& script) const;

	SectorMap tiles;
	Matrix<uint32_t> creature_chain;

	friend class Game;
};

extern Map g_map;
