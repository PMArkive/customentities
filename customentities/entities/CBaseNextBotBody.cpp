#include "CBaseNextBotBody.h"
#include <activitylist.h>
#include <NextBot/NextBotLocomotionInterface.h>

ConVar nb_saccade_time{"nb_saccade_time", "0.1", FCVAR_CHEAT};
ConVar nb_saccade_speed{"nb_saccade_speed", "1000", FCVAR_CHEAT};
ConVar nb_head_aim_steady_max_rate{"nb_head_aim_steady_max_rate", "100", FCVAR_CHEAT};
ConVar nb_head_aim_settle_duration{"nb_head_aim_settle_duration", "0.3", FCVAR_CHEAT};
ConVar nb_head_aim_resettle_angle{"nb_head_aim_resettle_angle", "100", FCVAR_CHEAT, "After rotating through this angle, the bot pauses to 'recenter' its virtual mouse on its virtual mousepad"};
ConVar nb_head_aim_resettle_time{"nb_head_aim_resettle_time", "0.3", FCVAR_CHEAT, "How long the bot pauses to 'recenter' its virtual mouse on its virtual mousepad"};

CBaseNextBotBody::CBaseNextBotBody(CCustomNextBotCombatCharacter *const bot) noexcept
	: IBody(bot), m_pCustomBot{bot}
{

}

int CBaseNextBotBody::SelectAnimationSequence(const Activity act) const
{
	if(act == ACT_INVALID)
		return -1;

	return m_pCustomBot->SelectWeightedSequence(act);
}

Activity CBaseNextBotBody::GetActivity() const
{
	return m_pCustomBot->GetSequenceActivity(m_pCustomBot->GetSequence());
}

bool CBaseNextBotBody::IsActivity(const Activity act) const
{
	return (GetActivity() == act);
}

bool CBaseNextBotBody::HasActivityType(const unsigned int flags) const
{
	return !!(m_nActivityFlags & flags);
}

unsigned int CBaseNextBotBody::GetSolidMask() const
{
	return m_pCustomBot->PhysicsSolidMaskForEntity();
}

unsigned int CBaseNextBotBody::GetCollisionGroup() const
{
	return m_pCustomBot->GetCollisionGroup();
}

void CBaseNextBotBody::SetArousal(const ArousalType arousal)
{
	m_nArousal = arousal;
}

IBody::ArousalType CBaseNextBotBody::GetArousal() const
{
	return m_nArousal;
}

bool CBaseNextBotBody::IsArousal(const ArousalType arousal) const
{
	return (GetArousal() == arousal);
}

void CBaseNextBotBody::SetDesiredPosture(const PostureType posture)
{
	if(IsPostureChanging() || !IsInDesiredPosture())
		return;

	Activity idealActivity{ACT_INVALID};

	if(idealActivity != ACT_INVALID) {
		m_nPostureActvity = idealActivity;
		StartActivity(idealActivity, ACTIVITY_UNINTERRUPTIBLE|MOTION_CONTROLLED_XY);
		m_bChangingPosture = true;
	} else {
		m_nPostureActvity = ACT_INVALID;
		m_bChangingPosture = false;
		m_nActualPosture = posture;
		m_pCustomBot->OnPostureChanged();
	}

	m_nDesiredPosture = posture;
}

IBody::PostureType CBaseNextBotBody::GetDesiredPosture() const
{
	return m_nDesiredPosture;
}

bool CBaseNextBotBody::IsDesiredPosture(const PostureType posture) const
{
	return (GetActualPosture() == posture);
}

bool CBaseNextBotBody::IsInDesiredPosture() const
{
	return (GetActualPosture() == GetDesiredPosture());
}

IBody::PostureType CBaseNextBotBody::GetActualPosture() const
{
	return m_nActualPosture;
}

bool CBaseNextBotBody::IsActualPosture(const PostureType posture) const
{
	return (GetActualPosture() == posture);
}

