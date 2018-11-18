#include "CCustomBaseEntity.h"

extern ConVar sv_teststepsimulation;

CCustomBaseEntity::EntityList_t &CCustomBaseEntity::EntityList() noexcept {
	static EntityList_t _list{};
	return _list;
}

CCustomBaseEntity *const CCustomBaseEntity::Instance(const int index) noexcept {
	const EntityList_t &list{EntityList()};
	for(CCustomBaseEntity *const it : list) {
		if(it->entindex() == index)
			return it;
	}
	return nullptr;
}

ILateInit::ILateInit(CCustomBaseEntity *const owner) noexcept
	: _owner{owner}
{ _owner->_initlist.push_front(this); }

ILateInit::~ILateInit() noexcept
{ _owner->_initlist.remove(this); }

/*
CCustomBaseEntity::CCustomBaseEntity(const char *const classname, const bool spawn) noexcept
{
	CBaseEntity *const entity{servertools->CreateEntityByName(classname)};

	Attach(entity);

	_delentity = true;

	if(spawn)
		servertools->DispatchSpawn(entity);
}
*/

void CCustomBaseEntity::Attach(CBaseEntity *const entity) noexcept
{
	_entity = entity;

	for(ILateInit *const it : _initlist)
		it->Initialize();

	EntityList().push_front(this);
}

void CCustomBaseEntity::DeAttach() noexcept
{
	for(ILateInit *const it : _initlist)
		it->Shutdown();

	if(_delentity) {
		_delentity = false;
		servertools->RemoveEntity(_entity);
	}

	//_entity = nullptr;

	EntityList().remove(this);
}

CHookInfo::DetouredList_t &CHookInfo::Detoured() noexcept {
	static DetouredList_t _detoured{};
	return _detoured;
}

void CHookInfo::Initialize() noexcept
{
	DetouredList_t &list{Detoured()};
	for(DetouredList_t::value_type &it : list) {
		if(_stricmp(it._name, _name) == 0) {
			it._ref++;
			_src = it._src;
			return;
		}
	}

	CCustomBaseEntity *const custom{_owner};
	CBaseEntity *const entity{custom->entity()};

	IGameConfig *const gameconf{CGameConfsManager::Instance().Load(_classname)};

	int index{0};
	if(gameconf->GetOffset(_name, &index))
		_src = MemUtils::GetVFunc(entity, index);
	else
		gameconf->GetMemSig(_name, const_cast<void **const>(&_src));

	if(!_src) {
		smutils->LogError(myself, "[%s] Invalid Hook %s\n", _classname, _name);
		return;
	}

	if(index != -1) {
		_detour = new CDetour{};
		if(!_detour->Hook(entity, index, custom, _dst, _args, _post)) {
			smutils->LogError(myself, "[%s] Failed to Hook %s\n", _classname, _name);
			delete _detour;
			_detour = nullptr;
			return;
		}
	} else if(_src) {
		asmjit::CodeHolder holder{};
		holder.init(CDetour::JitRuntime().getCodeInfo());

		asmjit::X86Assembler assem{&holder};
		assem.push(asmjit::x86::ecx);
		using Instance_t = CCustomBaseEntity *const (*const)(CBaseEntity *const);
		static constexpr const Instance_t instance{static_cast<const Instance_t>(CCustomBaseEntity::Instance)};
		assem.call(asmjit::imm_ptr(instance));
		assem.add(asmjit::x86::esp, 4);
		assem.lea(asmjit::x86::ecx, asmjit::x86::ptr(asmjit::x86::eax));
		assem.jmp(asmjit::imm_ptr(_dst));

		void *_newdst{nullptr};
		CDetour::JitRuntime().add(&_newdst, &holder);

		_detour = new CDetour{};
		if(!_detour->Detour(&_src, nullptr, _newdst, _src)) {
			smutils->LogError(myself, "[%s] Failed to Hook %s\n", _classname, _name);
			delete _detour;
			_detour = nullptr;
			CDetour::JitRuntime().release(_newdst);
			return;
		} else {
			DetInfo_t info{};
			info._name = _name;
			info._detour = _detour;
			info._src = _src;
			info._ref = 1;
			info._info = this;
			info._dst = _newdst;
			Detoured().push_front(std::move(info));
		}
	}
}

