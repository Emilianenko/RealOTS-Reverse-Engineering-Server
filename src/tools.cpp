#include "pch.h"

#include "tools.h"

void replaceString(std::string& str, const std::string& sought, const std::string& replacement)
{
	size_t pos = 0;
	size_t start = 0;
	size_t soughtLen = sought.length();
	size_t replaceLen = replacement.length();

	while ((pos = str.find(sought, start)) != std::string::npos) {
		str = str.substr(0, pos) + replacement + str.substr(pos + soughtLen);
		start = pos + replaceLen;
	}
}

void getFilesInDirectory(const boost::filesystem::path& root, const std::string& ext, std::vector<boost::filesystem::path>& ret)
{
	if (!boost::filesystem::exists(root)) {
		return;
	}

	if (boost::filesystem::is_directory(root)) {
		boost::filesystem::recursive_directory_iterator it(root);
		boost::filesystem::recursive_directory_iterator endit;
		while (it != endit) {
			if (boost::filesystem::is_regular_file(*it) && it->path().extension() == ext) {
				ret.push_back(it->path().filename());
			}
			++it;
		}
	}
}

std::vector<std::string> explodeString(const std::string& inString, const std::string& separator, int32_t limit)
{
	std::vector<std::string> returnVector;
	std::string::size_type start = 0, end = 0;

	while (--limit != -1 && (end = inString.find(separator, start)) != std::string::npos) {
		returnVector.push_back(inString.substr(start, end - start));
		start = end + separator.size();
	}

	returnVector.push_back(inString.substr(start));
	return returnVector;
}

uint8_t getPercentToGo(uint64_t count, uint64_t next_level)
{
	if (next_level == 0) {
		return 0;
	}

	const uint8_t result = (count * 100) / next_level;
	if (result > 100) {
		return 0;
	}
	return result;
}

int32_t random(int32_t min, int32_t max)
{
	int32_t value = max - min + 1;
	int32_t result = min;
	if (value > 0)
		result = rand() % value + min;
	return result;
}

int32_t getNumber(const std::string& str)
{
	int32_t value = 0;
	try {
		value = std::stoi(str);
	} catch (std::exception) {
		return 0;
	}

	return value;
}

int64_t getLong(const std::string& str)
{
	int64_t value = 0;
	try {
		value = std::stol(str);
	}
	catch (std::exception) {
		return 0;
	}

	return value;
}

