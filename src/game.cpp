#include "pch.h"

#include "game.h"
#include "creature.h"
#include "player.h"
#include "item.h"
#include "tile.h"
#include "map.h"
#include "protocol.h"
#include "config.h"
#include "channels.h"
#include "tools.h"
#include "magic.h"
#include "combat.h"
#include "party.h"
#include "vocation.h"
#include "itempool.h"

uint64_t getSystemMilliseconds()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

bool CreatureEntryComparator::operator()(const CreatureEntry& l, const CreatureEntry& r) const
{
	return l.interval > r.interval;
}

Game::~Game()
{
	for (auto it : stored_players) {
		delete it.second;
	}

	releaseObjects();
}

void Game::launchGame()
{
	using clock = std::chrono::steady_clock;
	game_state = GAME_RUNNING;

	fmt::print(">> Game-server is running (Pid={0})\n", std::this_thread::get_id());

	currentBeatMiliseconds = getSystemMilliseconds();
	while (game_state >= GAME_RUNNING) {
		clock::time_point next_time_point = clock::now() + std::chrono::milliseconds(g_config.Beat);

		const int64_t systemMillisecondsNow = getSystemMilliseconds();
		const int64_t delay = systemMillisecondsNow - serverMilliseconds();
		currentBeatMiliseconds = systemMillisecondsNow;

		// read data from connections
		receiveData();

		// move game
		advanceGame(delay);

		// send all data
		sendData();

		std::this_thread::sleep_until(next_time_point);
	}
}

void Game::shutdown()
{
	game_state = GAME_OFFLINE;
	releaseObjects();
}

void Game::setGameState(GameState_t new_state)
{
	game_state = new_state;

	if (game_state == GAME_OFFLINE) {
		for (Player* player : players) {
			if (player->connection_ptr) {
				player->connection_ptr->close();
			}
		}
	}
}

void Game::announceMissileEffect(const Position& from_pos, const Position& to_pos, uint8_t type)
{
	const int32_t dx = std::abs(to_pos.x - from_pos.x);
	const int32_t dy = std::abs(to_pos.y - from_pos.y);

	const int32_t radx = dx / 2 + 17;
	const int32_t rady = dy / 2 + 15;

	const int32_t x = (to_pos.x + from_pos.x) / 2;
	const int32_t y = (to_pos.y + from_pos.y) / 2;

	std::unordered_set<Creature*> spectator_list;
	getSpectators(spectator_list, x, y, radx, rady, true);
	
	for (Creature* creature : spectator_list) {
		if (Player* player = creature->getPlayer()) {
			if (!player->canSeePosition(from_pos) || !player->canSeePosition(to_pos)) {
				continue;
			}

			Protocol::sendMissile(player->connection_ptr, from_pos, to_pos, type);
		}
	}
}

void Game::announceGraphicalEffect(Object* object, uint8_t type)
{
	const Position& pos = object->getPosition();
	announceGraphicalEffect(pos.x, pos.y, pos.z, type);
}

void Game::announceGraphicalEffect(int32_t x, int32_t y, int32_t z, uint8_t type)
{
	std::unordered_set<Creature*> spectator_list;
	getSpectators(spectator_list, x, y, 16, 14, true);

	for (Creature* creature : spectator_list) {
		if (Player* player = creature->getPlayer()) {
			if (!player->canSeePosition(x, y, z)) {
				continue;
			}

			Protocol::sendGraphicalEffect(player->connection_ptr, x, y, z, type);
		}
	}
}

void Game::announceAnimatedText(const Position& pos, uint8_t color, const std::string& text)
{
	std::unordered_set<Creature*> spectator_list;
	getSpectators(spectator_list, pos.x, pos.y, 16, 14, true);

	for (Creature* creature : spectator_list) {
		if (Player* player = creature->getPlayer()) {
			if (!player->canSeePosition(pos) || player->getPosition().z != pos.z) {
				continue;
			}

			Protocol::sendAnimatedText(player->connection_ptr, pos, color, text);
		}
	}
}

void Game::announceChangedObject(Object* object, AnnounceType_t type)
{
	if (object->getParent()->getTile()) {
		announceChangedField(object, type);
	} else if (object->getParent()->getItem()) {
		announceChangedContainer(object, type);
	} else if (object->getParent()->getPlayer()) {
		announceChangedInventory(object, type);
	}
}

void Game::announceChangedField(Object* object, AnnounceType_t type)
{
	std::unordered_set<Creature*> spectator_list;
	const Position& pos = object->getPosition();

	getSpectators(spectator_list, pos.x, pos.y, 16, 14, true);

	for (Creature* creature : spectator_list) {
		if (Player* player = creature->getPlayer()) {
			if (!player->canSeePosition(pos)) {
				continue;
			}

			if (type == ANNOUNCE_CREATE) {
				if (player == object) {
					Protocol::sendInitGame(player->connection_ptr);
					Protocol::sendAmbiente(player->connection_ptr);
					Protocol::sendStats(player->connection_ptr);

					if (player->last_login_time > 0) {
						tm* t = localtime(&player->last_login_time);
						char time_data[256];
						strftime(time_data, 256, "%d. %b %Y %X %Z", t);
						Protocol::sendTextMessage(player->connection_ptr, 20, fmt::sprintf("Your last visit in Tibia: %s.", time_data));
					}

					announceGraphicalEffect(object, 11);
				} else {
					Protocol::sendAddField(player->connection_ptr, object);
				}
			} else if (type == ANNOUNCE_CHANGE) {
				Protocol::sendChangeField(player->connection_ptr, object);
			} else if (type == ANNOUNCE_DELETE) {
				Protocol::sendDeleteField(player->connection_ptr, object->getPosition(), object->getObjectIndex());
			}
		}
	}
}

void Game::announceChangedInventory(Object* object, AnnounceType_t type) const
{
	Player* player = object->getParent()->getPlayer();
	if (player == nullptr) {
		fmt::printf("INFO - Game::announceChangedInventory: player is null.\n");
		return;
	}

	Item* item = object->getItem();
	if (item == nullptr) {
		fmt::printf("ERROR - Game::announceChangedInventory: object is not an item.\n");
		return;
	}

	if (type == ANNOUNCE_CREATE || type == ANNOUNCE_CHANGE) {
		Protocol::sendSetInventory(player->connection_ptr, object->getObjectIndex(), item);
	} else if (type == ANNOUNCE_DELETE) {
		Protocol::sendDeleteInventory(player->connection_ptr, object->getObjectIndex());
	}
}

void Game::announceChangedContainer(Object* object, AnnounceType_t type)
{
	std::unordered_set<Creature*> spectator_list;
	const Position& pos = object->getPosition();

	getSpectators(spectator_list, pos.x, pos.y, 2, 2, true);

	for (Creature* creature : spectator_list) {
		if (Player* player = creature->getPlayer()) {
			Item* item = object->getItem();
			Item* parent_container = item->getParent()->getItem();
			const int32_t container_id = player->getContainerId(parent_container);

			if (container_id == -1) {
				continue;
			}

			if (type == ANNOUNCE_CREATE) {
				Protocol::sendCreateInContainer(player->connection_ptr, container_id, item);
			} else if (type == ANNOUNCE_CHANGE) {
				Protocol::sendChangeInContainer(player->connection_ptr, container_id, object->getObjectIndex(), item);
			} else if (type == ANNOUNCE_DELETE) {
				Protocol::sendDeleteInContainer(player->connection_ptr, container_id, object->getObjectIndex());
			}
		}
	}
}

void Game::announceMovingCreature(Creature* creature, const Position& from_pos, int32_t from_index, const Position& to_pos, int32_t to_index)
{
	const int32_t dx = std::abs(to_pos.x - from_pos.x);
	const int32_t dy = std::abs(to_pos.y - from_pos.y);

	const int32_t radx = dx / 2 + 17;
	const int32_t rady = dy / 2 + 15;

	const int32_t centerx = (to_pos.x + from_pos.x) / 2;
	const int32_t centery = (to_pos.y + from_pos.y) / 2;

	std::unordered_set<Creature*> spectator_list;
	getSpectators(spectator_list, centerx, centery, radx, rady, false);

	for (Creature* spectator : spectator_list) {
		if (creature == spectator) {
			continue;
		}

		spectator->onCreatureMove(creature);
		Protocol::sendMoveCreature(spectator->connection_ptr, creature, from_pos, from_index, to_pos, to_index);
	}
}

void Game::announceChangedCreature(Creature* creature, CreatureChangeType_t type)
{
	std::unordered_set<Creature*> spectator_list;
	const Position& pos = creature->getPosition();

	getSpectators(spectator_list, pos.x, pos.y, 16, 14, true);

	for (Creature* spectator : spectator_list) {
		if (!spectator->canSeePosition(spectator->getPosition())) {
			return;
		}

		if (Player* player = spectator->getPlayer()) {
			switch (type) {
				case CREATURE_OUTFIT:
					Protocol::sendCreatureOutfit(player->connection_ptr, creature);
					break;
				case CREATURE_HEALTH:
					Protocol::sendCreatureHealth(player->connection_ptr, creature);
					break;
				case CREATURE_LIGHT: 
					Protocol::sendCreatureLight(player->connection_ptr, creature);
					break;
				case CREATURE_SPEED: 
					Protocol::sendCreatureSpeed(player->connection_ptr, creature);
					break;
				case CREATURE_SKULL: 
					Protocol::sendCreatureSkull(player->connection_ptr, creature);
					break;
				case CREATURE_PARTY: 
					Protocol::sendCreatureParty(player->connection_ptr, creature);
					break;
				default: break;
			}
		}
	}
}

void Game::getSpectators(std::unordered_set<Creature*>& spectator_list, int32_t centerx, int32_t centery, int32_t rangex, int32_t rangey, bool only_players)
{
	for (int32_t x = centerx - rangex; x <= centerx + rangex; x++) {
		for (int32_t y = centery - rangey; y <= centery + rangey; y++) {
			const int32_t block_id = g_map.creature_chain.dx * ((y / 32 + y % 32) - g_map.creature_chain.ymin) + (x / 32 + x %
				32) - g_map.creature_chain.xmin;
			const int32_t first_creature_entry = g_map.creature_chain.entry[block_id];

			if (first_creature_entry <= 0) {
				continue;
			}

			Creature* creature = getCreatureById(first_creature_entry);
			if (!creature) {
				continue;
			}

			if (only_players && creature->getPlayer()) {
				spectator_list.insert(creature);
			} else if (!only_players) {
				spectator_list.insert(creature);
			}

			while (true) {
				creature = creature->next_chain_creature;
				if (!creature) {
					break;
				}

				if (only_players && creature->getPlayer()) {
					spectator_list.insert(creature);
				} else if (!only_players) {
					spectator_list.insert(creature);
				}
			}
		}
	}
}

void Game::getAmbiente(uint8_t& brightness, uint8_t& color) const
{
	time_t timer = time(nullptr);
	struct tm *v2 = localtime(&timer);
	const int v3 = v2->tm_sec + 60 * v2->tm_min;
	const int v4 = 2 * (v3 % 150) / 5 + 60 * (v3 / 150);

	if (v4 <= 59) {
		brightness = 51;
		color = 215;
		return;
	}
	if (v4 <= 119) {
		brightness = 102;
		color = 215;
		return;
	}
	if (v4 <= 179) {
		brightness = 153;
		color = 173;
		return;
	}
	if (v4 <= 239) {
		brightness = 204;
		color = 173;
		return;
	}
	if (v4 > 1380) {
		brightness = 51;
		color = 215;
		return;
	}
	if (v4 > 1320) {
		brightness = 102;
		color = 215;
		return;
	}
	if (v4 <= 1260) {
		if (v4 <= 1200) {
			brightness = 255;
			color = 215;
			return;
		}
		brightness = 204;
		color = 208;
		return;
	}
	
	brightness = 153;
	color = 208;
}

void Game::closeContainers(Item* item, bool force)
{
	std::unordered_set<Creature*> spectator_list;
	const Position& pos = item->getPosition();

	getSpectators(spectator_list, pos.x, pos.y, 12, 10, true);

	for (Creature* creature : spectator_list) {
		if (Player* player = creature->getPlayer()) {
			Player* holding_player = item->getHoldingPlayer();

			const Position& player_pos = player->getPosition();
			const int32_t dx = std::abs(pos.x - player_pos.x);
			const int32_t dy = std::abs(pos.y - player_pos.y);

			if (force || holding_player && holding_player != player || dx > 1 || dy > 1 || pos.z != player_pos.z) {
				std::vector<uint32_t> close_container_ids;
				for (const auto& it : player->open_containers) {
					Item* aux_item = it.second;
					while (aux_item) {
						if (aux_item == item) {
							close_container_ids.push_back(it.first);
						}

						aux_item = aux_item->getParent()->getItem();
					}
				}

				for (uint32_t container_id : close_container_ids) {
					player->closeContainer(container_id);
					Protocol::sendCloseContainer(player->connection_ptr, container_id);
				}
			} else {
				for (const auto& it : player->open_containers) {
					if (it.second == item) {
						Protocol::sendContainer(player->connection_ptr, it.first, item);
					}
				}
			}
		}
	}
}

uint64_t Game::serverMilliseconds() const
{
	return currentBeatMiliseconds;
}

void Game::addConnection(Connection_ptr connection)
{
	connection_mutex.lock();
	connections.push_back(connection);
	connection_mutex.unlock();
}

void Game::removeConnection(Connection_ptr connection)
{
	connection_mutex.lock();
	const auto it = std::find(connections.begin(), connections.end(), connection);
	if (it == connections.end()) {
		fmt::printf("ERROR - Game::removeConnection: connection not found.\n");
		return;
	}

	connections.erase(it);
	connection_mutex.unlock();
}

void Game::decayItem(Item* item)
{
	if (item->decaying) {
		fmt::printf("INFO - Game::decayItem: item %d is already decaying.\n", item->getId());
		return;
	}

	item->decaying = true;
	decaying_list[random(0, DECAY_SIZE - 1)].push_back(item);
}

void Game::stopDecay(Item* item) const
{
	if (!item->decaying) {
		fmt::printf("INFO - Game::stopDecay: item %d is not decaying.\n", item->getId());
		return;
	}

	item->decaying = false;

	for (Item* container_item : item->getItems()) {
		stopDecay(container_item);
	}
}

void Game::moveAllObjects(Tile* tile, Tile* to_tile, Object* ignore_object, bool move_unmovable)
{
	std::vector<Object*> moving_objects;
	for (Object* object : tile->getObjects()) {
		if (object == ignore_object) {
			continue;
		}

		if (Item* item = object->getItem()) {
			if (item->getFlag(LIQUIDPOOL) || item->getFlag(MAGICFIELD)) {
				moving_objects.push_back(object);
				continue;
			}

			if (move_unmovable || !item->getFlag(UNMOVE)) {
				moving_objects.push_back(object);
			}
		} else if (object->getCreature()) {
			moving_objects.push_back(object);
		}
	}

	for (Object* object : moving_objects) {
		if (Item* item = object->getItem()) {
			if (item->getFlag(LIQUIDPOOL) || item->getFlag(MAGICFIELD)) {
				removeItem(item, 1);
				releaseItem(item);
				continue;
			}

			moveItem(item, item->getAttribute(ITEM_AMOUNT), tile, to_tile, INDEX_ANYWHERE, nullptr, FLAG_NOLIMIT);
		} else if (Creature* creature = object->getCreature()) {
			moveCreature(creature, to_tile, FLAG_NOLIMIT);
		}
	}
}

void Game::clearTile(const Position& pos, Object* ignore_object, ItemFlags_t item_flag)
{
	Tile* from_tile = g_map.getTile(pos.x, pos.y, pos.z);

	Tile* to_tile = g_map.getTile(pos.x + 1, pos.y, pos.z);
	if (to_tile && to_tile->getFlag(BANK)) {
		if (!to_tile->getFlag(item_flag)) {
			moveAllObjects(from_tile, to_tile, ignore_object, false);
			return;
		}
	}

	to_tile = g_map.getTile(pos.x, pos.y + 1, pos.z);
	if (to_tile && to_tile->getFlag(BANK)) {
		if (!to_tile->getFlag(item_flag)) {
			moveAllObjects(from_tile, to_tile, ignore_object, false);
			return;
		}
	}

	to_tile = g_map.getTile(pos.x - 1, pos.y, pos.z);
	if (to_tile && to_tile->getFlag(BANK)) {
		if (!to_tile->getFlag(item_flag)) {
			moveAllObjects(from_tile, to_tile, ignore_object, false);
			return;
		}
	}

	to_tile = g_map.getTile(pos.x, pos.y - 1, pos.z);
	if (to_tile && to_tile->getFlag(BANK)) {
		if (!to_tile->getFlag(item_flag)) {
			moveAllObjects(from_tile, to_tile, ignore_object, false);
		}
	}
}

