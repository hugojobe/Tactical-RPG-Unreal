#include "AI/ChessMovementLogic.h"

void ChessMovementLogic::CalculateLegalMoves(
    const IChessBoardInterface* Board,
    const FIntPoint& StartPos,
    const FMovementRule& Rule,
    EChessTeam MyTeam,
    TArray<FVirtualMove>& OutMoves,
    bool bIncludeEmptyAttacks)
{
    if (!Board) return;

    if (Rule.MovementStyle == EMovementStyle::Pawn)
    {
        int32 Dir = (MyTeam == EChessTeam::Player) ? 1 : -1;

        FIntPoint Forward(StartPos.X, StartPos.Y + Dir);
        FIntPoint Backward(StartPos.X, StartPos.Y - Dir);

        if (Board->IsValidGridPos(Forward) && !Board->IsTileOccupied(Forward))
        {
            OutMoves.Add({ -1, StartPos, Forward, EVirtualMoveType::Move });
        }
        if (Board->IsValidGridPos(Backward) && !Board->IsTileOccupied(Backward))
        {
            OutMoves.Add({ -1, StartPos, Backward, EVirtualMoveType::Move });
        }

        FIntPoint Attacks[] = {
            FIntPoint(StartPos.X - 1, StartPos.Y + Dir),
            FIntPoint(StartPos.X + 1, StartPos.Y + Dir)
        };
        
        for (const FIntPoint& AttackPos : Attacks)
        {
            if (Board->IsValidGridPos(AttackPos))
            {
                bool bHasEnemy = Board->IsTileOccupied(AttackPos) && Board->GetTeamAt(AttackPos) != MyTeam;
                
                if (bHasEnemy || bIncludeEmptyAttacks)
                {
                    OutMoves.Add({ -1, StartPos, AttackPos, EVirtualMoveType::Attack });
                }
            }
        }
        return;
    }

    for (const FVector2D& DirVec : Rule.MoveVectors)
    {
        if (DirVec.IsNearlyZero()) continue;

        FIntPoint Dir(FMath::RoundToInt(DirVec.X), FMath::RoundToInt(DirVec.Y));
        if (Dir == FIntPoint(0,0)) continue;

        int32 MaxBoardSize = 16; 
        int32 SafeRange = (Rule.RangeLimit > 0) ? Rule.RangeLimit : MaxBoardSize;
        SafeRange = FMath::Min(SafeRange, MaxBoardSize);

        int32 Loops = (Rule.MovementStyle == EMovementStyle::Slide) ? SafeRange + 1 : 2;

        for (int32 i = 1; i < Loops; i++)
        {
            FIntPoint TargetPos = StartPos + (Dir * i);
            
            if (!Board->IsValidGridPos(TargetPos)) break;

            if (!Board->IsTileOccupied(TargetPos))
            {
                OutMoves.Add({ -1, StartPos, TargetPos, EVirtualMoveType::Move });
            }
            else
            {
                if (Board->GetTeamAt(TargetPos) != MyTeam)
                {
                    OutMoves.Add({ -1, StartPos, TargetPos, EVirtualMoveType::Attack });
                }
                break;
            }
            
            if (Rule.MovementStyle != EMovementStyle::Slide)
            {
                break;
            }
        }
    }
}