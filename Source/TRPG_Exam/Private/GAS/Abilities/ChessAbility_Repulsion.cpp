// By hugojobe.

#include "GAS/Abilities/ChessAbility_Repulsion.h"

#include "Units/ChessUnitBase.h"
#include "Managers/ChessGridManager.h"

UChessAbility_Repulsion::UChessAbility_Repulsion()
{
	AbilityType = EChessAbilityType::Repulsion;
	Targeting = EChessAbilityTargeting::Self;
	CooldownTurns = 3;
}

void UChessAbility_Repulsion::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return;

	static const FIntPoint Neighbors[] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);

	for (const FIntPoint& Offset : Neighbors)
	{
		const FIntPoint UnitPos = Origin + Offset;
		AChessUnitBase* Pushed = Grid->GetUnitAt(UnitPos);
		if (!Pushed) continue;

		const FIntPoint Destination = UnitPos + Offset;
		if (!Grid->IsValidGridPos(Destination)) continue;
		if (Grid->GetUnitAt(Destination)) continue;
		if (Grid->IsTileBlockedAt(Destination)) continue;

		Grid->TeleportUnitTo(Pushed, Destination);
		OnAbilityAffectedUnitFeedback(AbilityType, Unit, Pushed);
	}
}