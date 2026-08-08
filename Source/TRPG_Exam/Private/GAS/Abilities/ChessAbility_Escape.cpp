// By hugojobe.

#include "GAS/Abilities/ChessAbility_Escape.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"

UChessAbility_Escape::UChessAbility_Escape()
{
	AbilityType = EChessAbilityType::Escape;
	Targeting = EChessAbilityTargeting::Tile;
	CooldownTurns = -1;
}

TArray<FIntPoint> UChessAbility_Escape::GetValidTargets(const AChessUnitBase* Unit) const
{
	TArray<FIntPoint> Result;
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return Result;

	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);

	for (int32 OffsetX = -JumpRadius; OffsetX <= JumpRadius; OffsetX++)
	{
		for (int32 OffsetY = -JumpRadius; OffsetY <= JumpRadius; OffsetY++)
		{
			if (OffsetX == 0 && OffsetY == 0) continue;

			const FIntPoint Pos = Origin + FIntPoint(OffsetX, OffsetY);
			if (Grid->IsValidGridPos(Pos) && !Grid->GetUnitAt(Pos) && !Grid->IsTileBlockedAt(Pos))
			{
				Result.Add(Pos);
			}
		}
	}
	return Result;
}

void UChessAbility_Escape::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	if (AChessGridManager* Grid = GetGridManager(Unit))
	{
		Grid->TeleportUnitTo(Unit, TargetPos);
	}
}
