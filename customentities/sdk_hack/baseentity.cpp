#include "baseentity.h"
#include <CCustomBaseEntity.h>

bool g_bUseNetworkVars{true};

ConVar sv_teststepsimulation{"sv_teststepsimulation", "1", FCVAR_GAMEDLL};
ConVar phys_pushscale{"phys_pushscale", "1", FCVAR_GAMEDLL|FCVAR_REPLICATED};

CSharedEdictChangeInfo *g_pSharedChangeInfo{nullptr};

IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
	return engine->GetChangeAccessor((const edict_t *)this);
}

const IChangeInfoAccessor *CBaseEdict::GetChangeAccessor() const
{
	return engine->GetChangeAccessor((const edict_t *)this);
}

void CBaseEntity::SetAbsOrigin(const Vector &absOrigin)
{
	CalcAbsolutePosition();

	if(m_vecAbsOrigin == absOrigin)
		return;

	InvalidatePhysicsRecursive(POSITION_CHANGED);
	RemoveEFlags(EFL_DIRTY_ABSTRANSFORM);

	m_vecAbsOrigin = absOrigin;

	MatrixSetColumn(absOrigin, 3, m_rgflCoordinateFrame);

	Vector vecNewOrigin{};
	CBaseEntity *pMoveParent{GetMoveParent()};
	if(!pMoveParent)
		vecNewOrigin = absOrigin;
	else {
		matrix3x4_t tempMat{};
		matrix3x4_t &parentTransform{GetParentToWorldTransform(tempMat)};
		VectorITransform(absOrigin, parentTransform, vecNewOrigin);
	}

	if(m_vecOrigin != vecNewOrigin) {
		m_vecOrigin = vecNewOrigin;
		SetSimulationTime(gpGlobals->curtime);
		//UpdateCell();
	}
}

const CCollisionProperty *CBaseEntity::CollisionProp() const
{
	return const_cast<CBaseEntity *const>(this)->CollisionProp();
}

CCollisionProperty *CBaseEntity::CollisionProp()
{
	return reinterpret_cast<CCollisionProperty *const>(GetCollideable());
}

CServerNetworkProperty *CBaseEntity::NetworkProp()
{
	return reinterpret_cast<CServerNetworkProperty *const>(GetNetworkable());
}

const Vector &CBaseEntity::GetAbsOrigin() const
{
	if(IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
		const_cast<CBaseEntity *const>(this)->CalcAbsolutePosition();

	return m_vecAbsOrigin;
}

bool CBaseEntity::ClassMatches(const char *const pszClassOrWildcard)
{
	if(IDENT_STRINGS(m_iClassname, pszClassOrWildcard))
		return true;

	return ClassMatchesComplex(pszClassOrWildcard);
}

const char *CBaseEntity::GetClassname()
{
	return STRING(m_iClassname);
}

void CBaseEntity::EntityText(const int text_offset, const char *const text, const float duration, const int r, const int g, const int b, const int a)
{
	Vector origin{};
	Vector vecLocalCenter{};

	VectorAdd(CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), vecLocalCenter);
	vecLocalCenter *= 0.5f;

	if((CollisionProp()->GetCollisionAngles() == vec3_angle) || (vecLocalCenter == vec3_origin))
		VectorAdd(vecLocalCenter, CollisionProp()->GetCollisionOrigin(), origin);
	else
		VectorTransform(vecLocalCenter, CollisionProp()->CollisionToWorldTransform(), origin);

	NDebugOverlay::EntityTextAtPosition(origin, text_offset, text, duration, r, g, b, a);
}

CTeam *CBaseEntity::GetTeam() const
{
	return nullptr;
}

int CBaseEntity::GetTeamNumber() const
{
	return m_iTeamNum;
}

bool CBaseEntity::IsAIWalkable()
{
	return !IsEFlagSet(EFL_DONTWALKON);
}

void CBaseEntity::RemoveEFlags(const int nEFlagMask)
{
	m_iEFlags &= ~nEFlagMask;

	if(nEFlagMask & (EFL_FORCE_CHECK_TRANSMIT|EFL_IN_SKYBOX))
		DispatchUpdateTransmitState();
}

bool CBaseEntity::IsEFlagSet(int flag) const
{
	return !!(m_iEFlags & flag);
}

int CBaseEntity::DispatchUpdateTransmitState()
{
	const edict_t *const ed{edict()};
	if(m_nTransmitStateOwnedCounter != 0)
		return (ed ? ed->m_fStateFlags : 0);

	return UpdateTransmitState();
}

