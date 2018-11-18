#include "CGundyr.h"
#include <NextBot/Path/NextBotChasePath.h>
#include "CBaseNextBotPathCost.h"
#include <NextBot/NextBotVisionInterface.h>

LINK_CUSTOM_ENTITY_TO_CLASS(combatcharacter, npc_gundyr, CGundyr);

CGundyr::CGundyr() noexcept
	: CCustomNextBotCombatCharacter()
{
	_body = new CBaseNextBotBody{this};
	_locomotion = new CBaseNextBotLocomotion{this};

	ALLOCATE_INTENTION_INTERFACE(CGundyr)
}

CGundyr::~CGundyr() noexcept
{
	DEALLOCATE_INTENTION_INTERFACE

	delete _locomotion;
	delete _body;
}

void CGundyr::Spawn() noexcept
{
	__super::Spawn();

	SetModel("models/gundyr.mdl");
}

void CGundyr::Precache() noexcept
{
	__super::Precache();

	PrecacheModel("models/gundyr.mdl");
}

class CTestAction final : public Action<CGundyr>
{
	public:
		virtual const char *GetName() const noexcept override final { return "TestAction"; }

		virtual ActionResult<CGundyr> OnStart(CGundyr *const me, Action<CGundyr> *const priorAction) noexcept override final
		{
			_cost.SetNextBot(me);
			return Continue();
		}

		virtual ActionResult<CGundyr> Update(CGundyr *const me, const float interval) noexcept override final
		{
			me->GetBodyInterface()->StartActivity(ACT_WALK);

			CBasePlayer *const player{UTIL_GetListenServerHost()};
			if(player)
				_path.Update(me, player, _cost, nullptr);

			return Continue();
		}

	private:
		ChasePath _path{};
		CBaseNextBotPathCost _cost{};
};

IMPLEMENT_INTENTION_INTERFACE(CGundyr, CTestAction)