// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Intimidation.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_Intimidation : public UChessAbilityBase
{
	GENERATED_BODY()
	
public:
	UChessAbility_Intimidation();
	
	UPROPERTY(EditDefaultsOnly, Category="Chess Ability")
	int32 DebuffTurns = 1;
	
	UPROPERTY(EditDefaultsOnly, Category = "Chess Ability")
	TSubclassOf<UGameplayEffect> IntimidatedEffectClass;
	
protected:
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;	
};
