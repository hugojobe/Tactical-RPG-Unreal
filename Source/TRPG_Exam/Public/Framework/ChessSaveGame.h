// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Enums.h"
#include "ChessSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FPieceSkillSaveData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<FName> PurchasedNodeIDs;

	UPROPERTY(BlueprintReadWrite)
	FName EquippedNodeID = NAME_None;
};

UCLASS()
class TRPG_EXAM_API UChessSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "Progression")
	int32 PlayerTotalRP = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "Progression")
	TMap<int32, FPieceSkillSaveData> PieceProgressionMap;
	
	FPieceSkillSaveData& GetOrCreatePieceData(EChessUnitType PieceType)
	{
		return PieceProgressionMap.FindOrAdd(static_cast<int32>(PieceType));
	}
	
	bool IsNodePurchased(EChessUnitType PieceType, FName NodeID) const
	{
		const FPieceSkillSaveData* Data = PieceProgressionMap.Find(static_cast<int32>(PieceType));
		return Data && Data->PurchasedNodeIDs.Contains(NodeID);
	}
	
	FName GetEquippedNodeID(EChessUnitType PieceType) const
	{
		const FPieceSkillSaveData* Data = PieceProgressionMap.Find(static_cast<int32>(PieceType));
		return Data ? Data->EquippedNodeID : NAME_None;
	}
};
