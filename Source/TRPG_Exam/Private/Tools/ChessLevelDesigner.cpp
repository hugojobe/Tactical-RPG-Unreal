// By hugojobe.

#include "Tools/ChessLevelDesigner.h"

#include "Managers/ChessGridManager.h"
#include "Misc/FileHelper.h"


AChessLevelDesigner::AChessLevelDesigner()
{
	auto AddPosition = [this](bool bRoyals, std::initializer_list<const TCHAR*> Rows)
	{
		FTuningPosition Position;
		Position.bHasRoyals = bRoyals;
		for (const TCHAR* Row : Rows) Position.Rows.Add(Row);
		TuningPositions.Add(Position);
	};

	AddPosition(false, { TEXT("P.PP."), TEXT("....."), TEXT("....."), TEXT("....."), TEXT(".pp.p") });
	AddPosition(false, { TEXT("N.PPN"), TEXT("....."), TEXT("....."), TEXT("....."), TEXT("np.pn") });
	AddPosition(false, { TEXT("..P.P."), TEXT(".N.B.."), TEXT("......"), TEXT("......"), TEXT("..b.n."), TEXT(".p.p..") });
	AddPosition(true,  { TEXT("..PKP."), TEXT(".N...B"), TEXT("......"), TEXT("......"), TEXT("b...n."), TEXT(".pkp..") });
	AddPosition(true,  { TEXT("B.R.K."), TEXT(".N.P.P"), TEXT("......"), TEXT("......"), TEXT("n.p.p."), TEXT(".k.r.b") });
	AddPosition(true,  { TEXT(".Q.K.."), TEXT("B.P.PN"), TEXT("......"), TEXT("......"), TEXT("np.p.b"), TEXT("..k.q.") });
	AddPosition(true,  { TEXT("R.Q.K."), TEXT(".NBP.P"), TEXT("......"), TEXT("......"), TEXT("n.p.b."), TEXT(".k.q.r") });
	AddPosition(true,  { TEXT(".R.K.Q."), TEXT("B.PPP.N"), TEXT("......."), TEXT("......."), TEXT("......."), TEXT("n.p.p.b"), TEXT(".q.k.r.") });
	AddPosition(true,  { TEXT("R.Q.K.R"), TEXT(".PBP.PN"), TEXT("......."), TEXT("......."), TEXT("......."), TEXT(".b.p.pn"), TEXT("r.k.q..") });
	AddPosition(true,  { TEXT("RQ.E.QR"), TEXT("..NBN.P"), TEXT("......."), TEXT("......."), TEXT("......."), TEXT("b.p.p.n"), TEXT(".q.k.r.") });
	AddPosition(false, { TEXT("NP.PN"), TEXT("....."), TEXT("....."), TEXT("....."), TEXT("pn.np") });
	AddPosition(false, { TEXT("P.N.P"), TEXT("..P.."), TEXT("....."), TEXT("..p.."), TEXT("p.n.p") });
	AddPosition(false, { TEXT("..P.."), TEXT(".N.P."), TEXT("....."), TEXT(".p.n."), TEXT("..p..") });
	AddPosition(true,  { TEXT("..KP.."), TEXT(".PB.N."), TEXT("......"), TEXT("......"), TEXT(".pb.n."), TEXT("..kp..") });
	AddPosition(true,  { TEXT(".K.P.B"), TEXT("..N.P."), TEXT("......"), TEXT("......"), TEXT(".p.n.."), TEXT("b.p.k.") });
	AddPosition(false, { TEXT("R..P.R"), TEXT(".N.B.."), TEXT("......"), TEXT("......"), TEXT("..b.n."), TEXT("r.p..r") });
	AddPosition(true,  { TEXT("..Q.K."), TEXT(".P..P."), TEXT("......"), TEXT("......"), TEXT(".p..p."), TEXT(".k.q..") });
	AddPosition(true,  { TEXT("..R.K.."), TEXT(".P.B.P."), TEXT("...N..."), TEXT("......."), TEXT("...n..."), TEXT(".p.b.p."), TEXT("..r.k..") });
	AddPosition(true,  { TEXT(".Q..K.R"), TEXT("..P.B.."), TEXT("...N..."), TEXT("......."), TEXT(".n....."), TEXT("..b.p.."), TEXT("r.k..q.") });
	AddPosition(false, { TEXT("RN...NR"), TEXT("..PPP.."), TEXT("......."), TEXT("......."), TEXT("......."), TEXT("..ppp.."), TEXT("rn...nr") });
	AddPosition(true,  { TEXT("...K.."), TEXT("..P.P."), TEXT(".N.b.."), TEXT("..p.N."), TEXT(".q.rk."), TEXT("......") });
	AddPosition(true,  { TEXT("....K."), TEXT("..P.B."), TEXT(".N.p.."), TEXT("..b.n."), TEXT(".k...."), TEXT("......") });
	AddPosition(true,  { TEXT("..K..."), TEXT(".PP..."), TEXT("...N.."), TEXT("..n..."), TEXT("...pp."), TEXT("...k..") });
	AddPosition(false, { TEXT("...B.."), TEXT("..PP.."), TEXT("......"), TEXT("......"), TEXT("..pp.."), TEXT("..b...") });
	AddPosition(true,  { TEXT("..K..."), TEXT("...Q.."), TEXT("......"), TEXT("..r..."), TEXT(".p...."), TEXT("...k..") });
	AddPosition(true,  { TEXT("...K.."), TEXT("......"), TEXT(".R...."), TEXT("....r."), TEXT("......"), TEXT("..k...") });
	AddPosition(true,  { TEXT("..K..."), TEXT("...P.."), TEXT("......"), TEXT("......"), TEXT("..p..."), TEXT("...k..") });
	AddPosition(true,  { TEXT("..K...."), TEXT("....N.."), TEXT("......."), TEXT("......."), TEXT("......."), TEXT("..b...."), TEXT("...k...") });
	AddPosition(true,  { TEXT("..E..."), TEXT(".N.N.."), TEXT("......"), TEXT("..q..."), TEXT(".r.k.."), TEXT("......") });
	AddPosition(false, { TEXT("P.P.P."), TEXT("......"), TEXT("......"), TEXT("......"), TEXT(".p.p.p"), TEXT("......") });
}

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

	EnsurePlanners();
	PlannerA->MaxSearchNodes = 0;

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

	EnsurePlanners();
	PlannerA->MaxSearchNodes = 0;

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
		PlannerA->bExpectPlayerKing = LevelRules[CurrentLevel - 1].bIncludeRoyal;
		PlannerA->bExpectEnemyKing = LevelRules[CurrentLevel - 1].bIncludeRoyal;
		PlannerB->bExpectPlayerKing = LevelRules[CurrentLevel - 1].bIncludeRoyal;
		PlannerB->bExpectEnemyKing = LevelRules[CurrentLevel - 1].bIncludeRoyal;
		
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

