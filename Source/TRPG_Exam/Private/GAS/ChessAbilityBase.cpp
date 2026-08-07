// By hugojobe.

#include "GAS/ChessAbilityBase.h"
#include "GAS/ChessGameplayTags.h"
#include "Units/ChessUnitBase.h"
#include "Managers/ChessGridManager.h"


UChessAbilityBase::UChessAbilityBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	
	ActivationBlockedTags.AddTag(TAG_Ability_Cooldown);
}

TArray<FIntPoint> UChessAbilityBase::GetValidTargets(const AChessUnitBase* Unit) const
{
	return TArray<FIntPoint>();
}

void UChessAbilityBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, 
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	AChessUnitBase* Unit = Cast<AChessUnitBase>(ActorInfo->AvatarActor.Get());
	if (!Unit)
	{
		UE_LOG(LogTemp, Error, TEXT("[HJ] ChessAbilityBase activated without a valid unit"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	ExecuteAbilityEffect(Unit, Unit->PendingAbilityTarget);
	
	OnAbilityActivatedFeedback(AbilityType, Unit, Unit->PendingAbilityTarget);
	
	if (CooldownTurns != 0)
	{
		if (!CooldownEffectClass)
		{
			UE_LOG(LogTemp, Error, TEXT("[HJ] %s has no CooldownEffectClass assigned"), *GetName());
		}
		else
		{
			FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(CooldownEffectClass, 1.f);
			if (CooldownSpec.IsValid())
			{
				const int32 Stacks = (CooldownTurns < 0) ? 99 : CooldownTurns;
				CooldownSpec.Data->SetStackCount(Stacks);
				ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
			}
		}
	}
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UChessAbilityBase::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) {}

AChessGridManager* UChessAbilityBase::GetGridManager(const AChessUnitBase* Unit) const
{
	return Unit ? Unit->GridManager : nullptr;
}
