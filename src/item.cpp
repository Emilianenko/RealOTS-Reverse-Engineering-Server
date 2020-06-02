#include "pch.h"

#include "item.h"
#include "script.h"
#include "player.h"
#include "tools.h"
#include "game.h"
#include "magic.h"
#include "itempool.h"

ItemType::ItemType(): type_id(0), flags{}
{
	for (int32_t index = 0; index <= ITEM_ATTRIBUTES_SIZE; index++)
	{
		attributes[index] = -1;
	}
}

bool Items::loadItems()
{
	ScriptReader script;
	if (!script.loadScript("dat/objects.srv")) {
		return false;
	}

	TOKENTYPE token = script.nextToken();
	while (script.canRead()) {
		if (token == TOKEN_IDENTIFIER) {
			std::string identifier = script.getIdentifier();
			script.readSymbol('=');
			if (identifier == "typeid") {
				const uint16_t type_id = script.readNumber();
				if (type_id < 100 || type_id > 5999) {
					script.error("invalid Type id");
					return false;
				}

				ItemType& item_type = items[type_id];
				item_type.type_id = type_id;

				while (script.canRead()) {
					token = script.nextToken();
					if (token == TOKEN_IDENTIFIER) {
						identifier = script.getIdentifier();

						if (identifier == "name") {
							script.readSymbol('=');
							item_type.name = script.readString();
						} else if (identifier == "description") {
							script.readSymbol('=');
							item_type.description = script.readString();
						} else if (identifier == "flags") {
							script.readSymbol('=');
							script.readSymbol('{');
							while (script.canRead()) {
								token = script.nextToken();
								if (token == TOKEN_IDENTIFIER) {
									identifier = script.getIdentifier();

									if (identifier == "bank") {
										item_type.flags[BANK] = true;
									} else if (identifier == "clip") {
										item_type.flags[CLIP] = true;
									} else if (identifier == "bottom") {
										item_type.flags[BOTTOM] = true;
									} else if (identifier == "top") {
										item_type.flags[TOP] = true;
									} else if (identifier == "container") {
										item_type.flags[CONTAINER] = true;
									} else if (identifier == "chest") {
										item_type.flags[CHEST] = true;
									} else if (identifier == "cumulative") {
										item_type.flags[CUMULATIVE] = true;
									} else if (identifier == "useevent") {
										item_type.flags[USEEVENT] = true;
									} else if (identifier == "changeuse") {
										item_type.flags[CHANGEUSE] = true;
									} else if (identifier == "forceuse") {
										item_type.flags[FORCEUSE] = true;
									} else if (identifier == "multiuse") {
										item_type.flags[MULTIUSE] = true;
									} else if (identifier == "distuse") {
										item_type.flags[DISTUSE] = true;
									} else if (identifier == "movementevent") {
										item_type.flags[MOVEMENTEVENT] = true;
									} else if (identifier == "collisionevent") {
										item_type.flags[COLLISIONEVENT] = true;
									} else if (identifier == "separationevent") {
										item_type.flags[SEPARATIONEVENT] = true;
									} else if (identifier == "key") {
										item_type.flags[KEY] = true;
									} else if (identifier == "keydoor") {
										item_type.flags[KEYDOOR] = true;
									} else if (identifier == "namedoor") {
										item_type.flags[NAMEDOOR] = true;
									} else if (identifier == "leveldoor") {
										item_type.flags[LEVELDOOR] = true;
									} else if (identifier == "questdoor") {
										item_type.flags[QUESTDOOR] = true;
									} else if (identifier == "bed") {
										item_type.flags[BED] = true;
									} else if (identifier == "food") {
										item_type.flags[FOOD] = true;
									} else if (identifier == "rune") {
										item_type.flags[RUNE] = true;
									} else if (identifier == "information") {
										item_type.flags[INFORMATION] = true;
									} else if (identifier == "text") {
										item_type.flags[TEXT] = true;
									} else if (identifier == "write") {
										item_type.flags[WRITE] = true;
									} else if (identifier == "writeonce") {
										item_type.flags[WRITEONCE] = true;
									} else if (identifier == "liquidcontainer") {
										item_type.flags[LIQUIDCONTAINER] = true;
									} else if (identifier == "liquidsource") {
										item_type.flags[LIQUIDSOURCE] = true;
									} else if (identifier == "liquidpool") {
										item_type.flags[LIQUIDPOOL] = true;
									} else if (identifier == "teleportabsolute") {
										item_type.flags[TELEPORTABSOLUTE] = true;
									} else if (identifier == "teleportrelative") {
										item_type.flags[TELEPORTRELATIVE] = true;
									} else if (identifier == "unpass") {
										item_type.flags[UNPASS] = true;
									} else if (identifier == "unmove") {
										item_type.flags[UNMOVE] = true;
									} else if (identifier == "unthrow") {
										item_type.flags[UNTHROW] = true;
									} else if (identifier == "unlay") {
										item_type.flags[UNLAY] = true;
									} else if (identifier == "avoid") {
										item_type.flags[AVOID] = true;
									} else if (identifier == "magicfield") {
										item_type.flags[MAGICFIELD] = true;
									} else if (identifier == "restrictlevel") {
										item_type.flags[RESTRICTLEVEL] = true;
									} else if (identifier == "restrictprofession") {
										item_type.flags[RESTRICTPROFESSION] = true;
									} else if (identifier == "take") {
										item_type.flags[TAKE] = true;
									} else if (identifier == "hang") {
										item_type.flags[HANG] = true;
									} else if (identifier == "hooksouth") {
										item_type.flags[HOOKSOUTH] = true;
									} else if (identifier == "hookeast") {
										item_type.flags[HOOKEAST] = true;
									} else if (identifier == "rotate") {
										item_type.flags[ROTATE] = true;
									} else if (identifier == "destroy") {
										item_type.flags[DESTROY] = true;
									} else if (identifier == "clothes") {
										item_type.flags[CLOTHES] = true;
									} else if (identifier == "skillboost") {
										item_type.flags[SKILLBOOST] = true;
									} else if (identifier == "protection") {
										item_type.flags[PROTECTION] = true;
									} else if (identifier == "light") {
										item_type.flags[LIGHT] = true;
									} else if (identifier == "ropespot") {
										item_type.flags[ROPESPOT] = true;
									} else if (identifier == "corpse") {
										item_type.flags[CORPSE] = true;
									} else if (identifier == "expire") {
										item_type.flags[EXPIRE] = true;
									} else if (identifier == "expirestop") {
										item_type.flags[EXPIRESTOP] = true;
									} else if (identifier == "wearout") {
										item_type.flags[WEAROUT] = true;
									} else if (identifier == "weapon") {
										item_type.flags[WEAPON] = true;
									} else if (identifier == "shield") {
										item_type.flags[SHIELD] = true;
									} else if (identifier == "bow") {
										item_type.flags[BOW] = true;
									} else if (identifier == "throw") {
										item_type.flags[THROWABLE] = true;
									} else if (identifier == "wand") {
										item_type.flags[WAND] = true;
									} else if (identifier == "ammo") {
										item_type.flags[AMMO] = true;
									} else if (identifier == "armor") {
										item_type.flags[ARMOR] = true;
									} else if (identifier == "height") {
										item_type.flags[HEIGHT] = true;
									} else if (identifier == "disguise") {
										item_type.flags[DISGUISE] = true;
									} else if (identifier == "showdetail") {
										item_type.flags[SHOWDETAIL] = true;
									} else if (identifier == "special") {
										item_type.flags[SPECIAL] = true;
									} else {
										script.error("unknown object flag");
										return false;
									}

									continue;
								} else if (token == TOKEN_SPECIAL) {
									if (script.getSpecial() == ',') {
										continue;
									} else if (script.getSpecial() == '}') {
										break;
									}
								}

								script.error("'}' expected");
								return false;
							}
						} else if (identifier == "attributes") {
							script.readSymbol('=');
							script.readSymbol('{');
							while (script.canRead()) {
								token = script.nextToken();
								if (token == TOKEN_IDENTIFIER) {
									identifier = script.getIdentifier();
									script.readSymbol('=');

									if (identifier == "waypoints") {
										item_type.attributes[WAYPOINTS] = script.readNumber();
									} else if (identifier == "capacity") {
										item_type.attributes[CAPACITY] = script.readNumber();
									} else if (identifier == "changetarget") {
										item_type.attributes[CHANGETARGET] = script.readNumber();
									} else if (identifier == "keydoortarget") {
										item_type.attributes[KEYDOORTARGET] = script.readNumber();
									} else if (identifier == "namedoortarget") {
										item_type.attributes[NAMEDOORTARGET] = script.readNumber();
									} else if (identifier == "leveldoortarget") {
										item_type.attributes[LEVELDOORTARGET] = script.readNumber();
									} else if (identifier == "questdoortarget") {
										item_type.attributes[QUESTDOORTARGET] = script.readNumber();
									} else if (identifier == "nutrition") {
										item_type.attributes[NUTRITION] = script.readNumber();
									} else if (identifier == "informationtype") {
										item_type.attributes[INFORMATIONTYPE] = script.readNumber();
									} else if (identifier == "fontsize") {
										item_type.attributes[FONTSIZE] = script.readNumber();
									} else if (identifier == "maxlength") {
										item_type.attributes[MAXLENGTH] = script.readNumber();
									} else if (identifier == "maxlengthonce") {
										item_type.attributes[MAXLENGTHONCE] = script.readNumber();
									} else if (identifier == "sourceliquidtype") {
										item_type.attributes[SOURCELIQUIDTYPE] = script.readNumber();
									} else if (identifier == "absteleporteffect") {
										item_type.attributes[ABSTELEPORTEFFECT] = script.readNumber();
									} else if (identifier == "relteleportdisplacement") {
										item_type.attributes[RELTELEPORTDISPLACEMENT] = script.readNumber();
									} else if (identifier == "relteleporteffect") {
										item_type.attributes[RELTELEPORTEFFECT] = script.readNumber();
									} else if (identifier == "avoiddamagetypes") {
										item_type.attributes[AVOIDDAMAGETYPES] = script.readNumber();
									} else if (identifier == "minimumlevel") {
										item_type.attributes[MINIMUMLEVEL] = script.readNumber();
									} else if (identifier == "professions") {
										item_type.attributes[PROFESSIONS] = script.readNumber();
									} else if (identifier == "weight") {
										item_type.attributes[WEIGHT] = script.readNumber();
									} else if (identifier == "rotatetarget") {
										item_type.attributes[ROTATETARGET] = script.readNumber();
									} else if (identifier == "destroytarget") {
										item_type.attributes[DESTROYTARGET] = script.readNumber();
									} else if (identifier == "bodyposition") {
										item_type.attributes[BODYPOSITION] = script.readNumber();
									} else if (identifier == "skillnumber") {
										item_type.attributes[SKILLNUMBER] = script.readNumber();
									} else if (identifier == "skillmodification") {
										item_type.attributes[SKILLMODIFICATION] = script.readNumber();
									} else if (identifier == "protectiondamagetypes") {
										item_type.attributes[PROTECTIONDAMAGETYPES] = script.readNumber();
									} else if (identifier == "damagereduction") {
										item_type.attributes[DAMAGEREDUCTION] = script.readNumber();
									} else if (identifier == "brightness") {
										item_type.attributes[BRIGHTNESS] = script.readNumber();
									} else if (identifier == "lightcolor") {
										item_type.attributes[LIGHTCOLOR] = script.readNumber();
									} else if (identifier == "corpsetype") {
										item_type.attributes[CORPSETYPE] = script.readNumber();
									} else if (identifier == "totalexpiretime") {
										item_type.attributes[TOTALEXPIRETIME] = script.readNumber();
									} else if (identifier == "expiretarget") {
										item_type.attributes[EXPIRETARGET] = script.readNumber();
									} else if (identifier == "totaluses") {
										item_type.attributes[TOTALUSES] = script.readNumber();
									} else if (identifier == "wearouttarget") {
										item_type.attributes[WEAROUTTARGET] = script.readNumber();
									} else if (identifier == "weapontype") {
										item_type.attributes[WEAPONTYPE] = script.readNumber();
									} else if (identifier == "weaponattackvalue") {
										item_type.attributes[WEAPONATTACKVALUE] = script.readNumber();
									} else if (identifier == "weapondefendvalue") {
										item_type.attributes[WEAPONDEFENDVALUE] = script.readNumber();
									} else if (identifier == "shielddefendvalue") {
										item_type.attributes[SHIELDDEFENDVALUE] = script.readNumber();
									} else if (identifier == "bowrange") {
										item_type.attributes[BOWRANGE] = script.readNumber();
									} else if (identifier == "bowammotype") {
										item_type.attributes[BOWAMMOTYPE] = script.readNumber();
									} else if (identifier == "throwrange") {
										item_type.attributes[THROWRANGE] = script.readNumber();
									} else if (identifier == "throwattackvalue") {
										item_type.attributes[THROWATTACKVALUE] = script.readNumber();
									} else if (identifier == "throwdefendvalue") {
										item_type.attributes[THROWDEFENDVALUE] = script.readNumber();
									} else if (identifier == "throwmissile") {
										item_type.attributes[THROWMISSILE] = script.readNumber();
									} else if (identifier == "throwspecialeffect") {
										item_type.attributes[THROWSPECIALEFFECT] = script.readNumber();
									} else if (identifier == "throweffectstrength") {
										item_type.attributes[THROWEFFECTSTRENGTH] = script.readNumber();
									} else if (identifier == "throwfragility") {
										item_type.attributes[THROWFRAGILITY] = script.readNumber();
									} else if (identifier == "wandrange") {
										item_type.attributes[WANDRANGE] = script.readNumber();
									} else if (identifier == "wandmanaconsumption") {
										item_type.attributes[WANDMANACONSUMPTION] = script.readNumber();
									} else if (identifier == "wandattackstrength") {
										item_type.attributes[WANDATTACKSTRENGTH] = script.readNumber();
									} else if (identifier == "wandattackvariation") {
										item_type.attributes[WANDATTACKVARIATION] = script.readNumber();
									} else if (identifier == "wanddamagetype") {
										item_type.attributes[WANDDAMAGETYPE] = script.readNumber();
									} else if (identifier == "wandmissile") {
										item_type.attributes[WANDMISSILE] = script.readNumber();
									} else if (identifier == "ammotype") {
										item_type.attributes[AMMOTYPE] = script.readNumber();
									} else if (identifier == "ammoattackvalue") {
										item_type.attributes[AMMOATTACKVALUE] = script.readNumber();
									} else if (identifier == "ammomissile") {
										item_type.attributes[AMMOMISSILE] = script.readNumber();
									} else if (identifier == "ammospecialeffect") {
										item_type.attributes[AMMOSPECIALEFFECT] = script.readNumber();
									} else if (identifier == "ammoeffectstrength") {
										item_type.attributes[AMMOEFFECTSTRENGTH] = script.readNumber();
									} else if (identifier == "armorvalue") {
										item_type.attributes[ARMORVALUE] = script.readNumber();
									} else if (identifier == "elevation") {
										item_type.attributes[ELEVATION] = script.readNumber();
									} else if (identifier == "disguisetarget") {
										item_type.attributes[DISGUISETARGET] = script.readNumber();
									} else if (identifier == "meaning") {
										item_type.attributes[MEANING] = script.readNumber();
										special_items[item_type.attributes[MEANING]] = &item_type;
									} else {
										script.error("unknown Object attribute");
										return false;
									}

									continue;
								} else if (token == TOKEN_SPECIAL) {
									if (script.getSpecial() == ',') {
										continue;
									} else if (script.getSpecial() == '}') {
										break;
									}
								}

								script.error("'}' expected");
								return false;
							}
						} else if (identifier == "typeid") {
							break;
						} else {
							script.error("unknown Object identifier");
							return false;
						}
					} else {
						break;
					}
				}
			} else {
				script.error("object definition expected");
				return false;
			}
		}
	}

	return true;
}

