#include "AI/ChessAIPlanner.h"

#include "HLSLMathAliases.h"
#include "Framework/ChessGameMode.h"
#include "Managers/ChessGridManager.h"


int32 FVirtualBoard::GetIndex(const FIntPoint& Pos) const
{
	return Pos.Y * Width + Pos.X;
}

void FVirtualBoard::MoveUnit(FIntPoint SourcePos, FIntPoint TargetPos, FVirtualUnit Unit)
{
	if (!IsValidGridPos(TargetPos, Width, Height)) return;
	
	Grid[GetIndex(TargetPos)] = Unit;
	Grid[GetIndex(SourcePos)].Team = EChessTeam::None;
}

void FVirtualBoard::AttackUnit(FIntPoint SourcePos, FIntPoint TargetPos, FVirtualUnit SourceUnit, FVirtualUnit TargetUnit)
{
	if (!IsValidGridPos(TargetPos, Width, Height) || !IsTileOccupied(TargetPos)) return;
	if (SourceUnit.Team == TargetUnit.Team) return;
        
	int32 TargetIdx = GetIndex(TargetPos);
	int32 SourceIdx = GetIndex(SourcePos);

	if (Grid[TargetIdx].UnitStats.CurrentHP <= SourceUnit.UnitStats.Damage)
	{
		Grid[TargetIdx] = SourceUnit;
		Grid[SourceIdx].Team = EChessTeam::None;
	}
	else
	{
		Grid[TargetIdx].UnitStats.CurrentHP -= SourceUnit.UnitStats.Damage;
	}
}

uint64 FVirtualBoard::ComputeHash(const uint64 Zobrist[64][3][7], bool bIsMaximizing) const
{
	uint64 HashKey = 0;
	for (int i = 0; i < Width * Height; ++i)
	{
		if (Grid[i].Team != EChessTeam::None)
		{
			int TeamIdx = static_cast<int>(Grid[i].Team);
			int TypeIdx = static_cast<int>(Grid[i].UnitType);
            
			uint64 BaseKey = Zobrist[i][TeamIdx][TypeIdx];
			
			uint64 HPModifier = static_cast<uint64>(Grid[i].UnitStats.CurrentHP) * (i + 1) * 0x9E3779B185EBCA87ULL;
			
			HashKey ^= (BaseKey + HPModifier);
			
			if (BlockedTurns[i] > 0)
			{
				HashKey ^= Zobrist[i][2][0];
			}
		}
	}
	
	if (bIsMaximizing) HashKey ^= 0x123456789ABCDEFULL; 
	
	return HashKey;
}

bool FVirtualBoard::IsValidGridPos(const FIntPoint& Pos, int BoardWidth, int BoardHeight) const
{
	return (Pos.X >= 0 && Pos.X < BoardWidth && Pos.Y >= 0 && Pos.Y < BoardHeight);
}

bool FVirtualBoard::IsTileOccupied(const FIntPoint& Pos) const
{
	if (!IsValidGridPos(Pos, Width, Height)) return false;
	return Grid[GetIndex(Pos)].Team != EChessTeam::None;
}

EChessTeam FVirtualBoard::GetTeamAt(const FIntPoint& Pos) const
{
	if (!IsValidGridPos(Pos, Width, Height)) return EChessTeam::None;
	return Grid[GetIndex(Pos)].Team;
}

bool FVirtualBoard::IsTileBlocked(const FIntPoint& Pos) const
{
	if (!IsValidGridPos(Pos, Width, Height)) return false;
	return BlockedTurns[GetIndex(Pos)] > 0;
}

