// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_DoubleLeap.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_DoubleLeap : public UChessAbilityBase
{
	GENERATED_BODY()
	
	UChessAbility_DoubleLeap();
	
protected:
	virtual TArray<FIntPoint> GetValidTargets(const AChessUnitBase* Unit) const override;
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;
};
