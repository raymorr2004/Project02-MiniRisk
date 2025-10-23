#pragma once
#include <vector>
#include "Types.h"

// ------------------------------------------------------------
// MapSpec — deterministic random map generator for Mini-RISK
// ------------------------------------------------------------
// Generates a connected, readable 20-territory layout using
// random coordinates with spacing and minimal spanning tree
// connectivity, then adds limited extra edges for realism.
//
namespace MapSpec {

    // Build a 20-territory random map with a random seed.
    std::vector<Territory> build20();

    // Deterministic variant (for tests or reproducibility).
    std::vector<Territory> build20(unsigned seed);

} // namespace MapSpec
