#pragma once
#include <cstdint>
#include "Board.h"
#include "Types.h"

// High-level game orchestration (no implementations in this file).
namespace Game {

    // Build and validate a random 20-territory board.
    // - makeBoard() uses a random seed internally.
    // - makeBoard(seed) is deterministic for testing.
    Board makeBoard();
    Board makeBoard(std::uint32_t seed);

    // Print a short rules/controls blurb (requirement: rules explanation).
    void printRules();

    // Randomly assign starting ownership/armies (10 territories each, etc.).
    // Kept separate for testability and to keep play() focused on turn flow.
    void setupStartingPositions(Board& b, std::uint32_t seed);

    // Run one complete game. Returns the final GameState.
    // - cpuAsP2: if true, Player 2 is controlled by the CPU.
    // - seed: used for any RNG in turn flow (dice, AI choices, etc.).
    GameState play(Board& b, bool cpuAsP2, std::uint32_t seed);

} // namespace Game
