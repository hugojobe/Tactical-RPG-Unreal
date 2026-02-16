#include "AI/UChessHTNPlanner.h"

#include "Kismet/GameplayStatics.h"

class VirtualBoardWrapper : public IChessBoardInterface
{
public:
	const FVirtualBoard* VBoard;
	int32 Width, Height;

	VirtualBoardWrapper(const FVirtualBoard* InBoard, int32 W, int32 H) : VBoard(InBoard), Width(W), Height(H) {}

	virtual bool IsValidGridPos(const FIntPoint& Pos) const override
	{
		return Pos.X >= 0 && Pos.X < Width && Pos.Y >= 0 && Pos.Y < Height;
	}

	virtual bool IsTileOccupied(const FIntPoint& Pos) const override
	{
		int32 UnitIdx = VBoard->GetUnitIndexAt(Pos);
		return UnitIdx != -1 && VBoard->Units[UnitIdx].IsAlive();
	}

	virtual EChessTeam GetTeamAt(const FIntPoint& Pos) const override
	{
		int32 UnitIdx = VBoard->GetUnitIndexAt(Pos);
		if (UnitIdx != -1 && VBoard->Units[UnitIdx].IsAlive())
		{
			return VBoard->Units[UnitIdx].Team;
		}
		return EChessTeam::None;
	}
};

FHTNTask UChessHTNPlanner::MakePlan(const FVirtualBoard& InitialBoard)
{
		if (!GridManager || !MyUnit) return FHTNTask();

	int32 SearchDepth = FMath::Clamp(FMath::RoundToInt(SafetyFactor * 15.0f), 1, 4);

	int32 MyVirtualID = -1;
	FIntPoint MyPos(MyUnit->GridCoordinates.X, MyUnit->GridCoordinates.Y);
	
	for (int32 i = 0; i < InitialBoard.Units.Num(); ++i)
	{
		if (InitialBoard.Units[i].Pos == MyPos)
		{
			MyVirtualID = i;
			break;
		}
	}

	if (MyVirtualID == -1) return FHTNTask();

	TArray<FVirtualMove> PossibleMoves;
	GetLegalMovesForUnit(InitialBoard, MyVirtualID, PossibleMoves);
	
	FVirtualMove BestMove;
	float BestValue = -999999.0f;
	float Alpha = -999999.0f;
	float Beta = 999999.0f;

	for (const FVirtualMove& Move : PossibleMoves)
	{
		if (bHasLastMove && MyPos == LastMoveDestination && Move.To == LastMoveSource)
		{
			continue;
		}
		
		if (InitialBoard.Units[Move.UnitID].Pos != FIntPoint(MyUnit->GridCoordinates.X, MyUnit->GridCoordinates.Y))
			continue;

		FVirtualBoard NextState = InitialBoard;
		ApplyMove(NextState, Move);

		float MoveValue = Minimax(NextState, SearchDepth - 1, Alpha, Beta, false);
		if (MoveValue > BestValue)
		{
			BestValue = MoveValue;
			BestMove = Move;
		}

		Alpha = FMath::Max(Alpha, BestValue);
	}

	FHTNTask FinalTask;
	if (BestValue > -99999.0f)
	{
	    FinalTask.Score = BestValue;

	    AChessUnitBase* RealTarget = GridManager->GetUnitAt(BestMove.To);
	    TMap<FIntPoint, ETileInteraction> RealMoves = GridManager->GetValidMoves(MyUnit, false);
	    ETileInteraction* RealInteraction = RealMoves.Find(BestMove.To);

	    if (BestMove.Type == EVirtualMoveType::Attack)
	    {
	        if (RealTarget)
	        {
	            FinalTask.TaskName = FName("Attack");
	            FinalTask.TargetUnit = RealTarget;
	            FinalTask.TargetTile = BestMove.To;
	        }
	        else if (RealInteraction && *RealInteraction == ETileInteraction::Move)
	        {
	            FinalTask.TaskName = FName("Move");
	            FinalTask.TargetTile = BestMove.To;
	        }
	        else
	        {
	            bool bFound = false;
	            for (const auto& Pair : RealMoves)
	            {
	                if (Pair.Value == ETileInteraction::Attack)
	                {
	                    AChessUnitBase* T = GridManager->GetUnitAt(Pair.Key);
	                    if (T)
	                    {
	                        FinalTask.TaskName = FName("Attack");
	                        FinalTask.TargetUnit = T;
	                        FinalTask.TargetTile = Pair.Key;
	                        FinalTask.Score = BestValue * 0.5f;
	                        bFound = true;
	                        break;
	                    }
	                }
	            }
	            if (!bFound)
	            {
	                for (const auto& Pair : RealMoves)
	                {
	                    if (Pair.Value == ETileInteraction::Move)
	                    {
	                        FinalTask.TaskName = FName("Move");
	                        FinalTask.TargetTile = Pair.Key;
	                        FinalTask.Score = BestValue * 0.1f;
	                        bFound = true;
	                        break;
	                    }
	                }
	            }
	            if (!bFound)
	            {
	                return FHTNTask();
	            }
	        }
	    }
	    else
	    {
	        if (RealInteraction && *RealInteraction == ETileInteraction::Move)
	        {
	            FinalTask.TaskName = FName("Move");
	            FinalTask.TargetTile = BestMove.To;
	        }
	        else
	        {
	            return FHTNTask();
	        }
	    }

	    FinalTask.Type = EHTNTaskType::Action;
	}
    
	return FinalTask;
}

