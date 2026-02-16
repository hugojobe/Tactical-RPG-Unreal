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
	FHTNTask MakePlan(const FVirtualBoard& InitialBoard);
	
	AChessUnitBase* MyUnit;
	AChessGridManager* GridManager;

	UPROPERTY(VisibleAnywhere, Category = "AI Debug")
	FInfluenceMap LastThreatMap;

	UPROPERTY(VisibleAnywhere, Category = "AI Debug")
	FInfluenceMap LastBenefitMap;
	
	float AggressionFactor = 1.0f;
	float SafetyFactor = 1.0f;

	void NotifyExecutedMove(const FIntPoint& From, const FIntPoint& To);
	FIntPoint LastMoveSource;
	FIntPoint LastMoveDestination;
	bool bHasLastMove = false;


// MINIMAX
	float Minimax(FVirtualBoard& Board, int32 Depth, float Alpha, float Beta, bool bMaximizingPlayer);
	float EvaluateBoard(const FVirtualBoard& Board);
	FVirtualBoard CreateVirtualBoard();
	FVirtualBoard CreateVirtualBoard(const TArray<AChessUnitBase*>& AllUnits);
	void GetLegalMovesForUnit(const FVirtualBoard& Board, int32 UnitID, TArray<FVirtualMove>& OutMoves);
	void ApplyMove(FVirtualBoard& Board, const FVirtualMove& Move);
};
