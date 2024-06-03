#pragma once
#define SDL_MAIN_HANDLED
#include <chrono>
#include <SDL.h>
#include <SDL_image.h>
#include <map>
#include <string>
#include <cmath>

using namespace std;

namespace uti {
	enum Language {
		FR,
		ENG
	};

	enum Category {
		PLAYER,
		NPC
	};

	enum Direction {
		UP = 1,
		RIGHT = 3,
		DOWN = 6,
		LEFT = 11
	};

	enum Header {
		NE = 0,
		NES = 1,
		NESE = 2,
		NEF = 3
	};

	struct MoveRate {//utile ??
		float xRate, yRate;
	};

#pragma pack(push, 1)
	struct NetworkEntity {
		short header = 0;
		short id = 0, countDir = 0, hp = 0;
		int   xMap = 0, yMap = 0;
		uint64_t timestamp; // En microsecondes depuis l'epoch
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct NetworkEntitySpell {
		short header = 1;
		short id = 0, spellID = 0;
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct NetworkEntitySpellEffect {
		short header = 2;
		short id = 0, spellID = 0;
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct NetworkEntityFaction {
		short header = 3;
		short id = 0, faction = 1;
	};
#pragma pack(pop)

	uint64_t getCurrentTimestamp();

	extern map<int, map<int, string>> categories;
	extern map<float, MoveRate> pixDir;//useless??

	Uint32 get_pixel(SDL_Surface* surface, int x, int y);

	// Structure pour représenter un point
	struct Point {
		short x;
		short y;
	};

	// Structure pour représenter un cercle
	struct Circle {
		Point center;
		short radius;
	};

	// Fonction pour déterminer si un point est à l'intérieur d'un cercle
	bool isPointInCircle(const short x, const short y, const short circleCenterX, const short circleCenterY, const short circleRadius);
	// Fonction pour déterminer si deux cercles se croisent
	bool doCirclesIntersect(const Circle& c1, const Circle& c2);

}
