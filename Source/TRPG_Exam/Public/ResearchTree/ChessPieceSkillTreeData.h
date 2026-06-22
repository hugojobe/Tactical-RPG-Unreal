// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"
#include "Enums.h"
#include "ChessPieceSkillTreeData.generated.h"

USTRUCT(BlueprintType)
struct FSkillNodeData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName NodeID;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* Icon = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESkillNodeType Type = ESkillNodeType::Passive;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RpCost = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> RequiredNodeIDs;
	
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "Type == ESkillNodeType::Passive"))
	TSubclassOf<UGameplayEffect> PassiveEffect;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "Type == ESkillNodeType::Equippable"))
	TSubclassOf<UGameplayAbility> GrantedAbility;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "Type == ESkillNodeType::Equippable"))
	int32 AbilityLevel = 1;
};

UCLASS()
class TRPG_EXAM_API UChessPieceSkillTreeData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	EChessUnitType UnitType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nodes")
	TArray<FSkillNodeData> Nodes;
	
	const FSkillNodeData* FindNode(FName NodeID) const
	{
		return Nodes.FindByPredicate([NodeID](const FSkillNodeData& N)
		{
			return N.NodeID == NodeID;
		});
	}
};
