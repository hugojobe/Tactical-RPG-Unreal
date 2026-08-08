// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Swap.h"
#include "ChessAbility_GreatCastle.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_GreatCastle : public UChessAbility_Swap
{
	GENERATED_BODY()
	
	UChessAbility_GreatCastle();
	
protected:
	virtual TArray<FIntPoint> GetValidTargets(const AChessUnitBase* Unit) const override;
};
