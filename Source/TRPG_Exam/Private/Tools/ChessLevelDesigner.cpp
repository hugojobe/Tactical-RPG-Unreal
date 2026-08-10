// By hugojobe.

#include "Tools/ChessLevelDesigner.h"

#include "Managers/ChessGridManager.h"
#include "Misc/FileHelper.h"


void AChessLevelDesigner::GenerateLevels()
{
	using T = EChessUnitType;

	auto AddRule = [this](int32 Width, int32 Height, bool bRoyal, bool bBoss,
		int32 PlayerCount, int32 EnemyCount, const TArray<T>& Allowed)
	{
		FLevelGenerationRule Rule;
		Rule.BoardWidth = Width;
		Rule.BoardHeight = Height;
		Rule.bIncludeRoyal = bRoyal;
		Rule.bEnemyBoss = bBoss;
		Rule.PlayerPieceCount = PlayerCount;
		Rule.EnemyPieceCount = EnemyCount;
		Rule.AllowedPieces = Allowed;
		LevelRules.Add(Rule);
	};

	AddRule(5, 5, false, false, 3, 3, { T::Pawn });
	AddRule(5, 5, false, false, 4, 4, { T::Pawn, T::Knight });
	AddRule(6, 6, false, false, 4, 4, { T::Pawn, T::Knight, T::Bishop });
	AddRule(6, 6, true,  false, 4, 4, { T::Pawn, T::Knight, T::Bishop });
	AddRule(6, 6, true,  false, 5, 5, { T::Pawn, T::Knight, T::Bishop, T::Rook });
	AddRule(6, 6, true,  false, 5, 5, { T::Pawn, T::Knight, T::Bishop, T::Rook, T::Queen });
	AddRule(6, 6, true,  false, 5, 6, { T::Pawn, T::Knight, T::Bishop, T::Rook, T::Queen });
	AddRule(7, 7, true,  false, 6, 7, { T::Pawn, T::Knight, T::Bishop, T::Rook, T::Queen });
	AddRule(7, 7, true,  false, 6, 8, { T::Pawn, T::Knight, T::Bishop, T::Rook, T::Queen });
	AddRule(7, 7, true,  true,  6, 8, { T::Pawn, T::Knight, T::Bishop, T::Rook, T::Queen });
}

void AChessLevelDesigner::GenerateLevelsBlocking()
{
	StartGeneration();
	if (!bRunning) return;

	double LastLogTime = FPlatformTime::Seconds();
	while (bRunning)
	{
		StepGeneration(10);

		if (FPlatformTime::Seconds() - LastLogTime > 2.0)
		{
			LastLogTime = FPlatformTime::Seconds();
			const float BestScore = BestPerLevel.IsValidIndex(CurrentLevel - 1) ? BestPerLevel[CurrentLevel - 1].Score : 0.f;
			UE_LOG(LogTemp, Warning, TEXT("[HJ] LevelGen: %3.0f%% | level %d | current best score %.1f"),
				GetProgress() * 100.f, CurrentLevel, BestScore);
		}
	}

	for (int32 Level = 1; Level <= BestPerLevel.Num(); Level++)
	{
		const FLevelCandidate& Best = BestPerLevel[Level - 1];
		UE_LOG(LogTemp, Warning, TEXT("[HJ] ===== LEVEL %d | Score %.1f | Rounds %d | LeadChanges %d | Winner %d =====\n%s"),
			Level, Best.Score, Best.Rounds, Best.LeadChanges, static_cast<int32>(Best.Winner),
			*ExportCandidate(Best));
	}
}

void AChessLevelDesigner::StartGeneration()
{
	if (!UnitStatsTable) return;
	bSingleLevelMode = false;

	if (!Planner)
	{
		Planner = NewObject<UChessAIPlanner>(this, TEXT("DesignerPlanner"));
		
	}
	Planner->InitializeZobrist();

	RNG.Initialize(Seed);
	BestPerLevel.Empty();
	BestPerLevel.SetNum(10);
	for (FLevelCandidate& Best : BestPerLevel) Best = FLevelCandidate();

	CurrentLevel = 1;
	CandidateIndex = 0;
	bRunning = true;
}

