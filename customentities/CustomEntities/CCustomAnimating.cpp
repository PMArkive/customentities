#include "CCustomAnimating.h"
#include <bone_setup.h>
#include <animation.h>
#include <activitylist.h>

void CCustomAnimating::InitBoneControllers() noexcept
{
	const CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr)
		return;

	int nBoneControllerCount{pStudioHdr->numbonecontrollers()};
	if(nBoneControllerCount > NUM_BONECTRLS)
		nBoneControllerCount = NUM_BONECTRLS;

	for(int i{0}; i < nBoneControllerCount; i++)
		SetBoneController(i, 0.0f);

	if(pStudioHdr->SequencesAvailable()) {
		for(int i{0}; i < pStudioHdr->GetNumPoseParameters(); i++)
			SetPoseParameter(i, 0.0f);
	}
}

const float CCustomAnimating::SetBoneController(const int iController, const float flValue) noexcept
{
	const CStudioHdr *const pmodel{GetModelPtr()};

	float newValue{0.0f};
	const float32 retVal{Studio_SetController(pmodel, iController, flValue, newValue)};
	m_flEncodedController[iController] = newValue;

	return retVal;
}

void CCustomAnimating::LockStudioHdr() noexcept
{
	static void *addr{nullptr};
	if(!addr) {
		IGameConfig *const gameconf{CGameConfsManager::Instance().Load("CCustomAnimating")};

		gameconf->GetMemSig("LockStudioHdr", &addr);
	}

	using Func_t = void (CBaseEntity::*)();
	(GetBaseAnimating()->*MemUtils::any_cast<Func_t>(addr))();
}

CStudioHdr *const CCustomAnimating::GetModelPtr() const noexcept
{
	static int offset{-1};
	if(offset == -1)
		offset = GetEntPropOffset(entity(), "m_hLightingOrigin") + 60;

	CStudioHdr *const pStudioHdr{MemUtils::GetPtrVarAtOffset<CStudioHdr *>(entity(), offset)};

	const model_t *const mdl{GetModel()};
	if(!pStudioHdr && mdl)
		const_cast<CCustomAnimating *>(this)->LockStudioHdr();

	return pStudioHdr;
}

const float CCustomAnimating::SequenceDuration(const int iSequence) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return 0.1f;

	if(iSequence < 0 || iSequence > pStudioHdr->GetNumSeq())
		return 0.1f;

	return Studio_Duration(pStudioHdr, iSequence, GetPoseParameterArray());
}

const bool CCustomAnimating::IsSequenceFinished() const noexcept
{
	return m_bSequenceFinished;
}

const int CCustomAnimating::GetNumBodyGroups() const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return -1;

	return ::GetNumBodyGroups(pStudioHdr);
}

void CCustomAnimating::SetBodygroup(const int iGroup, const int iValue) noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return;

	int newBody{m_nBody};
	::SetBodygroup(pStudioHdr, newBody, iGroup, iValue);
	m_nBody = newBody;
}

const int CCustomAnimating::FindBodyGroupVariant(const int group, const int variant) const noexcept
{
	int numVariants{0};
	const int count{GetBodygroupCount(group)};
	for(int i{0}; i < count; i++) {
		const char *const partName{GetBodygroupPartName(group, i)};
		const char c{*partName};
		int val{-1};
		if(c != '\0')
			val = atoi(partName + 1);
		if(val != -1 && c != 'D')
			numVariants++;
		if(variant == numVariants)
			return i;
	}
	return -1;
}

const char *const CCustomAnimating::GetBodygroupPartName(const int iGroup, const int iPart) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return nullptr;

	return nullptr;// ::GetBodygroupPartName(pStudioHdr, iGroup, iPart);
}

const int CCustomAnimating::GetBodygroupCount(const int iGroup) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return -1;

	return ::GetBodygroupCount(pStudioHdr, iGroup);
}

const int CCustomAnimating::CountBodyGroupVariants(const int group) const noexcept
{
	int numVariants{0};
	const int count{GetBodygroupCount(group)};
	for(int i = 0; i < count; i++) {
		const char *const partName{GetBodygroupPartName(group, i)};
		const char c{*partName};
		int val{-1};
		if(c != '\0')
			val = atoi(partName + 1);
		if(val != -1 && c != 'D')
			numVariants++;
	}
	return numVariants;
}

const int CCustomAnimating::FindBodygroupByName(const char *const name) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return -1;

	return ::FindBodygroupByName(pStudioHdr, name);
}

void CCustomAnimating::RandomizeBodygroups(CUtlVector<const char *> &groups) noexcept
{
	CUtlVector<int> groupIndex{};
	int numVariants{10000};
	for(int i{0}; i < groups.Count(); i++) {
		const char *const name{groups.Element(i)};
		const int index{FindBodygroupByName(name)};
		if(index < 0)
			continue;
		groupIndex.AddToTail(index);
		numVariants = MIN(numVariants, CountBodyGroupVariants(index));
	}
	if(numVariants <= 0)
		return;
	const int variant{RandomInt(1, numVariants)};
	for(int i = 0; i < groupIndex.Count(); i++) {
		int index{groupIndex.Element(i)};
		int partIndex{FindBodyGroupVariant(index, variant)};
		if(partIndex >= 0)
			SetBodygroup(index, partIndex);
	}
}

