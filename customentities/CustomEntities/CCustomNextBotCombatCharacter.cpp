#include "CCustomNextBotCombatCharacter.h"

extern ConVar NextBotStop;

ILocomotion *CCustomNextBotCombatCharacter::GetLocomotionInterface() const
{
	if(m_baseLocomotion == nullptr)
		m_baseLocomotion = new NextBotGroundLocomotion{const_cast<CCustomNextBotCombatCharacter *const>(this)};
	return m_baseLocomotion;
}

IBody *CCustomNextBotCombatCharacter::GetBodyInterface() const
{
	if(m_baseBody == nullptr)
		m_baseBody = new IBody{const_cast<CCustomNextBotCombatCharacter *const>(this)};
	return m_baseBody;
}

IIntention *CCustomNextBotCombatCharacter::GetIntentionInterface() const
{
	if(m_baseIntention == nullptr)
		m_baseIntention = new IIntention{const_cast<CCustomNextBotCombatCharacter *const>(this)};
	return m_baseIntention;
}

IVision *CCustomNextBotCombatCharacter::GetVisionInterface() const
{
	if(m_baseVision == nullptr)
		m_baseVision = new IVision{const_cast<CCustomNextBotCombatCharacter *const>(this)};
	return m_baseVision;
}

CCustomNextBotCombatCharacter::CCustomNextBotCombatCharacter() noexcept
	: INextBot(), CCustomCombatCharacter()
{
	
}

CCustomNextBotCombatCharacter::~CCustomNextBotCombatCharacter() noexcept
{
	if(m_baseIntention)
		delete m_baseIntention;

	if(m_baseLocomotion)
		delete m_baseLocomotion;

	if(m_baseBody)
		delete m_baseBody;

	if(m_baseVision)
		delete m_baseVision;
}

void CCustomNextBotCombatCharacter::Spawn() noexcept
{
	__super::Spawn();

	GetLocomotionInterface();
	GetBodyInterface();
	GetVisionInterface();
	GetIntentionInterface();

	Reset();

	SetSimulatedEveryTick(true);

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	AddEFlags(EFL_DONTWALKON);
	SetMoveType(MOVETYPE_CUSTOM, MOVECOLLIDE_DEFAULT);
	SetCollisionGroup(COLLISION_GROUP_NPC);
	AddFlag(FL_NPC|FL_AIMTARGET|FL_OBJECT);

	m_iMaxHealth = m_iHealth;
	m_takedamage = DAMAGE_YES;

	m_iTeamNum = TEAM_ANY;
	m_iInitialTeamNum = TEAM_ANY;

	m_flPlaybackRate = 1.0f;
	SetAnimatedEveryTick(true);
	InitBoneControllers();

	const Vector &mins{GetBodyInterface()->GetHullMins()};
	const Vector &maxs{GetBodyInterface()->GetHullMaxs()};

	CollisionProp()->SetCollisionBounds(mins, maxs);
	//CollisionProp()->SetSurroundingBoundsType(USE_ROTATION_EXPANDED_SEQUENCE_BOUNDS, &mins, &maxs);

	SetNextThink(gpGlobals->curtime, nullptr);
}

void CCustomNextBotCombatCharacter::SetModel(const char *const model) noexcept
{
	__super::SetModel(model);
	m_didModelChange = true;
}

const bool CCustomNextBotCombatCharacter::IsAreaTraversable(const CNavArea *const area) const noexcept
{
	if(!GetLocomotionInterface()->IsAreaTraversable(area))
		return false;
	return __super::IsAreaTraversable(area);
}

const int32 CCustomNextBotCombatCharacter::OnTakeDamage_Alive(const CTakeDamageInfo &info) noexcept
{
	OnInjured(info);
	return __super::OnTakeDamage_Alive(info);
}

const int32 CCustomNextBotCombatCharacter::OnTakeDamage_Dying(const CTakeDamageInfo &info) noexcept
{
	OnInjured(info);
	return __super::OnTakeDamage_Dying(info);
}

const Vector CCustomNextBotCombatCharacter::EyePosition() const noexcept
{
	return GetBodyInterface()->GetEyePosition();
}

/*
void CCustomNextBotCombatCharacter::OnNavAreaChanged(CNavArea *enteredArea, CNavArea *leftArea) noexcept
{
	INextBotEventResponder::OnNavAreaChanged(enteredArea, leftArea);
	CCustomCombatCharacter::OnNavAreaChanged(enteredArea, leftArea);
}
*/

const bool CCustomNextBotCombatCharacter::BecomeRagdoll(const CTakeDamageInfo &info, const Vector &forceVector) noexcept
{
	Vector adjustedForceVector{forceVector};
	/*
	const CRagdollMagnet *const magnet{CRagdollMagnet::FindBestMagnet(this)};
	if(magnet)
		adjustedForceVector += magnet->GetForceVector(this);

	EmitSound("BaseCombatCharacter.StopWeaponSounds");
	*/

	return __super::BecomeRagdoll(info, adjustedForceVector);
}

void CCustomNextBotCombatCharacter::Event_Killed(const CTakeDamageInfo &info) noexcept
{
	OnKilled(info);

	m_lifeState = LIFE_DYING;

	CBaseEntity *const pOwner{GetOwnerEntity()};
	if(pOwner)
		pOwner->DeathNotice(GetEntity());

	//__super::Event_Killed(info);
}

void CCustomNextBotCombatCharacter::Ignite(const float32 flFlameLifetime, const bool bNPCOnly, const float32 flSize, const bool bCalledByLevelDesigner) noexcept
{
	__super::Ignite(flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner);
	OnIgnite();
}

const uint32 CCustomNextBotCombatCharacter::PhysicsSolidMaskForEntity() const noexcept
{
	return MASK_NPCSOLID;
}

void CCustomNextBotCombatCharacter::HandleAnimEvent(animevent_t *const pEvent) noexcept
{
	__super::HandleAnimEvent(pEvent);
	OnAnimationEvent(pEvent);
}

void CCustomNextBotCombatCharacter::PerformCustomPhysics(Vector *const pNewPosition, Vector *const pNewVelocity, QAngle *const pNewAngles, QAngle *const pNewAngVelocity) noexcept
{
	__super::PerformCustomPhysics(pNewPosition, pNewVelocity, pNewAngles, pNewAngVelocity);

	SetGroundEntity(GetLocomotionInterface()->GetGround());
}

void CCustomNextBotCombatCharacter::Think() noexcept
{
	SetNextThink(gpGlobals->curtime, nullptr);

	if(BeginUpdate()) {
		if(m_didModelChange) {
			m_didModelChange = false;

			OnModelChanged();

			for(INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder(sub))
				sub->OnModelChanged();
		}

		StudioFrameAdvance();
		DispatchAnimEvents();

		UpdateLastKnownArea();

		if(!NextBotStop.GetBool() && !IsFlagSet(FL_FROZEN))
			Update();

		EndUpdate();
	}
}