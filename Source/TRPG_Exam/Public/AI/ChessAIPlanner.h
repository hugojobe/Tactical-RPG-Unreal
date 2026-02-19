#pragma once

#include "CoreMinimal.h"
#include "ChessMovementLogic.h"
#include "Units/ChessUnitBase.h"
#include "ChessAIPlanner.generated.h"

UENUM(BlueprintType)
enum class EChessDecisionType: uint8
{    
    Null,
    Move,
    Attack
};

USTRUCT(BlueprintType)
struct FVirtualUnit
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EChessTeam Team;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMovementRule UnitMovementRule;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FUnitStats UnitStats;
};

UCLASS(BlueprintType)
class UVirtualBoard : public UObject, public IChessBoardInterface
{
    GENERATED_BODY()
    
public:
    UVirtualBoard();
    
    int Width, Height;
    
    TMap<FIntPoint, FVirtualUnit> Units;
    
public:
    virtual bool IsValidGridPos(const FIntPoint& Pos, int BoardWidth, int BoardHeight) const override;
    virtual bool IsTileOccupied(const FIntPoint& Pos) const override;
    virtual EChessTeam GetTeamAt(const FIntPoint& Pos) const override;
};

USTRUCT(BlueprintType)
struct FChessDecision
{
    GENERATED_BODY()
    
    EChessDecisionType Type;
    float Score;
    FIntPoint TargetPosition;
};

UCLASS()
class UChessAIPlanner : public UObject
{
    GENERATED_BODY()
    
public:
    AChessUnitBase* MyUnit;
    AChessGridManager* GridManager;
    
    float AggressionFactor = 1.0f;
    float SafetyFactor = 1.0f;
    
    static UVirtualBoard* GenerateVirtualBoard(AChessGridManager* GridManager);
    static TMap<FIntPoint, float> GenerateThreatMap(AChessGridManager* GridManager);
    
    FChessDecision GetUnitDecision();
    
};
