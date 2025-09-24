#pragma once
#include <vector>
#include <string>

// Which player owns a territory or is taking a turn
enum class PlayerId : int {
    None = -1,
    P1 = 0,
    P2 = 1
};

// What the overall game state is
enum class GameState : int {
    Ongoing,
    Player1Wins,
    Player2Wins,
    Draw
};

// A position on the ASCII grid (row/column coordinates)
struct Pos {
    int r{0};
    int c{0};
};

// Alias for territory index in the Board's vector
using TerrId = int;

// One territory on the map
struct Territory {
    char code{'?'};                 // Letter code (A, B, C, …)
    std::string name;               // Display name (optional)
    PlayerId owner{PlayerId::None}; // Who controls it
    int armies{0};                  // How many armies stationed
    std::vector<TerrId> adj;        // Neighboring territory indices
    int r{0}, c{0};                 // Coordinates for rendering on grid
};
