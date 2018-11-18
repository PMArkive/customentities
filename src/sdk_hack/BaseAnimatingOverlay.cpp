#include <BaseAnimatingOverlay.h>

CAnimationLayer::CAnimationLayer( )
{
	Init( NULL );
}


void CAnimationLayer::Init( CBaseAnimatingOverlay *pOverlay )
{
	m_pOwnerEntity = pOverlay;
	m_fFlags = 0;
	m_flWeight = 0;
	m_flCycle = 0;
	m_flPrevCycle = 0;
	m_bSequenceFinished = false;
	m_nActivity = ACT_INVALID;
	m_nSequence = 0;
	m_nPriority = 0;
	m_nOrder.Set( CBaseAnimatingOverlay::MAX_OVERLAYS );

	m_flBlendIn = 0.0;
	m_flBlendOut = 0.0;

	m_flKillRate = 100.0;
	m_flKillDelay = 0.0;
	m_flPlaybackRate = 1.0;
	m_flLastEventCheck = 0.0;
	m_flLastAccess = gpGlobals->curtime;
	m_flLayerAnimtime = 0;
	m_flLayerFadeOuttime = 0;
}

//------------------------------------------------------------------------------

bool CAnimationLayer::IsAbandoned( void )
{ 
	if (IsActive() && !IsAutokill() && !IsKillMe() && m_flLastAccess > 0.0 && (gpGlobals->curtime - m_flLastAccess > 0.2)) 
		return true; 
	else 
		return false;
}

void CAnimationLayer::MarkActive( void )
{ 
	m_flLastAccess = gpGlobals->curtime;
}