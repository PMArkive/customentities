#include "CCustomAnimatingOverlay.h"
#include <animation.h>

CUtlVector<CAnimationLayer> *const CCustomAnimatingOverlay::GetAnimOverlay() const noexcept
{
	return &const_cast<CUtlVector<CAnimationLayer> &>(m_AnimOverlay);
}

const int CCustomAnimatingOverlay::AddGestureSequence(const int sequence, const bool autokill) noexcept
{
	const int i{AddLayeredSequence(sequence, 0)};

	if(IsValidLayer(i))
		SetLayerAutokill(i, autokill);

	return i;
}

const int CCustomAnimatingOverlay::AddGestureSequence(const int nSequence, const float flDuration, const bool autokill) noexcept
{
	const int iLayer{AddGestureSequence(nSequence, autokill)};

	if(iLayer >= 0 && flDuration > 0)
		GetAnimOverlay()->Element(iLayer).m_flPlaybackRate = SequenceDuration(nSequence) / flDuration;

	return iLayer;
}

const int CCustomAnimatingOverlay::AddGesture(const Activity activity, const bool autokill) noexcept
{
	if(IsPlayingGesture(activity))
		return FindGestureLayer(activity);

	CStudioHdr *const pStudioHdr{GetModelPtr()};
	const int seq{::SelectWeightedSequence(pStudioHdr, activity, m_nSequence)};
	if(seq == -1)
		return -1;

	const int i{AddGestureSequence(seq, autokill)};
	if(i != -1)
		GetAnimOverlay()->Element(i).m_nActivity = activity;

	return i;
}

const int CCustomAnimatingOverlay::AddGesture(const Activity activity, const float flDuration, const bool autokill) noexcept
{
	const int iLayer{AddGesture(activity, autokill)};
	SetLayerDuration(iLayer, flDuration);
	return iLayer;
}

void CCustomAnimatingOverlay::SetLayerDuration(const int iLayer, const float flDuration) noexcept
{
	if(IsValidLayer(iLayer) && flDuration > 0) {
		CAnimationLayer *const layer{&GetAnimOverlay()->Element(iLayer)};
		layer->m_flPlaybackRate = SequenceDuration(layer->m_nSequence) / flDuration;
	}
}

const float CCustomAnimatingOverlay::GetLayerDuration(const int iLayer) const noexcept
{
	if(IsValidLayer(iLayer))
	{
		CAnimationLayer *const layer{&GetAnimOverlay()->Element(iLayer)};

		if(layer->m_flPlaybackRate != 0.0f)
			return (1.0f - layer->m_flCycle) * SequenceDuration(layer->m_nSequence) / layer->m_flPlaybackRate;

		return SequenceDuration(layer->m_nSequence);
	}

	return 0.0f;
}

const int CCustomAnimatingOverlay::AddLayeredSequence(const int sequence, const int iPriority) noexcept
{
	const int i{AllocateLayer(iPriority)};
	if(IsValidLayer(i))
	{
		CAnimationLayer *const layer{&GetAnimOverlay()->Element(i)};

		layer->m_flCycle = 0.0f;
		layer->m_flPrevCycle = 0.0f;
		layer->m_flPlaybackRate = 1.0f;
		layer->m_nActivity = ACT_INVALID;
		layer->m_nSequence = sequence;
		layer->m_flWeight = 1.0f;
		layer->m_flBlendIn = 0.0f;
		layer->m_flBlendOut = 0.0f;
		layer->m_bSequenceFinished = false;
		layer->m_flLastEventCheck = 0.0f;

		CStudioHdr *const pStudioHdr{GetModelPtr()};
		if(pStudioHdr && pStudioHdr->IsValid())
			layer->m_bLooping = ((GetSequenceFlags(pStudioHdr, sequence) & STUDIO_LOOPING) != 0);
	}

	return i;
}

