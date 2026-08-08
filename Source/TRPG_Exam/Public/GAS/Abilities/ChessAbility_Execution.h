// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Execution.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_Execution : public UChessAbilityBase
{
	GENERATED_BODY()
	
public:
	UChessAbility_Execution();
	
	UPROPERTY(EditDefaultsOnly, Category = "Chess Ability")
	TSubclassOf<UGameplayEffect> EmpoweredEffectClass;
	
protected:
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;	
};
