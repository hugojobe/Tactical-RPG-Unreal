#pragma once

#include "CoreMinimal.h"
#include "ChessAITypes.h"
#include "Managers/ChessGridManager.h"
#include "UChessHTNPlanner.generated.h"

UCLASS()
class UChessHTNPlanner : public UObject
{
	GENERATED_BODY()
	
public:
	FHTNTask MakePlan();
	
	AChessUnitBase* MyUnit;
	AChessGridManager* GridManager;
	
	float AggressionFactor = 1.0f;
	float SafetyFactor = 1.0f;

private:
	void BuildInfluenceMaps(FWorldState& State);
	
	bool Method_WinGame(const FWorldState& State, FHTNTask& OutTask);
	bool Method_TryKillKing(const FWorldState& State, FHTNTask& OutTask);
	bool Method_FindBestTrade(const FWorldState& State, FHTNTask& OutTask);
	bool Method_PlaySafe(const FWorldState& State, FHTNTask& OutTask);
	
	FHTNTask FindBestAttack(const FWorldState& State, bool bPrioritizeKing);
	FHTNTask FindSafeMove(const FWorldState& State);
	FHTNTask FindAnyMove(const FWorldState& State);
};