void AChessLevelDesigner::StartSingleLevelGeneration(int32 LevelIndex)
{
	if (!UnitStatsTable || !LevelRules.IsValidIndex(LevelIndex - 1)) return;

	if (!Planner)
	{
		Planner = NewObject<UChessAIPlanner>(this, TEXT("DesignerPlanner"));
	}
	Planner->InitializeZobrist();

	if (BestPerLevel.Num() != LevelRules.Num())
	{
		BestPerLevel.SetNum(LevelRules.Num());
	}
	BestPerLevel[LevelIndex - 1] = FLevelCandidate();

	RNG.Initialize(Seed + LevelIndex * 1000 + RegenCounter++);

	CurrentLevel = LevelIndex;
	CandidateIndex = 0;
	bSingleLevelMode = true;
	bRunning = true;
}

void AChessLevelDesigner::CancelGeneration()
{
	bRunning = false;
}

void AChessLevelDesigner::StepGeneration(int32 PlayoutsPerStep)
{
	if (!bRunning) return;

	for (int32 Step = 0; Step < PlayoutsPerStep && bRunning; Step++)
	{
		const FVirtualBoard Setup = BuildRandomSetup(RNG, CurrentLevel);		
		Planner->bExpectPlayerKing = LevelRules[CurrentLevel - 1].bIncludeRoyal;
		Planner->bExpectEnemyKing = LevelRules[CurrentLevel - 1].bIncludeRoyal;
		
		const FPlayoutResult Result = Playout(Setup);
		const float Score = ScoreCandidate(Result);

		FLevelCandidate& Best = BestPerLevel[CurrentLevel - 1];
		if (Score > Best.Score)
		{
			Best.Score = Score;
			Best.Rounds = Result.Rounds;
			Best.LeadChanges = Result.LeadChanges;
			Best.Winner = Result.Winner;
			Best.Placement.Empty();
			Best.BoardWidth = Setup.Width;
			Best.BoardHeight = Setup.Height;
			for (int i = 0; i < Setup.Width * Setup.Height; ++i)
			{
				if (Setup.Grid[i].Team == EChessTeam::None) continue;
				FLevelPlacementEntry Entry;
				Entry.UnitType = Setup.Grid[i].UnitType;
				Entry.Team = Setup.Grid[i].Team;
				Entry.Position = FIntPoint(i % Setup.Width, i / Setup.Width);
				Best.Placement.Add(Entry);
			}
		}

		if (++CandidateIndex >= CandidatesPerLevel)
		{
			CandidateIndex = 0;
			if (bSingleLevelMode || ++CurrentLevel > LevelRules.Num())
			{
				bRunning = false;
				bSingleLevelMode = false;
			}
		}
	}
}

float AChessLevelDesigner::GetProgress() const
{	
	if (bSingleLevelMode)
	{
		return CandidatesPerLevel > 0 ? static_cast<float>(CandidateIndex) / CandidatesPerLevel : 0.f;
	}
	
	const int32 Total = CandidatesPerLevel * 10;
	const int32 Done = (CurrentLevel - 1) * CandidatesPerLevel + CandidateIndex;
	
	return Total > 0 ? static_cast<float>(Done) / Total : 0.f;
}

FVirtualUnit AChessLevelDesigner::MakeUnit(EChessUnitType Type, EChessTeam Team) const
{
	FVirtualUnit Unit;
	Unit.Team = Team;
	Unit.UnitType = Type;

	const UEnum* EnumPtr = StaticEnum<EChessUnitType>();
	const FName RowName(*EnumPtr->GetNameStringByValue(static_cast<int64>(Type)));
	if (const FChessUnitData* Row = UnitStatsTable->FindRow<FChessUnitData>(RowName, TEXT("LevelDesigner"), false))
	{
		Unit.UnitStats.MaxHP = Row->MaxHP;
		Unit.UnitStats.CurrentHP = Row->MaxHP;
		Unit.UnitStats.Damage = Row->Damage;
		Unit.UnitStats.KillRpReward = Row->KillRpReward;
		UChessAIPlanner::RegisterMovementRule(Type, Row->MovementRule);
	}
	return Unit;
}

