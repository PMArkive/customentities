#ifndef _CUSTOMBASEENTITY_H_
#define _CUSTOMBASEENTITY_H_

#pragma once

#include "CCustomEntityFactory.h"
#include <iserverentity.h>
#include <typeinfo>

#include <basehandle.h>
#include <ehandle.h>
#include <predictioncopy.h>
#include <takedamageinfo.h>
#include <variant_t.h>
#include <collisionproperty.h>

class __single_inheritance __declspec(novtable) INextBot;
class CBaseCombatCharacter;
class CAI_BaseNPC;
enum Class_T : int;
class variant_t;

class CCustomBaseEntity;

class __single_inheritance __declspec(novtable) ILateInit
{
	private:
		friend class CCustomBaseEntity;

	public:
		ILateInit(CCustomBaseEntity *const owner) noexcept;
		virtual ~ILateInit() noexcept;

	protected:
		virtual void Initialize() noexcept = 0;
		virtual void Shutdown() noexcept = 0;

	protected:
		CCustomBaseEntity *_owner{nullptr};
};

class CHookInfo : public ILateInit
{
	public:
		template <typename T>
		CHookInfo(T *const owner, const char *const name, const void *const dst, const CArgsBuilder &args, const bool post) noexcept
			: ILateInit(owner), _classname{typeid(T).name()+6}, _name{name}, _dst{dst}, _args{args}, _post{post}
		{}
		virtual ~CHookInfo() noexcept override { Shutdown(); }

		void Pause() noexcept {
			if(_detour)
				_detour->Pause();
		}
		void Resume() noexcept {
			if(_detour)
				_detour->Resume();
		}

		virtual void Initialize() noexcept override final;
		virtual void Shutdown() noexcept override final;

	protected:
		const void *_src{nullptr};

	private:
		const char *const _classname{nullptr};
		const char *const _name{nullptr};
		const void *const _dst{nullptr};
		CArgsBuilder _args{};
		const bool _post{false};

		CDetour *_detour{nullptr};

		struct DetInfo_t final
		{
			public:
				const char *_name{nullptr};
				CDetour *_detour{nullptr};
				const void *_src{nullptr};
				const void *_dst{nullptr};
				int _ref{0};
				CHookInfo *_info{nullptr};

				const bool operator==(const DetInfo_t &other) const noexcept
					{ return (_stricmp(_name, other._name) == 0); }

				const bool operator!=(const DetInfo_t &other) const noexcept
					{ return !operator==(other); }
		};
		using DetouredList_t = std::forward_list<DetInfo_t>;
		static DetouredList_t &Detoured() noexcept;
};

#define DECLARE_HOOK_INTERNAL(varname, clsname, configname, ret, name, attrib, _post, ...) \
	class clsname final : private CHookInfo { \
		public: \
			template <typename T> \
			clsname(T *const owner) noexcept \
				: CHookInfo(owner, configname, MemUtils::any_cast<const void *>(static_cast<ret (T::*)(__VA_ARGS__) attrib noexcept>(&T::name)), CArgsBuilder{static_cast<ret (T::*)(__VA_ARGS__) attrib noexcept>(&T::name)}, _post) \
			{} \
		public: \
			template <typename ...Args> \
			ret Call(Args... args) const noexcept \
			{ \
				clsname *const pthis{const_cast<clsname *const>(this)}; \
				CBaseEntity *const ptr{_owner->entity()}; \
				using Call_t = ret (CBaseEntity::*)(__VA_ARGS__) attrib; \
				if constexpr(std::is_void_v<ret>) { \
					pthis->Pause(); \
					(reinterpret_cast<CBaseEntity *const>(ptr)->*MemUtils::any_cast<Call_t>(_src))(args...); \
					pthis->Resume(); \
				} else { \
					pthis->Pause(); \
					ret res{(reinterpret_cast<CBaseEntity *const>(ptr)->*MemUtils::any_cast<Call_t>(_src))(args...)}; \
					pthis->Resume(); \
					return res; \
				} \
			} \
	} varname{this}; \
	virtual ret name(__VA_ARGS__) attrib noexcept;

