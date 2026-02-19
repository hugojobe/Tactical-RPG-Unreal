// By hugojobe.


#include "Managers/ChessGridManager.h"
#include "ToolBuilderUtil.h"
#include "AI/ChessAIPlanner.h"
#include "Components/TimelineComponent.h"
#include "Framwork/ChessGameMode.h"
#include "Kismet/GameplayStatics.h"

AChessGridManager::AChessGridManager()
{
	PrimaryActorTick.bCanEverTick = true;

	MoveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimeline"));
	AttackTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("AttackTimeline"));

	TileSize = 100.0f;
	BoardWidth = 6;
	BoardHeight = 6;
}

void AChessGridManager::Initialize()
{
	Super::BeginPlay();
	RegisterStartingUnits();

	if(AttackRushCurve)
	{
		FOnTimelineFloat ProgressFunction;
		ProgressFunction.BindUFunction(this, FName("HandleAttackProgress"));
		AttackTimeline->AddInterpFloat(AttackRushCurve, ProgressFunction);

		FOnTimelineEvent FinishedFunction;
		FinishedFunction.BindUFunction(this, FName("HandleAttackFinished"));
		AttackTimeline->SetTimelineFinishedFunc(FinishedFunction);
	}
}

void AChessGridManager::RegisterStartingUnits()
{
	TArray<AActor*> StartingUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessUnitBase::StaticClass(), StartingUnits);

	for (auto StartingUnit : StartingUnits)
	{
		AChessUnitBase* ChessUnit = Cast<AChessUnitBase>(StartingUnit);
		FVector Location = StartingUnit->GetActorLocation();
		FIntPoint GridPos = WorldToGrid(Location);
		RegisterUnitAt(GridPos, ChessUnit);
		
		ChessUnit->OnUnitHoverUpdated.AddDynamic(this, &AChessGridManager::OnUnitHoverUpdated);
		ChessUnit->OnUnitClicked.AddDynamic(this, &AChessGridManager::OnUnitClicked);
		ChessUnit->GridManager = this;
	}
}

FIntPoint AChessGridManager::WorldToGrid(FVector WorldPos)
{
	FVector LocalPos = WorldPos - GetActorLocation();

	int32 X = FMath::FloorToInt(LocalPos.X / TileSize);
	int32 Y = FMath::FloorToInt(LocalPos.Y / TileSize);

	return FIntPoint(X, Y);
}

FVector AChessGridManager::GridToWorld(FIntPoint GridPos)
{
	float X = (GridPos.X * TileSize) + (TileSize / 2.0f);
	float Y = (GridPos.Y * TileSize) + (TileSize / 2.0f);

	return GetActorLocation() + FVector(X, Y, 0.0f);
}

void AChessGridManager::RegisterUnitAt(FIntPoint GridPos, AChessUnitBase* Unit)
{
	if(!Unit || !IsValidGridPos(GridPos)) return;

	GridMap.Add(GridPos, Unit);

	Unit->GridCoordinates = FVector2D(GridPos.X, GridPos.Y);
	Unit->SetActorLocation(GridToWorld(GridPos));
	
	if (Unit->UnitType == EChessUnitType::King)
	{
		if (Unit->Team == EChessTeam::Player)
			GameMode->bPlayerKingExists = true;
		else if (Unit->Team == EChessTeam::Enemy)
			GameMode->bEnemyKingExists = true;
	}
}

void AChessGridManager::ClearTile(FIntPoint GridPos)
{
	if(GridMap.Contains(GridPos))
	{
		GridMap.Remove(GridPos);
	}
}

AChessUnitBase* AChessGridManager::GetUnitAt(FIntPoint GridPos)
{
	if(GridMap.Contains(GridPos))
	{
		return GridMap[GridPos];
	}
	return nullptr;
}

bool AChessGridManager::IsValidGridPos(FIntPoint GridPos)
{
	return IsValidGridPos(GridPos, BoardWidth, BoardHeight);
}

bool AChessGridManager::IsValidGridPos(const FIntPoint& Pos, int Width, int Height) const
{
	return (Pos.X >= 0 && Pos.X < Width &&
			Pos.Y >= 0 && Pos.Y < Height);
}

void AChessGridManager::ClearBoardVisuals()
{
	for (const TPair<FIntPoint, AChessTileBase*>& TilePair : TileActors)
    {
        if (TilePair.Value)
        {
            TilePair.Value->Destroy();
        }
    }
    TileActors.Empty();
}

