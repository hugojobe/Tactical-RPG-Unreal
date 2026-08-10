// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ChessAbilityBase.h"
#include "ChessAbility_Bastion.generated.h"


UCLASS()
class TRPG_EXAM_API UChessAbility_Bastion : public UChessAbilityBase
{
	GENERATED_BODY()
	
public:
	UChessAbility_Bastion();
	
	UPROPERTY(EditDefaultsOnly, Category = "Chess Ability")
	int32 WallTurns = 2;
	
	UPROPERTY(EditDefaultsOnly, Category="Chess Ability")
	int32 RootedTurns = 1;
	
	UPROPERTY(EditDefaultsOnly, Category = "Chess Ability")
	TSubclassOf<UGameplayEffect> RootedEffectClass;
	
protected:
	virtual void ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos) override;	
};
