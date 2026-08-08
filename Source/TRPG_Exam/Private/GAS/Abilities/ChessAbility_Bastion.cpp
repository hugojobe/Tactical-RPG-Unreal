// By hugojobe.

#include "GAS/Abilities/ChessAbility_Bastion.h"
#include "GAS/ChessAbilityBase.h"
#include "Units/ChessUnitBase.h"

UChessAbility_Bastion::UChessAbility_Bastion()
{
	AbilityType = EChessAbilityType::Bastion;
	Targeting = EChessAbilityTargeting::Self;
	CooldownTurns = 3;
}

void UChessAbility_Bastion::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	FGameplayEffectContextHandle Context = Unit->AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);
	
	UGameplayEffect* ShieldEffect = NewObject<UGameplayEffect>(GetTransientPackage(), FName("BastionShield"));
	ShieldEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
	
	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UChessAttributeSet::GetHealthAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;
	ModifierInfo.ModifierMagnitude = FScalableFloat(BonusHealth);
	ShieldEffect->Modifiers.Add(ModifierInfo);
	
	Unit->AbilitySystemComponent->ApplyGameplayEffectToSelf(ShieldEffect, 1.0f, Context);
	
	FGameplayEffectSpecHandle RootedSpec = Unit->AbilitySystemComponent->MakeOutgoingSpec(RootedEffectClass, 1.f, Context);
	if (RootedSpec.IsValid())
	{
		RootedSpec.Data->SetStackCount(RootedTurns + 1);
		Unit->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*RootedSpec.Data.Get());
	}
}
