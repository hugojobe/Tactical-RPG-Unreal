#include "AI/UChessHTNPlanner.h"

FHTNTask UChessHTNPlanner::MakePlan()
{
	

	FWorldState State;
	State.ThreatMap.Init(GridManager->BoardWidth, GridManager->BoardHeight);
	State.BenefitMap.Init(GridManager->BoardWidth, GridManager->BoardHeight);

	LastThreatMap.Init(GridManager->BoardWidth, GridManager->BoardHeight);
	LastBenefitMap.Init(GridManager->BoardWidth, GridManager->BoardHeight);
    
	for (auto& Tile : GridManager->GridMap)
	{
		if (AChessUnitBase* UnitOnTile = Tile.Value)
		{
			if (UnitOnTile->Team == MyUnit->Team) State.MyUnits.Add(UnitOnTile);
			else State.EnemyUnits.Add(UnitOnTile);
		}
	}
    
	BuildInfluenceMaps(State);

	LastThreatMap = State.ThreatMap;
	LastBenefitMap = State.BenefitMap;
    
	FHTNTask BestPlan;
	if (Method_WinGame(State, BestPlan))
	{
		return BestPlan;
	}

	return FHTNTask();
}

void UChessHTNPlanner::BuildInfluenceMaps(FWorldState& State)
{
	for (AChessUnitBase* Enemy : State.EnemyUnits)
	{
		TMap<FIntPoint, ETileInteraction> Moves = GridManager->GetValidMoves(Enemy, true);
		float ThreatValue = Enemy->Attributes->GetDamage();

		for (auto& Move : Moves)
		{
			bool bIsThreat = false;
			if (Enemy->MovementRule.MovementStyle == EMovementStyle::Pawn)
			{
				if (Move.Value == ETileInteraction::Attack)
				{
					bIsThreat = true;
				}
			}
			else
			{
				if (Move.Value == ETileInteraction::Attack || Move.Value == ETileInteraction::Move)
				{
					bIsThreat = true;
				}
			}

			if(bIsThreat)
				State.ThreatMap.AddValue(Move.Key.X, Move.Key.Y, ThreatValue);
		}
	}
	
	float CenterX = GridManager->BoardWidth / 2.0f;
	float CenterY = GridManager->BoardHeight / 2.0f;
    
	for (int x = 0; x < GridManager->BoardWidth; x++)
	{
		for (int y = 0; y < GridManager->BoardHeight; y++)
		{
			float DistToCenter = FMath::Sqrt(FMath::Square(x - CenterX) + FMath::Square(y - CenterY));
			float CenterBonus = FMath::Max(0.0f, 5.0f - DistToCenter);
			State.BenefitMap.AddValue(x, y, CenterBonus);
		}
	}
	
	for (AChessUnitBase* Enemy : State.EnemyUnits)
	{
		FIntPoint Pos(Enemy->GridCoordinates.X, Enemy->GridCoordinates.Y);
		State.BenefitMap.AddValue(Pos.X, Pos.Y, static_cast<float>(Enemy->Stats.KillRpReward));
        
		if (Enemy->UnitType == EChessUnitType::King)
		{
			State.BenefitMap.AddValue(Pos.X, Pos.Y, 1000.0f);
		}
	}
}

bool UChessHTNPlanner::Method_WinGame(const FWorldState& State, FHTNTask& OutTask)
{
	if (Method_TryKillKing(State, OutTask)) return true;

	if (Method_FindBestTrade(State, OutTask)) return true;

	if (Method_PlaySafe(State, OutTask)) return true;

	return false;
}

bool UChessHTNPlanner::Method_TryKillKing(const FWorldState& State, FHTNTask& OutTask)
{
	FHTNTask Task = FindBestAttack(State, true);
	if (Task.IsValid())
	{
		OutTask = Task;
		return true;
	}
	return false;
}

bool UChessHTNPlanner::Method_FindBestTrade(const FWorldState& State, FHTNTask& OutTask)
{
	FHTNTask Task = FindBestAttack(State, false);
    
	if (Task.IsValid() && Task.Score > 0.0f) 
	{
		OutTask = Task;
		return true;
	}
	return false;
}

bool UChessHTNPlanner::Method_PlaySafe(const FWorldState& State, FHTNTask& OutTask)
{
	FHTNTask Task = FindSafeMove(State);
	if (Task.IsValid())
	{
		OutTask = Task;
		return true;
	}

	Task = FindAnyMove(State);
	if(Task.IsValid())
	{
		OutTask = Task;
		return true;
	}

	return false;
}

