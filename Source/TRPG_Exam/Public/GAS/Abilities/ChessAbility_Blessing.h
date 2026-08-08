// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Blessing.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_Blessing : public UChessAbilityBase
{
	GENERATED_BODY()
	
public:
	UChessAbility_Blessing();
	
	UPROPERTY(EditDefaultsOnly, Category="Chess Ability")
	int32 HealAmount = 1;
	
protected:
	virtual TArray<FIntPoint> GetValidTargets(const AChessUnitBase* Unit) const override;
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;
};
