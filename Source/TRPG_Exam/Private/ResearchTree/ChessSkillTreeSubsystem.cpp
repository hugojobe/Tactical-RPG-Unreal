// By hugojobe.


#include "ResearchTree/ChessSkillTreeSubsystem.h"
#include "Kismet/GameplayStatics.h"

const FString UChessSkillTreeSubsystem::SaveSlotName = TEXT("ChessSkillTree");
const int32 UChessSkillTreeSubsystem::SaveUserIndex = 0;

void UChessSkillTreeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadProgression();
}

void UChessSkillTreeSubsystem::SaveProgression()
{
	if (!SaveGame) return;
	UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, SaveUserIndex);
}

void UChessSkillTreeSubsystem::LoadProgression()
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		SaveGame = Cast<UChessSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
	}

	if (!SaveGame)
	{
		SaveGame = Cast<UChessSaveGame>(UGameplayStatics::CreateSaveGameObject(UChessSaveGame::StaticClass()));
	}
}

void UChessSkillTreeSubsystem::AddRP(int32 Amount)
{
	if (!SaveGame || Amount <= 0) return;
	SaveGame->PlayerTotalRP += Amount;
	OnRpChanged.Broadcast(SaveGame->PlayerTotalRP);
	SaveProgression();
}

int32 UChessSkillTreeSubsystem::GetRP() const
{
	return SaveGame ? SaveGame->PlayerTotalRP : 0;
}

bool UChessSkillTreeSubsystem::TryPurchaseNode(EChessUnitType PieceType, FName NodeID)
{
	if (!SaveGame) return false;
	if (IsNodePurchased(PieceType, NodeID)) return false;

	UChessPieceSkillTreeData* TreeData = GetTreeDataForPiece(PieceType);
	if (!TreeData) return false;

	const FSkillNodeData* Node = TreeData->FindNode(NodeID);
	if (!Node) return false;

	if (SaveGame->PlayerTotalRP < Node->RpCost) return false;
	if (!AreRequirementsMet(PieceType, *Node)) return false;

	SaveGame->PlayerTotalRP -= Node->RpCost;
	SaveGame->GetOrCreatePieceData(PieceType).PurchasedNodeIDs.Add(NodeID);

	OnNodePurchased.Broadcast(PieceType, NodeID);
	OnRpChanged.Broadcast(SaveGame->PlayerTotalRP);
	SaveProgression();
	return true;
}

bool UChessSkillTreeSubsystem::TryEquipNode(EChessUnitType PieceType, FName NodeID)
{
	if (!SaveGame) return false;
	if (!IsNodePurchased(PieceType, NodeID)) return false;

	UChessPieceSkillTreeData* TreeData = GetTreeDataForPiece(PieceType);
	if (!TreeData) return false;

	const FSkillNodeData* Node = TreeData->FindNode(NodeID);
	if (!Node || Node->Type != ESkillNodeType::Equippable) return false;

	SaveGame->GetOrCreatePieceData(PieceType).EquippedNodeID = NodeID;
	OnNodeEquipped.Broadcast(PieceType, NodeID);
	SaveProgression();
	return true;
}

void UChessSkillTreeSubsystem::UnequipNode(EChessUnitType PieceType)
{
	if (!SaveGame) return;
	SaveGame->GetOrCreatePieceData(PieceType).EquippedNodeID = NAME_None;
	SaveProgression();
}

bool UChessSkillTreeSubsystem::IsNodePurchased(EChessUnitType PieceType, FName NodeID) const
{
	return SaveGame && SaveGame->IsNodePurchased(PieceType, NodeID);
}

bool UChessSkillTreeSubsystem::IsNodeUnlocked(EChessUnitType PieceType, FName NodeID) const
{
	UChessPieceSkillTreeData* TreeData = GetTreeDataForPiece(PieceType);
	if (!TreeData) return false;

	const FSkillNodeData* Node = TreeData->FindNode(NodeID);
	if (!Node) return false;

	if (Node->RequiredNodeIDs.IsEmpty()) return true;

	return AreRequirementsMet(PieceType, *Node);
}

FName UChessSkillTreeSubsystem::GetEquippedNode(EChessUnitType PieceType) const
{
	return SaveGame ? SaveGame->GetEquippedNodeID(PieceType) : NAME_None;
}

UChessPieceSkillTreeData* UChessSkillTreeSubsystem::GetTreeDataForPiece(EChessUnitType PieceType) const
{
	const auto* Found = SkillTreeDataMap.Find(PieceType);
	return Found ? *Found : nullptr;
}

TArray<FSkillNodeData> UChessSkillTreeSubsystem::GetPurchasedPassiveNodes(EChessUnitType PieceType) const
{
	TArray<FSkillNodeData> Result;
	UChessPieceSkillTreeData* TreeData = GetTreeDataForPiece(PieceType);
	if (!TreeData || !SaveGame) return Result;

	const FPieceSkillSaveData* PieceData = SaveGame->PieceProgressionMap.Find(
		static_cast<int32>(PieceType));
	if (!PieceData) return Result;

	for (const FSkillNodeData& Node : TreeData->Nodes)
	{
		if (Node.Type == ESkillNodeType::Passive &&
			PieceData->PurchasedNodeIDs.Contains(Node.NodeID))
		{
			Result.Add(Node);
		}
	}
	return Result;
}

bool UChessSkillTreeSubsystem::GetEquippedAbilityNode(EChessUnitType PieceType, FSkillNodeData& OutNode) const
{
	FName EquippedID = GetEquippedNode(PieceType);
	if (EquippedID == NAME_None) return false;

	UChessPieceSkillTreeData* TreeData = GetTreeDataForPiece(PieceType);
	if (!TreeData) return false;

	const FSkillNodeData* Node = TreeData->FindNode(EquippedID);
	if (!Node || Node->Type != ESkillNodeType::Equippable) return false;

	OutNode = *Node;
	return true;
}

bool UChessSkillTreeSubsystem::AreRequirementsMet(EChessUnitType PieceType, const FSkillNodeData& Node) const
{
	for (const FName& RequiredID : Node.RequiredNodeIDs)
	{
		if (!IsNodePurchased(PieceType, RequiredID)) return false;
	}
	return true;
}