const float *const CCustomAnimating::GetPoseParameterArray() const noexcept
{
	return &m_flPoseParameter[0];
}

/*
bool CBaseAnimating::GetAttachment(int iAttachment, matrix3x4_t &attachmentToWorld)
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if(!pStudioHdr || !pStudioHdr->IsValid()) {
		MatrixCopy(EntityToWorldTransform(), attachmentToWorld);
		return false;
	}

	if(iAttachment < 1 || iAttachment > pStudioHdr->GetNumAttachments()) {
		MatrixCopy(EntityToWorldTransform(), attachmentToWorld);
		return false;
	}

	const mstudioattachment_t &pattachment = pStudioHdr->pAttachment(iAttachment-1);
	int iBone = pStudioHdr->GetAttachmentBone(iAttachment-1);

	matrix3x4_t bonetoworld;
	GetBoneTransform(iBone, bonetoworld);
	if((pattachment.flags & ATTACHMENT_FLAG_WORLD_ALIGN) == 0)
		ConcatTransforms( bonetoworld, pattachment.local, attachmentToWorld);
	else {
		Vector vecLocalBonePos, vecWorldBonePos;
		MatrixGetColumn(pattachment.local, 3, vecLocalBonePos);
		VectorTransform(vecLocalBonePos, bonetoworld, vecWorldBonePos);
		SetIdentityMatrix(attachmentToWorld);
		MatrixSetColumn(vecWorldBonePos, 3, attachmentToWorld);
	}

	return true;
}

bool CCustomAnimating::GetAttachment(int index, Vector *origin, QAngle *ang)
{
	matrix3x4_t attachmentToWorld;
	bool bRet = GetAttachment(index, attachmentToWorld);

	QAngle vecangles;
	Vector vecorigin;
	MatrixAngles(attachmentToWorld, vecangles, vecorigin);

	if(origin)
		*origin = vecorigin;

	if(ang)
		*ang = vecangles;
	
	return bRet;
}

bool CCustomAnimating::GetAttachmentLocal(int index, Vector *origin, QAngle *ang)
{
	matrix3x4_t attachmentToWorld;
	bool bRet = GetAttachment(index, attachmentToWorld);

	matrix3x4_t attachmentToLocal;
	matrix3x4_t worldToEntity;
	MatrixInvert(EntityToWorldTransform(), worldToEntity);
	ConcatTransforms(worldToEntity, attachmentToWorld, attachmentToLocal);

	QAngle vecangles;
	Vector vecorigin;
	MatrixAngles(attachmentToLocal, vecangles, vecorigin);

	if(origin)
		*origin = vecorigin;

	if(ang)
		*ang = vecangles;
	
	return bRet;
}
*/

const int CCustomAnimating::LookupPoseParameter(const char *const szName) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return -1;

	for(int i{0}; i < pStudioHdr->GetNumPoseParameters(); i++) {
		if(_stricmp(pStudioHdr->pPoseParameter(i).pszName(), szName) == 0)
			return i;
	}

	return -1;
}

const float CCustomAnimating::GetPoseParameter(const int iParameter) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return 0.0f;

	if(iParameter < 0 || iParameter > pStudioHdr->GetNumPoseParameters())
		return 0.0f;

	return Studio_GetPoseParameter(pStudioHdr, iParameter, m_flPoseParameter[iParameter]);
}

const float CCustomAnimating::SetPoseParameter(const int iParameter, const float flValue) noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return 0.0f;

	if(iParameter < 0 || iParameter > pStudioHdr->GetNumPoseParameters())
		return 0.0f;

	float flNewValue{0.0f};
	float flEncodedValue{Studio_SetPoseParameter(pStudioHdr, iParameter, flValue, flNewValue)};
	m_flPoseParameter[iParameter] = flNewValue;

	return flEncodedValue;
}

const float CCustomAnimating::GetSequenceMoveDist(const int iSequence) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return 0.0f;

	Vector vecReturn{};
	::GetSequenceLinearMotion(pStudioHdr, iSequence, GetPoseParameterArray(), &vecReturn);
	return vecReturn.Length();
}

const float CCustomAnimating::GetSequenceGroundSpeed(const int iSequence) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return 0.0f;

	const float t{SequenceDuration(iSequence)};
	if(t > 0.0f)
		return GetSequenceMoveDist(iSequence) / t;
	
	return 0.0f;
}

const Activity CCustomAnimating::GetSequenceActivity(const int iSequence) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return ACT_INVALID;

	if(iSequence < 0 || iSequence > pStudioHdr->GetNumSeq())
		return ACT_INVALID;

	return static_cast<const Activity>(::GetSequenceActivity(pStudioHdr, iSequence));
}

const int CCustomAnimating::SelectWeightedSequence(const Activity activity) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return -1;

	return ::SelectWeightedSequence(pStudioHdr, activity, GetSequence());
}

const int CCustomAnimating::LookupSequence(const char *name) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return -1;

	return ::LookupSequence(pStudioHdr, name);
}

