#include "pch.h"

ConVar r_visualizetraces("r_visualizetraces", "0", FCVAR_CHEAT);
ConVar developer("developer", "0", 0, "Set developer message level"); // developer mode

// work-around since client header doesn't like inlined gpGlobals->curtime
float IntervalTimer::Now(void) const
{
	return gpGlobals->curtime;
}

// work-around since client header doesn't like inlined gpGlobals->curtime
float CountdownTimer::Now(void) const
{
	return gpGlobals->curtime;
}