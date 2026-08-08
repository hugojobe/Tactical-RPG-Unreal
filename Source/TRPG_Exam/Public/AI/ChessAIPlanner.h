#pragma once

#include "CoreMinimal.h"
#include "ChessMovementLogic.h"
#include "Tiles/ChessTileBase.h"
#include "Units/ChessUnitBase.h"
#include "ChessAIPlanner.generated.h"

#define MAX_CHESS_BOARD_SIZE 64
#define TT_SIZE 1048583

UENUM(BlueprintType)
enum class EChessDecisionType: uint8
{    
    Null,
    Move,
    Attack
};

enum class EHashBound : uint8 
{ 
    Exact, 
    LowerBound, 
    UpperBound 
};

struct FTTEntry 
{
    uint64 Key = 0;
    float Score = 0.0f;
    int Depth = -1;
    EHashBound Bound = EHashBound::Exact;
    FIntPoint BestMoveTarget = FIntPoint(-1, -1);
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
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EChessUnitType UnitType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasMoved = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAbilityUsedThisMatch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RootedTurns = 0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EChessAbilityType AbilityType = EChessAbilityType::None;
};

USTRUCT(BlueprintType)
struct FVirtualBoard
{
    GENERATED_BODY()
    
public:
    int Width = 6;
    int Height = 6;
    
    FVirtualUnit Grid[MAX_CHESS_BOARD_SIZE];
    uint8 BlockedTurns[MAX_CHESS_BOARD_SIZE] = {};

    int32 GetIndex(const FIntPoint& Pos) const;

    void MoveUnit(FIntPoint SourcePos, FIntPoint TargetPos, FVirtualUnit Unit);
    void AttackUnit(FIntPoint SourcePos, FIntPoint TargetPos, FVirtualUnit SourceUnit, FVirtualUnit TargetUnit);

    uint64 ComputeHash(const uint64 Zobrist[64][3][7], bool bIsMaximizing) const;
    virtual bool IsValidGridPos(const FIntPoint& Pos, int BoardWidth, int BoardHeight) const;
    virtual bool IsTileOccupied(const FIntPoint& Pos) const;
    virtual EChessTeam GetTeamAt(const FIntPoint& Pos) const;
    virtual bool IsTileBlocked(const FIntPoint& Pos) const;
};

USTRUCT(BlueprintType)
struct FChessDecision
{
    GENERATED_BODY()
    
    EChessDecisionType Type;
    float Score;
    FIntPoint TargetPosition;
};

struct FAvailableMove {
    FIntPoint From;
    FIntPoint To;
    FVirtualUnit Unit;
    ETileInteraction Interaction;
    float Priority;
};

UCLASS()
class UChessAIPlanner : public UObject
{
    GENERATED_BODY()
    
public:
    AChessUnitBase* MyUnit;
    AChessGridManager* GridManager;
    
    int32 PositionsEvaluated = 0;
    bool bAbortSearch = false;
    double SearchStartTime = 0.0;
    double MaxSearchTime = 0.5;
    
    int32 LastCompletedDepth = 0;
    
    UFUNCTION(BlueprintCallable)
    
    static FVirtualBoard GenerateVirtualBoard(TMap<FIntPoint, FVirtualUnit> VUnits, int Width, int Height);
    static TMap<FIntPoint, float> GenerateThreatMap(FVirtualBoard VBoard);
    static TMap<FIntPoint, float> GenerateDefenseMap(FVirtualBoard VBoard);
    static TMap<FIntPoint, float> GenerateBalanceMap(TArray<FIntPoint> Tiles, TMap<FIntPoint, float> Threats, TMap<FIntPoint, float> Benefits);
    
    FChessDecision GetUnitDecision(int ThinkDepth, TArray<FIntPoint> Tiles, FVirtualBoard Board);
    float Minimax(TArray<FIntPoint> Tiles, const FVirtualBoard& Board, int Depth, float Alpha, float Beta, bool bIsMaximizing);
    float EvaluateBoard(FVirtualBoard Board, TArray<FIntPoint> Tiles);
};