// By hugojobe.

#include "Units/ChessUnitBase.h"
#include "Components/StaticMeshComponent.h"
#include "Framwork/ChessGameMode.h"
#include "Managers/ChessGridManager.h"

AChessUnitBase::AChessUnitBase()
{
	PrimaryActorTick.bCanEverTick = true;
	Stats.MaxHP = 1;
	Stats.CurrentHP = Stats.MaxHP;
	Stats.Damage = 0;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	UnitMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitMesh"));
	UnitMeshComponent->SetupAttachment(RootComponent);

	UnitHUD = CreateDefaultSubobject<USceneComponent>(TEXT("AttackableVisualRoot"));
	UnitHUD->SetHiddenInGame(true, true);
	UnitHUD->SetupAttachment(UnitMeshComponent);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	Attributes = CreateDefaultSubobject<UChessAttributeSet>(TEXT("Attributes"));
}

void AChessUnitBase::BeginPlay()
{
	Super::BeginPlay();
	InitializeStats();
}

void AChessUnitBase::InitializeStats()
{
	if(!UnitStatsTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[HJ] UnitStatsTable is missing on %s"), *GetName());
	}

	const UEnum* EnumPtr = StaticEnum<EChessUnitType>();
	if(!EnumPtr) return;

	FName RowName = FName(*EnumPtr->GetNameStringByValue(static_cast<int64>(UnitType)));
	static const FString ContextString(TEXT("Unit Stats"));
	
	FChessUnitData* RowData = UnitStatsTable->FindRow<FChessUnitData>(RowName, ContextString, true);

	if(RowData && AbilitySystemComponent)
	{
		Stats.MaxHP = RowData->MaxHP;
		Stats.CurrentHP = RowData->MaxHP;
		Stats.Damage = RowData->Damage;
		Stats.KillRpReward = RowData->KillRpReward;
		MovementRule = RowData->MovementRule;

		Attributes->InitHealth(RowData->MaxHP);
		Attributes->InitDamage(RowData->Damage);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[HJ] Could not find row '%s' in UnitStatsTable!"), *RowName.ToString());
	}
}

void AChessUnitBase::DealDamageTo(AChessUnitBase* Target)
{
	if (!Target || !Target->GetAbilitySystemComponent()) return;

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	UGameplayEffect* DamageEffect = NewObject<UGameplayEffect>(GetTransientPackage(), FName("BasicDamage"));
	DamageEffect->DurationPolicy = EGameplayEffectDurationType::Instant;

	int32 Damage = Attributes->GetDamage();
	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UChessAttributeSet::GetHealthAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;
	ModifierInfo.ModifierMagnitude = FScalableFloat(-Damage);

	DamageEffect->Modifiers.Add(ModifierInfo);

	Target->GetAbilitySystemComponent()->ApplyGameplayEffectToSelf(DamageEffect, 1.0f, Context);
}

void AChessUnitBase::NotifyActorBeginCursorOver()
{
	Super::NotifyActorBeginCursorOver();
	
	if (OnUnitHoverUpdated.IsBound())
	{
		OnUnitHoverUpdated.Broadcast(this);
		AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode());
		if (!GridManager->IsUnitClicked && GameMode->CurrentTurn == ETurnState::Player)
			ToggleUnitHUD(true, Team == EChessTeam::Enemy, this);
	}
}

void AChessUnitBase::NotifyActorEndCursorOver()
{
	Super::NotifyActorEndCursorOver();
	
	if (OnUnitHoverUpdated.IsBound())
	{
		OnUnitHoverUpdated.Broadcast(nullptr);
		if (!GridManager->IsUnitClicked)
			ToggleUnitHUD(false, Team == EChessTeam::Enemy, this);
	}
}

void AChessUnitBase::NotifyActorOnClicked(FKey ButtonPressed)
{
	Super::NotifyActorOnClicked(ButtonPressed);
	
	if (OnUnitClicked.IsBound())
	{
		OnUnitClicked.Broadcast(this);
	}
}

void AChessUnitBase::OnUnitClickedFeedback_Implementation(){}
void AChessUnitBase::OnUnitUnclickedFeedback_Implementation() {}

void AChessUnitBase::ToggleUnitHUD_Implementation(bool Visible, bool IsEnemy, AChessUnitBase* SelectedInstigator) {}