float UChessHTNPlanner::Minimax(FVirtualBoard& Board, int32 Depth, float Alpha, float Beta, bool bMaximizingPlayer)
{
	if (Depth == 0)
	{
		return EvaluateBoard(Board);
	}

	EChessTeam CurrentTeam = bMaximizingPlayer ? MyUnit->Team : (MyUnit->Team == EChessTeam::Player ? EChessTeam::Enemy : EChessTeam::Player);
	
	TArray<FVirtualMove> LegalMoves;
	for (int32 i = 0; i < Board.Units.Num(); ++i)
	{
		if (Board.Units[i].Team == CurrentTeam && Board.Units[i].IsAlive())
		{
			GetLegalMovesForUnit(Board, i, LegalMoves);
		}
	}

	if (LegalMoves.Num() == 0) return EvaluateBoard(Board);
	
	if (LegalMoves.Num() == 0) return EvaluateBoard(Board);

	if (bMaximizingPlayer)
	{
		float MaxEval = -999999.0f;
		for (const FVirtualMove& Move : LegalMoves)
		{
			FVirtualBoard NextState = Board;
			ApplyMove(NextState, Move);
            
			float Eval = Minimax(NextState, Depth - 1, Alpha, Beta, false);
			MaxEval = FMath::Max(MaxEval, Eval);
			Alpha = FMath::Max(Alpha, Eval);
            
			if (Beta <= Alpha) break;
		}
		
		return MaxEval;
	}
	else
	{
		float MinEval = 999999.0f;
		for (const FVirtualMove& Move : LegalMoves)
		{
			FVirtualBoard NextState = Board;
			ApplyMove(NextState, Move);
            
			float Eval = Minimax(NextState, Depth - 1, Alpha, Beta, true);
			MinEval = FMath::Min(MinEval, Eval);
			Beta = FMath::Min(Beta, Eval);
            
			if (Beta <= Alpha) break;
		}
		return MinEval;
	}
}

float UChessHTNPlanner::EvaluateBoard(const FVirtualBoard& Board)
{
	float TotalScore = 0.0f;
	int32 MyKingCount = 0;
	int32 EnemyKingCount = 0;

	TMap<FIntPoint, int32> EnemyAttackCount;
	TMap<FIntPoint, int32> MyDefenseCount;
	
	VirtualBoardWrapper Wrapper(&Board, GridManager->BoardWidth, GridManager->BoardHeight);

	for (const FVirtualUnit& U : Board.Units)
	{
		if (!U.IsAlive()) continue;
		TArray<FVirtualMove> Moves;
		ChessMovementLogic::CalculateLegalMoves(&Wrapper, U.Pos, U.MovementRule, U.Team, Moves, true);
		for (const FVirtualMove& M : Moves)
		{
			if (U.Team == MyUnit->Team) MyDefenseCount.FindOrAdd(M.To)++;
			else EnemyAttackCount.FindOrAdd(M.To)++;
		}
	}

	for (const FVirtualUnit& Unit : Board.Units) {
		if (!Unit.IsAlive()) continue;

		float MaterialValue = (Unit.Type == EChessUnitType::King) ? 30000.0f : static_cast<float>(Unit.KillReward);
		float StatValue = MaterialValue + (Unit.HP * 2.0f) + (Unit.Damage * 5.0f);
        
		int32 Attacks = EnemyAttackCount.FindRef(Unit.Pos);
		int32 Defenders = MyDefenseCount.FindRef(Unit.Pos);
		float CombatMod = 0.0f;

		if (Unit.Team == MyUnit->Team) {
			if (Unit.Type == EChessUnitType::King) MyKingCount++;
			if (Attacks > Defenders) CombatMod -= (StatValue * 0.5f);
            
			if (LastMoveDestination == Unit.Pos) CombatMod -= 10.0f;
		} else {
			if (Unit.Type == EChessUnitType::King) EnemyKingCount++;
			if (Defenders > Attacks) CombatMod += (StatValue * 0.3f);
		}

		float DistToCenter = FVector2D::Distance(FVector2D(Unit.Pos), FVector2D(GridManager->BoardWidth/2.0f, GridManager->BoardHeight/2.0f));
		float PosBonus = FMath::Max(0.0f, 5.0f - DistToCenter) * 0.5f;

		if (Unit.Team == MyUnit->Team) TotalScore += (StatValue + CombatMod + PosBonus);
		else TotalScore -= ((StatValue * AggressionFactor) - CombatMod + PosBonus);
	}

	if (EnemyKingCount == 0) return 999999.0f;
	if (MyKingCount == 0) return -999999.0f;
	return TotalScore;
}

