#include "pch.h"

#include "server.h"
#include "game.h"
#include "map.h"
#include "rsa.h"
#include "config.h"
#include "item.h"
#include "channels.h"
#include "magic.h"
#include "vocation.h"
#include "itempool.h"

boost::asio::io_service io_service;

Config g_config;
Vocations g_vocations;
Items g_items;
TRSA RSA;
Channels g_channels;
Game g_game;
ItemPool g_itempool;
Map g_map;
Magic g_magic;

bool initAll();

int main(int, char**)
{
#ifdef _WIN32
	SetConsoleTitle("Tibia Game Server");
#endif

	fmt::printf(":: RealOTS - for Tibia 7.72\n");
	fmt::printf(":: A Tibia server reverse engineering progress\n");
	fmt::printf(":: By Ezzz (Alejandro Mujica)\n\n");

	std::srand(time(nullptr));
	g_game.setGameState(GAME_STARTING);

	if (!g_config.loadConfig()) {
		std::cin.get();
		return false;
	}

	const char* p("14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
	const char* q("7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
	RSA.setKey(p, q);

	Server server(io_service);
	server.open();

	std::thread io_thread([]() {
		io_service.run();
	});

	if (!initAll()) {
		fmt::printf(">> FATAL: g_game server is not online!\n");
		std::cin.get();
		return 0;
	}

	SetConsoleCtrlHandler([](DWORD) -> BOOL {
		fmt::printf(">> Shutting down...\n");

		g_game.setGameState(GAME_OFFLINE);
		io_service.stop();
		ExitThread(0);
	}, 1);

	g_game.launchGame();

	io_thread.join();
	return 0;
}

bool initAll()
{
	// load items first
	fmt::printf(">> Loading objects...\n");
	if (!g_items.loadItems()) {
		return false;
	}

	fmt::printf(">> Allocating %d items...\n", g_config.ItemCount);
	g_itempool.allocate(g_config.ItemCount);

	fmt::printf(">> Loading vocations...\n");
	g_vocations.loadVocations();

	// load spells
	fmt::printf(">> Loading magic...\n");
	g_magic.initSpells();

	// load map data
	fmt::printf(">> Loading map...\n");
	if (!g_map.loadMap()) {
		return false;
	}

	return true;
}