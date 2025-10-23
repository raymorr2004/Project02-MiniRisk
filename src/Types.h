#pragma once
#include <string>
#include <vector>

// --------------------------------------
// Player and Game State Enumerations
// --------------------------------------

enum class PlayerId : int {
    None = -1,
    P1 = 0,
    P2 = 1
};

enum class GameState : int {
    Ongoing,
    Player1Wins,
    Player2Wins,
    Draw
};

// --------------------------------------
// Grid Position and Type Aliases
// --------------------------------------

struct Pos {
    int r{0};
    int c{0};
};

// Each territory is identified by its index in the Board’s vector
using TerrId = int;

// --------------------------------------
// Territory Definition
// --------------------------------------

struct Territory {
    char code{'?'};                  // Letter code (A, B, C, …)
    std::string name;                // Optional display name
    PlayerId owner{PlayerId::None};  // Who controls the territory
    int armies{0};                   // Number of armies stationed
    std::vector<TerrId> adj;         // Adjacency list (neighboring territory IDs)
    int r{0};                        // Row on grid
    int c{0};                        // Column on grid
};