FVirtualBoard AChessLevelDesigner::BuildRandomSetup(FRandomStream& Rng, int32 LevelIndex) const
{
	FVirtualBoard Board;
	Board.Width = LevelRules[LevelIndex - 1].BoardWidth;
	Board.Height = LevelRules[LevelIndex - 1].BoardHeight;
	const int32 SpawnRanks = (Board.Height >= 6) ? 2 : 1;
	
	if (!LevelRules.IsValidIndex(LevelIndex - 1)) return Board;
	const FLevelGenerationRule& Rule = LevelRules[LevelIndex - 1];

	auto PlaceTeam = [&](EChessTeam Team, int32 PieceCount, bool bBoss)
	{		
		const int32 RowMin = (Team == EChessTeam::Player) ? 0 : Board.Height - SpawnRanks;
		const int32 RowMax = RowMin + SpawnRanks - 1;

		TArray<FIntPoint> FreeTiles;
		for (int32 Y = RowMin; Y <= RowMax; Y++)
			for (int32 X = 0; X < Board.Width; X++)
				FreeTiles.Add(FIntPoint(X, Y));

		auto PlaceAt = [&](const FVirtualUnit& Unit)
		{
			const int32 Index = Rng.RandRange(0, FreeTiles.Num() - 1);
			Board.Grid[Board.GetIndex(FreeTiles[Index])] = Unit;
			FreeTiles.RemoveAtSwap(Index);
		};

		if (Rule.bIncludeRoyal) PlaceAt(MakeUnit(bBoss ? EChessUnitType::Emperor : EChessUnitType::King, Team));

		for (int32 i = 0; i < PieceCount && FreeTiles.Num() > 0 && Rule.AllowedPieces.Num() > 0; i++)
		{
			const EChessUnitType Type = Rule.AllowedPieces[RNG.RandRange(0, Rule.AllowedPieces.Num() - 1)];
			PlaceAt(MakeUnit(Type, Team));
		}
	};

	PlaceTeam(EChessTeam::Player, Rule.PlayerPieceCount, false);
	PlaceTeam(EChessTeam::Enemy, Rule.EnemyPieceCount, Rule.bEnemyBoss);
	return Board;
}

bool AChessLevelDesigner::FindBestMove(const TArray<FIntPoint>& Tiles, const FVirtualBoard& Board, EChessTeam Team,
	FIntPoint& OutFrom, FIntPoint& OutTo, ETileInteraction& OutInteraction)
{
	const bool bMaximizing = (Team == EChessTeam::Enemy);
	float BestScore = bMaximizing ? -BIG_NUMBER : BIG_NUMBER;
	bool bFound = false;

	for (int i = 0; i < Board.Width * Board.Height; ++i)
	{
		const FVirtualUnit& Unit = Board.Grid[i];
		if (Unit.Team != Team) continue;
		const FIntPoint From(i % Board.Width, i / Board.Width);

		for (const auto& MovePair : AChessGridManager::GetValidMoves(Unit, From, Board))
		{
			FVirtualBoard Sim = Board;
			if (MovePair.Value == ETileInteraction::Move)
				Sim.MoveUnit(From, MovePair.Key, Unit);
			else
				Sim.AttackUnit(From, MovePair.Key, Unit, Board.Grid[Board.GetIndex(MovePair.Key)]);

			Planner->SearchStartTime = FPlatformTime::Seconds();
			Planner->MaxSearchTime = 0.05;
			Planner->bAbortSearch = false;
			const float Score = Planner->Minimax(Tiles, Sim, SelfPlayDepth - 1, -BIG_NUMBER, BIG_NUMBER, !bMaximizing);

			if ((bMaximizing && Score > BestScore) || (!bMaximizing && Score < BestScore))
			{
				BestScore = Score;
				OutFrom = From; OutTo = MovePair.Key; OutInteraction = MovePair.Value;
				bFound = true;
			}
		}
	}
	return bFound;
}

