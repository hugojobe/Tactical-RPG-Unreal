// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Escape.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_Escape : public UChessAbilityBase
{
	GENERATED_BODY()
	
public:
	UChessAbility_Escape();
	
	UPROPERTY(EditDefaultsOnly, Category = "Chess Ability")
	int32 JumpRadius = 2;
	
protected:
	virtual TArray<FIntPoint> GetValidTargets(const AChessUnitBase* Unit) const override;
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;	
};