void Game::changeItem(Item* item, uint16_t type_id, uint32_t value)
{
	const ItemType* item_type = g_items.getItemType(type_id);
	if (item_type == nullptr) {
		fmt::printf("ERROR - Game::changeItem: invalid type id (%d).\n", type_id);
		return;
	}

	const ItemType* old_item_type = item->item_type;

	if (old_item_type->getFlag(CONTAINER) && !item_type->getFlag(CONTAINER)) {
		closeContainers(item, true);

		// delete container content
		for (Item* aux_item : item->items) {
			releaseItem(aux_item);
		}
		item->items.clear();
	}

	if (item_type->getFlag(CUMULATIVE) && !old_item_type->getFlag(CUMULATIVE)) {
		item->setAttribute(ITEM_AMOUNT, value);
	}

	if (item_type->getFlag(MAGICFIELD) && !old_item_type->getFlag(MAGICFIELD)) {
		item->setAttribute(ITEM_RESPONSIBLE, value);
	}

	if (item_type->getFlag(LIQUIDPOOL) && !old_item_type->getFlag(LIQUIDPOOL)) {
		item->setAttribute(ITEM_LIQUID_TYPE, value);
	}

	if (item_type->getFlag(LIQUIDCONTAINER) && !old_item_type->getFlag(LIQUIDCONTAINER)) {
		item->setAttribute(ITEM_LIQUID_TYPE, value);
	}

	if (item_type->getFlag(KEY) && !old_item_type->getFlag(KEY)) {
		item->setAttribute(ITEM_KEY_NUMBER, value);
	}

	if (item_type->getFlag(RUNE) && !old_item_type->getFlag(RUNE)) {
		item->setAttribute(ITEM_CHARGES, value);
	}

	if (old_item_type->getFlag(EXPIRE) && !item_type->getFlag(EXPIRE)) {
		item->setAttribute(ITEM_SAVED_EXPIRE_TIME, item->getAttribute(ITEM_REMAINING_EXPIRE_TIME));
		item->setAttribute(ITEM_REMAINING_EXPIRE_TIME, 0);
		stopDecay(item);
	}

	if (old_item_type->getFlag(EXPIRESTOP) && item_type->getFlag(EXPIRE)) {
		if (item->getAttribute(ITEM_SAVED_EXPIRE_TIME) <= 0) {
			item->setAttribute(ITEM_REMAINING_EXPIRE_TIME, item_type->getAttribute(TOTALEXPIRETIME) * 1000);
		} else if (item->getAttribute(ITEM_SAVED_EXPIRE_TIME) > 0) {
			item->setAttribute(ITEM_REMAINING_EXPIRE_TIME, item->getAttribute(ITEM_SAVED_EXPIRE_TIME));
		}

		item->setAttribute(ITEM_SAVED_EXPIRE_TIME, 0);
		decayItem(item);
	} else if (item_type->getFlag(EXPIRE)) {
		if (item->getAttribute(ITEM_SAVED_EXPIRE_TIME) <= 0) {
			item->setAttribute(ITEM_REMAINING_EXPIRE_TIME, item_type->getAttribute(TOTALEXPIRETIME) * 1000);
		}

		item->setAttribute(ITEM_SAVED_EXPIRE_TIME, 0);

		if (!item->decaying) {
			decayItem(item);
		}
	}

	item->item_type = item_type;

	announceChangedObject(item, ANNOUNCE_CHANGE);

	if (Player* player = item->getHoldingPlayer()) {
		const int32_t index = player->getObjectIndex(item);
		const bool inventory = index >= INVENTORY_HEAD && index <= INVENTORY_EXTRA;
		notifyPlayer(player, item, inventory);
	}

	notifyTrades(item);
}

void Game::queueCreature(Creature* creature)
{
	if (creature->is_dead || creature->isRemoved()) {
		return;
	}

	CreatureEntry entry;
	entry.interval = creature->next_wakeup;
	entry.creature = creature;
	creature_queue.push(entry);
}

void Game::addCreatureSkillCheck(Creature* creature)
{
	if (creature->skill_processing) {
		return;
	}

	creature->skill_processing = true;
	creature_skills[random(0, CREATURE_SKILL_SIZE - 1)].push_back(creature);
}

void Game::addPlayerList(Player* player)
{
	players.push_back(player);
}

void Game::addCreatureList(Creature* creature)
{
	creatures.push_back(creature);
}

void Game::storePlayer(Player* player, uint32_t user_id)
{
	const auto it = stored_players.find(user_id);
	if (it != stored_players.end()) {
		fmt::printf("ERROR - Game::storePlayer: user already stored (%d)\n", user_id);
		return;
	}

	player->stored = true;
	stored_players[user_id] = player;

	indexPlayer(player);
}

void Game::removePlayerList(Player * player)
{
	const auto it = std::find(players.begin(), players.end(), player);
	if (it == players.end()) {
		fmt::printf("ERROR - Game::removePlayerList: %s not found in list.\n", player->getName());
	} else {
		players.erase(it);
	}
}

void Game::removeCreatureList(Creature * creature)
{
	const auto it = std::find(creatures.begin(), creatures.end(), creature);
	if (it == creatures.end()) {
		fmt::printf("ERROR - Game::removeCreature: %s not found in list.\n", creature->getName());
	} else {
		creatures.erase(it);
	}
}

void Game::removeCreature(Creature* creature)
{
	announceChangedObject(creature, ANNOUNCE_DELETE);

	removeChainCreature(creature);

	Tile* tile = creature->getParent()->getTile();
	tile->removeObject(creature);

	if (creature->creature_type != CREATURE_PLAYER) {
		releaseCreature(creature);
	}

	creature->onDelete();
}

void Game::removeItem(Item* item, uint8_t amount)
{
	if (item->getAttribute(ITEM_AMOUNT) - amount <= 0) {
		announceChangedObject(item, ANNOUNCE_DELETE);

		if (Player* player = item->getHoldingPlayer()) {
			const int32_t index = player->getObjectIndex(item);
			const bool inventory = index >= INVENTORY_HEAD && index <= INVENTORY_EXTRA;
			notifyPlayer(player, item, inventory);
		}

		Cylinder* parent = item->getParent();
		parent->removeObject(item);
		releaseItem(item);
	} else {
		item->setAttribute(ITEM_AMOUNT, item->getAttribute(ITEM_AMOUNT) - amount);
		announceChangedObject(item, ANNOUNCE_CHANGE);
	}
}

ReturnValue_t Game::setCreatureOnMap(Creature* creature, int32_t x, int32_t y, int32_t z)
{
	Tile* tile = g_map.getTile(x, y, z);
	if (!tile) {
		fmt::printf("WARNING - Game::setCreatureOnMap: tile does not exist (%d,%d,%d)\n", x, y, z);
		return NOTPOSSIBLE;
	}

	const ReturnValue_t ret = tile->queryAdd(INDEX_ANYWHERE, creature, 1, FLAG_NOLIMIT, nullptr);
	if (ret != ALLGOOD) {
		return ret;
	}

	creature->onCreate();

	tile->addObject(creature, INDEX_ANYWHERE);

	insertChainCreature(creature, x, y);
	announceChangedObject(creature, ANNOUNCE_CREATE);
	return ALLGOOD;
}

ReturnValue_t Game::moveCreature(Creature* creature, Tile* to_tile, uint32_t flags)
{
	const Position& pos = to_tile->getPosition();
	return moveCreature(creature, pos.x, pos.y, pos.z, flags);
}

ReturnValue_t Game::moveCreature(Creature* creature, int32_t x, int32_t y, int32_t z, uint32_t flags)
{
	Tile* tile = g_map.getTile(x, y, z);
	if (!tile) {
		fmt::printf("ERROR - Game::moveCreature: tile does not exist (%d,%d,%d)\n", x, y, z);
		return NOTPOSSIBLE;
	}

	const ReturnValue_t ret = tile->queryAdd(INDEX_ANYWHERE, creature, 1, flags, nullptr);
	if (ret != ALLGOOD) {
		if (creature->creature_type == CREATURE_PLAYER) {
			Protocol::sendResult(creature->connection_ptr, ret);
			Protocol::sendSnapback(creature->connection_ptr);
		}
		return ret;
	}

	const int32_t from_index = creature->getObjectIndex();
	Tile* from_tile = creature->parent->getTile();

	moveChainCreature(creature, x, y);

	from_tile->removeObject(creature);
	tile->addObject(creature, INDEX_ANYWHERE);

	const int32_t to_index = creature->getObjectIndex();

	creature->onWalk(from_tile->getPosition(), from_index, tile->getPosition(), to_index);
	announceMovingCreature(creature, from_tile->getPosition(), from_index, tile->getPosition(), to_index);
	return ret;
}

ReturnValue_t Game::moveItem(Item* item, uint8_t amount, Cylinder* from_cylinder, Cylinder* to_cylinder, int32_t to_index, Creature* actor, uint32_t flags)
{
	Item* to_item = nullptr;
	to_cylinder = to_cylinder->queryDestination(to_index, item, &to_item, flags);

	if (item == to_item) {
		return ALLGOOD;
	}

	const int32_t from_index = from_cylinder->getObjectIndex(item);

	ReturnValue_t ret = to_cylinder->queryAdd(to_index, item, amount, flags, actor);
	
	// player inventory exchanging items
	if (ret == NEEDEXCHANGE) {
		ret = from_cylinder->queryAdd(from_cylinder->getObjectIndex(item), to_item, to_item->getAttribute(ITEM_AMOUNT), flags, actor);
		if (ret == ALLGOOD) {
			uint32_t max_exchange_amount = 0;

			const ReturnValue_t ret_max_amount = from_cylinder->queryMaxCount(INDEX_ANYWHERE, to_item, to_item->getAttribute(ITEM_AMOUNT), max_exchange_amount, flags);
			if (ret_max_amount != ALLGOOD && max_exchange_amount == 0) {
				return ret_max_amount;
			}

			announceChangedObject(to_item, ANNOUNCE_DELETE);
			to_cylinder->removeObject(to_item);
			from_cylinder->addObject(to_item, from_cylinder->getObjectIndex(item));
			announceChangedObject(to_item, ANNOUNCE_CREATE);

			ret = to_cylinder->queryAdd(to_index, item, amount, flags, actor);
			to_item = nullptr;
		}
	}

	if (ret != ALLGOOD) {
		return ret;
	}

	uint32_t max_amount = 0;

	const ReturnValue_t ret_max_amount = to_cylinder->queryMaxCount(to_index, item, amount, max_amount, flags);
	if (ret_max_amount != ALLGOOD && max_amount == 0) {
		return ret_max_amount;
	}

	notifyTrades(item);

	uint32_t m;
	if (item->getFlag(CUMULATIVE)) {
		m = std::min<uint32_t>(amount, max_amount);
	} else {
		m = max_amount;
	}

	if (item->getFlag(CUMULATIVE)) {
		if (item->getAttribute(ITEM_AMOUNT) - m <= 0) {
			announceChangedObject(item, ANNOUNCE_DELETE);
			from_cylinder->removeObject(item);
			releaseItem(item);
		} else {
			item->setAttribute(ITEM_AMOUNT, item->getAttribute(ITEM_AMOUNT) - m);
			announceChangedObject(item, ANNOUNCE_CHANGE);
		}

		uint32_t n;
		if (to_item && item->getId() == to_item->getId()) {
			n = std::min<uint32_t>(100 - to_item->getAttribute(ITEM_AMOUNT), m);
			to_item->setAttribute(ITEM_AMOUNT, to_item->getAttribute(ITEM_AMOUNT) + n);
			announceChangedObject(to_item, ANNOUNCE_CHANGE);
		} else {
			n = 0;
		}

		const int32_t new_count = m - n;
		if (new_count > 0) {
			Item* new_item = g_itempool.createItem(item->getId());
			new_item->setAttribute(ITEM_AMOUNT, new_count);
			to_cylinder->addObject(new_item, to_index);
			announceChangedObject(new_item, ANNOUNCE_CREATE);
		}
	} else {
		announceChangedObject(item, ANNOUNCE_DELETE);
		from_cylinder->removeObject(item);
		to_cylinder->addObject(item, to_index);
		announceChangedObject(item, ANNOUNCE_CREATE);

		if (item->getFlag(CONTAINER)) {
			closeContainers(item, false);
		}
	}

	Player* notify_player = from_cylinder->getPlayer();
	if (notify_player == nullptr) {
		notify_player = to_cylinder->getPlayer();
	}

	if (notify_player == nullptr) {
		notify_player = from_cylinder->getHoldingPlayer();
		if (notify_player == nullptr) {
			notify_player = to_cylinder->getHoldingPlayer();
		}
	}

	if (notify_player) {
		const bool inventory = to_index >= INVENTORY_HEAD && to_index <= INVENTORY_EXTRA || from_index >= INVENTORY_HEAD && from_index <= INVENTORY_EXTRA;
		notifyPlayer(notify_player, item, inventory);
	}

	notifyTrades(item);

	if (item->getFlag(CUMULATIVE) && max_amount < amount) {
		return ret_max_amount;
	}

	return ret;
}

ReturnValue_t Game::canPlayerCarryItem(Player* player, const Item* item, uint32_t flags) const
{
	int32_t index = INDEX_ANYWHERE;
	Item* to_item = nullptr;

	Cylinder* cylinder = player->queryDestination(index, item, &to_item, flags);

	uint32_t max_amount = 0;

	const ReturnValue_t ret = cylinder->queryMaxCount(index, item, item->getAttribute(ITEM_AMOUNT), max_amount, flags);
	if (ret != ALLGOOD) {
		return ret;
	}

	return cylinder->queryAdd(index, item, item->getAttribute(ITEM_AMOUNT), flags);
}

ReturnValue_t Game::addItem(Item* item, Cylinder* to_cylinder)
{
	uint32_t flags = 0;
	int32_t index = INDEX_ANYWHERE;
	Item* to_item = nullptr;

	Cylinder* dest_cylinder = to_cylinder->queryDestination(index, item, &to_item, flags);

	ReturnValue_t ret = dest_cylinder->queryAdd(index, item, item->getAttribute(ITEM_AMOUNT), flags, nullptr);
	if (ret != ALLGOOD) {
		return ret;
	}

	uint32_t max_amount = 0;
	ret = dest_cylinder->queryMaxCount(INDEX_ANYWHERE, item, item->getAttribute(ITEM_AMOUNT), max_amount, flags);

	if (ret != ALLGOOD) {
		return ret;
	}

	if (item->getFlag(CUMULATIVE) && to_item && to_item->getId() == item->getId()) {
		const uint32_t m = std::min<uint32_t>(item->getAttribute(ITEM_AMOUNT), max_amount);
		const uint32_t n = std::min<uint32_t>(100 - to_item->getAttribute(ITEM_AMOUNT), m);

		to_item->setAttribute(ITEM_AMOUNT, to_item->getAttribute(ITEM_AMOUNT) + n);
		announceChangedObject(to_item, ANNOUNCE_CHANGE);

		const int32_t count = m - n;
		if (count > 0) {
			if (item->getAttribute(ITEM_AMOUNT) != count) {
				Item* remainder_item = g_itempool.createItem(item->getId());
				remainder_item->setAttribute(ITEM_AMOUNT, count);
				if (addItem(remainder_item, dest_cylinder) != ALLGOOD) {
					releaseItem(remainder_item);
				}
			} else {
				dest_cylinder->addObject(item, index);
				announceChangedObject(item, ANNOUNCE_CREATE);
			}
		} else {
			releaseItem(item);
		}
	} else {
		dest_cylinder->addObject(item, index);
		announceChangedObject(item, ANNOUNCE_CREATE);
	}

	Player* notify_player = dest_cylinder->getPlayer();
	if (notify_player == nullptr) {
		notify_player = dest_cylinder->getPlayer();
	}

	if (notify_player == nullptr) {
		notify_player = dest_cylinder->getHoldingPlayer();
		if (notify_player == nullptr) {
			notify_player = dest_cylinder->getHoldingPlayer();
		}
	}

	if (notify_player) {
		const bool inventory = index >= INVENTORY_HEAD && index <= INVENTORY_EXTRA;
		notifyPlayer(notify_player, item, inventory);
	}

	return ALLGOOD;
}

Item* Game::createItemAtPlayer(Player* player, const ItemType* item_type, uint32_t value)
{
	if (player == nullptr) {
		fmt::printf("ERROR - Game::createItemAtPlayer: player is null.\n");
		return nullptr;
	}

	if (item_type == nullptr) {
		fmt::printf("ERROR - Game::createItemAtPlayer: item type is null.\n");
		return nullptr;
	}

	Item* item = g_itempool.createItem(item_type->type_id);
	if (item->getFlag(CUMULATIVE)) {
		item->setAttribute(ITEM_AMOUNT, value);
	}

	if (item->getFlag(RUNE)) {
		item->setAttribute(ITEM_CHARGES, g_magic.getMagicItemCharges(item->getId()));
	}

	if (addItem(item, player) != ALLGOOD) {
		if (addItem(item, player->getParent()) != ALLGOOD) {
			fmt::printf("ERROR - Game::createItemAtPlayer: failed to create item on player (%d).\n", item_type->type_id);
			releaseItem(item);
			return nullptr;
		}
	}

	return item;
}

Item* Game::createLiquidPool(const Position& pos, const ItemType* item_type, uint8_t liquid_type)
{
	if (item_type == nullptr) {
		fmt::printf("ERROR - Game::createLiquidPool: item type is null.\n");
		return nullptr;
	}

	Item* item = g_itempool.createItem(item_type->type_id);
	item->setAttribute(ITEM_LIQUID_TYPE, liquid_type);

	if (addItem(item, g_map.getTile(pos)) != ALLGOOD) {
		releaseItem(item);
		return nullptr;
	}

	return item;
}