bool CBaseNextBotBody::IsPostureMobile() const
{
	PostureType posture = GetActualPosture();
	if(posture == SIT || posture == LIE)
		return false;

	return !IsPostureChanging();
}

bool CBaseNextBotBody::IsPostureChanging() const
{
	return m_bChangingPosture;
}

void CBaseNextBotBody::OnPostureChanged()
{
	__super::OnPostureChanged();

	Activity idealActivity{ACT_INVALID};

	if(idealActivity != ACT_INVALID)
		StartActivity(idealActivity, MOTION_CONTROLLED_XY);
}

void CBaseNextBotBody::OnAnimationActivityComplete(const int activity)
{
	__super::OnAnimationActivityComplete(activity);

	if(m_pCustomBot->IsDebugging(NEXTBOT_ANIMATION))
		DevMsg("%3.2f: bot(#%d) CBaseNextBotBody::OnAnimationActivityComplete: %i %s\n", gpGlobals->curtime, m_pCustomBot->entindex(), activity, ActivityList_NameForIndex(activity));

	if(activity == m_nPostureActvity) {
		m_nPostureActvity = ACT_INVALID;
		m_bChangingPosture = false;
		m_nActualPosture = m_nDesiredPosture;
		m_pCustomBot->OnPostureChanged();
	}
}

void CBaseNextBotBody::OnAnimationActivityInterrupted(const int activity)
{
	__super::OnAnimationActivityInterrupted(activity);

	if(m_pCustomBot->IsDebugging(NEXTBOT_ANIMATION))
		DevMsg("%3.2f: bot(#%d) CBaseNextBotBody::OnAnimationActivityInterrupted: %i %s\n", gpGlobals->curtime, m_pCustomBot->entindex(), activity, ActivityList_NameForIndex(activity));
}

void CBaseNextBotBody::Reset()
{
	__super::Reset();

	//m_pCustomBot->ResetSequence(0);

	m_nOldActivityFlags = 0;
	m_nActivityFlags = 0;

	m_nOldDesiredActivity = ACT_INVALID;
	m_nDesiredActivity = ACT_INVALID;

	m_nArousal = NEUTRAL;

	m_nPostureActvity = ACT_INVALID;
	m_nActualPosture = STAND;
	m_nDesiredPosture = STAND;
	m_bChangingPosture = false;

	m_lookAtPos = vec3_origin;
	m_lookAtSubject = nullptr;
	m_lookAtReplyWhenAimed = nullptr;
	m_lookAtVelocity = vec3_origin;
	m_lookAtExpireTimer.Invalidate();

	m_lookAtPriority = BORING;
	m_lookAtExpireTimer.Invalidate();
	m_lookAtDurationTimer.Invalidate();
	m_isSightedIn = false;
	m_hasBeenSightedIn = false;
	m_headSteadyTimer.Invalidate();

	m_priorAngles = vec3_angle;
	m_anchorRepositionTimer.Invalidate();
	m_anchorForward = vec3_origin;
}

bool CBaseNextBotBody::StartActivity(const Activity act, const unsigned int flags)
{
	if(act == ACT_INVALID)
		return false;

	if(IsPostureChanging() || !IsInDesiredPosture())
		return false;

	if(HasActivityType(ACTIVITY_UNINTERRUPTIBLE) && !m_pCustomBot->IsSequenceFinished())
		return false;

	const int animDesired{SelectAnimationSequence(act)};

	const Activity curact{m_pCustomBot->GetSequenceActivity(m_pCustomBot->GetSequence())};
	const Activity newact{m_pCustomBot->GetSequenceActivity(animDesired)};
	if(curact == newact)
		return false;

	m_nOldActivityFlags = m_nActivityFlags;
	m_nActivityFlags = flags;

	m_nOldDesiredActivity = m_nDesiredActivity;
	m_nDesiredActivity = act;

	if(m_nOldDesiredActivity != act)
		m_pCustomBot->OnAnimationActivityInterrupted(m_nOldDesiredActivity);

	m_pCustomBot->ResetSequence(animDesired);
	return true;
}

