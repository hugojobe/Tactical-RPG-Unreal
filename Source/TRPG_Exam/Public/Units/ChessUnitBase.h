// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/ChessAttributeSet.h"
#include "ResearchTree/ChessSkillTreeSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "ChessUnitBase.generated.h"

class AChessAIController;
class AChessGridManager;

USTRUCT(BlueprintType)
struct FMovementRule
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMovementStyle MovementStyle;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector2D> MoveVectors;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RangeLimit = 8;
};

USTRUCT(BlueprintType)
struct FUnitStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MaxHP = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 CurrentHP = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Damage = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 KillRpReward = 0;
};

USTRUCT(BlueprintType)
struct FChessUnitData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balancing")
	int32 MaxHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balancing")
	int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Balancing")
	int32 KillRpReward;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FMovementRule MovementRule;
};

class AChessUnitBase;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitHighlightUpdated, AChessUnitBase*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitClicked, AChessUnitBase*, Unit);

UCLASS()
class TRPG_EXAM_API AChessUnitBase : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AChessUnitBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Personality")
	float Aggressiveness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Personality")
	float SelfPreservation = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UChessAttributeSet* Attributes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
	TArray<TSubclassOf<UGameplayEffect>> PassiveEffects;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
	TSubclassOf<UGameplayAbility> ActiveAbility;

	FGameplayAbilitySpecHandle ActiveAbilityHandle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Skills")
	FIntPoint PendingAbilityTarget = FIntPoint(-1, -1);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	bool bHasMoved = false;
	
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "AI Personality")
	int32 TurnsSinceLastAction = 0;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	UFUNCTION(BlueprintCallable, Category = "GAS")
	int DealDamageTo(AChessUnitBase* Target);
	
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	void ApplySkillTreeProgression();

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	AChessGridManager* GridManager;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Grid")
	FVector2D GridCoordinates;
	
	FIntPoint LastPosition;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	EChessUnitType UnitType;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Stats")
	FUnitStats Stats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	EChessTeam Team;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FMovementRule MovementRule;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	UDataTable* UnitStatsTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
	UStaticMeshComponent* UnitMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
	USceneComponent* UnitHUD;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUnitHighlightUpdated OnUnitHoverUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUnitClicked OnUnitClicked;

	UFUNCTION(BlueprintCallable, Category = "Setup")
	void InitializeStats();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnUnitClickedFeedback();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnUnitUnclickedFeedback();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat Logic")
	void ToggleUnitHUD(bool Visible, bool IsEnemy, AChessUnitBase* SelectedInstigator);
	
protected:
	virtual void NotifyActorBeginCursorOver() override;
	virtual void NotifyActorEndCursorOver() override;
	virtual void NotifyActorOnClicked(FKey ButtonPressed = EKeys::LeftMouseButton) override;
};

