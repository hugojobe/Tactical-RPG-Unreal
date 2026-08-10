// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "AI/ChessAIPlanner.h"
#include "GameFramework/GameModeBase.h"
#include "ChessGameMode.generated.h"

class AChessGridManager;

struct FEnemyAbilityCandidate
{
	FIntPoint UnitPos = FIntPoint(-1, -1);
	EChessAbilityType Type = EChessAbilityType::None;
};

struct FEnemyAbilityChoice
{
	bool bUseAbility = false;
	FIntPoint UnitPos = FIntPoint(-1, -1);
	FIntPoint TargetPos = FIntPoint(-1, -1);
};

UCLASS()
class TRPG_EXAM_API AChessGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AChessGameMode();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Framework")
	int32 TurnCount = 0;
	
	bool bPlayerKingExists = false;
	bool bEnemyKingExists = false;
	
	bool bDisplayVirtualBoard = false;
	bool bDisplayThreatMap = false;
	bool bDisplayDefenseMap = false;
	bool bDisplayBalanceMap = false;
	
	FVirtualBoard VirtualBoard;
	TMap<FIntPoint, float> ThreatMap;
	TMap<FIntPoint, float> DefenseMap;
	TMap<FIntPoint, float> BalanceMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AChessGridManager* GridManager;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
	ETurnState CurrentTurn;
	
	UPROPERTY()
	UChessAIPlanner* GlobalPlanner;
	
	UPROPERTY(EditAnywhere, Category = "AI")
	float AbilityScoreMargin = 2.f;

	UPROPERTY(EditAnywhere, Category = "AI")
	int32 AbilitySearchDepth = 8;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void PlayerActionFinished(AChessGridManager* Manager);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void EndPlayerTurn();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void EndEnemyTurn();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void SetCurrentTurn(ETurnState NewTurnState);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void StartEnemyTurn();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void StartPlayerTurn();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	bool CheckWinCondition(bool TriggerGameOver = true);

	UFUNCTION(BlueprintNativeEvent, Category = "Game Flow")
	void OnGameOver(EChessTeam WinningTeam, EGameOverCause Cause);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AI")
	void GenerateVirtualBoard();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AI")
	void GenerateThreatMap();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AI")
	void GenerateDefenseMap();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AI")
	void GenerateBalanceMap();
	
	UFUNCTION(BlueprintNativeEvent, Category = "AI")
	void ComputeGlobalAI(FChessDecision& BestTask, AChessUnitBase*& OutBestUnit, const TArray<AChessUnitBase*>& EnemyUnitsThinking);
	
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void TickAbilityCooldowns(EChessTeam Team);
	
	UPROPERTY(EditAnywhere, Category = "AI")
	int32 AbilityScreeningDepth = 2;

	UPROPERTY(EditAnywhere, Category = "AI")
	int32 AbilityFinalDepth = 5;

	UPROPERTY(EditAnywhere, Category = "AI")
	int32 AbilityFinalistCount = 3;

	TArray<FEnemyAbilityCandidate> GatherEnemyAbilityCandidates() const;
	FEnemyAbilityChoice EvaluateAbilityCandidates(const TArray<FEnemyAbilityCandidate>& Candidates, FVirtualBoard Board) const;
	void FinishEnemyAbilityPhase(const FEnemyAbilityChoice& Choice);
	void BeginEnemyMovePhase();
	
	UFUNCTION(Exec)
	void DisplayVirtualBoard();
	
	UFUNCTION(Exec)
	void DisplayThreatMap();
	
	UFUNCTION(Exec)
	void DisplayDefenseMap();
	
	UFUNCTION(Exec)
	void DisplayBalanceMap();
};
