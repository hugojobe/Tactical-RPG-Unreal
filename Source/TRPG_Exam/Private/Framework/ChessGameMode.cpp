// By hugojobe.


#include "Framework/ChessGameMode.h"
#include "AI/ChessAIController.h"
#include "AI/ChessAIPlanner.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GAS/ChessGameplayTags.h"
#include "Managers/ChessGridManager.h"

AChessGameMode::AChessGameMode() {}

void AChessGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	GridManager = Cast<AChessGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessGridManager::StaticClass()));
	GridManager->GameMode = this;
	GridManager->Initialize();
	GridManager->InitializeZobrist();
	
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	{
		GenerateVirtualBoard();
		GenerateThreatMap();
		GenerateDefenseMap();
		GenerateBalanceMap();
		StartPlayerTurn();
	}, 0.5f, false);
}

void AChessGameMode::GenerateVirtualBoard_Implementation()
{
	TMap<FIntPoint, FVirtualUnit> VUnitMap;
	for (auto TileTuple : GridManager->TileActors)
	{
		if (TileTuple.Value != nullptr)
		{
			AChessUnitBase* Unit = GridManager->GetUnitAt(TileTuple.Key);
			
			if (Unit == nullptr) continue;
			
			FVirtualUnit VUnit = AChessGridManager::MakeVirtualUnit(Unit);
			VUnitMap.Add(TileTuple.Key, VUnit);
		}
	}
	
	VirtualBoard = UChessAIPlanner::GenerateVirtualBoard(VUnitMap, GridManager->BoardWidth, GridManager->BoardHeight);
	
	if (bDisplayVirtualBoard)
	{
		DisplayVirtualBoard();
		DisplayVirtualBoard();
	}
}

void AChessGameMode::GenerateThreatMap_Implementation()
{
	ThreatMap.Empty();
	ThreatMap = UChessAIPlanner::GenerateThreatMap(VirtualBoard);
	
	if (bDisplayThreatMap)
	{
		DisplayThreatMap();
		DisplayThreatMap();
	}
}

void AChessGameMode::GenerateDefenseMap_Implementation()
{
	DefenseMap.Empty();
	DefenseMap = UChessAIPlanner::GenerateDefenseMap(VirtualBoard);
	
	if (bDisplayDefenseMap)
	{
		DisplayDefenseMap();
		DisplayDefenseMap();
	}
}

void AChessGameMode::GenerateBalanceMap_Implementation()
{
	BalanceMap.Empty();
	TArray<FIntPoint> Tiles;
	GridManager->TileActors.GenerateKeyArray(Tiles);
	BalanceMap = UChessAIPlanner::GenerateBalanceMap(Tiles, ThreatMap, DefenseMap);
	
	if (bDisplayBalanceMap)
	{
		DisplayBalanceMap();
		DisplayBalanceMap();
	}
}

void AChessGameMode::PlayerActionFinished_Implementation(AChessGridManager* Manager)
{
	Manager->CurrentTurnActionState = CurrentTurn;

	CheckWinCondition();
	
	if(CurrentTurn == ETurnState::Player)
		EndPlayerTurn();
	else
		EndEnemyTurn();
}

void AChessGameMode::EndPlayerTurn_Implementation()
{
	if (CurrentTurn == ETurnState::None) return;
	
	SetCurrentTurn(ETurnState::Enemy);
	GridManager->CurrentTurnActionState = ETurnState::None;
	
	GenerateVirtualBoard();
	GenerateThreatMap();
	GenerateDefenseMap();
	GenerateBalanceMap();
	
	StartEnemyTurn();
}

void AChessGameMode::EndEnemyTurn_Implementation()
{
	if (CurrentTurn == ETurnState::None) return;
	
	SetCurrentTurn(ETurnState::Player);
	GridManager->CurrentTurnActionState = ETurnState::None;
	
	TickAbilityCooldowns(EChessTeam::Player);
	
	GenerateVirtualBoard();
	
	StartPlayerTurn();
}


