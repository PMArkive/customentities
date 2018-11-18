#include <collisionproperty.h>

void CCollisionProperty::CollisionAABBToWorldAABB(const Vector &entityMins, const Vector &entityMaxs, Vector *const pWorldMins, Vector *const pWorldMaxs) const
{
	if(!IsBoundsDefinedInEntitySpace() || (GetCollisionAngles() == vec3_angle)) {
		VectorAdd(entityMins, GetCollisionOrigin(), *pWorldMins);
		VectorAdd(entityMaxs, GetCollisionOrigin(), *pWorldMaxs);
	} else
		TransformAABB(CollisionToWorldTransform(), entityMins, entityMaxs, *pWorldMins, *pWorldMaxs);
}

void CCollisionProperty::CalcNearestPoint(const Vector &vecWorldPt, Vector *const pVecNearestWorldPt) const
{
	Vector localPt{};
	Vector localClosestPt{};
	WorldToCollisionSpace(vecWorldPt, &localPt);
	CalcClosestPointOnAABB(m_vecMins, m_vecMaxs, localPt, localClosestPt);
	CollisionToWorldSpace(localClosestPt, pVecNearestWorldPt);
}

float CCollisionProperty::CalcDistanceFromPoint(const Vector &vecWorldPt) const
{
	Vector localPt{};
	Vector localClosestPt{};
	WorldToCollisionSpace(vecWorldPt, &localPt);
	CalcClosestPointOnAABB(m_vecMins, m_vecMaxs, localPt, localClosestPt);
	return localPt.DistTo(localClosestPt);
}

void CCollisionProperty::MarkPartitionHandleDirty()
{
	if(m_pOuter->entindex() == 0)
		return;

	if(!m_pOuter->IsEFlagSet(EFL_DIRTY_SPATIAL_PARTITION))
		m_pOuter->AddEFlags(EFL_DIRTY_SPATIAL_PARTITION);
}

void CCollisionProperty::MarkSurroundingBoundsDirty()
{
	GetOuter()->AddEFlags(EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS);
	MarkPartitionHandleDirty();

	GetOuter()->NetworkProp()->MarkPVSInformationDirty();
}

//-----------------------------------------------------------------------------
// Sets the collision bounds + the size
//-----------------------------------------------------------------------------
void CCollisionProperty::SetCollisionBounds( const Vector &mins, const Vector &maxs )
{
	if ( ( m_vecMinsPreScaled != mins ) || ( m_vecMaxsPreScaled != maxs ) )
	{
		m_vecMinsPreScaled = mins;
		m_vecMaxsPreScaled = maxs;
	}

	bool bDirty = false;

	// Check if it's a scaled model
	CBaseAnimating *pAnim = GetOuter()->GetBaseAnimating();
	if ( pAnim && pAnim->GetModelScale() != 1.0f )
	{
		// Do the scaling
		Vector vecNewMins = mins * pAnim->GetModelScale();
		Vector vecNewMaxs = maxs * pAnim->GetModelScale();

		if ( ( m_vecMins != vecNewMins ) || ( m_vecMaxs != vecNewMaxs ) )
		{
			m_vecMins = vecNewMins;
			m_vecMaxs = vecNewMaxs;
			bDirty = true;
		}
	}
	else
	{
		// No scaling needed!
		if ( ( m_vecMins != mins ) || ( m_vecMaxs != maxs ) )
		{
			m_vecMins = mins;
			m_vecMaxs = maxs;
			bDirty = true;
		}
	}
	
	if ( bDirty )
	{
		//ASSERT_COORD( m_vecMins.Get() );
		//ASSERT_COORD( m_vecMaxs.Get() );

		Vector vecSize;
		VectorSubtract( m_vecMaxs, m_vecMins, vecSize );
		m_flRadius = vecSize.Length() * 0.5f;

		MarkSurroundingBoundsDirty();
	}
}

static void GetAllChildren_r(CBaseEntity *pEntity, CUtlVector<CBaseEntity *> &list) noexcept
{
	for(; pEntity != nullptr; pEntity = pEntity->NextMovePeer()) {
		list.AddToTail(pEntity);
		GetAllChildren_r(pEntity->FirstMoveChild(), list);
	}
}

const int GetAllChildren(CBaseEntity *const pParent, CUtlVector<CBaseEntity *> &list) noexcept
{
	if(!pParent)
		return 0;

	GetAllChildren_r(pParent->FirstMoveChild(), list);
	return list.Count();
}

//-----------------------------------------------------------------------------
// Computes a "normalized" point (range 0,0,0 - 1,1,1) in collision space
//-----------------------------------------------------------------------------
const Vector & CCollisionProperty::NormalizedToCollisionSpace( const Vector &in, Vector *pResult ) const
{
	pResult->x = Lerp( in.x, m_vecMins.Get().x, m_vecMaxs.Get().x );
	pResult->y = Lerp( in.y, m_vecMins.Get().y, m_vecMaxs.Get().y );
	pResult->z = Lerp( in.z, m_vecMins.Get().z, m_vecMaxs.Get().z );
	return *pResult;
}

//-----------------------------------------------------------------------------
// Computes a "normalized" point (range 0,0,0 - 1,1,1) in world space
//-----------------------------------------------------------------------------
const Vector & CCollisionProperty::NormalizedToWorldSpace( const Vector &in, Vector *pResult ) const
{
	Vector vecCollisionSpace;
	NormalizedToCollisionSpace( in, &vecCollisionSpace );
	CollisionToWorldSpace( vecCollisionSpace, pResult );
	return *pResult;
}

