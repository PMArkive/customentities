#include "CCustomCombatCharacter.h"

const int CCustomCombatCharacter::OnTakeDamage_Alive(const CTakeDamageInfo &info) noexcept { return HOOK_CALL(OnTakeDamage_Alive, info); }
const int CCustomCombatCharacter::OnTakeDamage_Dying(const CTakeDamageInfo &info) noexcept { return HOOK_CALL(OnTakeDamage_Dying, info); }
const bool CCustomCombatCharacter::IsAreaTraversable(const CNavArea *const area) const noexcept { return HOOK_CALL(IsAreaTraversable, area); }
const bool CCustomCombatCharacter::BecomeRagdoll(const CTakeDamageInfo &info, const Vector &forceVector) noexcept { return HOOK_CALL(BecomeRagdoll, info, forceVector); }
void CCustomCombatCharacter::UpdateLastKnownArea() noexcept { HOOK_CALL(UpdateLastKnownArea); }
//void CCustomCombatCharacter::OnNavAreaChanged(CNavArea *enteredArea, CNavArea *leftArea) noexcept { HOOK_CALL(OnNavAreaChanged, enteredArea, leftArea); }