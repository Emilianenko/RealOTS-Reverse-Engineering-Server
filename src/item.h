#pragma once

#include "cylinder.h"
#include "enums.h"

#include <deque>

struct ItemType
{
	ItemType();

	uint16_t type_id;
	std::string name;
	std::string description;

	bool flags[ITEM_FLAGS_SIZE];
	int32_t attributes[ITEM_ATTRIBUTES_SIZE];

	bool getFlag(ItemFlags_t flag) const {
		return flags[flag];
	}

	int32_t getAttribute(ItemAttribute_t attribute) const {
		return attributes[attribute];
	}
};

class Items
{
public:
	bool loadItems();

	const ItemType* getItemType(uint16_t type_id) const {
		if (type_id < 100 || type_id > 5999) {
			return nullptr;
		}

		return &items[type_id];
	}

	const ItemType* getItemTypeByName(const std::string& name) const;
	ItemType* getSpecialItem(SpecialMeaning_t meaning) const;
private:
	ItemType items[6000];
	std::map<uint8_t, ItemType*> special_items;
};

extern Items g_items;

class Item;

class ScriptReader;
class Game;

class ItemAttributes
{
private:
	struct ItemAttribute
	{
		int64_t value = 0;
	};

public:
	~ItemAttributes() {
		for (ItemAttribute* attr : attributes) {
			delete attr;
		}
	}

	void setAttribute(ItemInstance_t attribute, int64_t value) {
		if (attributes[attribute] == nullptr) {
			attributes[attribute] = new ItemAttribute();
		}

		attributes[attribute]->value = value;
	}
	int64_t getAttribute(ItemInstance_t attribute) const {
		if (attributes[attribute] == nullptr) {
			return 0;
		}

		return attributes[attribute]->value;
	}
private:

	std::array<ItemAttribute*, ITEM_INSTANCE_SIZE> attributes;
};

class ItemPool;

class Item : public Cylinder
{
protected:
	explicit Item() = default;
	bool createItem(uint16_t type_id);
public:
	//static Item* createItem(uint16_t type_id);

	//Item(const Item&) = delete; // non construction-copyable
	//Item& operator=(const Item&) = delete; // non copyable

	virtual ~Item();

	Item* getItem() final {
		return this;
	}

	const Item* getItem() const final {
		return this;
	}

	uint16_t getId() const {
		return item_type->type_id;
	}

	std::string getName(int32_t count) const;
	const std::string& getDescription() const;

	void setAttribute(ItemInstance_t attr, int64_t value) {
		attributes.setAttribute(attr, value);
	}
	int64_t getAttribute(ItemInstance_t attr) const {
		return attributes.getAttribute(attr);
	}

	void setText(const std::string& new_text) {
		text = new_text;
	}
	const std::string& getText() const {
		return text;
	}

	void setEditor(const std::string& new_editor) {
		editor = new_editor;
	}
	const std::string& getEditor() const {
		return editor;
	}

	bool isRemoved() const final {
		return removed;
	}

	std::string getObjectDescription(const Creature* creature) const final;

	uint32_t getHoldingItemCount() const;
	uint32_t getWeight() const;

	const std::deque<Item*>& getItems() const {
		return items;
	}

	bool getFlag(ItemFlags_t flag) const {
		return item_type->getFlag(flag);
	}

	int32_t getAttribute(ItemAttribute_t attribute) const {
		return item_type->getAttribute(attribute);
	}

	bool hasParentContainer() const;

	bool loadData(ScriptReader& script);
	bool loadContent(ScriptReader& script);

	ReturnValue_t queryAdd(int32_t index, const Object* object, uint32_t amount, uint32_t flags, Creature* actor) const final;
	ReturnValue_t queryMaxCount(int32_t index, const Object* object, uint32_t amount, uint32_t& max_amount, uint32_t flags) const final;
	Cylinder* queryDestination(int32_t& index, const Object* object, Item** to_item, uint32_t& flags) final;
	
	void addObject(Object* object, int32_t index) final;
	void removeObject(Object* object) final;

	uint8_t getObjectPriority() const final;
	int32_t getObjectIndex(const Object* object) const final;
	Object* getObjectIndex(uint32_t index) const final;
	Item* getitemIndex(uint32_t index) const;

	bool isHoldingItem(Item* item) const;
protected:
	const ItemType* item_type = nullptr;

	std::deque<Item*> items{};

	std::string text;
	std::string editor;

	bool removed = true;
	bool decaying = false;

	ItemAttributes attributes;

	friend class Game;
	friend class ItemPool;
};