const ItemType* Items::getItemTypeByName(const std::string& name) const
{
	for (const ItemType& item : items) {
		if (item.name.find(name) != std::string::npos) {
			return &item;
		}
	}

	return nullptr;
}

ItemType* Items::getSpecialItem(SpecialMeaning_t meaning) const
{
	const auto it = special_items.find(meaning);
	if (it == special_items.end()) {
		return nullptr;
	}

	return it->second;
}

bool Item::createItem(uint16_t type_id)
{
	item_type = g_items.getItemType(type_id);
	if (item_type == nullptr || type_id < 100) {
		fmt::printf("ERROR - Item::createItem: invalid item type (typeid:%d)\n", type_id);
		return false;
	}

	if (getFlag(EXPIRE)) {
		setAttribute(ITEM_REMAINING_EXPIRE_TIME, getAttribute(TOTALEXPIRETIME) * 1000);
		g_game.decayItem(this);
	}

	return true;
}

/*Item* Item::createItem(uint16_t type_id)
{
	const ItemType* item_type = g_items.getItemType(type_id);
	if (!item_type) {
		fmt::printf("ERROR - Item::createItem: invalid item type (typeid:%d)\n", type_id);
		return nullptr;
	}

	const auto item = new Item();
	item->item_type = item_type;

	if (item->getFlag(EXPIRE)) {
		item->setAttribute(ITEM_REMAINING_EXPIRE_TIME, item->getAttribute(TOTALEXPIRETIME) * 1000);
		g_game.decayItem(item);
	}

	item->incRef();
	return item;
}*/