void CBaseNextBotBody::OnModelChanged()
{
	__super::OnModelChanged();

	
}

const Vector &CBaseNextBotBody::GetViewVector() const
{
	static Vector view{};
	AngleVectors(m_pCustomBot->EyeAngles(), &view);
	return view;
}

void CBaseNextBotBody::Update()
{
	__super::Update();

	const float deltaT{GetUpdateInterval()};
	if(deltaT < 0.00001f)
		return;

	ILocomotion *const locomotion{m_pCustomBot->GetLocomotionInterface()};
	const Vector &velocity{locomotion->GetVelocity()};
	const float walkspeed{locomotion->GetWalkSpeed()};
	const float speed{velocity.Length2D()};

	float playbackrate{1.0f};
	if(walkspeed > speed)
		playbackrate = speed / walkspeed;
	playbackrate = clamp(playbackrate, 0.01f, 10.0f);

	if(HasActivityType(MOTION_CONTROLLED_XY) || HasActivityType(MOTION_CONTROLLED_Z)) {
		bool finished{false};
		Vector pos{}; QAngle ang{};
		const float interval{m_pCustomBot->GetAnimTimeInterval()};
		if(m_pCustomBot->GetIntervalMovement(interval, finished, pos, ang)) {
			const Vector &feet{locomotion->GetFeet()};
			if(HasActivityType(MOTION_CONTROLLED_Z) && !HasActivityType(MOTION_CONTROLLED_XY)) {
				pos.x = feet.x;
				pos.y = feet.y;
			} else if(HasActivityType(MOTION_CONTROLLED_XY) && !HasActivityType(MOTION_CONTROLLED_Z))
				pos.z = feet.z;

			locomotion->DriveTo(pos);

			if(HasActivityType(MOTION_CONTROLLED_XY)) {
				QAngle angles{m_pCustomBot->GetLocalAngles()};

				const float angleDiff{UTIL_AngleDiff(ang.y, angles.y)};
				const float deltaYaw{250.0f * deltaT};

				if(angleDiff < -deltaYaw)
					angles.y -= deltaYaw;
				else if(angleDiff > deltaYaw)
					angles.y += deltaYaw;
				else
					angles.y += angleDiff;

				angles.x = ang.x;
				angles.z = ang.z;

				m_pCustomBot->SetLocalAngles(angles);
			}
		}
	}

	if(m_pCustomBot->IsSequenceFinished()) {
		if(m_nDesiredActivity != ACT_INVALID)
			m_pCustomBot->OnAnimationActivityComplete(m_nDesiredActivity);
		if(HasActivityType(ACTIVITY_TRANSITORY)) {
			if(m_nOldDesiredActivity != ACT_INVALID)
				StartActivity(m_nOldDesiredActivity, m_nOldActivityFlags);
		} else if(!HasActivityType(ACTIVITY_UNINTERRUPTIBLE)) {
			if(m_nDesiredActivity != ACT_INVALID)
				m_pCustomBot->ResetSequence(SelectAnimationSequence(m_nDesiredActivity));
		}
	}

	const QAngle &currentAngles{m_pCustomBot->EyeAngles()};

	bool isSteady{true};
	const float actualPitchRate{AngleDiff(currentAngles.x, m_priorAngles.x)};
	if(abs(actualPitchRate) > nb_head_aim_steady_max_rate.GetFloat() * deltaT) {
		isSteady = false;
	} else {
		float actualYawRate = AngleDiff(currentAngles.y, m_priorAngles.y);
		if(abs(actualYawRate) > nb_head_aim_steady_max_rate.GetFloat() * deltaT)
			isSteady = false;
	}

	m_priorAngles = currentAngles;

	if(isSteady) {
		if(!m_headSteadyTimer.HasStarted())
			m_headSteadyTimer.Start();
	} else
		m_headSteadyTimer.Invalidate();

	const Vector &eyepos{GetEyePosition()};

	if(m_pCustomBot->IsDebugging(NEXTBOT_LOOK_AT)) {
		if(IsHeadSteady()) {
			float t{GetHeadSteadyDuration() / 3.0f};
			t = clamp(t, 0.0f, 1.0f);
			NDebugOverlay::Circle(eyepos, t * 10.0f, 0, 255, 0, 255, true, 2.0f * deltaT);
		}
	}

	if(m_hasBeenSightedIn && m_lookAtExpireTimer.IsElapsed())
		return;

	const Vector &view{GetViewVector()};
	const float deltaAngle{RAD2DEG(acos(DotProduct(view, m_anchorForward)))};
	if(deltaAngle > nb_head_aim_resettle_angle.GetFloat()) {
		m_anchorRepositionTimer.Start(RandomFloat(0.9f, 1.1f) * nb_head_aim_resettle_time.GetFloat());
		m_anchorForward = view;
		return;
	}

	if(m_anchorRepositionTimer.HasStarted() && !m_anchorRepositionTimer.IsElapsed())
		return;

	m_anchorRepositionTimer.Invalidate();

	CBaseEntity *const subject{m_lookAtSubject};
	if(subject) {
		if(m_lookAtTrackingTimer.IsElapsed()) {
			Vector desiredLookAtPos{};
			if(!subject->MyCombatCharacterPointer())
				desiredLookAtPos = subject->WorldSpaceCenter();
			desiredLookAtPos += GetHeadAimSubjectLeadTime() * subject->GetAbsVelocity();
			Vector errorVector{desiredLookAtPos - m_lookAtPos};
			const float error{errorVector.NormalizeInPlace()};
			float trackingInterval{GetHeadAimTrackingInterval()};
			if(trackingInterval < deltaT)
				trackingInterval = deltaT;
			float errorVel = error / trackingInterval;
			m_lookAtVelocity = (errorVel * errorVector) + subject->GetAbsVelocity();
			m_lookAtTrackingTimer.Start(RandomFloat(0.8f, 1.2f) * trackingInterval);
		}
		m_lookAtPos += deltaT * m_lookAtVelocity;
	}

	Vector to{(m_lookAtPos - eyepos)};
	to.NormalizeInPlace();

	if(m_pCustomBot->IsDebugging(NEXTBOT_LOOK_AT)) {
		NDebugOverlay::Line(eyepos, eyepos + 100.0f * view, 255, 255, 0, false, 2.0f * deltaT);
		const float thickness{isSteady ? 2.0f : 3.0f};
		const int r{m_isSightedIn ? 255 : 0};
		const int g{subject ? 255 : 0};
		NDebugOverlay::HorzArrow(eyepos, m_lookAtPos, thickness, r, g, 255, 255, false, 2.0f * deltaT);
	}

	const float dot{DotProduct(view, to)};
	if(dot > 0.98f) {
		m_isSightedIn = true;
		if(!m_hasBeenSightedIn) {
			m_hasBeenSightedIn = true;
			if(m_pCustomBot->IsDebugging(NEXTBOT_LOOK_AT))
				ConColorMsg(Color{255, 100, 0, 255}, "%3.2f: %s Look At SIGHTED IN\n", gpGlobals->curtime, m_pCustomBot->GetDebugIdentifier());
		}
		if(m_lookAtReplyWhenAimed) {
			m_lookAtReplyWhenAimed->OnSuccess(m_pCustomBot);
			m_lookAtReplyWhenAimed = nullptr;
		}
	} else
		m_isSightedIn = false;

	float approachRate{GetMaxHeadAngularVelocity()};

	const float easeOut{0.7f};
	if(dot > easeOut) {
		const float t{RemapVal(dot, easeOut, 1.0f, 1.0f, 0.02f)};
		approachRate *= sin(1.57f * t);
	}

	const float easeInTime{0.25f};
	if(m_lookAtDurationTimer.GetElapsedTime() < easeInTime)
		approachRate *= m_lookAtDurationTimer.GetElapsedTime() / easeInTime;

	QAngle desiredAngles{};
	VectorAngles(to, desiredAngles);

	QAngle angles{};
	angles.y = ApproachAngle(desiredAngles.y, currentAngles.y, approachRate * deltaT);
	angles.x = ApproachAngle(desiredAngles.x, currentAngles.x, 0.5f * approachRate * deltaT);
	angles.z = 0.0f;

	angles.x = AngleNormalize(angles.x);
	angles.y = AngleNormalize(angles.y);

	angles.x = clamp(angles.x, -90.0f, 45.0f);
	angles.y = clamp(angles.y, -180.0f, 180.0f);
}

