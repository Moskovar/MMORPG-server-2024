#pragma once
#include "Spell.h"
class Whirlwind : public Spell
{
public:
	Whirlwind(float* xCenterBox, float* yCenterBox);
	void isInRange(float x, float y) override;

	static const short id = 4;
};