const Activity CCustomAnimating::LookupActivity(const char *const name) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return ACT_INVALID;

	Activity act{static_cast<const Activity>(::LookupActivity(pStudioHdr, name))};
	if(act == ACT_INVALID)
		act = static_cast<const Activity>(ActivityList_IndexForName(name));

	return act;
}

const int CCustomAnimating::LookupAttachment(const char *const name) const noexcept
{
	CStudioHdr *const pStudioHdr{GetModelPtr()};
	if(!pStudioHdr || !pStudioHdr->IsValid())
		return -1;

	return Studio_FindAttachment(pStudioHdr, name) + 1;
}

const int CCustomAnimating::GetSequence() const noexcept
{
	return m_nSequence;
}

void CCustomAnimating::SetSequence(const int nSequence) noexcept
{
	bool changed{(m_nSequence != nSequence)};

	m_nSequence = nSequence;

	if(changed)
		InvalidatePhysicsRecursive(ANIMATION_CHANGED);
}

void CCustomAnimating::ResetSequence(const int nSequence) noexcept
{
	if(!SequenceLoops())
		SetCycle(0);

	const bool changed{nSequence != GetSequence() ? true : false};

	SetSequence(nSequence);
	if(changed || !SequenceLoops())
		ResetSequenceInfo();
}

void CCustomAnimating::ResetSequenceInfo() noexcept
{
	if(GetSequence() == -1)
		SetSequence(0);

	CStudioHdr *const pStudioHdr{GetModelPtr()};
	m_flGroundSpeed = GetSequenceGroundSpeed(GetSequence()) * GetModelScale();
	m_bSequenceLoops = ((GetSequenceFlags(pStudioHdr, GetSequence()) & STUDIO_LOOPING) != 0);
	//m_flAnimTime = gpGlobals->time;
	m_flPlaybackRate = 1.0f;
	m_bSequenceFinished = false;
	m_flLastEventCheck = 0.0f;

	m_nNewSequenceParity = (m_nNewSequenceParity + 1) & EF_PARITY_MASK;
	m_nResetEventsParity = (m_nResetEventsParity + 1) & EF_PARITY_MASK;

	if(pStudioHdr)
		SetEventIndexForSequence(pStudioHdr->pSeqdesc(GetSequence()));
}

const float CCustomAnimating::GetAnimTimeInterval() const noexcept
{
	#define MAX_ANIMTIME_INTERVAL 0.2f

	float flInterval{0.0f};
	if(m_flAnimTime < gpGlobals->curtime)
		flInterval = clamp(gpGlobals->curtime - m_flAnimTime, 0.0f, MAX_ANIMTIME_INTERVAL);
	else
		flInterval = clamp(m_flAnimTime - m_flPrevAnimTime, 0.0f, MAX_ANIMTIME_INTERVAL);
	return flInterval;
}

const float CCustomAnimating::GetSequenceCycleRate(const int iSequence) const noexcept
{
	const float t{SequenceDuration(iSequence)};
	if(t > 0.0f)
		return 1.0f / t;
	else
		return 1.0f / 0.1f;
}

const bool CCustomAnimating::GetIntervalMovement(float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles) const noexcept
{
	CStudioHdr *const pstudiohdr{GetModelPtr()};
	if(!pstudiohdr || !pstudiohdr->SequencesAvailable())
		return false;

	const float flComputedCycleRate{GetSequenceCycleRate(GetSequence())};

	float flNextCycle{GetCycle() + flIntervalUsed * flComputedCycleRate * m_flPlaybackRate};

	if((!m_bSequenceLoops) && flNextCycle > 1.0) {
		flIntervalUsed = GetCycle() / (flComputedCycleRate * m_flPlaybackRate);
		flNextCycle = 1.0;
		bMoveSeqFinished = true;
	} else
		bMoveSeqFinished = false;

	Vector deltaPos{};
	QAngle deltaAngles{};

	if(Studio_SeqMovement(pstudiohdr, GetSequence(), GetCycle(), flNextCycle, GetPoseParameterArray(), deltaPos, deltaAngles)) {
		VectorYawRotate(deltaPos, GetLocalAngles().y, deltaPos);
		newPosition = GetLocalOrigin() + deltaPos;
		newAngles.Init();
		newAngles.y = GetLocalAngles().y + deltaAngles.y;
		return true;
	} else {
		newPosition = GetLocalOrigin();
		newAngles = GetLocalAngles();
		return false;
	}
}

void CCustomAnimating::HandleAnimEvent(animevent_t *const pEvent) noexcept { HOOK_CALL(HandleAnimEvent, pEvent); }
void CCustomAnimating::DispatchAnimEvents(CBaseAnimating *const eventHandler) noexcept { HOOK_CALL(DispatchAnimEvents, eventHandler); }
void CCustomAnimating::StudioFrameAdvance() noexcept { HOOK_CALL(StudioFrameAdvance); }
void CCustomAnimating::Ignite(const float32 flFlameLifetime, const bool bNPCOnly, const float32 flSize, const bool bCalledByLevelDesigner) noexcept
	{ HOOK_CALL(Ignite, flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner); }