FPlayoutResult AChessLevelDesigner::Playout(FVirtualBoard Board)
{
	FPlayoutResult Result;

	TArray<FIntPoint> Tiles;
	for (int32 Y = 0; Y < Board.Height; Y++)
		for (int32 X = 0; X < Board.Width; X++)
			Tiles.Add(FIntPoint(X, Y));

	auto CountTeam = [&](EChessTeam Team)
	{
		int32 Count = 0;
		for (int i = 0; i < Board.Width * Board.Height; ++i)
			if (Board.Grid[i].Team == Team) Count++;
		return Count;
	};
	const int32 StartPlayer = CountTeam(EChessTeam::Player);
	const int32 StartEnemy = CountTeam(EChessTeam::Enemy);

	const float LeadThreshold = 8.f;
	int32 LastSign = 0;
	
	float LastEval = 0.f;
	for (Result.Rounds = 1; Result.Rounds <= MaxRounds; Result.Rounds++)
	{
		for (const EChessTeam Team : { EChessTeam::Player, EChessTeam::Enemy })
		{
			FIntPoint From, To;
			ETileInteraction Interaction;
			if (!FindBestMove(Tiles, Board, Team, From, To, Interaction))
			{
				Result.Winner = (Team == EChessTeam::Player) ? EChessTeam::Enemy : EChessTeam::Player;
				goto Done;
			}

			const FVirtualUnit Unit = Board.Grid[Board.GetIndex(From)];
			if (Interaction == ETileInteraction::Move) Board.MoveUnit(From, To, Unit);
			else Board.AttackUnit(From, To, Unit, Board.Grid[Board.GetIndex(To)]);
		}

		const float Eval = Planner->EvaluateBoard(Board, Tiles);
		
		const int32 Sign = (Eval > LeadThreshold) ? 1 : (Eval < -LeadThreshold ? -1 : 0);
		if (Sign != 0 && LastSign != 0 && Sign != LastSign) Result.LeadChanges++;
		if (Sign != 0) LastSign = Sign;
		
		if ((Eval > 0) != (LastEval > 0) && Result.Rounds > 1) Result.LeadChanges++;
		LastEval = Eval;
	}

Done:
	Result.PlayerPiecesLost = StartPlayer - CountTeam(EChessTeam::Player);
	Result.EnemyPiecesLost = StartEnemy - CountTeam(EChessTeam::Enemy);
	Result.FinalEval = LastEval;
	return Result;
}

float AChessLevelDesigner::ScoreCandidate(const FPlayoutResult& Result) const
{
	if (Result.Winner == EChessTeam::None) return -100.f;
	
	float Score = 0.f;
	Score += Result.LeadChanges * 10.f;
	Score += FMath::Min(Result.PlayerPiecesLost, Result.EnemyPiecesLost) * 8.f;
	Score += (Result.Rounds >= 6 && Result.Rounds <= 25) ? 15.f : -10.f;
	Score += (Result.Winner == EChessTeam::Player) ? 20.f : 0.f;
	return Score;
}

FString AChessLevelDesigner::ExportCandidate(const FLevelCandidate& Candidate) const
{
	const int32 Width = Candidate.BoardWidth;
	const int32 Height = Candidate.BoardHeight;

	TCHAR Grid[MAX_CHESS_BOARD_SIZE];
	for (int32 i = 0; i < Width * Height; i++) Grid[i] = TEXT('.');

	for (const FLevelPlacementEntry& Entry : Candidate.Placement)
	{
		TCHAR Letter = TEXT('?');
		switch (Entry.UnitType)
		{
		case EChessUnitType::Pawn:    Letter = TEXT('P'); break;
		case EChessUnitType::Knight:  Letter = TEXT('N'); break;
		case EChessUnitType::Bishop:  Letter = TEXT('B'); break;
		case EChessUnitType::Rook:    Letter = TEXT('R'); break;
		case EChessUnitType::Queen:   Letter = TEXT('Q'); break;
		case EChessUnitType::King:    Letter = TEXT('K'); break;
		case EChessUnitType::Emperor: Letter = TEXT('E'); break;
		}

		if (Entry.Team == EChessTeam::Player) Letter = FChar::ToLower(Letter);

		if (Entry.Position.X >= 0 && Entry.Position.X < Width && Entry.Position.Y >= 0 && Entry.Position.Y < Height)
		{
			Grid[Entry.Position.Y * Width + Entry.Position.X] = Letter;
		}
	}

	FString Result;
	for (int32 Y = Height - 1; Y >= 0; Y--)
	{
		for (int32 X = 0; X < Width; X++)
		{
			Result.AppendChar(Grid[Y * Width + X]);
			Result.AppendChar(TEXT(' '));
		}
		Result.AppendChar(TEXT('\n'));
	}
	return Result;
}
