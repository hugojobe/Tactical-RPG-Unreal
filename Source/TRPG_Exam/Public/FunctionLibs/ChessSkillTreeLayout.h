// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ResearchTree/ChessPieceSkillTreeData.h"
#include "ChessSkillTreeLayout.generated.h"

USTRUCT(BlueprintType)
struct FSkillNodeLayoutEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName NodeID;

	UPROPERTY(BlueprintReadOnly)
	FName ParentNodeID;

	UPROPERTY(BlueprintReadOnly)
	FVector2D Position;

	UPROPERTY(BlueprintReadOnly)
	FSkillNodeData NodeData;
	
	UPROPERTY(BlueprintReadOnly)
	EChessUnitType PieceType;
};

UCLASS()
class TRPG_EXAM_API UChessSkillTreeLayout : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Layout")
	static TArray<FSkillNodeLayoutEntry> ComputeFullLayout(
		const TMap<EChessUnitType, UChessPieceSkillTreeData*>& SkillTreeDataMap,
		FVector2D CanvasSize,
		float DepthStep = 100.f,
		float AngleSpreadDeg = 22.f
	);
	
	UFUNCTION(BlueprintCallable, Category = "SkillTree|UI")
	static FVector2D GetGameViewportSize();
	
private:
	struct FNodeWork
	{
		FName NodeID;
		FName ParentID;
		int32 Depth;
		float Angle;
		int32 LeafCount;
		FSkillNodeData Data;
	};

	static void AssignAngles(
		const FName& NodeID,
		float CenterAngle,
		float HalfSpan,
		TMap<FName, FNodeWork>& WorkMap,
		const TMap<FName, TArray<FName>>& ChildrenMap);
};
