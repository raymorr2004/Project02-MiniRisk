#pragma once
#include <vector>
#include <utility>
#include "Types.h"
#include "Board.h"

// Pure game logic (no I/O). Implements Risk-style mechanics.
namespace Rules {

    // ---------- Ownership ----------
    std::vector<TerrId> owned(const Board& b, PlayerId p);
    std::vector<TerrId> borders(const Board& b, PlayerId p);

    // ---------- Game state / victory ----------
    GameState gameStatus(const Board& b);

    // ---------- Reinforcements ----------
    int baseReinforcements(const Board& b, PlayerId p);

    // Connected component bonus: if any cluster of ≥5 owned territories exists,
    // grant a +5 bonus to one of them (returns true and fills tIdx).
    bool chainOf5BonusTarget(const Board& b, PlayerId p, TerrId& tIdx);

    // ---------- Legality ----------
    bool canAttack(const Board& b, TerrId from, TerrId to, PlayerId attacker);
    bool canFortify(const Board& b, TerrId from, TerrId to, PlayerId p);
    bool canFortifyPath(const Board& b, TerrId from, TerrId to, PlayerId p);

    // ---------- Battle mechanics ----------
    int attackerDice(int armiesAtFrom);
    int defenderDice(int armiesAtTo);

    struct BattleLosses { int attacker; int defender; };

    // Rolls dice, compares, and returns losses (no mutation).
    BattleLosses simulateBattleOnce(int attDice, int defDice, unsigned seed);

    // ---------- State updates ----------
    bool applyBattle(Board& b, TerrId from, TerrId to,
                     PlayerId attacker, unsigned seed,
                     BattleLosses* last = nullptr);

    void moveAfterCapture(Board& b, TerrId from, TerrId to, int armiesToMove);

} // namespace Rules