void AChessGridManager::GenerateGrid()
{
	ClearBoardVisuals();

	if(!WhiteTileClass || !BlackTileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HJ] No black or white tile assigned in ChessGridManager"));
		return;
	}

	for (int32 x = 0; x < BoardWidth; x++)
	{
		for (int32 y = 0; y < BoardHeight; y++)
		{
			bool bIsBlack = (x + y) % 2 == 0;
			TSubclassOf<AChessTileBase> ClassToSpawn = bIsBlack ? BlackTileClass : WhiteTileClass;

			FIntPoint GridPos(x, y);
			FVector WorldPos = GridToWorld(GridPos);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AChessTileBase* NewTile = GetWorld()->SpawnActor<AChessTileBase>(ClassToSpawn, WorldPos, FRotator::ZeroRotator, SpawnParams);

			if(NewTile)
			{
				NewTile->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
				NewTile->GridPosition = GridPos;
				NewTile->GridManager = this;
				NewTile->TileWorldLocation = WorldPos;
				NewTile->OnTileClicked.AddDynamic(this, &AChessGridManager::OnTileClicked);

				TileActors.Add(GridPos, NewTile);
			}
		}
	}
}

bool AChessGridManager::IsEnemy(const AChessUnitBase* Unit, const AChessUnitBase* TargetUnit)
{
	if (!Unit || !TargetUnit) return false;
	return Unit->Team != TargetUnit->Team;
}

TMap<FIntPoint, ETileInteraction> AChessGridManager::GetValidMoves(FVirtualUnit Unit, FIntPoint UnitPos, UVirtualBoard* Board, bool bIncludeEmptyAttacks)
{
	if (Board == nullptr) return {};
	
	TMap<FIntPoint, ETileInteraction> Result;
	
	TArray<FChessDecision> RawMoves = {};
	ChessMovementLogic::CalculateLegalMoves(
		Board,
		FIntPoint(UnitPos.X, UnitPos.Y),
		Unit.UnitMovementRule,
		Unit.Team,
		RawMoves,
		bIncludeEmptyAttacks
	);
	
	for (const FChessDecision& Decision : RawMoves)
	{
		ETileInteraction Interaction = (Decision.Type == EChessDecisionType::Attack) ? ETileInteraction::Attack : ETileInteraction::Move;
		Result.Add(Decision.TargetPosition, Interaction);
	}
    
	return Result;
}

void AChessGridManager::OnTileClicked_Implementation(AChessTileBase* ClickedTile)
{
	if (!IsUnitClicked || CurrentTurnActionState != ETurnState::Player) return;
	
	FIntPoint TilePos = ClickedTile->GridPosition;
	AChessUnitBase* UnitOnTile = GetUnitAt(TilePos);
	
	if (UnitOnTile != nullptr)
	{
		if (UnitOnTile == SelectedUnit)
			DeselectUnit();
		else if(UnitOnTile->Team != SelectedUnit->Team && HighlightedTiles.Find(TilePos) != nullptr)
			AttackUnit(SelectedUnit, UnitOnTile);
	}
	else
	{
		if (HighlightedTiles.Find(TilePos) != nullptr && SelectedUnit->Team == EChessTeam::Player)
			MoveUnit(SelectedUnit, TilePos);
		else
			DeselectUnit();
	}
}

void AChessGridManager::OnUnitClicked_Implementation(AChessUnitBase* Unit)
{
	if(CurrentTurnActionState != ETurnState::Player) return;
	
	if (!IsUnitClicked)
	{
		if (Unit->Team == EChessTeam::Player)
		{
			HighlightTilesForUnit(Unit);
			Unit->OnUnitClickedFeedback();
			IsUnitClicked = true;
		}
	}
	else
	{
		if (HighlightedTiles.Find(FIntPoint(Unit->GridCoordinates.X, Unit->GridCoordinates.Y)) && SelectedUnit->Team == EChessTeam::Player)
		{
			AttackUnit(SelectedUnit, Unit);
		}
		else if(Unit->Team == EChessTeam::Player)
		{
			if (Unit == SelectedUnit) return;
			DeselectUnit();
			HighlightTilesForUnit(Unit);
			Unit->OnUnitClickedFeedback();
			IsUnitClicked = true;
		}
	}
}

void AChessGridManager::OnUnitHoverUpdated_Implementation(AChessUnitBase* Unit)
{
	if (!IsUnitClicked && CurrentTurnActionState == ETurnState::Player)
		HighlightTilesForUnit(Unit);
}

void AChessGridManager::DeselectUnit_Implementation()
{	
	if(SelectedUnit != nullptr){
 		SelectedUnit->OnUnitUnclickedFeedback();
 		SelectedUnit = nullptr;
 	}
	HighlightTilesForUnit(nullptr);
	IsUnitClicked = false;
	HighlightedTiles.Empty();
	
}

