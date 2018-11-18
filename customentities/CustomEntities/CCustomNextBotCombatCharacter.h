#ifndef _CCUSTOMNEXTBOTCOMBATCHARACTER_H_
#define _CCUSTOMNEXTBOTCOMBATCHARACTER_H_

#pragma once

#include "CCustomCombatCharacter.h"
#include <NextBot/NextBotInterface.h>
#include <NextBot/NextBotVisionInterface.h>
#include <NextBot/NextBotBodyInterface.h>
#include <NextBot/NextBotGroundLocomotion.h>
#include <NextBot/NextBotIntentionInterface.h>

class __multiple_inheritance CCustomNextBotCombatCharacter : public INextBot, public CCustomCombatCharacter
{
	public:
		CCustomNextBotCombatCharacter() noexcept;
		virtual ~CCustomNextBotCombatCharacter() noexcept override;

	public:
		virtual void Spawn() noexcept override;
		virtual void Think() noexcept override;
		virtual void SetModel(const char8 *const szModelName) noexcept override;
		virtual void PerformCustomPhysics(Vector *const pNewPosition, Vector *const pNewVelocity, QAngle *const pNewAngles, QAngle *const pNewAngVelocity) noexcept override;
		virtual void HandleAnimEvent(animevent_t *const pEvent) noexcept override;
		virtual void Ignite(const float32 flFlameLifetime, const bool bNPCOnly, const float32 flSize, const bool bCalledByLevelDesigner) noexcept override;
		virtual const int32 OnTakeDamage_Alive(const CTakeDamageInfo &info) noexcept override;
		virtual const int32 OnTakeDamage_Dying(const CTakeDamageInfo &info) noexcept override;
		virtual void Event_Killed(const CTakeDamageInfo &info) noexcept override;
		//virtual void OnNavAreaChanged(CNavArea *enteredArea, CNavArea *leftArea) noexcept override;
		virtual const bool BecomeRagdoll(const CTakeDamageInfo &info, const Vector &forceVector) noexcept override;
		virtual ILocomotion *GetLocomotionInterface() const override;
		virtual IBody *GetBodyInterface() const override;
		virtual IIntention *GetIntentionInterface() const override;
		virtual IVision *GetVisionInterface() const override;

		virtual const bool IsNPC() const noexcept override { return true; }
		virtual INextBot *const MyNextBotPointer() const noexcept override { return const_cast<CCustomNextBotCombatCharacter *const>(this); }
		virtual CBaseCombatCharacter *const MyCombatCharacterPointer() const noexcept override { return reinterpret_cast<CBaseCombatCharacter *const>(entity()); }
		virtual const Class_T Classify() const noexcept override { return CLASS_NONE; }
		//virtual CAI_BaseNPC *const MyNPCPointer() const noexcept override { return nullptr; }
		virtual const uint32 PhysicsSolidMaskForEntity() const noexcept override;
		virtual const bool IsAreaTraversable(const CNavArea *const area) const noexcept override;
		virtual const Vector EyePosition() const noexcept override;

		virtual CBaseCombatCharacter *GetEntity() const noexcept override { return MyCombatCharacterPointer(); }
		virtual NextBotCombatCharacter *GetNextBotCombatCharacter() const noexcept override { return nullptr; }

	private:
		bool m_didModelChange{false};

		mutable IVision *m_baseVision{nullptr};
		mutable IBody *m_baseBody{nullptr};
		mutable NextBotGroundLocomotion *m_baseLocomotion{nullptr};
		mutable IIntention *m_baseIntention{nullptr};
};

#endif