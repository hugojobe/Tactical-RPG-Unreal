// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Repulsion.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_Repulsion : public UChessAbilityBase
{
	GENERATED_BODY()
	
public:
	UChessAbility_Repulsion();
	
protected:
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;	
};