#define DECLARE_HOOK(ret, name, attrib, _post, ...) \
	DECLARE_HOOK_INTERNAL(_##name##hook, C##name##Hook, #name, ret, name, attrib, _post, __VA_ARGS__);

#define DECLARE_HOOK_OVERLOAD(num, ret, name, attrib, _post, ...) \
	DECLARE_HOOK_INTERNAL(_##name##hook##num, C##name##Hook##num, #name "_" #num, ret, name, attrib, _post, __VA_ARGS__);

#define HOOK_CALL(name, ...) _##name##hook.Call(__VA_ARGS__)
#define HOOK_CALL_OVERLOAD(num, name, ...) _##name##hook##num.Call(__VA_ARGS__)

extern const int GetEntPropOffset(CBaseEntity *const entity, const char *const prop, const int element=-1, bool *const netprop=nullptr) noexcept;

template <typename T>
inline void SetEntProp(CBaseEntity *const entity, const char *const name, const T &value, const int element=-1) noexcept
{
	bool netprop{false};
	const int offset{GetEntPropOffset(entity, name, element, &netprop)};

	MemUtils::SetPtrVarAtOffset<T>(entity, offset, value);

	if(netprop) {
		const int index{gamehelpers->EntityToBCompatRef(entity)};
		edict_t *const edict{gamehelpers->EdictOfIndex(index)};
		gamehelpers->SetEdictStateChanged(edict, static_cast<const unsigned short>(offset));
	}
}

template <typename T>
inline T &GetEntProp(const CBaseEntity *const entity, const char *const name, const int element=-1) noexcept
{
	CBaseEntity *const nconst{const_cast<CBaseEntity *const>(entity)};

	bool netprop{false};
	const int offset{GetEntPropOffset(nconst, name, element, &netprop)};

	T &value{MemUtils::GetPtrVarAtOffset<T>(nconst, offset)};

	if(netprop) {
		/*
		const int index{gamehelpers->EntityToBCompatRef(nconst)};
		edict_t *const edict{gamehelpers->EdictOfIndex(index)};
		gamehelpers->SetEdictStateChanged(edict, static_cast<const unsigned short>(offset));
		*/
	}

	return value;
}

#define DECLARE_PROP(type, name) \
	void Set##name(const type &value) noexcept { SetProp<type>(#name, value, -1); } \
	type &Get##name() const noexcept { return GetProp<type>(#name, -1); } \
	__declspec(property(get=Get##name, put=Set##name)) type name;

#define DECLARE_PROP_ARRAY(type, name, size) \
	void Set##name(const int element, const type &value) noexcept { SetProp<type>(#name, element, value); } \
	type &Get##name(const int element) const noexcept { return GetProp<type>(#name, element); } \
	__declspec(property(get=Get##name, put=Set##name)) type name[size];

class CCustomBaseEntity
{
	private:
		friend void __cdecl RemoveEntity(CBaseEntity *const) noexcept;
		friend class __single_inheritance __declspec(novtable) ICustomEntityFactory;
		friend class __single_inheritance __declspec(novtable) ILateInit;
		friend class CHookInfo;

		using InitList_t = std::forward_list<ILateInit *>;
		InitList_t _initlist{};

	public:
		constexpr CCustomBaseEntity() noexcept {}
		//CCustomBaseEntity(CBaseEntity *const entity) noexcept { Attach(entity); }
		//CCustomBaseEntity(const char *const classname, const bool spawn=true) noexcept;

		virtual ~CCustomBaseEntity() noexcept { _delcustom = false; DeAttach(); }

	public:
		DECLARE_HOOK(void, Spawn, , false);
		DECLARE_HOOK(void, Precache, , false);
		DECLARE_HOOK(void, SetModel, , false, const char8 *const);
		DECLARE_HOOK(void, Think, , false);
		DECLARE_HOOK(const bool, IsNPC, const, false);
		DECLARE_HOOK(INextBot *const, MyNextBotPointer, const, false);
		DECLARE_HOOK(CBaseCombatCharacter *const, MyCombatCharacterPointer, const, false);
		DECLARE_HOOK(const Class_T, Classify, const, false);
		DECLARE_HOOK(const Vector, EyePosition, const, false);
		//DECLARE_HOOK(CAI_BaseNPC *const, MyNPCPointer, const, false);
		DECLARE_HOOK(void, Event_Killed, , false, const CTakeDamageInfo &);
		DECLARE_HOOK(void, PerformCustomPhysics, , false, Vector *const, Vector *const, QAngle *const, QAngle *const);
		DECLARE_HOOK(const unsigned int, PhysicsSolidMaskForEntity, const, false);
		DECLARE_HOOK(void, Touch, , false, CBaseEntity *const);
		DECLARE_HOOK(const bool, AcceptInput, , false, const char *const, CBaseEntity *const, CBaseEntity *const, variant_t, const int);
		DECLARE_HOOK_OVERLOAD(1, const bool, KeyValue, , false, const char *const, const char *const);
		DECLARE_HOOK_OVERLOAD(2, const bool, KeyValue, , false, const char *const, const float);
		//DECLARE_HOOK_OVERLOAD(3, const bool, KeyValue, , false, const char *const, const int);
		DECLARE_HOOK_OVERLOAD(4, const bool, KeyValue, , false, const char *const, const Vector &);

	protected:
		DECLARE_PROP(int, m_iMaxHealth);
		DECLARE_PROP(int, m_iInitialTeamNum);
		DECLARE_PROP(int, m_fEffects);
		DECLARE_PROP(color32, m_clrRender);
		DECLARE_PROP(unsigned char, m_nRenderMode);
		DECLARE_PROP(unsigned char, m_nRenderFX);
		DECLARE_PROP(string_t, m_iClassname);
		DECLARE_PROP(string_t, m_iName);
		DECLARE_PROP(Vector, m_vecAbsOrigin);
		DECLARE_PROP(Vector, m_vecOrigin);
		DECLARE_PROP(QAngle, m_angRotation);
		DECLARE_PROP(Vector, m_vecViewOffset);
		DECLARE_PROP(QAngle, m_angAbsRotation);
		DECLARE_PROP(Vector, m_vecAbsVelocity);
		DECLARE_PROP(Vector, m_vecVelocity);
		DECLARE_PROP(matrix3x4_t, m_rgflCoordinateFrame);
		DECLARE_PROP(int, m_iEFlags);
		DECLARE_PROP(int, m_fFlags);
		DECLARE_PROP(EHANDLE, m_hMoveParent);
		DECLARE_PROP(unsigned char, m_iParentAttachment);
		DECLARE_PROP(float, m_flSimulationTime);
		DECLARE_PROP(EHANDLE, m_hMoveChild);
		DECLARE_PROP(EHANDLE, m_hMovePeer);
		DECLARE_PROP(EHANDLE, m_hGroundEntity);
		DECLARE_PROP(int, m_iTeamNum);
		DECLARE_PROP(char, m_takedamage);
		DECLARE_PROP(int, m_iHealth);
		DECLARE_PROP(char, m_lifeState);
		DECLARE_PROP(unsigned char, m_MoveType);
		DECLARE_PROP(unsigned char, m_MoveCollide);
		DECLARE_PROP(EHANDLE, m_hOwnerEntity);
		DECLARE_PROP(int, m_CollisionGroup);
		DECLARE_PROP(bool, m_bSimulatedEveryTick);
		DECLARE_PROP(bool, m_bAnimatedEveryTick);
		DECLARE_PROP(float, m_flMoveDoneTime);
		DECLARE_PROP(float, m_flLocalTime);
		DECLARE_PROP(unsigned char, m_nWaterLevel);
		DECLARE_PROP(unsigned char, m_nWaterType);
		DECLARE_PROP(int, touchStamp);
		DECLARE_PROP(unsigned char, m_nTransmitStateOwnedCounter);
		DECLARE_PROP(short, m_nModelIndex);
		/*
		DECLARE_PROP(int, m_cellX);
		DECLARE_PROP(int, m_cellY);
		DECLARE_PROP(int, m_cellZ);
		DECLARE_PROP(int, m_cellbits);
		*/

	public:
		constexpr operator CBaseEntity *const () const noexcept { return entity(); }

		void Attach(CBaseEntity *const entity) noexcept;
		void DeAttach() noexcept;
		virtual void Delete() noexcept;

		//void DispatchSpawn() noexcept { servertools->DispatchSpawn(_entity); }

		void SetSimulatedEveryTick(const bool sim) noexcept
			{ if(m_bSimulatedEveryTick != sim) { m_bSimulatedEveryTick = sim; } }

		void SetAnimatedEveryTick(const bool anim) noexcept
			{ if(m_bAnimatedEveryTick != anim) { m_bAnimatedEveryTick = anim; } }

		CCollisionProperty *const CollisionProp() const noexcept { return _entity->CollisionProp(); }

		void SetSolid(const SolidType_t type) noexcept { CollisionProp()->SetSolid(type); }
		void AddSolidFlags(const int flags) noexcept { CollisionProp()->AddSolidFlags(flags); }
		void AddEFlags(const int nEFlagMask) noexcept;
		void SetMoveType(const MoveType_t val, const MoveCollide_t moveCollide=MOVECOLLIDE_DEFAULT) noexcept;
		void CheckHasGamePhysicsSimulation() noexcept;
		void CollisionRulesChanged() noexcept;
		const MoveType_t GetMoveType() const noexcept { return static_cast<MoveType_t>(m_MoveType); }
		const float GetMoveDoneTime() const noexcept;
		const float GetLocalTime() const noexcept { return m_flLocalTime; }
		const bool WillSimulateGamePhysics() const noexcept;
		const bool IsPlayer() const noexcept { return entity()->IsPlayer(); }
		const bool IsEFlagSet(const int flag) const noexcept { return !!(m_iEFlags & flag); }
		void RemoveEFlags(const int nEFlagMask) noexcept;
		void UpdateWaterState() noexcept;
		void SetWaterLevel(const int nLevel) noexcept { m_nWaterLevel = static_cast<const unsigned char>(nLevel); }
		void SetWaterType(const int nType) noexcept;
		const Vector &WorldSpaceCenter() const noexcept { return CollisionProp()->WorldSpaceCenter(); }
		const Vector &GetAbsOrigin() const noexcept;
		const Vector &GetViewOffset() const noexcept { return m_vecViewOffset; }
		void CalcAbsolutePosition() noexcept;
		matrix3x4_t &GetParentToWorldTransform(matrix3x4_t &tempMatrix) const noexcept;
		CBaseEntity *const GetMoveParent() const noexcept { return m_hMoveParent.Get(); }
		CBaseEntity *const FirstMoveChild() const noexcept { return m_hMoveChild.Get(); }
		CBaseEntity *const NextMovePeer() const noexcept { return m_hMovePeer.Get(); }
		CBaseEntity *const GetGroundEntity() const noexcept { return m_hGroundEntity.Get(); }
		const bool IsPointSized() const noexcept { return (CollisionProp()->BoundingRadius() == 0.0f); }
		void SetCollisionGroup(const int collisionGroup) noexcept;
		void AddFlag(const int flag) noexcept { m_fFlags |= flag; }
		void RemoveFlag(const int flag) noexcept { m_fFlags &= ~flag; }
		const bool IsFlagSet(const int flag) const noexcept { return !!(m_fFlags & flag); }
		CBaseEntity *const GetOwnerEntity() const noexcept { return m_hOwnerEntity.Get(); }
		void SetGroundEntity(CBaseEntity *const ground) noexcept;
		void SetNextThink(const float time, const char *const context=nullptr) noexcept;
		const model_t *const GetModel() const noexcept { return const_cast<model_t *const>(modelinfo->GetModel(GetModelIndex())); }
		const int GetModelIndex() const noexcept { return m_nModelIndex; }
		void InvalidatePhysicsRecursive(const int nChangeFlags) noexcept;
		const int DispatchUpdateTransmitState() noexcept;
		const int GetCollisionGroup() const noexcept { return m_CollisionGroup; }
		const QAngle &EyeAngles() const noexcept { return GetAbsAngles(); }
		const QAngle &GetAbsAngles() const noexcept;
		const Vector &GetLocalOrigin() const noexcept { return m_vecOrigin; }
		const QAngle &GetLocalAngles() const noexcept { return m_angRotation; }
		void SetLocalAngles(const QAngle &angles) noexcept;
		void SetSimulationTime(const float time) { m_flSimulationTime = time; }

		static const int PrecacheModel(const char *const model) noexcept;

		edict_t *const edict() const noexcept { return entity()->edict(); }
		constexpr CBaseEntity *const entity() const noexcept { return _entity; }
		const char *const GetClassname() const noexcept { return STRING(m_iClassname); }
		const int entindex() const noexcept { return entity()->entindex(); }

		template <typename T>
		void SetProp(const char *const name, const T &value, const int element=-1) noexcept
			{ SetEntProp<T>(_entity, name, value, element); }
		template <typename T>
		T &GetProp(const char *const name, const int element=-1) const noexcept
			{ return GetEntProp<T>(_entity, name, element); }

		static CCustomBaseEntity *const Instance(CBaseEntity *const entity) noexcept
			{ return Instance(entity->entindex()); }
		static CCustomBaseEntity *const Instance(const int index) noexcept;

		static void RemoveAll() noexcept;
		static void RemoveAll(const char *const classname) noexcept;

	private:
		CBaseEntity *_entity{nullptr};
		bool _delentity{true};
		bool _delcustom{true};
		ICustomEntityFactory *_factory{nullptr};

		using EntityList_t = std::forward_list<CCustomBaseEntity *>;
		static EntityList_t &EntityList() noexcept;
};

#endif