void AChessLevelDesigner::RunTuning()
{
	EnsurePlanners();
	ConfigureForTuning(PlannerA);
	ConfigureForTuning(PlannerB);

	FRandomStream Rng(Seed);
	FAIWeights Champion;
	float Sigma = 0.15f;

	for (int32 Iteration = 1; Iteration <= TuningIterations; Iteration++)
	{
		const FAIWeights Challenger = MutateWeights(Champion, Sigma, Rng);
		float Score = RunMatch(Challenger, Champion);
		if (Score > AcceptThreshold)
		{
			Score = 0.5f * (Score + RunMatch(Challenger, Champion));
			if (Score > AcceptThreshold) { Champion = Challenger; }
		}

		if (Score > AcceptThreshold)
		{
			Champion = Challenger;
			UE_LOG(LogTemp, Warning, TEXT("[HJ] Tuning %d/%d: challenger wins (%.1f%%) — new champion"),
				Iteration, TuningIterations, Score * 100.f);
		}
		else
		{
			Sigma = FMath::Max(Sigma * 0.95f, 0.05f);
			UE_LOG(LogTemp, Warning, TEXT("[HJ] Tuning %d/%d: rejected (%.1f%%), sigma %.3f"),
				Iteration, TuningIterations, Score * 100.f, Sigma);
		}
	}

	PlannerA->Weights = Champion;
	PlannerA->SaveWeights(TEXT("AIWeights.json"));
	UE_LOG(LogTemp, Warning, TEXT("[HJ] Tuning finished, weights saved to Saved/AIWeights.json"));
}

void AChessLevelDesigner::EnsurePlanners()
{
	if (!PlannerA)
	{
		PlannerA = NewObject<UChessAIPlanner>(this, TEXT("PlannerA"));
	}
	if (!PlannerB)
	{
		PlannerB = NewObject<UChessAIPlanner>(this, TEXT("PlannerB"));
	}
	PlannerA->InitializeZobrist();
	PlannerB->InitializeZobrist();
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
			const EChessUnitType Type = Rule.AllowedPieces[Rng.RandRange(0, Rule.AllowedPieces.Num() - 1)];
			PlaceAt(MakeUnit(Type, Team));
		}
	};

	PlaceTeam(EChessTeam::Player, Rule.PlayerPieceCount, false);
	PlaceTeam(EChessTeam::Enemy, Rule.EnemyPieceCount, Rule.bEnemyBoss);
	return Board;
}

