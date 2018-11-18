#include "CCustomWorld.h"
#include <activitylist.h>
#include <eventlist.h>

//LINK_CUSTOM_ENTITY_TO_CLASS(worldspawn, worldspawn, CCustomWorld);

void CCustomWorld::Spawn() noexcept
{
	__super::Spawn();

	ActivityList_Free();
	ActivityList_Init();
	ActivityList_RegisterSharedActivities();

	EventList_Free();
	EventList_Init();
	EventList_RegisterSharedEvents();
}