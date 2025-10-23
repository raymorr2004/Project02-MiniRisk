#pragma once
#include <string>
#include <vector>
#include "Types.h"   // contains PlayerId, TerrId, Territory

class Board {
public:
    // --- Constructors ---
    Board() = default;
    explicit Board(std::vector<Territory> territories);

    // --- Accessors ---
    const std::vector<Territory>& getTerritories() const;
    std::vector<Territory>& getTerritories();
    int count() const;
    const Territory& at(TerrId id) const;
    Territory& at(TerrId id);

    // --- Adjacency helpers ---
    const std::vector<TerrId>& neighbors(TerrId id) const;
    bool areAdjacent(TerrId a, TerrId b) const;

    // --- Rendering ---
    std::string render(bool showOwner = false,
                       bool showArmies = false,
                       int cellWidth = 3) const;

    std::string renderColor(int cellWidth = 3) const;

    // --- Validators ---
    bool validateAdjUndirected() const;
    bool validateUniqueCodesAndCoords() const;

private:
    std::vector<Territory> territories_;
};
