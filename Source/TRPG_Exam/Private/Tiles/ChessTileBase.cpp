// By hugojobe.


#include "Tiles/ChessTileBase.h"
#include "Components/StaticMeshComponent.h"
#include "Managers/ChessGridManager.h"

AChessTileBase::AChessTileBase()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	TileMesh->SetupAttachment(RootComponent);
}

void AChessTileBase::SetBlockedVisual_Implementation(bool bBlocked) {}

void AChessTileBase::NotifyActorOnClicked(FKey ButtonPressed)
{
	Super::NotifyActorOnClicked(ButtonPressed);
	
	if (OnTileClicked.IsBound())
	{
		OnTileClicked.Broadcast(this);
	}
}

void AChessTileBase::SetHighlight_Implementation(ETileInteraction InteractionType)
{
	switch(InteractionType)
	{
		default: break;
		case ETileInteraction::None:
			if(AChessUnitBase* Unit = GridManager->GetUnitAt(GridPosition))
			{
				Unit->ToggleUnitHUD(false, Unit->Team == EChessTeam::Enemy, nullptr);
			}
			break;
		case ETileInteraction::Move:
			break;
		case ETileInteraction::Attack:
			if(AChessUnitBase* Unit = GridManager->GetUnitAt(GridPosition))
			{
				Unit->ToggleUnitHUD(true, Unit->Team == EChessTeam::Enemy, GridManager->SelectedUnit);
			}
			break;
	}
}
