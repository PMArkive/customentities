#ifndef _CCUSTOMANIMATINGOVERLAY_H_
#define _CCUSTOMANIMATINGOVERLAY_H_

#pragma once

#include "CCustomAnimating.h"

class CCustomAnimatingOverlay : public CCustomAnimating
{
	public:
		using CCustomAnimating::CCustomAnimating;

	protected:
		enum { MAX_OVERLAYS = CBaseAnimatingOverlay::MAX_OVERLAYS };

		DECLARE_PROP(CUtlVector<CAnimationLayer>, m_AnimOverlay);

		CUtlVector<CAnimationLayer> *const GetAnimOverlay() const noexcept;

		constexpr CBaseAnimatingOverlay *const GetBaseAnimatingOverlay() const noexcept
			{ return reinterpret_cast<CBaseAnimatingOverlay *const>(GetBaseAnimating()); }

		const int AddGestureSequence(const int sequence, const bool autokill=true) noexcept;
		const int AddGestureSequence(const int sequence, const float flDuration, const bool autokill=true) noexcept;
		const int AddGesture(const Activity activity, const bool autokill=true) noexcept;
		const int AddGesture(const Activity activity, const float flDuration, const bool autokill=true) noexcept;
		const bool IsPlayingGesture(Activity activity) const noexcept;
		void RestartGesture(const Activity activity, const bool addifmissing=true, const bool autokill=true) noexcept;
		void RemoveGesture(const Activity activity) noexcept;
		void RemoveAllGestures() noexcept;
		const int AddLayeredSequence(const int sequence, const int iPriority) noexcept;
		void SetLayerPriority(const int iLayer, const int iPriority) noexcept;
		const bool IsValidLayer(const int iLayer) const noexcept;
		void SetLayerDuration(const int iLayer, const float flDuration) noexcept;
		const float GetLayerDuration(const int iLayer) const noexcept;
		void SetLayerCycle(const int iLayer, const float flCycle) noexcept;
		void SetLayerCycle(const int iLayer, const float flCycle, const float flPrevCycle) noexcept;
		const float GetLayerCycle(const int iLayer) const noexcept;
		void SetLayerPlaybackRate(const int iLayer, const float flPlaybackRate) noexcept;
		void SetLayerWeight(const int iLayer, const float flWeight) noexcept;
		const float GetLayerWeight(const int iLayer) const noexcept;
		void SetLayerBlendIn(const int iLayer, const float flBlendIn) noexcept;
		void SetLayerBlendOut(const int iLayer, const float flBlendOut) noexcept;
		void SetLayerAutokill(const int iLayer, const bool bAutokill) noexcept;
		void SetLayerLooping(const int iLayer, const bool bLooping) noexcept;
		void SetLayerNoRestore(const int iLayer, const bool bNoRestore) noexcept;
		const Activity GetLayerActivity(const int iLayer) const noexcept;
		const int GetLayerSequence(const int iLayer) const noexcept;
		const int FindGestureLayer(const Activity activity) const noexcept;
		void RemoveLayer(const int iLayer, const float flKillRate=0.2f, const float flKillDelay=0.0f) noexcept;
		void FastRemoveLayer(const int iLayer) noexcept;
		const int GetNumAnimOverlays() const noexcept;
		void SetNumAnimOverlays(const int num) noexcept;
		const bool HasActiveLayer() const noexcept;
		const int AllocateLayer(const int iPriority=0) noexcept;
		CAnimationLayer *const GetAnimLayer(const int iIndex) noexcept;
};

#endif