FVirtualBoard UChessAIPlanner::GenerateVirtualBoard(TMap<FIntPoint, FVirtualUnit> VUnits, int Width, int Height)
{
	FVirtualBoard VBoard;
	VBoard.Width = Width;
	VBoard.Height = Height;

	for (int i = 0; i < MAX_CHESS_BOARD_SIZE; ++i)
	{
		VBoard.Grid[i].Team = EChessTeam::None;
	}
	
	for (const auto& UnitPair : VUnits)
	{
		if (VBoard.IsValidGridPos(UnitPair.Key, Width, Height))
		{
			VBoard.Grid[VBoard.GetIndex(UnitPair.Key)] = UnitPair.Value;
		}
	}
	
	return VBoard;
}

TMap<FIntPoint, float> UChessAIPlanner::GenerateThreatMap(FVirtualBoard VBoard)
{
	TMap<FIntPoint, float> ThreatMap;

	for (int i = 0; i < VBoard.Width * VBoard.Height; ++i)
	{
		const FVirtualUnit& Unit = VBoard.Grid[i];

		if (Unit.Team == EChessTeam::None || Unit.Team == EChessTeam::Enemy) 
		{
			continue;
		}

		FIntPoint Location(i % VBoard.Width, i / VBoard.Width);

		const float DamageValue = Unit.UnitStats.Damage;
		const bool bIsPawn = (Unit.UnitMovementRule.MovementStyle == EMovementStyle::Pawn);

		TMap<FIntPoint, ETileInteraction> LegalMoves = AChessGridManager::GetValidMoves(Unit, Location, VBoard, true);
       
		for (const auto& [TargetCoord, Interaction] : LegalMoves)
		{
			if (bIsPawn && Interaction != ETileInteraction::Attack)
			{
				continue;
			}

			float& ThreatScore = ThreatMap.FindOrAdd(TargetCoord);
			ThreatScore += DamageValue;
		}
	}
    
	return ThreatMap;
}

TMap<FIntPoint, float> UChessAIPlanner::GenerateDefenseMap(FVirtualBoard VBoard)
{
	TMap<FIntPoint, float> DefenseMap;

	for (int i = 0; i < VBoard.Width * VBoard.Height; ++i)
	{
		const FVirtualUnit& Unit = VBoard.Grid[i];
		
		if (Unit.Team == EChessTeam::None || Unit.Team == EChessTeam::Player) 
		{
			continue;
		}
		
		FIntPoint Location(i % VBoard.Width, i / VBoard.Width);
		
		const float DamageValue = Unit.UnitStats.Damage;
		const bool bIsPawn = (Unit.UnitMovementRule.MovementStyle == EMovementStyle::Pawn);

		TMap<FIntPoint, ETileInteraction> LegalMoves = AChessGridManager::GetValidMoves(Unit, Location, VBoard, true);
		
		for (const auto& [TargetCoord, Interaction] : LegalMoves)
		{
			if (bIsPawn && Interaction != ETileInteraction::Attack)
			{
				continue;
			}

			float& DefenseScore = DefenseMap.FindOrAdd(TargetCoord);
			DefenseScore += DamageValue;
		}
	}
	
	return DefenseMap;
}

TMap<FIntPoint, float> UChessAIPlanner::GenerateBalanceMap(TArray<FIntPoint> Tiles, TMap<FIntPoint, float> Threats, TMap<FIntPoint, float> Benefits)
{
	TMap<FIntPoint, float> Balance;
	for (auto Tile : Tiles)
	{
		float TileScore = Threats.FindRef(Tile) - Benefits.FindRef(Tile);
		Balance.Add(Tile, TileScore);
	}
	
	return Balance;
}

