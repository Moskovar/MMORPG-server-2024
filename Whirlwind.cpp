#include "Whirlwind.h"

Whirlwind::Whirlwind(float* xCenterBox, float* yCenterBox) : Spell(xCenterBox, yCenterBox)
{
	range = 50;
	dmg = 10;
	aoe = true;
}

void Whirlwind::isInRange(float x, float y)
{

}