Item::~Item()
{
	/*for (Item* item : items) {
		item->decRef();
	}*/
}

std::string Item::getName(int32_t count) const
{
	std::ostringstream ss;
	ss << pluralizeItemName(item_type->name, count);
	
	if (getAttribute(ITEM_LIQUID_TYPE) > 0 && getFlag(LIQUIDCONTAINER) || getFlag(LIQUIDPOOL)) {
		ss << " of " << getLiquidName(getAttribute(ITEM_LIQUID_TYPE));
	}

	return ss.str();
}

const std::string& Item::getDescription() const
{
	return item_type->description;
}

std::string Item::getObjectDescription(const Creature* creature) const
{
	std::ostringstream ss;
	ss << getName(getAttribute(ITEM_AMOUNT));

	if (getFlag(LEVELDOOR)) {
		ss << " for level 0";
	}

	if (getFlag(CONTAINER)) {
		ss << " (Vol:" << getAttribute(CAPACITY) << ')';
	}

	if (getFlag(WEAPON)) {
		const int32_t attack = getAttribute(WEAPONATTACKVALUE);
		const int32_t defense = getAttribute(WEAPONDEFENDVALUE);
		ss << " (Atk:" << attack << " Def:" << defense << ")";
	}

	if (getFlag(SHIELD)) {
		const int32_t defense = getAttribute(SHIELDDEFENDVALUE);
		ss << " (Def:" << defense << ")";
	}

	if (getFlag(ARMOR)) {
		const int32_t armor = getAttribute(ARMORVALUE);
		ss << " (Arm:" << armor << ")";
	}

	if (getFlag(KEY)) {
		ss << fmt::sprintf(" (Key:%04ld)", getAttribute(ITEM_KEY_NUMBER));
	}

	if (!getFlag(SHOWDETAIL)) {
		if (getFlag(RESTRICTLEVEL) || getFlag(RESTRICTPROFESSION)) {
			ss << ".\nIt can only be wielded by ";
			if (getFlag(RESTRICTPROFESSION)) {
				int32_t professions = getAttribute(PROFESSIONS);

				bool first = true;
				while (professions > 0) {
					if (!first) {
						ss << " and ";
					}

					first = false;
					if (hasBitSet(8, professions)) {
						ss << "sorcerers";
						professions -= 8;
					} else if (hasBitSet(16, professions)) {
						ss << "druids";
						professions -= 16;
					} else if (hasBitSet(32, professions)) {
						ss << "paladins";
						professions -= 32;
					} else if (hasBitSet(64, professions)) {
						ss << "knights";
						professions -= 64;
					} else {
						break;
					}
				}
			} else {
				ss << "players";
			}

			if (getFlag(RESTRICTLEVEL)) {
				ss << " of level " << getAttribute(MINIMUMLEVEL) << " or higher";
			}
		}

		if (getFlag(RUNE)) {
			std::string spell_name;
			uint16_t rune_level = 0;
			g_magic.getMagicItemDescription(getId(), spell_name, rune_level);

			ss << fmt::sprintf(" for magic level %d", rune_level);
			ss << fmt::sprintf(". It's an \"%s\"-spell", spell_name);
			ss << fmt::sprintf(" (%dx)", getAttribute(ITEM_CHARGES));
		}

		if (getFlag(BED)) {
			if (!text.empty()) {
				ss << fmt::sprintf(". %s is sleeping there", text);
			} else {
				ss << ". Nobody is sleeping there";
			}
		}

		if (getFlag(NAMEDOOR)) {
			ss << ". It belongs to house 'Thais House'";
			ss << ". Nobody owns this house";
		}

		ss << ".";

		if ((getFlag(LIQUIDCONTAINER) && getAttribute(ITEM_LIQUID_TYPE) == 0) ||
			(getFlag(LIQUIDPOOL) && getAttribute(ITEM_LIQUID_TYPE) == 0)) {
			ss << " It is empty.";
		}
	} else {
		if (getFlag(WEAROUT)) {
			if (getAttribute(ITEM_REMAINING_USES) > 1) {
				ss << fmt::sprintf(" that has %d charges left", getAttribute(ITEM_REMAINING_USES));
			} else {
				ss << fmt::sprintf(" that has 1 charge left");
			}
		} else {
			if (!getFlag(EXPIRE)) {
				if (!getFlag(EXPIRESTOP)) {
					fmt::printf("ERROR - Item::getObjectDescription: Object has flag SHOWDETAIL but it's not a WEAROUT or EXPIRE item flag.");
					return ss.str();
				}
			}

			uint64_t remaining = 0;
			if (getFlag(EXPIRESTOP)) {
				remaining = getAttribute(ITEM_SAVED_EXPIRE_TIME);
			} else if (getFlag(EXPIRE)) {
				remaining = getAttribute(ITEM_REMAINING_EXPIRE_TIME);
			}

			if (remaining == 0 || remaining == getAttribute(TOTALEXPIRETIME)) {
				ss << " that is brand-new.";
			} else {
				remaining /= 1000;

				if (remaining >= 120) {
					ss << fmt::sprintf(" that has energy for %d minutes left.", remaining / 60);
				} else {
					ss << fmt::sprintf(" that has energy for 1 minute left.");
				}
			}
		}
	}

	bool in_range = true;

	int32_t dx = 0;
	int32_t dy = 0;

	if (creature) {
		const Position& creature_pos = creature->getPosition();
		const Position& pos = getPosition();

		dx = std::abs(pos.x - creature_pos.x);
		dy = std::abs(pos.y - creature_pos.y);

		if (dx > 1 || dy > 1) {
			in_range = false;
		}
	}

	if (in_range) {
		if (getFlag(TAKE)) {
			const int32_t total_weight = getWeight();
			if (getFlag(CUMULATIVE) && getAttribute(ITEM_AMOUNT) > 1) {
				ss << fmt::sprintf("\nThey weigh %d.%02d oz.", total_weight / 100, total_weight % 100);
			} else {
				ss << fmt::sprintf("\nIt weighs %d.%02d oz.", total_weight / 100, total_weight % 100);
			}
		}

		if (!getDescription().empty()) {
			ss << "\n" << getDescription() << '.';
		}
	}

	if (getFlag(TEXT)) {
		const int32_t font_size = getAttribute(FONTSIZE);
		if (font_size > 1) {
			if (!text.empty()) {
				if (std::max<int32_t>(dx, dy) > font_size) {
					ss << "\nYou are too far away to read it.";
				} else if (editor.empty()) {
					ss << fmt::sprintf("\nYou read: %s", text);
				} else {
					ss << fmt::sprintf("\n%s has written: %s", editor, text);
				}
			} else {
				ss << "\nNothing is written on it.";
			}
		} else if (font_size == 0) {
			if (!text.empty()) {
				ss << '\n' << text;
			}
		}
	}

	return ss.str();
}

