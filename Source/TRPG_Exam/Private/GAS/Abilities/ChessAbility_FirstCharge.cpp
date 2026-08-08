// By hugojobe.

#include "GAS/Abilities/ChessAbility_FirstCharge.h"
#include "GAS/ChessAbilityBase.h"
#include "Units/ChessUnitBase.h"

UChessAbility_FirstCharge::UChessAbility_FirstCharge()
{
	AbilityType = EChessAbilityType::FirstCharge;
	bActivatable = false;
	CooldownTurns = 0;
}
