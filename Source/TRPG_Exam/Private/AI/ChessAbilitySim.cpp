#include "AI/ChessAbilitySim.h"

namespace
{
	const FIntPoint Neighbors8[] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
	const FIntPoint Orthogonals[] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
	const FIntPoint Diagonals[] = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };
	const FIntPoint KnightOffsets[] = { {1,2}, {2,1}, {2,-1}, {1,-2}, {-1,-2}, {-2,-1}, {-2,1}, {-1,2} };

	bool IsFreeTile(const FVirtualBoard& Board, const FIntPoint& Pos)
	{
		return Board.IsValidGridPos(Pos, Board.Width, Board.Height) && !Board.IsTileOccupied(Pos) && !Board.IsTileBlocked(Pos);
	}
}

TArray<FIntPoint> ChessAbilitySim::GetAbilityTargets(const FVirtualBoard& Board, const FIntPoint& UnitPos, EChessAbilityType Type)
{
	TArray<FIntPoint> Result;
	const FVirtualUnit& Self = Board.Grid[Board.GetIndex(UnitPos)];
	const EChessTeam MyTeam = Self.Team;
	const int32 Dir = (MyTeam == EChessTeam::Player) ? 1 : -1;

	switch (Type)
	{
	case EChessAbilityType::Caltrop:
	{
		const FIntPoint ForwardDiagonals[] = { FIntPoint(UnitPos.X - 1, UnitPos.Y + Dir), FIntPoint(UnitPos.X + 1, UnitPos.Y + Dir) };
		for (const FIntPoint& Pos : ForwardDiagonals)
			if (IsFreeTile(Board, Pos)) Result.Add(Pos);
		break;
	}
	case EChessAbilityType::Translation:
	{
		for (const FIntPoint& Offset : Orthogonals)
			if (IsFreeTile(Board, UnitPos + Offset)) Result.Add(UnitPos + Offset);
		break;
	}
	case EChessAbilityType::DoubleLeap:
	{
		for (const FIntPoint& Hop : KnightOffsets)
			if (IsFreeTile(Board, UnitPos + Hop)) Result.Add(UnitPos + Hop);
		break;
	}
	case EChessAbilityType::Escape:
	{
		for (int32 OffsetX = -2; OffsetX <= 2; OffsetX++)
			for (int32 OffsetY = -2; OffsetY <= 2; OffsetY++)
				if ((OffsetX != 0 || OffsetY != 0) && IsFreeTile(Board, UnitPos + FIntPoint(OffsetX, OffsetY)))
					Result.Add(UnitPos + FIntPoint(OffsetX, OffsetY));
		break;
	}
	case EChessAbilityType::Blessing:
	{
		for (const FIntPoint& DiagDir : Diagonals)
		{
			FIntPoint Pos = UnitPos + DiagDir;
			while (Board.IsValidGridPos(Pos, Board.Width, Board.Height))
			{
				if (Board.IsTileBlocked(Pos)) break;
				if (Board.IsTileOccupied(Pos))
				{
					const FVirtualUnit& Other = Board.Grid[Board.GetIndex(Pos)];
					if (Other.Team == MyTeam && Other.UnitStats.CurrentHP < Other.UnitStats.MaxHP)
						Result.Add(Pos);
					break;
				}
				Pos += DiagDir;
			}
		}
		break;
	}
	case EChessAbilityType::Swap:
	{
		for (const FIntPoint& Offset : Neighbors8)
		{
			const FIntPoint Pos = UnitPos + Offset;
			if (Board.IsValidGridPos(Pos, Board.Width, Board.Height) && Board.GetTeamAt(Pos) == MyTeam)
				Result.Add(Pos);
		}
		break;
	}
	case EChessAbilityType::GreatCastle:
	{
		for (const FIntPoint& OrthoDir : Orthogonals)
		{
			FIntPoint Pos = UnitPos + OrthoDir;
			while (Board.IsValidGridPos(Pos, Board.Width, Board.Height))
			{
				if (Board.IsTileBlocked(Pos)) break;
				if (Board.IsTileOccupied(Pos))
				{
					if (Board.GetTeamAt(Pos) == MyTeam) Result.Add(Pos);
					break;
				}
				Pos += OrthoDir;
			}
		}
		break;
	}
	case EChessAbilityType::Summon:
	{
		bool bHasFreeAdjacent = false;
		for (const FIntPoint& Offset : Neighbors8)
			if (IsFreeTile(Board, UnitPos + Offset)) { bHasFreeAdjacent = true; break; }
		if (!bHasFreeAdjacent) break;

		for (int i = 0; i < Board.Width * Board.Height; ++i)
		{
			if (Board.Grid[i].Team != MyTeam) continue;
			const FIntPoint Pos(i % Board.Width, i / Board.Width);
			if (Pos == UnitPos) continue;
			if (FMath::Max(FMath::Abs(Pos.X - UnitPos.X), FMath::Abs(Pos.Y - UnitPos.Y)) <= 1) continue;
			Result.Add(Pos);
		}
		break;
	}
	case EChessAbilityType::Bastion:
	case EChessAbilityType::Execution:
	{
		Result.Add(UnitPos);
		break;
	}
	case EChessAbilityType::Intimidation:
	case EChessAbilityType::Punishment:
	{
		for (const FIntPoint& Offset : Neighbors8)
		{
			const EChessTeam Team = Board.GetTeamAt(UnitPos + Offset);
			if (Team != EChessTeam::None && Team != MyTeam) { Result.Add(UnitPos); break; }
		}
		break;
	}
	case EChessAbilityType::Repulsion:
	{
		for (const FIntPoint& Offset : Neighbors8)
		{
			if (Board.GetTeamAt(UnitPos + Offset) != EChessTeam::None) { Result.Add(UnitPos); break; }
		}
		break;
	}
	default: break;
	}
	
	return Result;
}

