#include "RandomAI.h"
#include "Rules.h"

#include <algorithm>
#include <random>
#include <vector>

namespace RandomAI {

// ---------- Reinforcement ----------

TerrId chooseReinforcement(const Board& b, PlayerId p,
                           int /*reinforcements*/, std::uint32_t seed) {
    auto ownedList = Rules::owned(b, p);
    if (ownedList.empty()) return -1;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, static_cast<int>(ownedList.size()) - 1);
    return ownedList[dist(rng)];
}

// ---------- Attack ----------

AttackPlan chooseAttack(const Board& b, PlayerId p, std::uint32_t seed) {
    AttackPlan plan;
    auto borders = Rules::borders(b, p);
    if (borders.empty()) {
        plan.valid = false;
        return plan;
    }

    std::mt19937 rng(seed);
    std::vector<std::pair<TerrId,TerrId>> legalPairs;

    for (TerrId from : borders) {
        if (b.at(from).armies < 2) continue; // must leave 1 behind
        for (TerrId to : b.neighbors(from)) {
            if (Rules::canAttack(b, from, to, p)) {
                legalPairs.emplace_back(from, to);
            }
        }
    }

    if (legalPairs.empty()) {
        plan.valid = false;
        return plan;
    }

    std::uniform_int_distribution<int> dist(0, static_cast<int>(legalPairs.size()) - 1);
    auto [f, t] = legalPairs[dist(rng)];
    plan.from = f;
    plan.to   = t;
    plan.valid = true;
    return plan;
}

// ---------- Fortify ----------

FortifyPlan chooseFortify(const Board& b, PlayerId p, std::uint32_t seed) {
    FortifyPlan plan;

    auto ownedList = Rules::owned(b, p);
    if (ownedList.empty()) {
        plan.valid = false;
        return plan;
    }

    std::mt19937 rng(seed);
    std::vector<std::tuple<TerrId,TerrId,int>> options;

    for (TerrId from : ownedList) {
        if (b.at(from).armies < 2) continue; // must leave 1 behind
        for (TerrId to : b.neighbors(from)) {
            if (Rules::canFortify(b, from, to, p)) {
                int maxMove = b.at(from).armies - 1;
                if (maxMove > 0) {
                    std::uniform_int_distribution<int> amtDist(1, maxMove);
                    int amt = amtDist(rng);
                    options.emplace_back(from, to, amt);
                }
            }
        }
    }

    if (options.empty()) {
        plan.valid = false;
        return plan;
    }

    std::uniform_int_distribution<int> dist(0, static_cast<int>(options.size()) - 1);
    auto [f, t, amt] = options[dist(rng)];
    plan.from = f;
    plan.to   = t;
    plan.amount = amt;
    plan.valid = true;
    return plan;
}

} // namespace RandomAI
