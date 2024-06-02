#pragma once
#include <string>

using namespace std;

class Spell
{
public:
	Spell(float* xCenterBox, float* yCenterBox);
	virtual void isInRange(float x, float y) = 0;

	bool isAoe() { return aoe; }

protected:
	short range = 0, dmg = 0;
	float boostSpeed = 1;
	float* xCenterBox = nullptr, *yCenterBox = nullptr;
	bool moving = false, aoe = false;
};

