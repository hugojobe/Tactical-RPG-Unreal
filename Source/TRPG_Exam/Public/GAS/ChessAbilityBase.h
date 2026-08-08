// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "Abilities/GameplayAbility.h"
#include "ChessAbilityBase.generated.h"

class AChessUnitBase;
class AChessGridManager;

UENUM(BlueprintType)
enum class EChessAbilityTargeting : uint8
{
	Self		UMETA(DisplayName = "Self"),
	Tile		UMETA(DisplayName = "Tile"),
	AllyUnit	UMETA(DisplayName = "Ally Unit"),
	EnemyUnit	UMETA(DisplayName = "Enemy Unit")
};

UCLASS(Abstract, Blueprintable)
class TRPG_EXAM_API UChessAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UChessAbilityBase();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Chess Ability")
	void OnAbilityActivatedFeedback(EChessAbilityType Type, AChessUnitBase* Unit, FIntPoint TargetPos);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Chess Ability")
	void OnAbilityAffectedUnitFeedback(EChessAbilityType Type, AChessUnitBase* Caster, AChessUnitBase* AffectedUnit);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chess Ability")
	EChessAbilityType AbilityType = EChessAbilityType::None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chess Ability")
	EChessAbilityTargeting Targeting = EChessAbilityTargeting::Self;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chess Ability")
	int32 CooldownTurns = 3;
	
	UPROPERTY(EditDefaultsOnly, Category = "Chess Ability")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chess Ability")
	bool bActivatable = true;
	
	UFUNCTION(BlueprintCallable, Category = "Chess Ability")
	virtual TArray<FIntPoint> GetValidTargets(const AChessUnitBase* Unit) const;
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;
	
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos);
	
	AChessGridManager* GetGridManager(const AChessUnitBase* Unit) const;
	
	void ApplyFlatDamage(AChessUnitBase* Source, AChessUnitBase* Target, int32 Amount);
};