uint32_t Item::getHoldingItemCount() const
{
	uint32_t count = items.size();
	for (const Item* item : items) {
		if (item->getFlag(CONTAINER)) {
			count += item->getHoldingItemCount();
		}
	}
	return count;
}

uint32_t Item::getWeight() const
{
	uint32_t weight = getAttribute(WEIGHT);
	if (getId() == 2904) {
		weight = 19500;
	} else if (getId() == 3458) {
		weight = 50000;
	} else if (getId() == 3510) {
		weight = 22800;
	} else if (getId() == 4311) {
		weight = 80000;
	}

	if (getFlag(CUMULATIVE)) {
		weight *= getAttribute(ITEM_AMOUNT);
	}

	if (getFlag(CONTAINER) || getFlag(CHEST)) {
		for (Item* item : items) {
			weight += item->getWeight();
		}
	}

	return weight;
}

bool Item::hasParentContainer() const
{
	return parent->getItem() != nullptr;
}

bool Item::loadData(ScriptReader& script)
{
	while (script.canRead()) {
		script.nextToken();
		if (script.getToken() == TOKEN_IDENTIFIER) {
			const std::string identifier = script.getIdentifier();
			script.readSymbol('=');

			if (identifier == "amount") {
				setAttribute(ITEM_AMOUNT, script.readNumber());
			} else if (identifier == "charges") {
				setAttribute(ITEM_CHARGES, script.readNumber());
			} else if (identifier == "containerliquidtype" || identifier == "poolliquidtype") {
				setAttribute(ITEM_LIQUID_TYPE, script.readNumber());
			} else if (identifier == "savedexpiretime") {
				setAttribute(ITEM_SAVED_EXPIRE_TIME, script.readNumber() * 1000);
			} else if (identifier == "absteleportdestination") {
				script.readNumber();
			} else if (identifier == "remainingexpiretime") {
				setAttribute(ITEM_REMAINING_EXPIRE_TIME, script.readNumber() * 1000);
			} else if (identifier == "remaininguses") {
				setAttribute(ITEM_REMAINING_USES, script.readNumber());
			} else if (identifier == "chestquestnumber") {
				script.readNumber();
			} else if (identifier == "level") {
				script.readNumber();
			} else if (identifier == "keynumber") {
				setAttribute(ITEM_KEY_NUMBER, script.readNumber());
			} else if (identifier == "keyholenumber") {
				script.readNumber();
			} else if (identifier == "doorquestnumber") {
				script.readNumber();
			} else if (identifier == "doorquestvalue") {
				script.readNumber();
			} else if (identifier == "responsible") {
				script.readNumber();
			} else if (identifier == "string") {
				text = script.readString();
			} else if (identifier == "editor") {
				editor = script.readString();
			} else if (identifier == "content") {
				if (!loadContent(script)) {
					script.error("failed to load content");
					return false;
				}
			} else {
				std::ostringstream ss;
				ss << "unknown attribute '" << identifier << '\'';
				script.error(ss.str());
				return false;
			}
		} else {
			break;
		}
	}

	return true;
}

