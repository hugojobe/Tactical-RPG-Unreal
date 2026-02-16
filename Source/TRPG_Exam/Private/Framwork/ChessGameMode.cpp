// By hugojobe.


#include "Framwork/ChessGameMode.h"

#include "AI/ChessAIController.h"
#include "AI/UChessHTNPlanner.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/ChessGridManager.h"

AChessGameMode::AChessGameMode() {}

void AChessGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	GridManager = Cast<AChessGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessGridManager::StaticClass()));
	
	StartPlayerTurn();
}

void AChessGameMode::PlayerActionFinished_Implementation(AChessGridManager* Manager)
{
	Manager->CurrentTurnActionState = CurrentTurn;
	EndPlayerTurn();
}

void AChessGameMode::EndPlayerTurn_Implementation()
{
	if (CurrentTurn == ETurnState::None) return;
	
	CurrentTurn = ETurnState::Enemy;
	GridManager->CurrentTurnActionState = ETurnState::None;
	StartEnemyTurn();
}

void AChessGameMode::StartEnemyTurn_Implementation()
{	
	CurrentTurn = ETurnState::Enemy;
	GridManager->CurrentTurnActionState = ETurnState::Enemy;
	
	FHTNTask BestTask;
	AChessUnitBase* BestUnit = nullptr;
	ComputeGlobalAI(BestTask, *BestUnit);
}

void AChessGameMode::ComputeGlobalAI_Implementation(FHTNTask& BestTask, AChessUnitBase& OutBestUnit)
{
	FHTNTask BestGlobalTask;
	BestGlobalTask.Score = -999999.0f;
	BestGlobalTask.Type = EHTNTaskType::Action;
	
	AChessUnitBase* BestComputedUnit = nullptr;
	
	if (!GridManager) return;
	
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessUnitBase::StaticClass(), AllUnits);
	
	for (AActor* Actor : AllUnits)
	{
		AChessUnitBase* Unit = Cast<AChessUnitBase>(Actor);
		if (Unit && Unit->Team == EChessTeam::Enemy && !Unit->IsHidden())
		{
			AChessAIController* ChessAIC = Cast<AChessAIController>(Unit->GetController());
			if (!ChessAIC) return;
			
			FWorldState State;
			FHTNTask UnitTask = ChessAIC->HTNPlanner->MakePlan(); 

			if (UnitTask.IsValid() && UnitTask.Score > BestGlobalTask.Score)
			{
				BestGlobalTask = UnitTask;
				BestComputedUnit = Unit;
			}
		}
	}
	
	OutBestUnit = BestComputedUnit;
}

void AChessGameMode::ExecuteAIAction_Implementation(AChessUnitBase* Unit, const FHTNTask& Task)
{
	
}

void AChessGameMode::StartPlayerTurn_Implementation()
{
	if (CurrentTurn == ETurnState::None) return;
	
	CurrentTurn = ETurnState::Player;
	GridManager->CurrentTurnActionState = ETurnState::Player;
}

void AChessGameMode::CheckWinCondition_Implementation()
{
	TArray<AActor*> Units;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessUnitBase::StaticClass(), Units);
	
	bool bPlayerKingAlive = false;
	bool bEnemyKingAlive = false;
	int32 EnemyCount = 0;
	int32 PlayerCount = 0;
	
	for (AActor* Actor : Units)
	{
		AChessUnitBase* Unit = Cast<AChessUnitBase>(Actor);
		if (Unit)
		{
			if (Unit->Team == EChessTeam::Player && Unit->Attributes->GetHealth() > 0)
			{
				PlayerCount++;
				if (Unit->UnitType == EChessUnitType::King) bPlayerKingAlive = true;
			}
			else if (Unit->Attributes->GetHealth() > 0)
			{
				EnemyCount++;
				if (Unit->UnitType == EChessUnitType::King) bEnemyKingAlive = true;
			}
		}
	}
	
	if (!bEnemyKingAlive && bEnemyKingExists)
	{
		CurrentTurn = ETurnState::None;
		OnGameOver(EChessTeam::Player);
		return;
	}
	if (!bPlayerKingAlive && bPlayerKingExists)
	{
		CurrentTurn = ETurnState::None;
		OnGameOver(EChessTeam::Enemy);
		return;
	}

	if (EnemyCount == 0)
	{
		CurrentTurn = ETurnState::None;
		OnGameOver(EChessTeam::Player);
		return;
	}
	if (PlayerCount == 0)
	{
		CurrentTurn = ETurnState::None;
		OnGameOver(EChessTeam::Enemy);
	}
}

void AChessGameMode::OnGameOver_Implementation(EChessTeam WinningTeam)
{
}


