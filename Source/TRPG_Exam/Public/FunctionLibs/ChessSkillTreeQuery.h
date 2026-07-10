// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Enums.h"
#include "ResearchTree/ChessPieceSkillTreeData.h"
#include "ChessSkillTreeQuery.generated.h"

UCLASS()
class TRPG_EXAM_API UChessSkillTreeQuery : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "SkillTree|Query", meta = (WorldContext = "WorldContextObject"))
	static int32 GetPlayerRP(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category = "SkillTree|Query", meta = (WorldContext = "WorldContextObject"))
	static bool CanAffordNode(const UObject* WorldContextObject, EChessUnitType PieceType, FName NodeID);
	
	UFUNCTION(BlueprintPure, Category = "SkillTree|Query", meta = (WorldContext = "WorldContextObject"))
	static ESkillNodeState GetNodeState(const UObject* WorldContextObject, EChessUnitType PieceType, FName NodeID);
	
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Action", meta = (WorldContext = "WorldContextObject"))
	static bool PurchaseOrEquipNode(const UObject* WorldContextObject, EChessUnitType PieceType, FName NodeID, bool& bOutWasPurchase);
	
	UFUNCTION(BlueprintPure, Category = "SkillTree|Query", meta = (WorldContext = "WorldContextObject"))
	static class UChessSkillTreeSubsystem* GetSkillTreeSubsystem(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Cheat", 
	meta = (WorldContext = "WorldContextObject"))
	static void CheatAddRP(const UObject* WorldContextObject, int32 Amount);
};