Item* Game::createField(const Position& pos, FieldType_t field, uint32_t owner, bool peaceful)
{
	const ItemType* item_type = nullptr;
	switch (field) {
		case FIELD_FIRE: {
			if (peaceful) {
				item_type = g_items.getSpecialItem(SPECIAL_MAGICFIELD_FIRE_HARMLESS);
			} else {
				item_type = g_items.getSpecialItem(SPECIAL_MAGICFIELD_FIRE_DANGEROUS);
			}
			break;
		}
		case FIELD_POISON: {
			if (peaceful) {
				item_type = g_items.getSpecialItem(SPECIAL_MAGICFIELD_POISON_HARMLESS);
			} else {
				item_type = g_items.getSpecialItem(SPECIAL_MAGICFIELD_POISON_DANGEROUS);
			}
			break;
		}
		case FIELD_ENERGY: {
			if (peaceful) {
				item_type = g_items.getSpecialItem(SPECIAL_MAGICFIELD_ENERGY_HARMLESS);
			} else {
				item_type = g_items.getSpecialItem(SPECIAL_MAGICFIELD_ENERGY_DANGEROUS);
			}
			break;
		}
		case FIELD_MAGIC_WALL: {
			item_type = g_items.getSpecialItem(SPECIAL_MAGICFIELD_MAGICWALL);
			break;
		}
		case FIELD_WILD_GROWTH: {
			item_type = g_items.getSpecialItem(SPECIAL_MAGICFIELD_RUSHWOOD);
			break;
		}
		default: break;
	}

	if (item_type == nullptr) {
		fmt::printf("ERROR - Game::createField: item does not exists (%d)\n", field);
		return nullptr;
	}

	Item* item = g_itempool.createItem(item_type->type_id);
	item->setAttribute(ITEM_RESPONSIBLE, owner);

	if (addItem(item, g_map.getTile(pos)) != ALLGOOD) {
		releaseItem(item);
		return nullptr;
	}

	if (!peaceful) {
		Player* player = getPlayerByUserId(owner);
		if (player) {
			player->blockLogout(60, true);
		}
	}

	return item;
}

void Game::massCreateField(Creature* creature, const Position& pos, uint8_t radius, FieldType_t type)
{
	announceMissileEffect(creature->getPosition(), pos, (type != 1) + 4);

	AreaCombat area_combat;
	area_combat.setupArea(radius);

	std::forward_list<Tile*> tiles;
	area_combat.getList(pos, pos, tiles);

	for (Tile* tile : tiles) {
		if (tile->isProtectionZone() || !g_map.throwPossible(pos, tile->getPosition()) || !g_map.fieldPossible(tile->getPosition(), type)) {
			continue;
		}

		createField(tile->getPosition(), type, creature->getId(), false);
	}
}

void Game::deleteField(const Position& pos)
{
	Tile* tile = g_map.getTile(pos);
	if (tile == nullptr) {
		fmt::printf("ERROR - Game::deleteField: tile does not exist (%d,%d,%d)\n", pos.x, pos.y, pos.z);
		return;
	}

	Item* field = tile->getMagicFieldItem();
	if (field) {
		g_game.removeItem(field, 1);
	}

	g_game.announceGraphicalEffect(pos.x, pos.y, pos.z, 3);
}

void Game::cleanUpTile(Tile* tile)
{
	if (tile == nullptr) {
		fmt::printf("ERROR - Game::cleanUpTile: tile is null.\n");
		return;
	}

	std::vector<Item*> items;
	for (Object* object : tile->getObjects()) {
		if (Item* item = object->getItem()) {
			if (item->getFlag(UNMOVE)) {
				continue;
			}

			if (!item->getFlag(CORPSE) || item->getAttribute(CORPSETYPE)) {
				items.push_back(item);
			}
		}
	}

	for (Item* item : items) {
		removeItem(item, item->getAttribute(ITEM_AMOUNT));
	}

	announceGraphicalEffect(tile, 3);
}

int32_t Game::identifyPlayer(Player** player, bool exact_match, const std::string& name) const
{
	if (name.empty()) {
		return 0;
	}

	std::string test_name = name;

	if (!exact_match) {
		if (name[name.length() - 1] != '~') {
			exact_match = true;
		} else {
			test_name = test_name.substr(0, test_name.length() - 1);
		}
	}

	int32_t hits = 0;
	for (Player* p : players) {
		*player = p;
		
		std::string p_name = lowerString(p->name);

		if (p_name.compare(lowerString(test_name)) == 0) {
			hits++;
			break;
		}

		if (!exact_match) {
			if (p_name.compare(0, test_name.size(), test_name) == 0) {
				hits++;
			}
		}
	}

	return hits;
}

Player* Game::findPerson(Player* player, bool exact_match, const std::string& name)
{
	Player* found_player = nullptr;
	int32_t hits = identifyPlayer(&found_player, false, name);
	if (hits == 0) {
		Protocol::sendResult(player->connection_ptr, PLAYERWITHTHISNAMEISNOTONLINE);
		return nullptr;
	}

	if (hits > 1) {
		Protocol::sendResult(player->connection_ptr, NAMEISTOOAMBIGIOUS);
		return nullptr;
	}

	enum distance_t 
	{
		DISTANCE_BESIDE,
		DISTANCE_CLOSE,
		DISTANCE_FAR,
		DISTANCE_VERYFAR,
	};

	enum direction_t 
	{
		DIR_N, DIR_S, DIR_E, DIR_W,
		DIR_NE, DIR_NW, DIR_SE, DIR_SW,
	};

	enum level_t 
	{
		LEVEL_HIGHER,
		LEVEL_LOWER,
		LEVEL_SAME,
	};

	const Position& my_pos = player->getPosition();
	const Position& target_pos = found_player->getPosition();

	int32_t dx = Position::getOffsetX(my_pos, target_pos);
	int32_t dy = Position::getOffsetY(my_pos, target_pos);
	int32_t dz = Position::getOffsetZ(my_pos, target_pos);
	
	distance_t distance;
	direction_t direction;
	level_t level;

	//getting floor
	if (dz > 0) {
		level = LEVEL_HIGHER;
	} else if (dz < 0) {
		level = LEVEL_LOWER;
	} else {
		level = LEVEL_SAME;
	}

	//getting distance
	if (std::abs(dx) < 4 && std::abs(dy) < 4) {
		distance = DISTANCE_BESIDE;
	} else {
		int32_t distance2 = dx * dx + dy * dy;
		if (distance2 < 10000) {
			distance = DISTANCE_CLOSE;
		} else if (distance2 < 75076) {
			distance = DISTANCE_FAR;
		} else {
			distance = DISTANCE_VERYFAR;
		}
	}

	//getting direction
	float tan;
	if (dx != 0) {
		tan = static_cast<float>(dy) / dx;
	} else {
		tan = 10.;
	}

	if (std::abs(tan) < 0.4142) {
		if (dx > 0) {
			direction = DIR_W;
		} else {
			direction = DIR_E;
		}
	} else if (std::abs(tan) < 2.4142) {
		if (tan > 0) {
			if (dy > 0) {
				direction = DIR_NW;
			} else {
				direction = DIR_SE;
			}
		} else {
			if (dx > 0) {
				direction = DIR_SW;
			} else {
				direction = DIR_NE;
			}
		}
	} else {
		if (dy > 0) {
			direction = DIR_N;
		} else {
			direction = DIR_S;
		}
	}

	std::ostringstream ss;
	ss << found_player->getName();

	if (distance == DISTANCE_BESIDE) {
		if (level == LEVEL_SAME) {
			ss << " is standing next to you.";
		} else if (level == LEVEL_HIGHER) {
			ss << " is above you.";
		} else if (level == LEVEL_LOWER) {
			ss << " is below you.";
		}
	} else {
		switch (distance) {
			case DISTANCE_CLOSE:
				if (level == LEVEL_SAME) {
					ss << " is to the ";
				} else if (level == LEVEL_HIGHER) {
					ss << " is on a higher level to the ";
				} else if (level == LEVEL_LOWER) {
					ss << " is on a lower level to the ";
				}
				break;
			case DISTANCE_FAR:
				ss << " is far to the ";
				break;
			case DISTANCE_VERYFAR:
				ss << " is very far to the ";
				break;
			default:
				break;
		}

		switch (direction) {
			case DIR_N:
				ss << "north.";
				break;
			case DIR_S:
				ss << "south.";
				break;
			case DIR_E:
				ss << "east.";
				break;
			case DIR_W:
				ss << "west.";
				break;
			case DIR_NE:
				ss << "north-east.";
				break;
			case DIR_NW:
				ss << "north-west.";
				break;
			case DIR_SE:
				ss << "south-east.";
				break;
			case DIR_SW:
				ss << "south-west.";
				break;
		}
	}

	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, ss.str());
	g_game.announceGraphicalEffect(player, 13);
	return found_player;
}

void Game::creatureTalk(Creature* creature, TalkType_t type, const std::string& text)
{
	if (type != TALKTYPE_SAY && type != TALKTYPE_WHISPER && type!= TALKTYPE_YELL) {
		fmt::printf("ERROR - Game::creatureTalk: invalid talk type (%s:%d)\n", creature->getName(), type);
		return;
	}

	if (text == "p") {
		creature->damage(creature, 200, DAMAGE_POISONING);
	}

	int32_t range = 7;
	std::string real_text = text;

	if (type == TALKTYPE_YELL || type == TALKTYPE_MONSTER_YELL) {
		if (Player* player = creature->getPlayer()) {
			if (serverMilliseconds() < player->earliest_yell_round) {
				Protocol::sendResult(player->connection_ptr, YOUAREEXHAUSTED);
				return;
			}

			player->earliest_yell_round = serverMilliseconds() + 30 * 1000;
		}

		range = 30;
		real_text = boost::to_upper_copy(text);
	}

	uint32_t statement_id = 0;

	if (Player* player = creature->getPlayer()) {
		statement_id = appendStatement(player, text, 0, type);
	}

	std::unordered_set<Creature*> spectator_list;
	const Position& pos = creature->getPosition();

	getSpectators(spectator_list, pos.x, pos.y, range, range, true);

	for (Creature* spectator : spectator_list) {
		const Position& creature_pos = spectator->getPosition();
		if (creature_pos.z != pos.z) {
			continue;
		}

		const int32_t dx = Position::getOffsetX(pos, creature_pos);
		const int32_t dy = Position::getOffsetY(pos, creature_pos);

		if (Player* player_spectator = spectator->getPlayer()) {
			if (type == TALKTYPE_WHISPER) {
				if (dx <= 7 && dy <= 5) {
					if (dx <= 1 && dy <= 1) {
						Protocol::sendTalk(player_spectator->connection_ptr, statement_id, creature->getName(), type, real_text, pos);
					} else {
						Protocol::sendTalk(player_spectator->connection_ptr, statement_id, creature->getName(), type, "pspsps", pos);
					}
				}
			} else if (type == TALKTYPE_SAY || type == TALKTYPE_MONSTER_SAY) {
				if (dx <= 7 && dy <= 5) {
					Protocol::sendTalk(player_spectator->connection_ptr, statement_id, creature->getName(), type, real_text, pos);
				}
			} else {
				Protocol::sendTalk(player_spectator->connection_ptr, statement_id, creature->getName(), type, real_text, pos);
			}
		}
	}
}

void Game::creatureAttack(Creature* creature, uint32_t creature_id, bool follow)
{
	if (creature_id == 0) {
		Combat::setAttackDest(creature, nullptr, follow);	
		return;
	}

	Creature* target = getCreatureById(creature_id);
	if (target == nullptr) {
		fmt::printf("INFO - Game::creatureAttack: attempt to attack invalid creature (%d)\n", creature_id);
		return;
	}

	if (!creature->canSeeCreature(target)) {
		fmt::printf("INFO - Game::creatureAttack: creature cannot see target.\n");
		return;
	}

	Combat::setAttackDest(creature, target, follow);
	creature->toDoAttack();
	creature->toDoStart();
}

void Game::playerUseObject(Player* player, const Position& pos, uint16_t type_id, uint8_t index, uint8_t container_id)
{
	Object* object = getObject(player, pos, index, type_id, SEARCH_USEITEM);
	if (object == nullptr) {
		Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
		return;
	}

	Item* item = object->getItem();
	if (item == nullptr || item->getFlag(MULTIUSE) || (item->getFlag(DISGUISE) && type_id != item->getAttribute(DISGUISETARGET)) || item->getId() != type_id) {
		Protocol::sendResult(player->connection_ptr, CANNOTUSETHISOBJECT);
		return;
	}

	const Position& item_pos = item->getPosition();
	const Position& player_pos = player->getPosition();

	if (item_pos.z > player_pos.z) {
		Protocol::sendResult(player->connection_ptr, FIRSTGODOWNSTAIRS);
		return;
	}

	if (item_pos.z < player_pos.z) {
		Protocol::sendResult(player->connection_ptr, FIRSTGOUPSTAIRS);
		return;
	}

	if (!Position::isAccessible(player_pos, item_pos, 1)) {
		Shortway shortway(player);
		if (!shortway.calculate(item_pos.x, item_pos.y, false)) {
			Protocol::sendResult(player->connection_ptr, THEREISNOWAY);
			return;
		}
		player->toDoWait(100);
		player->toDoUse(pos, type_id, index, container_id);
		player->toDoStart();
		return;
	}

	if (pos.x != 0xFFFF && item->getFlag(HANG)) {
		if (!Position::isAccessible(player_pos, item_pos, 1)) {
			Protocol::sendResult(player->connection_ptr, THEREISNOWAY);
			return;
		}
	}

	if (item->getFlag(CONTAINER)) {
		const int32_t old_container_id = player->getContainerId(item);
		if (old_container_id != -1) {
			for (const auto& it : player->open_containers) {
				if (it.second == item) {
					Protocol::sendCloseContainer(player->connection_ptr, it.first);
				}
			}

			player->closeContainer(old_container_id);
		} else {
			player->addContainer(container_id, item);

			for (const auto& it : player->open_containers) {
				if (it.second == item) {
					Protocol::sendContainer(player->connection_ptr, it.first, it.second);
				}
			}
		}
		return;
	} 

	if (item->getFlag(CHANGEUSE)) {
		playerUseChangeItem(player, item);
		return;
	} 
	
	if (item->getFlag(FOOD)) {
		playerUseFood(player, item);
		return;
	} 
	
	if (item->getFlag(WRITE) || item->getFlag(WRITEONCE)) {
		playerUseTextItem(player, item);
		return;
	} 
	
	if (item->getFlag(INFORMATION)) {
		return;
	} 

	if (item->getFlag(KEYDOOR)) {
		playerUseKeyDoor(player, item);
		return;
	}
	
	if (item->getFlag(NAMEDOOR)) {
		playerUseNameDoor(player, item);
		return;
	} 
	
	if (item->getFlag(LEVELDOOR)) {

		return;
	} 
	
	if (item->getFlag(QUESTDOOR)) {

		return;
	} 

	if (item->getFlag(TEXT)) {
		if (item->getAttribute(FONTSIZE) == 1) {
			playerUseTextItem(player, item);
			return;
		}
	}
	
	Protocol::sendResult(player->connection_ptr, CANNOTUSETHISOBJECT);
}

void Game::playerUseOnCreature(Player* player, const Position& pos, uint16_t type_id, uint8_t index,
	uint32_t creature_id)
{
	Object* object = getObject(player, pos, index, type_id, SEARCH_USEITEM);
	if (object == nullptr) {
		Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
		return;
	}

	Item* item = object->getItem();
	if (item == nullptr || (item->getFlag(DISGUISE) && type_id != item->getAttribute(DISGUISETARGET)) || item->getId() != type_id) {
		Protocol::sendResult(player->connection_ptr, CANNOTUSETHISOBJECT);
		fmt::printf("INFO - Game::playerUseOnCreature: %s used invalid item type (typeid:%d,found:%d).\n", player->getName(), type_id, item->getId());
		return;
	}

	Creature* creature = getCreatureById(creature_id);
	if (creature == nullptr) {
		Protocol::sendResult(player->connection_ptr, CANNOTUSETHISOBJECT);
		fmt::printf("INFO - Game::playerUseOnCreature: %s used on null creature (%d).\n", player->getName(), creature_id);
		return;
	}

	if (!player->canSeePosition(creature->getPosition())) {
		Protocol::sendResult(player->connection_ptr, CANNOTUSETHISOBJECT);
		fmt::printf("INFO - Game::playerUseOnCreature: %s used on creature that is too far away (%s).\n", player->getName(), creature->getName());
		return;
	}

	if (item->getFlag(RUNE)) {
		playerUseRune(player, item, creature);
		return;
	}

	Protocol::sendResult(player->connection_ptr, CANNOTUSETHISOBJECT);
}

void Game::playerUseTwoObjects(Player* player, const Position& pos, uint16_t type_id, uint8_t index, const Position& to_pos, uint16_t to_type_id, uint8_t to_index)
{
	Object* object = getObject(player, pos, index, type_id, SEARCH_USEITEM);
	if (object == nullptr) {
		Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
		return;
	}

	Item* item = object->getItem();
	if (item == nullptr || (item->getFlag(DISGUISE) && type_id != item->getAttribute(DISGUISETARGET)) || item->getId() != type_id) {
		Protocol::sendResult(player->connection_ptr, CANNOTUSETHISOBJECT);
		fmt::printf("INFO - Game::playerUseTwoObjects: %s used invalid item type (typeid:%d,found:%d).\n", player->getName(), type_id, item->getId());
		return;
	}
	
	playerUseTwoObjects(player, item, to_pos, to_type_id, to_index);
}

