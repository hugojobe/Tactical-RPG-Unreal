// By hugojobe.

#include "GAS/Abilities/ChessAbility_Bastion.h"
#include "GAS/ChessAbilityBase.h"
#include "Managers/ChessGridManager.h"
#include "Units/ChessUnitBase.h"

UChessAbility_Bastion::UChessAbility_Bastion()
{
	AbilityType = EChessAbilityType::Bastion;
	Targeting = EChessAbilityTargeting::Self;
	CooldownTurns = 3;
}

void UChessAbility_Bastion::ExecuteAbilityEffect(AChessUnitBase* Unit, FIntPoint TargetPos)
{
	AChessGridManager* Grid = GetGridManager(Unit);
	if (!Grid) return;

	const FIntPoint Origin(Unit->GridCoordinates.X, Unit->GridCoordinates.Y);
	static const FIntPoint RowDirections[] = { {1,0}, {-1,0} };

	for (const FIntPoint& Dir : RowDirections)
	{
		FIntPoint Pos = Origin + Dir;
		while (Grid->IsValidGridPos(Pos) && !Grid->GetUnitAt(Pos) && !Grid->IsTileBlockedAt(Pos))
		{
			Grid->BlockTile(Pos, WallTurns);
			Pos += Dir;
		}
	}

	FGameplayEffectContextHandle Context = Unit->AbilitySystemComponent->MakeEffectContext();
	FGameplayEffectSpecHandle RootedSpec = Unit->AbilitySystemComponent->MakeOutgoingSpec(RootedEffectClass, 1.f, Context);
	if (RootedSpec.IsValid())
	{
		RootedSpec.Data->SetStackCount(RootedTurns + 2);
		Unit->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*RootedSpec.Data.Get());
	}
}
