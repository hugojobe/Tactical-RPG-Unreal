// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ChessCheatManager.generated.h"

UCLASS()
class TRPG_EXAM_API UChessCheatManager : public UCheatManager
{
	GENERATED_BODY()
	
	UFUNCTION(Exec)
	void AddRP(int32 Amount);
};
