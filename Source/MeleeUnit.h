#ifndef __MELEEUNIT_H__
#define __MELEEUNIT_H__

#include "Behaviour.h"
#include "Gameobject.h"
#include "Component.h"

class Gameobject;

class MeleeUnit : public B_Unit
{
public:
	MeleeUnit(Gameobject* go);
	~MeleeUnit();
	void OnDamage(int d);
	void CheckSprite();
	void OnKill();

protected:

	int MU_lvl = 1;
	int MU_max_lvl = 5;
	int MU_damage = 5;
	int attack_speed = 1;

};

#endif

