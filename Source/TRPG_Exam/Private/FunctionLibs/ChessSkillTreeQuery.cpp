// By hugojobe.


#include "FunctionLibs/ChessSkillTreeQuery.h"
#include "ResearchTree/ChessSkillTreeSubsystem.h"
#include "Kismet/GameplayStatics.h"

UChessSkillTreeSubsystem* UChessSkillTreeQuery::GetSkillTreeSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;
    
	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!GI) return nullptr;

	return GI->GetSubsystem<UChessSkillTreeSubsystem>();
}

int32 UChessSkillTreeQuery::GetPlayerRP(const UObject* WorldContextObject)
{
	UChessSkillTreeSubsystem* SkillTree = GetSkillTreeSubsystem(WorldContextObject);
	return SkillTree ? SkillTree->GetRP() : 0;
}

void UChessSkillTreeQuery::CheatAddRP(const UObject* WorldContextObject, int32 Amount)
{
	if (UChessSkillTreeSubsystem* SkillTree = GetSkillTreeSubsystem(WorldContextObject)) SkillTree->AddRP(Amount);
}

bool UChessSkillTreeQuery::CanAffordNode(const UObject* WorldContextObject, EChessUnitType PieceType, FName NodeID)
{
	UChessSkillTreeSubsystem* SkillTree = GetSkillTreeSubsystem(WorldContextObject);
	if (!SkillTree) return false;

	UChessPieceSkillTreeData* TreeData = SkillTree->GetTreeDataForPiece(PieceType);
	if (!TreeData) return false;

	const FSkillNodeData* Node = TreeData->FindNode(NodeID);
	if (!Node) return false;

	return SkillTree->GetRP() >= Node->RpCost;
}

ESkillNodeState UChessSkillTreeQuery::GetNodeState(const UObject* WorldContextObject, EChessUnitType PieceType, FName NodeID)
{
	UChessSkillTreeSubsystem* SkillTree = GetSkillTreeSubsystem(WorldContextObject);
	if (!SkillTree) return ESkillNodeState::Locked;

	if (const bool bPurchased = SkillTree->IsNodePurchased(PieceType, NodeID))
	{
		UChessPieceSkillTreeData* TreeData = SkillTree->GetTreeDataForPiece(PieceType);
		const FSkillNodeData* Node = TreeData ? TreeData->FindNode(NodeID) : nullptr;

		if (Node && Node->Type == ESkillNodeType::Equippable)
		{
			const FName Equipped = SkillTree->GetEquippedNode(PieceType);
			return (Equipped == NodeID) ? ESkillNodeState::Equipped 
										  : ESkillNodeState::Purchased;
		}
		return ESkillNodeState::Purchased;
	}

	const bool bUnlocked = SkillTree->IsNodeUnlocked(PieceType, NodeID);
	return bUnlocked ? ESkillNodeState::Unlockable : ESkillNodeState::Locked;
}

bool UChessSkillTreeQuery::PurchaseOrEquipNode(const UObject* WorldContextObject, EChessUnitType PieceType, FName NodeID, bool& bOutWasPurchase)
{
	UChessSkillTreeSubsystem* SkillTree = GetSkillTreeSubsystem(WorldContextObject);
	if (!SkillTree) { bOutWasPurchase = false; return false; }

	const bool bAlreadyPurchased = SkillTree->IsNodePurchased(PieceType, NodeID);

	if (!bAlreadyPurchased)
	{
		bOutWasPurchase = true;
		return SkillTree->TryPurchaseNode(PieceType, NodeID);
	}
	else
	{
		UChessPieceSkillTreeData* TreeData = SkillTree->GetTreeDataForPiece(PieceType);
		const FSkillNodeData* Node = TreeData ? TreeData->FindNode(NodeID) : nullptr;
        
		bOutWasPurchase = false;

		if (Node && Node->Type == ESkillNodeType::Equippable)
		{
			const FName Equipped = SkillTree->GetEquippedNode(PieceType);
			if (Equipped == NodeID)
			{
				SkillTree->UnequipNode(PieceType);
				return true;
			}
			return SkillTree->TryEquipNode(PieceType, NodeID);
		}

		return false;
	}
}
