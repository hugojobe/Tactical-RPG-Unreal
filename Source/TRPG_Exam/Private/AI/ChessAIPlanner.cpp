#include "AI/ChessAIPlanner.h"

#include "HLSLMathAliases.h"
#include "Framework/ChessGameMode.h"
#include "Managers/ChessGridManager.h"

FMovementRule UChessAIPlanner::MovementRuleByType[7];

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
		if (BlockedTurns[i] > 0)
		{
			HashKey ^= Zobrist[i][2][0];
		}
		
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

void UChessAIPlanner::ClearSearchHeuristics()
{
	for (int32 Ply = 0; Ply < MAX_SEARCH_PLY; Ply++)
	{
		KillerMoves[Ply][0] = FKillerMove();
		KillerMoves[Ply][1] = FKillerMove();
	}

	for (int32 From = 0; From < 64; From++)
		for (int32 To = 0; To < 64; To++)
			HistoryHeuristic[From][To] /= 2;
}

float UChessAIPlanner::Quiescence(const TArray<FIntPoint>& Tiles, const FVirtualBoard& Board, float Alpha, float Beta, bool bIsMaximizing, int32 QDepth)
{
	if (bAbortSearch) return 0.0f;

	PositionsEvaluated++;
	if (PositionsEvaluated % 1000 == 0 && FPlatformTime::Seconds() - SearchStartTime > MaxSearchTime)
	{
		bAbortSearch = true;
		return 0.0f;
	}

	const float StandPat = EvaluateBoard(Board, Tiles);
	if (QDepth <= 0) return StandPat;

	if (bIsMaximizing)
	{
		if (StandPat >= Beta) return Beta;
		Alpha = FMath::Max(Alpha, StandPat);
	}
	else
	{
		if (StandPat <= Alpha) return Alpha;
		Beta = FMath::Min(Beta, StandPat);
	}

	const EChessTeam CurrentTeam = bIsMaximizing ? EChessTeam::Enemy : EChessTeam::Player;

	TArray<FAvailableMove> Attacks;
	for (int i = 0; i < Board.Width * Board.Height; ++i)
	{
		const FVirtualUnit& Unit = Board.Grid[i];
		if (Unit.Team == EChessTeam::None || Unit.Team != CurrentTeam) continue;

		const FIntPoint Pos(i % Board.Width, i / Board.Width);
		
		SearchScratchMoves.Reset();
		ChessMovementLogic::CalculateLegalMoves(Board, Pos, UChessAIPlanner::GetMovementRule(Unit.UnitType), Unit.Team, SearchScratchMoves, false);

		for (const FChessDecision& Decision : SearchScratchMoves)
		{
			if (Decision.Type != EChessDecisionType::Attack) continue;

			FAvailableMove NewMove;
			NewMove.From = Pos;
			NewMove.To = Decision.TargetPosition;
			NewMove.Unit = Unit;
			NewMove.Interaction = ETileInteraction::Attack;

			const FVirtualUnit& Victim = Board.Grid[Board.GetIndex(Decision.TargetPosition)];
			NewMove.Priority = Victim.UnitStats.KillRpReward * 10.0f + Victim.UnitStats.CurrentHP * 2.0f;
			Attacks.Add(NewMove);
		}
	}

	if (Attacks.Num() == 0) return StandPat;

	Attacks.Sort([](const FAvailableMove& A, const FAvailableMove& B) { return A.Priority > B.Priority; });

	for (const FAvailableMove& Move : Attacks)
	{
		FVirtualBoard SimBoard = Board;
		SimBoard.AttackUnit(Move.From, Move.To, Move.Unit, SimBoard.Grid[SimBoard.GetIndex(Move.To)]);

		const float Eval = Quiescence(Tiles, SimBoard, Alpha, Beta, !bIsMaximizing, QDepth - 1);

		if (bIsMaximizing)
		{
			Alpha = FMath::Max(Alpha, Eval);
			if (Alpha >= Beta) return Beta;
		}
		else
		{
			Beta = FMath::Min(Beta, Eval);
			if (Beta <= Alpha) return Alpha;
		}
	}

	return bIsMaximizing ? Alpha : Beta;
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
		const bool bIsPawn = GetMovementRule(Unit.UnitType).MovementStyle == EMovementStyle::Pawn;

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
		const bool bIsPawn = GetMovementRule(Unit.UnitType).MovementStyle == EMovementStyle::Pawn;

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
	
	FMemory::Memzero(HistoryHeuristic, sizeof(HistoryHeuristic));
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
	
	TArray<float> RootNoise;
	RootNoise.Reserve(RootMoves.Num());
	for (int32 i = 0; i < RootMoves.Num(); i++)
	{
		RootNoise.Add(FMath::FRandRange(0.f, RootNoiseAmplitude));
	}
	
	if (RootMoves.Num() == 0) return BestOverallDecision;
	
	BestOverallDecision.Type = (RootMoves[0].Interaction == ETileInteraction::Move) ? EChessDecisionType::Move : EChessDecisionType::Attack;
	BestOverallDecision.SourcePosition = RootMoves[0].From;
	BestOverallDecision.TargetPosition = RootMoves[0].To;
	
	RootMoves.Sort([](const FRootMove& A, const FRootMove& B) {
		return (A.Interaction == ETileInteraction::Attack) > (B.Interaction == ETileInteraction::Attack);
	});
	
	ClearSearchHeuristics();
	
	for (int CurrentDepth = 1; CurrentDepth <= ThinkDepth; CurrentDepth++)
	{
		FChessDecision DepthDecision;
		DepthDecision.Type = EChessDecisionType::Null;
		DepthDecision.Score = -BIG_NUMBER;

		float Alpha = -BIG_NUMBER;
		float Beta = BIG_NUMBER;

		for (int32 MoveIndex = 0; MoveIndex < RootMoves.Num(); MoveIndex++)
		{
			const FRootMove& Move = RootMoves[MoveIndex];

			FVirtualBoard SimBoard = Board;
			if (Move.Interaction == ETileInteraction::Move)
				SimBoard.MoveUnit(Move.From, Move.To, Move.Unit);
			else
				SimBoard.AttackUnit(Move.From, Move.To, Move.Unit, Board.Grid[Board.GetIndex(Move.To)]);

			const int32 IdleTurns = FMath::Min(UnitIdleTurns.FindRef(Move.From), ActivityBonusCap);

			const float ActionScore = Minimax(Tiles, SimBoard, CurrentDepth - 1, Alpha, Beta, false);

			if (bAbortSearch) break;

			const float AdjustedScore =
			   ActionScore
			   + IdleTurns * ActivityBonusWeight
			   + RootNoise[MoveIndex]
			   - (Move.bIsShuffle ? 1.0f : 0.0f);

			if (AdjustedScore > DepthDecision.Score)
			{
				DepthDecision.Type = (Move.Interaction == ETileInteraction::Move) ? EChessDecisionType::Move : EChessDecisionType::Attack;
				DepthDecision.SourcePosition = Move.From;
				DepthDecision.TargetPosition = Move.To;
				DepthDecision.Score = AdjustedScore;
			}

			Alpha = FMath::Max(Alpha, ActionScore);
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
	
	if (Depth <= 0) return Quiescence(Tiles, Board, Alpha, Beta, bIsMaximizing, MaxQuiescenceDepth);
	
	EChessTeam CurrentTeam = bIsMaximizing ? EChessTeam::Enemy : EChessTeam::Player;
	
	if (bUseNullMove && Depth >= 3 && !bAbortSearch)
	{
		const bool bHasFiniteBound = bIsMaximizing ? (Beta < BIG_NUMBER * 0.5f) : (Alpha > -BIG_NUMBER * 0.5f);

		int32 MovingPieces = 0;
		for (int i = 0; i < Board.Width * Board.Height; ++i)
		{
			if (Board.Grid[i].Team == CurrentTeam) MovingPieces++;
		}

		if (bHasFiniteBound && MovingPieces >= 3)
		{
			const int32 Reduction = 2;
			const float NullWindow = 0.5f;

			if (bIsMaximizing)
			{
				const float NullScore = Minimax(Tiles, Board, Depth - 1 - Reduction, Beta - NullWindow, Beta, false);
				if (!bAbortSearch && NullScore >= Beta) return Beta;
			}
			else
			{
				const float NullScore = Minimax(Tiles, Board, Depth - 1 - Reduction, Alpha, Alpha + NullWindow, true);
				if (!bAbortSearch && NullScore <= Alpha) return Alpha;
			}
		}
	}
	
	float OriginalAlpha = Alpha;
	float OriginalBeta = Beta;
	
	TArray<FAvailableMove> AllMoves;
	
	const int32 KillerIndex = FMath::Clamp(Depth, 0, MAX_SEARCH_PLY - 1);
	
	for (int i = 0; i < Board.Width * Board.Height; ++i)
	{
		FVirtualUnit Unit = Board.Grid[i];
    
		if (Unit.Team == EChessTeam::None || Unit.Team != CurrentTeam) continue;

		FIntPoint Pos(i % Board.Width, i / Board.Width);

		SearchScratchMoves.Reset();
		ChessMovementLogic::CalculateLegalMoves(Board, Pos, UChessAIPlanner::GetMovementRule(Unit.UnitType), Unit.Team, SearchScratchMoves, false);

		for (const FChessDecision& Decision : SearchScratchMoves)
		{
			FAvailableMove NewMove;
			NewMove.From = Pos;
			NewMove.To = Decision.TargetPosition;
			NewMove.Unit = Unit;
			NewMove.Interaction = (Decision.Type == EChessDecisionType::Attack) ? ETileInteraction::Attack : ETileInteraction::Move;
        
			if (Decision.TargetPosition == TTBestMove)
			{
				NewMove.Priority = 1000000.f;
			}
			else if (NewMove.Interaction == ETileInteraction::Attack)
			{
				const FVirtualUnit& Victim = Board.Grid[Board.GetIndex(Decision.TargetPosition)];
				NewMove.Priority = 100000.f + Victim.UnitStats.KillRpReward * 10.f + Victim.UnitStats.CurrentHP * 2.f;
			}
			else if (KillerMoves[KillerIndex][0].From == Pos && KillerMoves[KillerIndex][0].To == Decision.TargetPosition)
			{
				NewMove.Priority = 90000.f;
			}
			else if (KillerMoves[KillerIndex][1].From == Pos && KillerMoves[KillerIndex][1].To == Decision.TargetPosition)
			{
				NewMove.Priority = 80000.f;
			}
			else
			{
				NewMove.Priority = FMath::Min(static_cast<float>(HistoryHeuristic[Board.GetIndex(Pos)][Board.GetIndex(Decision.TargetPosition)]), 70000.f);
			}
        
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
		int32 MoveIndex = 0;
		for (const FAvailableMove& Move : AllMoves)
		{
			FVirtualBoard SimBoard = Board;

			if (Move.Interaction == ETileInteraction::Move)
				SimBoard.MoveUnit(Move.From, Move.To, Move.Unit);
			else if (Move.Interaction == ETileInteraction::Attack)
				SimBoard.AttackUnit(Move.From, Move.To, Move.Unit, SimBoard.Grid[SimBoard.GetIndex(Move.To)]);
			
			float Eval;
			const bool bCanReduce = bUseLMR && MoveIndex >= 4 && Depth >= 3 && Move.Interaction != ETileInteraction::Attack;

			if (bCanReduce)
			{
				Eval = Minimax(Tiles, SimBoard, Depth - 2, Alpha, Beta, false);
				if (Eval > Alpha)
				{
					Eval = Minimax(Tiles, SimBoard, Depth - 1, Alpha, Beta, false);
				}
			}
			else
			{
				Eval = Minimax(Tiles, SimBoard, Depth - 1, Alpha, Beta, false);
			}
			
			if (Eval > BestScore)
			{
				BestScore = Eval;
				CurrentBestMoveTarget = Move.To;
			}
			
			Alpha = FMath::Max(Alpha, Eval);
			if (Beta <= Alpha)
			{
				if (Move.Interaction != ETileInteraction::Attack)
				{
					FKillerMove& First = KillerMoves[KillerIndex][0];
					if (First.From != Move.From || First.To != Move.To)
					{
						KillerMoves[KillerIndex][1] = First;
						KillerMoves[KillerIndex][0] = { Move.From, Move.To };
					}
					HistoryHeuristic[Board.GetIndex(Move.From)][Board.GetIndex(Move.To)] += Depth * Depth;
				}
				break;
			}
			
			MoveIndex++;
		}
	}
	else
	{
		int32 MoveIndex = 0;
		for (const FAvailableMove& Move : AllMoves)
		{
			FVirtualBoard SimBoard = Board;
			
			if (Move.Interaction == ETileInteraction::Move)
				SimBoard.MoveUnit(Move.From, Move.To, Move.Unit);
			else if (Move.Interaction == ETileInteraction::Attack)
				SimBoard.AttackUnit(Move.From, Move.To, Move.Unit, SimBoard.Grid[SimBoard.GetIndex(Move.To)]);            
			
			float Eval;
			const bool bCanReduce = bUseLMR && MoveIndex >= 4 && Depth >= 3 && Move.Interaction != ETileInteraction::Attack;
			
			if (bCanReduce)
			{
				Eval = Minimax(Tiles, SimBoard, Depth - 2, Alpha, Beta, true);
				if (Eval < Beta)
				{
					Eval = Minimax(Tiles, SimBoard, Depth - 1, Alpha, Beta, true);
				}
			}
			else
			{
				Eval = Minimax(Tiles, SimBoard, Depth - 1, Alpha, Beta, true);
			}
			
			if (Eval < BestScore)
			{
				BestScore = Eval;
				CurrentBestMoveTarget = Move.To;
			}
			
			Beta = FMath::Min(Beta, Eval);
			if (Beta <= Alpha)
			{
				if (Move.Interaction != ETileInteraction::Attack)
				{
					FKillerMove& First = KillerMoves[KillerIndex][0];
					if (First.From != Move.From || First.To != Move.To)
					{
						KillerMoves[KillerIndex][1] = First;
						KillerMoves[KillerIndex][0] = { Move.From, Move.To };
					}
					HistoryHeuristic[Board.GetIndex(Move.From)][Board.GetIndex(Move.To)] += Depth * Depth;
				}
				break;
			}
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
	
	int32 Coverage[2][MAX_CHESS_BOARD_SIZE] = {};
	int32 Reach[2] = { 0, 0 };
	float AlliedMaterial[2] = { 0.f, 0.f };
	TArray<TPair<FIntPoint, EChessTeam>, TInlineAllocator<16>> UnitPositions;

	if (bEnableStrategicEval)
	{
		for (int i = 0; i < Board.Width * Board.Height; ++i)
		{
			const FVirtualUnit& Unit = Board.Grid[i];
			if (Unit.Team == EChessTeam::None) continue;

			const int32 TeamIdx = (Unit.Team == EChessTeam::Player) ? 0 : 1;
			const FIntPoint Pos(i % Board.Width, i / Board.Width);
			
			UnitPositions.Add({ Pos, Unit.Team });
			
			if (Unit.UnitType != EChessUnitType::King && Unit.UnitType != EChessUnitType::Emperor)
			{
				AlliedMaterial[TeamIdx] += Unit.UnitStats.KillRpReward * 10.0f + Unit.UnitStats.CurrentHP * 2.0f;
			}

			EvalScratchMoves.Reset();
			ChessMovementLogic::CalculateLegalMoves(Board, Pos, UChessAIPlanner::GetMovementRule(Unit.UnitType), Unit.Team, EvalScratchMoves, true);

			Reach[TeamIdx] += EvalScratchMoves.Num();
			for (const FChessDecision& Decision : EvalScratchMoves)
			{
				if (Decision.Type == EChessDecisionType::Attack)
				{
					Coverage[TeamIdx][Board.GetIndex(Decision.TargetPosition)]++;
				}
			}
		}
	}
	
	float KingSafetyScale[2];
	for (int32 T = 0; T < 2; T++)
	{
		KingSafetyScale[T] = FMath::Clamp(AlliedMaterial[T] / 40.0f, 0.0f, 1.0f);
	}

	for (int i = 0; i < Board.Width * Board.Height; ++i)
	{
		const FVirtualUnit& Unit = Board.Grid[i];
		if (Unit.Team == EChessTeam::None) continue;

		float PieceValueScore = Unit.UnitStats.KillRpReward * 10.0f;
		float HPScore = Unit.UnitStats.CurrentHP * 6.0f;
		float UnitPower = PieceValueScore + HPScore;

		float PositionalBonus = 0.0f;
		float MaxBonus = UnitPower * 0.10f;

		int32 X = i % Board.Width;
		int32 Y = i / Board.Width;

		if (UChessAIPlanner::GetMovementRule(Unit.UnitType).MovementStyle == EMovementStyle::Pawn)
		{
			float Advancement = (Unit.Team == EChessTeam::Enemy) ? (Board.Height - 1 - Y) : Y;
			PositionalBonus = Advancement * (MaxBonus / Board.Height);
		}
		else if (Unit.UnitType != EChessUnitType::King && Unit.UnitType != EChessUnitType::Emperor)
		{
			const float CenterX = (Board.Width - 1) / 2.0f;
			const float CenterY = (Board.Height - 1) / 2.0f;
			const float MaxDist = CenterX + CenterY;
			const float DistToCenter = FMath::Abs(X - CenterX) + FMath::Abs(Y - CenterY);
			PositionalBonus = (MaxDist - DistToCenter) * (MaxBonus / MaxDist);

			const int32 HomeDepth = (Unit.Team == EChessTeam::Enemy) ? (Y - (Board.Height - 2)) : (1 - Y);
			if (HomeDepth == 0)
			{
				PositionalBonus -= MaxBonus * 0.75f;
			}
			else if (HomeDepth >= 1)
			{
				PositionalBonus -= MaxBonus * 1.5f;
			}
		}
		
		float FinalUnitScore = 0.0f;
		const int32 OwnIdx = (Unit.Team == EChessTeam::Player) ? 0 : 1;
		const int32 FoeIdx = 1 - OwnIdx;

		if (Unit.UnitType == EChessUnitType::King || Unit.UnitType == EChessUnitType::Emperor)
		{
			if (Unit.Team == EChessTeam::Player) bPlayerKingAlive = true;
			else bAiKingAlive = true;

			FinalUnitScore = 500.0f + (Unit.UnitStats.CurrentHP * 50.0f) + PositionalBonus;

			if (bEnableStrategicEval)
			{
				float Danger = Coverage[FoeIdx][i] * KingThreatWeight;
				if (Coverage[OwnIdx][i] > 0) Danger *= 0.5f;

				const float SafetyScale = (Unit.UnitType == EChessUnitType::Emperor) ? 0.0f : KingSafetyScale[OwnIdx];
				FinalUnitScore -= Danger * SafetyScale;

				if (Unit.UnitType == EChessUnitType::Emperor)
				{
					for (const auto& Entry : UnitPositions)
					{
						if (Entry.Value == Unit.Team || Entry.Value == EChessTeam::None) continue;
						const int32 Dist = FMath::Max(FMath::Abs(Entry.Key.X - X), FMath::Abs(Entry.Key.Y - Y));
						if (Dist <= 2) FinalUnitScore += (3 - Dist) * EmperorAggression;
					}
				}
			}
		}
		else
		{
			FinalUnitScore = UnitPower + PositionalBonus;
		}
		
		if (bEnableStrategicEval && Coverage[OwnIdx][i] == 0 && Coverage[FoeIdx][i] > 0)
		{
			FinalUnitScore -= UnitPower * HangingPenalty;
		}

		if (Unit.Team == EChessTeam::Player) Score -= FinalUnitScore;
		else Score += FinalUnitScore;
	}
	
	if (bEnableStrategicEval) Score += (Reach[1] - Reach[0]) * ReachWeight;

	if (!bPlayerKingAlive && bExpectPlayerKing) Score += 1000000.0f;
	if (!bAiKingAlive && bExpectEnemyKing) Score -= 1000000.0f;

	return Score;
}

float UChessAIPlanner::EvaluateBoardShallow(const TArray<FIntPoint>& Tiles, const FVirtualBoard& Board, int Depth, double TimeBudget)
{
	SearchStartTime = FPlatformTime::Seconds();
	MaxSearchTime = TimeBudget;
	bAbortSearch = false;

	return Minimax(Tiles, Board, Depth, -BIG_NUMBER, BIG_NUMBER, true);
}

