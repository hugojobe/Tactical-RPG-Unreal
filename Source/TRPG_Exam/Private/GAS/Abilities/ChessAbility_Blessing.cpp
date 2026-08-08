// By hugojobe.

#include "GAS/Abilities/ChessAbility_Blessing.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"

UChessAbility_Blessing::UChessAbility_Blessing()
{
	AbilityType = EChessAbilityType::Blessing;
	Targeting = EChessAbilityTargeting::AllyUnit;
	CooldownTurns = 3;
}

TArray<FIntPoint> UChessAbility_Blessing::GetValidTargets(const AChessUnitBase* Unit) const
{
	TArray<FIntPoint> Result;
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return Result;
	
	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);
	const FIntPoint DiagonalPos[] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
	
	for (const FIntPoint& Dir : DiagonalPos)
	{
		FIntPoint Pos = Origin + Dir;
		while (Grid->IsValidGridPos(Pos))
		{
			if (AChessUnitBase* Other = Grid->GetUnitAt(Pos))
			{
				if (Other->Team == Unit->Team && Other->Attributes->GetHealth() < Other->Attributes->GetMaxHealth())
				{
					Result.Add(Pos);
				} 
				break;
			}
			Pos += Dir;
		}
	}
	return Result;
}

void UChessAbility_Blessing::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return;
	
	AChessUnitBase* Target = Grid->GetUnitAt(TargetPos);
	if (!Target) return;
	
	const float MissingHealth = Target->Attributes->GetMaxHealth() - Target->Attributes->GetHealth();
	const int32 Heal = FMath::Min(static_cast<int32>(HealAmount), MissingHealth);
	if (Heal <= 0) return;
	
	OnAbilityAffectedUnitFeedback(AbilityType, Unit, Target);
	ApplyFlatDamage(Unit, Target, -Heal);
}