void Game::playerUseTwoObjects(Player* player, Item* item, const Position& to_pos, uint16_t to_type_id, uint8_t to_index)
{
	if (!item->getFlag(MULTIUSE)) {
		fmt::printf("INFO - Game::playerUseTwoObjects: %s used item (%d) which has no MULTIUSE flag.\n", player->getName(), item->getId());
		return;
	}

	Object* target = getObject(player, to_pos, to_index, to_type_id, SEARCH_LOOK);
	if (target == nullptr) {
		Protocol::sendResult(player->connection_ptr, CANNOTUSETHISOBJECT);
		fmt::printf("INFO - Game::playerUseTwoObjects: %s used to null target.\n", player->getName());
		return;
	}

	const Position& item_pos = item->getPosition();
	const Position& player_pos = player->getPosition();

	if (!Position::isAccessible(player_pos, item_pos, 1)) {
		Shortway shortway(player);
		if (!shortway.calculate(item_pos.x, item_pos.y, false)) {
			Protocol::sendResult(player->connection_ptr, THEREISNOWAY);
			return;
		}
		player->toDoWait(100);
		player->toDoUseTwoObjects(item, to_pos, to_type_id, to_index);
		player->toDoStart();
		return;
	}

	const Position& target_pos = target->getPosition();
	if (!Position::isAccessible(player_pos, target_pos, 1) && !item->getFlag(DISTUSE)) {
		if (item->getHoldingPlayer() != player) {
			const ReturnValue_t ret = moveItem(item, item->getAttribute(ITEM_AMOUNT), item->getParent(), player, INDEX_ANYWHERE, player, 0);
			if (ret != ALLGOOD) {
				Protocol::sendResult(player->connection_ptr, ret);
				return;
			}
		}

		Shortway shortway(player);
		if (!shortway.calculate(target_pos.x, target_pos.y, false)) {
			Protocol::sendResult(player->connection_ptr, THEREISNOWAY);
			return;
		}

		player->toDoWait(100);
		player->toDoUseTwoObjects(item, to_pos, to_type_id, to_index);
		player->toDoStart();
		return;
	}

	if (target_pos.z != player_pos.z) {
		Protocol::sendResult(player->connection_ptr, target_pos.z < player_pos.z ? FIRSTGOUPSTAIRS : FIRSTGODOWNSTAIRS);
		return;
	}

	if (to_pos.x != 0xFFFF) {
		if (!g_map.throwPossible(item_pos, to_pos)) {
			Protocol::sendResult(player->connection_ptr, CANNOTTHROW);
			return;
		}
	}

	if (item->getFlag(RUNE)) {
		playerUseRune(player, item, target);
		return;
	}

	Protocol::sendResult(player->connection_ptr, CANNOTUSETHISOBJECT);
}

void Game::playerMoveObject(Player* player, const Position& from_pos, uint8_t from_index, const Position& to_pos, uint16_t type_id, uint8_t amount)
{
	if (from_pos.x == 0xFFFF) {
		if (from_pos.y & 0x40) {
			from_index = from_pos.z;
		} else {
			from_index = static_cast<uint8_t>(from_pos.y);
		}
	}

	Object* object = getObject(player, from_pos, from_index, type_id, SEARCH_MOVE);
	if (object == nullptr) {
		Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
		return;
	}

	if (Item* moving_item = object->getItem()) {
		Cylinder* to_cylinder = getCylinder(player, to_pos);
		if (to_cylinder == nullptr) {
			Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
			return;
		}

		playerMoveItem(player, moving_item, amount, type_id, object->getParent(), to_cylinder, to_pos);
	} else if (Creature* creature = object->getCreature()) {
		Tile* to_tile = g_map.getTile(to_pos.x, to_pos.y, to_pos.z);
		if (to_tile == nullptr) {
			Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
			return;
		}

		const Position& creature_pos = creature->getPosition();
		if (to_pos.z != creature_pos.z) {
			Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
			return;
		}

		const Position& player_pos = player->getPosition();
		if (!Position::isAccessible(player_pos, creature_pos, 1)) {
			Shortway shortway(player);
			if (!shortway.calculate(creature_pos.x, creature_pos.y, false)) {
				Protocol::sendResult(player->connection_ptr, THEREISNOWAY);
				return;
			}
			player->toDoMoveObject(from_pos, type_id, from_index, to_pos, amount);
			player->toDoStart();
			return;
		}

		if (creature->earliest_walk_time > serverMilliseconds()) {
			return;
		}

		moveCreature(creature, to_pos.x, to_pos.y, to_pos.z, 0);
	}
}

void Game::playerMoveItem(Player* player, Item* item, uint8_t amount, uint16_t type_id, Cylinder* from_cylinder, Cylinder* to_cylinder, const Position& to_pos)
{
	if ((item->getFlag(DISGUISE) && item->getAttribute(DISGUISETARGET) != type_id) || item->getId() != type_id) {
		fmt::printf("INFO - Game::playerMoveItem: %s tried to move invalid item (sent type:%d,real type:%d)\n", player->getName(), type_id, item->getId());
		Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
		return;
	}

	if (item->getFlag(UNMOVE)) {
		Protocol::sendResult(player->connection_ptr, NOTMOVEABLE);
		return;
	}

	const Position& player_pos = player->getPosition();
	const Position& item_pos = item->getPosition();

	if (player_pos.z != item_pos.z) {
		Protocol::sendResult(player->connection_ptr, player_pos.z > item_pos.z ? FIRSTGOUPSTAIRS : FIRSTGODOWNSTAIRS);
		return;
	}

	// first we must make sure the player can reach the item
	if (!Position::isAccessible(player_pos, item_pos, 1)) {
		// player is not in 1/1 range to the item, find path to the item...
		Shortway shortway(player);
		if (!shortway.calculate(item_pos.x, item_pos.y, false)) {
			Protocol::sendResult(player->connection_ptr, THEREISNOWAY);
			return;
		}

		// then move the item to the destination
		player->toDoMoveItem(item, amount, to_pos);
		player->toDoStart();
		return;
	}

	if (to_pos.x != 0xFFFF) {
		// hangables
		if (item->getFlag(HANG) && player_pos.z != to_pos.z) {
			Protocol::sendResult(player->connection_ptr, player_pos.z > to_pos.z ? FIRSTGOUPSTAIRS : FIRSTGODOWNSTAIRS);
			return;
		}

		if (item->getFlag(HANG) && (g_map.getCoordinateFlag(to_pos, HOOKSOUTH) || g_map.getCoordinateFlag(to_pos, HOOKEAST))) {
			if (!Position::isAccessible(player_pos, to_pos, 1)) {
				if (item->getHoldingPlayer() != player) {
					const ReturnValue_t ret = moveItem(item, item->getAttribute(ITEM_AMOUNT), from_cylinder, player, INDEX_ANYWHERE, player, 0);
					if (ret != ALLGOOD) {
						Protocol::sendResult(player->connection_ptr, ret);
						return;
					}
				}

				Shortway shortway(player);
				if (!shortway.calculate(to_pos.x, to_pos.y, false)) {
					Protocol::sendResult(player->connection_ptr, THEREISNOWAY);
					return;
				}

				player->toDoMoveItem(item, amount, to_pos);
				player->toDoStart();
				return;
			}
		}
	}

	if (to_pos.x != 0xFFFF) {
		if (!g_map.throwPossible(item_pos, to_pos)) {
			Protocol::sendResult(player->connection_ptr, CANNOTTHROW);
			return;
		}

		if (item->getFlag(UNPASS)) {
			if (!Position::isAccessible(item_pos, to_pos, 3) || item_pos.z != to_pos.z) {
				Protocol::sendResult(player->connection_ptr, CANNOTTHROW);
				return;
			}
		}
	}

	uint8_t to_index = 0;
	if (to_pos.x == 0xFFFF) {
		if (to_pos.y & 0x40) {
			to_index = to_pos.z;
		} else {
			to_index = to_pos.y;
		}
	}

	const ReturnValue_t ret = moveItem(item, amount, from_cylinder, to_cylinder, to_index, player, 0);
	if (ret != ALLGOOD) {
		Protocol::sendResult(player->connection_ptr, ret);
	}
}

void Game::playerTurnObject(Player* player, const Position& pos, uint16_t type_id, uint8_t index)
{
	if (pos.x == 0xFFFF) {
		if (pos.y & 0x40) {
			index = pos.z;
		} else {
			index = static_cast<uint8_t>(pos.y);
		}
	}

	Object* object = getObject(player, pos, index, type_id, SEARCH_MOVE);
	if (object == nullptr) {
		Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
		return;
	}

	Item* item = object->getItem();
	if (item == nullptr) {
		fmt::printf("INFO - Game::playerTurnObject: object is not an item (user:%s)\n", player->getName());
		return;
	}

	if ((item->getFlag(DISGUISE) && item->getAttribute(DISGUISETARGET) != type_id) && item->getId() != type_id) {
		fmt::printf("INFO - Game::playerTurnObject: item type does not match (sent typeid:%d,typeid:%d)\n", type_id, item->getId());
		return;
	}

	if (!item->getFlag(ROTATE)) {
		fmt::printf("INFO - Game::playerTurnObject: item has no flag ROTATE (typeid:%d,user:%s)\n", item->getId(), player->getName());
		return;
	}

	const int32_t target = item->getAttribute(ROTATETARGET);
	if (target == 0) {
		fmt::printf("INFO - Game::playerTurnObject: item type %d ROTATETARGET is 0.\n", item->getId());
		return;
	}

	changeItem(item, target, 0);
}

void Game::playerCloseContainer(Player* player, uint8_t container_id) const
{
	player->closeContainer(container_id);
	Protocol::sendCloseContainer(player->connection_ptr, container_id);
}

void Game::playerUpContainer(Player* player, uint8_t container_id) const
{
	Item* item = player->getContainerById(container_id);
	if (item == nullptr) {
		return;
	}

	Item* parent_item = item->getParent()->getItem();
	if (parent_item == nullptr) {
		return;
	}

	player->addContainer(container_id, parent_item);
	Protocol::sendContainer(player->connection_ptr, container_id, parent_item);
}

void Game::playerLookAtObject(Player* player, const Position& pos, uint16_t type_id, uint8_t index) const
{
	Object* object = getObject(player, pos, index, type_id, SEARCH_LOOK);
	if (object == nullptr) {
		fmt::printf("INFO - Game::playerLookAtObject: %s - object is null.\n", player->getName());
		return;
	}

	const Position& object_pos = object->getPosition();
	if (!player->canSeePosition(object_pos)) {
		fmt::printf("INFO - Game::playerLookAtObject: %s - position is out of range.\n", player->getName());
		return;
	}

	std::ostringstream ss;
	ss << "You see " << object->getObjectDescription(player);
	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, ss.str());
}

void Game::playerOpenChannel(Player* player) const
{
	PrivateChannel* channel = g_channels.getPrivateChannel(player);
	if (channel == nullptr) {
		fmt::printf("ERROR - Game::playerOpenChannel: failed to create private channel for player (%d).\n", player->getName());
		return;
	}

	channel->addSubscriber(player);
	Protocol::sendOpenOwnChannel(player->connection_ptr, channel);
}

void Game::playerInviteToChannel(Player* player, const std::string& name) const
{
	PrivateChannel* channel = g_channels.getPrivateChannel(player);
	if (channel == nullptr) {
		fmt::printf("ERROR - Game::playerInviteToChannel: failed to create private channel for player (%d).\n", player->getName());
		return;
	}

	const std::string real_name = getPlayerName(name);
	if (!playerExists(real_name)) {
		Protocol::sendResult(player->connection_ptr, APLAYERWITHTHISNAMEDOESNOTEXIST);
		return;
	}

	const uint32_t player_id = getPlayerId(real_name);
	if (player_id == player->getId()) {
		// cannot invite self to channel
		return;
	}

	if (channel->isInvited(player_id)) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has already been invited.", real_name));
		return;
	}

	Player* invited_player = getPlayerByName(real_name);
	if (invited_player) {
		Protocol::sendTextMessage(invited_player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s invites you to %s private chat channel.", player->getName(), player->getSex() == SEX_MALE ? "his" : "her"));
	}

	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has been invited.", real_name));
	channel->addInvitation(player_id);
}

void Game::playerExcludeFromChannel(Player* player, const std::string& name) const
{
	PrivateChannel* channel = g_channels.getPrivateChannel(player);
	if (channel == nullptr) {
		fmt::printf("ERROR - Game::playerExcludeFromChannel: failed to create private channel for player (%d).\n", player->getName());
		return;
	}

	const std::string real_name = getPlayerName(name);
	if (!playerExists(real_name)) {
		Protocol::sendResult(player->connection_ptr, APLAYERWITHTHISNAMEDOESNOTEXIST);
		return;
	}

	const uint32_t player_id = getPlayerId(real_name);
	if (player_id == player->getId()) {
		// cannot exclude self to channel
		return;
	}

	if (!channel->isInvited(player_id)) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has not been invited.", real_name));
		return;
	}

	Player* invited_player = getPlayerByName(real_name);
	if (invited_player) {
		Protocol::sendCloseChannel(invited_player->connection_ptr, channel);
	}

	channel->removeSubscriber(invited_player);
	channel->revokeInvitation(player_id);

	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has been excluded.", real_name));
}

void Game::playerRequestChannels(Player* player) const
{
	const std::vector<const Channel*> channels = g_channels.getChannelsForPlayer(player);
	Protocol::sendChannels(player->connection_ptr, channels);
}

void Game::playerJoinChannel(Player* player, uint16_t channel_id) const
{
	Channel* channel = g_channels.getChannelById(channel_id);
	if (channel == nullptr) {
		fmt::printf("INFO - Game::playerJoinChannel: %s - channel is invalid (%d).\n", player->getName(), channel_id);
		return;
	}

	if (!channel->mayJoin(player)) {
		fmt::printf("INFO - Game::playerJoinChannel: %s - may not join channel (%d).\n", player->getName(), channel_id);
		return;
	}

	channel->addSubscriber(player);
	if (channel->getId() == CHANNEL_RULEVIOLATIONS) {
		Protocol::sendRuleViolationChannel(player->connection_ptr, CHANNEL_RULEVIOLATIONS);
	} else {
		Protocol::sendChannel(player->connection_ptr, channel);
	}
}

void Game::playerCloseChannel(Player* player, uint16_t channel_id) const
{
	Channel* channel = g_channels.getChannelById(channel_id);
	if (channel == nullptr) {
		fmt::printf("INFO - Game::playerCloseChannel: %s - channel is null (%d).\n", player->getName(), channel_id);
		return;
	}

	channel->removeSubscriber(player);

	if (channel->isOwnChannel(player)) {
		for (Player* subscriber : channel->getSubscribers()) {
			Protocol::sendCloseChannel(subscriber->connection_ptr, channel);
		}
		channel->closeChannel();
	}
}

void Game::playerOpenPrivateChannel(Player* player, const std::string& address) const
{
	if (!playerExists(address)) {
		Protocol::sendResult(player->connection_ptr, APLAYERWITHTHISNAMEDOESNOTEXIST);
		return;
	}

	const std::string formatted_name = getPlayerName(address);
	Protocol::sendPrivateChannel(player->connection_ptr, formatted_name);
}

