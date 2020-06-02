#include "pch.h"

#include "config.h"
#include "script.h"
#include "object.h"
#include "game.h"

bool Config::loadConfig()
{
	ScriptReader script;
	if (!script.loadScript("dat/config.dat")) {
		return false;
	}

	while (script.canRead()) {
		TOKENTYPE token = script.nextToken();
		if (token == TOKEN_IDENTIFIER) {
			std::string identifier = script.getIdentifier();
			script.readSymbol('=');

			if (identifier == "ip") {
				IP = script.readString();
			} else if (identifier == "port") {
				Port = script.readNumber();
			} else if (identifier == "beat") {
				Beat = script.readNumber();
			} else if (identifier == "world") {
				World = script.readString();
			} else if (identifier == "sectorxmin") {
				SectorXMin = script.readNumber();
			} else if (identifier == "sectorxmax") {
				SectorXMax = script.readNumber();
			} else if (identifier == "sectorymin") {
				SectorYMin = script.readNumber();
			} else if (identifier == "sectorymax") {
				SectorYMax = script.readNumber();
			} else if (identifier == "sectorzmin") {
				SectorZMin = script.readNumber();
			} else if (identifier == "sectorzmax") {
				SectorZMax = script.readNumber();
			} else if (identifier == "newbiestart") {
				script.readSymbol('[');
				Position pos;
				pos.x = script.readNumber();
				script.readSymbol(',');
				pos.y = script.readNumber();
				script.readSymbol(',');
				pos.z = script.readNumber();
				script.readSymbol(']');

				g_game.newbie_start_pos = pos;
			} else if (identifier == "mark") {
				script.readSymbol('(');
				const std::string name = script.readString();
				script.readSymbol(',');
				script.readSymbol('[');
				Position pos;
				pos.x = script.readNumber();
				script.readSymbol(',');
				pos.y = script.readNumber();
				script.readSymbol(',');
				pos.z = script.readNumber();
				script.readSymbol(']');
				g_game.map_points[name] = pos;
			} else if (identifier == "itemcount") {
				ItemCount = script.readNumber();
			} else {
				script.error("unknown identifier");
				return false;
			}
		}
	}

	return true;
}
