#include <basecombatcharacter.h>

class CTraceFilterNoCombatCharacters : public CTraceFilterSimple
{
public:
	CTraceFilterNoCombatCharacters( const IHandleEntity *passentity = NULL, int collisionGroup = COLLISION_GROUP_NONE )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
		{
			CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
			if ( !pEntity )
				return NULL;

			if ( pEntity->MyCombatCharacterPointer() || pEntity->MyCombatWeaponPointer() )
				return false;

			// Honor BlockLOS - this lets us see through partially-broken doors, etc
			if ( !pEntity->BlocksLOS() )
				return false;

			return true;
		}

		return false;
	}
};

bool CBaseCombatCharacter::ComputeLOS(const Vector &vecEyePosition, const Vector &vecTarget) const
{
	// We simply can't see because the world is in the way.
	trace_t result;
	CTraceFilterNoCombatCharacters traceFilter(NULL, COLLISION_GROUP_NONE);
	UTIL_TraceLine(vecEyePosition, vecTarget, MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_MONSTER, &traceFilter, &result);
	return (result.fraction == 1.0f);
}

bool CBaseCombatCharacter::IsAbleToSee( const CBaseEntity *pEntity, FieldOfViewCheckType checkFOV )
{
	CBaseCombatCharacter *pBCC = const_cast<CBaseEntity *>( pEntity )->MyCombatCharacterPointer();
	if ( pBCC )
		return IsAbleToSee( pBCC, checkFOV );

	// Test this every time; it's cheap.
	Vector vecEyePosition = EyePosition();
	Vector vecTargetPosition = pEntity->WorldSpaceCenter();

#ifdef GAME_DLL
	Vector vecEyeToTarget;
	VectorSubtract( vecTargetPosition, vecEyePosition, vecEyeToTarget );
	float flDistToOther = VectorNormalize( vecEyeToTarget ); 

	// We can't see because they are too far in the fog
	if ( IsHiddenByFog( flDistToOther ) )
		return false;
#endif

	if ( !ComputeLOS( vecEyePosition, vecTargetPosition ) )
		return false;

#if defined(GAME_DLL) && defined(TERROR)
	if ( flDistToOther > NavObscureRange.GetFloat() )
	{
		const float flMaxDistance = 100.0f;
		TerrorNavArea *pTargetArea = static_cast< TerrorNavArea* >( TheNavMesh->GetNearestNavArea( vecTargetPosition, false, flMaxDistance ) );
		if ( !pTargetArea || pTargetArea->HasSpawnAttributes( TerrorNavArea::SPAWN_OBSCURED ) )
			return false;

		if ( ComputeTargetIsInDarkness( vecEyePosition, pTargetArea, vecTargetPosition ) )
			return false;
	}
#endif

	return ( checkFOV != USE_FOV || IsInFieldOfView( vecTargetPosition ) );
}

static void ComputeSeeTestPosition( Vector *pEyePosition, CBaseCombatCharacter *pBCC )
{
#if defined(GAME_DLL) && defined(TERROR)
	if ( pBCC->IsPlayer() )
	{
		CTerrorPlayer *pPlayer = ToTerrorPlayer( pBCC );
		*pEyePosition = !pPlayer->IsDead() ? pPlayer->EyePosition() : pPlayer->GetDeathPosition();
	}
	else
#endif
	{
		*pEyePosition = pBCC->EyePosition();
	}	
}

bool CBaseCombatCharacter::IsAbleToSee( CBaseCombatCharacter *pBCC, FieldOfViewCheckType checkFOV )
{
	Vector vecEyePosition, vecOtherEyePosition;
	ComputeSeeTestPosition( &vecEyePosition, this );
	ComputeSeeTestPosition( &vecOtherEyePosition, pBCC );

#ifdef GAME_DLL
	Vector vecEyeToTarget;
	VectorSubtract( vecOtherEyePosition, vecEyePosition, vecEyeToTarget );
	float flDistToOther = VectorNormalize( vecEyeToTarget ); 

	// Test this every time; it's cheap.
	// We can't see because they are too far in the fog
	if ( IsHiddenByFog( flDistToOther ) )
		return false;

#ifdef TERROR
	// Check this every time also, it's cheap; check to see if the enemy is in an obscured area.
	bool bIsInNavObscureRange = ( flDistToOther > NavObscureRange.GetFloat() );
	if ( bIsInNavObscureRange )
	{
		TerrorNavArea *pOtherNavArea = static_cast< TerrorNavArea* >( pBCC->GetLastKnownArea() );
		if ( !pOtherNavArea || pOtherNavArea->HasSpawnAttributes( TerrorNavArea::SPAWN_OBSCURED ) )
			return false;
	}
#endif // TERROR
#endif

	/*
	// Check if we have a cached-off visibility
	int iCache = s_CombatCharVisCache.LookupVisibility( this, pBCC );
	VisCacheResult_t nResult = s_CombatCharVisCache.HasVisibility( iCache );

	// Compute symmetric visibility
	if ( nResult == VISCACHE_UNKNOWN )
	{
		bool bThisCanSeeOther = false, bOtherCanSeeThis = false;
		if ( ComputeLOS( vecEyePosition, vecOtherEyePosition ) )
		{
#if defined(GAME_DLL) && defined(TERROR)
			if ( !bIsInNavObscureRange )
			{
				bThisCanSeeOther = true, bOtherCanSeeThis = true;
			}
			else
			{
				bThisCanSeeOther = !ComputeTargetIsInDarkness( vecEyePosition, pBCC->GetLastKnownArea(), vecOtherEyePosition );
				bOtherCanSeeThis = !ComputeTargetIsInDarkness( vecOtherEyePosition, GetLastKnownArea(), vecEyePosition );
			}
#else
			bThisCanSeeOther = true, bOtherCanSeeThis = true;
#endif
		}

		s_CombatCharVisCache.RegisterVisibility( iCache, bThisCanSeeOther, bOtherCanSeeThis );
		nResult = bThisCanSeeOther ? VISCACHE_IS_VISIBLE : VISCACHE_IS_NOT_VISIBLE;
	}

	if ( nResult == VISCACHE_IS_VISIBLE )
		return ( checkFOV != USE_FOV || IsInFieldOfView( pBCC ) );
	*/

	return false;
}