// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/ChessAIPlanner.h"
#include "Units/ChessUnitBase.h"
#include "ChessLevelDesigner.generated.h"

USTRUCT()
struct FPlayoutResult
{
	GENERATED_BODY()

	EChessTeam Winner = EChessTeam::None;
	int32 Rounds = 0;
	int32 LeadChanges = 0;
	int32 PlayerPiecesLost = 0;
	int32 EnemyPiecesLost = 0;
	float FinalEval = 0.f;
};

USTRUCT(BlueprintType)
struct FLevelPlacementEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) EChessUnitType UnitType = EChessUnitType::Pawn;
	UPROPERTY(BlueprintReadOnly) EChessTeam Team = EChessTeam::None;
	UPROPERTY(BlueprintReadOnly) FIntPoint Position = FIntPoint(-1, -1);
};

USTRUCT(BlueprintType)
struct FLevelCandidate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TArray<FLevelPlacementEntry> Placement;
	UPROPERTY(BlueprintReadOnly) float Score = -BIG_NUMBER;
	UPROPERTY(BlueprintReadOnly) int32 Rounds = 0;
	UPROPERTY(BlueprintReadOnly) int32 LeadChanges = 0;
	UPROPERTY(BlueprintReadOnly) EChessTeam Winner = EChessTeam::None;
	UPROPERTY(BlueprintReadOnly) int32 BoardWidth = 6;
	UPROPERTY(BlueprintReadOnly) int32 BoardHeight = 6;
};

USTRUCT(BlueprintType)
struct FLevelGenerationRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 4, ClampMax = 8))
	int32 BoardWidth = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 4, ClampMax = 8))
	int32 BoardHeight = 6;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<EChessUnitType> AllowedPieces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PlayerPieceCount = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EnemyPieceCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnemyBoss = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIncludeRoyal = true;
};

USTRUCT()
struct FTuningPosition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) TArray<FString> Rows;
	UPROPERTY(EditAnywhere) bool bHasRoyals = true;
};

UCLASS()
class TRPG_EXAM_API AChessLevelDesigner : public AActor
{
	GENERATED_BODY()
	
	AChessLevelDesigner();

public:
	UPROPERTY(EditAnywhere, Category = "Design")
	UDataTable* UnitStatsTable;

	UPROPERTY(EditAnywhere, Category = "Design")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, Category = "Design")
	int32 CandidatesPerLevel = 60;

	UPROPERTY(EditAnywhere, Category = "Design")
	int32 SelfPlayDepth = 4;

	UPROPERTY(EditAnywhere, Category = "Design")
	int32 MaxRounds = 30;

	UFUNCTION(CallInEditor, Category = "Design")
	void GenerateLevels();
	
	UFUNCTION(CallInEditor, Category = "Design")
	void GenerateLevelsBlocking();
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Design")
	TArray<FLevelCandidate> BestPerLevel;
	
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 CurrentLevel = 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 CandidateIndex = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Design")
	TArray<FLevelGenerationRule> LevelRules;

	UFUNCTION(BlueprintCallable, Category = "Design") void StartGeneration();
	UFUNCTION(BlueprintCallable, Category = "Design") void StartSingleLevelGeneration(int32 LevelIndex);
	UFUNCTION(BlueprintCallable, Category = "Design") void CancelGeneration();
	UFUNCTION(BlueprintCallable, Category = "Design") void StepGeneration(int32 PlayoutsPerStep);
	UFUNCTION(BlueprintPure, Category = "Design") bool IsRunning() const { return bRunning; }
	UFUNCTION(BlueprintPure, Category = "Design") float GetProgress() const;
	
	UPROPERTY(EditAnywhere, Category = "Tuning") TArray<FTuningPosition> TuningPositions;
	UPROPERTY(EditAnywhere, Category = "Tuning") int32 TuningNodeBudget = 50000;
	UPROPERTY(EditAnywhere, Category = "Tuning") int32 TuningIterations = 50;
	UPROPERTY(EditAnywhere, Category = "Tuning") float AcceptThreshold = 0.57f;

	UFUNCTION(CallInEditor, Category = "Tuning") void RunTuning();

private:
	UPROPERTY(Transient) UChessAIPlanner* PlannerA;
	UPROPERTY(Transient) UChessAIPlanner* PlannerB;
	bool bRunning = false;
	
	FRandomStream RNG;
	bool bSingleLevelMode = false;
	int32 RegenCounter = 0;
	
	UPROPERTY(EditAnywhere, Category = "Design")
	int32 NoContactCutoff = 10;

	void EnsurePlanners();
	FVirtualUnit MakeUnit(EChessUnitType Type, EChessTeam Team) const;
	FVirtualBoard BuildRandomSetup(FRandomStream& RNG, int32 LevelIndex) const;
	FPlayoutResult Playout(FVirtualBoard Board);
	FPlayoutResult PlayoutVersus(FVirtualBoard Board, UChessAIPlanner* EnemyBrain, UChessAIPlanner* PlayerBrain);
	bool FindBestMove(UChessAIPlanner* Planner, const TArray<FIntPoint>& Tiles, const FVirtualBoard& Board, EChessTeam Team, FIntPoint& OutFrom, FIntPoint& OutTo, ETileInteraction& OutInteraction);	float ScoreCandidate(const FPlayoutResult& Result) const;
	FString ExportBoard(const FVirtualBoard& Board) const;
	FString ExportCandidate(const FLevelCandidate& Candidate) const;
	
	void ConfigureForTuning(UChessAIPlanner* Planner);
	FVirtualBoard ParsePosition(const FTuningPosition& Position) const;
	FAIWeights MutateWeights(const FAIWeights& Source, float Sigma, FRandomStream& RNG) const;
	float RunMatch(const FAIWeights& Challenger, const FAIWeights& Champion);
};