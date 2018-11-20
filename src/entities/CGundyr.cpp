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

class SelectNthAreaFunctor final
{
	public:
		SelectNthAreaFunctor(const int count) noexcept
			: m_count{count}
		{}

		const bool operator()(CNavArea *const area) noexcept
		{
			m_area = area;
			return (m_count-- > 0);
		}

	public:
		int m_count{0};
		CNavArea *m_area{nullptr};
};

class CGundyrBehavior final : public Action<CGundyr>
{
	public:
		virtual const char *GetName() const noexcept override final { return "TestAction"; }

		virtual ActionResult<CGundyr> OnStart(CGundyr *const me, Action<CGundyr> *const priorAction) noexcept override final
		{
			_cost.SetNextBot(me);
			_path.SetMinLookAheadDistance(300.0f);
			return Continue();
		}

		virtual ActionResult<CGundyr> Update(CGundyr *const me, const float interval) noexcept override final
		{
			me->GetBodyInterface()->StartActivity(ACT_WALK);

			if(_path.IsValid() && !_timer.IsElapsed())
				_path.Update(me);
			else {
				SelectNthAreaFunctor pick{RandomInt(0, TheNavMesh->GetNavAreaCount()-1)};
				TheNavMesh->ForAllAreas(pick);

				if(pick.m_area)
					_path.Compute(me, pick.m_area->GetCenter(), _cost);

				_timer.Start(RandomFloat(5.0f, 10.0f));
			}

			return Continue();
		}

		virtual EventDesiredResult<CGundyr> OnStuck(CGundyr *const me) noexcept override final
		{
			_path.Invalidate();
			return TryContinue();
		}

	private:
		PathFollower _path{};
		CountdownTimer _timer{};
		CBaseNextBotPathCost _cost{};
};

IMPLEMENT_INTENTION_INTERFACE(CGundyr, CGundyrBehavior)