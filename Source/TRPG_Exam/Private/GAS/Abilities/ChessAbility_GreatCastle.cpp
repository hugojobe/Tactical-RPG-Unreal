// By hugojobe.

#include "GAS/Abilities/ChessAbility_GreatCastle.h"

#include "GAS/Abilities/ChessAbility_Swap.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"

UChessAbility_GreatCastle::UChessAbility_GreatCastle()
{
	AbilityType = EChessAbilityType::GreatCastle;
	Targeting = EChessAbilityTargeting::AllyUnit;
	CooldownTurns = 3;
}

TArray<FIntPoint> UChessAbility_GreatCastle::GetValidTargets(const AChessUnitBase* Unit) const
{
	TArray<FIntPoint> Result;
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return Result;
	
	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);
	const FIntPoint OrthogonalsPos[] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
	
	for (const FIntPoint& Dir : OrthogonalsPos)
	{
		FIntPoint Pos = Origin + Dir;
		while (Grid->IsValidGridPos(Pos))
		{
			if (AChessUnitBase* Other = Grid->GetUnitAt(Pos))
			{
				if (Other->Team == Unit->Team && Other->Attributes->GetHealth() > 0.f)
				{
					Result.Add(Pos);
				} 
				break;
			}
			Pos += Dir;
		}
	}
	return Result;
}
