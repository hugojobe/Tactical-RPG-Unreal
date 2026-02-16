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

	if(CurrentTurn == ETurnState::Player)
		EndPlayerTurn();
	else
		EndEnemyTurn();
}

void AChessGameMode::EndPlayerTurn_Implementation()
{
	if (CurrentTurn == ETurnState::None) return;
	
	CurrentTurn = ETurnState::Enemy;
	GridManager->CurrentTurnActionState = ETurnState::None;
	StartEnemyTurn();
}

void AChessGameMode::EndEnemyTurn_Implementation()
{
	if (CurrentTurn == ETurnState::None) return;
	
	CurrentTurn = ETurnState::Player;
	GridManager->CurrentTurnActionState = ETurnState::None;
	StartPlayerTurn();
}


void AChessGameMode::StartEnemyTurn_Implementation()
{	
	CurrentTurn = ETurnState::Enemy;
	GridManager->CurrentTurnActionState = ETurnState::Enemy;

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	{
		FHTNTask BestTask;
		AChessUnitBase* BestUnit = nullptr;
		ComputeGlobalAI(BestTask, BestUnit);

		if(!BestUnit || !BestTask.IsValid())
		{
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[AI] Best task for this turn: %s with score %f by unit %s"), *BestTask.TaskName.ToString(), BestTask.Score, *BestUnit->GetName());

		AChessAIController* ChessAIC = Cast<AChessAIController>(BestUnit->GetController());
		if (!ChessAIC)
		{
			return;
		}
		
		FTimerHandle EndTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(EndTimerHandle, [this, ChessAIC, BestTask]()
		{	
			ChessAIC->PerformTask(BestTask);
		}, 1.0f, false);
	}, 0.5f, false);
}

void AChessGameMode::ComputeGlobalAI_Implementation(FHTNTask& BestTask, AChessUnitBase*& OutBestUnit)
{
	FHTNTask BestGlobalTask;
	BestGlobalTask.Score = -10000000.0f;
	BestGlobalTask.Type = EHTNTaskType::Action;
	
	AChessUnitBase* BestComputedUnit = nullptr;
	
	if (!GridManager) return;
	
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessUnitBase::StaticClass(), ActorList);
    
	TArray<AChessUnitBase*> AllBoardUnits;
	TArray<AChessUnitBase*> EnemyUnitsThinking;

	for (AActor* Actor : ActorList)
	{
		AChessUnitBase* Unit = Cast<AChessUnitBase>(Actor);
		if (Unit && Unit->Attributes->GetHealth() > 0)
		{
			AllBoardUnits.Add(Unit);
			if (Unit->Team == EChessTeam::Enemy && Unit->GetController())
			{
				EnemyUnitsThinking.Add(Unit);
			}
		}
	}

	UChessHTNPlanner* DummyPlanner = NewObject<UChessHTNPlanner>(); 
	FVirtualBoard GlobalSnapshot = DummyPlanner->CreateVirtualBoard(AllBoardUnits);

	FCriticalSection CriticalSection;

	ParallelFor(EnemyUnitsThinking.Num(), [&](int32 Index)
	{
		AChessUnitBase* Unit = EnemyUnitsThinking[Index];
		AChessAIController* AIC = Cast<AChessAIController>(Unit->GetController());
        
		if (AIC && AIC->HTNPlanner)
		{
			FHTNTask UnitTask = AIC->HTNPlanner->MakePlan(GlobalSnapshot);

			FScopeLock Lock(&CriticalSection);
			if (UnitTask.IsValid() && UnitTask.Score > BestGlobalTask.Score)
			{
				BestGlobalTask = UnitTask;
				BestComputedUnit = Unit;
			}
		}
	});

	BestTask = BestGlobalTask;
	OutBestUnit = BestComputedUnit;

	UE_LOG(LogTemp, Warning, TEXT("[AI] Global: Multithreaded computation finished. Best: %s"), BestComputedUnit ? *BestComputedUnit->GetName() : TEXT("None"));
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
		if (AChessUnitBase* Unit = Cast<AChessUnitBase>(Actor))
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

void AChessGameMode::OnGameOver_Implementation(EChessTeam WinningTeam) {}