float CBaseNextBotBody::GetMaxHeadAngularVelocity() const
{
	return nb_saccade_speed.GetFloat();
}

void CBaseNextBotBody::ClearPendingAimReply()
{
	m_lookAtReplyWhenAimed = nullptr;
}

float CBaseNextBotBody::GetHeadSteadyDuration() const
{
	//return IsHeadAimingOnTarget() ? m_headSteadyTimer.GetElapsedTime() : 0.0f;
	return m_headSteadyTimer.HasStarted() ? m_headSteadyTimer.GetElapsedTime() : 0.0f;
}

bool CBaseNextBotBody::IsHeadSteady() const
{
	return m_headSteadyTimer.HasStarted();
}

bool CBaseNextBotBody::IsHeadAimingOnTarget() const
{
	return m_isSightedIn;
}

void CBaseNextBotBody::AimHeadTowards(const Vector &lookAtPos, const LookAtPriorityType priority, float duration, INextBotReply *const replyWhenAimed, const char *const reason)
{
	if(duration <= 0.0f)
		duration = 0.1f;

	if(m_lookAtPriority == priority) {
		if(!IsHeadSteady() || GetHeadSteadyDuration() < nb_head_aim_settle_duration.GetFloat()) {
			if(replyWhenAimed)
				replyWhenAimed->OnFail(m_pCustomBot, INextBotReply::DENIED);
			if(m_pCustomBot->IsDebugging(NEXTBOT_LOOK_AT))
				ConColorMsg(Color{255, 0, 0, 255}, "%3.2f: %s Look At '%s' rejected - previous aim not %s\n", gpGlobals->curtime, m_pCustomBot->GetDebugIdentifier(), reason, IsHeadSteady() ? "settled long enough" : "head-steady");
			return;
		}
	}

	if(m_lookAtPriority > priority && !m_lookAtExpireTimer.IsElapsed()) {
		if(replyWhenAimed)
			replyWhenAimed->OnFail(m_pCustomBot, INextBotReply::DENIED);
		if(m_pCustomBot->IsDebugging(NEXTBOT_LOOK_AT))
			ConColorMsg(Color{255, 0, 0, 255}, "%3.2f: %s Look At '%s' rejected - higher priority aim in progress\n", gpGlobals->curtime, m_pCustomBot->GetDebugIdentifier(), reason);
		return;
	}

	if(m_lookAtReplyWhenAimed)
		m_lookAtReplyWhenAimed->OnFail(m_pCustomBot, INextBotReply::INTERRUPTED);

	m_lookAtReplyWhenAimed = replyWhenAimed;
	m_lookAtExpireTimer.Start(duration);

	if((m_lookAtPos - lookAtPos).IsLengthLessThan(1.0f)) {
		m_lookAtPriority = priority;
		return;
	}

	m_lookAtPos = lookAtPos;
	m_lookAtSubject = nullptr;
	m_lookAtPriority = priority;
	m_lookAtDurationTimer.Start();
	//m_isSightedIn = false;
	m_hasBeenSightedIn = false;

	if(m_pCustomBot->IsDebugging(NEXTBOT_LOOK_AT)) {
		NDebugOverlay::Cross3D(lookAtPos, 2.0f, 255, 255, 100, true, 2.0f * duration);
		const char *priName{""};
		switch(priority) {
			case BORING:		priName = "BORING"; break;
			case INTERESTING:	priName = "INTERESTING"; break;
			case IMPORTANT:		priName = "IMPORTANT"; break;
			case CRITICAL:		priName = "CRITICAL"; break;
		}
		ConColorMsg(Color{255, 100, 0, 255}, "%3.2f: %s Look At ( %g, %g, %g ) for %3.2f s, Pri = %s, Reason = %s\n", gpGlobals->curtime, m_pCustomBot->GetDebugIdentifier(), lookAtPos.x, lookAtPos.y, lookAtPos.z, duration, priName, (reason) ? reason : "");
	}
}

