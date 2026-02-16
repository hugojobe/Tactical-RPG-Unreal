// By hugojobe.


#include "AI/ChessAIController.h"
#include "Kismet/GameplayStatics.h"

AChessAIController::AChessAIController()
{
	GridManager = Cast<AChessGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessGridManager::StaticClass()));
	MyPawn = Cast<AChessUnitBase>(GetPawn());
	
	HTNPlanner = CreateDefaultSubobject<UChessHTNPlanner>(TEXT("HTNPlanner"));
	HTNPlanner->MyUnit = MyPawn;
	HTNPlanner->GridManager = GridManager;
	HTNPlanner->AggressionFactor = Aggressiveness;
	HTNPlanner->SafetyFactor = SelfPreservation;
}

FHTNTask AChessAIController::GetPlan()
{
	FHTNTask NullPlan;
	NullPlan.TaskName = FName("None");
	NullPlan.Score = -99999.0f;
	NullPlan.Type = EHTNTaskType::Action;
	
	if (!GridManager || !MyPawn || !HTNPlanner) return NullPlan;
	
	CurrentPlan = HTNPlanner->MakePlan();
	return CurrentPlan;
}

void AChessAIController::PerformTask(const FHTNTask& Task)
{
	if (Task.TaskName == "Attack")
	{
		GridManager->AttackUnit(MyPawn, Task.TargetUnit);
	}
	else if (Task.TaskName == "Move")
	{
		GridManager->MoveUnit(MyPawn, Task.TargetTile);
	}
}
