#include "AI/ChessAIPlanner.h"

#include "Framwork/ChessGameMode.h"
#include "Managers/ChessGridManager.h"


UVirtualBoard::UVirtualBoard() {}

bool UVirtualBoard::IsValidGridPos(const FIntPoint& Pos, int BoardWidth, int BoardHeight) const
{
	return (Pos.X >= 0 && Pos.X < BoardWidth &&
			Pos.Y >= 0 && Pos.Y < BoardHeight);
}

bool UVirtualBoard::IsTileOccupied(const FIntPoint& Pos) const
{
	return Units.Contains(Pos);
}

EChessTeam UVirtualBoard::GetTeamAt(const FIntPoint& Pos) const
{
	if (!IsValidGridPos(Pos, Width, Height) || !IsTileOccupied(Pos)) return EChessTeam::None;
	return Units.FindRef(Pos).Team;
}

UVirtualBoard* UChessAIPlanner::GenerateVirtualBoard(AChessGridManager* Manager)
{
	UVirtualBoard* VBoard = NewObject<UVirtualBoard>();
	VBoard->Width = Manager->BoardWidth;
	VBoard->Height = Manager->BoardHeight;

	for (auto Tile : Manager->TileActors)
	{
		if (AChessUnitBase* UnitOnTile = Manager->GetUnitAt(Tile.Key))
		{
			FVirtualUnit VUnit;
			VUnit.UnitMovementRule = UnitOnTile->MovementRule;
			VUnit.UnitStats = UnitOnTile->Stats;
			VUnit.Team = UnitOnTile->Team;
			
			VBoard->Units.Add(Tile.Key, VUnit);
		}
	}
	
	return VBoard;
}

TMap<FIntPoint, float> UChessAIPlanner::GenerateThreatMap(AChessGridManager* GridManager)
{
	TMap<FIntPoint, float> ThreatMap;
	if (!GridManager) return ThreatMap;

	for (const auto& [Location, Unit] : GridManager->GameMode->VirtualBoard->Units)
	{
		if (Unit.Team == EChessTeam::Enemy) continue;

		const float DamageValue = Unit.UnitStats.Damage;
		const bool bIsPawn = (Unit.UnitMovementRule.MovementStyle == EMovementStyle::Pawn);

		TMap<FIntPoint, ETileInteraction> LegalMoves = GridManager->GetValidMoves(Unit, Location, GridManager->GameMode->VirtualBoard,true);
		
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

FChessDecision UChessAIPlanner::GetUnitDecision()
{
	FChessDecision Decision;
	Decision.Type = EChessDecisionType::Null;
	Decision.Score = -BIG_NUMBER;
	
	if (!GridManager)
		return Decision;
	
	return Decision;
}