FHTNTask UChessHTNPlanner::FindBestAttack(const FWorldState& State, bool bPrioritizeKing)
{
	FHTNTask BestTask;
	BestTask.TaskName = FName("None");
	BestTask.Score = -99999.0f;
	BestTask.Type = EHTNTaskType::Action;
	
	if (!MyUnit || !GridManager) return BestTask;

	TMap<FIntPoint, ETileInteraction> PotentialMoves = GridManager->GetValidMoves(MyUnit);
	if (PotentialMoves.IsEmpty()) return BestTask;
	
	for (const TPair<FIntPoint, ETileInteraction>& Move : PotentialMoves)
    {
        if (Move.Value != ETileInteraction::Attack) continue;

        AChessUnitBase* Target = GridManager->GetUnitAt(Move.Key);
        if (!Target) continue;
		
        if (bPrioritizeKing && Target->UnitType != EChessUnitType::King)
            continue; 
		
        int32 MyDamage = MyUnit->Attributes->GetDamage();
        int32 TargetHP = Target->Attributes->GetHealth();
        int32 DeltaL = TargetHP - MyDamage;
        bool bIsLethal = (DeltaL <= 0);
		
        float Score = 0.0f;
		
        float TargetValue = static_cast<float>(Target->Stats.KillRpReward);
        float MyValue = static_cast<float>(MyUnit->Stats.KillRpReward);
		
		TargetValue *= AggressionFactor;

		int32 DestX, DestY;
		
        if (bIsLethal)
        {
        	DestX = Move.Key.X;
        	DestY = Move.Key.Y;
        	
            Score += TargetValue;
        	
        	if (Target->UnitType == EChessUnitType::King)
        		Score += 10000.0f;
        	else
        		Score += 50.0f; 
        	
        	float ThreatAtDest = State.ThreatMap.GetValue(DestX, DestY);
        	float BenefitAtDest = State.BenefitMap.GetValue(DestX, DestY);
        	
        	Score += BenefitAtDest;
        	
        	float EffectiveThreat = ThreatAtDest * SafetyFactor;
            if (EffectiveThreat > 0)
            {
            	float TradeNetValue = TargetValue - (MyValue * 1.2f);
            	Score -= (EffectiveThreat * 0.5f);
            }
        }
        else
        {
        	DestX = MyUnit->GridCoordinates.X;
        	DestY = MyUnit->GridCoordinates.Y;
        	
        	float DamageScore = static_cast<float>(MyDamage) * 10.0f;
        	Score += DamageScore * AggressionFactor; 

        	float ThreatAtOrigin = State.ThreatMap.GetValue(DestX, DestY);
        	Score -= (ThreatAtOrigin * SafetyFactor);
        }

        if (Score > BestTask.Score)
        {
            BestTask.TaskName = FName("Attack");
            BestTask.Type = EHTNTaskType::Action;
            BestTask.TargetUnit = Target;
            BestTask.TargetTile = Move.Key;
            BestTask.Score = Score;
        }
    }

    return BestTask;
}

FHTNTask UChessHTNPlanner::FindSafeMove(const FWorldState& State)
{
	FHTNTask BestTask;
	BestTask.TaskName = FName("None");
	BestTask.Score = -99999.0f;
	BestTask.Type = EHTNTaskType::Action;

	if (!GridManager || !MyUnit) return BestTask;

	int32 StartX = MyUnit->GridCoordinates.X;
	int32 StartY = MyUnit->GridCoordinates.Y;

	float CurrentThreat = State.ThreatMap.GetValue(StartX, StartY);
	float CurrentBenefit = State.BenefitMap.GetValue(StartX, StartY);
	float CurrentScore = CurrentBenefit - (CurrentThreat * 2.0f * SafetyFactor);

	TMap<FIntPoint, ETileInteraction> Moves = GridManager->GetValidMoves(MyUnit);

	for (const TPair<FIntPoint, ETileInteraction>& Move : Moves)
	{
		if (Move.Value != ETileInteraction::Move) continue;

		int32 DestX = Move.Key.X;
		int32 DestY = Move.Key.Y;

		float ThreatAtDest = State.ThreatMap.GetValue(DestX, DestY);
		float BenefitAtDest = State.BenefitMap.GetValue(DestX, DestY);

		float NetScore = BenefitAtDest - (ThreatAtDest * 2.0f * SafetyFactor);
        
		if (NetScore > (CurrentScore + 0.5f))
		{
			if (NetScore > BestTask.Score)
			{
				BestTask.TaskName = FName("Move");
				BestTask.Type = EHTNTaskType::Action;
				BestTask.TargetUnit = nullptr;
				BestTask.TargetTile = Move.Key;
				BestTask.Score = NetScore;
			}
		}
	}

	return BestTask;
}

