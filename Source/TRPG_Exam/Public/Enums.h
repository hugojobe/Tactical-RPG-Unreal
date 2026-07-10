#pragma once

UENUM(BlueprintType)
enum class ETileInteraction : uint8
{
	None,
	Move,
	Attack
};

UENUM(BlueprintType)
enum class EChessUnitType : uint8
{
	Pawn	UMETA(DisplayName = "Pawn"),
	Bishop	UMETA(DisplayName = "Bishop"),
	Knight	UMETA(DisplayName = "Knight"),
	Rook	UMETA(DisplayName = "Rook"),
	Queen	UMETA(DisplayName = "Queen"),
	King	UMETA(DisplayName = "King"),
	Emperor	UMETA(DisplayName = "Emperor")
};

UENUM(BlueprintType)
enum class EChessTeam : uint8
{
	Player  UMETA(DisplayName = "Player"),
	Enemy   UMETA(DisplayName = "Enemy"),
	None	UMETA(DisplayName = "None")
};

UENUM(BlueprintType)
enum class EMovementStyle : uint8
{
	Slide	UMETA(DisplayName = "Slide (Moves until blocked"),
	Jump	UMETA(DisplayName = "Jump (Moves to specific location"),
	Pawn	UMETA(DisplayName = "Pawn"),
};

UENUM(BlueprintType)
enum class ETurnState : uint8
{
	Player	UMETA(DisplayName = "Player"),
	Enemy	UMETA(DisplayName = "Enemy"),
	None	UMETA(DisplayName = "None"),
};

enum class EAttackPhase : uint8
{
	Rush,
	Resolve,
	Return,
	Capture
};

UENUM(BlueprintType)
enum class EGameOverCause : uint8
{
	NoPiecesRemaining	UMETA(DisplayName = "No Pieces Remaining"),
	KingKilled			UMETA(DisplayName = "King Killed"),		
	CantMove			UMETA(DisplayName = "Can't Move")
};

UENUM(BlueprintType)
enum class ESkillNodeType : uint8
{
	Passive     UMETA(DisplayName = "Passive"),
	Equippable  UMETA(DisplayName = "Equipable"),
};

UENUM(BlueprintType)
enum class ESkillNodeState : uint8
{
	Locked,
	Unlockable,
	Purchased, 
	Equipped,
};
