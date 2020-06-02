#include "pch.h"

#include "map.h"
#include "item.h"
#include "tools.h"
#include "config.h"
#include "creature.h"
#include "script.h"
#include "game.h"
#include "itempool.h"

template<typename T>
void Matrix<T>::init(int32_t xmin, int32_t xmax, int32_t ymin, int32_t ymax)
{
	dx = xmax - xmin + 1;
	dy = ymax - ymin + 1;

	this->xmin = xmin;
	this->ymin = ymin;
	this->xmax = xmax;
	this->ymax = ymax;

	if (dx <= 0 || dy <= 0) {
		fmt::printf("ERROR - Matrix::init: invalid array size (%d,%d)\n", dx, dy);
		return;
	}

	entry = new T[4 * dx * dy];
}

bool ShortwayPointEntryComparator::operator()(const ShortwayPoint* l, const ShortwayPoint* r) const
{
	return l->heuristic > r->heuristic;
}

Shortway::Shortway(Creature* creature)
{
	const Position& pos = creature->getPosition();
	startx = pos.x;
	starty = pos.y;
	startz = pos.z;
	this->creature = creature;

	points.init(pos.x - 11, pos.x + 11, pos.y - 11, pos.y + 11);
}

bool Shortway::calculate(int32_t destx, int32_t desty, bool must_reach)
{
	this->destx = destx;
	this->desty = desty;

	fillMap();

	open_points.push(start_point);

	while (!open_points.empty()) {
		ShortwayPoint* point = open_points.top();

		if (point->x == destx && point->y == desty) {
			end_point = point;
			break;
		}

		open_points.pop();

		const int32_t basicwaylength = point->waylength + point->waypoints;

		for (int32_t x = -1; x <= 1; x++) {
			for (int32_t y = -1; y <= 1; y++) {
				if (x == 0 && y == 0) {
					// center point
					continue;
				}

				// get the point
				const int32_t dx = point->x + x - points.xmin;
				const int32_t dy = point->y + y - points.ymin;

				if (dx >= 0 && dx < points.dx && dy >= 0 && dy < points.dy) {
					ShortwayPoint* neighbor = &points.entry[points.dx * dy + dx];
					if (basicwaylength > neighbor->waylength || neighbor->waypoints == -1) {
						continue;
					}

					if (!neighbor->open) {
						neighbor->open = true;
						open_points.push(neighbor);
					}

					int32_t waylength = basicwaylength;
					if (std::abs(x) + std::abs(y) != 1) {
						waylength = basicwaylength + 2 * point->waypoints;
					}

					if (neighbor->waylength > waylength) {
						neighbor->waylength = waylength;
						neighbor->parent = point;

						waylength = neighbor->waypoints + waylength;

						neighbor->heuristic = minwaypoints * (waylength + (std::abs(neighbor->x - destx) + std::abs(neighbor->y - desty)));
					}
				}
			}
		}
	}

	if (end_point == nullptr) {
		return false;
	}

	// reconstruct path
	std::deque<ShortwayPoint*> path;

	ShortwayPoint* next = end_point;
	while (next) {
		path.push_back(next);
		next = next->parent;
	}

	path.pop_back(); // pop first one

	while (!path.empty()) {
		next = path.back();
		path.pop_back();

		ToDoEntry entry;
		entry.code = TODO_GO;
		entry.to_pos.x = next->x;
		entry.to_pos.y = next->y;
		entry.to_pos.z = startz;
		creature->toDoAdd(entry);

		if (!must_reach && std::abs(next->x - destx) + 1 <= 2 && std::abs(next->y - desty) + 1 <= 2) {
			break;
		}
	}

	return true;
}