void Game::playerTalk(Player* player, const std::string& text, const std::string& address, uint16_t channel_id, TalkType_t type, bool check_spamming)
{
	if (text == "d") {
		player->damage(player, 100, DAMAGE_FIRE);
	}

	const uint8_t spell_result = playerCastSpell(player, text);
	if (spell_result == SPELL_SUCCESS) {
		creatureTalk(player, TALKTYPE_SAY, text);
		return;
	}

	if (spell_result == SPELL_FAILED) {
		g_game.announceGraphicalEffect(player, 3);
		return;
	}

	if (check_spamming) {
		int64_t muting = player->checkForMuting() / 1000;
		if (muting > 0) {
			if (type - 1 <= TALKTYPE_YELL || channel_id >= CHANNEL_GAMEMASTER && channel_id <= CHANNEL_HELP) {
				std::ostringstream ss;
				ss << "You are still muted for " << muting << ((muting > 1) ? " seconds" : " second") << '.';
				Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, ss.str());
				return;
			}
		}

		if (type - 1 <= TALKTYPE_YELL || channel_id >= CHANNEL_GAMEMASTER && channel_id <= CHANNEL_HELP) {
			muting = player->recordTalk();
			if (muting > 0) {
				std::ostringstream ss;
				ss << "You are muted for " << muting << ((muting > 1) ? " seconds" : " second") << '.';
				Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, ss.str());
				return;
			}
		}
	}

	if (type == TALKTYPE_CHANNEL_Y || type == TALKTYPE_CHANNEL_R1 ||
		type == TALKTYPE_CHANNEL_R2) {
		if (channel_id == 0) {
			fmt::printf("INFO - Game::playerTalk: %s - talk to zero channel (%d).", player->getName(), channel_id);
			return;
		}

		if (channel_id == CHANNEL_TRADE) {
			if (player->earliest_trade_channel_round > serverMilliseconds()) {
				Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, "You may only place one offer in two minutes.");
				return;
			}

			player->earliest_trade_channel_round = serverMilliseconds() + 2 * 60 * 1000;
		}

		playerTalkChannel(player, channel_id, text, type);
	} else if (type == TALKTYPE_PRIVATE || type == TALKTYPE_PRIVATE_RED || type == TALKTYPE_RVR_ANSWER) {
		if (address.empty()) {
			fmt::printf("INFO - Game::playerTalk: %s to empty address (\"%s\").\n", player->getName(), address);
			return;
		}

		Player* address_player = getPlayerByName(address);
		if (address_player == nullptr) {
			Protocol::sendResult(player->connection_ptr, PLAYERWITHTHISNAMEISNOTONLINE);
			return;
		}

		if (type == TALKTYPE_RVR_ANSWER) {
			Protocol::sendTalk(address_player->connection_ptr, address_player->getId(), "Gamemaster", TALKTYPE_RVR_ANSWER, text, serverMilliseconds() / 1000);
			return;
		}

		const int64_t muting = player->recordMessage(address_player->getId());
		if (muting > 0) {
			std::ostringstream ss;
			ss << "You have adressed too many players. You are muted for " << muting / 1000 << ((muting / 1000 > 1) ? " seconds" : " second") << '.';
			Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, ss.str());
			return;
		}

		const uint32_t statement_id = appendStatement(player, text, channel_id, type);
		
		Protocol::sendTalk(address_player->connection_ptr, statement_id, player->getName(), type, text, 0);
		
		std::ostringstream ss;
		ss << "Message sent to " << address_player->getName() << '.';
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, ss.str());
	} else if (type == TALKTYPE_SAY || type == TALKTYPE_WHISPER || type == TALKTYPE_YELL) {
		creatureTalk(player, type, text);
	} else if (type == TALKTYPE_RVR_CHANNEL) {
		playerReportRuleViolation(player, text);
	} else if (type == TALKTYPE_RVR_CONTINUE) {
		playerContinueRuleViolationReport(player, text);
	}
}

void Game::playerTalkChannel(Player* player, uint16_t channel_id, const std::string& text, TalkType_t type) const
{
	Channel* channel = g_channels.getChannelById(channel_id);
	if (channel == nullptr) {
		fmt::printf("INFO - Game::playerTalkChannel: %s channel is null (%d).\n", player->getName(), channel_id);
		return;
	}

	// if player is not subscribed then,
	// there is no need to check if palyer can join or not
	if (!channel->isSubscribed(player)) {
		fmt::printf("INFO - Game::playerTalkChannel: %s channel is null (%d).\n", player->getName(), channel_id);
		return;
	}

	const uint32_t statement_id = appendStatement(player, text, channel_id, type);

	for (Player* subscriber : channel->getSubscribers()) {
		Protocol::sendTalk(subscriber->connection_ptr, statement_id, player->getName(), type, channel->getId(), text);
	}
}

void Game::playerReportRuleViolation(Player* player, const std::string & text)
{
	const auto it = player_requests.find(player->getId());
	if (it != player_requests.end()) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, "You already have a pending rule violation report. Close it before starting a new one.");
		return;
	}

	RuleViolationEntry entry;
	entry.player = player;
	entry.text = text;
	entry.timestamp = serverMilliseconds();
	player_requests[player->getId()] = entry;

	Channel* channel = g_channels.getChannelById(CHANNEL_RULEVIOLATIONS);
	if (channel == nullptr) {
		fmt::printf("ERROR - Game::playerReportRuleViolation: channel is null.\n");
		return;
	}

	for (Player* gamemaster : channel->getSubscribers()) {
		Protocol::sendTalk(gamemaster->connection_ptr, player->getId(), player->getName(), TALKTYPE_RVR_CHANNEL, text, entry.timestamp / 1000);
	}
}

void Game::playerContinueRuleViolationReport(Player* player, const std::string& text)
{
	auto it = player_requests.find(player->getId());
	if (it == player_requests.end()) {
		fmt::printf("ERROR - Game::playerContinueRuleViolationReport: %s has no open report.\n", player->getName());
		return;
	}

	RuleViolationEntry& entry = it->second;
	if (entry.gamemaster == nullptr) {
		fmt::printf("ERROR - Game::playerContinueRuleViolationReport: %s connected gamemaster is null.\n", player->getName());
		return;
	}

	Protocol::sendTalk(entry.gamemaster->connection_ptr, player->getId(), player->getName(), TALKTYPE_RVR_CONTINUE, text, entry.timestamp / 1000);
	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, "Message sent to gamemaster.");
}

void Game::playerTradeObject(Player* player, const Position& pos, uint16_t type_id, uint8_t index, uint32_t player_id)
{
	Player* partner = getPlayerByUserId(player_id);
	if (partner == nullptr) {
		fmt::printf("INFO - Game::playerTradeObject: %s - partner is null.\n", player->getName());
		return;
	}

	if (partner == player) {
		fmt::printf("INFO - Game::playerTradeObject: %s attempted to trade with itself.\n", player->getName());
		return;
	}

	const Position& partner_pos = partner->getPosition();
	const Position& player_pos = player->getPosition();

	if (!Position::isAccessible(partner_pos, player_pos, 2) || partner_pos.z != player_pos.z) {
		std::ostringstream ss;
		ss << player->getName() << " tells you to move closer.";
		Protocol::sendTextMessage(partner->connection_ptr, MESSAGE_OBJECT_INFO, ss.str());
		return;
	}

	Object* object = getObject(player, pos, index, type_id, SEARCH_MOVE);
	if (object == nullptr) {
		fmt::printf("INFO - Game::playerTradeObject: %s - object is null.\n", player->getName());
		return;
	}

	Item* trade_item = object->getItem();
	if (trade_item == nullptr) {
		fmt::printf("INFO - Game::playerTradeObject: %s - object is not an item.\n", player->getName());
		return;
	}

	if ((trade_item->getFlag(DISGUISE) && trade_item->getAttribute(DISGUISETARGET) != type_id) || trade_item->getId() != type_id) {
		fmt::printf("INFO - Game::playerTradeObject: %s (sent type:%d, real type:%d)\n", player->getName(), type_id, trade_item->getId());
		return;
	}

	if (!trade_item->getFlag(TAKE)) {
		Protocol::sendResult(player->connection_ptr, NOTPOSSIBLE);
		return;
	}

	for (const auto& it : trading_items) {
		Item* item = it.first;
		if (item == trade_item) {
			Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, "This item is already being traded.");
			return;
		}

		if (item->getFlag(CONTAINER) && item->isHoldingItem(trade_item)) {
			Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, "This item is already being traded.");
			return;
		}
	}

	if (trade_item->getFlag(CONTAINER) && trade_item->getHoldingItemCount() + 1 > 100) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, "You can not trade more than 100 items.");
		return;
	}

	if (player->trade_state != TRADE_NONE && !(player->trade_state == TRADE_ACKNOWLEDGE && player->trade_partner == partner)) {
		Protocol::sendResult(player->connection_ptr, YOUAREALREADYTRADING);
		return;
	} else if (partner->trade_state != TRADE_NONE && partner->trade_partner != player) {
		Protocol::sendResult(player->connection_ptr, THISPLAYERISALREADYTRADING);
		return;
	}

	player->trade_partner = partner;
	player->trading_item = trade_item;
	player->trade_state = TRADE_INITIATED;

	trading_items[trade_item] = player->getId();

	Protocol::sendTradeItemRequest(player->connection_ptr, player->getName(), trade_item, true);

	if (partner->trade_state == TRADE_NONE) {
		std::ostringstream ss;
		ss << player->getName() << " wants to trade with you.";

		Protocol::sendTextMessage(partner->connection_ptr, MESSAGE_EVENT_ADVANCE, ss.str());
		partner->trade_state = TRADE_ACKNOWLEDGE;
		partner->trade_partner = player;
	} else {
		Item* counter_offer = partner->trading_item;
		Protocol::sendTradeItemRequest(player->connection_ptr, partner->getName(), counter_offer, false);
		Protocol::sendTradeItemRequest(partner->connection_ptr, player->getName(), trade_item, false);
	}
}

void Game::playerInspectTrade(Player* player, bool own_offer, uint8_t index) const
{
	Player* trade_partner = player->trade_partner;
	if (trade_partner == nullptr) {
		fmt::printf("INFO - Game::playerInspectTrade: %s - no active trade.\n", player->getName());
		return;
	}

	Item* trade_item;
	if (own_offer) {
		trade_item = trade_partner->trading_item;
	} else {
		trade_item = player->trading_item;
	}

	if (trade_item == nullptr) {
		fmt::printf("INFO - Game::playerInspectTrade: %s - item is null.\n", player->getName());
		return;
	}

	std::ostringstream ss;
	if (index == 0) {
		ss << "You see " << trade_item->getObjectDescription(player);
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, ss.str());
		return;
	}

	if (!trade_item->getFlag(CONTAINER)) {
		return;
	}

	std::vector<const Item*> containers{ trade_item };
	int32_t i = 0;
	while (i < containers.size()) {
		const Item* container = containers[i++];
		for (Item* item : container->getItems()) {
			if (item->getFlag(CONTAINER)) {
				containers.push_back(item);
			}

			if (--index == 0) {
				ss << "You see " << item->getObjectDescription(player);
				Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, ss.str());
				return;
			}
		}
	}
}

void Game::playerAcceptTrade(Player* player)
{
	if (!(player->trade_state == TRADE_ACKNOWLEDGE || player->trade_state == TRADE_INITIATED)) {
		return;
	}

	Player* trade_partner = player->trade_partner;
	if (trade_partner == nullptr) {
		fmt::printf("INFO - Game::playerAcceptTrade: %s - partner is null.\n", player->getName());
		return;
	}

	player->trade_state = TRADE_ACCEPT;

	if (trade_partner->trade_state == TRADE_ACCEPT) {
		Item* item_1 = player->trading_item;
		Item* item_2 = trade_partner->trading_item;

		player->trade_state = TRADE_TRANSFER;
		trade_partner->trade_state = TRADE_TRANSFER;

		auto it = trading_items.find(item_1);
		if (it != trading_items.end()) {
			trading_items.erase(it);
		}

		it = trading_items.find(item_2);
		if (it != trading_items.end()) {
			trading_items.erase(it);
		}

		bool success_trade = false;

		ReturnValue_t ret_player = canPlayerCarryItem(player, item_2, 0);
		ReturnValue_t ret_partner = canPlayerCarryItem(trade_partner, item_1, 0);

		if (ret_player == ALLGOOD && ret_partner == ALLGOOD) {
			success_trade = true;
		}

		if (!success_trade) {
			Protocol::sendResult(player->connection_ptr, ret_player);
			Protocol::sendResult(trade_partner->connection_ptr, ret_partner);
		} else {
			ret_player = moveItem(item_2, item_2->getAttribute(ITEM_AMOUNT), item_2->getParent(), player, INDEX_ANYWHERE, nullptr, 0);
			ret_partner = moveItem(item_1, item_1->getAttribute(ITEM_AMOUNT), item_1->getParent(), trade_partner, INDEX_ANYWHERE, nullptr, 0);

			if (ret_player != ALLGOOD || ret_partner != ALLGOOD) {
				fmt::printf("WARNING - Game::playerAcceptTrade: failure to move items for players '%s' and '%s'.\n", player->getName(), trade_partner->getName());
			}
		}

		player->trade_state = TRADE_NONE;
		player->trading_item = nullptr;
		player->trade_partner = nullptr;

		trade_partner->trade_state = TRADE_NONE;
		trade_partner->trading_item = nullptr;
		trade_partner->trade_partner = nullptr;

		Protocol::sendTradeClose(player->connection_ptr);
		Protocol::sendTradeClose(trade_partner->connection_ptr);
	}
}

void Game::playerEditText(Player* player, uint32_t edit_text_id, const std::string & text) const
{
	if (player->edit_item == nullptr) {
		fmt::printf("INFO - Game::playerEditText: %s has no edit item.\n", player->getName());
		return;
	}

	if (player->edit_text_id != edit_text_id) {
		fmt::printf("INFO - Game::playerEditText: %s edit text (%d) does not match (%d).\n", player->getName(), player->edit_text_id, edit_text_id);
		return;
	}

	player->edit_item->setText(text);
	player->edit_item->setEditor(player->getName());

	player->edit_item = nullptr;
}

void Game::playerProcessRuleViolationReport(Player* player, const std::string& reporter)
{
	Player* reporter_player = getPlayerByName(reporter);
	if (reporter_player == nullptr) {
		return;
	}

	auto it = player_requests.find(reporter_player->getId());
	if (it == player_requests.end()) {
		fmt::printf("INFO - Game::playerProcessRuleViolationReport: %s's request is not available.\n", reporter_player->getName());
		return;
	}

	RuleViolationEntry& entry = it->second;
	if (!entry.available) {
		fmt::printf("INFO - Game::playerProcessRuleViolationReport: %s's request is already being attended by another gamemaster.\n", reporter_player->getName());
		return;
	}

	entry.gamemaster = player;
	entry.available = false;

	Channel* channel = g_channels.getChannelById(CHANNEL_RULEVIOLATIONS);
	if (channel == nullptr) {
		return;
	}

	for (Player* gamemaster : channel->getSubscribers()) {
		Protocol::sendRemoveRuleViolationReport(gamemaster->connection_ptr, reporter_player->getName());
	}
}

void Game::playerCloseRuleViolationReport(Player* player, const std::string& reporter)
{
	Player* reporter_player = getPlayerByName(reporter);
	if (reporter_player == nullptr) {
		fmt::printf("INFO - Game::playerCloseRuleViolationReport: %s player is not online.\n", reporter);
		return;
	}

	closeRuleViolationReport(reporter_player);
}

void Game::playerCancelRuleViolationReport(Player* player)
{
	const auto it = player_requests.find(player->getId());
	if (it == player_requests.end()) {
		return;
	}

	RuleViolationEntry& entry = it->second;
	if (!entry.available && entry.gamemaster) {
		Protocol::sendRuleViolationCancel(entry.gamemaster->connection_ptr, player->getName());
	}

	Channel* channel = g_channels.getChannelById(CHANNEL_RULEVIOLATIONS);
	if (channel == nullptr) {
		return;
	}

	for (Player* gamemaster : channel->getSubscribers()) {
		Protocol::sendRemoveRuleViolationReport(gamemaster->connection_ptr, player->getName());
	}

	player_requests.erase(it);
}

void Game::playerInviteToParty(Player* player, uint32_t player_id) const
{
	if (player->getId() == player_id) {
		fmt::printf("INFO - Game::playerInviteToParty: %s invited himself to a party.\n", player->getName());
		return;
	}

	Player* target = getPlayerByUserId(player_id);
	if (target == nullptr) {
		fmt::printf("INFO - Game::playerInviteToParty: %s attempted to invite null player.\n", player->getName());
		return;
	}

	if (target->party) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s is already member of a party.", target->getName()));
		return;
	}

	Party* party = player->party;
	if (party == nullptr) {
		// create a new party instance
		party = new Party(player);
		party->addMember(player);
		player->party = party;
	} else if (party->getHost() != player) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, "You may not invite players.");

		fmt::printf("INFO - Game::playerInviteToParty: %s tried to invite a player to a party he is not the host of.\n", player->getName());
		return;
	}

	if (party->isInvited(target)) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has already been invited.", target->getName()));
		return;
	}

	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has been invited.", target->getName()));
	Protocol::sendTextMessage(target->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s invites you to %s party.", player->getName(), (player->sex_type == SEX_MALE ? "his" : "her")));

	party->addInvitation(target);

	Protocol::sendCreatureParty(player->connection_ptr, player);
	Protocol::sendCreatureSkull(player->connection_ptr, player);
	Protocol::sendCreatureParty(player->connection_ptr, target);
	Protocol::sendCreatureParty(target->connection_ptr, player);
}

void Game::playerJoinParty(Player* player, uint32_t player_id) const
{
	if (player->getId() == player_id) {
		fmt::printf("INFO - Game::playerJoinParty: %s tried to join to self-party.\n", player->getName());
		return;
	}

	Player* target = getPlayerByUserId(player_id);
	if (target == nullptr) {
		fmt::printf("INFO - Game::playerJoinParty: %s attempted to join invalid party to NULL player.\n", player->getName());
		return;
	}

	Party* party = target->party;
	if (party == nullptr) {
		fmt::printf("INFO - Game::playerJoinParty: %s attempted to join to invalid party.\n", player->getName());
		return;
	}

	if (party->getHost() != target) {
		return;
	}

	if (!party->isInvited(player)) {
		fmt::printf("INFO - Game::playerJoinParty: %s attempted to join party it is not invited to.\n", player->getName());
		return;
	}

	if (player->party) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("You are already member of %s party.", player->party->getHost()->getName()));
		return;
	}

	party->joinParty(player);
}