FChessDecision UChessAIPlanner::GetUnitDecision(int ThinkDepth, TArray<FIntPoint> Tiles, FVirtualBoard Board)
{	
	PositionsEvaluated = 0;
	
	FChessDecision BestOverallDecision;
	BestOverallDecision.Type = EChessDecisionType::Null;
	BestOverallDecision.Score = -BIG_NUMBER;
	
	if (!GridManager) return BestOverallDecision;
	
	FIntPoint UnitOriginalPos = FIntPoint(MyUnit->GridCoordinates.X, MyUnit->GridCoordinates.Y);
	FVirtualUnit VUnit = Board.Grid[Board.GetIndex(UnitOriginalPos)];
	TMap<FIntPoint, ETileInteraction> PossibleFirstMove = GridManager->GetValidMoves(VUnit, UnitOriginalPos, GridManager->GameMode->VirtualBoard);
	
	SearchStartTime = FPlatformTime::Seconds();
	MaxSearchTime = 2;
	bAbortSearch = false;
	LastCompletedDepth = 0;
	
	FChessDecision FallbackDecision;
	FallbackDecision.Type = EChessDecisionType::Null;
	for (TTuple<FIntPoint, ETileInteraction> FirstMove : PossibleFirstMove)
	{
		if (FallbackDecision.Type == EChessDecisionType::Null)
		{
			FallbackDecision.Type = (FirstMove.Value == ETileInteraction::Move) ? EChessDecisionType::Move : EChessDecisionType::Attack;
			FallbackDecision.TargetPosition = FirstMove.Key;
			FallbackDecision.Score = -BIG_NUMBER;
		}
    
		if (FirstMove.Key != MyUnit->LastPosition)
		{
			FallbackDecision.Type = (FirstMove.Value == ETileInteraction::Move) ? EChessDecisionType::Move : EChessDecisionType::Attack;
			FallbackDecision.TargetPosition = FirstMove.Key;
			FallbackDecision.Score = -BIG_NUMBER;
			break;
		}
	}
	
	for (int CurrentDepth = 1; CurrentDepth <= ThinkDepth; CurrentDepth++)
	{
		FChessDecision DepthDecision;
		DepthDecision.Type = EChessDecisionType::Null;
		DepthDecision.Score = -BIG_NUMBER;
		
		float Alpha = -BIG_NUMBER;
		float Beta = BIG_NUMBER;

		for (TTuple<FIntPoint, ETileInteraction> FirstMove : PossibleFirstMove)
		{			
			FVirtualBoard SimBoard = Board;
			switch (FirstMove.Value)
			{
			default: break;
			case ETileInteraction::Move:
				if (FirstMove.Key == MyUnit->LastPosition) continue;
				SimBoard.MoveUnit(UnitOriginalPos, FirstMove.Key, VUnit);
				break;
			case ETileInteraction::Attack:
				SimBoard.AttackUnit(UnitOriginalPos, FirstMove.Key, VUnit, Board.Grid[Board.GetIndex(FirstMove.Key)]);				
				break;
			}
			
			float ActionScore = Minimax(Tiles, SimBoard, CurrentDepth - 1, Alpha, Beta, false);
			
			if (bAbortSearch) break;
			
			if (ActionScore > DepthDecision.Score)
			{
				DepthDecision.Score = ActionScore;
				DepthDecision.Type = (FirstMove.Value == ETileInteraction::Move) ? EChessDecisionType::Move : EChessDecisionType::Attack;
				DepthDecision.TargetPosition = FirstMove.Key;
			}
			
			Alpha = FMath::Max(Alpha, DepthDecision.Score);
		}
		
		if (bAbortSearch) break;

		LastCompletedDepth = CurrentDepth;
		BestOverallDecision = DepthDecision;
		if (BestOverallDecision.Score > 500000.0f) break;
	}
	
	if (BestOverallDecision.Type == EChessDecisionType::Null)
	{
		return FallbackDecision;
	}
	
	return BestOverallDecision;
}