void Shortway::fillMap()
{
	for (int32_t x = points.xmin; x <= points.xmax; x++) {
		for (int32_t y = points.ymin; y <= points.ymax; y++) {
			const int32_t dx = x - points.xmin;
			const int32_t dy = y - points.ymin;

			if (dx >= 0 && dx < points.dx && dy >= 0 && dy < points.dy) {
				ShortwayPoint* point = &points.entry[points.dx * dy + dx];
				point->x = x;
				point->y = y;
				point->parent = nullptr;
				point->waypoints = -1;
				point->waylength = point->heuristic = std::numeric_limits<int32_t>::max();
				point->open = false;

				const Tile* tile = g_map.getTile(x, y, startz);
				if (tile == nullptr) {
					continue;
				}

				const Item* item = tile->getBankItem();
				if (item == nullptr) {
					continue;
				}

				if (x == startx && y == starty) {
					start_point = point;
					start_point->waylength = 0;
				}

				if (item->getFlag(UNPASS)) {
					continue;
				}

				int32_t waypoints = -1;

				const bool move_possible = tile->queryAdd(INDEX_ANYWHERE, creature, 1, FLAG_PATHFINDING, nullptr) == ALLGOOD;
				if (move_possible || (x == destx && y == desty || x == startx && y == starty)) {
					waypoints = item->getAttribute(WAYPOINTS);
				}

				if (waypoints > 0 && waypoints < minwaypoints) {
					minwaypoints = waypoints;
				}

				point->waypoints = waypoints;
			} else {
				fmt::printf("ERROR - Shortway::fillMap: invalid bounds.\n");
			}
		}
	}
}

Sector::~Sector()
{
	for (auto& row : tile) {
		for (auto tile : row) {
			delete tile;
		}
	}
}

void SectorMap::init(int32_t xmin, int32_t xmax, int32_t ymin, int32_t ymax, int32_t zmin, int32_t zmax)
{
	dx = xmax - xmin + 1;
	dy = ymax - ymin + 1;
	dz = zmax - zmin + 1;

	this->xmin = xmin;
	this->ymin = ymin;
	this->zmin = zmin;
	this->xmax = xmax;
	this->ymax = ymax;
	this->zmax = zmax;

	if (dx <= 0 || dy <= 0 || dz <= 0) {
		fmt::printf("ERROR - SectorMap::init: invalid array size (%d,%d,%d)\n", dx, dy, dz);
		return;
	}

	entry = new Sector[dz * dy * dx];
}

bool Map::loadMap()
{
	tiles.init(g_config.SectorXMin, g_config.SectorXMax, g_config.SectorYMin, g_config.SectorYMax, g_config.SectorZMin, g_config.SectorZMax);

	creature_chain.init(g_config.SectorXMin, g_config.SectorXMax + 1, g_config.SectorYMin, g_config.SectorYMax + 1);
	for (int32_t i = 0; i != 4 * creature_chain.dx * creature_chain.dy; i++) {
		creature_chain.entry[i] = 0;
	}

	std::vector<boost::filesystem::path> sectors;

#ifdef DEBUG
	getFilesInDirectory("origmap/", ".sec", sectors);
#else
	getFilesInDirectory("map/", ".sec", sectors);
#endif

	while (!sectors.empty()) {
		auto file = sectors.back();
		sectors.pop_back();

		fmt::printf(" > Loading sector %s (%d sectors left)\n", file.filename().string(), sectors.size());

		if (!loadSector(file.filename().string())) {
			return false;
		}
	}

	return true;
}

Tile* Map::getTile(int32_t x, int32_t y, int32_t z) const
{
	int32_t dx = (x / 32) - tiles.xmin;
	int32_t dy = (y / 32) - tiles.ymin;
	const int32_t dz = z - tiles.zmin;

	if (dx >= 0 && dx < tiles.dx && dy >= 0 && dy < tiles.dy && dz >= 0 && dz < tiles.dz) {
		Sector& sector = tiles.entry[dx + tiles.dy * tiles.dx * dz + tiles.dx * dy];
		dx = x % 32;
		dy = y % 32;
		return sector.tile[dx][dy];
	}

	return nullptr;
}

bool Map::isProtectionZone(const Position& pos) const
{
	const Tile* tile = getTile(pos);
	if (tile == nullptr) {
		return false;
	}

	return tile->isProtectionZone();
}