const bool CCustomAnimatingOverlay::IsValidLayer(const int iLayer) const noexcept
{
	return (iLayer >= 0 && iLayer < GetAnimOverlay()->Count() && GetAnimOverlay()->Element(iLayer).IsActive());
}

const int CCustomAnimatingOverlay::AllocateLayer(const int iPriority) noexcept
{
	int iNewOrder{0};
	int iOpenLayer{-1};
	int iNumOpen{0};

	for(int i{0}; i < GetAnimOverlay()->Count(); i++)
	{
		CAnimationLayer *const layer{&GetAnimOverlay()->Element(i)};

		if(layer->IsActive()) {
			if(layer->m_nPriority <= iPriority)
				iNewOrder = MAX(iNewOrder, layer->m_nOrder + 1);
		}
		else if(layer->IsDying())
			continue;
		else if(iOpenLayer == -1)
			iOpenLayer = i;
		else
			iNumOpen++;
	}

	if(iOpenLayer == -1)
	{
		if(GetAnimOverlay()->Count() >= MAX_OVERLAYS)
			return -1;

		iOpenLayer = GetAnimOverlay()->AddToTail();
		GetAnimOverlay()->Element(iOpenLayer).Init(GetBaseAnimatingOverlay());
		CBaseAnimatingOverlay *pOwner = GetAnimOverlay()->Element(iOpenLayer).m_pOwnerEntity;
		edict_t *ed{nullptr};
		if(pOwner)
			ed = pOwner->GetNetworkable()->GetEdict();
		if(ed)
			ed->StateChanged(0);
	}

	if(iNumOpen == 0)
	{
		if(GetAnimOverlay()->Count() < MAX_OVERLAYS) {
			const int i{GetAnimOverlay()->AddToTail()};
			GetAnimOverlay()->Element(i).Init(GetBaseAnimatingOverlay());
			CBaseAnimatingOverlay *pOwner = GetAnimOverlay()->Element(i).m_pOwnerEntity;
			edict_t *ed = nullptr;
			if(pOwner)
				ed = pOwner->GetNetworkable()->GetEdict();
			if(ed)
				ed->StateChanged(0);
		}
	}

	for(int i{0}; i < GetAnimOverlay()->Count(); i++)
	{
		CAnimationLayer *const layer{&GetAnimOverlay()->Element(i)};

		if(layer->m_nOrder >= iNewOrder && layer->m_nOrder < MAX_OVERLAYS)
			layer->m_nOrder++;
	}

	CAnimationLayer *const layer{&GetAnimOverlay()->Element(iOpenLayer)};

	layer->m_fFlags = ANIM_LAYER_ACTIVE;
	layer->m_nOrder = iNewOrder;
	layer->m_nPriority = iPriority;
	layer->MarkActive();

	return iOpenLayer;
}

void CCustomAnimatingOverlay::SetLayerPriority(const int iLayer, const int iPriority) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *const layer{&GetAnimOverlay()->Element(iLayer)};

	if(layer->m_nPriority == iPriority)
		return;

	int i{0};
	for(i = 0; i < GetAnimOverlay()->Count(); i++)
	{
		CAnimationLayer *const l{&GetAnimOverlay()->Element(i)};

		if(l->IsActive()) {
			if(l->m_nOrder > layer->m_nOrder)
				l->m_nOrder--;
		}
	}

	int iNewOrder{0};
	for(i = 0; i < GetAnimOverlay()->Count(); i++)
	{
		CAnimationLayer *const l{&GetAnimOverlay()->Element(i)};

		if(i != iLayer && l->IsActive()) {
			if(l->m_nPriority <= iPriority)
				iNewOrder = MAX(iNewOrder, l->m_nOrder + 1);
		}
	}

	for(i = 0; i < GetAnimOverlay()->Count(); i++)
	{
		CAnimationLayer *const l{&GetAnimOverlay()->Element(i)};

		if(i != iLayer && l->IsActive()) {
			if(l->m_nOrder >= iNewOrder)
				l->m_nOrder++;
		}
	}

	layer->m_nOrder = iNewOrder;
	layer->m_nPriority = iPriority;
	layer->MarkActive();
}

