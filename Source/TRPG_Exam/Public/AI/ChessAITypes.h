#pragma once

#include "CoreMinimal.h"
#include "ChessAITypes.generated.h"

class AChessUnitBase;

USTRUCT(BlueprintType)
struct FInfluenceMap
{
	GENERATED_BODY()
	
	TArray<float> GridValues;
	int32 Width;
	int32 Height;

	void Init(int32 W, int32 H)
	{
		Width = W;
		Height = H;
		GridValues.Init(0.0f, W * H);
	}

	void AddValue(int32 X, int32 Y, float Val)
	{
		if (X >= 0 && X < Width && Y >= 0 && Y < Height)
		{
			GridValues[Y * Width + X] += Val;
		}
	}

	float GetValue(int32 X, int32 Y) const
	{
		if (X >= 0 && X < Width && Y >= 0 && Y < Height)
		{
			return GridValues[Y * Width + X];
		}
		return 0.0f;
	}
};

UENUM()
enum class EHTNTaskType : uint8
{
	Action,
	Goal
};

USTRUCT(BlueprintType)
struct FHTNTask
{
	FName TaskName;
	EHTNTaskType Type;
	
	AChessUnitBase* TargetUnit = nullptr;
	FIntPoint TargetTile = FIntPoint(-1, -1);
	float Score = 0.0f;

	bool IsValid() const { return !TaskName.IsNone(); }
};


struct FWorldState
{
	bool bKingExposed;
	bool bInCheck;
    
	FInfluenceMap ThreatMap;
	FInfluenceMap BenefitMap; 
    
	TArray<AChessUnitBase*> MyUnits;
	TArray<AChessUnitBase*> EnemyUnits;
};