void ChessAbilitySim::ApplyAbility(FVirtualBoard& Board, const FIntPoint& UnitPos, EChessAbilityType Type, const FIntPoint& TargetPos)
{
	const int32 SelfIdx = Board.GetIndex(UnitPos);
	FVirtualUnit Self = Board.Grid[SelfIdx];
	const EChessTeam MyTeam = Self.Team;

	switch (Type)
	{
	case EChessAbilityType::Caltrop:
		Board.BlockedTurns[Board.GetIndex(TargetPos)] = 2;
		break;

	case EChessAbilityType::Translation:
	case EChessAbilityType::DoubleLeap:
	case EChessAbilityType::Escape:
		Board.MoveUnit(UnitPos, TargetPos, Self);
		break;

	case EChessAbilityType::Blessing:
	{
		FVirtualUnit& Target = Board.Grid[Board.GetIndex(TargetPos)];
		Target.UnitStats.CurrentHP = FMath::Min(Target.UnitStats.CurrentHP + 1, Target.UnitStats.MaxHP);
		break;
	}
	case EChessAbilityType::Swap:
	case EChessAbilityType::GreatCastle:
	{
		const int32 TargetIdx = Board.GetIndex(TargetPos);
		const FVirtualUnit Other = Board.Grid[TargetIdx];
		Board.Grid[TargetIdx] = Self;
		Board.Grid[SelfIdx] = Other;
		break;
	}
	case EChessAbilityType::Summon:
	{
		// Same deterministic rule as the real ability: free adjacent tile closest to the ally
		FIntPoint BestTile(-1, -1);
		int32 BestDist = MAX_int32;
		for (const FIntPoint& Offset : Neighbors8)
		{
			const FIntPoint Pos = UnitPos + Offset;
			if (!IsFreeTile(Board, Pos)) continue;
			const int32 Dist = FMath::Abs(Pos.X - TargetPos.X) + FMath::Abs(Pos.Y - TargetPos.Y);
			if (Dist < BestDist) { BestDist = Dist; BestTile = Pos; }
		}
		if (BestTile.X >= 0)
		{
			Board.MoveUnit(TargetPos, BestTile, Board.Grid[Board.GetIndex(TargetPos)]);
		}
		break;
	}
	case EChessAbilityType::Bastion:
		Board.Grid[SelfIdx].UnitStats.CurrentHP += 2;
		Board.Grid[SelfIdx].RootedTurns = 2;
		break;

	case EChessAbilityType::Execution:
		Board.Grid[SelfIdx].UnitStats.Damage += 2;
		break;

	case EChessAbilityType::Intimidation:
	{
		for (const FIntPoint& Offset : Neighbors8)
		{
			const FIntPoint Pos = UnitPos + Offset;
			if (!Board.IsValidGridPos(Pos, Board.Width, Board.Height)) continue;
			FVirtualUnit& Other = Board.Grid[Board.GetIndex(Pos)];
			if (Other.Team != EChessTeam::None && Other.Team != MyTeam)
				Other.UnitStats.Damage = FMath::Max(0, Other.UnitStats.Damage - 1);
		}
		break;
	}
	case EChessAbilityType::Punishment:
	{
		for (const FIntPoint& Offset : Neighbors8)
		{
			const FIntPoint Pos = UnitPos + Offset;
			if (!Board.IsValidGridPos(Pos, Board.Width, Board.Height)) continue;
			FVirtualUnit& Other = Board.Grid[Board.GetIndex(Pos)];
			if (Other.Team == EChessTeam::None || Other.Team == MyTeam) continue;
			Other.UnitStats.CurrentHP -= 1;
			if (Other.UnitStats.CurrentHP <= 0) Other.Team = EChessTeam::None;
		}
		break;
	}
	case EChessAbilityType::Repulsion:
	{
		for (const FIntPoint& Offset : Neighbors8)
		{
			const FIntPoint Pos = UnitPos + Offset;
			if (!Board.IsValidGridPos(Pos, Board.Width, Board.Height) || !Board.IsTileOccupied(Pos)) continue;
			const FIntPoint Destination = Pos + Offset;
			if (!IsFreeTile(Board, Destination)) continue;
			Board.MoveUnit(Pos, Destination, Board.Grid[Board.GetIndex(Pos)]);
		}
		break;
	}
	default: break;
	}
}