float UChessAIPlanner::Minimax(TArray<FIntPoint> Tiles, const FVirtualBoard& Board, int Depth, float Alpha, float Beta, bool bIsMaximizing)
{
	PositionsEvaluated++;
	if (bAbortSearch) return 0.0f;
	
	if (PositionsEvaluated % 1000 == 0)
	{
		if (FPlatformTime::Seconds() - SearchStartTime > MaxSearchTime)
		{
			bAbortSearch = true;
			return 0.0f;
		}
	}
	
	uint64 CurrentHash = Board.ComputeHash(GridManager->ZobristTable, bIsMaximizing);
	int TTIndex = CurrentHash % TT_SIZE;
	FTTEntry& TT = GridManager->TranspositionTable[TTIndex];
	FIntPoint TTBestMove = FIntPoint(-1, -1);
	
	if (TT.Key == CurrentHash) 
	{
		TTBestMove = TT.BestMoveTarget;
		
		if (TT.Depth >= Depth)
		{
			if (TT.Bound == EHashBound::Exact) return TT.Score;
			if (TT.Bound == EHashBound::LowerBound && TT.Score > Alpha) Alpha = TT.Score;
			if (TT.Bound == EHashBound::UpperBound && TT.Score < Beta) Beta = TT.Score;
			if (Alpha >= Beta) return TT.Bound == EHashBound::LowerBound ? Alpha : Beta; 
		}
	}
	
	if (Depth == 0) return EvaluateBoard(Board, Tiles);
	
	float OriginalAlpha = Alpha;
	float OriginalBeta = Beta;
	
	EChessTeam CurrentTeam = bIsMaximizing ? EChessTeam::Enemy : EChessTeam::Player;
	TArray<FAvailableMove> AllMoves;
	
	for (int i = 0; i < Board.Width * Board.Height; ++i)
	{
		FVirtualUnit Unit = Board.Grid[i];
    
		if (Unit.Team == EChessTeam::None || Unit.Team != CurrentTeam) continue;

		FIntPoint Pos(i % Board.Width, i / Board.Width);

		auto Moves = GridManager->GetValidMoves(Unit, Pos, Board);
    
		for (auto& MovePair : Moves)
		{
			FAvailableMove NewMove;
			NewMove.From = Pos;
			NewMove.To = MovePair.Key;
			NewMove.Unit = Unit;
			NewMove.Interaction = MovePair.Value;
        
			if (MovePair.Key == TTBestMove)
				NewMove.Priority = 1000.0f;
			else if (MovePair.Value == ETileInteraction::Attack)
				NewMove.Priority = 100.0f;
			else
				NewMove.Priority = 0.0f;
        
			AllMoves.Add(NewMove);
		}
	}
	
	AllMoves.Sort([](const FAvailableMove& A, const FAvailableMove& B) {
		return A.Priority > B.Priority;
	});
	
	if (AllMoves.Num() == 0)
	{
		return bIsMaximizing ? -999999.0f : 999999.0f;
	}
	
	float BestScore = bIsMaximizing ? -BIG_NUMBER : BIG_NUMBER;
	FIntPoint CurrentBestMoveTarget = FIntPoint(-1, -1);

	if (bIsMaximizing)
	{
		for (const FAvailableMove& Move : AllMoves)
		{
			FVirtualBoard SimBoard = Board;	
			
			if (Move.Interaction == ETileInteraction::Move)
				SimBoard.MoveUnit(Move.From, Move.To, Move.Unit);
			else if (Move.Interaction == ETileInteraction::Attack)
				SimBoard.AttackUnit(Move.From, Move.To, Move.Unit, SimBoard.Grid[SimBoard.GetIndex(Move.To)]);
			
			float Eval = Minimax(Tiles, SimBoard, Depth - 1, Alpha, Beta, false);
			
			if (Eval > BestScore)
			{
				BestScore = Eval;
				CurrentBestMoveTarget = Move.To;
			}
			
			Alpha = FMath::Max(Alpha, Eval);
			if (Beta <= Alpha) break;
		}
	}
	else
	{
		for (const FAvailableMove& Move : AllMoves)
		{
			FVirtualBoard SimBoard = Board;
			
			if (Move.Interaction == ETileInteraction::Move)
				SimBoard.MoveUnit(Move.From, Move.To, Move.Unit);
			else if (Move.Interaction == ETileInteraction::Attack)
				SimBoard.AttackUnit(Move.From, Move.To, Move.Unit, SimBoard.Grid[SimBoard.GetIndex(Move.To)]);            
			
			float Eval = Minimax(Tiles, SimBoard, Depth - 1, Alpha, Beta, true);
			
			if (Eval < BestScore)
			{
				BestScore = Eval;
				CurrentBestMoveTarget = Move.To;
			}
			
			Beta = FMath::Min(Beta, Eval);
			if (Beta <= Alpha) break;
		}
	}
	
	if (bAbortSearch) return BestScore;
	
	EHashBound BoundType = EHashBound::Exact;
	if (BestScore <= OriginalAlpha) 
		BoundType = EHashBound::UpperBound;
	else if (BestScore >= OriginalBeta) 
		BoundType = EHashBound::LowerBound;
	
	TT.Key = CurrentHash;
	TT.Score = BestScore;
	TT.Depth = Depth;
	TT.Bound = BoundType;
	TT.BestMoveTarget = CurrentBestMoveTarget;

	return BestScore;
}

