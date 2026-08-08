// By hugojobe.

#include "GAS/Abilities/ChessAbility_Swap.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"

UChessAbility_Swap::UChessAbility_Swap()
{
	AbilityType = EChessAbilityType::Swap;
	Targeting = EChessAbilityTargeting::AllyUnit;
	CooldownTurns = 3;
}

TArray<FIntPoint> UChessAbility_Swap::GetValidTargets(const AChessUnitBase* Unit) const
{
	TArray<FIntPoint> Result;
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return Result;

	static FIntPoint Neighbors[] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);

	for (const FIntPoint& Dir : Neighbors)
	{
		const FIntPoint Pos = Origin + Dir;
		
		if (AChessUnitBase* Other = Grid->GetUnitAt(Pos))
		{
			if (Other->Team == Unit->Team && Other->Attributes->GetHealth() > 0.f)
            {
            	Result.Add(Pos);
            }
		}
		
	}
	return Result;
}

void UChessAbility_Swap::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return;

	AChessUnitBase* Other = Grid->GetUnitAt(TargetPos);
	if (!Other) return;
	
	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);
	
	Grid->TeleportUnitTo(Other, Origin);
	Grid->TeleportUnitTo(Unit, TargetPos);
	
	OnAbilityAffectedUnitFeedback(AbilityType, Unit, Other);
	OnAbilityAffectedUnitFeedback(AbilityType, Unit, Unit);
}