//-----------------------------------------------------------------------------
// Sets the solid type
//-----------------------------------------------------------------------------
void CCollisionProperty::SetSolid( SolidType_t val )
{
	if ( m_nSolidType == val )
		return;

#ifndef CLIENT_DLL
	bool bWasNotSolid = IsSolid();
#endif

	MarkSurroundingBoundsDirty();

	// OBB is not yet implemented
	if ( val == SOLID_BSP )
	{
		if ( GetOuter()->GetMoveParent() )
		{
			if ( GetOuter()->GetRootMoveParent()->GetSolid() != SOLID_BSP )
			{
				// must be SOLID_VPHYSICS because parent might rotate
				val = SOLID_VPHYSICS;
			}
		}
#ifndef CLIENT_DLL
		// UNDONE: This should be fine in the client DLL too.  Move GetAllChildren() into shared code.
		// If the root of the hierarchy is SOLID_BSP, then assume that the designer
		// wants the collisions to rotate with this hierarchy so that the player can
		// move while riding the hierarchy.
		if ( !GetOuter()->GetMoveParent() )
		{
			// NOTE: This assumes things don't change back from SOLID_BSP
			// NOTE: This is 100% true for HL2 - need to support removing the flag to support changing from SOLID_BSP
			CUtlVector<CBaseEntity *> list;
			GetAllChildren( GetOuter(), list );
			for ( int i = list.Count()-1; i>=0; --i )
			{
				list[i]->AddSolidFlags( FSOLID_ROOT_PARENT_ALIGNED );
			}
		}
#endif
	}

	m_nSolidType = val;

#ifndef CLIENT_DLL
	m_pOuter->CollisionRulesChanged();

	UpdateServerPartitionMask( );

	if ( bWasNotSolid != IsSolid() )
	{
		CheckForUntouch();
	}
#endif
}

//-----------------------------------------------------------------------------
// Sets the solid flags
//-----------------------------------------------------------------------------
void CCollisionProperty::SetSolidFlags( int flags )
{
	int oldFlags = m_usSolidFlags;
	m_usSolidFlags = (unsigned short)(flags & 0xFFFF);
	if ( oldFlags == m_usSolidFlags )
		return;

	// These two flags, if changed, can produce different surrounding bounds
	if ( (oldFlags & (FSOLID_FORCE_WORLD_ALIGNED | FSOLID_USE_TRIGGER_BOUNDS)) != 
		 (m_usSolidFlags & (FSOLID_FORCE_WORLD_ALIGNED | FSOLID_USE_TRIGGER_BOUNDS)) )
	{
		MarkSurroundingBoundsDirty();
	}

	if ( (oldFlags & (FSOLID_NOT_SOLID|FSOLID_TRIGGER)) != (m_usSolidFlags & (FSOLID_NOT_SOLID|FSOLID_TRIGGER)) )
	{
		m_pOuter->CollisionRulesChanged();
	}

#ifndef CLIENT_DLL
	if ( (oldFlags & (FSOLID_NOT_SOLID | FSOLID_TRIGGER)) != (m_usSolidFlags & (FSOLID_NOT_SOLID | FSOLID_TRIGGER)) )
	{
		UpdateServerPartitionMask( );
		CheckForUntouch();
	}
#endif
}

//-----------------------------------------------------------------------------
// Check for untouch
//-----------------------------------------------------------------------------
void CCollisionProperty::CheckForUntouch()
{
#ifndef CLIENT_DLL
	if ( !IsSolid() && !IsSolidFlagSet(FSOLID_TRIGGER))
	{
		// If this ent's touch list isn't empty, it's transitioning to not solid
		if ( m_pOuter->IsCurrentlyTouching() )
		{
			// mark ent so that at the end of frame it will check to 
			// see if it's no longer touching ents
			m_pOuter->SetCheckUntouch( true );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Updates the spatial partition
//-----------------------------------------------------------------------------
void CCollisionProperty::UpdateServerPartitionMask( )
{
#ifndef CLIENT_DLL
	SpatialPartitionHandle_t handle = GetPartitionHandle();
	if ( handle == PARTITION_INVALID_HANDLE )
		return;

	// Remove it from whatever lists it may be in at the moment
	// We'll re-add it below if we need to.
	partition->Remove( handle );

	// Don't bother with deleted things
	if ( !m_pOuter->edict() )
		return;

	// don't add the world
	if ( m_pOuter->entindex() == 0 )
		return;		

	// Make sure it's in the list of all entities
	bool bIsSolid = IsSolid() || IsSolidFlagSet(FSOLID_TRIGGER);
	if ( bIsSolid || m_pOuter->IsEFlagSet(EFL_USE_PARTITION_WHEN_NOT_SOLID) )
	{
		partition->Insert( PARTITION_ENGINE_NON_STATIC_EDICTS, handle );
	}

	if ( !bIsSolid )
		return;

	// Insert it into the appropriate lists.
	// We have to continually reinsert it because its solid type may have changed
	SpatialPartitionListMask_t mask = 0;
	if ( !IsSolidFlagSet(FSOLID_NOT_SOLID) )
	{
		mask |=	PARTITION_ENGINE_SOLID_EDICTS;
	}
	if ( IsSolidFlagSet(FSOLID_TRIGGER) )
	{
		mask |=	PARTITION_ENGINE_TRIGGER_EDICTS;
	}
	Assert( mask != 0 );
	partition->Insert( mask, handle );
#endif
}