float UChessAIPlanner::EvaluateBoard(FVirtualBoard Board, TArray<FIntPoint> Tiles)
{
	bool bPlayerKingAlive = false;
	bool bAiKingAlive = false;
	float Score = 0;

	for (int i = 0; i < Board.Width * Board.Height; ++i)
	{
		const FVirtualUnit& Unit = Board.Grid[i];
		if (Unit.Team == EChessTeam::None) continue;

		float PieceValueScore = Unit.UnitStats.KillRpReward * 10.0f;
		float HPScore = Unit.UnitStats.CurrentHP * 2.0f;
		float UnitPower = PieceValueScore + HPScore;

		float PositionalBonus = 0.0f;
		float MaxBonus = UnitPower * 0.10f;

		int32 X = i % Board.Width;
		int32 Y = i / Board.Width;

		if (Unit.UnitMovementRule.MovementStyle == EMovementStyle::Pawn)
		{
			float Advancement = (Unit.Team == EChessTeam::Enemy) ? Y : (Board.Height - 1 - Y);
			PositionalBonus = Advancement * (MaxBonus / Board.Height);
		}
		else if (Unit.UnitType != EChessUnitType::King)
		{
			bool bIsBackRank = (Unit.Team == EChessTeam::Enemy) ? (Y == 0) : (Y == Board.Height - 1);
        
			if (bIsBackRank && (Unit.UnitType == EChessUnitType::Knight || Unit.UnitType == EChessUnitType::Bishop))
			{
				PositionalBonus = -MaxBonus * 1.5f;
			}
			else
			{
				float CenterX = Board.Width / 2.0f;
				float CenterY = Board.Height / 2.0f;
				float DistToCenter = FMath::Abs(X - CenterX) + FMath::Abs(Y - CenterY);
            
				PositionalBonus = (6.0f - DistToCenter) * (MaxBonus / 6.0f);
			}
		}
		
		float FinalUnitScore = 0.0f;

		if (Unit.UnitType == EChessUnitType::King) 
		{
			if (Unit.Team == EChessTeam::Player) bPlayerKingAlive = true;
			else bAiKingAlive = true;

			FinalUnitScore = 500.0f + (Unit.UnitStats.CurrentHP * 50.0f) + PositionalBonus;
		}
		else
		{
			FinalUnitScore = UnitPower + PositionalBonus;
		}
		
		if (Unit.Team == EChessTeam::Player)
		{
			Score -= FinalUnitScore;
		}
		else if (Unit.Team == EChessTeam::Enemy)
		{
			Score += FinalUnitScore;
		}
	}
	
	if (!bPlayerKingAlive && GridManager->GameMode->bPlayerKingExists) Score += 1000000.0f;
	if (!bAiKingAlive && GridManager->GameMode->bEnemyKingExists) Score -= 1000000.0f;
            
	return Score;
}