FVirtualBoard UChessHTNPlanner::CreateVirtualBoard()
{
	FVirtualBoard Board;
    
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessUnitBase::StaticClass(), Actors);

	int32 Index = 0;
	for (AActor* ActorUnit : Actors)
	{
		AChessUnitBase* RealUnit = Cast<AChessUnitBase>(ActorUnit);

		if (RealUnit->MovementRule.MoveVectors.Num() > 100) 
		{
			continue;
		}
		
		if (RealUnit && RealUnit->Attributes->GetHealth() > 0 && !RealUnit->IsHidden())
		{
			FVirtualUnit VirtualUnit;
			VirtualUnit.ID = Index;
			VirtualUnit.Team = RealUnit->Team;
			VirtualUnit.Type = RealUnit->UnitType;
			VirtualUnit.HP = static_cast<int32>(RealUnit->Attributes->GetHealth());
			VirtualUnit.Damage = static_cast<int32>(RealUnit->Attributes->GetDamage());
			VirtualUnit.KillReward = RealUnit->Stats.KillRpReward;
			VirtualUnit.MovementRule = RealUnit->MovementRule;
			VirtualUnit.Pos = FIntPoint(RealUnit->GridCoordinates.X, RealUnit->GridCoordinates.Y);

			Board.Units.Add(VirtualUnit);
			Board.Grid.Add(VirtualUnit.Pos, Index);
            
			Index++;
		}
	}
	return Board;
}

FVirtualBoard UChessHTNPlanner::CreateVirtualBoard(const TArray<AChessUnitBase*>& AllUnits)
{
	FVirtualBoard Board;
	int32 Index = 0;

	for (AChessUnitBase* RealUnit : AllUnits)
	{
		if (RealUnit && RealUnit->Attributes->GetHealth() > 0 && !RealUnit->IsHidden())
		{
			FVirtualUnit VirtualUnit;
			VirtualUnit.ID = Index;
			VirtualUnit.Team = RealUnit->Team;
			VirtualUnit.Type = RealUnit->UnitType;
			VirtualUnit.HP = static_cast<int32>(RealUnit->Attributes->GetHealth());
			VirtualUnit.Damage = static_cast<int32>(RealUnit->Attributes->GetDamage());
			VirtualUnit.KillReward = RealUnit->Stats.KillRpReward;
			VirtualUnit.MovementRule = RealUnit->MovementRule;
			VirtualUnit.Pos = FIntPoint(RealUnit->GridCoordinates.X, RealUnit->GridCoordinates.Y);

			Board.Units.Add(VirtualUnit);
			Board.Grid.Add(VirtualUnit.Pos, Index);
			Index++;
		}
	}
	return Board;
}

void UChessHTNPlanner::GetLegalMovesForUnit(const FVirtualBoard& Board, int32 UnitID, TArray<FVirtualMove>& OutMoves)
{
	if (!Board.Units.IsValidIndex(UnitID) || !Board.Units[UnitID].IsAlive()) return;

	const FVirtualUnit& Unit = Board.Units[UnitID];
	VirtualBoardWrapper Wrapper(&Board, GridManager->BoardWidth, GridManager->BoardHeight);

	TArray<FVirtualMove> UnitMoves;
	ChessMovementLogic::CalculateLegalMoves(
		&Wrapper,
		Unit.Pos,
		Unit.MovementRule,
		Unit.Team,
		UnitMoves,
		true
	);
	
	for (FVirtualMove& M : UnitMoves)
	{
		M.UnitID = UnitID;
		OutMoves.Add(M);
	}
}

void UChessHTNPlanner::ApplyMove(FVirtualBoard& Board, const FVirtualMove& Move)
{
	Board.Grid.Remove(Move.From);
	if (Move.Type == EVirtualMoveType::Attack) {
		int32 TargetIdx = Board.GetUnitIndexAt(Move.To);
		if (TargetIdx != -1) {
			Board.Units[TargetIdx].HP -= Board.Units[Move.UnitID].Damage;

			if (Board.Units[TargetIdx].HP <= 0) {
				Board.Grid.Remove(Move.To);
			}
		}
	}

	Board.Grid.Add(Move.To, Move.UnitID);
    
	if (Board.Units.IsValidIndex(Move.UnitID))
	{
		Board.Units[Move.UnitID].Pos = Move.To;
	}
}

void UChessHTNPlanner::NotifyExecutedMove(const FIntPoint& From, const FIntPoint& To)
{
	LastMoveSource = From;
	LastMoveDestination = To;
	bHasLastMove = true;
}
