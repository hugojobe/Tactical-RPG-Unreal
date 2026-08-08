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
			
			uint64 StateModifier =
				static_cast<uint64>(Grid[i].UnitStats.CurrentHP) * 0x9E3779B185EBCA87ULL
				+ static_cast<uint64>(Grid[i].UnitStats.Damage) * 0xC2B2AE3D27D4EB4FULL
				+ static_cast<uint64>(Grid[i].RootedTurns) * 0x165667B19E3779F9ULL
				+ (Grid[i].bHasMoved ? 0x27D4EB2F165667C5ULL : 0);
			
			HashKey ^= (BaseKey + StateModifier * (i + 1));
			
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

void UChessAIPlanner::InitializeZobrist()
{
	for (int sq = 0; sq < 64; ++sq) {
		for (int team = 0; team < 3; ++team) {
			for (int type = 0; type < 7; ++type) {
				uint64 High = (uint64)(FMath::RandRange(0, 0x7FFFFFFF));
				uint64 Low = (uint64)(FMath::RandRange(0, 0x7FFFFFFF));
				ZobristTable[sq][team][type] = (High << 32) | Low;
			}
		}
	}

	TranspositionTable.SetNum(TT_SIZE); 
	for (int i = 0; i < TT_SIZE; ++i) {
		TranspositionTable[i].Key = 0;
		TranspositionTable[i].Depth = -1;
	}
}

FChessDecision UChessAIPlanner::GetGlobalDecision(int ThinkDepth, const TArray<FIntPoint>& Tiles, FVirtualBoard Board)
{
	PositionsEvaluated = 0;

	FChessDecision BestOverallDecision;
	BestOverallDecision.Type = EChessDecisionType::Null;
	BestOverallDecision.Score = -BIG_NUMBER;

	SearchStartTime = FPlatformTime::Seconds();
	MaxSearchTime = 1;
	bAbortSearch = false;
	LastCompletedDepth = 0;
	
	struct FRootMove
	{
		FIntPoint From;
		FIntPoint To;
		FVirtualUnit Unit;
		ETileInteraction Interaction;
		bool bIsShuffle = false;
	};
	
	TArray<FRootMove> RootMoves;
	
	for (int i = 0; i < Board.Width * Board.Height; ++i)
	{
		const FVirtualUnit& Unit = Board.Grid[i];
		if (Unit.Team != EChessTeam::Enemy) continue;

		const FIntPoint From(i % Board.Width, i / Board.Width);
		TMap<FIntPoint, ETileInteraction> Moves = AChessGridManager::GetValidMoves(Unit, From, Board);

		for (const TPair<FIntPoint, ETileInteraction>& MovePair : Moves)
		{
			FRootMove NewMove{ From, MovePair.Key, Unit, MovePair.Value };
			NewMove.bIsShuffle = (MovePair.Value == ETileInteraction::Move && UnitLastPositions.FindRef(From) == MovePair.Key);
			RootMoves.Add(NewMove);
		}
	}
	
	if (RootMoves.Num() == 0) return BestOverallDecision;
	
	BestOverallDecision.Type = (RootMoves[0].Interaction == ETileInteraction::Move) ? EChessDecisionType::Move : EChessDecisionType::Attack;
	BestOverallDecision.SourcePosition = RootMoves[0].From;
	BestOverallDecision.TargetPosition = RootMoves[0].To;
	
	RootMoves.Sort([](const FRootMove& A, const FRootMove& B) {
		return (A.Interaction == ETileInteraction::Attack) > (B.Interaction == ETileInteraction::Attack);
	});
	
	for (int CurrentDepth = 1; CurrentDepth <= ThinkDepth; CurrentDepth++)
	{
		FChessDecision DepthDecision;
		DepthDecision.Type = EChessDecisionType::Null;
		DepthDecision.Score = -BIG_NUMBER;

		float Alpha = -BIG_NUMBER;
		float Beta = BIG_NUMBER;

		for (const FRootMove& Move : RootMoves)
		{
			FVirtualBoard SimBoard = Board;
			if (Move.Interaction == ETileInteraction::Move)
				SimBoard.MoveUnit(Move.From, Move.To, Move.Unit);
			else
				SimBoard.AttackUnit(Move.From, Move.To, Move.Unit, Board.Grid[Board.GetIndex(Move.To)]);

			const float ActionScore = Minimax(Tiles, SimBoard, CurrentDepth - 1, Alpha, Beta, false);
			
			float AdjustedScore = ActionScore - (Move.bIsShuffle ? 1.0f : 0.0f);
			
			if (bAbortSearch) break;

			if (AdjustedScore > DepthDecision.Score)
			{
				DepthDecision.Type = (Move.Interaction == ETileInteraction::Move) ? EChessDecisionType::Move : EChessDecisionType::Attack;
				DepthDecision.SourcePosition = Move.From;
				DepthDecision.TargetPosition = Move.To;
				DepthDecision.Score = AdjustedScore;
			}

			Alpha = FMath::Max(Alpha, AdjustedScore);
		}

		if (bAbortSearch) break;

		if (DepthDecision.Type != EChessDecisionType::Null)
		{
			BestOverallDecision = DepthDecision;
			LastCompletedDepth = CurrentDepth;
		}

		if (BestOverallDecision.Score > 500000.0f) break;
	}

	return BestOverallDecision;
}

float UChessAIPlanner::Minimax(const TArray<FIntPoint>& Tiles, const FVirtualBoard& Board, int Depth, float Alpha, float Beta, bool bIsMaximizing)
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
	
	uint64 CurrentHash = Board.ComputeHash(ZobristTable, bIsMaximizing);
	int TTIndex = CurrentHash % TT_SIZE;
	FTTEntry& TT = TranspositionTable[TTIndex];
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

		auto Moves = AChessGridManager::GetValidMoves(Unit, Pos, Board);
    
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
		return bIsMaximizing ? -(999999.0f + Depth) : (999999.0f + Depth);
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

float UChessAIPlanner::EvaluateBoard(const FVirtualBoard& Board, const TArray<FIntPoint>& Tiles)
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
			float Advancement = (Unit.Team == EChessTeam::Enemy) ? (Board.Height - 1 - Y) : Y;			PositionalBonus = Advancement * (MaxBonus / Board.Height);
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

float UChessAIPlanner::EvaluateBoardShallow(const TArray<FIntPoint>& Tiles, const FVirtualBoard& Board, int Depth)
{
	SearchStartTime = FPlatformTime::Seconds();
	MaxSearchTime = 0.2;
	bAbortSearch = false;

	return Minimax(Tiles, Board, Depth, -BIG_NUMBER, BIG_NUMBER, true);
}

