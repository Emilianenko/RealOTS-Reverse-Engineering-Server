#pragma once

#include "enums.h"

#include <boost/filesystem.hpp>

void replaceString(std::string& str, const std::string& sought, const std::string& replacement);
void getFilesInDirectory(const boost::filesystem::path& root, const std::string& ext, std::vector<boost::filesystem::path>& ret);

std::vector<std::string> explodeString(const std::string& inString, const std::string& separator, int32_t limit = -1);

inline bool hasBitSet(uint32_t flag, uint32_t flags) {
	return (flags & flag) != 0;
}

uint8_t getPercentToGo(uint64_t count, uint64_t next_level);
int32_t random(int32_t min, int32_t max);
int32_t getNumber(const std::string& str);
int64_t getLong(const std::string& str);

const char* getReturnMessage(ReturnValue_t ret);

std::string pluralizeItemName(const std::string& str, int32_t amount);
std::string lowerString(const std::string& str);

uint8_t getLiquidColor(uint8_t nr);
std::string getLiquidName(uint8_t nr);
std::string getProfessionName(uint8_t profession);

SkillType_t getSkillType(const std::string& str);