void AChessGridManager::HighlightTilesForUnit_Implementation(AChessUnitBase* Unit)
{
	if (Unit != nullptr)
	{
		if (SelectedUnit != Unit)
		{
			SelectedUnit = Unit;
			
			FVirtualUnit VUnit;
			VUnit.Team = Unit->Team;
			VUnit.UnitMovementRule = Unit->MovementRule;
			VUnit.UnitStats = Unit->Stats;
			
			HighlightedTiles = GetValidMoves(VUnit, FIntPoint(Unit->GridCoordinates.X, Unit->GridCoordinates.Y), GameMode->VirtualBoard, false);
			
			if (!HighlightedTiles.IsEmpty())
			{
				for (const TPair<FIntPoint, ETileInteraction>& TileTuple : HighlightedTiles)
				{
					AChessTileBase* Tile = TileActors.FindRef(TileTuple.Key);
					if (Tile != nullptr)
					{
						Tile->SetHighlight(TileTuple.Value);
					}
				}
			}
		}
	}
	else
	{
		for (const TPair<FIntPoint, AChessTileBase*>& Tile : TileActors)
		{
			if (Tile.Value) Tile.Value->SetHighlight(ETileInteraction::None);
		}
		
		SelectedUnit = nullptr;
	}
}

void AChessGridManager::MoveUnit_Implementation(AChessUnitBase* Unit, FIntPoint Position)
{
	if(!Unit || !UnitMovementSpeedCurve || CurrentTurnActionState != GameMode->CurrentTurn) return;

	CurrentTurnActionState = ETurnState::None;
	
	MovingUnit = Unit;
	MoveStartLoc = Unit->GetActorLocation();
	MoveTargetLoc = GridToWorld(Position);

	FOnTimelineFloat ProgressFunction;
	ProgressFunction.BindUFunction(this, FName("HandleMoveProgress"));

	FOnTimelineEvent FinishedFunction;
	FinishedFunction.BindUFunction(this, FName("HandleMoveFinished"));

	MoveTimeline->AddInterpFloat(UnitMovementSpeedCurve, ProgressFunction);
	MoveTimeline->SetTimelineFinishedFunc(FinishedFunction);

	HighlightTilesForUnit(nullptr);
	MoveTimeline->PlayFromStart();
}

void AChessGridManager::HandleMoveProgress(float Value)
{
	if(MovingUnit)
	{
		FVector NewLoc = FMath::Lerp(MoveStartLoc, MoveTargetLoc, Value);
		MovingUnit->SetActorLocation(NewLoc);

		FVector MoveDirection = (MoveTargetLoc - MoveStartLoc).GetSafeNormal();
		FRotator TargetRotation = MoveDirection.Rotation();

		float LeanFactor = FMath::Sin(Value * PI);
		TargetRotation.Pitch = UnitMovementTiltAngle * LeanFactor;
		MovingUnit->UnitMeshComponent->SetRelativeRotation(TargetRotation);
	}
}

void AChessGridManager::HandleMoveFinished_Implementation()
{
	if (MovingUnit)
	{
		MovingUnit->SetActorLocation(MoveTargetLoc);

		FRotator FinalRot = MovingUnit->GetActorRotation();
        FinalRot.Pitch = 0.0f;
        FinalRot.Roll = 0.0f;
		MovingUnit->UnitMeshComponent->SetRelativeRotation(FinalRot);

		ClearTile(FIntPoint(MovingUnit->GridCoordinates.X, MovingUnit->GridCoordinates.Y));
		MovingUnit->GridCoordinates = FVector2D(WorldToGrid(MoveTargetLoc));
		RegisterUnitAt(WorldToGrid(MoveTargetLoc), MovingUnit);

		MovingUnit->OnUnitUnclickedFeedback();
		MovingUnit = nullptr;
		SelectedUnit = nullptr;
		IsUnitClicked = false;

		PlayerActionFinished();
	}
}

void AChessGridManager::AttackUnit_Implementation(AChessUnitBase* Attacker, AChessUnitBase* TargetUnit)
{
	if(!Attacker || !TargetUnit || !AttackRushCurve || !AttackBounceBackCurve || CurrentTurnActionState != GameMode->CurrentTurn) return;

	CurrentTurnActionState = ETurnState::None;
	
	AttackerUnit = Attacker;
	AttackTargetUnit = TargetUnit;

	AttackStartPos = AttackerUnit->GetActorLocation();
	FVector TargetPos = TargetUnit->GetActorLocation();

	float TargetHP = TargetUnit->Attributes->GetHealth();
	float AttackerDamage = AttackerUnit->Attributes->GetDamage();
	bWillKillUnit = (TargetHP - AttackerDamage) <= 0;

	if (bWillKillUnit)
	{
		AttackerImpactPos = TargetPos; 
	}
	else
	{
		FVector AttackDirection = (TargetPos - AttackStartPos).GetSafeNormal();
		AttackerImpactPos = TargetPos - (AttackDirection * 40.0f);
	}

	CurrentAttackPhase = EAttackPhase::Rush;

	if(AttackTimeline->IsPlaying())
		AttackTimeline->Stop();

	AttackTimeline->SetPlayRate(1.0f);

	float Distance = FVector::Dist(AttackStartPos, AttackerImpactPos);
    
	float DesiredDuration = FMath::Max(Distance / 2000, 0.1f);

	float CurveLength = 0.2f;
	if (AttackRushCurve)
	{
		float MinTime, MaxTime;
		AttackRushCurve->GetTimeRange(MinTime, MaxTime);
		CurveLength = MaxTime - MinTime;
	}

	float NewPlayRate = CurveLength / DesiredDuration;
	AttackTimeline->SetPlayRate(NewPlayRate);

	HighlightTilesForUnit(nullptr);
	AttackTimeline->PlayFromStart();
}