bool Map::getCoordinateFlag(const Position& pos, ItemFlags_t flag) const
{
	const Tile* tile = getTile(pos);
	if (tile == nullptr) {
		return false;
	}

	return tile->getFlag(flag);
}

bool Map::searchFreeField(const Creature* creature, int32_t x, int32_t y, int32_t z, int32_t distance) const
{
	Position pos(x, y, z);
	return searchFreeField(creature, pos, distance);
}

bool Map::searchFreeField(const Creature* creature, Position& pos, uint8_t distance) const
{
	int32_t direction = 4;
	int32_t dist = 0;
	int32_t dx = 0, dy = 0;

	while (true) {
		const Tile* tile = getTile(pos.x + dx, pos.y + dy, pos.z);
		if (tile != nullptr) {
			const ReturnValue_t ret = tile->queryAdd(INDEX_ANYWHERE, creature, 1, 0, nullptr);
			if (ret == ALLGOOD) {
				pos = tile->getPosition();
				return true;
			}
		}

		if (direction == 2) {
			if (!(dist + --dx)) {
				direction = 3;
			}
		} else if (direction > 2) {
			if (direction == 3) {
				if (++dy == dist) {
					direction = 4;
				}
			} else if (direction == 4 && ++dx == dist + 1) {
				direction = 1;
				dist = dx;
			}
		} else if (direction == 1 && !(dist + --dy)) {
			direction = 2;
		}

		if (dist > distance) {
			break;
		}
	}

	return false;
}

bool Map::throwPossible(const Position& from_pos, const Position& to_pos) const
{
	if (from_pos == to_pos) {
		return true;
	}

	Position start(from_pos.z > to_pos.z ? to_pos : from_pos);
	const Position destination(from_pos.z > to_pos.z ? from_pos : to_pos);

	const int8_t mx = start.x < destination.x ? 1 : start.x == destination.x ? 0 : -1;
	const int8_t my = start.y < destination.y ? 1 : start.y == destination.y ? 0 : -1;

	const int32_t A = std::abs(destination.y - start.y);
	const int32_t B = std::abs(start.x - destination.x);
	const int32_t C = -(A * destination.x + B * destination.y);

	while (start.x != destination.x || start.y != destination.y) {
		const int32_t move_hor = std::abs(A * (start.x + mx) + B * (start.y) + C);
		const int32_t move_ver = std::abs(A * (start.x) + B * (start.y + my) + C);
		const int32_t move_cross = std::abs(A * (start.x + mx) + B * (start.y + my) + C);

		if (start.y != destination.y && (start.x == destination.x || move_hor > move_ver || move_hor > move_cross)) {
			start.y += my;
		}

		if (start.x != destination.x && (start.y == destination.y || move_ver > move_hor || move_ver > move_cross)) {
			start.x += mx;
		}

		const Tile* tile = getTile(start.x, start.y, start.z);
		if (tile && tile->getFlag(UNTHROW)) {
			return false;
		}
	}

	return true;
}

bool Map::fieldPossible(const Position& pos, FieldType_t field_type) const
{
	Tile* tile = getTile(pos);
	if (tile == nullptr) {
		return false;
	}

	if (!tile->getFlag(BANK)) {
		return false;
	}

	for (Object* object : tile->objects) {
		if (field_type == FIELD_MAGIC_WALL && object->getCreature()) {
			return false;
		}

		if (Item* item = object->getItem()) {
			if (item->getFlag(UNLAY)) {
				return false;
			}

			if (item->getFlag(UNPASS)) {
				return false;
			}

			if (item->getFlag(AVOID) && item->getAttribute(AVOIDDAMAGETYPES) == 0) {
				return false;
			}
		}
	}

	return true;
}

