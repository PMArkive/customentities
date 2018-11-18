#include <util_shared.h>
#include <model_types.h>

CTraceFilterSimple::CTraceFilterSimple(const IHandleEntity *const passentity, const int collisionGroup, const ShouldHitFunc_t pExtraShouldHitCheckFn)
	: m_pPassEnt{passentity}, m_collisionGroup{collisionGroup}, m_pExtraShouldHitCheckFunction{pExtraShouldHitCheckFn} {}

bool PassServerEntityFilter(const IHandleEntity *const pTouch, const IHandleEntity *const pPass)
{
	if(!pPass)
		return true;

	if(pTouch == pPass)
		return false;

	const CBaseEntity *const pEntTouch{EntityFromEntityHandle(pTouch)};
	const CBaseEntity *const pEntPass{EntityFromEntityHandle(pPass)};
	if(!pEntTouch || !pEntPass)
		return true;

	if(pEntTouch->GetOwnerEntity() == pEntPass)
		return false;

	if(pEntPass->GetOwnerEntity() == pEntTouch)
		return false;

	return true;
}

bool StandardFilterRules(IHandleEntity *const pHandleEntity, const int fContentsMask)
{
	CBaseEntity *const pCollide{EntityFromEntityHandle(pHandleEntity)};
	if(!pCollide)
		return true;

	const SolidType_t solid{pCollide->GetSolid()};
	const model_t *const pModel{pCollide->GetModel()};

	if((modelinfo->GetModelType(pModel) != mod_brush) || (solid != SOLID_BSP && solid != SOLID_VPHYSICS)) {
		if(!(fContentsMask & CONTENTS_MONSTER))
			return false;
	}

	if(!(fContentsMask & CONTENTS_WINDOW) && pCollide->IsTransparent())
		return false;

	if(!(fContentsMask & CONTENTS_MOVEABLE) && (pCollide->GetMoveType() == MOVETYPE_PUSH))
		return false;

	return true;
}

bool CTraceFilterSimple::ShouldHitEntity(IHandleEntity *const pHandleEntity, const int contentsMask)
{
	if(!StandardFilterRules(pHandleEntity, contentsMask))
		return false;

	if(m_pPassEnt) {
		if(!PassServerEntityFilter(pHandleEntity, m_pPassEnt))
			return false;
	}

	CBaseEntity *const pEntity{EntityFromEntityHandle(pHandleEntity)};
	if(!pEntity)
		return false;
	if(!pEntity->ShouldCollide(m_collisionGroup, contentsMask))
		return false;
	//if(pEntity && !g_pGameRules->ShouldCollide(m_collisionGroup, pEntity->GetCollisionGroup()))
	//	return false;
	if(m_pExtraShouldHitCheckFunction && (!(m_pExtraShouldHitCheckFunction(pHandleEntity, contentsMask))))
		return false;

	return true;
}

float UTIL_VecToYaw( const Vector &vec )
{
	if (vec.y == 0 && vec.x == 0)
		return 0;
	
	float yaw = atan2( vec.y, vec.x );

	yaw = RAD2DEG(yaw);

	if (yaw < 0)
		yaw += 360;

	return yaw;
}

//-----------------------------------------------------------------------------
// Purpose: Trace filter that only hits anything but NPCs and the player
//-----------------------------------------------------------------------------
bool CTraceFilterNoNPCsOrPlayer::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( !pEntity )
			return NULL;
#ifndef CLIENT_DLL
		if ( pEntity->Classify() == CLASS_PLAYER_ALLY )
			return false; // CS hostages are CLASS_PLAYER_ALLY but not IsNPC()
#endif
		return (!pEntity->IsNPC() && !pEntity->IsPlayer());
	}
	return false;
}