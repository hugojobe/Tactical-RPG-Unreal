// By hugojobe.


#include "AI/ChessAIController.h"

#include "AI/ChessAIPlanner.h"
#include "Framework/ChessGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/ChessGridManager.h"

AChessAIController::AChessAIController()
{
	GridManager = Cast<AChessGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessGridManager::StaticClass()));
	
	AIPlanner = CreateDefaultSubobject<UChessAIPlanner>(TEXT("AI Planner"));
}

void AChessAIController::OnPossess(APawn* InPawn)
{
 	Super::OnPossess(InPawn);
	
	MyPawn = Cast<AChessUnitBase>(GetPawn());
	AIPlanner->MyUnit = MyPawn;
	AIPlanner->GridManager = GridManager;
	Aggressiveness = Cast<AChessUnitBase>(InPawn)->Aggressiveness;
	SelfPreservation = Cast<AChessUnitBase>(InPawn)->SelfPreservation;
}

void AChessAIController::PerformTask(const FChessDecision& Task)
{
	if (Task.Type == EChessDecisionType::Null)
	{
		GEngine->AddOnScreenDebugMessage(-1, 7.0f, FColor::Red, TEXT("No decision available, skipping turn"));
		GridManager->GameMode->EndEnemyTurn();
		return;
	}
	
	if (Task.Type == EChessDecisionType::Attack)
	{
		AChessUnitBase* Target = GridManager->GetUnitAt(Task.TargetPosition);
		if (!Target)
		{
			GridManager->MoveUnit(MyPawn, Task.TargetPosition);
			return;
		}
		GridManager->AttackUnit(MyPawn, Target);	}
	else if (Task.Type == EChessDecisionType::Move)
	{
		GridManager->MoveUnit(MyPawn, Task.TargetPosition);
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("Unknown decision"));
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Unknown decision"));
	}
}


