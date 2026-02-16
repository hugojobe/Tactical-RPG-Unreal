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

	UPROPERTY(VisibleAnywhere, Category = "AI Debug")
	FInfluenceMap LastThreatMap;

	UPROPERTY(VisibleAnywhere, Category = "AI Debug")
	FInfluenceMap LastBenefitMap;

	void DrawDebugVisualization(UWorld* World, AChessGridManager* Grid, bool bDrawThreats, bool bDrawBenefits);
	
	float AggressionFactor = 1.0f;
	float SafetyFactor = 1.0f;

// MINIMAX
	float Minimax(FVirtualBoard& Board, int32 Depth, float Alpha, float Beta, bool bMaximizingPlayer);
	float EvaluateBoard(const FVirtualBoard& Board);
	FVirtualBoard CreateVirtualBoard();
	void GetLegalMoves(const FVirtualBoard& Board, EChessTeam Team, TArray<FVirtualMove>& OutMoves);
	void ApplyMove(FVirtualBoard& Board, const FVirtualMove& Move);

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
