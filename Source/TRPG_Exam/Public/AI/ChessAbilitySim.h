#pragma once

#include "CoreMinimal.h"
#include "AI/ChessAIPlanner.h"

class TRPG_EXAM_API ChessAbilitySim
{
public:
	static TArray<FIntPoint> GetAbilityTargets(const FVirtualBoard& Board, const FIntPoint& UnitPos, EChessAbilityType Type);
	static void ApplyAbility(FVirtualBoard& Board, const FIntPoint& UnitPos, EChessAbilityType Type, const FIntPoint& TargetPos);
};