void AChessGameMode::StartEnemyTurn_Implementation()
{   
	SetCurrentTurn(ETurnState::Enemy);
	GridManager->CurrentTurnActionState = ETurnState::Enemy;
	
	TickAbilityCooldowns(EChessTeam::Enemy);
    
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	{
	   TArray<AActor*> ActorList;
	   UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessUnitBase::StaticClass(), ActorList);
       
	   TArray<AChessUnitBase*> EnemyUnitsThinking;
	   for (AActor* Actor : ActorList)
	   {
		  AChessUnitBase* Unit = Cast<AChessUnitBase>(Actor);
		  if (Unit && Unit->Attributes->GetHealth() > 0 && Unit->Team == EChessTeam::Enemy && Unit->GetController())
		  {
			 EnemyUnitsThinking.Add(Unit);
		  }
	   }
       
	   Async(EAsyncExecution::Thread, [this, EnemyUnitsThinking]()
	   {
		   FChessDecision BestDecision;
		   AChessUnitBase* BestUnit = nullptr;
           
		   ComputeGlobalAI(BestDecision, BestUnit, EnemyUnitsThinking);

		   Async(EAsyncExecution::TaskGraphMainThread, [this, BestDecision, BestUnit]()
		   {
			  if(BestUnit && BestUnit->GetController())
			  {
				 AChessAIController* ChessAIC = Cast<AChessAIController>(BestUnit->GetController());
				 if (ChessAIC)
				 {
					ChessAIC->PerformTask(BestDecision);
				 }
			  }
		   });
	   });
       
	}, 0.5f, false);
}

void AChessGameMode::ComputeGlobalAI_Implementation(FChessDecision& BestTask, AChessUnitBase*& OutBestUnit, const TArray<AChessUnitBase*>& EnemyUnitsThinking)
{
    FChessDecision BestDecision;
    BestDecision.Type = EChessDecisionType::Null;
    BestDecision.Score = -BIG_NUMBER;
    
    AChessUnitBase* BestUnit = nullptr;
    
    if (!GridManager) return;

    FCriticalSection CriticalSection;

    TArray<FIntPoint> Tiles;
    GridManager->TileActors.GenerateKeyArray(Tiles);
	
	int32 TotalPositions = 0;
	int32 MaxAchievedDepth = 0;
	int32 TotalDepthSum = 0;

    ParallelFor(EnemyUnitsThinking.Num(), [&](int32 Index)
    {
    	AChessUnitBase* Unit = EnemyUnitsThinking[Index];
    	AChessAIController* AIC = Cast<AChessAIController>(Unit->GetController());
    	if (AIC && AIC->AIPlanner)
    	{
    		FChessDecision UnitDecision = AIC->AIPlanner->GetUnitDecision(150, Tiles, VirtualBoard);
    		
    		FScopeLock Lock(&CriticalSection);
    		
    		TotalPositions += AIC->AIPlanner->PositionsEvaluated;
    		int32 UnitDepth = AIC->AIPlanner->LastCompletedDepth;
			MaxAchievedDepth = FMath::Max(MaxAchievedDepth, UnitDepth);
			TotalDepthSum += UnitDepth;
    		
    		if (UnitDecision.Type != EChessDecisionType::Null)
    		{
    			if (UnitDecision.Score > BestDecision.Score)
    			{
    				BestDecision = UnitDecision;
    				BestUnit = Unit;
    			} else if (BestUnit == nullptr){
    				BestDecision = UnitDecision;
    				BestUnit = Unit;
    			}
    		}
    	}
    });
    
    if (BestUnit == nullptr)
    {
    	GEngine->AddOnScreenDebugMessage(-1, 7.0f, FColor::Cyan, TEXT("No best unit found"));
    	return;
    }

    BestTask = BestDecision;
    OutBestUnit = BestUnit;
	
	if (GEngine)
	{
		float AverageDepth = EnemyUnitsThinking.Num() > 0 ? static_cast<float>(TotalDepthSum) / EnemyUnitsThinking.Num() : 0.0f;

		FString StatsMessage = FString::Printf(TEXT("IA Terminé | Noeuds: %d | Profondeur Max: %d | Moyenne: %.1f | Best Task: %d"), 
			TotalPositions, MaxAchievedDepth, AverageDepth, BestDecision.Type);
			
		GEngine->AddOnScreenDebugMessage(-1, 7.0f, FColor::Cyan, StatsMessage);
	}
}