const int CCustomAnimatingOverlay::FindGestureLayer(const Activity activity) const noexcept
{
	for(int i{0}; i < GetAnimOverlay()->Count(); i++)
	{
		CAnimationLayer *const layer{&GetAnimOverlay()->Element(i)};

		if(!(layer->IsActive()))
			continue;

		if(layer->IsKillMe())
			continue;

		if(layer->m_nActivity == ACT_INVALID)
			continue;

		if(layer->m_nActivity == activity)
			return i;
	}

	return -1;
}

const bool CCustomAnimatingOverlay::IsPlayingGesture(const Activity activity) const noexcept
{
	return FindGestureLayer(activity) != -1 ? true : false;
}

void CCustomAnimatingOverlay::RestartGesture(const Activity activity, const bool addifmissing, const bool autokill) noexcept
{
	const int idx{FindGestureLayer(activity)};
	if(idx == -1) {
		if(addifmissing)
			AddGesture(activity, autokill);
		return;
	}

	CAnimationLayer *const layer{&GetAnimOverlay()->Element(idx)};

	layer->m_flCycle = 0.0f;
	layer->m_flPrevCycle = 0.0f;
	layer->m_flLastEventCheck = 0.0f;
}

void CCustomAnimatingOverlay::RemoveGesture(const Activity activity) noexcept
{
	const int iLayer{FindGestureLayer(activity)};
	if(iLayer == -1)
		return;

	RemoveLayer(iLayer);
}

void CCustomAnimatingOverlay::RemoveAllGestures() noexcept
{
	for(int i{0}; i < GetAnimOverlay()->Count(); i++)
		RemoveLayer(i);
}

void CCustomAnimatingOverlay::SetLayerCycle(const int iLayer, float flCycle) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *const layer{&GetAnimOverlay()->Element(iLayer)};

	if(!layer->m_bLooping)
		flCycle = clamp(flCycle, 0.0, 1.0);

	layer->m_flCycle = flCycle;
	layer->MarkActive();
}

void CCustomAnimatingOverlay::SetLayerCycle(const int iLayer, float flCycle, float flPrevCycle) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *const layer{&GetAnimOverlay()->Element(iLayer)};

	if(!layer->m_bLooping){
		flCycle = clamp(flCycle, 0.0, 1.0);
		flPrevCycle = clamp(flPrevCycle, 0.0, 1.0);
	}

	layer->m_flCycle = flCycle;
	layer->m_flPrevCycle = flPrevCycle;
	layer->m_flLastEventCheck = flPrevCycle;
	layer->MarkActive();
}

const float CCustomAnimatingOverlay::GetLayerCycle(const int iLayer) const noexcept
{
	if(!IsValidLayer(iLayer))
		return 0.0f;

	return GetAnimOverlay()->Element(iLayer).m_flCycle;
}

void CCustomAnimatingOverlay::SetLayerPlaybackRate(const int iLayer, const float flPlaybackRate) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	GetAnimOverlay()->Element(iLayer).m_flPlaybackRate = flPlaybackRate;
}

void CCustomAnimatingOverlay::SetLayerWeight(const int iLayer, float flWeight) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	flWeight = clamp(flWeight, 0.0f, 1.0f);

	CAnimationLayer *layer = &GetAnimOverlay()->Element(iLayer);
	layer->m_flWeight = flWeight;
	layer->MarkActive();
}

const float CCustomAnimatingOverlay::GetLayerWeight(const int iLayer) const noexcept
{
	if(!IsValidLayer(iLayer))
		return 0.0f;

	return GetAnimOverlay()->Element(iLayer).m_flWeight;
}