bool AChessLevelDesigner::FindBestMove(UChessAIPlanner* Planner, const TArray<FIntPoint>& Tiles, const FVirtualBoard& Board, EChessTeam Team, FIntPoint& OutFrom, FIntPoint& OutTo, ETileInteraction& OutInteraction)
{
	Planner->PositionsEvaluated = 0;
	Planner->bAbortSearch = false;
	Planner->SearchStartTime = FPlatformTime::Seconds();
	
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

			Planner->MaxSearchTime = 0.05;
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
	return PlayoutVersus(Board, PlannerA, PlannerA);
}

FPlayoutResult AChessLevelDesigner::PlayoutVersus(FVirtualBoard Board, UChessAIPlanner* EnemyBrain, UChessAIPlanner* PlayerBrain)
{
	FPlayoutResult Result;
	if (!EnemyBrain || !PlayerBrain) return Result;

	TArray<FIntPoint> Tiles;
	for (int32 Y = 0; Y < Board.Height; Y++)
		for (int32 X = 0; X < Board.Width; X++)
			Tiles.Add(FIntPoint(X, Y));

	auto CountTeam = [&Board](EChessTeam Team)
	{
		int32 Count = 0;
		for (int i = 0; i < Board.Width * Board.Height; ++i)
			if (Board.Grid[i].Team == Team) Count++;
		return Count;
	};

	const int32 StartPlayer = CountTeam(EChessTeam::Player);
	const int32 StartEnemy = CountTeam(EChessTeam::Enemy);

	const float LeadThreshold = 8.0f;
	float LastEval = 0.f;
	int32 LastSign = 0;

	for (Result.Rounds = 1; Result.Rounds <= MaxRounds; Result.Rounds++)
	{
		for (const EChessTeam Team : { EChessTeam::Player, EChessTeam::Enemy })
		{
			UChessAIPlanner* Brain = (Team == EChessTeam::Enemy) ? EnemyBrain : PlayerBrain;

			FIntPoint From, To;
			ETileInteraction Interaction;
			if (!FindBestMove(Brain, Tiles, Board, Team, From, To, Interaction))
			{
				Result.Winner = (Team == EChessTeam::Player) ? EChessTeam::Enemy : EChessTeam::Player;
				goto Done;
			}

			const FVirtualUnit Unit = Board.Grid[Board.GetIndex(From)];
			if (Interaction == ETileInteraction::Move)
				Board.MoveUnit(From, To, Unit);
			else
				Board.AttackUnit(From, To, Unit, Board.Grid[Board.GetIndex(To)]);
		}

		bool bPlayerRoyalAlive = false;
		bool bEnemyRoyalAlive = false;
		for (int i = 0; i < Board.Width * Board.Height; ++i)
		{
			const FVirtualUnit& U = Board.Grid[i];
			if (U.UnitType != EChessUnitType::King && U.UnitType != EChessUnitType::Emperor) continue;
			if (U.Team == EChessTeam::Player) bPlayerRoyalAlive = true;
			else if (U.Team == EChessTeam::Enemy) bEnemyRoyalAlive = true;
		}
		if (EnemyBrain->bExpectPlayerKing && !bPlayerRoyalAlive) { Result.Winner = EChessTeam::Enemy; goto Done; }
		if (EnemyBrain->bExpectEnemyKing && !bEnemyRoyalAlive) { Result.Winner = EChessTeam::Player; goto Done; }

		if (CountTeam(EChessTeam::Player) == 0) { Result.Winner = EChessTeam::Enemy; goto Done; }
		if (CountTeam(EChessTeam::Enemy) == 0) { Result.Winner = EChessTeam::Player; goto Done; }

		const float Eval = EnemyBrain->EvaluateBoard(Board, Tiles);
		const int32 Sign = (Eval > LeadThreshold) ? 1 : (Eval < -LeadThreshold ? -1 : 0);
		if (Sign != 0 && LastSign != 0 && Sign != LastSign) Result.LeadChanges++;
		if (Sign != 0) LastSign = Sign;
		LastEval = Eval;

		if (Result.Rounds >= NoContactCutoff
			&& CountTeam(EChessTeam::Player) == StartPlayer
			&& CountTeam(EChessTeam::Enemy) == StartEnemy)
		{
			Result.Winner = EChessTeam::None;
			goto Done;
		}
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

void AChessLevelDesigner::ConfigureForTuning(UChessAIPlanner* Planner)
{
	if (!Planner) return;
	Planner->MaxSearchNodes = TuningNodeBudget;
	Planner->MaxSearchTime = 3600.0;
	Planner->RootNoiseAmplitude = 0.f;
}

FVirtualBoard AChessLevelDesigner::ParsePosition(const FTuningPosition& Position) const
{
	FVirtualBoard Board;
	if (Position.Rows.Num() == 0) return Board;

	Board.Height = Position.Rows.Num();
	Board.Width = Position.Rows[0].Len();

	for (int32 Row = 0; Row < Position.Rows.Num(); Row++)
	{
		const int32 Y = Position.Rows.Num() - 1 - Row;

		for (int32 X = 0; X < Position.Rows[Row].Len(); X++)
		{
			const TCHAR Char = Position.Rows[Row][X];
			if (Char == TEXT('.')) continue;

			const bool bIsEnemy = FChar::IsUpper(Char);
			EChessUnitType Type = EChessUnitType::Pawn;

			switch (FChar::ToUpper(Char))
			{
			case TEXT('P'): Type = EChessUnitType::Pawn; break;
			case TEXT('N'): Type = EChessUnitType::Knight; break;
			case TEXT('B'): Type = EChessUnitType::Bishop; break;
			case TEXT('R'): Type = EChessUnitType::Rook; break;
			case TEXT('Q'): Type = EChessUnitType::Queen; break;
			case TEXT('K'): Type = EChessUnitType::King; break;
			case TEXT('E'): Type = EChessUnitType::Emperor; break;
			default: continue;
			}

			Board.Grid[Board.GetIndex(FIntPoint(X, Y))] = MakeUnit(Type, bIsEnemy ? EChessTeam::Enemy : EChessTeam::Player);
		}
	}
	return Board;
}

FAIWeights AChessLevelDesigner::MutateWeights(const FAIWeights& Source, float Sigma, FRandomStream& Rng) const
{
	FAIWeights Result = Source;

	for (TFieldIterator<FFloatProperty> It(FAIWeights::StaticStruct()); It; ++It)
	{
		float* ValuePtr = It->ContainerPtrToValuePtr<float>(&Result);
		*ValuePtr = FMath::Max(0.f, *ValuePtr * (1.f + Rng.FRandRange(-Sigma, Sigma)));
	}
	return Result;
}

float AChessLevelDesigner::RunMatch(const FAIWeights& Challenger, const FAIWeights& Champion)
{
	float Points = 0.f;
	int32 Games = 0;

	for (const FTuningPosition& Position : TuningPositions)
	{
		const FVirtualBoard Start = ParsePosition(Position);

		PlannerA->bExpectPlayerKing = Position.bHasRoyals;
		PlannerA->bExpectEnemyKing = Position.bHasRoyals;
		PlannerB->bExpectPlayerKing = Position.bHasRoyals;
		PlannerB->bExpectEnemyKing = Position.bHasRoyals;

		for (int32 Side = 0; Side < 2; Side++)
		{
			PlannerA->Weights = (Side == 0) ? Challenger : Champion;
			PlannerB->Weights = (Side == 0) ? Champion : Challenger;

			const FPlayoutResult Result = PlayoutVersus(Start, PlannerA, PlannerB);
			const EChessTeam ChallengerTeam = (Side == 0) ? EChessTeam::Enemy : EChessTeam::Player;

			float GamePoints = 0.5f;
			if (Result.Winner == ChallengerTeam) GamePoints = 1.f;
			else if (Result.Winner != EChessTeam::None) GamePoints = 0.f;

			const int32 ChallengerKills = (ChallengerTeam == EChessTeam::Enemy) ? Result.PlayerPiecesLost : Result.EnemyPiecesLost;
			const int32 ChallengerLosses = (ChallengerTeam == EChessTeam::Enemy) ? Result.EnemyPiecesLost : Result.PlayerPiecesLost;
			GamePoints += (ChallengerKills - ChallengerLosses) * 0.05f;

			Points += FMath::Clamp(GamePoints, 0.f, 1.f);
			Games++;
		}
	}

	return Games > 0 ? Points / Games : 0.5f;
}