void AChessGridManager::HandleAttackProgress_Implementation(float Value)
{
	if(!AttackerUnit) return;

	FVector FinalDest = GridToWorld(WorldToGrid(AttackerImpactPos));

	switch (CurrentAttackPhase)
	{
		default: break;
		case EAttackPhase::Rush:
		{
			FVector NewLocation = FMath::Lerp(AttackStartPos, AttackerImpactPos, Value);
			AttackerUnit->SetActorLocation(NewLocation);

			FRotator LookAt = (AttackerImpactPos - AttackStartPos).Rotation();
			LookAt.Pitch = -20.0f * Value;
			AttackerUnit->UnitMeshComponent->SetRelativeRotation(LookAt);
			break;
		}
		case EAttackPhase::Return:
		{
			FVector NewLocation = FMath::Lerp(AttackerImpactPos, AttackStartPos, Value);
			AttackerUnit->SetActorLocation(NewLocation);
            
			FRotator LookAt = (AttackStartPos - AttackerImpactPos).Rotation();
			LookAt.Pitch = 10.0f * (1.0f - Value);
			AttackerUnit->UnitMeshComponent->SetRelativeRotation(LookAt);
			break;
		}
		case EAttackPhase::Capture:
		{
			FVector NewLocation = FMath::Lerp(AttackerImpactPos, FinalDest, Value);
			AttackerUnit->SetActorLocation(NewLocation);

			FRotator CurrentRot = AttackerUnit->UnitMeshComponent->GetRelativeRotation();
			CurrentRot.Pitch = FMath::Lerp(CurrentRot.Pitch, 0.0f, Value);
			AttackerUnit->UnitMeshComponent->SetRelativeRotation(CurrentRot);
				
			break;
		}
	}
}

void AChessGridManager::HandleAttackFinished_Implementation()
{
	if (!AttackerUnit) return;
	
	switch (CurrentAttackPhase)
	{
		default: break;
		case EAttackPhase::Rush:
		{
			AttackerUnit->DealDamageTo(AttackTargetUnit);
			PlayImpactFeedback(AttackerUnit, AttackTargetUnit, bWillKillUnit);
				
			if(bWillKillUnit)
			{
				CurrentAttackPhase = EAttackPhase::Capture;
				
				AttackTimeline->SetPlayRate(1.0f);
				AttackTimeline->PlayFromStart();
			}
			else
			{
				CurrentAttackPhase = EAttackPhase::Return;
				AttackTimeline->PlayFromStart();
			}
			break;
		}
		case EAttackPhase::Return:
		case EAttackPhase::Capture:
		{
			FIntPoint FinalGridPos;
			if (bWillKillUnit)
			{
				FinalGridPos = WorldToGrid(AttackerImpactPos);
				ClearTile(WorldToGrid(AttackStartPos));
				RegisterUnitAt(FinalGridPos, AttackerUnit);
			}
			else
			{
				FinalGridPos = WorldToGrid(AttackStartPos);
				AttackerUnit->SetActorLocation(AttackStartPos);
			}

			AttackerUnit->UnitMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
			AttackerUnit->OnUnitUnclickedFeedback();
			SelectedUnit = nullptr;
			IsUnitClicked = false;
			HighlightTilesForUnit(nullptr);
				
			PlayerActionFinished();
				
			break;
		}
	}
}

void AChessGridManager::PlayImpactFeedback_Implementation(AChessUnitBase* Attacker, AChessUnitBase* Target,bool bIsFatal) {}

void AChessGridManager::PlayerActionFinished_Implementation()
{
	GameMode->CheckWinCondition();
	GameMode->PlayerActionFinished(this);
}

bool AChessGridManager::IsTileOccupied(const FIntPoint& Pos) const
{
	return GridMap.FindRef(Pos) != nullptr;
}

EChessTeam AChessGridManager::GetTeamAt(const FIntPoint& Pos) const
{
	if (const AChessUnitBase* const FoundUnit = GridMap.FindRef(Pos))
	{
		if (FoundUnit) return (FoundUnit)->Team;
	}
	return EChessTeam::None;
}
