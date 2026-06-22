// By hugojobe.

#include "Units/ChessUnitBase.h"

#include "AI/ChessAIController.h"
#include "Components/StaticMeshComponent.h"
#include "Framework/ChessGameMode.h"
#include "Managers/ChessGridManager.h"

AChessUnitBase::AChessUnitBase()
{
	PrimaryActorTick.bCanEverTick = true;
	Stats.MaxHP = 1;
	Stats.CurrentHP = 1;
	Stats.Damage = 1;
	Stats.KillRpReward = 1;

	MovementRule.MovementStyle = EMovementStyle::Pawn; 
	MovementRule.MoveVectors.Empty(); 
	MovementRule.RangeLimit = 0;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	UnitMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitMesh"));
	UnitMeshComponent->SetupAttachment(RootComponent);

	UnitHUD = CreateDefaultSubobject<USceneComponent>(TEXT("AttackableVisualRoot"));
	UnitHUD->SetHiddenInGame(true, true);
	UnitHUD->SetupAttachment(UnitMeshComponent);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	Attributes = CreateDefaultSubobject<UChessAttributeSet>(TEXT("Attributes"));

	if(Team == EChessTeam::Enemy)
	{
		AIControllerClass = AChessAIController::StaticClass();
		AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	} else
	{
		AIControllerClass = nullptr;
		AutoPossessAI = EAutoPossessAI::Disabled;
	}
}

void AChessUnitBase::BeginPlay()
{
	Super::BeginPlay();
	InitializeStats();
}

void AChessUnitBase::ApplySkillTreeProgression()
{
	if (Team != EChessTeam::Player) return;
	if (!AbilitySystemComponent) return;
	
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI) return;
	
	UChessSkillTreeSubsystem* SkillTree = GI->GetSubsystem<UChessSkillTreeSubsystem>();
	if (!SkillTree) return;
	
	TArray<FSkillNodeData> PassiveNodes = SkillTree->GetPurchasedPassiveNodes(UnitType);

	for (const FSkillNodeData& Node : PassiveNodes)
	{
		if (!Node.PassiveEffect) continue;

		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();

		FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(Node.PassiveEffect, 1.f, Context);

		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	
	FSkillNodeData EquippedNode;
	if (SkillTree->GetEquippedAbilityNode(UnitType, EquippedNode))
	{
		if (EquippedNode.GrantedAbility)
		{
			FGameplayAbilitySpec AbilitySpec(
				EquippedNode.GrantedAbility,
				EquippedNode.AbilityLevel,
				INDEX_NONE,
				this
			);
			AbilitySystemComponent->GiveAbility(AbilitySpec);
		}
	}
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

int AChessUnitBase::DealDamageTo(AChessUnitBase* Target)
{
	if (!Target || !Target->GetAbilitySystemComponent()) return 0;

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
	
	return Damage;
}

void AChessUnitBase::NotifyActorBeginCursorOver()
{
	Super::NotifyActorBeginCursorOver();
	
	if (OnUnitHoverUpdated.IsBound())
	{
		OnUnitHoverUpdated.Broadcast(this);
		AChessGameMode* GameMode = nullptr;
		if (UWorld* World = GetWorld())
		{
			GameMode = Cast<AChessGameMode>(World->GetAuthGameMode());
		}

		if (GridManager && GameMode)
		{
			if (!GridManager->IsUnitClicked && GameMode->CurrentTurn == ETurnState::Player)
			{
				ToggleUnitHUD(true, Team == EChessTeam::Enemy, this);
			}
		}
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