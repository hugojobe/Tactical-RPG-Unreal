// By hugojobe.

#include "GAS/Abilities/ChessAbility_Caltrop.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"

UChessAbility_Caltrop::UChessAbility_Caltrop()
{
	AbilityType = EChessAbilityType::Caltrop;
	Targeting = EChessAbilityTargeting::Tile;
	CooldownTurns = 3;
}

TArray<FIntPoint> UChessAbility_Caltrop::GetValidTargets(const AChessUnitBase* Unit) const
{
	TArray<FIntPoint> Result;
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return Result;

	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);
	const int32 Dir = (Unit->Team == EChessTeam::Player) ? 1 : -1;

	const FIntPoint ForwardDiagonals[] = {
		FIntPoint(Origin.X - 1, Origin.Y + Dir),
		FIntPoint(Origin.X + 1, Origin.Y + Dir)
	};

	for (const FIntPoint& Pos : ForwardDiagonals)
	{
		if (Grid->IsValidGridPos(Pos) && !Grid->GetUnitAt(Pos) && !Grid->IsTileBlockedAt(Pos))
		{
			Result.Add(Pos);
		}
	}
	return Result;
}

void UChessAbility_Caltrop::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	if (AChessGridManager* Grid = GetGridManager(Unit))
	{
		Grid->BlockTile(TargetPos, BlockTurns);
	}
}