void Game::playerRevokePartyInvitation(Player* player, uint32_t player_id) const
{
	if (player->getId() == player_id) {
		fmt::printf("INFO - Game::playerRevokePartyInvitation: %s tried to revoke to self-party.\n", player->getName());
		return;
	}

	Player* target = getPlayerByUserId(player_id);
	if (target == nullptr) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, "This player has not been invited.");
		fmt::printf("INFO - Game::playerRevokePartyInvitation: %s attempted to revoke invalid party to NULL player.\n", player->getName());
		return;
	}

	Party* party = player->party;
	if (party == nullptr || party->getHost() != player) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, "You may not invite players.");
		return;
	}

	if (!party->isInvited(target)) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has not been invited.", target->getName()));
		return;
	}

	party->removeInvitation(target);

	Protocol::sendTextMessage(target->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has revoked %s invitation.", player->getName(), (player->sex_type == SEX_MALE ? "his" : "her")));
	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("Invitation for %s has been revoked.", target->getName()));

	Protocol::sendCreatureParty(player->connection_ptr, target);
	Protocol::sendCreatureParty(target->connection_ptr, player);

	if (party->isEmpty()) {
		party->disband();
	}
}

void Game::playerPassPartyLeadership(Player* player, uint32_t player_id) const
{
	Player* target = getPlayerByUserId(player_id);
	if (target == nullptr) {
		fmt::printf("INFO - Game::playerPassPartyLeadership: %s attempt to pass leadership to invalid player.\n", player->getName());
		return;
	}

	Party* party = player->party;
	if (party == nullptr || party->getHost() != player) {
		return;
	}

	if (party != target->party) {
		return;
	}

	party->passLeadership(target);
}

void Game::playerLeaveParty(Player* player) const
{
	Party* party = player->party;
	if (party == nullptr) {
		return;
	}

	if (player->earliest_logout_round > round_number) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, "You may not leave your party during or immediately after a fight!");
		return;
	}

	party->leaveParty(player);
}

void Game::playerItemIllusion(Player* player, const Item* item) const
{
	uint16_t type_id = item->getId();
	if (item->getFlag(DISGUISE)) {
		type_id = item->getAttribute(DISGUISETARGET);
	}

	Outfit outfit;
	outfit.type_id = type_id;
	player->setOriginalOutfit(player->getCurrentOutfit());
	player->setCurrentOutfit(outfit);
	player->skill_illusion->setTiming(1, 300, 300, -1);
}

void Game::playerTeleportToFriend(Player* player, const std::string& name)
{
	Player* target = getPlayerByName(name);
	if (target == nullptr) {
		Protocol::sendResult(player->connection_ptr, PLAYERWITHTHISNAMEISNOTONLINE);
		return;
	}

	Position pos = target->getPosition();
	if (!g_map.searchFreeField(player, pos, 1)) {
		Protocol::sendResult(player->connection_ptr, NOTENOUGHROOM);
		return;
	}

	announceGraphicalEffect(player, 3);
	moveCreature(player, pos.x, pos.y, pos.z, 0);
	announceGraphicalEffect(player, 11);
}

void Game::playerRetrieveFriend(Player* player, const std::string& name)
{
	Player* target = getPlayerByName(name);
	if (target == nullptr) {
		Protocol::sendResult(player->connection_ptr, PLAYERWITHTHISNAMEISNOTONLINE);
		return;
	}

	Position pos = player->getPosition();
	if (!g_map.searchFreeField(target, pos, 1)) {
		Protocol::sendResult(player->connection_ptr, NOTENOUGHROOM);
		return;
	}

	announceGraphicalEffect(target, 3);
	moveCreature(target, pos.x, pos.y, pos.z, 0);
	announceGraphicalEffect(target, 11);
}

void Game::playerHomeTeleport(Player* player, const std::string& name)
{
	Player* target = player;
	if (!name.empty()) {
		if (identifyPlayer(&target, true, name) == 0) {
			Protocol::sendResult(player->connection_ptr, PLAYERWITHTHISNAMEISNOTONLINE);
			return;
		}

		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has been moved to the temple.", target->getName()));
	}

	Tile* tile = g_map.getTile(target->start_pos);
	if (tile == nullptr) {
		tile = g_map.getTile(newbie_start_pos);
	}

	announceGraphicalEffect(target, 3);
	moveCreature(target, tile, FLAG_NOLIMIT);
	announceGraphicalEffect(target, 11);
}

void Game::playerChangeSpecial(Player* player, const std::string& param)
{
	const std::vector<std::string> params = explodeString(param, ",");
	if (params.size() < 2) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, "Not enough parameters.");
		return;
	}

	const std::string type = lowerString(params[0]);
	const std::string data = params[1];

	if (type == "sex" || type == "gender") {
		const int32_t data_num = getNumber(data);
		if (data == "female" || data == "girl") {
			player->sex_type = SEX_FEMALE;
		} else {
			player->sex_type = SEX_MALE;
		}

		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("Your sex has been change to %s.", player->sex_type == SEX_FEMALE ? "a female" : "a male"));
		g_game.announceGraphicalEffect(player, 15);
	} else if (type == "profession" || type == "vocation") {
		const Vocation* new_vocation = g_vocations.getVocationById(getNumber(data));

		if (data == "none" || data == "novocation") {
			new_vocation = g_vocations.getVocationById(0);
		} else if (data == "knight") {
			new_vocation = g_vocations.getVocationById(1);
		} else if (data == "paladin") {
			new_vocation = g_vocations.getVocationById(2);
		} else if (data == "druid") {
			new_vocation = g_vocations.getVocationById(3);
		} else if (data == "sorcerer") {
			new_vocation = g_vocations.getVocationById(4);
		} else if (data == "elite knight") {
			new_vocation = g_vocations.getVocationById(11);
		} else if (data == "royal paladin") {
			new_vocation = g_vocations.getVocationById(12);
		} else if (data == "elder druid") {
			new_vocation = g_vocations.getVocationById(13);
		} else if (data == "master sorcerer") {
			new_vocation = g_vocations.getVocationById(14);
		}

		if (new_vocation) {
			playerChangeProfession(player, new_vocation);
		}
	} else {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, "Unknown change type.");
	}
}

void Game::playerChangeProfession(Player* player, const Vocation* vocation)
{
	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("Your vocation has been changed to %s.", vocation->getDescription()));
	g_game.announceGraphicalEffect(player, 15);

	player->vocation = vocation;

	if (player->skill_level[SKILL_LEVEL] <= 8) {
		return;
	}

	int32_t max_health = 185;
	int32_t max_mana = 35;
	int32_t max_capacity = 435;
	for (int32_t i = 8; i <= player->skill_level[SKILL_LEVEL]; i++) {
		max_health += vocation->getGainHp();
		max_mana += vocation->getGainMp();
		max_capacity += vocation->getGainCapacity();
	}

	player->skill_hitpoints->setMax(max_health);
	player->skill_hitpoints->setValue(max_health);

	player->skill_manapoints->setMax(max_mana);
	player->skill_manapoints->setValue(max_mana);

	player->capacity = max_capacity;

	Protocol::sendStats(player->connection_ptr);
}

void Game::playerKickPlayer(Player* player, const std::string& name)
{
	Player* target = nullptr;
	int32_t hits = identifyPlayer(&target, false, name);
	if (hits > 1) {
		Protocol::sendResult(player->connection_ptr, NAMEISTOOAMBIGIOUS);
		return;
	} else if (hits == 0) {
		Protocol::sendResult(player->connection_ptr, PLAYERWITHTHISNAMEISNOTONLINE);
		return;
	}

	announceGraphicalEffect(target, 3);
	removeCreature(target);
	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s has been kicked.", target->getName()));
}

bool Game::playerEnchantItem(Player* player, const ItemType* origin, const ItemType* target, int32_t mana, int32_t soulpoints)
{
	Item* item = player->inventory[INVENTORY_LEFT];
	if (item == nullptr || item->getId() != origin->type_id) {
		item = player->inventory[INVENTORY_RIGHT];
		if (item == nullptr || item->getId() != origin->type_id) {
			Protocol::sendResult(player->connection_ptr, YOUNEEDAMAGICITEMTOCASTSPELL);
			return false;
		}
	}

	if (!g_magic.checkMana(player, mana, soulpoints, 1000)) {
		return false;
	}

	changeItem(item, target->type_id, 1);
	announceGraphicalEffect(player, 15);
	return true;
}

Creature* Game::getCreatureById(uint32_t creature_id)
{
	for (Creature* creature : creatures) {
		if (creature->id == creature_id) {
			return creature;
		}
	}
	return nullptr;
}

Player* Game::getStoredPlayer(uint32_t user_id) const
{
	const auto it = stored_players.find(user_id);
	if (it == stored_players.end()) {
		return nullptr;
	}

	return it->second;
}

Player* Game::getPlayerByUserId(uint32_t user_id) const
{
	for (Player* player : players) {
		if (player->id == user_id) {
			return player;
		}
	}
	return nullptr;
}

Player* Game::getPlayerByName(const std::string& name, bool exact_match) const
{
	Player* player = nullptr;
	if (identifyPlayer(&player, exact_match, name) >= 1) {
		return player;
	}

	return nullptr;
}

Cylinder* Game::getCylinder(Player* player, const Position& pos) const
{
	if (pos.x != 0xFFFF) {
		return g_map.getTile(pos.x, pos.y, pos.z);
	}

	if (pos.y & 0x40) {
		const uint8_t container_id = pos.y & 0x0F;
		return player->getContainerById(container_id);
	}

	return player;
}

Object* Game::getObject(Player* player, const Position& pos, uint8_t index, uint16_t type_id, SearchObjectType_t search_type) const
{
	if (pos.x != 0xFFFF) {
		Tile* tile = g_map.getTile(pos.x, pos.y, pos.z);
		if (tile == nullptr) {
			return nullptr;
		}

		Object* object = nullptr;
		switch (search_type) {
			case SEARCH_MOVE: {
				object = tile->getTopObject(true);
				break;
			}
			case SEARCH_USEITEM: {
				object = tile->getTopItem();
				break;
			}
			case SEARCH_LOOK: {
				object = tile->getTopCreature();
				if (object == nullptr) {
					object = tile->getTopItem();
				}
				break;
			}
			default: break;
		}

		return object;
	}

	if (pos.y & 0x40) {
		const uint8_t container_id = pos.y & 0x0F;
		
		Item* item = player->getContainerById(container_id);
		if (item == nullptr) {
			return nullptr;
		}

		const uint8_t slot = pos.z;
		return item->getitemIndex(slot);
	}

	const auto slot = static_cast<InventorySlot_t>(pos.y);
	return player->getInventoryItem(slot);
}

bool Game::playerExists(const std::string& name) const
{
	return player_names_index.find(name) != player_names_index.end();
}

const std::string& Game::getPlayerName(const std::string& name) const
{
	const auto it = player_names_index.find(name);
	if (it == player_names_index.end()) {
		return name;
	}

	return (*it);
}

uint32_t Game::getPlayerId(const std::string& name) const
{
	const auto it = player_ids.find(name);
	if (it == player_ids.end()) {
		return 0;
	}

	return it->second;
}

void Game::closeTrade(Player* player)
{
	Player* trade_partner = player->trade_partner;
	if ((trade_partner && trade_partner->trade_state == TRADE_TRANSFER)) {
		return;
	}

	if (player->trading_item) {
		const auto it = trading_items.find(player->trading_item);
		if (it != trading_items.end()) {
			trading_items.erase(it);
		}

		player->trading_item = nullptr;
	}

	player->trade_state = TRADE_NONE;
	player->trade_partner = nullptr;

	Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, "Trade cancelled.");
	Protocol::sendTradeClose(player->connection_ptr);

	if (trade_partner) {
		if (trade_partner->trading_item) {
			const auto it = trading_items.find(trade_partner->trading_item);
			if (it != trading_items.end()) {
				trading_items.erase(it);
			}

			trade_partner->trading_item = nullptr;
		}

		trade_partner->trade_state = TRADE_NONE;
		trade_partner->trade_partner = nullptr;

		Protocol::sendTextMessage(trade_partner->connection_ptr, MESSAGE_STATUS_SMALL, "Trade cancelled.");
		Protocol::sendTradeClose(trade_partner->connection_ptr);
	}
}

void Game::loginPlayers(int32_t count)
{
	std::vector<boost::filesystem::path> sectors;
	getFilesInDirectory("usr/20/", ".usr", sectors);
	getFilesInDirectory("usr/21/", ".usr", sectors);
	getFilesInDirectory("usr/22/", ".usr", sectors);
	getFilesInDirectory("usr/23/", ".usr", sectors);
	getFilesInDirectory("usr/24/", ".usr", sectors);
	getFilesInDirectory("usr/25/", ".usr", sectors);
	getFilesInDirectory("usr/26/", ".usr", sectors);
	getFilesInDirectory("usr/27/", ".usr", sectors);
	getFilesInDirectory("usr/28/", ".usr", sectors);
	getFilesInDirectory("usr/29/", ".usr", sectors);
	getFilesInDirectory("usr/30/", ".usr", sectors);

	std::random_shuffle(sectors.begin(), sectors.end());

	int32_t loggedin = 0;
	for (auto file : sectors) {
		uint32_t user_id = 0;
		sscanf(file.filename().string().c_str(), "%d.usr", &user_id);

		Player* player = new Player();

		if (!player->loadData(user_id)) {
			delete player;
			return;
		}

		const Position& pos = player->getPosition();
		if (setCreatureOnMap(player, pos.x, pos.y, pos.z) != ALLGOOD) {
			//return;
		}

		loggedin++;

		if (count-- == 0) {
			break;
		}
	}
}

void Game::playerUseChangeItem(Player* player, Item* item)
{
	if (item == nullptr) {
		fmt::printf("ERROR - Game::playerUseChangeItem: item is null.\n");
		return;
	}

	if (player == nullptr) {
		fmt::printf("ERROR - Game::playerUseChangeItem: player is null (typeid:%d)\n", item->getId());
	}

	if (!item->getFlag(CHANGEUSE)) {
		fmt::printf("ERROR - Game::playerUseChangeItem: item has no CHANGEUSE flag (typeid:%d,user:%s)\n", item->getId(), player->getName());
		return;
	}

	const int32_t target = item->getAttribute(CHANGETARGET);
	const ItemType* new_type = g_items.getItemType(target);

	if (target == 0 || new_type == nullptr) {
		fmt::printf("ERROR - Game::playerUseChangeItem: item CHANGETARGET is invalid (typeid:%d)\n", item->getId());
		return;
	}

	if (!item->getFlag(UNPASS)) {
		if (new_type->getFlag(UNPASS)) {
			clearTile(item->getPosition(), item, UNPASS);
		}
	} else if (!item->getFlag(UNLAY)) {
		if (new_type->getFlag(UNLAY)) {
			clearTile(item->getPosition(), item, UNLAY);
		}
	}

	changeItem(item, target, 0);
}

void Game::playerUseKeyDoor(Player* player, Item* item)
{
	if (item == nullptr) {
		fmt::printf("ERROR - Game::playerUseKeyDoor: item is null.\n");
		return;
	}

	if (player == nullptr) {
		fmt::printf("ERROR - Game::playerUseKeyDoor: player is null (typeid:%d)\n", item->getId());
		return;
	}

	if (!item->getFlag(KEYDOOR)) {
		fmt::printf("ERROR - Game::playerUseKeyDoor: item has no KEYDOOR flag (typeid:%d,user:%s)\n", item->getId(), player->getName());
		return;
	}

	const int32_t target = item->getAttribute(CHANGETARGET);
	const ItemType* new_type = g_items.getItemType(target);

	if (target <= 0) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("%s.", item->getDescription()));
		return;
	}
}

void Game::playerUseNameDoor(Player* player, Item* item)
{
	if (item == nullptr) {
		fmt::printf("ERROR - Game::playerUseNameDoor: item is null.\n");
		return;
	}

	if (player == nullptr) {
		fmt::printf("ERROR - Game::playerUseNameDoor: player is null (typeid:%d)\n", item->getId());
	}

	if (!item->getFlag(NAMEDOOR)) {
		fmt::printf("ERROR - Game::playerUseNameDoor: item has no NAMEDOOR flag (typeid:%d,user:%s)\n", item->getId(), player->getName());
		return;
	}

	const int32_t target = item->getAttribute(NAMEDOORTARGET);
	const ItemType* new_type = g_items.getItemType(target);

	if (target == 0 || new_type == nullptr) {
		fmt::printf("ERROR - Game::playerUseNameDoor: item NAMEDOORTARGET is invalid (typeid:%d)\n", item->getId());
		return;
	}

	if (!item->getFlag(UNPASS)) {
		if (new_type->getFlag(UNPASS)) {
			clearTile(item->getPosition(), item, UNPASS);
		}
	} else if (!item->getFlag(UNLAY)) {
		if (new_type->getFlag(UNLAY)) {
			clearTile(item->getPosition(), item, UNLAY);
		}
	}

	changeItem(item, target, 0);
}

void Game::playerUseFood(Player* player, Item* item)
{
	if (!item->getFlag(FOOD)) {
		fmt::printf("ERROR - Game::playerUseFood: item has no FOOD flag (%d).\n", item->getId());
		return;
	}

	SkillFed* skill = player->skill_fed;
	const int32_t cycle = skill->getTiming();
	const int32_t nutrition = item->getAttribute(NUTRITION);

	if (cycle > (1200 - 12 * nutrition)) {
		Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, "You are full.");
		return;
	}

	skill->setTiming(cycle + 12 * nutrition, 0, 0, -1);
	removeItem(item, 1);
}