void CBaseNextBotBody::AimHeadTowards(CBaseEntity *const subject, const LookAtPriorityType priority, float duration, INextBotReply *const replyWhenAimed, const char *const reason)
{
	if(duration <= 0.0f)
		duration = 0.1f;

	if(subject == nullptr)
		return;

	if(m_lookAtPriority == priority) {
		if(!IsHeadSteady() || GetHeadSteadyDuration() < nb_head_aim_settle_duration.GetFloat()) {
			if(replyWhenAimed)
				replyWhenAimed->OnFail(m_pCustomBot, INextBotReply::DENIED);
			if(m_pCustomBot->IsDebugging(NEXTBOT_LOOK_AT))
				ConColorMsg(Color{255, 0, 0, 255}, "%3.2f: %s Look At '%s' rejected - previous aim not %s\n", gpGlobals->curtime, m_pCustomBot->GetDebugIdentifier(), reason, IsHeadSteady() ? "head-steady" : "settled long enough");
			return;
		}
	}

	if(m_lookAtPriority > priority && !m_lookAtExpireTimer.IsElapsed()) {
		if(replyWhenAimed)
			replyWhenAimed->OnFail(m_pCustomBot, INextBotReply::DENIED);
		if(m_pCustomBot->IsDebugging(NEXTBOT_LOOK_AT))
			ConColorMsg(Color{255, 0, 0, 255}, "%3.2f: %s Look At '%s' rejected - higher priority aim in progress\n", gpGlobals->curtime, m_pCustomBot->GetDebugIdentifier(), reason);
		return;
	}

	if(m_lookAtReplyWhenAimed)
		m_lookAtReplyWhenAimed->OnFail(m_pCustomBot, INextBotReply::INTERRUPTED);

	m_lookAtReplyWhenAimed = replyWhenAimed;
	m_lookAtExpireTimer.Start(duration);

	if(subject == m_lookAtSubject) {
		m_lookAtPriority = priority;
		return;
	}

	m_lookAtSubject = subject;
	m_lookAtPriority = priority;
	m_lookAtDurationTimer.Start();
	//m_isSightedIn = false;
	m_hasBeenSightedIn = false;

	if(m_pCustomBot->IsDebugging(NEXTBOT_LOOK_AT)) {
		NDebugOverlay::Cross3D(m_lookAtPos, 2.0f, 100, 100, 100, true, duration);
		const char *priName = "";
		switch(priority) {
			case BORING:		priName = "BORING"; break;
			case INTERESTING:	priName = "INTERESTING"; break;
			case IMPORTANT:		priName = "IMPORTANT"; break;
			case CRITICAL:		priName = "CRITICAL"; break;
		}
		ConColorMsg(Color{255, 100, 0, 255}, "%3.2f: %s Look At subject %s for %3.2f s, Pri = %s, Reason = %s\n", gpGlobals->curtime, m_pCustomBot->GetDebugIdentifier(), subject->GetClassname(), duration, priName, (reason) ? reason : "");
	}
}