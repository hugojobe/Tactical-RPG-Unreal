// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Punishment.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_Punishment : public UChessAbilityBase
{
	GENERATED_BODY()
	
public:
	UChessAbility_Punishment();
	
	UPROPERTY(EditDefaultsOnly, Category="Chess Ability")
	int32 DamageAmount = 1;
	
protected:
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;	
};