void CHookInfo::Shutdown() noexcept
{
	DetouredList_t &list{Detoured()};
	for(DetouredList_t::value_type &it : list) {
		if(_stricmp(it._name, _name) == 0) {
			it._ref--;
			if(it._ref == 0) {
				delete it._detour;
				CDetour::JitRuntime().release(it._dst);
				Detoured().remove(it);
			}
			return;
		}
	}

	if(_detour)
		delete _detour;
	_detour = nullptr;
}

void CCustomBaseEntity::RemoveAll() noexcept
{
	EntityList_t &list{EntityList()};
	EntityList_t::iterator it{list.begin()};
	while(it != list.end()) {
		CCustomBaseEntity *const custom{*it};
		it++;
		custom->Delete();
	}
}

void CCustomBaseEntity::RemoveAll(const char *const classname) noexcept
{
	EntityList_t &list{EntityList()};
	EntityList_t::iterator it{list.begin()};
	while(it != list.end()) {
		CCustomBaseEntity *const custom{*it};
		it++;
		if(_stricmp(classname, custom->GetClassname()) == 0)
			custom->Delete();
	}
}

void CCustomBaseEntity::Delete() noexcept
{
	if(_delcustom)
		delete this;
}

const int GetEntPropOffset(CBaseEntity *const entity, const char *const name, const int element, bool *const netprop) noexcept
{
	if(!entity)
		return -1;

	int offset{-1};

	IServerEntity *const server{reinterpret_cast<IServerEntity *const>(entity)};
	ServerClass *const svcalss{server->GetNetworkable()->GetServerClass()};

	sm_sendprop_info_t snd_info{};
	if(gamehelpers->FindSendPropInfo(svcalss->GetName(), name, &snd_info)) {
		offset = snd_info.actual_offset;

		if(element != -1) {
			SendTable *const table{snd_info.prop->GetDataTable()};
			if(table) {
				const SendProp *const prop{table->GetProp(element)};
				offset += prop->GetOffset();
			}
		}

		if(netprop)
			*netprop = true;
	} else {
		datamap_t *const map{gamehelpers->GetDataMap(entity)};
		sm_datatable_info_t dt_info{};
		if(gamehelpers->FindDataMapInfo(map, name, &dt_info)) {
			offset = dt_info.actual_offset;

			if(element != -1) {
				const datamap_t *const table{dt_info.prop->td};
				if(table) {
					const typedescription_t *const prop{&table->dataDesc[element]};
					offset += prop->fieldOffset[TD_OFFSET_NORMAL];
				}
			}
		}
	}

	return offset;
}

void CCustomBaseEntity::Precache() noexcept { HOOK_CALL(Precache); }
void CCustomBaseEntity::Spawn() noexcept { HOOK_CALL(Spawn); }
void CCustomBaseEntity::SetModel(const char8 *const str) noexcept { HOOK_CALL(SetModel, str); }
void CCustomBaseEntity::Think() noexcept { HOOK_CALL(Think); }
const bool CCustomBaseEntity::IsNPC() const noexcept { return HOOK_CALL(IsNPC); }
INextBot *const CCustomBaseEntity::MyNextBotPointer() const noexcept { return HOOK_CALL(MyNextBotPointer); }
CBaseCombatCharacter *const CCustomBaseEntity::MyCombatCharacterPointer() const noexcept { return HOOK_CALL(MyCombatCharacterPointer); }
const Class_T CCustomBaseEntity::Classify() const noexcept { return HOOK_CALL(Classify); }
const Vector CCustomBaseEntity::EyePosition() const noexcept { return HOOK_CALL(EyePosition); }
//CAI_BaseNPC *const CCustomBaseEntity::MyNPCPointer() const noexcept { return HOOK_CALL(MyNPCPointer); }
void CCustomBaseEntity::Event_Killed(const CTakeDamageInfo &info) noexcept { HOOK_CALL(Event_Killed, info); }
void CCustomBaseEntity::PerformCustomPhysics(Vector *const pNewPosition, Vector *const pNewVelocity, QAngle *const pNewAngles, QAngle *const pNewAngVelocity) noexcept
	{ HOOK_CALL(PerformCustomPhysics, pNewPosition, pNewVelocity, pNewAngles, pNewAngVelocity); }
