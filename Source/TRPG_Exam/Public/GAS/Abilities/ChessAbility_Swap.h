// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Swap.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_Swap : public UChessAbilityBase
{
	GENERATED_BODY()
	
public:
	UChessAbility_Swap();
	
protected:
	virtual TArray<FIntPoint> GetValidTargets(const AChessUnitBase* Unit) const override;
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;	
};
