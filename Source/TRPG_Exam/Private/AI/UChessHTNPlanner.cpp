#include "AI/UChessHTNPlanner.h"

FHTNTask UChessHTNPlanner::MakePlan()
{
	if (!GridManager) return FHTNTask();

	FWorldState State;
	State.ThreatMap.Init(GridManager->BoardWidth, GridManager->BoardHeight);
	State.BenefitMap.Init(GridManager->BoardWidth, GridManager->BoardHeight);
    
	for (auto& Tile : GridManager->GridMap)
	{
		if (AChessUnitBase* UnitOnTile = Tile.Value)
		{
			State.EnemyUnits.Empty();
			State.MyUnits.Empty();
			if (UnitOnTile->Team == MyUnit->Team) State.MyUnits.Add(UnitOnTile);
			else State.EnemyUnits.Add(UnitOnTile);
		}
	}
    
	BuildInfluenceMaps(State);
    
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
		TMap<FIntPoint, ETileInteraction> Moves = GridManager->GetValidMoves(Enemy);
		float ThreatValue = Enemy->Attributes->GetDamage();

		for (auto& Move : Moves)
		{
			if (Move.Value == ETileInteraction::Attack || Move.Value == ETileInteraction::Move)
			{
				State.ThreatMap.AddValue(Move.Key.X, Move.Key.Y, ThreatValue);
			}
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
    
	if (Task.IsValid() && Task.Score > 10.0f) 
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
    BestTask.Score = -9999.0f;
	BestTask.Type = EHTNTaskType::Action;

    if (!GridManager) return BestTask;

    for (AChessUnitBase* Me : State.MyUnits)
    {
        int32 StartX = Me->GridCoordinates.X;
        int32 StartY = Me->GridCoordinates.Y;
        
        float CurrentThreat = State.ThreatMap.GetValue(StartX, StartY);
        float CurrentBenefit = State.BenefitMap.GetValue(StartX, StartY);
    	float CurrentScore = CurrentBenefit - (CurrentThreat * 2.0f * SafetyFactor);
    	
        TMap<FIntPoint, ETileInteraction> Moves = GridManager->GetValidMoves(Me);

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
    }

    return BestTask;
}

FHTNTask UChessHTNPlanner::FindAnyMove(const FWorldState& State)
{
	FHTNTask BestTask;
	BestTask.TaskName = FName("None");
	BestTask.Score = -99999.0f;
	BestTask.Type = EHTNTaskType::Action;

	if (!GridManager) return BestTask;

	for (AChessUnitBase* Me : State.MyUnits)
	{
		TMap<FIntPoint, ETileInteraction> Moves = GridManager->GetValidMoves(Me);

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
	}
	
	return BestTask;
}