void AChessGameMode::SetCurrentTurn_Implementation(ETurnState NewTurnState)
{
	CurrentTurn = NewTurnState;
}

void AChessGameMode::TickAbilityCooldowns(EChessTeam Team)
{
	TArray<AActor*> Units;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessUnitBase::StaticClass(), Units);

	UE_LOG(LogTemp, Warning, TEXT("[HJ] TickAbilityCooldowns for team %d"), static_cast<int32>(Team));
	
	for (AActor* Actor : Units)
	{
		AChessUnitBase* Unit = Cast<AChessUnitBase>(Actor);
		if (!Unit || Unit->Team != Team || !Unit->AbilitySystemComponent) continue;
		if (Unit->Attributes->GetHealth() <= 0) continue;
		
		FGameplayTagContainer TickTags;
		TickTags.AddTag(TAG_Ability_Cooldown);
		TickTags.AddTag(TAG_Ability_Rooted);
		
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(TickTags);
	
		TArray<FActiveGameplayEffectHandle> Handles = Unit->AbilitySystemComponent->GetActiveEffects(Query);
		for (const FActiveGameplayEffectHandle& EffectHandle : Handles)
		{
			UE_LOG(LogTemp, Warning, TEXT("[HJ] Removing 1 stack on %s (current stacks: %d)"),
				*Unit->GetName(), Unit->AbilitySystemComponent->GetCurrentStackCount(EffectHandle));
			
			Unit->AbilitySystemComponent->RemoveActiveGameplayEffect(EffectHandle, 1);
		}
	}
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
			bool IsOccupied = VirtualBoard.IsTileOccupied(Tile.Key);
			
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
				FColor(FMath::Lerp(0.0f, 200.0f, Alpha), 0, FMath::Lerp(150.0f, 0.0f, Alpha), 220),
				true,
				2.0f,
				1
			);
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

void AChessGameMode::DisplayDefenseMap()
{
	if (!bDisplayDefenseMap)
	{
		if (!GridManager || DefenseMap.Num() == 0) return;
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Displaying Defense Map"));
		}
		
		FVector HalfExtent = FVector(100 / 2.0f, 100 / 2.0f, 1.0f);
		
		float MinDefense = 0; 
		float MaxDefense = 0;
		
		for (const auto& [Coord, TileActor] : GridManager->TileActors)
		{
			float DefenseValue = DefenseMap.FindRef(Coord);
			MinDefense = FMath::Min(MinDefense, DefenseValue);
			MaxDefense = FMath::Max(MaxDefense, DefenseValue);
		}
		
		for (auto Tile : GridManager->TileActors)
		{
			float DefenseValue = DefenseMap.FindRef(Tile.Key);
			float Alpha = (MaxDefense > MinDefense) ? FMath::GetRangePct(MinDefense, MaxDefense, DefenseValue) : 0.0f;		
			
			FVector TileLocation = Tile.Value->GetActorLocation() + FVector(0, 0, 11);
			
			DrawDebugSolidBox(
				GetWorld(),
				TileLocation,
				HalfExtent,
				FColor(0, FMath::Lerp(0.0f, 200.0f, Alpha), FMath::Lerp(150.0f, 0.0f, Alpha), 220),
				true,
				2.0f,
				2
			);
		}
		
		bDisplayDefenseMap = true;
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Hiding Defense Map"));
		}
		
		FlushPersistentDebugLines(GetWorld());
		bDisplayDefenseMap = false;
	}
}

