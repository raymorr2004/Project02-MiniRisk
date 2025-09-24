#pragma once
#include <vector>
#include <utility>
#include "Types.h"
#include "Board.h"

// Pure game logic (no I/O). Implementations live in Rules.cpp.
namespace Rules {

    // ---- Ownership helpers ----
    std::vector<TerrId> owned(const Board& b, PlayerId p);
    std::vector<TerrId> borders(const Board& b, PlayerId p); // owned territories that touch an enemy

    // ---- Game state / victory ----
    GameState gameStatus(const Board& b); // Player1Wins / Player2Wins / Ongoing

    // ---- Reinforcements ----
    // Base troops at start of turn: max(3, floor(owned/3))
    int baseReinforcements(const Board& b, PlayerId p);

    // Chain-of-5 bonus: if a connected component (of p's territories) has size >= 5,
    // award +5 to ONE territory within that component (we'll pass back the chosen tIdx).
    // Return true if a bonus is applicable (and tIdx is set).
    bool chainOf5BonusTarget(const Board& b, PlayerId p, TerrId& tIdx);

    // ---- Legality checks ----
    bool canAttack(const Board& b, TerrId from, TerrId to, PlayerId attacker);

    // Single-hop fortify (adjacent only)
    bool canFortify(const Board& b, TerrId from, TerrId to, PlayerId p);

    // NEW: Path fortify (multi-hop) — true if 'to' is reachable from 'from'
    // by traversing only territories owned by player p. Still requires leaving >=1 behind.
    bool canFortifyPath(const Board& b, TerrId from, TerrId to, PlayerId p);

    // ---- Battle resolution (dice math only) ----
    int attackerDice(int armiesAtFrom);           // up to 3, but must leave 1 behind
    int defenderDice(int armiesAtTo);             // up to 2
    struct BattleLosses { int attacker; int defender; };

    // Roll virtual dice, compare, and return losses (no board mutation here).
    BattleLosses simulateBattleOnce(int attDice, int defDice, unsigned seed);

    // ---- State updates (apply results) ----
    // Apply a battle between adjacent territories. Returns whether defender territory was captured.
    // On capture, caller must then move X armies from 'from' to 'to' (leaving >=1 in 'from').
    bool applyBattle(Board& b, TerrId from, TerrId to, PlayerId attacker, unsigned seed, BattleLosses* last = nullptr);

    // Move armies after a successful capture (or for fortify moves); assumes legality already checked.
    void moveAfterCapture(Board& b, TerrId from, TerrId to, int armiesToMove);

} // namespace Rules