const uint32 CCustomBaseEntity::PhysicsSolidMaskForEntity() const noexcept { return HOOK_CALL(PhysicsSolidMaskForEntity); }
void CCustomBaseEntity::Touch(CBaseEntity *const pOther) noexcept { HOOK_CALL(Touch, pOther); }
const bool CCustomBaseEntity::AcceptInput(const char8 *const szInputName, CBaseEntity *const pActivator, CBaseEntity *const pCaller, variant_t Value, const int32 outputID) noexcept
	{ return HOOK_CALL(AcceptInput, szInputName, pActivator, pCaller, Value, outputID); }
const bool CCustomBaseEntity::KeyValue(const char8 *const szKeyName, const char8 *const szValue) noexcept { return HOOK_CALL_OVERLOAD(1, KeyValue, szKeyName, szValue); }
const bool CCustomBaseEntity::KeyValue(const char8 *const szKeyName, const float32 flValue) noexcept { return HOOK_CALL_OVERLOAD(2, KeyValue, szKeyName, flValue); }
//const bool CCustomBaseEntity::KeyValue(const char8 *const szKeyName, const int32 iValue) noexcept { return HOOK_CALL_OVERLOAD(3, KeyValue, szKeyName, iValue); }
const bool CCustomBaseEntity::KeyValue(const char8 *const szKeyName, const Vector &vecValue) noexcept { return HOOK_CALL_OVERLOAD(4, KeyValue, szKeyName, vecValue); }

void CCustomBaseEntity::AddEFlags(const int nEFlagMask) noexcept
{
	m_iEFlags |= nEFlagMask;

	if(nEFlagMask & (EFL_FORCE_CHECK_TRANSMIT|EFL_IN_SKYBOX))
		DispatchUpdateTransmitState();
}

const int CCustomBaseEntity::DispatchUpdateTransmitState() noexcept
{
	const edict_t *const ed{edict()};
	if(m_nTransmitStateOwnedCounter != 0)
		return (ed ? ed->m_fStateFlags : 0);

	return entity()->UpdateTransmitState();
}

void CCustomBaseEntity::CollisionRulesChanged() noexcept
{

}

void CCustomBaseEntity::SetWaterType(const int nType) noexcept
{
	m_nWaterType = 0;
	if(nType & CONTENTS_WATER)
		m_nWaterType |= 1;
	if(nType & CONTENTS_SLIME)
		m_nWaterType |= 2;
}

void CCustomBaseEntity::UpdateWaterState() noexcept
{
	Vector point{};
	CollisionProp()->NormalizedToWorldSpace(Vector{0.5f, 0.5f, 0.0f}, &point);

	SetWaterLevel(0);
	SetWaterType(CONTENTS_EMPTY);

	const int32 cont{UTIL_PointContents(point)};
	if((cont & MASK_WATER) == 0)
		return;

	SetWaterType(cont);
	SetWaterLevel(1);

	if(IsPointSized())
		SetWaterLevel(3);
	else {
		point[2] = WorldSpaceCenter().z;
		const int32 midcont{UTIL_PointContents(point)};
		if(midcont & MASK_WATER) {
			SetWaterLevel(2);
			point[2] = EyePosition().z;
			const int32 eyecont{UTIL_PointContents(point)};
			if(eyecont & MASK_WATER)
				SetWaterLevel(3);
		}
	}
}

