#pragma once
#include <cstdint>
#include <optional>
#include "Board.h"
#include "Types.h"

namespace RandomAI {

    // ---- Reinforcements ----
    // Choose one territory to receive reinforcements.
    // Returns a valid owned territory id.
    TerrId chooseReinforcement(const Board& b, PlayerId p,
                               int reinforcements, std::uint32_t seed);

    // ---- Attack ----
    struct AttackPlan {
        TerrId from{-1};
        TerrId to{-1};
        bool valid{false};
    };

    // Choose a single attack (from->to). May return valid=false if AI decides not to attack.
    AttackPlan chooseAttack(const Board& b, PlayerId p, std::uint32_t seed);

    // ---- Fortify ----
    struct FortifyPlan {
        TerrId from{-1};
        TerrId to{-1};
        int amount{0};
        bool valid{false};
    };

    // Choose a fortification move (or valid=false if AI skips).
    FortifyPlan chooseFortify(const Board& b, PlayerId p, std::uint32_t seed);

} // namespace RandomAI