Tile* Map::createTile(int32_t x, int32_t y, int32_t z) const
{
	int32_t dx = x - tiles.xmin;
	int32_t dy = y - tiles.ymin;
	const int32_t dz = z - tiles.zmin;

	if (dx >= 0 && dx < tiles.dx && dy >= 0 && dy < tiles.dy && dz >= 0 && dz < tiles.dz) {
		Sector& sector = tiles.entry[dx + tiles.dy * tiles.dx * dz + tiles.dx * dy];
		dx = x / 32;
		dy = y / 32;
		Tile* new_tile = new Tile();
		sector.tile[dx][dy] = new_tile;
		return new_tile;
	}

	return nullptr;
}

Sector* Map::getSector(int32_t x, int32_t y, int32_t z) const
{
	const int32_t dx = x - tiles.xmin;
	const int32_t dy = y - tiles.ymin;
	const int32_t dz = z - tiles.zmin;

	if (dx >= 0 && dx < tiles.dx && dy >= 0 && dy < tiles.dy && dz >= 0 && dz < tiles.dz) {
		return &tiles.entry[dx + tiles.dy * tiles.dx * dz + tiles.dx * dy];
	}

	return nullptr;
}

bool Map::loadSector(const std::string& filename) const
{
	std::ostringstream ss;

#ifdef DEBUG
	ss << "origmap/" << filename;
#else
	ss << "map/" << filename;
#endif

	ScriptReader script;
	if (!script.loadScript(ss.str())) {
		return false;
	}

	int32_t base_x, base_y, base_z;

	if (sscanf(filename.c_str(), "%d-%d-%d.sec", &base_x, &base_y, &base_z) < 3) {
		return true;
	}

	script.nextToken();
	while (script.canRead()) {
		if (script.getToken() == TOKEN_NUMBER) {
			const int32_t x = script.getNumber();
			script.readSymbol('-');
			const int32_t y = script.readNumber();

			Sector* sector = getSector(base_x, base_y, base_z);
			if (sector == nullptr) {
				script.error("sector is null");
				return false;
			}

			Tile* tile = new Tile();

			if (sector->tile[x][y]) {
				fmt::printf("INFO - Map::loadSector: tile already parsed (%d,%d) in sector file '%s'\n", x, y, filename);
				delete sector->tile[x][y];
			}

			sector->tile[x][y] = tile;

			tile->current_position.x = x + 32 * base_x;
			tile->current_position.y = y + 32 * base_y;
			tile->current_position.z = base_z;

			script.readSymbol(':');
			
			while (script.canRead()) {
				script.nextToken();
				if (script.getToken() == TOKEN_IDENTIFIER) {
					const std::string identifier = script.getIdentifier();
					if (identifier == "nologout") {
						tile->nologout_zone = true;
					} else if (identifier == "protectionzone") {
						tile->protection_zone = true;
					} else if (identifier == "refresh") {
						tile->refresh_zone = true;
					} else if (identifier == "content") {
						script.readSymbol('=');
						script.readSymbol('{');
						if (!loadContents(tile, script)) {
							return false;
						}
					} else {
						ss.clear();
						ss << "unknown identifier '" << identifier << '\'';
						script.error(ss.str());
						return false;
					}
					continue;
				} else if (script.getToken() == TOKEN_SPECIAL) {
					if (script.getSpecial() == ',') {
						continue;
					}
				}

				break;
			}
		} else {
			script.error("next map point expected");
			return false;
		}
	}
	return true;
}

bool Map::loadContents(Tile* tile, ScriptReader& script) const
{
	script.nextToken();
	while (script.canRead()) {
		if (script.getToken() == TOKEN_NUMBER) {
			const uint16_t type_id = script.getNumber();

			Item* new_item = g_itempool.createItem(type_id);
			if (new_item == nullptr) {
				script.error("unknown type id");
				return false;
			}

			tile->addObject(new_item, INDEX_ANYWHERE);

			if (!new_item->loadData(script)) {
				return false;
			}
		} else if (script.getToken() == TOKEN_SPECIAL) {
			if (script.getSpecial() == ',') {
				script.nextToken();
			} else if (script.getSpecial() == '}') {
				break;
			}
		} else {
			script.error("'}' or ',' expected");
			return false;
		}
	}
	return true;
}
