// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Caltrop.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_Caltrop : public UChessAbilityBase
{
	GENERATED_BODY()
	
public:
	UChessAbility_Caltrop();
	
	UPROPERTY(EditDefaultsOnly, Category="Chess Ability")
	int32 BlockTurns = 2;
	
protected:
	virtual TArray<FIntPoint> GetValidTargets(const AChessUnitBase* Unit) const override;
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;
};