const Vector &CCustomBaseEntity::GetAbsOrigin() const noexcept
{
	if(IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
		const_cast<CCustomBaseEntity *const>(this)->CalcAbsolutePosition();

	return m_vecAbsOrigin;
}

void CCustomBaseEntity::SetMoveType(const MoveType_t val, const MoveCollide_t moveCollide) noexcept
{
	if(m_MoveType == val) {
		m_MoveCollide = static_cast<const unsigned char>(moveCollide);
		return;
	}

	m_MoveType = static_cast<const unsigned char>(val);
	m_MoveCollide = static_cast<const unsigned char>(moveCollide);

	CollisionRulesChanged();

	switch(m_MoveType) {
		case MOVETYPE_WALK: {
			SetSimulatedEveryTick(true);
			SetAnimatedEveryTick(true);
			break;
		} case MOVETYPE_STEP: {
			SetSimulatedEveryTick(sv_teststepsimulation.GetBool() ? true : false);
			SetAnimatedEveryTick(false);
			break;
		} case MOVETYPE_FLY: case MOVETYPE_FLYGRAVITY: {
			UpdateWaterState();
			break;
		} default: {
			SetSimulatedEveryTick(true);
			SetAnimatedEveryTick(false);
		}
	}

	CheckHasGamePhysicsSimulation();
}

const float CCustomBaseEntity::GetMoveDoneTime() const noexcept
{
	return ((m_flMoveDoneTime >= 0.0f) ? (m_flMoveDoneTime - GetLocalTime()) : -1);
}

void CCustomBaseEntity::RemoveEFlags(const int nEFlagMask) noexcept
{
	m_iEFlags &= ~nEFlagMask;

	if(nEFlagMask & (EFL_FORCE_CHECK_TRANSMIT|EFL_IN_SKYBOX))
		DispatchUpdateTransmitState();
}

const bool CCustomBaseEntity::WillSimulateGamePhysics() const noexcept
{
	if(!IsPlayer()) {
		const MoveType_t movetype{GetMoveType()};
		if(movetype == MOVETYPE_NONE || movetype == MOVETYPE_VPHYSICS)
			return false;
		if(movetype == MOVETYPE_PUSH && (GetMoveDoneTime() <= 0.0f))
			return false;
	}

	return true;
}

void CCustomBaseEntity::SetCollisionGroup(const int32 collisionGroup) noexcept
{
	if(static_cast<const int>(m_CollisionGroup) != collisionGroup) {
		m_CollisionGroup = collisionGroup;
		CollisionRulesChanged();
	}
}

void CCustomBaseEntity::CheckHasGamePhysicsSimulation() noexcept
{
	const bool isSimulating{WillSimulateGamePhysics()};
	if(isSimulating != IsEFlagSet(EFL_NO_GAME_PHYSICS_SIMULATION))
		return;

	if(isSimulating)
		RemoveEFlags(EFL_NO_GAME_PHYSICS_SIMULATION);
	else
		AddEFlags(EFL_NO_GAME_PHYSICS_SIMULATION);
}

matrix3x4_t &CCustomBaseEntity::GetParentToWorldTransform(matrix3x4_t &tempMatrix) const noexcept
{
	CBaseEntity *pMoveParent{GetMoveParent()};
	if(!pMoveParent) {
		SetIdentityMatrix(tempMatrix);
		return tempMatrix;
	}

	if(m_iParentAttachment != 0) {
		CBaseAnimating *const pAnimating{pMoveParent->GetBaseAnimating()};
		if(pAnimating && pAnimating->GetAttachment(m_iParentAttachment, tempMatrix))
			return tempMatrix;
	}

	return pMoveParent->EntityToWorldTransform();
}

void CCustomBaseEntity::CalcAbsolutePosition() noexcept
{
	if(!IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
		return;

	RemoveEFlags(EFL_DIRTY_ABSTRANSFORM);

	AngleMatrix(m_angRotation, m_vecOrigin, m_rgflCoordinateFrame);

	CBaseEntity *pMoveParent{GetMoveParent()};
	if(!pMoveParent) {
		m_vecAbsOrigin = m_vecOrigin;
		m_angAbsRotation = m_angRotation;
		return;
	}

	matrix3x4_t tmpMatrix{};
	matrix3x4_t scratchSpace{};
	ConcatTransforms(GetParentToWorldTransform(scratchSpace), m_rgflCoordinateFrame, tmpMatrix);
	MatrixCopy(tmpMatrix, m_rgflCoordinateFrame);

	MatrixGetColumn(m_rgflCoordinateFrame, 3, m_vecAbsOrigin);

	if((m_angRotation == vec3_angle) && (m_iParentAttachment == 0))
		VectorCopy(pMoveParent->GetAbsAngles(), m_angAbsRotation);
	else
		MatrixAngles(m_rgflCoordinateFrame, m_angAbsRotation);
}

void CCustomBaseEntity::SetGroundEntity(CBaseEntity *const ground) noexcept
{
	if(m_hGroundEntity.Get() == ground)
		return;

	m_hGroundEntity = ground;

	if(ground)
		AddFlag(FL_ONGROUND);
	else
		RemoveFlag(FL_ONGROUND);
}

void CCustomBaseEntity::SetNextThink(const float time, const char *const context) noexcept
{
	static void *addr{nullptr};
	if(!addr) {
		IGameConfig *const gameconf{CGameConfsManager::Instance().Load("CCustomBaseEntity")};

		gameconf->GetMemSig("SetNextThink", &addr);
	}

	using Func_t = void (CBaseEntity::*)(const float, const char *const);
	(_entity->*MemUtils::any_cast<Func_t>(addr))(time, context);
}

void CCustomBaseEntity::InvalidatePhysicsRecursive(int nChangeFlags) noexcept
{
	int nDirtyFlags{0};

	if(nChangeFlags & VELOCITY_CHANGED)
		nDirtyFlags |= EFL_DIRTY_ABSVELOCITY;

	if(nChangeFlags & POSITION_CHANGED) {
		nDirtyFlags |= EFL_DIRTY_ABSTRANSFORM;

		entity()->NetworkProp()->MarkPVSInformationDirty();

		CollisionProp()->MarkPartitionHandleDirty();
	}

	if(nChangeFlags & ANGLES_CHANGED) {
		nDirtyFlags |= EFL_DIRTY_ABSTRANSFORM;

		if(CollisionProp()->DoesRotationInvalidateSurroundingBox())
			CollisionProp()->MarkSurroundingBoundsDirty();

		nChangeFlags |= POSITION_CHANGED|VELOCITY_CHANGED;
	}

	AddEFlags(nDirtyFlags);

	bool bOnlyDueToAttachment{false};
	if(nChangeFlags & ANIMATION_CHANGED) {
		if(!(nChangeFlags & (POSITION_CHANGED|VELOCITY_CHANGED|ANGLES_CHANGED)))
			bOnlyDueToAttachment = true;
		nChangeFlags = POSITION_CHANGED|ANGLES_CHANGED|VELOCITY_CHANGED;
	}

	for(CBaseEntity *pChild{FirstMoveChild()}; pChild; pChild = pChild->NextMovePeer()) {
		if(bOnlyDueToAttachment) {
			if(pChild->GetParentAttachment() == 0)
				continue;
		}
		pChild->InvalidatePhysicsRecursive(nChangeFlags);
	}
}

const QAngle &CCustomBaseEntity::GetAbsAngles() const noexcept
{
	if(IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
		const_cast<CCustomBaseEntity *>(this)->CalcAbsolutePosition();

	return m_angAbsRotation;
}

void CCustomBaseEntity::SetLocalAngles(const QAngle &angles) noexcept
{
	if(m_angRotation != angles) {
		InvalidatePhysicsRecursive(ANGLES_CHANGED);
		m_angRotation = angles;
		SetSimulationTime(gpGlobals->curtime);
	}
}

const int CCustomBaseEntity::PrecacheModel(const char *const model) noexcept { return engine->PrecacheModel(model); }