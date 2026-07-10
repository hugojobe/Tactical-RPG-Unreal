// By hugojobe.

#include "Cheats/ChessCheatManager.h"

#include "FunctionLibs/ChessSkillTreeQuery.h"

void UChessCheatManager::AddRP(int32 Amount)
{
	UChessSkillTreeQuery::CheatAddRP(this, Amount);
}
