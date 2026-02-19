// By hugojobe.


#include "AI/ChessAIController.h"

#include "AI/ChessAIPlanner.h"
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
	AIPlanner->AggressionFactor = Aggressiveness;
	AIPlanner->SafetyFactor = SelfPreservation;
}

/*FHTNTask AChessAIController::GetPlan()
{
	return CurrentPlan;
}*/

void AChessAIController::PerformTask(const FChessDecision& Task)
{
	if (Task.Type == EChessDecisionType::Attack)
	{
		GridManager->AttackUnit(MyPawn, GridManager->GetUnitAt(Task.TargetPosition));
	}
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


