// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Enums.h"
#include "ResearchTree/ChessPieceSkillTreeData.h"
#include "Framework/ChessSaveGame.h"
#include "ChessSkillTreeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodePurchased, EChessUnitType, PieceType, FName, NodeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeEquipped, EChessUnitType, PieceType, FName, NodeID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRpChanged, int32, NewRP);

UCLASS()
class TRPG_EXAM_API UChessSkillTreeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	TMap<EChessUnitType, UChessPieceSkillTreeData*> SkillTreeDataMap;
	
	UPROPERTY(BlueprintAssignable)
	FOnNodePurchased OnNodePurchased;
	
	UPROPERTY(BlueprintAssignable)
	FOnNodeEquipped OnNodeEquipped;
	
	UPROPERTY(BlueprintAssignable)
	FOnRpChanged OnRpChanged;
	
	UFUNCTION(BlueprintCallable, Category = "ResearchTree|Save")
	void SaveProgression();
	
	UFUNCTION(BlueprintCallable, Category = "ResearchTree|Save")
	void LoadProgression();
	
	UFUNCTION(BlueprintCallable, Category = "ResearchTree|RP")
	void AddRP(int32 Amount);
	
	UFUNCTION(BlueprintCallable, Category = "ResearchTree|RP")
	int32 GetRP() const;
	
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Purchase")
	bool TryPurchaseNode(EChessUnitType PieceType, FName NodeID);
	
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Equip")
	bool TryEquipNode(EChessUnitType PieceType, FName NodeID);
	
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Equip")
	void UnequipNode(EChessUnitType PieceType);
	
	UFUNCTION(BlueprintPure, Category = "SkillTree|Query")
	bool IsNodePurchased(EChessUnitType PieceType, FName NodeID) const;
	
	UFUNCTION(BlueprintPure, Category = "SkillTree|Query")
	bool IsNodeUnlocked(EChessUnitType PieceType, FName NodeID) const;
	
	UFUNCTION(BlueprintPure, Category = "SkillTree|Query")
	FName GetEquippedNode(EChessUnitType PieceType) const;
	
	UFUNCTION(BlueprintPure, Category = "SkillTree|Query")
	UChessPieceSkillTreeData* GetTreeDataForPiece(EChessUnitType PieceType) const;
	
	UFUNCTION(BlueprintPure, Category = "SkillTree|Query")
	TArray<FSkillNodeData> GetPurchasedPassiveNodes(EChessUnitType PieceType) const;
	
	UFUNCTION(BlueprintPure, Category = "SkillTree|Query")
	bool GetEquippedAbilityNode(EChessUnitType PieceType, FSkillNodeData& OutNode) const;

private:
	UPROPERTY()
	UChessSaveGame* SaveGame = nullptr;

	static const FString SaveSlotName;
	static const int32 SaveUserIndex;

	bool AreRequirementsMet(EChessUnitType PieceType, const FSkillNodeData& Node) const;
};
