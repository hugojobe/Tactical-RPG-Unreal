// By hugojobe.

#include "GAS/Abilities/ChessAbility_Intimidation.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"

UChessAbility_Intimidation::UChessAbility_Intimidation()
{
	AbilityType = EChessAbilityType::Intimidation;
	Targeting = EChessAbilityTargeting::Self;
	CooldownTurns = 3;
}

void UChessAbility_Intimidation::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	AChessGridManager* Grid = Unit->GridManager;
	
	static FIntPoint Neighbors[] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);

	for (const FIntPoint& Offset : Neighbors)
	{
		AChessUnitBase* Target = Grid->GetUnitAt(Origin + Offset);
		if (!Target || Target->Team == Unit->Team) continue;

		FGameplayEffectContextHandle Context = Unit->AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = Unit->AbilitySystemComponent->MakeOutgoingSpec(IntimidatedEffectClass, 1.f, Context);
		if (Spec.IsValid())
		{
			Spec.Data->SetStackCount(DebuffTurns);
			Target->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			OnAbilityAffectedUnitFeedback(AbilityType, AbilityDisplayName, Unit, Target);
		}
	}
}
