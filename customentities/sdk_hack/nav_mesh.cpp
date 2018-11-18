#include <nav_mesh.h>

ConVar nav_solid_props("nav_solid_props", "0", FCVAR_CHEAT, "Make props solid to nav generation/editing");

CNavArea *CNavMesh::GetNearestNavArea(const Vector &pos, const bool anyZ, const float maxDist, const bool checkLOS, const bool checkGround, const int team) const
{
	if(!m_grid.Count())
		return nullptr;

	CNavArea *close{nullptr};
	float closeDistSq{maxDist * maxDist};

	if(!checkLOS && !checkGround) {
		close = GetNavArea(pos, 120.0f);
		if(close)
			return close;
	}

	Vector source{};
	source.x = pos.x;
	source.y = pos.y;
	if(GetGroundHeight(pos, &source.z, nullptr) == false) {
		if(!checkGround)
			source.z = pos.z;
		else
			return nullptr;
	}

	source.z += HalfHumanHeight;

	static unsigned int searchMarker{static_cast<const unsigned int>(RandomInt(0, 1024 * 1024))};
	++searchMarker;
	if(searchMarker == 0)
		++searchMarker;

	const int originX{WorldToGridX(pos.x)};
	const int originY{WorldToGridY(pos.y)};

	int shiftLimit{static_cast<const int>(ceil(maxDist / m_gridCellSize))};

	for(int shift{0}; shift <= shiftLimit; ++shift) {
		for(int x{originX - shift}; x <= originX + shift; ++x) {
			if(x < 0 || x >= m_gridSizeX)
				continue;

			for(int y{originY - shift}; y <= originY + shift; ++y) {
				if(y < 0 || y >= m_gridSizeY)
					continue;

				if(x > originX - shift &&
					x < originX + shift &&
					y > originY - shift &&
					y < originY + shift)
					continue;

				NavAreaVector *const areaVector{&m_grid.Element(x + y * m_gridSizeX)};

				for(int it{0}; it < areaVector->Count(); it++)
				{
					CNavArea *const area{areaVector->Element(it)};

					if(area->m_nearNavSearchMarker == searchMarker)
						continue;

					if(area->IsBlocked(team, false))
						continue;

					area->m_nearNavSearchMarker = searchMarker;

					Vector areaPos{};
					area->GetClosestPointOnArea(source, &areaPos);

					float distSq{(areaPos - pos).LengthSqr()};

					if(distSq >= closeDistSq)
						continue;

					if(checkLOS) {
						trace_t result{};
						Vector safePos{};
						UTIL_TraceLine(pos, pos + Vector{0, 0, StepHeight}, MASK_NPCSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, &result);

						if(result.startsolid)
							safePos = result.endpos + Vector{0, 0, 1.0f};
						else
							safePos = pos;

						const float heightDelta{static_cast<const float>(fabs(areaPos.z - safePos.z))};
						if(heightDelta > StepHeight) {
							UTIL_TraceLine(areaPos + Vector{0, 0, StepHeight}, Vector{areaPos.x, areaPos.y, safePos.z}, MASK_NPCSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, &result);
							if(result.fraction != 1.0f)
								continue;
						}

						UTIL_TraceLine(safePos, Vector{areaPos.x, areaPos.y, safePos.z + StepHeight}, MASK_NPCSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, &result);
						if(result.fraction != 1.0f)
							continue;
					}

					closeDistSq = distSq;
					close = area;
					shiftLimit = shift + 1;
				}
			}
		}
	}

	return close;
}

class CTraceFilterGroundEntities final : public CTraceFilterWalkableEntities
{
	public:
		using CTraceFilterWalkableEntities::CTraceFilterWalkableEntities;

	public:
		virtual bool ShouldHitEntity(IHandleEntity *const pServerEntity, const int contentsMask) override
		{
			CBaseEntity *const pEntity{EntityFromEntityHandle(pServerEntity)};
			if(FClassnameIs(pEntity, "prop_door") ||
				FClassnameIs(pEntity, "prop_door_rotating") ||
				FClassnameIs(pEntity, "func_breakable")) {
				return false;
			}

			return __super::ShouldHitEntity(pServerEntity, contentsMask);
		}
};

bool CNavMesh::GetGroundHeight(const Vector &pos, float *const height, Vector *const normal) const
{
	constexpr const float flMaxOffset{100.0f};

	CTraceFilterGroundEntities filter{nullptr, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING};

	trace_t result{};
	Vector to{pos.x, pos.y, pos.z - 10000.0f};
	Vector from{pos.x, pos.y, pos.z + static_cast<const float>(HalfHumanHeight + 1e-3f)};
	while(to.z - pos.z < flMaxOffset) {
		UTIL_TraceLine(from, to, MASK_NPCSOLID_BRUSHONLY, &filter, &result);
		if(!result.startsolid && ((result.fraction == 1.0f) || ((from.z - result.endpos.z) >= HalfHumanHeight))) {
			*height = result.endpos.z;
			if(normal)
				*normal = !result.plane.normal.IsZero(0.01f) ? result.plane.normal : Vector{0.0f, 0.0f, 1.0f};
			return true;
		}
		to.z = (result.startsolid ? from.z : result.endpos.z);
		from.z = to.z + HalfHumanHeight + 1e-3f;
	}

	*height = 0.0f;
	if(normal)
		normal->Init(0.0f, 0.0f, 1.0f);
	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Given a position, return the nav area that IsOverlapping and is *immediately* beneath it
 */
CNavArea *CNavMesh::GetNavArea( const Vector &pos, float beneathLimit ) const
{
	if ( !m_grid.Count() )
		return NULL;

	// get list in cell that contains position
	int x = WorldToGridX( pos.x );
	int y = WorldToGridY( pos.y );
	NavAreaVector *areaVector = &m_grid[ x + y*m_gridSizeX ];

	// search cell list to find correct area
	CNavArea *use = NULL;
	float useZ = -99999999.9f;
	Vector testPos = pos + Vector( 0, 0, 5 );

	FOR_EACH_VEC( (*areaVector), it )
	{
		CNavArea *area = (*areaVector)[ it ];

		// check if position is within 2D boundaries of this area
		if (area->IsOverlapping( testPos ))
		{
			// project position onto area to get Z
			float z = area->GetZ( testPos );

			// if area is above us, skip it
			if (z > testPos.z)
				continue;

			// if area is too far below us, skip it
			if (z < pos.z - beneathLimit)
				continue;

			// if area is higher than the one we have, use this instead
			if (z > useZ)
			{
				use = area;
				useZ = z;
			}
		}
	}

	return use;
}