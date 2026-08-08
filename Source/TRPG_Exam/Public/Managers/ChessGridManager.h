// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "AI/ChessAIPlanner.h"
#include "AI/ChessMovementLogic.h"
#include "GameFramework/Actor.h"
#include "Tiles/ChessTileBase.h"
#include "Units/ChessUnitBase.h"
#include "ChessGridManager.generated.h"

class AChessGameMode;
class UTimelineComponent;
class UCurveFloat;
class UChessAbilityBase;
struct FVirtualBoard;

USTRUCT(BlueprintType)
struct FUnitSpawnInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EChessUnitType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EChessTeam Team;
};


UCLASS()
class TRPG_EXAM_API AChessGridManager : public AActor, public IChessBoardInterface
{
	GENERATED_BODY()
	
public:	
	AChessGridManager();

	virtual void Initialize();
	void InitializeZobrist();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Config")
	float TileSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Config")
	int32 BoardWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Config")
	int32 BoardHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid State")
	TMap<FIntPoint, AChessUnitBase*> GridMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
	ETurnState CurrentTurnActionState = ETurnState::Player;

// VISUAL CONFIG
	UPROPERTY(EditAnywhere, Category = "Grid Visuals")
	TSubclassOf<AChessTileBase> WhiteTileClass;
	
	UPROPERTY(EditAnywhere, Category = "Grid Visuals")
	TSubclassOf<AChessTileBase> BlackTileClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visuals")
	TMap<FIntPoint, AChessTileBase*> TileActors;
	
	UPROPERTY(BlueprintReadOnly, Category = "Abilities")
	TMap<FIntPoint, int32> BlockedTiles;
	

// HELPER FUNCTIONS
	UFUNCTION(BlueprintCallable, Category = "Grid Logic")
	FIntPoint WorldToGrid(FVector WorldPos);

	UFUNCTION(BlueprintCallable, Category = "Grid Logic")
	FVector GridToWorld(FIntPoint GridPos);

	UFUNCTION(BlueprintCallable, Category = "Grid Logic")
	void RegisterUnitAt(FIntPoint GridPos, AChessUnitBase* Unit);

	UFUNCTION(BlueprintCallable, Category = "Grid Logic")
	void ClearTile(FIntPoint GridPos);

	UFUNCTION(BlueprintCallable, Category = "Grid Logic")
	AChessUnitBase* GetUnitAt(FIntPoint GridPos);

	UFUNCTION(BlueprintCallable, Category = "Grid Logic")
	bool IsValidGridPos(FIntPoint GridPos);
	
	UFUNCTION(BlueprintCallable, Category = "CombatLogic")
	static TMap<FIntPoint, ETileInteraction> GetValidMoves(FVirtualUnit Unit, FIntPoint UnitPos, FVirtualBoard Board, bool bIncludeEmptyAttacks = false);

	static bool IsEnemy(const AChessUnitBase* Unit, const AChessUnitBase* TargetUnit);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void BlockTile(FIntPoint Pos, int32 Turns);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void TickBlockedTiles();

	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsTileBlockedAt(FIntPoint Pos) const;
	
// GAMEPLAY
	TArray<FTTEntry> TranspositionTable;
	uint64 ZobristTable[64][3][7];
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Framewrok")
	AChessGameMode* GameMode;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Logic")
	AChessUnitBase* SelectedUnit;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Logic")
	TMap<FIntPoint, ETileInteraction> HighlightedTiles;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatLogic")
	bool IsUnitClicked;

	UPROPERTY()
	UTimelineComponent* MoveTimeline;

	UPROPERTY()
	UTimelineComponent* AttackTimeline;

	UPROPERTY(EditAnywhere, Category = "Pieces Movement")
	UCurveFloat* UnitMovementSpeedCurve;

	UPROPERTY(EditAnywhere, Category = "Pieces Movement")
    float UnitMovementTiltAngle = -10.0f;

	UPROPERTY()
	AChessUnitBase* MovingUnit;
	FVector MoveStartLoc;
	FVector MoveTargetLoc;

	UPROPERTY(EditAnywhere, Category = "Combat Visuals")
	UCurveFloat* AttackRushCurve;

	UPROPERTY(EditAnywhere, Category = "Combat Visuals")
	UCurveFloat* AttackBounceBackCurve;

	AChessUnitBase* AttackerUnit;
	AChessUnitBase* AttackTargetUnit;
	FVector AttackStartPos;
	FVector AttackerImpactPos;
	FVector AttackerFinalTargetPos;
    
	bool bWillKillUnit;
	EAttackPhase CurrentAttackPhase;

	UFUNCTION()
	void HandleMoveProgress(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pieces Movement")
	void HandleMoveFinished();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatLogic")
	void HandleAttackProgress(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat Visuals")
	void HandleAttackFinished();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat Visuals")
	void OnWinningHitPerformed();
	
	void RegisterStartingUnits();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatLogic")
	void OnTileClicked(AChessTileBase* ClickedTile);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatLogic")
	void OnUnitHoverUpdated(AChessUnitBase* Unit);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatLogic")
	void OnUnitClicked(AChessUnitBase* Unit);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatLogic")
	void DeselectUnit();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatLogic")
	void HighlightTilesForUnit(AChessUnitBase* Unit);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatLogic")
	void AttackUnit(AChessUnitBase* Attacker, AChessUnitBase* TargetUnit);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatLogic")
	void MoveUnit(AChessUnitBase* Unit, FIntPoint Position);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatLogic")
	void PlayerActionFinished();

	UFUNCTION(BlueprintNativeEvent, Category = "Combat Feedback")
	void PlayImpactFeedback(AChessUnitBase* Attacker, AChessUnitBase* Target, bool bIsFatal, int DamageReceived);
	
	bool CanTeamMove(EChessTeam Team, FVirtualBoard& Board);
	
	static FVirtualUnit MakeVirtualUnit(const AChessUnitBase* Unit);
	
// ABILITIES
	UPROPERTY(BlueprintReadOnly, Category = "Abilities")
	bool bAbilityTargetingMode = false;
	
	UFUNCTION(BlueprintPure, Category = "Abilities")
	UChessAbilityBase* GetSelectedUnitAbility() const;
	
	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool CanUseSelectedUnitAbility() const;
	
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void OnAbilityButtonPressed();
	
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void EnterAbilityTargetingMode();
	
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ActivateSelectedUnitAbility(FIntPoint TargetPos);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Abilities")
	void OnAbilityTargetingModeChanged(bool bEnabled);
	
	void ClearHighlightedTiles();
	
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void TeleportUnitTo(AChessUnitBase* Unit, FIntPoint TargetPos);

// CHESSBOARD INTERFACES
	virtual bool IsValidGridPos(const FIntPoint& Pos, int Width, int Height) const override;
	virtual bool IsTileOccupied(const FIntPoint& Pos) const override;
	virtual EChessTeam GetTeamAt(const FIntPoint& Pos) const override;

// Editor Tools
	UFUNCTION(CallInEditor, Category = "Grid Visuals")
	void GenerateGrid();
	
	void ClearBoardVisuals();
};
