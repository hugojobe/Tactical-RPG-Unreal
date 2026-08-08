// By hugojobe.

#include "GAS/Abilities/ChessAbility_Execution.h"
#include "GAS/ChessAbilityBase.h"
#include "Units/ChessUnitBase.h"

UChessAbility_Execution::UChessAbility_Execution()
{
	AbilityType = EChessAbilityType::Execution;
	Targeting = EChessAbilityTargeting::Self;
	CooldownTurns = 3;
}

void UChessAbility_Execution::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	FGameplayEffectContextHandle Context = Unit->AbilitySystemComponent->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = Unit->AbilitySystemComponent->MakeOutgoingSpec(EmpoweredEffectClass, 1.f, Context);
	if (Spec.IsValid())
	{
		Unit->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}