void Game::playerUseTextItem(Player* player, Item* item) const
{
	if (player == nullptr) {
		fmt::printf("ERROR - Game::playerUseTextItem: player is null.\n");
		return;
	}

	if (!item->getFlag(TEXT)) {
		fmt::printf("ERROR - Game::playerUseTextItem: item (%d) has no TEXT flag.\n", item->getId());
		return;
	}

	player->edit_text_id++;
	player->edit_item = item;
	Protocol::sendEditText(player->connection_ptr, item, player->edit_text_id);
}

void Game::playerUseRune(Player* player, Item* item, Object* target)
{
	if (!item->getFlag(RUNE)) {
		fmt::printf("ERROR - Game::playerUseRune: item is not a RUNE (%d)\n", item->getId());
		return;
	}

	if (target == nullptr) {
		fmt::printf("ERROR - Game::playerUseItem: target is null (%s)\n", player->getName());
		return;
	}

	const Spell* spell = nullptr;
	for (const auto it : g_magic.getSpells()) {
		if (it.second->rune_nr == item->getId()) {
			spell = it.second;
			break;
		}
	}

	if (spell == nullptr) {
		fmt::printf("ERROR - Game::playerUseItem: no spell for rune (%d)\n", item->getId());
		return;
	}

	if (spell->aggressive) {
		if (g_map.isProtectionZone(player->getPosition()) || g_map.isProtectionZone(target->getPosition())) {
			Protocol::sendResult(player->connection_ptr, ACTIONNOTPERMITTEDINPROTECTIONZONE);
			return;
		}
	}

	Creature* to_creature = target->getCreature();
	Item* to_item = target->getItem();

	if (spell->need_target && !to_creature) {
		Protocol::sendResult(player->connection_ptr, CANONLYUSETHISRUNEONCREATURES);
		return;
	}

	if (player->earliest_spell_time >= serverMilliseconds()) {
		Protocol::sendResult(player->connection_ptr, YOUAREEXHAUSTED);
		return;
	}

	if (spell->aggressive && to_creature) {
		if (Player* to_player = to_creature->getPlayer()) {
			if (player->secure_mode && !player->isAttackJustified(to_player)) {
				Protocol::sendResult(player->connection_ptr, TURNSECUREMODETOATTACKUNMARKEDPLAYERS);
				return;
			}
		}
	}

	if (!g_map.fieldPossible(target->getPosition(), FIELD_NONE)) {
		Protocol::sendResult(player->connection_ptr, NOTENOUGHROOM);
		return;
	}

	switch (spell->id) {
		case 4: { // intense healing rune
			g_magic.heal(to_creature, Combat::computeDamage(player, 70, 30, true));
			break;
		}
		case 5: { // ultimate healing rune
			g_magic.heal(to_creature, Combat::computeDamage(player, 250, 0, true));
			break;
		}
		case 7: { // heavy magic missile rune
			Combat::massCombat(player, target, 0, 0, Combat::computeDamage(player, 15, 5, true), 6, 0, DAMAGE_ENERGY, 4);
			break;
		}
		case 8: { // heavy magic missile rune
			Combat::massCombat(player, target, 0, 0, Combat::computeDamage(player, 30, 10, true), 6, 0, DAMAGE_ENERGY, 4);
			break;
		}
		case 14: { // chameleon rune
			if (to_item != nullptr) {
				if (to_item->getFlag(UNMOVE)) {
					Protocol::sendResult(player->connection_ptr, NOTMOVEABLE);
					return;
				}

				playerItemIllusion(player, to_item);
			}

			g_game.announceGraphicalEffect(player, 13);
			break;
		}
		case 15: { // fireball rune
			Combat::massCombat(player, target, 0, 0, Combat::computeDamage(player, 30, 10, true), 7, 4, DAMAGE_FIRE, 4);
			break;
		}
		case 16: { // great fireball rune
			Combat::massCombat(player, target, 0, 0, Combat::computeDamage(player, 50, 15, true), 7, 5, DAMAGE_FIRE, 4);
			break;
		}
		case 17: { // firebomb rune
			massCreateField(player, target->getPosition(), 3, FIELD_FIRE);
			break;
		}
		case 18: { // explosion rune
			Combat::massCombat(player, target, 0, 0, Combat::computeDamage(player, 60, 40, true), 5, 2, DAMAGE_PHYSICAL, 4);
			break;
		}
		case 21: { // sudden death rune
			Combat::massCombat(player, target, 0, 0, Combat::computeDamage(player, 150, 20, true), 18, 0, DAMAGE_PHYSICAL, 11);
			break;
		}
		case 25: { // fire field rune
			massCreateField(player, target->getPosition(), 0, FIELD_FIRE);
			break;
		}
		case 26: { // poison field rune
			massCreateField(player, target->getPosition(), 0, FIELD_POISON);
			break;
		}
		case 27: { // energy field rune
			massCreateField(player, target->getPosition(), 0, FIELD_ENERGY);
			break;
		}
		case 28: { // fire wall rune
			FieldImpact impact;
			impact.actor = player;
			impact.field_type = FIELD_FIRE;
			Combat::angleWallSpell(player->getPosition(), target->getPosition(), 0, (impact.field_type != 1) + 4, impact);
			break;
		}
		case 30: { // destroy field rune
			deleteField(target->getPosition());
			break;
		}
		case 31: { // antidote rune
			g_magic.negatePoison(to_creature);
			break;
		}
		case 32: { // poison wall rune
			FieldImpact impact;
			impact.actor = player;
			impact.field_type = FIELD_POISON;
			Combat::angleWallSpell(player->getPosition(), target->getPosition(), 0, (impact.field_type != 1) + 4, impact);
			break;
		}
		case 33: { // energy wall rune
			FieldImpact impact;
			impact.actor = player;
			impact.field_type = FIELD_ENERGY;
			Combat::angleWallSpell(player->getPosition(), target->getPosition(), 0, (impact.field_type != 1) + 4, impact);
			break;
		}
		case 50: { // soulfire rune
			Combat::massCombat(player, target, 0, 0, Combat::computeDamage(player, 120, 20, true), 16, 0, DAMAGE_BURNING, 4);
			break;
		}
		case 54: { // paralyze rune
			g_magic.magicGoStrength(player, to_creature, -101, 1);
			break;
		}
		case 55: { // energybomb rune
			massCreateField(player, target->getPosition(), 3, FIELD_ENERGY);
			break;
		}
		case 77: { // envenom rune
			Combat::massCombat(player, target, 0, 0, Combat::computeDamage(player, 70, 20, true), 17, 0, DAMAGE_POISONING, 5);
			break;
		}
		case 78: { // desintegrate rune
			if (!Position::isAccessible(player->getPosition(), target->getPosition(), 1)) {
				Protocol::sendResult(player->connection_ptr, DESTINATIONOUTOFREACH);
				return;
			}

			Tile* to_tile = g_map.getTile(target->getPosition());
			cleanUpTile(to_tile);
			break;
		}
		case 86: { // magic wall rune
			massCreateField(player, target->getPosition(), 0, FIELD_MAGIC_WALL);
			break;
		}
		case 91: { // poisonbomb rune
			massCreateField(player, target->getPosition(), 3, FIELD_POISON);
			break;
		}
		default: fmt::printf("ERROR - Game::playerUseRune: unhandled rune (%d)\n", spell->id); break;
	}

	if (spell->aggressive) {
		player->blockLogout(60, false);
	}

	if (item->getAttribute(ITEM_CHARGES) <= 1) {
		g_game.removeItem(item, 1);
	} else {
		item->setAttribute(ITEM_CHARGES, item->getAttribute(ITEM_CHARGES) - 1);
	}

	uint32_t delay = 1000;
	if (spell->aggressive) {
		delay = 2000;
	}

	player->earliest_spell_time = serverMilliseconds() + delay;
}

