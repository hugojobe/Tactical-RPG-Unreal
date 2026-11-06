# Turn-based tactics prototype with a Minimax AI
 
A grid-based tactics game built on chess pieces, made in **Unreal Engine** with **C++** and the **Gameplay Ability System (GAS)**. You fight a computer opponent across **ten levels of rising difficulty**, equip one ability per piece type, and grow your army through a skill tree.

The project has three technical pillars:
 
1. An **adversarial search AI** (Minimax with alpha-beta pruning and a full set of search optimizations).
2. A **data-driven ability and skill system** on top of GAS, balanceable by designers without touching code.
3. **Two editor tools that use the AI as a playtester**: a level generator and an automatic evaluation-weight tuner.

> **Context:** school project (HEAJ, 2025-2026), solo project.

<img width="800" height="450" alt="ezgif-6926ce4611e7c522" src="https://github.com/user-attachments/assets/ba9652c3-caec-46cf-b467-4a65a53229f6" />

## Table of contents
 
- [Game design](#game-design)
- [The AI](#the-ai)
- [Abilities (GAS)](#abilities-gas)
- [Skill tree](#skill-tree)
- [Tool: level generator](#tool-level-generator)
- [Tool: weight optimizer](#tool-weight-optimizer)
- [What I learned](#what-i-learned)
- [Known limitations and next steps](#known-limitations-and-next-steps)

## Game design
 
### Why chess?
 
- Most players already know chess at least vaguely, so **onboarding is light**.
- That familiarity can be **subverted**: the game changes what the player thinks they know.

### How the game breaks chess expectations
 
The game starts from three assumptions the player brings with them, and keeps one while breaking two.
 
| Player assumption | What the game does |
|---|---|
| "I know how this piece moves" | **Kept.** Movement is faithful to chess. |
| "Touching a piece captures it" | **Broken.** Pieces have **hit points**, so a fight lasts several turns. |
| "The board is fixed" | **Broken.** Squares can be blocked. |

### Key mechanics
 
| Mechanic | Problem it solves | Effect on the player's mental model |
|---|---|---|
| Hit points | One-hit captures make buffs and debuffs impossible | Players judge a fight over several turns, not a single capture |
| One action per turn | Keeps turns readable | Each turn asks one clear question |
| Free abilities (cost no turn) | An ability that costs the turn would never be used | Players look for ability + movement combos |
| One equipped ability per piece type | Prevents stacking and forces a choice | Players build their own playstyle |
| Passive bonuses | Progression without new rules to learn | A felt sense of growing stronger |

### Information contract
 
The game has **perfect information** and **no randomness**. HP, damage and cooldowns are all visible, and **the AI uses exactly the information the player can see**, with no hidden advantage. A loss is therefore always the player's own doing.
 
One small exception is deliberate: a tiny amount of noise is added to the AI's final choice, only so that replaying the same game does not unfold identically.
 
### Difficulty levers
 
1. **Level composition**: number and type of pieces on each side.
2. **Player progression**: the skill tree can offset a numerical disadvantage.
3. **AI thinking time**: more time means better anticipation.
### Inspirations
 
| Source | What was taken |
|---|---|
| Chess | Pieces and movement |
| Into the Breach | Small board, perfect information |
| Tactical RPGs | Progression between battles, abilities |

## The AI
 
### The problem
 
Each turn the AI chooses among many actions (move, attack, use an ability). Taking the most immediate gain is rarely right: capturing a piece can expose another one to a counter-attack. The AI must explore consequences: its move, the player's reply, the reply to that reply, and so on. That exploration forms a **game tree**.

### Lightweight virtual board
 
Searching the tree means trying thousands of moves, which is impossible with the real on-screen actors because they are heavy. Instead, the AI works on a **minimal copy of the board**: a structure with an array of 64 squares, each holding a `Virtual Unit` or nothing. Copying it takes a few microseconds, which makes it possible to simulate **hundreds of thousands of moves per second**. The copy is taken just before the AI starts thinking, from the real board state.
 
### Minimax with alpha-beta pruning
 
Minimax suits two-player games with opposed interests. Each position gets a score: positive if it favors the computer, negative if it favors the player. The computer assumes it picks the highest score and the player picks the lowest, and plays the move that guarantees the best outcome.
 
### Evaluation function
 
The evaluation function scores a position and defines the AI's "personality". It sums several terms:
 
| Term | What it measures |
|---|---|
| Material | Piece value, HP and damage |
| Royalty | Very high value for the king |
| King threat | Number of pieces able to attack the king |
| Pawn advancement | How far pawns have progressed toward the enemy side |
| Center control | Proximity to the center of the board |
| Development | Penalty for pieces still on their starting square |
| Reach | Number of squares each side can reach |
| Losing pieces | Penalty for attacked, undefended pieces |
 
### Optimizations
 
A full search is infeasible: with about 50 moves per side, ten turns means hundreds of billions of positions. The following techniques make the search tractable:
 
1. **Alpha-beta pruning.** Track the best result already found for each side, and stop exploring a branch once it cannot influence the final decision.
2. **Move ordering.** Pruning depends on trying the best move first. Order: the stored best move for the position, then captures (largest units first), then **killer moves** (moves that caused a cutoff elsewhere in the tree).
3. **Transposition table.** The same position can be reached by different paths. Each position is hashed with a **64-bit Zobrist hash** (piece, position, HP, status, etc.) and used as a key into a large table of already-searched positions, so known results are reused instead of re-simulated.
4. **Iterative deepening.** Search one turn ahead, then two, then three, until the time budget runs out. Each pass fills the transposition table and improves move ordering for the next. The search is **interruptible** and always returns at least one valid move.
5. **Quiescence search.** With a time-limited search, the *horizon effect* can make the AI see a capture but miss the counter-attack. The AI keeps simulating, attacks only, until the position is quiet.
6. **Late move reductions.** Since moves are sorted from most to least promising, the last ones are likely bad. They are searched less deeply unless they prove interesting.
7. **Lightweight data structures.** No memory allocations in the hot path, and data shared with actors already in the scene.
### Handling abilities in the search
 
Abilities are GAS objects acting on real actors, so they cannot run inside the simulation, and adding them as moves would blow up the branching factor. The AI therefore splits the decision in two steps:
 
1. A **simplified layer** reproduces each ability's effect on the virtual board. Each candidate is scored with a short evaluation and kept only if it improves the position. The best ability is then **played for real**.
2. The **deep search** then picks the turn's movement, on a virtual board that already accounts for the ability just triggered.
### Result
 
On a **6x6 board with about a dozen pieces**, the AI explores roughly **2 to 3 million moves in 1.5 seconds** at a **depth of nine turns**.

## Abilities (GAS)
 
Abilities are central to the game and fall along three axes: **mobility**, **area control** and **support**.
 
| Piece | Ability | Effect |
|---|---|---|
| Pawn | First Charge | Double move on its first move |
| Pawn | Caltrop | Makes a diagonal square inaccessible |
| Bishop | Translation | Moves one square orthogonally, switching diagonal color |
| Bishop | Blessing | Heals an ally on its diagonal |
| Knight | Intimidation | Reduces damage dealt by adjacent enemies |
| Knight | Double Leap | An extra free jump |
| Rook | Great Castle | Swaps position with an aligned ally |
| Rook | Bastion | Makes every square on its row inaccessible |
| Queen | Execution | Bonus damage on the next attack |
| Queen | Repulsion | Pushes back all adjacent pieces |
| King | Escape | Double move, once per game |
| King | Swap | Swaps places with an adjacent ally |
| Emperor (boss) | Summon | Teleports an ally to him |
| Emperor (boss) | Punishment | Area damage around him |
 
### Architecture
 
- A **parent ability class** defines the targeting type (self, ally, enemy or square), the list of valid targets, and the effect to apply.
- Each ability has two parts: a **C++ class** holding the logic and a **Blueprint asset** holding the data, so designers can balance the game without code changes.
- The valid-targets method is reused to **highlight squares** on the board, **validate the player's click**, and **gray out the UI button** when no target exists.
- **Turn-based cooldowns on top of GAS.** GAS only knows cooldowns in seconds. When an ability is activated, the unit receives an **infinite-duration gameplay effect** whose stack count equals the cooldown. At the start of each turn the stack is reduced by one until the cooldown ends.

## Skill tree
 
Players spend **Research Points** to unlock abilities: **passives** (permanent bonuses) and **actives** (equippable skills). Each piece type has its own tree.
 
- **Data-driven.** Each piece's tree is a **data asset** containing a list of node structs.
- **Subsystem.** On `BeginPlay`, each player unit queries the skill tree subsystem, applies purchased passives one by one, then equips the selected active ability.
- Enemy units do not use the skill tree. Their passives and actives are set directly in the editor on each piece.

<img width="400" height="400" alt="image" src="https://github.com/user-attachments/assets/31f518df-bc8c-4f8a-8448-62bc9fdf95cc" />

# Tool: level generator
 
Designing ten starting positions that are balanced, progressive and interesting means playing many games. This tool **reuses the AI as a tester**: if the AI can play, it can play against itself.
 
### How it works
 
Each level is described by a **rule**: board size, allowed piece types, number of pieces per side, presence of a king, presence of the Emperor. Rules encode the difficulty and learning curve (early levels use a small board and few pieces).
 
An **Editor Utility Widget** shows the ten grids, a progress bar, and generate / regenerate buttons per level.
 
### Scoring a game's "interest"
 
Fun cannot be measured directly, so the tool uses indirect indicators chosen to match what makes a game interesting:
 
| Indicator | Effect on score | Rationale |
|---|---|---|
| Game with no winner | Disqualifying | A game that stalls is never good |
| Advantage reversals | Strongly positive | The definition of tension |
| Losses on both sides | Positive | Both sides should lose units |
| Medium duration | Positive | Neither too fast nor too slow |
| Player win | Positive | The level must be winnable on equal terms |

<img width="1300" height="594" alt="image" src="https://github.com/user-attachments/assets/0b3ae107-882a-40c5-a8c0-a8ba4a604293" />


## Tool: weight optimizer
 
The evaluation function relies on about **ten coefficients** (how much an HP point is worth, how much weight center control gets, etc.). They were first picked by intuition, with no guarantee they are good and no way to check except by playing.
 
This tool tunes them automatically. It is **not a neural network** but **black-box optimization**: good coefficients cannot be computed directly, but a candidate set can be tested by letting it play. This is the same principle used to tune hyperparameters of machine learning models.
 
It uses a **(1+1) evolution strategy**:
 
1. At the start of a cycle, apply a **random perturbation** to the coefficients.
2. Test the new coefficients against the current ones.
3. If the new ones win, they become the **champion**.
4. The next cycle tests new perturbations against the champion.
5. After a fixed number of iterations, the champion holds the best weights found.
**Each match is played twice with sides swapped.** The game is not perfectly symmetric, so without this swap the tool would measure side advantage instead of the coefficients' quality.

## What I learned
 
- How a **turn-based game AI** works end to end. I had touched such algorithms in earlier projects, but never this far or this complex.
- **Editor Utility Widgets**, which let me build in-editor tools. I plan to build better tools with them in future projects.
## Known limitations and next steps
 
- **Abilities are not part of the search tree.** The AI cannot imagine deeper combos, and it is blind to the player's abilities. Integrating abilities into the search is the next AI step.
- **Texel Tuning** would be the best effort-to-gain upgrade over the current weight optimizer.
