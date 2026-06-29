#include "FunctionLibs/ChessSkillTreeLayout.h"

static const TMap<EChessUnitType, float> PieceBaseAngles =
{
    { EChessUnitType::Pawn,   270.f },
    { EChessUnitType::Knight, 330.f },
    { EChessUnitType::Bishop,  30.f },
    { EChessUnitType::Rook,    90.f },
    { EChessUnitType::Queen,  150.f },
    { EChessUnitType::King,   210.f },
};

TArray<FSkillNodeLayoutEntry> UChessSkillTreeLayout::ComputeFullLayout(
    const TMap<EChessUnitType, UChessPieceSkillTreeData*>& SkillTreeDataMap,
    FVector2D CanvasSize,
    float DepthStep,
    float AngleSpreadDeg)
{
    TArray<FSkillNodeLayoutEntry> Result;
    const FVector2D Center = CanvasSize / 2.f;
    const float HalfSpanRad = FMath::DegreesToRadians(AngleSpreadDeg);
    const float RootMargin = DepthStep * 0.8f;

    for (const auto& Pair : SkillTreeDataMap)
    {
        EChessUnitType PieceType = Pair.Key;
        UChessPieceSkillTreeData* TreeData = Pair.Value;
        
        if (!TreeData) continue;
        // Ne pas skip si Nodes est vide — on veut quand même le root

        const float* BaseAngleDeg = PieceBaseAngles.Find(PieceType);
        
        if (!BaseAngleDeg) continue;
        const float BaseAngleRad = FMath::DegreesToRadians(*BaseAngleDeg);

        TMap<FName, FNodeWork> WorkMap;
        TMap<FName, TArray<FName>> ChildrenMap;

        // --- Node racine synthétique ---
        const FName RootID = FName(*FString::Printf(TEXT("ROOT_%d"),
            static_cast<int32>(PieceType)));

        FNodeWork RootWork;
        RootWork.NodeID = RootID;
        RootWork.ParentID = NAME_None;
        RootWork.Depth = 0;
        RootWork.Angle = BaseAngleRad;
        RootWork.LeafCount = 1;
        RootWork.Data.NodeID = RootID;
        RootWork.Data.RpCost = 0;
        RootWork.Data.DisplayName = TreeData->RootDisplayName.IsEmpty()
            ? FText::FromString(UEnum::GetDisplayValueAsText(PieceType).ToString())
            : TreeData->RootDisplayName;
        RootWork.Data.Icon = TreeData->RootIcon;

        WorkMap.Add(RootID, RootWork);
        ChildrenMap.Add(RootID, TArray<FName>());

        // --- PASSE UNIQUE : construire WorkMap + ChildrenMap ---
        for (int32 i = 0; i < TreeData->Nodes.Num(); ++i)
        {
            const FSkillNodeData& NodeData = TreeData->Nodes[i];

            FNodeWork Work;
            Work.NodeID    = NodeData.NodeID;
            Work.Depth     = 0;
            Work.Angle     = BaseAngleRad;
            Work.LeafCount = 1;
            Work.Data      = NodeData;

            // Parent : premier RequiredNodeID, ou root synthétique si vide
            if (NodeData.RequiredNodeIDs.IsEmpty())
            {
                Work.ParentID = RootID;
            }
            else
            {
                Work.ParentID = NodeData.RequiredNodeIDs[0];
            }

            WorkMap.Add(NodeData.NodeID, Work);
            ChildrenMap.Add(NodeData.NodeID, TArray<FName>());
        }

        // Relier les enfants à leurs parents (après que tous les nodes sont dans la map)
        for (const FSkillNodeData& NodeData : TreeData->Nodes)
        {
            const FName ParentID = WorkMap[NodeData.NodeID].ParentID;
            if (TArray<FName>* Children = ChildrenMap.Find(ParentID))
            {
                Children->Add(NodeData.NodeID);
            }
        }

        // --- Passe 1 : LeafCount (bottom-up) ---
        // Inclure le root dans le calcul
        TArray<FName> AllIDs;
        AllIDs.Add(RootID);
        for (const FSkillNodeData& NodeData : TreeData->Nodes)
            AllIDs.Add(NodeData.NodeID);

        // Ordre inverse = feuilles d'abord
        for (int32 i = AllIDs.Num() - 1; i >= 0; i--)
        {
            const FName& ID = AllIDs[i];
            const TArray<FName>& Children = ChildrenMap[ID];
            if (Children.IsEmpty())
            {
                WorkMap[ID].LeafCount = 1;
            }
            else
            {
                int32 Sum = 0;
                for (const FName& ChildID : Children)
                    Sum += WorkMap[ChildID].LeafCount;
                WorkMap[ID].LeafCount = FMath::Max(Sum, 1);
            }
        }

        // --- Passe 2 : AssignAngles ---
        const int32 RootLeaves = WorkMap[RootID].LeafCount;
        const float TotalSpan  = HalfSpanRad * FMath::Max(RootLeaves, 1);
        AssignAngles(RootID, BaseAngleRad, TotalSpan, WorkMap, ChildrenMap);

        // --- Passe 3 : Profondeur BFS + position écran ---
        TQueue<FName> Queue;
        Queue.Enqueue(RootID);
        WorkMap[RootID].Depth = 1;

        while (!Queue.IsEmpty())
        {
            FName Current;
            Queue.Dequeue(Current);
            for (const FName& ChildID : ChildrenMap[Current])
            {
                WorkMap[ChildID].Depth = WorkMap[Current].Depth + 1;
                Queue.Enqueue(ChildID);
            }
        }

        // --- Convertir en entries ---
        for (auto& WorkPair : WorkMap)
        {
            FNodeWork& Work = WorkPair.Value;
            const float Dist = RootMargin + Work.Depth * DepthStep;
            const FVector2D Pos = Center + FVector2D(
                FMath::Cos(Work.Angle) * Dist,
                FMath::Sin(Work.Angle) * Dist
            );

            FSkillNodeLayoutEntry Entry;
            Entry.NodeID       = Work.NodeID;
            Entry.ParentNodeID = Work.ParentID;
            Entry.Position     = Pos;
            Entry.NodeData     = Work.Data;
            Entry.PieceType    = PieceType;
            Result.Add(Entry);
        }
    }

    return Result;
}

void UChessSkillTreeLayout::AssignAngles(
    const FName& NodeID,
    float CenterAngle,
    float HalfSpan,
    TMap<FName, FNodeWork>& WorkMap,
    const TMap<FName, TArray<FName>>& ChildrenMap)
{
    WorkMap[NodeID].Angle = CenterAngle;

    const TArray<FName>& Children = ChildrenMap[NodeID];
    if (Children.IsEmpty()) return;

    const int32 TotalLeaves = FMath::Max(WorkMap[NodeID].LeafCount, 1);
    float CurrentAngle = CenterAngle - HalfSpan;

    for (const FName& ChildID : Children)
    {
        const int32 ChildLeaves = WorkMap[ChildID].LeafCount;
        const float ChildSpan   = HalfSpan * 2.f * ChildLeaves / TotalLeaves;
        const float ChildCenter = CurrentAngle + ChildSpan * 0.5f;

        AssignAngles(ChildID, ChildCenter, ChildSpan * 0.5f, WorkMap, ChildrenMap);
        CurrentAngle += ChildSpan;
    }
}

FVector2D UChessSkillTreeLayout::GetGameViewportSize()
{
    FVector2D Size;
    if (GEngine && GEngine->GameViewport)
        GEngine->GameViewport->GetViewportSize(Size);
    return Size;
}