void CBaseEntity::SetSimulationTime(const float time)
{ 
	m_flSimulationTime = time;
}

void CBaseEntity::CalcAbsolutePosition()
{
	if(!IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
		return;

	RemoveEFlags(EFL_DIRTY_ABSTRANSFORM);

	AngleMatrix(m_angRotation, m_vecOrigin, m_rgflCoordinateFrame);

	CBaseEntity *const pMoveParent{GetMoveParent()};
	if(!pMoveParent) {
		m_vecAbsOrigin = m_vecOrigin;
		m_angAbsRotation = m_angRotation;
		return;
	}

	matrix3x4_t tmpMatrix{};
	matrix3x4_t scratchSpace{};
	ConcatTransforms(GetParentToWorldTransform(scratchSpace), m_rgflCoordinateFrame, tmpMatrix);
	MatrixCopy(tmpMatrix, m_rgflCoordinateFrame);

	MatrixGetColumn(m_rgflCoordinateFrame, 3, m_vecAbsOrigin);

	if((m_angRotation == vec3_angle) && (m_iParentAttachment == 0))
		VectorCopy(pMoveParent->GetAbsAngles(), m_angAbsRotation);
	else
		MatrixAngles(m_rgflCoordinateFrame, m_angAbsRotation);
}

bool CBaseEntity::IsMarkedForDeletion()
{
	return IsEFlagSet(EFL_KILLME);
}

const QAngle &CBaseEntity::GetAbsAngles() const
{
	if(IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
		const_cast<CBaseEntity *const>(this)->CalcAbsolutePosition();

	return m_angAbsRotation;
}

matrix3x4_t &CBaseEntity::GetParentToWorldTransform(matrix3x4_t &tempMatrix)
{
	CBaseEntity *const pMoveParent{GetMoveParent()};
	if(!pMoveParent) {
		SetIdentityMatrix(tempMatrix);
		return tempMatrix;
	}

	if(m_iParentAttachment != 0) {
		CBaseAnimating *const pAnimating{pMoveParent->GetBaseAnimating()};
		if(pAnimating && pAnimating->GetAttachment(m_iParentAttachment, tempMatrix))
			return tempMatrix;
	}

	return pMoveParent->EntityToWorldTransform();
}

matrix3x4_t &CBaseEntity::EntityToWorldTransform()
{
	if(IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
		CalcAbsolutePosition();

	return m_rgflCoordinateFrame;
}

const matrix3x4_t &CBaseEntity::EntityToWorldTransform() const
{
	if(IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
		const_cast<CBaseEntity *const>(this)->CalcAbsolutePosition();

	return m_rgflCoordinateFrame;
}

CBaseEntity *CBaseEntity::GetMoveParent()
{
	return m_hMoveParent.Get();
}

CBaseEntity *CBaseEntity::FirstMoveChild()
{
	return m_hMoveChild.Get();
}

CBaseEntity *CBaseEntity::NextMovePeer()
{
	return m_hMovePeer.Get();
}

CBaseEntity *CBaseEntity::GetOwnerEntity() const
{
	return m_hOwnerEntity.Get();
}

CBaseEntity *CBaseEntity::GetGroundEntity()
{
	return m_hGroundEntity.Get();
}

static const bool NamesMatch(const char8 *pszQuery, string_t nameToMatch)
{
	if(nameToMatch == NULL_STRING)
		return (*pszQuery == '\0' || *pszQuery == '*');

	const char *pszNameToMatch{STRING(nameToMatch)};

	if(pszNameToMatch == pszQuery)
		return true;

	while(*pszNameToMatch && *pszQuery) {
		const char cName{*pszNameToMatch};
		const char cQuery{*pszQuery};
		if(cName != cQuery && tolower(cName) != tolower(cQuery))
			break;
		++pszNameToMatch;
		++pszQuery;
	}

	if(*pszQuery == '\0' && *pszNameToMatch == '\0')
		return true;

	if(*pszQuery == '*')
		return true;

	return false;
}

bool CBaseEntity::ClassMatchesComplex(const char *const pszClassOrWildcard)
{
	return NamesMatch(pszClassOrWildcard, m_iClassname);
}

void CBaseEntity::InvalidatePhysicsRecursive(int nChangeFlags)
{
	int nDirtyFlags{0};

	if(nChangeFlags & VELOCITY_CHANGED)
		nDirtyFlags |= EFL_DIRTY_ABSVELOCITY;

	if(nChangeFlags & POSITION_CHANGED) {
		nDirtyFlags |= EFL_DIRTY_ABSTRANSFORM;

		NetworkProp()->MarkPVSInformationDirty();

		CollisionProp()->MarkPartitionHandleDirty();
	}

	if(nChangeFlags & ANGLES_CHANGED) {
		nDirtyFlags |= EFL_DIRTY_ABSTRANSFORM;

		if(CollisionProp()->DoesRotationInvalidateSurroundingBox())
			CollisionProp()->MarkSurroundingBoundsDirty();

		nChangeFlags |= POSITION_CHANGED|VELOCITY_CHANGED;
	}

	AddEFlags(nDirtyFlags);

	bool bOnlyDueToAttachment{false};
	if(nChangeFlags & ANIMATION_CHANGED) {
		if(!(nChangeFlags & (POSITION_CHANGED|VELOCITY_CHANGED|ANGLES_CHANGED)))
			bOnlyDueToAttachment = true;
		nChangeFlags = POSITION_CHANGED|ANGLES_CHANGED|VELOCITY_CHANGED;
	}

	for(CBaseEntity *pChild{FirstMoveChild()}; pChild; pChild = pChild->NextMovePeer()) {
		if(bOnlyDueToAttachment) {
			if(pChild->GetParentAttachment() == 0)
				continue;
		}
		pChild->InvalidatePhysicsRecursive(nChangeFlags);
	}
}

int CBaseEntity::GetParentAttachment()
{
	return m_iParentAttachment;
}

void CBaseEntity::AddEFlags(const int nEFlagMask)
{
	m_iEFlags |= nEFlagMask;

	if(nEFlagMask & (EFL_FORCE_CHECK_TRANSMIT|EFL_IN_SKYBOX))
		DispatchUpdateTransmitState();
}

model_t *CBaseEntity::GetModel()
{
	return const_cast<model_t *const>(modelinfo->GetModel(GetModelIndex()));
}

bool CBaseEntity::IsTransparent() const
{
	return (m_nRenderMode != kRenderNormal);
}

MoveType_t CBaseEntity::GetMoveType() const
{
	return static_cast<const MoveType_t>(m_MoveType.Get());
}

const char *CBaseEntity::GetDebugName()
{
	if(this == NULL)
		return "<<null>>";

	if(m_iName != NULL_STRING)
		return STRING(m_iName);
	else
		return STRING(m_iClassname);
}

bool CBaseEntity::BlocksLOS()
{
	return !IsEFlagSet(EFL_DONTBLOCKLOS);
}

//-----------------------------------------------------------------------------
// Purpose: Scale damage done and call OnTakeDamage
//-----------------------------------------------------------------------------
int CBaseEntity::TakeDamage( const CTakeDamageInfo &inputInfo )
{
	//if ( !g_pGameRules )
	//	return 0;

	/*
	bool bHasPhysicsForceDamage = !g_pGameRules->Damage_NoPhysicsForce( inputInfo.GetDamageType() );
	if ( bHasPhysicsForceDamage && inputInfo.GetDamageType() != DMG_GENERIC )
	{
		// If you hit this assert, you've called TakeDamage with a damage type that requires a physics damage
		// force & position without specifying one or both of them. Decide whether your damage that's causing 
		// this is something you believe should impart physics force on the receiver. If it is, you need to 
		// setup the damage force & position inside the CTakeDamageInfo (Utility functions for this are in
		// takedamageinfo.cpp. If you think the damage shouldn't cause force (unlikely!) then you can set the 
		// damage type to DMG_GENERIC, or | DMG_CRUSH if you need to preserve the damage type for purposes of HUD display.

		if ( inputInfo.GetDamageForce() == vec3_origin || inputInfo.GetDamagePosition() == vec3_origin )
		{
			static int warningCount = 0;
			if ( ++warningCount < 10 )
			{
				if ( inputInfo.GetDamageForce() == vec3_origin )
				{
					DevWarning( "CBaseEntity::TakeDamage:  with inputInfo.GetDamageForce() == vec3_origin\n" );
				}
				if ( inputInfo.GetDamagePosition() == vec3_origin )
				{
					DevWarning( "CBaseEntity::TakeDamage:  with inputInfo.GetDamagePosition() == vec3_origin\n" );
				}
			}
		}
	}
	*/

	// Make sure our damage filter allows the damage.
	if ( !PassesDamageFilter( inputInfo ))
	{
		return 0;
	}

	/*
	if( !g_pGameRules->AllowDamage(this, inputInfo) )
	{
		return 0;
	}
	*/

	/*if ( PhysIsInCallback() )
	{
		PhysCallbackDamage( this, inputInfo );
	}
	else*/
	{
		CTakeDamageInfo info = inputInfo;
		
		// Scale the damage by the attacker's modifier.
		if ( info.GetAttacker() )
		{
			info.ScaleDamage( info.GetAttacker()->GetAttackDamageScale( this ) );
		}

		// Scale the damage by my own modifiers
		info.ScaleDamage( GetReceivedDamageScale( info.GetAttacker() ) );

		//Msg("%s took %.2f Damage, at %.2f\n", GetClassname(), info.GetDamage(), gpGlobals->curtime );

		return OnTakeDamage( info );
	}
	return 0;
}

void CBaseEntity::SetAbsVelocity(const Vector &AbsVelocity)
{
	if(m_vecAbsVelocity == AbsVelocity)
		return;

	InvalidatePhysicsRecursive(VELOCITY_CHANGED);
	RemoveEFlags(EFL_DIRTY_ABSVELOCITY);

	m_vecAbsVelocity = AbsVelocity;

	CBaseEntity *const pMoveParent{GetMoveParent()};
	if(!pMoveParent) {
		m_vecVelocity = AbsVelocity;
		return;
	}

	Vector relVelocity{};
	VectorSubtract(AbsVelocity, pMoveParent->GetAbsVelocity(), relVelocity);

	Vector vNew{};
	VectorIRotate(relVelocity, pMoveParent->EntityToWorldTransform(), vNew);

	m_vecVelocity = vNew;
}

const Vector &CBaseEntity::GetAbsVelocity() const
{
	if(IsEFlagSet(EFL_DIRTY_ABSVELOCITY))
		const_cast<CBaseEntity *const>(this)->CalcAbsoluteVelocity();

	return m_vecAbsVelocity;
}

void CBaseEntity::SetGroundEntity(CBaseEntity *const ground)
{
	if(GetGroundEntity() == ground)
		return;

	m_hGroundEntity = ground;

	if(ground)
		AddFlag(FL_ONGROUND);
	else
		RemoveFlag(FL_ONGROUND);
}

void CBaseEntity::SetLocalAngles(const QAngle &angles)
{
	if(m_angRotation != angles) {
		InvalidatePhysicsRecursive(ANGLES_CHANGED);
		m_angRotation = angles;
		SetSimulationTime(gpGlobals->curtime);
	}
}

const QAngle &CBaseEntity::GetLocalAngles() const
{
	return m_angRotation;
}

int CBaseEntity::GetFlags() const
{
	return m_fFlags;
}

void CBaseEntity::AddFlag(const int nEFlagMask)
{
	m_fFlags |= nEFlagMask;
}

void CBaseEntity::RemoveFlag(const int nEFlagMask)
{
	m_fFlags &= ~nEFlagMask;
}

void CBaseEntity::CalcAbsoluteVelocity()
{
	if(!IsEFlagSet(EFL_DIRTY_ABSVELOCITY))
		return;

	RemoveEFlags(EFL_DIRTY_ABSVELOCITY);

	CBaseEntity *const pMoveParent{GetMoveParent()};
	if(!pMoveParent) {
		m_vecAbsVelocity = m_vecVelocity;
		return;
	}

	VectorRotate(m_vecVelocity, pMoveParent->EntityToWorldTransform(), m_vecAbsVelocity);

	m_vecAbsVelocity += pMoveParent->GetAbsVelocity();
}

int CBaseEntity::entindex() const
{
	return m_Network.entindex();
}

CBaseEntity *CBaseEntity::GetRootMoveParent()
{
	CBaseEntity *pEntity{this};
	CBaseEntity *pParent{GetMoveParent()};
	while(pParent) {
		pEntity = pParent;
		pParent = pEntity->GetMoveParent();
	}
	return pEntity;
}

void CBaseEntity::CollisionRulesChanged()
{

}

bool CBaseEntity::IsCurrentlyTouching() const
{
	return false;
}

void CBaseEntity::SetCheckUntouch(const bool check)
{
	if(check) {
		touchStamp++;
		if(!IsEFlagSet(EFL_CHECK_UNTOUCH))
			AddEFlags(EFL_CHECK_UNTOUCH);
	} else
		RemoveEFlags(EFL_CHECK_UNTOUCH);
}