bool Item::loadContent(ScriptReader& script)
{
	script.readSymbol('{');
	script.nextToken();
	while (script.canRead()) {
		if (script.getToken() == TOKEN_NUMBER) {
			const uint16_t type_id = script.getNumber();

			Item* new_item = g_itempool.createItem(type_id);
			if (new_item == nullptr) {
				script.error("unknown type id");
				return false;
			}

			addObject(new_item, INDEX_ANYWHERE);

			if (!new_item->loadData(script)) {
				return false;
			}
		} else if (script.getToken() == TOKEN_SPECIAL) {
			if (script.getSpecial() == ',') {
				script.nextToken();
				continue;
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

ReturnValue_t Item::queryAdd(int32_t index, const Object* object, uint32_t amount, uint32_t flags, Creature* actor) const
{
	const bool childIsOwner = hasBitSet(FLAG_CHILDISOWNER, flags);
	if (childIsOwner) {
		//a child container is querying, since we are the top container (not carried by a player)
		//just return with no error.
		return ALLGOOD;
	}

	if (!getFlag(CONTAINER)) {
		return NOTPOSSIBLE;
	}

	const Item* item = object->getItem();
	if (item == nullptr) {
		return NOTPOSSIBLE;
	}

	if (item == this) {
		return THISISIMPOSSIBLE;
	}

	if (!item->getFlag(TAKE)) {
		return CANNOTPICKUP;
	}

	const Cylinder* cylinder = getParent();
	while (cylinder) {
		if (cylinder == object) {
			return THISISIMPOSSIBLE;
		}

		cylinder = cylinder->getParent();
	}

	if (index == INDEX_ANYWHERE && items.size() >= getAttribute(CAPACITY)) {
		return CONTAINERNOTENOUGHROOM;
	}

	const Cylinder* top_parent = getTopParent();
	if (top_parent != this) {
		return top_parent->queryAdd(index, object, amount, flags | FLAG_CHILDISOWNER, actor);
	}

	return ALLGOOD;
}

ReturnValue_t Item::queryMaxCount(int32_t index, const Object* object, uint32_t amount, uint32_t & max_amount, uint32_t flags) const
{
	if (!getFlag(CONTAINER)) {
		return NOTPOSSIBLE;
	}

	const Item* item = object->getItem();
	if (item == nullptr) {
		return NOTPOSSIBLE;
	}

	const int32_t free_slots = std::max<int32_t>(getAttribute(CAPACITY) - items.size(), 0);

	if (item->getFlag(CUMULATIVE)) {
		uint32_t n = 0;

		if (index == INDEX_ANYWHERE) {
			//Iterate through every item and check how much free stackable slots there is.
			uint32_t slotIndex = 0;
			for (Item* containerItem : items) {
				if (containerItem != item && containerItem->getId() == item->getId() && containerItem->getAttribute(ITEM_AMOUNT) < 100) {
					const uint32_t remainder = (100 - containerItem->getAttribute(ITEM_AMOUNT));
					if (queryAdd(slotIndex++, item, remainder, flags, nullptr) == ALLGOOD) {
						n += remainder;
					}
				}
			}
		} else {
			const Item* dest_item = getitemIndex(index);
			if (dest_item && item->getId() == dest_item->getId() && dest_item->getAttribute(ITEM_AMOUNT) < 100) {
				const uint32_t remainder = 100 - dest_item->getAttribute(ITEM_AMOUNT);
				if (queryAdd(index, item, remainder, flags, nullptr) == ALLGOOD) {
					n = remainder;
				}
			}
		}

		max_amount = free_slots * 100 + n;
		if (max_amount < amount) {
			return CONTAINERNOTENOUGHROOM;
		}
	} else {
		max_amount = free_slots;
		if (max_amount == 0) {
			return CONTAINERNOTENOUGHROOM;
		}
	}

	return ALLGOOD;
}

Cylinder* Item::queryDestination(int32_t& index, const Object* object, Item** to_item, uint32_t & flags)
{
	if (index == 254 /*move up*/) {
		index = INDEX_ANYWHERE;
		*to_item = nullptr;

		Item* parent_item = getParent()->getItem();
		if (parent_item) {
			return parent_item;
		}

		return this;
	}

	if (index == 255 /*add wherever*/) {
		index = INDEX_ANYWHERE;
		*to_item = nullptr;
	} else if (index >= static_cast<int32_t>(getAttribute(CAPACITY))) {
		/*
		if you have a container, maximize it to show all 20 slots
		then you open a bag that is inside the container you will have a bag with 8 slots
		and a "grey" area where the other 12 slots where from the container
		if you drop the item on that grey area
		the client calculates the slot position as if the bag has 20 slots
		*/
		index = INDEX_ANYWHERE;
		*to_item = nullptr;
	}

	const Item* item = object->getItem();
	if (item == nullptr) {
		return this;
	}

	if (index != INDEX_ANYWHERE) {
		Item* item_at_index = getitemIndex(index);
		if (item_at_index) {
			*to_item = item_at_index;
		}

		if (item_at_index && item_at_index->getFlag(CONTAINER)) {
			index = INDEX_ANYWHERE;
			*to_item = nullptr;
			return item_at_index;
		}
	}

	return this;
}

void Item::addObject(Object* object, int32_t index)
{
	if (!getFlag(CONTAINER)) {
		return;
	}

	Item* item = object->getItem();
	if (item == nullptr) {
		return;
	}

	item->setParent(this);
	items.push_front(item);
}

void Item::removeObject(Object* object)
{
	if (!getFlag(CONTAINER)) {
		return;
	}
	
	Item* item = object->getItem();
	if (item == nullptr) {
		fmt::printf("ERROR - Item::removeObject: object is not an item.");
		return;
	}

	const auto it = std::find(items.begin(), items.end(), item);
	if (it == items.end()) {
		fmt::printf("ERRROR - Item::removeObject: item not found.");
		return;
	}

	items.erase(it);
}

uint8_t Item::getObjectPriority() const
{
	int32_t priority = 5;
	if (getFlag(BANK)) {
		priority = 0;
	} else if (getFlag(CLIP)) {
		priority = 1;
	} else if (getFlag(BOTTOM)) {
		priority = 2;
	} else if (getFlag(TOP)) {
		priority = 3;
	}
	return priority;
}

int32_t Item::getObjectIndex(const Object* object) const
{
	if (!getFlag(CONTAINER)) {
		return -1;
	}

	int32_t index = 0;
	for (const Item* help_item : items) {
		if (help_item == object) {
			return index;
		}

		index++;
	}

	return -1;
}

Object* Item::getObjectIndex(uint32_t index) const
{
	if (!getFlag(CONTAINER)) {
		return nullptr;
	}

	uint32_t n = 0;
	for (Item* item : items) {
		if (n == index) {
			return item;
		}
		n++;
	}
	return nullptr;
}

Item* Item::getitemIndex(uint32_t index) const
{
	if (!getFlag(CONTAINER)) {
		return nullptr;
	}

	uint32_t n = 0;
	for (Item* item : items) {
		if (n == index) {
			return item;
		}

		n++;
	}
	return nullptr;
}

bool Item::isHoldingItem(Item* item) const
{
	for (Item* aux_item : items) {
		if (aux_item == item) {
			return true;
		}

		if (aux_item->getFlag(CONTAINER) && aux_item->isHoldingItem(item)) {
			return true;
		}
	}

	return false;
}
