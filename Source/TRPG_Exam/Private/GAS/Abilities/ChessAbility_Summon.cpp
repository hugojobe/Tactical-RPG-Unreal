// By hugojobe.

#include "GAS/Abilities/ChessAbility_Summon.h"

#include "HairStrandsInterface.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"
#include "Kismet/GameplayStatics.h"

UChessAbility_Summon::UChessAbility_Summon()
{
	AbilityType = EChessAbilityType::Summon;
	Targeting = EChessAbilityTargeting::AllyUnit;
	CooldownTurns = 3;
}

TArray<FIntPoint> UChessAbility_Summon::GetValidTargets(const AChessUnitBase* Unit) const
{
	TArray<FIntPoint> Result;
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return Result;

	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);
	static const FIntPoint Neighbors[] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };

	bool bHasFreeAdjacentTile = false;
	for (const FIntPoint& Offset : Neighbors)
	{
		const FIntPoint Pos = Origin + Offset;
		if (Grid->IsValidGridPos(Pos) && !Grid->GetUnitAt(Pos) && !Grid->IsTileBlockedAt(Pos))
		{
			bHasFreeAdjacentTile = true;
			break;
		}
	}
	if (!bHasFreeAdjacentTile) return Result;

	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(Unit->GetWorld(), AChessUnitBase::StaticClass(), AllUnits);

	for (AActor* Actor : AllUnits)
	{
		const AChessUnitBase* Other = Cast<AChessUnitBase>(Actor);
		if (!Other || Other == Unit) continue;
		if (Other->Team != Unit->Team) continue;
		if (Other->Attributes->GetHealth() <= 0.f) continue;

		const FIntPoint OtherPos(Other->GridCoordinates.X, Other->GridCoordinates.Y);
		const int32 ChebyshevDist = FMath::Max(FMath::Abs(OtherPos.X - Origin.X), FMath::Abs(OtherPos.Y - Origin.Y));
		if (ChebyshevDist <= 1) continue;
		
		Result.Add(OtherPos);
	}
	return Result;
}

void UChessAbility_Summon::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	AChessGridManager* Grid = Unit->GridManager;
	static const FIntPoint Neighbors[] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);

	FIntPoint BestTile(-1, -1);
	int32 BestDist = MAX_int32;
	for (const FIntPoint& Offset : Neighbors)
	{
		const FIntPoint Pos = Origin + Offset;
		if (!Grid->IsValidGridPos(Pos) || Grid->GetUnitAt(Pos)) continue;

		const int32 Dist = FMath::Abs(Pos.X - TargetPos.X) + FMath::Abs(Pos.Y - TargetPos.Y);
		if (Dist < BestDist) { BestDist = Dist; BestTile = Pos; }
	}

	if (BestTile.X < 0) return;

	if (AChessUnitBase* Ally = Grid->GetUnitAt(TargetPos))
	{
		Grid->TeleportUnitTo(Ally, BestTile);
		OnAbilityAffectedUnitFeedback(AbilityType, Unit, Ally);
	}
}