FHTNTask UChessHTNPlanner::FindAnyMove(const FWorldState& State)
{
	FHTNTask BestTask;
	BestTask.TaskName = FName("None");
	BestTask.Score = -99999.0f;
	BestTask.Type = EHTNTaskType::Action;

	if (!GridManager || !MyUnit) return BestTask;

	TMap<FIntPoint, ETileInteraction> Moves = GridManager->GetValidMoves(MyUnit);

	for (const TPair<FIntPoint, ETileInteraction>& Move : Moves)
	{
		if (Move.Value != ETileInteraction::Move) continue;
        
		float ThreatAtDest = State.ThreatMap.GetValue(Move.Key.X, Move.Key.Y);
		float BenefitAtDest = State.BenefitMap.GetValue(Move.Key.X, Move.Key.Y);
        
		float NetScore = BenefitAtDest - ThreatAtDest;
        
		NetScore += 10.0f; 

		if (NetScore > BestTask.Score)
		{
			BestTask.TaskName = FName("Move");
			BestTask.Type = EHTNTaskType::Action;
			BestTask.TargetUnit = nullptr;
			BestTask.TargetTile = Move.Key;
			BestTask.Score = NetScore;
		}
	}
    
	return BestTask;
}

void UChessHTNPlanner::DrawDebugVisualization(UWorld* World, AChessGridManager* Grid, bool bDrawThreats, bool bDrawBenefits)
{
	if (!World || !Grid) return;

	FlushPersistentDebugLines(World);

	float TileSize = Grid->TileSize;
	float ZOffset = 25.0f;
    
    float MaxThreat = 1.0f;
    float MaxBenefit = 1.0f;

    for (float Val : LastThreatMap.GridValues)
    {
        if (Val > MaxThreat) MaxThreat = Val;
    }
    for (float Val : LastBenefitMap.GridValues)
    {
        if (Val > MaxBenefit) MaxBenefit = Val;
    }

	for (int32 x = 0; x < Grid->BoardWidth; x++)
	{
		for (int32 y = 0; y < Grid->BoardHeight; y++)
		{
			FIntPoint GridPos(x, y);
			FVector TileCenter = Grid->GridToWorld(GridPos);
			TileCenter.Z += ZOffset;

			if (bDrawThreats)
			{
				float ThreatVal = LastThreatMap.GetValue(x, y);
				if (ThreatVal > 0)
				{
                    float Scale = ThreatVal / MaxThreat;
                    
                    float BoxSize = (TileSize * 0.1f) + (TileSize * 0.35f * Scale);
                    
					FVector Extent(BoxSize, BoxSize, 10.0f);
					FColor Color = FColor::Red;
                    Color = FLinearColor::LerpUsingHSV(FLinearColor::Yellow, FLinearColor::Red, Scale).ToFColor(true);

					DrawDebugBox(World, TileCenter, Extent, Color, true, -1.0f, 0, 2.0f);
					DrawDebugString(World, TileCenter + FVector(0,0,50), FString::Printf(TEXT("%.0f"), ThreatVal), nullptr, FColor::White, 0.0f, true);
				}
			}

			if (bDrawBenefits)
			{
				float BenefitVal = LastBenefitMap.GetValue(x, y);
				if (BenefitVal > 0)
				{
                    float Scale = BenefitVal / MaxBenefit;
                    float BoxSize = (TileSize * 0.1f) + (TileSize * 0.35f * Scale);

					FVector GreenCenter = TileCenter + FVector(15, 15, 20);
					FVector Extent(BoxSize, BoxSize, 10.0f);
					FColor Color = FColor::Green;
                    
                    Color = FLinearColor::LerpUsingHSV(FLinearColor::Blue, FLinearColor::Green, Scale).ToFColor(true);

					DrawDebugBox(World, GreenCenter, Extent, Color, true, -1.0f, 0, 2.0f);
					DrawDebugString(World, GreenCenter + FVector(0,0,50), FString::Printf(TEXT("%.0f"), BenefitVal), nullptr, FColor::White, 0.0f, true);
				}
			}
		}
	}
}