const char* getReturnMessage(ReturnValue_t value)
{
	switch (value) {
		case DESTINATIONOUTOFREACH:
			return "Destination is out of reach.";

		case NOTMOVEABLE:
			return "You cannot move this object.";

		case DROPTWOHANDEDITEM:
			return "Drop the double-handed object first.";

		case BOTHHANDSNEEDTOBEFREE:
			return "Both hands need to be free.";

		case CANNOTBEDRESSED:
			return "You cannot dress this object there.";

		case PUTTHISOBJECTINYOURHAND:
			return "Put this object in your hand.";

		case PUTTHISOBJECTINBOTHHANDS:
			return "Put this object in both hands.";

		case CANONLYUSEONEWEAPON:
			return "You may only use one weapon.";

		case TOOFARAWAY:
			return "Too far away.";

		case FIRSTGODOWNSTAIRS:
			return "First go downstairs.";

		case FIRSTGOUPSTAIRS:
			return "First go upstairs.";

		case NOTENOUGHCAPACITY:
			return "This object is too heavy for you to carry.";

		case CONTAINERNOTENOUGHROOM:
			return "You cannot put more objects in this container.";

		case NEEDEXCHANGE:
		case NOTENOUGHROOM:
			return "There is not enough room.";

		case CANNOTPICKUP:
			return "You cannot take this object.";

		case CANNOTTHROW:
			return "You cannot throw there.";

		case THEREISNOWAY:
			return "There is no way.";

		case THISISIMPOSSIBLE:
			return "This is impossible.";

		case PLAYERISPZLOCKED:
			return "Characters who attacked other players may not enter a protection zone.";

		case PLAYERISNOTINVITED:
			return "You are not invited.";

		case CREATUREDOESNOTEXIST:
			return "Creature does not exist.";

		case DEPOTISFULL:
			return "You cannot put more items in this depot.";

		case CANNOTUSETHISOBJECT:
			return "You cannot use this object.";

		case PLAYERWITHTHISNAMEISNOTONLINE:
			return "A player with this name is not online.";

		case APLAYERWITHTHISNAMEDOESNOTEXIST:
			return "A player with this name does not exist.";

		case NOTREQUIREDLEVELTOUSERUNE:
			return "You do not have the required magic level to use this rune.";

		case YOUAREALREADYTRADING:
			return "You are already trading.";

		case THISPLAYERISALREADYTRADING:
			return "This player is already trading.";

		case YOUMAYNOTLOGOUTDURINGAFIGHT:
			return "You may not logout during or immediately after a fight!";

		case DIRECTPLAYERSHOOT:
			return "You are not allowed to shoot directly on players.";

		case NOTENOUGHLEVEL:
			return "You do not have enough level.";

		case NOTENOUGHMAGICLEVEL:
			return "You do not have enough magic level.";

		case NOTENOUGHMANA:
			return "You do not have enough mana.";

		case NOTENOUGHSOUL:
			return "You do not have enough soul.";

		case YOUAREEXHAUSTED:
			return "You are exhausted.";

		case CANONLYUSETHISRUNEONCREATURES:
			return "You can only use this rune on creatures.";

		case PLAYERISNOTREACHABLE:
			return "Player is not reachable.";

		case CREATUREISNOTREACHABLE:
			return "Creature is not reachable.";

		case ACTIONNOTPERMITTEDINPROTECTIONZONE:
			return "This action is not permitted in a protection zone.";

		case YOUMAYNOTATTACKTHISPLAYER:
			return "You may not attack this player.";

		case YOUMAYNOTATTACKTHISCREATURE:
			return "You may not attack this creature.";

		case YOUMAYNOTATTACKAPERSONINPROTECTIONZONE:
			return "You may not attack a person in a protection zone.";

		case YOUMAYNOTATTACKAPERSONWHILEINPROTECTIONZONE:
			return "You may not attack a person while you are in a protection zone.";

		case YOUCANONLYUSEITONCREATURES:
			return "You can only use it on creatures.";

		case TURNSECUREMODETOATTACKUNMARKEDPLAYERS:
			return "Turn secure mode off if you really want to attack unmarked players.";

		case YOUNEEDPREMIUMACCOUNT:
			return "You need a premium account.";

		case YOUNEEDTOLEARNTHISSPELL:
			return "You need to learn this spell first.";

		case YOURVOCATIONCANNOTUSETHISSPELL:
			return "Your vocation cannot use this spell.";

		case YOUNEEDAWEAPONTOUSETHISSPELL:
			return "You need to equip a weapon to use this spell.";

		case PLAYERISPZLOCKEDLEAVEPVPZONE:
			return "You can not leave a pvp zone after attacking another player.";

		case PLAYERISPZLOCKEDENTERPVPZONE:
			return "You can not enter a pvp zone after attacking another player.";

		case ACTIONNOTPERMITTEDINANOPVPZONE:
			return "This action is not permitted in a non pvp zone.";

		case YOUCANNOTLOGOUTHERE:
			return "You can not logout here.";

		case YOUNEEDAMAGICITEMTOCASTSPELL:
			return "A magic item is necessary to cast this spell.";

		case CANNOTCONJUREITEMHERE:
			return "You cannot conjure items here.";

		case YOUNEEDTOSPLITYOURSPEARS:
			return "You need to split your spears first.";

		case NAMEISTOOAMBIGIOUS:
			return "Name is too ambigious.";

		case CANONLYUSEONESHIELD:
			return "You may use only one shield.";

		case TARGETLOST:
			return "Target lost.";

		default: // NOTPOSSIBLE, etc
			return "Sorry, not possible.";
	}
}

void extractArticleAndName(const std::string& data, std::string& article, std::string& name)
{
	std::string xarticle = data.substr(0, 3);
	if (xarticle == "an ") {
		name = data.substr(3, data.size());
		article = "an";
	} else {
		xarticle = data.substr(0, 2);
		if (xarticle == "a ") {
			name = data.substr(2, data.size());
			article = "a";
		} else {
			name = data;
			article = "";
		}
	}
}