void CCustomAnimatingOverlay::SetLayerBlendIn(const int iLayer, const float flBlendIn) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	GetAnimOverlay()->Element(iLayer).m_flBlendIn = flBlendIn;
}

void CCustomAnimatingOverlay::SetLayerBlendOut(const int iLayer, const float flBlendOut) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	GetAnimOverlay()->Element(iLayer).m_flBlendOut = flBlendOut;
}

void CCustomAnimatingOverlay::SetLayerAutokill(const int iLayer, const bool bAutokill) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *const layer{&GetAnimOverlay()->Element(iLayer)};

	if(bAutokill)
		layer->m_fFlags |= ANIM_LAYER_AUTOKILL;
	else
		layer->m_fFlags &= ~ANIM_LAYER_AUTOKILL;
}

void CCustomAnimatingOverlay::SetLayerLooping(const int iLayer, const bool bLooping) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	GetAnimOverlay()->Element(iLayer).m_bLooping = bLooping;
}

void CCustomAnimatingOverlay::SetLayerNoRestore(const int iLayer, const bool bNoRestore) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *const layer{&GetAnimOverlay()->Element(iLayer)};

	if(bNoRestore)
		layer->m_fFlags |= ANIM_LAYER_DONTRESTORE;
	else
		layer->m_fFlags &= ~ANIM_LAYER_DONTRESTORE;
}

const Activity CCustomAnimatingOverlay::GetLayerActivity(const int iLayer) const noexcept
{
	if(!IsValidLayer(iLayer))
		return ACT_INVALID;

	return GetAnimOverlay()->Element(iLayer).m_nActivity;
}

const int CCustomAnimatingOverlay::GetLayerSequence(const int iLayer) const noexcept
{
	if(!IsValidLayer(iLayer))
		return ACT_INVALID;

	return GetAnimOverlay()->Element(iLayer).m_nSequence;
}

void CCustomAnimatingOverlay::RemoveLayer(const int iLayer, const float flKillRate, const float flKillDelay) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *const layer{&GetAnimOverlay()->Element(iLayer)};

	if(flKillRate > 0)
		layer->m_flKillRate = layer->m_flWeight / flKillRate;
	else
		layer->m_flKillRate = 100;

	layer->m_flKillDelay = flKillDelay;

	layer->KillMe();
}

void CCustomAnimatingOverlay::FastRemoveLayer(const int iLayer) noexcept
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *const layer{&GetAnimOverlay()->Element(iLayer)};

	for(int j{0}; j < GetAnimOverlay()->Count(); j++)
	{
		CAnimationLayer *const l{&GetAnimOverlay()->Element(j)};

		if((l->IsActive()) && l->m_nOrder > layer->m_nOrder)
			l->m_nOrder--;
	}

	layer->Init(GetBaseAnimatingOverlay());
}

void CCustomAnimatingOverlay::SetNumAnimOverlays(const int num) noexcept
{
	if(GetAnimOverlay()->Count() < num) {
		GetAnimOverlay()->AddMultipleToTail(num - GetAnimOverlay()->Count());
		//NetworkStateChanged();
	} else if(GetAnimOverlay()->Count() > num) {
		GetAnimOverlay()->RemoveMultiple(num, GetAnimOverlay()->Count() - num);
		//NetworkStateChanged();
	}
}

const bool CCustomAnimatingOverlay::HasActiveLayer() const noexcept
{
	for(int j = 0; j < GetAnimOverlay()->Count(); j++) {
		if(GetAnimOverlay()->Element(j).IsActive())
			return true;
	}

	return false;
}

const int CCustomAnimatingOverlay::GetNumAnimOverlays() const noexcept
{
	return GetAnimOverlay()->Count();
}

CAnimationLayer *const CCustomAnimatingOverlay::GetAnimLayer(int iIndex) noexcept
{
	iIndex = clamp(iIndex, 0, GetNumAnimOverlays()-1);
	return &GetAnimOverlay()->Element(iIndex);
}