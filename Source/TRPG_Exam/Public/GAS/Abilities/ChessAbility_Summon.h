// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Summon.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_Summon : public UChessAbilityBase
{
	GENERATED_BODY()
	
public:
	UChessAbility_Summon();
	
protected:
	virtual TArray<FIntPoint> GetValidTargets(const AChessUnitBase* Unit) const override;
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;	
};
