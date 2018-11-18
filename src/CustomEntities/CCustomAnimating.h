#ifndef _CCUSTOMANIMATING_H_
#define _CCUSTOMANIMATING_H_

#pragma once

#include "CCustomBaseEntity.h"

class CCustomAnimating : public CCustomBaseEntity
{
	public:
		using CCustomBaseEntity::CCustomBaseEntity;

		enum {
			NUM_BONECTRLS = CBaseAnimating::NUM_BONECTRLS,
			NUM_POSEPAREMETERS = CBaseAnimating::NUM_POSEPAREMETERS,
		};

		const float SetPoseParameter(const int iParameter, const float flValue) noexcept;
		const float SetBoneController(const int iController, const float flValue) noexcept;
		void InitBoneControllers() noexcept;

		const float SequenceDuration(const int iSequence) const noexcept;

		const int LookupAttachment(const char *const name) const noexcept;
		/*
		bool GetAttachment(int iAttachment, matrix3x4_t &attachmentToWorld);
		bool GetAttachment(int index, Vector *origin=nullptr, QAngle *ang=nullptr);
		bool GetAttachmentLocal(int index, Vector *origin=nullptr, QAngle *ang=nullptr);
		*/
		const Activity LookupActivity(const char *const label) const noexcept;
		const int LookupSequence(const char *const label) const noexcept;
		const int LookupPoseParameter(const char *const szName) const noexcept;
		const float GetPoseParameter(const int iParameter) const noexcept;
		const float *const GetPoseParameterArray() const noexcept;
		const int GetSequence() const noexcept;
		const int SelectWeightedSequence(const Activity activity) const noexcept;
		//void EnableServerIK();
		//void DisableServerIK();
		const float GetSequenceGroundSpeed(const int iSequence) const noexcept;
		const float GetSequenceMoveDist(const int iSequence) const noexcept;
		const Activity GetSequenceActivity(const int iSequence) const noexcept;
		void SetSequence(const int sequence) noexcept;
		void RandomizeBodygroups(CUtlVector<const char *> &groups) noexcept;
		const int FindBodygroupByName(const char *const name) const noexcept;
		const int CountBodyGroupVariants(const int group) const noexcept;
		const int GetBodygroupCount(const int iGroup) const noexcept;
		const char *const GetBodygroupPartName(const int iGroup, const int iPart) const noexcept;
		const int FindBodyGroupVariant(const int group, const int variant) const noexcept;
		void SetBodygroup(const int iGroup, const int iValue) noexcept;
		const int GetNumBodyGroups() const noexcept;
		const bool IsSequenceFinished() const noexcept;
		void ResetSequenceInfo() noexcept;
		const float GetModelScale() const noexcept { return m_flModelScale; }
		void ResetSequence(const int nSequence) noexcept;
		const bool SequenceLoops() const noexcept { return m_bSequenceLoops; }
		void SetCycle(const float flCycle) noexcept { m_flCycle = flCycle; }
		const float GetAnimTimeInterval() const noexcept;
		const bool GetIntervalMovement(const float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles) const noexcept;
		const float GetSequenceCycleRate(const int iSequence) const noexcept;
		const float GetCycle() const noexcept { return m_flCycle; }

		void LockStudioHdr() noexcept;
		CStudioHdr *const GetModelPtr() const noexcept;

		constexpr CBaseAnimating *const GetBaseAnimating() const noexcept
			{ return reinterpret_cast<CBaseAnimating *const>(entity()); }

	protected:
		DECLARE_HOOK(void, HandleAnimEvent, , false, animevent_t *const);
		void DispatchAnimEvents() noexcept { DispatchAnimEvents(entity()->GetBaseAnimating()); }
		DECLARE_HOOK(void, DispatchAnimEvents, , false, CBaseAnimating *const);
		DECLARE_HOOK(void, StudioFrameAdvance, , false);
		DECLARE_HOOK(void, Ignite, , false, const float, const bool, const float, const bool);

	public:
		DECLARE_PROP_ARRAY(float, m_flEncodedController, NUM_BONECTRLS);
		DECLARE_PROP_ARRAY(float, m_flPoseParameter, NUM_POSEPAREMETERS);
		//DECLARE_PROP(CStudioHdr *, m_pStudioHdr);
		DECLARE_PROP(unsigned short, m_fBoneCacheFlags);
		DECLARE_PROP(float, m_flPlaybackRate);
		DECLARE_PROP(float, m_flModelScale);
		DECLARE_PROP(int, m_nSequence);
		DECLARE_PROP(bool, m_bClientSideAnimation);
		DECLARE_PROP(bool, m_bClientSideFrameReset);
		DECLARE_PROP(int, m_nBody);
		DECLARE_PROP(int, m_nSkin);
		//DECLARE_PROP(CIKContext *, m_pIk);
		DECLARE_PROP(int, m_iIKCounter);
		DECLARE_PROP(float, m_flGroundSpeed);
		DECLARE_PROP(float, m_flCycle);
		DECLARE_PROP(bool, m_bSequenceLoops);
		DECLARE_PROP(bool, m_bSequenceFinished);
		DECLARE_PROP(int, m_nNewSequenceParity);
		DECLARE_PROP(int, m_nResetEventsParity);
		DECLARE_PROP(float, m_flLastEventCheck);
		DECLARE_PROP(float, m_flAnimTime);
		DECLARE_PROP(float, m_flPrevAnimTime);
};

#endif