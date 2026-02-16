// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "AI/ChessAITypes.h"
#include "GameFramework/GameModeBase.h"
#include "ChessGameMode.generated.h"

class AChessGridManager;
/**
 * 
 */
UCLASS()
class TRPG_EXAM_API AChessGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AChessGameMode();
	
	virtual void BeginPlay() override;
	
	bool bPlayerKingExists = false;
	bool bEnemyKingExists = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AChessGridManager* GridManager;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
	ETurnState CurrentTurn;
    
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void PlayerActionFinished(AChessGridManager* Manager);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void EndPlayerTurn();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void StartEnemyTurn();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void StartPlayerTurn();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Flow")
	void CheckWinCondition();

	UFUNCTION(BlueprintNativeEvent, Category = "Game Flow")
	void OnGameOver(EChessTeam WinningTeam);
	
	UFUNCTION(BlueprintNativeEvent, Category = "AI")
	void ExecuteAIAction(AChessUnitBase* Unit, const FHTNTask& Task);
	
	UFUNCTION(BlueprintNativeEvent, Category = "AI")
	void ComputeGlobalAI(FHTNTask& BestTask, AChessUnitBase& OutBestUnit);
};
