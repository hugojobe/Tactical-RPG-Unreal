// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "GameFramework/Actor.h"
#include "ChessTileBase.generated.h"

class AChessGridManager;
class AChessTileBase;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTileClicked, AChessTileBase*, Tile);

UCLASS()
class TRPG_EXAM_API AChessTileBase : public AActor
{
	GENERATED_BODY()
	
public:    
	AChessTileBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
	UStaticMeshComponent* TileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	FIntPoint GridPosition;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	AChessGridManager* GridManager;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	FVector TileWorldLocation;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Visuals")
	void SetHighlight(ETileInteraction HighlightMode);
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTileClicked OnTileClicked;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Visuals")
	void SetBlockedVisual(bool bBlocked);
	
protected:
	virtual void NotifyActorOnClicked(FKey ButtonPressed = EKeys::LeftMouseButton) override;
};
