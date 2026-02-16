// By hugojobe.


#include "AI/ChessAIController.h"
#include "Kismet/GameplayStatics.h"

AChessAIController::AChessAIController()
{
	GridManager = Cast<AChessGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessGridManager::StaticClass()));
	
	HTNPlanner = CreateDefaultSubobject<UChessHTNPlanner>(TEXT("HTNPlanner"));
}

void AChessAIController::OnPossess(APawn* InPawn)
{
 	Super::OnPossess(InPawn);
	
	MyPawn = Cast<AChessUnitBase>(GetPawn());
	HTNPlanner->MyUnit = MyPawn;
	HTNPlanner->GridManager = GridManager;
	Aggressiveness = Cast<AChessUnitBase>(InPawn)->Aggressiveness;
	SelfPreservation = Cast<AChessUnitBase>(InPawn)->SelfPreservation;
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
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Executing Task %s"), *Task.TaskName.ToString()));
	
	if (Task.TaskName == "Attack")
	{
		GridManager->AttackUnit(MyPawn, Task.TargetUnit);
	}
	else if (Task.TaskName == "Move")
	{
		GridManager->MoveUnit(MyPawn, Task.TargetTile);
	}
}


