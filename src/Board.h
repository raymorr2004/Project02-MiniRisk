#pragma once
#include <string>
#include <vector>
#include "Types.h"

class Board {
public:
    // ---- construction ----
    explicit Board(std::vector<Territory> terrs);

    // ---- accessors ----
    const std::vector<Territory>& territories() const;
    std::vector<Territory>& territories();
    int count() const;
    const Territory& at(TerrId id) const;
    Territory& at(TerrId id);

    // ---- adjacency helpers ----
    const std::vector<TerrId>& neighbors(TerrId id) const;
    bool areAdjacent(TerrId a, TerrId b) const;

    // ---- rendering (ASCII text) ----
    // showOwner: prints 1/2 marker; showArmies: prints army counts inside cell; cellWidth: spacing per cell.
    std::string render(bool showOwner = false,
                       bool showArmies = false,
                       int cellWidth = 3) const;

    // ---- rendering (ANSI color blocks, multi-line) ----
    // Each territory is drawn as two rows:
    //   - top row shows the territory code
    //   - bottom row shows the army count
    // cellWidth controls horizontal spacing (2–4 typical).
    std::string renderColor(int cellWidth = 3) const;

    // ---- validators ----
    bool validateAdjUndirected() const;        // for every a->b there is b->a
    bool validateUniqueCodesAndCoords() const; // no duplicate code or (r,c)

private:
    std::vector<Territory> t_;
};
