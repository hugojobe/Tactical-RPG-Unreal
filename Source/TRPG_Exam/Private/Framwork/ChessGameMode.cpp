// By hugojobe.


#include "Framwork/ChessGameMode.h"
#include "AI/ChessAIController.h"
#include "AI/ChessAIPlanner.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Managers/ChessGridManager.h"

AChessGameMode::AChessGameMode() {}

void AChessGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	GridManager = Cast<AChessGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessGridManager::StaticClass()));
	GridManager->GameMode = this;
	GridManager->Initialize();
	
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	{
		GenerateVirtualBoard();
		GenerateThreatMap();
		StartPlayerTurn();
	}, 0.5f, false);
}

void AChessGameMode::GenerateVirtualBoard_Implementation()
{
	VirtualBoard = nullptr;
	VirtualBoard = UChessAIPlanner::GenerateVirtualBoard(GridManager);
	
	if (bDisplayVirtualBoard)
	{
		DisplayVirtualBoard();
		DisplayVirtualBoard();
	}
}

void AChessGameMode::GenerateThreatMap_Implementation()
{
	ThreatMap.Empty();
	ThreatMap = UChessAIPlanner::GenerateThreatMap(GridManager);
	
	if (bDisplayThreatMap)
	{
		DisplayThreatMap();
		DisplayThreatMap();
	}
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
	
	GenerateVirtualBoard();
	GenerateThreatMap();
	
	//StartEnemyTurn();
	StartPlayerTurn();
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
		FChessDecision BestDecision;
		AChessUnitBase* BestUnit = nullptr;
		ComputeGlobalAI(BestDecision, BestUnit);

		if(!BestUnit)
			return;

		UE_LOG(LogTemp, Warning, TEXT("[AI] Best task for this turn with score %f by unit %s"), BestDecision.Score, *BestUnit->GetName());

		AChessAIController* ChessAIC = Cast<AChessAIController>(BestUnit->GetController());
		if (!ChessAIC)
		{
			return;
		}
		
		FTimerHandle EndTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(EndTimerHandle, [this, ChessAIC, BestDecision]()
		{	
			ChessAIC->PerformTask(BestDecision);
		}, 1.0f, false);
	}, 0.5f, false);
}

void AChessGameMode::ComputeGlobalAI_Implementation(FChessDecision& BestTask, AChessUnitBase*& OutBestUnit)
{
	FChessDecision BestDecision;
	BestDecision.Type = EChessDecisionType::Null;
	BestDecision.Score = -BIG_NUMBER;
	
	AChessUnitBase* BestUnit = nullptr;
	
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

	FCriticalSection CriticalSection;

	ParallelFor(EnemyUnitsThinking.Num(), [&](int32 Index)
	{
		AChessUnitBase* Unit = EnemyUnitsThinking[Index];
		AChessAIController* AIC = Cast<AChessAIController>(Unit->GetController());
        
		if (AIC && AIC->AIPlanner)
		{
			FChessDecision UnitDecision = AIC->AIPlanner->GetUnitDecision();

			FScopeLock Lock(&CriticalSection);
			if (UnitDecision.Score > BestDecision.Score)
			{
				BestDecision = UnitDecision;
				BestUnit = Unit;
			}
		}
	});

	BestTask = BestDecision;
	OutBestUnit = BestUnit;

	UE_LOG(LogTemp, Warning, TEXT("[AI] Global: Multithreaded computation finished. Best: %s"), BestUnit ? *BestUnit->GetName() : TEXT("None"));
}

void AChessGameMode::DisplayVirtualBoard()
{
	if (!bDisplayVirtualBoard)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Displaying Virtual Board"));
		}
		
		FVector HalfExtent = FVector(100 / 2.0f, 100 / 2.0f, 1.0f);
		
		for (auto Tile : GridManager->TileActors)
		{
			bool IsOccupied = VirtualBoard->Units.Find(Tile.Key) != nullptr;
			
			DrawDebugSolidBox(
				GetWorld(),
				Tile.Value->GetActorLocation() + FVector(0, 0, 5),
				HalfExtent,
				FColor(0, 0, 0, IsOccupied ? 200 : 50),
				true,
				0
			);
		}
		
		bDisplayVirtualBoard = true;
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Hiding Virtual Board"));
		}
		
		FlushPersistentDebugLines(GetWorld());
		bDisplayVirtualBoard = false;
	}
}

void AChessGameMode::DisplayThreatMap()
{
	if (!bDisplayThreatMap)
	{
		if (!GridManager || ThreatMap.Num() == 0) return;
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Displaying Threat Map"));
		}
		
		FVector HalfExtent = FVector(100 / 2.0f, 100 / 2.0f, 1.0f);
		
		float MinThreat = 0; 
		float MaxThreat = 0;
		
		for (const auto& [Coord, TileActor] : GridManager->TileActors)
		{
			float ThreatValue = ThreatMap.FindRef(Coord);
			MinThreat = FMath::Min(MinThreat, ThreatValue);
			MaxThreat = FMath::Max(MaxThreat, ThreatValue);
		}
		
		for (auto Tile : GridManager->TileActors)
		{
			float ThreatValue = ThreatMap.FindRef(Tile.Key);
			float Alpha = (MaxThreat > MinThreat) ? FMath::GetRangePct(MinThreat, MaxThreat, ThreatValue) : 0.0f;		
			
			FVector TileLocation = Tile.Value->GetActorLocation() + FVector(0, 0, 10);
			
			DrawDebugSolidBox(
				GetWorld(),
				TileLocation,
				HalfExtent,
				FColor(FMath::Lerp(0.0f, 200.0f, Alpha), 0, FMath::Lerp(200.0f, 0.0f, Alpha), 220),
				true,
				2.0f,
				1
			);
			
			FString ThreatString = FString::Printf(TEXT("%.0f"), ThreatValue);

			/*DrawDebugString(
				GetWorld(),
				TileLocation + FVector(50, 50, 5),
				ThreatString,
				nullptr, 
				FColor::White, 
				-1,
				true,
				1.2f 
			);*/
		}
		
		bDisplayThreatMap = true;
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Hiding Threat Map"));
		}
		
		FlushPersistentDebugLines(GetWorld());
		bDisplayThreatMap = false;
	}
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