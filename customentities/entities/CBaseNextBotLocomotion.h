#ifndef _CBASENEXTBOTLOCOMOTION_H_
#define _CBASENEXTBOTLOCOMOTION_H_

#pragma once

#include <NextBot/NextBotGroundLocomotion.h>
#include <CCustomNextBotCombatCharacter.h>

class CBaseNextBotLocomotion : public NextBotGroundLocomotion
{
	public:
		CBaseNextBotLocomotion(CCustomNextBotCombatCharacter *const bot) noexcept;

		virtual float GetMaxJumpHeight() const override;
		virtual float GetDeathDropHeight() const override;
		virtual float GetRunSpeed() const override;
		virtual float GetWalkSpeed() const override;
		virtual float GetMaxYawRate() const override;
		virtual float GetMaxAcceleration() const override;
		virtual float GetMaxDeceleration() const override;

	private:
		CCustomNextBotCombatCharacter *m_pCustomBot{nullptr};
};

#endif