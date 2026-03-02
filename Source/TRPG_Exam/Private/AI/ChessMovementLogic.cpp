#include "AI/ChessMovementLogic.h"

#include "AI/ChessAIPlanner.h"

void ChessMovementLogic::CalculateLegalMoves(
    FVirtualBoard Board,
    const FIntPoint& StartPos,
    const FMovementRule& Rule,
    EChessTeam MyTeam,
    TArray<FChessDecision>& OutMoves,
    bool bIncludeEmptyAttacks)
{
    if (Rule.MovementStyle == EMovementStyle::Pawn)
    {
        int32 Dir = (MyTeam == EChessTeam::Player) ? 1 : -1;

        FIntPoint Forward(StartPos.X, StartPos.Y + Dir);
        FIntPoint Backward(StartPos.X, StartPos.Y - Dir);

        if (Board.IsValidGridPos(Forward, Board.Width, Board.Height) && !Board.IsTileOccupied(Forward) && !bIncludeEmptyAttacks)
        {
            OutMoves.Add({ EChessDecisionType::Move, -BIG_NUMBER, Forward });
        }
        if (Board.IsValidGridPos(Backward, Board.Width, Board.Height) && !Board.IsTileOccupied(Backward) && !bIncludeEmptyAttacks)
        {
            OutMoves.Add({EChessDecisionType::Move, -BIG_NUMBER, Backward});
        }

        FIntPoint Attacks[] = {
            FIntPoint(StartPos.X - 1, StartPos.Y + Dir),
            FIntPoint(StartPos.X + 1, StartPos.Y + Dir)
        };
        
        for (const FIntPoint& AttackPos : Attacks)
        {
            if (Board.IsValidGridPos(AttackPos, Board.Width, Board.Height))
            {
                bool bHasEnemy = Board.IsTileOccupied(AttackPos) && Board.GetTeamAt(AttackPos) != MyTeam;
                
                if (bHasEnemy || bIncludeEmptyAttacks)
                {
                    OutMoves.Add({EChessDecisionType::Attack, -BIG_NUMBER, AttackPos});
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
        
        bool HasEncounteredAlly = false;

        for (int32 i = 1; i < Loops; i++)
        {
            FIntPoint TargetPos = StartPos + (Dir * i);
            
            if (!Board.IsValidGridPos(TargetPos, Board.Width, Board.Height)) break;

            if (!Board.IsTileOccupied(TargetPos))
            {
                OutMoves.Add({ EChessDecisionType::Move, -BIG_NUMBER, TargetPos });
            }
            else
            {
                if (Board.GetTeamAt(TargetPos) != MyTeam)
                {
                    OutMoves.Add({ EChessDecisionType::Attack, -BIG_NUMBER, TargetPos });
                }
                else
                {
                    if (!HasEncounteredAlly && bIncludeEmptyAttacks)
                    {
                        OutMoves.Add({ EChessDecisionType::Attack, -BIG_NUMBER, TargetPos });
                        HasEncounteredAlly = true;
                    }
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