SpellCastResult_t Game::playerCastSpell(Player* player, std::string text)
{
	std::string params;
	Spell* spell = g_magic.castSpell(text, params);
	if (spell == nullptr) {
		return SPELL_NONE;
	}

	if (spell->has_param) {
		replaceString(params, "\"", "");
	}

	if (spell->spell_type != CHARACTER_RIGHT_SPELL) {
		if (player->earliest_spell_time >= serverMilliseconds()) {
			Protocol::sendResult(player->connection_ptr, YOUAREEXHAUSTED);
			announceGraphicalEffect(player, 3);
			return SPELL_FAILED;
		}
	}

	switch (spell->spell_type) {
		case NORMAL_SPELL: {
			switch (spell->id) {
				case 1: { // light healing
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.heal(player, Combat::computeDamage(player, 20, 10, true));
					break;
				}
				case 2: { // intense healing
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.heal(player, Combat::computeDamage(player, 40, 20, true));
					break;
				}
				case 3: { // ultimate healing
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.heal(player, Combat::computeDamage(player, 250, 50, true));
					break;
				}
				case 6: { // haste
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.magicGoStrength(player, player, 30, 3);
					break;
				}
				case 10: { // light
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.enlight(player, 8, 500);
					break;
				}
				case 11: { // great light
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.enlight(player, 8, 1000);
					break;
				}
				case 13: { // energy wave
					if (!Combat::angleCombat(player, spell->mana, spell->soulpoints, Combat::computeDamage(player, 250, 50, true), 11, 5, 3, DAMAGE_ENERGY)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 19: { // fire wave
					if (!g_map.throwPossible(player->getPosition(), Position::getNextPosition(player->getPosition(), player->getLookDirection()))) {
						return SPELL_FAILED;
					}

					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 2000)) {
						return SPELL_FAILED;
					}

					std::list<uint32_t> area = {
						1, 1, 1, 1, 1,
					    0, 1, 1, 1, 0,
						0, 1, 1, 1, 0,
						0, 0, 3, 0, 0,
					};

					DamageImpact impact;
					impact.actor = player;
					impact.allow_defense = true;
					impact.power = Combat::computeDamage(player, 30, 10, true);
					impact.damage_type = DAMAGE_FIRE;
					Combat::customShapeSpell(area, 4, player->getPosition(), 7, 0, impact);
					break;
				}
				case 20: { // find person
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					findPerson(player, false, params);
					break;
				}
				case 22: { // energy beam
					if (!Combat::angleCombat(player, spell->mana, spell->soulpoints, Combat::computeDamage(player, 60, 20, true), 6, 8, 0, DAMAGE_ENERGY)) {
						return SPELL_FAILED;
					}
					break;
				}
				case 23: { // great energy beam
					if (!Combat::angleCombat(player, spell->mana, spell->soulpoints, Combat::computeDamage(player, 120, 80, true), 6, 8, 0, DAMAGE_ENERGY)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 24: { // ultimate explosion
					if (!Combat::massCombat(player, player, spell->mana, spell->soulpoints, Combat::computeDamage(player, 250, 50, true), 5, 6, DAMAGE_PHYSICAL, 4)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 29: { // antidote
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.negatePoison(player);
					break;
				}
				// TODO: spell 38 *utevo res ina*
				case 39: { // great haste
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.magicGoStrength(player, player, 70, 2);
					break;
				}
				case 42: { // food
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.createFood(player);
					break;
				}
				case 44: { // magic shield
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.magicShield(player, 200);
					break;
				}
				case 45: { // invisible
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.invisibility(player, 200);
					break;
				}
				case 48: { // poisoned arrow
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					createItemAtPlayer(player, g_items.getItemType(3448), 5);
					g_game.announceGraphicalEffect(player, 13);
					break;
				}
				case 49: { // explosive arrow
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					createItemAtPlayer(player, g_items.getItemType(3449), 3);
					g_game.announceGraphicalEffect(player, 13);
					break;
				}
				case 51: { // conjure arrow
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					createItemAtPlayer(player, g_items.getItemType(3447), 10);
					g_game.announceGraphicalEffect(player, 13);
					break;
				}
				case 56: { // poison storm
					if (!Combat::massCombat(player, player, spell->mana, spell->soulpoints, Combat::computeDamage(player, 200, 50, true), 9, 8, DAMAGE_POISONING, 0)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 75: { // ultimate light
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					g_magic.enlight(player, 9, 2000);
					break;
				}
				case 76: { // magic rope
					if (!g_magic.magicRope(player)) {
						return SPELL_FAILED;
					}

					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 79: { // conjure  bolt
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					createItemAtPlayer(player, g_items.getItemType(3446), 5);
					g_game.announceGraphicalEffect(player, 13);
					break;
				}
				case 80: { // berserk
					int32_t power = Combat::computeDamage(player, 80, 20, true);
					power *= player->skill_level[SKILL_LEVEL] / 25;

					if (!Combat::massCombat(player, player, 4 * player->skill_level[SKILL_LEVEL], spell->soulpoints, power, 10, 3, DAMAGE_PHYSICAL, 0)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 81: { // levitate
					if (!g_magic.magicClimbing(player, params, spell->mana, spell->soulpoints)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 82: { // mass healing
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					HealingImpact impact;
					impact.actor = player;
					impact.power = Combat::computeDamage(player, 200, 40, true);
					Combat::circleShapeSpell(player->getPosition(), player->getPosition(), 4, 13, 0, impact);
					break;
				}
				case 84: { // heal friend
					Player* target = nullptr;
					int32_t hits = identifyPlayer(&target, false, params);
					if (hits > 1) {
						Protocol::sendResult(player->connection_ptr, NAMEISTOOAMBIGIOUS);
						return SPELL_FAILED;
					} else if (hits == 0) {
						Protocol::sendResult(player->connection_ptr, PLAYERWITHTHISNAMEISNOTONLINE);
						return SPELL_FAILED;
					}

					if (!g_map.throwPossible(player->getPosition(), target->getPosition()) || player->getPosition().z != target->getPosition().z || !player->canSeeCreature(target)) {
						Protocol::sendResult(player->connection_ptr, DESTINATIONOUTOFREACH);
						return SPELL_FAILED;
					}

					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					announceGraphicalEffect(player, 13);

					int32_t power = Combat::computeDamage(player, 120, 40, true);

					if (target->skill_hitpoints->getValue() > 0) {
						target->skill_hitpoints->change(power);
					}

					if (target->skill_go_strength->getDelta() < 0) {
						target->skill_go_strength->setTiming(0, 0, 0, 0);
					}

					announceGraphicalEffect(player, 15);
					break;
				}
				case 87: { // force strike
					if (!Combat::angleCombat(player, spell->mana, spell->soulpoints, Combat::computeDamage(player, 45, 10, true), 18, 1, 0, DAMAGE_PHYSICAL)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 88: { // energy strike
					if (!Combat::angleCombat(player, spell->mana, spell->soulpoints, Combat::computeDamage(player, 45, 10, true), 11, 1, 0, DAMAGE_ENERGY)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 89: { // flame strike
					if (!Combat::angleCombat(player, spell->mana, spell->soulpoints, Combat::computeDamage(player, 45, 10, true), 7, 1, 0, DAMAGE_FIRE)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 90: { // cancel invisibility
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 2000)) {
						return SPELL_FAILED;
					}

					CancelInvisibleImpact impact;
					impact.actor = player;
					Combat::circleShapeSpell(player->getPosition(), player->getPosition(), 4, 13, 0, impact);
					break;
				}
				case 92: { // enchant staff
					if (!playerEnchantItem(player, g_items.getItemType(3289), g_items.getItemType(3321), spell->mana, spell->soulpoints)) {
						return SPELL_FAILED;
					}

					break;
				}
				case 94: { // wild growth
					if (!g_map.throwPossible(player->getPosition(), Position::getNextPosition(player->getPosition(), player->getLookDirection()))) {
						Protocol::sendResult(player->connection_ptr, NOTENOUGHROOM);
						return SPELL_FAILED;
					}

					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					FieldImpact impact;
					impact.actor = player;
					impact.field_type = FIELD_WILD_GROWTH;
					Combat::angleShapeSpell(player->getPosition(), 1, 0, 0, impact);
					break;
				}
				case 95: { // conjure power bolt
					if (!g_magic.checkMana(player, spell->mana, spell->soulpoints, 1000)) {
						return SPELL_FAILED;
					}

					createItemAtPlayer(player, g_items.getItemType(3450), 1);
					g_game.announceGraphicalEffect(player, 13);
					break;
				}
				default:
					fmt::printf("INFO - Game::playerCastSpell: unhandled spell id (%d).\n", spell->id);
					break;
			}
			break;
		}
		case RUNE_SPELL: {
			if (playerCastRuneSpell(player, spell, text) != SPELL_SUCCESS) {
				return SPELL_FAILED;
			}

			break;
		}
		case CHARACTER_RIGHT_SPELL: playerCastCharacterRightSpell(player, spell, text, params); return SPELL_FAILED;
		default: break;
	}

	return SPELL_SUCCESS;
}

SpellCastResult_t Game::playerCastRuneSpell(Player* player, Spell* spell, const std::string& text)
{
	if (spell == nullptr) {
		fmt::printf("ERROR - Game::playerCastRuneSpell: spell is null.\n");
		return SPELL_NONE; 
	}

	if (spell->rune_nr == 0) {
		fmt::printf("ERROR - Game::playerCastRuneSpell: spell has no rune (%d)\n", spell->id);
		return SPELL_NONE; 
	}

	Item* left_item = player->inventory[INVENTORY_LEFT];
	Item* right_item = player->inventory[INVENTORY_RIGHT];

	const ItemType* type = g_items.getSpecialItem(SPECIAL_RUNE_BLANK);
	if (type == nullptr) {
		fmt::printf("ERROR - Game::playerCastRuneSpell: RUNE_BLANK meaning is 0.\n");
		return SPELL_NONE;
	}

	bool executed = false;
	if (left_item) {
		if (left_item->getId() == type->type_id) {
			if (!g_magic.checkSpellReq(player, spell)) {
				announceGraphicalEffect(player, 3);
				return SPELL_FAILED;	
			}

			g_magic.takeMana(player, spell->mana);
			g_magic.takeSoulpoints(player, spell->soulpoints);
			g_game.changeItem(left_item, spell->rune_nr, spell->amount);
			executed = true;
		}
	}

	if (right_item) {
		if (right_item->getId() == type->type_id) {
			if (!g_magic.checkSpellReq(player, spell)) {
				announceGraphicalEffect(player, 3);
				return SPELL_FAILED;	
			}

			g_magic.takeMana(player, spell->mana);
			g_magic.takeSoulpoints(player, spell->soulpoints);
			g_game.changeItem(right_item, spell->rune_nr, spell->amount);
			executed = true;
		}
	}

	if (!executed) {
		Protocol::sendResult(player->connection_ptr, YOUNEEDAMAGICITEMTOCASTSPELL);
		announceGraphicalEffect(player, 3);
		return SPELL_FAILED;
	}

	g_game.announceGraphicalEffect(player, 14);
	return SPELL_SUCCESS;
}

void Game::playerCastCharacterRightSpell(Player* player, Spell* spell, const std::string& text, const std::string& params)
{
	if (spell == nullptr) {
		fmt::printf("ERROR - Game::playerCastCharacterRightSpell: spell is null.\n");
		return;
	}

	switch (spell->id) {
		case 34: { // Get Item
			playerCreateItem(player, params); 
			break;
		}
		case 37: { // Teleport
			playerTeleportSpell(player, params);
			break;
		}
		case 47: // teleport to friend
			playerTeleportToFriend(player, params);
			break;
		case 52: // retrieve friend
			playerRetrieveFriend(player, params);
			break;
		case 58: { // get position
			const Position& pos = player->getPosition();
			Protocol::sendTextMessage(player->connection_ptr, MESSAGE_OBJECT_INFO, fmt::sprintf("Your position is [%d,%d,%d].", pos.x, pos.y, pos.z));
			break;
		}
		case 60: // temple teleport
			g_game.playerHomeTeleport(player, params);
			break;
		case 63: // create gold
			g_magic.createGold(player, getNumber(params));
			break;
		case 64: // change profession or sex
			playerChangeSpecial(player, params);
			break;
		case 67: // kick player
			playerKickPlayer(player, params);
			break;
		case 103: // Add Mana
			player->changeManaPoints(getNumber(params));
			break;
		case 104: { // Remove Experience
			std::vector<std::string> real_params = explodeString(params, ",");
			if (real_params.size() != 2) {
				Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, "Not enough parameters.");
				return;
			}

			const SkillType_t skill = getSkillType(real_params[0].c_str());
			const uint64_t value = getLong(real_params[1].c_str());
			player->removeExperience(skill, value);
			break;
		}
		case 105: { // Add Experience
			std::vector<std::string> real_params = explodeString(params, ",");
			if (real_params.size() != 2) {
				Protocol::sendTextMessage(player->connection_ptr, MESSAGE_STATUS_SMALL, "Not enough parameters.");
				return;
			}

			const SkillType_t skill = getSkillType(real_params[0].c_str());
			const uint64_t value = getLong(real_params[1].c_str());
			player->addExperience(skill, value);
			break;
		}
		default:
			fmt::printf("INFO - Game::playerCastCharacterRightSpell: unhandled spell (%d).\n", spell->id);
			break;
	}
}

void Game::playerCreateItem(Player* player, const std::string& text)
{
	if (player == nullptr) {
		fmt::printf("ERROR - Game::playerCreateItem: player is null.\n");
		return;
	}

	const std::vector<std::string> params = explodeString(text, ",");
	if (params.size() == 0) {
		return;	
	}

	uint16_t amount = 1;
	if (params.size() >= 2) {
		amount = getNumber(params[1]);
	}

	std::string type_name;
	uint16_t type_id = getNumber(params[0]);

	if (type_id == 0) {
		type_name = lowerString(params[0]);
	}

	const ItemType* item_type = nullptr;
	if (type_id) {
		item_type = g_items.getItemType(type_id);
	} else if (!type_name.empty()) {
		item_type = g_items.getItemTypeByName(type_name);
	}

	if (item_type == nullptr) {
		Protocol::sendTextMessage(player->connection_ptr, 23, "There is no such takable object.");
		return;
	}

	if (amount > 100) {
		Protocol::sendTextMessage(player->connection_ptr, 23, "You may only create 1 to 100 objects.");
		return;
	}

	if (amount == 0 || !item_type->getFlag(TAKE)) {
		Protocol::sendTextMessage(player->connection_ptr, 23, "You may only create one untakeable object.");
		return;
	}

	if (item_type->getFlag(CUMULATIVE)) {
		createItemAtPlayer(player, item_type, amount);
	} else {
		for (int32_t i = 0; i < amount; i++) {
			createItemAtPlayer(player, item_type, 1);
		}
	}

	announceGraphicalEffect(player, 15);
}

void Game::playerTeleportSpell(Player* player, const std::string& text)
{
	int32_t x, y, z;
	const std::string& param1 = text;
	if (param1 == "up" || param1 == "down") {
		Position pos = player->getPosition();

		if (param1 == "up") {
			pos.z--;
		} else {
			pos.z++;
		}

		if (!g_map.searchFreeField(player, pos, 1)) {
			Protocol::sendResult(player->connection_ptr, NOTENOUGHROOM);
			return;
		}

		moveCreature(player, pos.x, pos.y, pos.z, FLAG_NOLIMIT);
		announceGraphicalEffect(player, 11);
	} else if (param1 == "fast") {
		player->skill_go_strength->setDelta(100);
		announceGraphicalEffect(player, 13);
	} else if (param1 == "fastest") {
		player->skill_go_strength->setDelta(200);
		announceGraphicalEffect(player, 13);
	} else if (param1 == "slow" || param1 == "normal") {
		player->skill_go_strength->setDelta(0);
		announceGraphicalEffect(player, 13);
	} else if (sscanf(param1.c_str(), "%d,%d,%d", &x, &y, &z) == 3 || sscanf(param1.c_str(), "[%d,%d,%d]", &x, &y, &z) == 3) {
		if (!g_map.searchFreeField(player, x, y, z, 1)) {
			Protocol::sendResult(player->connection_ptr, NOTENOUGHROOM);
			return;
		}

		moveCreature(player, x, y, z, FLAG_NOLIMIT);
	} else {
		const auto it = map_points.find(param1);
		if (it == map_points.end()) {
			Protocol::sendTextMessage(player->connection_ptr, 23, "There is no mark of this name.");
			return;
		}

		const Position pos = it->second;
		if (!g_map.searchFreeField(player, pos.x, pos.y, pos.z, 1)) {
			Protocol::sendResult(player->connection_ptr, NOTENOUGHROOM);
			return;
		}

		moveCreature(player, pos.x, pos.y, pos.z, FLAG_NOLIMIT);
	}
}

void Game::notifyPlayer(Player* player, const Item* item, bool inventory)
{
	if (item->getFlag(LIGHT) && inventory) {
		announceChangedCreature(player, CREATURE_LIGHT);
	}

	Protocol::sendStats(player->connection_ptr);
}

void Game::notifyTrades(const Item* item)
{
	const Position& pos = item->getPosition();

	std::unordered_set<Creature*> spectator_list;
	getSpectators(spectator_list, pos.x, pos.y, 12, 10, true);

	for (Creature* creature : spectator_list) {
		if (Player* player = creature->getPlayer()) {
			if (!player->trading_item || player->trade_partner == nullptr) {
				continue;
			}

			const Item* trading_item = player->trading_item;
			if (trading_item == item) {
				closeTrade(player);
				return;
			}

			const Item* container = dynamic_cast<const Item*>(item->getParent());
			while (container) {
				if (container == trading_item) {
					closeTrade(player);
					return;
				}

				container = dynamic_cast<const Item*>(container->getParent());
			}
		}
	}
}

void Game::closeRuleViolationReport(Player* player)
{
	const auto it = player_requests.find(player->getId());
	if (it == player_requests.end()) {
		return;
	}

	player_requests.erase(it);

	Protocol::sendLockRuleViolation(player->connection_ptr);

	Channel* channel = g_channels.getChannelById(CHANNEL_RULEVIOLATIONS);
	if (channel == nullptr) {
		fmt::printf("ERROR - Game::playerReportRuleViolation: channel is null.\n");
		return;
	}

	for (Player* gamemaster : channel->getSubscribers()) {
		Protocol::sendRemoveRuleViolationReport(gamemaster->connection_ptr, player->getName());
	}
}

void Game::insertChainCreature(Creature* creature, int32_t x, int32_t y)
{
	const int32_t block_id = g_map.creature_chain.dx * ((y / 32 + y % 32) - g_map.creature_chain.ymin) + (x / 32 + x % 32) - g_map.creature_chain.xmin;
	const int32_t first_chain_creature = g_map.creature_chain.entry[block_id];

	if (first_chain_creature == 0) {
		g_map.creature_chain.entry[block_id] = creature->id;
		return;
	}

	Creature* next_chain_creature = getCreatureById(first_chain_creature);
	if (!next_chain_creature) {
		fmt::printf("ERROR - Game::insertChainCreature: valid creature number but creature is null (%d,%d):%d\n", x, y, block_id);
	}

	creature->next_chain_creature = next_chain_creature;
	g_map.creature_chain.entry[block_id] = creature->id;
}

void Game::moveChainCreature(Creature* creature, int32_t x, int32_t y)
{
	removeChainCreature(creature);
	insertChainCreature(creature, x, y);
}

void Game::removeChainCreature(Creature* creature)
{
	const Position& pos = creature->getPosition();
	const int32_t x = pos.x;
	const int32_t y = pos.y;

	const int32_t block_id = g_map.creature_chain.dx * ((y / 32 + y % 32) - g_map.creature_chain.ymin) + (x / 32 + x % 32) - g_map.creature_chain.xmin;
	const int32_t first_chain_creature = g_map.creature_chain.entry[block_id];

	if (first_chain_creature == creature->id) {
		if (creature->next_chain_creature) {
			g_map.creature_chain.entry[block_id] = creature->next_chain_creature->id;
			creature->next_chain_creature = nullptr;
		} else {
			g_map.creature_chain.entry[block_id] = 0;
		}

		return;
	}

	Creature* next_chain_creature = getCreatureById(first_chain_creature);
	Creature* prev_chain_creature = next_chain_creature;

	while (next_chain_creature) {
		if (next_chain_creature == creature) {
			prev_chain_creature->next_chain_creature = creature->next_chain_creature;
			creature->next_chain_creature = nullptr;
			return;
		}

		prev_chain_creature = next_chain_creature;
		next_chain_creature = next_chain_creature->next_chain_creature;
	}
}

void Game::indexPlayer(const Player* player)
{
	player_ids[player->getName()] = player->getId();
	player_names_index.emplace(player->getName());
}

uint32_t Game::appendStatement(Player* player, const std::string& text, uint16_t channel_id, TalkType_t type) const
{
	static uint32_t next_statement_id = 0;

	PlayerStatement statement;
	statement.channel_id = channel_id;
	statement.player = player;
	statement.reported = false;
	statement.statement_id = next_statement_id++;
	statement.text = text;
	statement.timestamp = serverMilliseconds();
	statement.type = type;
	return statement.statement_id;
}

void Game::advanceGame(int32_t delay)
{
	// advance game
	decay_time_counter += delay;
	skill_time_counter += delay;
	other_time_counter += delay;
	creature_time_counter += delay;

	if (decay_time_counter > 250) {
		decay_time_counter -= 250;
		processItems(DECAY_SIZE * 250);
	}

	if (skill_time_counter > 100) {
		skill_time_counter -= 100;
		processSkills();
	}

	if (other_time_counter > 999) {
		other_time_counter -= 1000;
		round_number++;

		// world ambiente
		uint8_t brightness, color;
		getAmbiente(brightness, color);
		if (brightness != old_ambiente) {
			old_ambiente = brightness;
			for (Player* player : players) {
				Protocol::sendAmbiente(player->connection_ptr);
			}
		}

		processConnections();
	}

	if (creature_time_counter > 1749) {
		creature_time_counter -= 1000;

		processCreatures();
	}

	if (delay > 999) {
		fmt::printf("Game is lagging with %d ms.\n", delay);
	} else {
		moveCreatures();
	}
}

void Game::processConnections()
{
	std::list<Creature*> aux_removed_list;
	for (Player* player : players) {
		bool connected = true;
		const uint64_t time_now = serverMilliseconds();

		if (player->last_ping == 0) {
			player->last_ping = time_now;
			player->last_pong = time_now;
		}

		if ((time_now - player->last_ping) >= 5000) {
			player->last_ping = time_now;

			if (player->connection_ptr) {
				Protocol::sendPing(player->connection_ptr);
			} else {
				connected = false;
			}
		}

		if (round_number - 900 == player->timestamp_action) {
			Protocol::sendTextMessage(player->connection_ptr, MESSAGE_WARNING, fmt::sprintf("You have been idle for %d minutes. You will be disconnected if you are still idle then.\n", 15));
		} else if (round_number - player->timestamp_action == 959) {
			connected = false;
		}

		const uint64_t no_pong_time = time_now - player->last_pong;
		if (!connected || no_pong_time >= 60000) {
			aux_removed_list.push_back(player);
		}
	}

	for (Creature* creature : aux_removed_list) {
		removeCreature(creature);
	}

	connection_mutex.lock();
	for (auto it = connections.begin(); it != connections.end();) {
		const Connection_ptr connection = *it;
		if (connection->getState() == CONNECTION_STATE_CLOSED) {
			it = connections.erase(it);
		} else {
			++it;
		}
	}
	connection_mutex.unlock();
}

void Game::moveCreatures()
{
	while (game_state >= GAME_RUNNING) {
		if (creature_queue.empty()) {
			break;
		}

		const CreatureEntry entry = creature_queue.top();
		if (entry.interval > serverMilliseconds()) {
			break;
		}
		creature_queue.pop();

		entry.creature->execute();
	}
}

void Game::processItems(int32_t interval)
{
	if (last_decay_bucket > DECAY_SIZE - 1) {
		last_decay_bucket = 0;
	}

	for (auto it = decaying_list[last_decay_bucket].begin(); it != decaying_list[last_decay_bucket].end();) {
		Item* item = *it;
		if (item->isRemoved() || !item->decaying || !item->getFlag(EXPIRE)) {
			it = decaying_list[last_decay_bucket].erase(it);
			continue;
		}

		item->setAttribute(ITEM_REMAINING_EXPIRE_TIME, item->getAttribute(ITEM_REMAINING_EXPIRE_TIME) - interval);
		if (item->getAttribute(ITEM_REMAINING_EXPIRE_TIME) <= 0) {
			const int32_t target = item->getAttribute(EXPIRETARGET);
			if (target == 0) {
				item->decaying = false;
				removeItem(item, item->getAttribute(ITEM_AMOUNT));
				it = decaying_list[last_decay_bucket].erase(it);
				continue;
			}

			changeItem(item, target, 0);
		}
			
		++it;
	}

	last_decay_bucket++;
}

void Game::processSkills()
{
	if (last_creature_bucket > CREATURE_SKILL_SIZE - 1) {
		last_creature_bucket = 0;
	}

	std::vector<Creature*>& list = creature_skills[last_creature_bucket];
	for (auto it = list.begin(); it != list.end();) {
		Creature* c = *it;
		if (c->skill_processing) {
			c->process();
			++it;
		} else {
			it = list.erase(it);
		}
	}

	last_creature_bucket++;
}

void Game::processCreatures()
{
	for (Creature* creature : creatures) {
		if (creature->skill_hitpoints->getValue() <= 0 && !creature->is_dead) {
			fmt::printf("ERROR - Game::processCreatures: creature has no hitpoints and is not set as dead.\n");
			creature->is_dead = true;
		}

		if (creature->is_dead) {
			creature->onDeath();
			removeCreature(creature);
		}

		if (Player* player = creature->getPlayer()) {
			player->checkState();
			if (player->earliest_logout_round <= round_number) {
				player->clearKillingMarks();
				player->earliest_logout_round = 0;
				player->earliest_protection_zone_round = 0;
			}
		}
	}
}

void Game::receiveData()
{
	connection_mutex.lock();
	for (Connection_ptr connection : connections) {
		if (connection->pending_data > 0) {
			connection->parseData();
		}
	}
	connection_mutex.unlock();
}

void Game::sendData()
{
	connection_mutex.lock();
	for (Connection_ptr connection : connections) {
		connection->sendAll();
	}
	connection_mutex.unlock();
}

void Game::releaseObjects()
{
	for (Creature* creature : removed_creatures) {
		delete creature;
	}
	removed_creatures.clear();
}

void Game::releaseItem(Item* item)
{
	g_itempool.freeItem(item);
}

void Game::releaseCreature(Creature* creature)
{
	if (creature->removed) {
		fmt::printf("ERROR - Game::releaseCreature: creature is already removed (%s)\n", creature->getName());
		return;
	}

	creature->removed = true;
	removed_creatures.push_back(creature);
}
