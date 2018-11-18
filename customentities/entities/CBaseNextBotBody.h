#ifndef _CBASENEXTBOTBODY_H_
#define _CBASENEXTBOTBODY_H_

#pragma once

#include <NextBot/NextBotBodyInterface.h>
#include <CCustomNextBotCombatCharacter.h>

class CBaseNextBotBody : public IBody
{
	public:
		CBaseNextBotBody(CCustomNextBotCombatCharacter *const bot) noexcept;

		virtual bool StartActivity(const Activity act, const unsigned int flags = 0) override;
		virtual int SelectAnimationSequence(const Activity act) const override;
		virtual Activity GetActivity() const override;
		virtual bool IsActivity(const Activity act) const override;
		virtual bool HasActivityType(unsigned int flags) const override;
		virtual unsigned int GetSolidMask() const override;
		virtual unsigned int GetCollisionGroup() const override;
		virtual void Update() override;
		virtual void OnModelChanged() override;
		virtual void OnAnimationActivityComplete(const int activity) override;
		virtual void OnAnimationActivityInterrupted(const int activity) override;
		virtual void Reset() override;
		virtual void SetArousal(const ArousalType arousal) override;
		virtual ArousalType GetArousal() const override;
		virtual bool IsArousal(const ArousalType arousal) const override;
		virtual const Vector &GetViewVector() const;
		virtual void SetDesiredPosture(const PostureType posture) override;
		virtual PostureType GetDesiredPosture() const override;
		virtual bool IsDesiredPosture(const PostureType posture) const override;
		virtual bool IsInDesiredPosture() const override;
		virtual PostureType GetActualPosture() const override;
		virtual bool IsActualPosture(const PostureType posture) const override;
		virtual bool IsPostureMobile() const override;
		virtual bool IsPostureChanging() const override;
		virtual void OnPostureChanged() override;

		virtual void AimHeadTowards(const Vector &lookAtPos, const LookAtPriorityType priority = BORING, const float duration = 0.0f, INextBotReply *const replyWhenAimed = nullptr, const char *const reason = nullptr) override;
		virtual void AimHeadTowards(CBaseEntity *const subject, const LookAtPriorityType priority = BORING, const float duration = 0.0f, INextBotReply *const replyWhenAimed = nullptr, const char *const reason = nullptr) override;
		virtual bool IsHeadAimingOnTarget() const override;
		virtual bool IsHeadSteady() const override;
		virtual float GetHeadSteadyDuration() const override;
		//virtual float GetHeadAimSubjectLeadTime() const override;
		//virtual float GetHeadAimTrackingInterval() const override;
		virtual void ClearPendingAimReply() override;
		virtual float GetMaxHeadAngularVelocity() const override;

	protected:
		CCustomNextBotCombatCharacter *m_pCustomBot = nullptr;

		Activity m_nOldDesiredActivity{ACT_INVALID};
		int m_nOldActivityFlags{0};

		Activity m_nDesiredActivity{ACT_INVALID};
		int m_nActivityFlags{0};

		ArousalType m_nArousal{NEUTRAL};

		PostureType m_nActualPosture{STAND};
		PostureType m_nDesiredPosture{STAND};
		bool m_bChangingPosture{false};
		Activity m_nPostureActvity{ACT_INVALID};

		Vector m_lookAtPos{};
		EHANDLE m_lookAtSubject{};
		Vector m_lookAtVelocity{};
		CountdownTimer m_lookAtTrackingTimer{};
		LookAtPriorityType m_lookAtPriority{BORING};
		CountdownTimer m_lookAtExpireTimer{};
		IntervalTimer m_lookAtDurationTimer{};
		INextBotReply *m_lookAtReplyWhenAimed{nullptr};
		bool m_isSightedIn{false};
		bool m_hasBeenSightedIn{false};
		IntervalTimer m_headSteadyTimer{};
		CountdownTimer m_anchorRepositionTimer{};
		Vector m_anchorForward{};
		QAngle m_priorAngles{};
};

#endif