std::string pluralizeString(const std::string& str)
{
	if (str == "meat") return "meat";

	int n = str.length();
	char ch = str[n - 1];
	char ch2 = str[n - 2];

	std::string str2;
	if (ch == 'y')
		str2 = str.substr(0, n - 1) + "ies";
	else if (ch == 'o' || ch == 's' || ch == 'x')
		str2 = str + "es";
	else if (ch == 'h'&& ch2 == 'c')
		str2 = str + "es";
	else if (ch == 'f')
		str2 = str.substr(0, n - 1) + "ves";
	else if (ch == 'e'&&ch2 == 'f')
		str2 = str.substr(0, n - 2) + "ves";
	else
		str2 = str + "s";

	return str2;
}

std::string pluralizeItemName(const std::string& str, int32_t amount)
{
	std::string article, name;
	extractArticleAndName(str, article, name);

	std::ostringstream ss;

	if (amount > 0) {
		if (amount > 1) {
			ss << amount << ' ' << pluralizeString(name);
		} else {
			ss << str;
		}
	} else {
		ss << name;
	}

	return ss.str();
}

std::string lowerString(const std::string& str)
{
	std::string copy = str;
	std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
	return copy;
}

uint8_t getLiquidColor(uint8_t nr)
{
	uint8_t color = 0;
	switch (nr) {
		case 0:
			color = 0;
			break;
		case 1:
			color = 1;
			break;
		case 2:
		case 10:
			color = 7;
			break;
		case 3:
		case 4:
		case 7:
			color = 3;
			break;
		case 5:
		case 11:
			color = 2;
			break;
		case 6:
			color = 4;
			break;
		case 8:
		case 12:
			color = 5;
			break;
		case 9:
			color = 6;
			break;
		default: 
			fmt::printf("ERROR - getLiquidColor: unknown number (color:%d)", nr);
			break;
	}
	return color;
}

std::string getLiquidName(uint8_t nr)
{
	std::string result;
	switch (nr) {
		case 0:
			result = "nothing";
			break;
		case 1:
			result = "water";
			break;
		case 2:
			result = "wine";
			break;
		case 3:
			result = "beer";
			break;
		case 4:
			result = "mud";
			break;
		case 5:
			result = "blood";
			break;
		case 6:
			result = "slime";
			break;
		case 7:
			result = "oil";
			break;
		case 8:
			result = "urine";
			break;
		case 9:
			result = "milk";
			break;
		case 10:
			result = "manafluid";
			break;
		case 11:
			result = "lifefluid";
			break;
		case 12:
			result = "lemonade";
			break;
		default:
			result = "unknown";
			break;
	}

	return result;
}

std::string getProfessionName(uint8_t profession)
{
	std::string result;
	switch (profession) {
		case 0:
			result = "none";
			break;
		case 1:
			result = "a knight";
			break;
		case 2:
			result = "a paladin";
			break;
		case 3:
			result = "a sorcerer";
			break;
		case 4:
			result = "a druid";
			break;
		case 11:
			result = "an elite knight";
			break;
		case 12:
			result = "a royal paladin";
			break;
		case 13:
			result = "a master sorcerer";
			break;
		case 14:
			result = "an elder druid";
			break;
		default:
			break;
	}
	return result;
}

SkillType_t getSkillType(const std::string& str)
{
	if (str == "level") {
		return SKILL_LEVEL;
	}
	
	if (str == "magic") {
		return SKILL_MAGIC;
	}

	if (str == "club") {
		return SKILL_CLUBFIGHTING;
	}

	if (str == "axe") {
		return SKILL_AXEFIGHTING;
	}

	if (str == "sword") {
		return SKILL_SWORDFIGHTING;
	}

	if (str == "distance") {
		return SKILL_DISTANCEFIGHTING;
	}

	if (str == "shielding") {
		return SKILL_SHIELDING;
	}

	if (str == "fishing") {
		return SKILL_FISHING;
	}

	// no match or fist
	return SKILL_FISTFIGHTING;
}