void AChessGameMode::DisplayBalanceMap()
{
	if (!bDisplayBalanceMap)
	{
		if (!GridManager || DefenseMap.Num() == 0) return;
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Displaying Balance Map"));
		}
		
		FVector HalfExtent = FVector(100 / 2.0f, 100 / 2.0f, 1.0f);
		
		float MinBalance = 0; 
		float MaxBalance = 0;
		
		for (const auto& [Coord, TileActor] : GridManager->TileActors)
		{
			float Balance = BalanceMap.FindRef(Coord);
			MinBalance = FMath::Min(MinBalance, Balance);
			MaxBalance = FMath::Max(MaxBalance, Balance);
		}
		
		for (auto Tile : GridManager->TileActors)
		{
			if (BalanceMap.Find(Tile.Key) == nullptr) continue;
			float BalanceValue = BalanceMap.FindRef(Tile.Key);
			float Alpha = (MaxBalance > MinBalance) ? FMath::GetRangePct(MinBalance, MaxBalance, BalanceValue) : 0.0f;
			
			FVector TileLocation = Tile.Value->GetActorLocation() + FVector(0, 0, 11);
			
			DrawDebugSolidBox(
				GetWorld(),
				TileLocation,
				HalfExtent,
				FColor(FMath::Lerp(0.0f, 200.0f, Alpha), FMath::Lerp(200.0f, 0.0f, Alpha), 0.0f, 220),
				true,
				2.0f,
				2
			);
		}
		
		bDisplayBalanceMap = true;
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Hiding Balance Map"));
		}
		
		FlushPersistentDebugLines(GetWorld());
		bDisplayBalanceMap = false;
	}
}

void AChessGameMode::StartPlayerTurn_Implementation()
{
	if (CurrentTurn == ETurnState::None) return;
	
	SetCurrentTurn(ETurnState::Player);
	GridManager->CurrentTurnActionState = ETurnState::Player;
}

bool AChessGameMode::CheckWinCondition_Implementation(bool TriggerGameOver)
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
	
	if (EnemyCount <= 0)
	{
		if (TriggerGameOver)
		{
			OnGameOver(EChessTeam::Player, EGameOverCause::NoPiecesRemaining);
		}
		return true;
	}
	if (PlayerCount <= 0)
	{
		if (TriggerGameOver)
		{
			OnGameOver(EChessTeam::Enemy, EGameOverCause::NoPiecesRemaining);
		}
		return true;
	}
	
	if (!bEnemyKingAlive && bEnemyKingExists)
	{
		if (TriggerGameOver)
		{
			OnGameOver(EChessTeam::Player, EGameOverCause::KingKilled);
		}
		return true;
	}
	if (!bPlayerKingAlive && bPlayerKingExists)
	{
		if (TriggerGameOver)
		{
			OnGameOver(EChessTeam::Enemy, EGameOverCause::KingKilled);
		}
		return true;
	}
	
	EChessTeam TeamToCheck = CurrentTurn == ETurnState::Player ? EChessTeam::Enemy : EChessTeam::Player;
	if (!GridManager->CanTeamMove(TeamToCheck, VirtualBoard))
	{
		EChessTeam WinningTeam = (TeamToCheck == EChessTeam::Player) ? EChessTeam::Enemy : EChessTeam::Player;
        
		if (TriggerGameOver)
		{
			OnGameOver(WinningTeam, EGameOverCause::CantMove);
		}
		return true;
	}
	
	return false;
}

void AChessGameMode::OnGameOver_Implementation(EChessTeam WinningTeam, EGameOverCause Cause)
{
	SetCurrentTurn(ETurnState::None);
}