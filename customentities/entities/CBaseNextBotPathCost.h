#ifndef _CBASENEXTBOTPATHCOST_H_
#define _CBASENEXTBOTPATHCOST_H_

#pragma once

#include <nav.h>
#include <nav_area.h>
#include <nav_mesh.h>
#include <NextBot/Path/NextBotPath.h>

class CFuncElevator;

class CBaseNextBotPathCost : public IPathCost
{
	public:
		constexpr CBaseNextBotPathCost() noexcept = default;
		CBaseNextBotPathCost(INextBot *const bot) noexcept
			: IPathCost() { SetNextBot(bot); }

		void SetNextBot(INextBot *const bot) noexcept;

		virtual float operator()(CNavArea *const area, CNavArea *const fromArea, const CNavLadder *const ladder, const CFuncElevator *const elevator, const float length) const override;

	private:
		INextBot *m_pNextBot{nullptr};
		ILocomotion *m_pLocomotion{nullptr};
		CBaseEntity *m_pEntity{nullptr};
		int m_iEntindex{-1};
		int m_iTeamNum{TEAM_ANY};
		float m_flStepHeight{0.0f};
		float m_flMaxJumpHeight{0.0f};
		float m_flDeathDropHeight{0.0f};
};

#endif