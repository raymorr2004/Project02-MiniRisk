#pragma once
#include <cstdint>
#include "Board.h"
#include "Types.h"

namespace RandomAI {

    // ---------------- REINFORCEMENTS ----------------
    // Chooses one owned territory to receive reinforcements.
    // Returns a valid owned territory id (or -1 if none).
    TerrId chooseReinforcement(const Board& b, PlayerId p,
                               int reinforcements, std::uint32_t seed);

    // ---------------- ATTACK ----------------
    struct AttackPlan {
        TerrId from{-1};
        TerrId to{-1};
        bool valid{false};
    };

    // Selects an attack (from→to). May skip attack (valid=false)
    // if probabilities are too low.
    AttackPlan chooseAttack(const Board& b, PlayerId p, std::uint32_t seed);

    // ---------------- FORTIFY ----------------
    struct FortifyPlan {
        TerrId from{-1};
        TerrId to{-1};
        int amount{0};
        bool valid{false};
    };

    // Chooses a fortification move between adjacent owned territories.
    // Returns valid=false if AI chooses to skip.
    FortifyPlan chooseFortify(const Board& b, PlayerId p, std::uint32_t seed);

} // namespace RandomAI
