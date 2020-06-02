#pragma once

class Config
{
public:
	std::string IP;
	std::string World;
	uint16_t Port = 0;
	uint16_t Beat = 0;
	int32_t SectorXMin = 0;
	int32_t SectorXMax = 0;
	int32_t SectorYMin = 0;
	int32_t SectorYMax = 0;
	int32_t SectorZMin = 0;
	int32_t SectorZMax = 0;
	int32_t ItemCount = 0;

	bool loadConfig();
};

extern Config g_config;
