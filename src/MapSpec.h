#pragma once
#include <vector>
#include "Types.h"

namespace MapSpec {

    // Build a 20-territory random map on a larger ASCII grid.
    // Overload with optional seed so you can reproduce maps when debugging.
    std::vector<Territory> build20();
    std::vector<Territory> build20(unsigned seed);

} // namespace MapSpec
