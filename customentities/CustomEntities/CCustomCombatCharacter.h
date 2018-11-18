#ifndef _CCUSTOMCOMBATCHARACTER_H_
#define _CCUSTOMCOMBATCHARACTER_H_

#pragma once

#include "CCustomFlex.h"

class CCustomCombatCharacter : public CCustomFlex
{
	public:
		using CCustomFlex::CCustomFlex;

	protected:
		DECLARE_HOOK(const int, OnTakeDamage_Alive, , false, const CTakeDamageInfo &);
		DECLARE_HOOK(const int, OnTakeDamage_Dying, , false, const CTakeDamageInfo &);
		DECLARE_HOOK(const bool, IsAreaTraversable, const, false, const CNavArea *const);
		DECLARE_HOOK(const bool, BecomeRagdoll, , false, const CTakeDamageInfo &, const Vector &);
		//DECLARE_HOOK(void, OnNavAreaChanged, , false, CNavArea *const, CNavArea *const);
		DECLARE_HOOK(void, UpdateLastKnownArea, , false);
};

#endif