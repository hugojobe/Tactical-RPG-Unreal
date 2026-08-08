// By hugojobe.

#include "GAS/Abilities/ChessAbility_DoubleLeap.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"

UChessAbility_DoubleLeap::UChessAbility_DoubleLeap()
{
	AbilityType = EChessAbilityType::DoubleLeap;
	Targeting = EChessAbilityTargeting::Tile;
	CooldownTurns = 3;
}

TArray<FIntPoint> UChessAbility_DoubleLeap::GetValidTargets(const AChessUnitBase* Unit) const
{
	AChessGridManager* Grid = GetGridManager(Unit);
	static const FIntPoint KnightOffsets[] = { {1,2}, {2,1}, {2,-1}, {1,-2}, {-1,-2}, {-2,-1}, {-2,1}, {-1,2} };
	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);
	
	TArray<FIntPoint> Result;

	for (const FIntPoint& Hop : KnightOffsets)
	{
		const FIntPoint End = Origin + Hop;
		if (Grid->IsValidGridPos(End) && !Grid->GetUnitAt(End) && !Grid->IsTileBlockedAt(End))
		{
			Result.AddUnique(End);
		}
	}
	
	return Result;
}

void UChessAbility_DoubleLeap::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	if (AChessGridManager* Grid = GetGridManager(Unit))
	{
		Grid->TeleportUnitTo(Unit, TargetPos);
	}
}


