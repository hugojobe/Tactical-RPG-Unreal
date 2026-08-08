// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ChessAIPlanner.h"
#include "ChessAIController.generated.h"

class AChessUnitBase;

/**
 * 
 */

UCLASS()
class TRPG_EXAM_API AChessAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AChessAIController();

	UPROPERTY(VisibleAnywhere)
	UChessAIPlanner* AIPlanner;
	
	AChessGridManager* GridManager;
	AChessUnitBase* MyPawn;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Personality")
	float Aggressiveness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Personality")
	float SelfPreservation = 1.0f; // High means high ELO

	void PerformTask(const FChessDecision& Task);

	virtual void OnPossess(APawn* InPawn) override;
};
