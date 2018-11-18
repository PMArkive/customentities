#include <util.h>

extern void __cdecl  RemoveEntity(CBaseEntity *const) noexcept;

void UTIL_Remove(CBaseEntity *entity)
{
	RemoveEntity(entity);
}

CBasePlayer *UTIL_GetListenServerHost()
{
	return UTIL_PlayerByIndex(1);
}

CBasePlayer	*UTIL_PlayerByIndex(int playerIndex)
{
	CBasePlayer *pPlayer = NULL;

	if(playerIndex > 0 && playerIndex <= gpGlobals->maxClients) {
		edict_t *pPlayerEdict = INDEXENT(playerIndex);
		if(pPlayerEdict && !pPlayerEdict->IsFree()) {
			pPlayer = (CBasePlayer*)GetContainingEntity(pPlayerEdict);
		}
	}

	return pPlayer;
}