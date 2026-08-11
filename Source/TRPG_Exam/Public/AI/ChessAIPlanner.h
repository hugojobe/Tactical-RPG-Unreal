#pragma once

#include "CoreMinimal.h"
#include "ChessMovementLogic.h"
#include "JsonObjectConverter.h"
#include "Units/ChessUnitBase.h"
#include "ChessAIPlanner.generated.h"

#define MAX_CHESS_BOARD_SIZE 64
#define TT_SIZE 1048583
#define MAX_SEARCH_PLY 64

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
    EChessTeam Team = EChessTeam::None;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FUnitStats UnitStats;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EChessUnitType UnitType = EChessUnitType::Pawn;

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
    bool IsValidGridPos(const FIntPoint& Pos, int BoardWidth, int BoardHeight) const;
    bool IsTileOccupied(const FIntPoint& Pos) const;
    EChessTeam GetTeamAt(const FIntPoint& Pos) const;
    bool IsTileBlocked(const FIntPoint& Pos) const;
};

USTRUCT(BlueprintType)
struct FChessDecision
{
    GENERATED_BODY()
    
    EChessDecisionType Type;
    float Score;
    FIntPoint TargetPosition;
    FIntPoint SourcePosition;
};

struct FAvailableMove {
    FIntPoint From;
    FIntPoint To;
    FVirtualUnit Unit;
    ETileInteraction Interaction;
    float Priority;
};

struct FKillerMove
{
    FIntPoint From = FIntPoint(-1, -1);
    FIntPoint To = FIntPoint(-1, -1);
};

USTRUCT()
struct FAIWeights
{
    GENERATED_BODY()

    UPROPERTY() float MaterialWeight = 10.0f; 
    UPROPERTY() float HPWeight = 6.0f;
    UPROPERTY() float DamageWeight = 8.0f;

    // Positional
    UPROPERTY() float PositionalScale = 3.0f;
    UPROPERTY() float DevelopmentPenalty = 1.5f;
    UPROPERTY() float PawnChainBonus = 2.0f;

    // Strategic
    UPROPERTY() float ReachWeight = 0.15f;
    UPROPERTY() float HangingPenalty = 0.10f;

    // Royals
    UPROPERTY() float KingThreatWeight = 25.0f;
    UPROPERTY() float EmperorAggression = 5.0f;

    // Root behaviour
    UPROPERTY() float ActivityBonusWeight = 0.005f;
};

UCLASS()
class UChessAIPlanner : public UObject
{
    GENERATED_BODY()
    
public:
    AChessGridManager* GridManager;
    
    int32 PositionsEvaluated = 0;
    bool bAbortSearch = false;
    double SearchStartTime = 0.0;
    double MaxSearchTime = 1.5f;
    int32 MaxSearchNodes = 0;
    
    int32 ActivityBonusCap = 1;
    
    float RootNoiseAmplitude = 0.0f;
    
    bool bExpectPlayerKing = true;
    bool bExpectEnemyKing = true;
    
    UPROPERTY()
    FAIWeights Weights;
    
    FIntPoint SourcePosition = FIntPoint(-1, -1);
    
    int32 LastCompletedDepth = 0;
    
    TMap<FIntPoint, FIntPoint> UnitLastPositions;
    TMap<FIntPoint, int32> UnitIdleTurns;
    
    uint64 ZobristTable[64][3][7];
    TArray<FTTEntry> TranspositionTable;
    
    FKillerMove KillerMoves[MAX_SEARCH_PLY][2];
    int32 HistoryHeuristic[64][64];
    void ClearSearchHeuristics();
    
    int32 MaxQuiescenceDepth = 4;
    float Quiescence(const TArray<FIntPoint>& Tiles, const FVirtualBoard& Board, float Alpha, float Beta, bool bIsMaximizing, int32 QDepth);
    
    bool bEnableStrategicEval = false;
    TArray<FChessDecision> EvalScratchMoves;
    
    bool bUseNullMove = false;
    bool bUseLMR = true;
    
    TArray<FChessDecision> SearchScratchMoves;
    
    static FMovementRule MovementRuleByType[7];

    static const FMovementRule& GetMovementRule(EChessUnitType Type)
    {
        return MovementRuleByType[static_cast<int32>(Type)];
    }

    static void RegisterMovementRule(EChessUnitType Type, const FMovementRule& Rule)
    {
        MovementRuleByType[static_cast<int32>(Type)] = Rule;
    }
    
    UFUNCTION(BlueprintCallable)
    static FVirtualBoard GenerateVirtualBoard(TMap<FIntPoint, FVirtualUnit> VUnits, int Width, int Height);
    static TMap<FIntPoint, float> GenerateThreatMap(FVirtualBoard VBoard);
    static TMap<FIntPoint, float> GenerateDefenseMap(FVirtualBoard VBoard);
    static TMap<FIntPoint, float> GenerateBalanceMap(TArray<FIntPoint> Tiles, TMap<FIntPoint, float> Threats, TMap<FIntPoint, float> Benefits);
    
    void InitializeZobrist();
    FChessDecision GetGlobalDecision(int ThinkDepth, const TArray<FIntPoint>& Tiles, FVirtualBoard Board);
    
    float Minimax(const TArray<FIntPoint>& Tiles, const FVirtualBoard& Board, int Depth, float Alpha, float Beta, bool bIsMaximizing);
    float EvaluateBoard(const FVirtualBoard& Board, const TArray<FIntPoint>& Tiles);
    float EvaluateBoardShallow(const TArray<FIntPoint>& Tiles, const FVirtualBoard& Board, int Depth, double TimeBudget);

    void SaveWeights(const FString& FileName) const;
    bool LoadWeights(const FString& FileName);
};