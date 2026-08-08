// By hugojobe.

#include "GAS/Abilities/ChessAbility_Translation.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"

UChessAbility_Translation::UChessAbility_Translation()
{
	AbilityType = EChessAbilityType::Translation;
	Targeting = EChessAbilityTargeting::Tile;
	CooldownTurns = 3;
}

TArray<FIntPoint> UChessAbility_Translation::GetValidTargets(const AChessUnitBase* Unit) const
{
	TArray<FIntPoint> Result;
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return Result;

	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);
	static const FIntPoint Orthogonals[] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

	for (const FIntPoint& Dir : Orthogonals)
	{
		const FIntPoint Pos = Origin + Dir;
		if (Grid->IsValidGridPos(Pos) && !Grid->GetUnitAt(Pos) && !Grid->IsTileBlockedAt(Pos))
		{
			Result.Add(Pos);
		}
	}
	return Result;
}

void UChessAbility_Translation::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	if (AChessGridManager* Grid = GetGridManager(Unit))
	{
		Grid->TeleportUnitTo(Unit, TargetPos);
	}
}
