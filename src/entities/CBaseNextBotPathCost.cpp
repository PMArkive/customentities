#include "CBaseNextBotPathCost.h"
#include <NextBot/NextBotLocomotionInterface.h>
#include <nav_area.h>

void CBaseNextBotPathCost::SetNextBot(INextBot *const bot) noexcept
{
	if(!bot)
		return;

	m_pNextBot = bot;
	m_pLocomotion = m_pNextBot->GetLocomotionInterface();
	m_flStepHeight = m_pLocomotion->GetStepHeight();
	m_flMaxJumpHeight = m_pLocomotion->GetMaxJumpHeight();
	m_flDeathDropHeight = m_pLocomotion->GetDeathDropHeight();
	m_pEntity = m_pNextBot->GetEntity();
	if(m_pEntity) {
		m_iEntindex = m_pEntity->entindex();
		m_iTeamNum = m_pEntity->GetTeamNumber();
	}
}

float CBaseNextBotPathCost::operator()(CNavArea *const area, CNavArea *const fromArea, const CNavLadder *const ladder, const CFuncElevator *const elevator, const float length) const
{
	if(!fromArea)
		return 0.0f;

	if(m_pLocomotion && !m_pLocomotion->IsAreaTraversable(area))
		return -1.0f;

	float dist{0.0f};

	if(ladder)
		dist = 2.0f * ladder->m_length;
	else if(length > 0.0f)
		dist = length;
	else
		dist = (area->GetCenter() - fromArea->GetCenter()).Length();

	float cost{dist + fromArea->GetCostSoFar()};

	const float deltaZ{fromArea->ComputeAdjacentConnectionHeightChange(area)};
	if(deltaZ >= m_flStepHeight) {
		if(deltaZ >= m_flMaxJumpHeight)
			return -1.0f;
		cost += 5.0f * dist;
	} else if(deltaZ < -m_flDeathDropHeight)
		return -1.0f;

	float multiplier{RandomFloat(1.0f, 5.0f)};

	int seed{static_cast<const int>(gpGlobals->curtime * 0.1f) + 1};
	seed *= area->GetID();
	seed *= m_iEntindex;
	multiplier += (cosf(static_cast<const float>(seed)) + 1.0f) * 50.0f;

	cost *= multiplier;

	return cost;
}