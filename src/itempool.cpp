#include "pch.h"

#include "itempool.h"
#include "item.h"

ItemPool::~ItemPool()
{
	while (!items.empty()) {
		delete items.back();
		items.pop_back();
	}
}

void ItemPool::allocate(uint32_t count)
{
	if (!items.empty()) {
		fmt::printf("ERROR - ItemPool::allocate: pool already allocated.\n");
		return;
	}

	for (int32_t i = count; i != 0; i--) {
		Item* item = new Item();
		items.push_back(item);
		free_items.push_front(item);
	}
}

Item* ItemPool::createItem(uint16_t type_id)
{
	if (free_items.empty()) {
		reallocate();
	}

	Item* item = free_items.front();
	if (!item->removed) {
		fmt::printf("ERROR - ItemPool::createItem: item is not free (%s)\n", item->getName(-1));
	}

	if (!item->createItem(type_id)) {
		return nullptr;
	}

	item->removed = false;
	free_items.pop_front();
	return item;
}

void ItemPool::freeItem(Item* item)
{
	if (item->removed) {
		fmt::printf("ERROR - ItemPool::deleteItem: item is already removed (%s).\n", item->getName(-1));
		return;
	}

	item->attributes = {};
	item->removed = true;
	item->decaying = false;
	free_items.push_front(item);
}

void ItemPool::reallocate()
{
	fmt::printf("INFO - ItemPool:: reallocating %d total new items, consider increasing item pool to %d.\n", items.size(), items.size() * 2);

	int32_t size = items.size();
	for (int32_t i = 0; i != size; i++) {
		Item* item = new Item();
		items.push_back(item);
		free_items.push_front(item);
	}
}
