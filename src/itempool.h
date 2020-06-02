#pragma once

class Item;

class ItemPool
{
public:
	explicit ItemPool() = default;
	~ItemPool();

	void allocate(uint32_t count);

	Item* createItem(uint16_t type_id);
	void freeItem(Item* item);
private:
	void reallocate();

	std::vector<Item*> items;
	std::forward_list<Item*> free_items;
};

extern ItemPool g_itempool;