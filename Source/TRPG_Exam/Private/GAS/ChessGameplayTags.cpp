// By hugojobe.

#include "GAS/ChessGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Cooldown, "Ability.Cooldown", "Unit ability is recovering; one stack is removed at the start of each of the unit's turns.")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Rooted, "Ability.Rooted", "Unit cannot move while this tag is present; one stack is removed at the start of each of the unit's turns.")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Empowered, "Ability.Empowered", "Unit deals bonus damage; consumed by the next attack.")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Intimidated, "Ability.Intimidated", "Unit deals reduced damage; one stack is removed at the start of each of the unit's turns.")