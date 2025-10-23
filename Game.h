#pragma once
#include <cstdint>
#include <random>
#include "src/Board.h"
#include "src/Types.h"

// ------------------------------------------------------------
// Game
//  • Manages a full Mini-RISK match
//  • Encapsulates board state, RNG seed, and turn logic
// ------------------------------------------------------------
class Game {
public:
    // ---------- Constructors ----------
    Game();                                   // random seed
    explicit Game(uint32_t seed);             // fixed seed for reproducibility

    // ---------- Core Methods ----------
    void printRules() const;                  // show basic rules
    void setupStartingPositions(uint32_t seed = 0);
    void resetBoard(uint32_t seed);           // rebuild board with new seed
    GameState play(bool cpuAsP2 = true);      // run one full game

    // ---------- Accessors ----------
    Board& board();
    const Board& board() const;

private:
    // ---------- Helpers ----------
    Board makeBoard(uint32_t seed);
    bool anyLegalAttack(PlayerId p) const;
    bool anyLegalFortify(PlayerId p) const;

    // ---------- Members ----------
    Board board_;
    PlayerId current_{PlayerId::P1};
    uint32_t seed_{0};
    std::mt19937 rng_;
};
