#pragma once

#include "ChessAITypes.h"
#include "CoreMinimal.h"
#include "Enums.h"

class IChessBoardInterface
{
public:
	virtual bool IsValidGridPos(const FIntPoint& Pos) const = 0;
	virtual bool IsTileOccupied(const FIntPoint& Pos) const = 0;
	virtual EChessTeam GetTeamAt(const FIntPoint& Pos) const = 0;
};

class TRPG_EXAM_API ChessMovementLogic
{
	public:
		static void CalculateLegalMoves(
			const IChessBoardInterface* Board,
			const FIntPoint& StartPos,
			const FMovementRule& Rule,
			EChessTeam MyTeam,
			TArray<FVirtualMove>& OutMoves,
			bool bIncludeEmptyAttacks = false
		);
};