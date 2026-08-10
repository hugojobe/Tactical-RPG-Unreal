// By hugojobe.

#include "GAS/Abilities/ChessAbility_Punishment.h"

#include "Framework/ChessGameMode.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"

UChessAbility_Punishment::UChessAbility_Punishment()
{
	AbilityType = EChessAbilityType::Intimidation;
	Targeting = EChessAbilityTargeting::Self;
	CooldownTurns = 3;
}

void UChessAbility_Punishment::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	AChessGridManager* Grid = Unit->GridManager;
	
	static FIntPoint Neighbors[] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);

	for (const FIntPoint& Offset : Neighbors)
	{
		AChessUnitBase* Target = Grid->GetUnitAt(Origin + Offset);
		if (!Target || Target->Team == Unit->Team) continue;

		ApplyFlatDamage(Unit, Target, DamageAmount);
		
		const bool bFatal = Target->Attributes->GetHealth() <= 0.f;
		Grid->PlayImpactFeedback(Unit, Target, bFatal, DamageAmount);
		OnAbilityAffectedUnitFeedback(AbilityType, AbilityDisplayName, Unit, Target);
		if (bFatal)
		{
			Grid->ClearTile(Origin + Offset);
			Grid->GameMode->CheckWinCondition(true);
		}
	}
}
