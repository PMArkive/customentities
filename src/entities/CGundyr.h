#ifndef _CGUNDYR_H_
#define _CGUNDYR_H_

#pragma once

#include <CCustomNextBotCombatCharacter.h>
#include <NextBot/NextBotIntentionInterface.h>
#include <NextBot/NextBotBehavior.h>
#include "CBaseNextBotBody.h"
#include "CBaseNextBotLocomotion.h"

class CGundyr final : public CCustomNextBotCombatCharacter
{
	public:
		CGundyr() noexcept;
		virtual ~CGundyr() noexcept;

	public:
		virtual void Spawn() noexcept override final;
		virtual void Precache() noexcept override final;

		virtual IBody *GetBodyInterface() const override final { return _body; }
		virtual ILocomotion *GetLocomotionInterface() const override final { return _locomotion; }

		DECLARE_INTENTION_INTERFACE(CGundyr);

	private:
		CBaseNextBotBody *_body{nullptr};
		CBaseNextBotLocomotion *_locomotion{nullptr};


};

#endif