#include "MapSpec.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace {

constexpr int kRows = 15;           // canvas rows
constexpr int kCols = 25;           // canvas cols
constexpr int kN = 20;              // territories
constexpr int kMinSep = 2;          // minimum spacing
constexpr int kExtraEdgesTarget = 10;
constexpr int kMaxDegree = 4;

// Euclidean distance for layout
inline double dist(int r1, int c1, int r2, int c2) {
    double dr = static_cast<double>(r1 - r2);
    double dc = static_cast<double>(c1 - c2);
    return std::sqrt(dr * dr + dc * dc);
}

// Chebyshev (king-move) distance
inline int cheb(int r1, int c1, int r2, int c2) {
    return std::max(std::abs(r1 - r2), std::abs(c1 - c2));
}

// Build a Prim MST and return undirected edge list (a<b)
std::vector<std::pair<int,int>> buildMST(const std::vector<std::pair<int,int>>& pts) {
    int n = static_cast<int>(pts.size());
    std::vector<double> best(n, std::numeric_limits<double>::infinity());
    std::vector<int> parent(n, -1);
    std::vector<char> used(n, 0);
    best[0] = 0.0;

    for (int it = 0; it < n; ++it) {
        int v = -1;
        for (int i = 0; i < n; ++i)
            if (!used[i] && (v == -1 || best[i] < best[v])) v = i;
        used[v] = 1;
        for (int u = 0; u < n; ++u) {
            if (used[u]) continue;
            double d = dist(pts[v].first, pts[v].second, pts[u].first, pts[u].second);
            if (d < best[u]) { best[u] = d; parent[u] = v; }
        }
    }

    std::vector<std::pair<int,int>> edges;
    edges.reserve(n - 1);
    for (int u = 1; u < n; ++u) {
        int v = parent[u];
        if (v >= 0) edges.emplace_back(std::min(u,v), std::max(u,v));
    }
    return edges;
}

// Add a few extra near-neighbor edges within degree limits.
void addExtraEdges(const std::vector<std::pair<int,int>>& pts,
                   std::vector<std::pair<int,int>>& edges) {
    const int n = static_cast<int>(pts.size());

    auto hasEdge = [&](int a, int b) {
        if (a > b) std::swap(a,b);
        return std::find(edges.begin(), edges.end(), std::pair<int,int>(a,b)) != edges.end();
    };

    auto degreeOf = [&](int v) {
        int d = 0;
        for (auto& e : edges) if (e.first == v || e.second == v) ++d;
        return d;
    };

    struct Pair { int a,b; double d; };
    std::vector<Pair> cand;
    cand.reserve(n*(n-1)/2);
    for (int a = 0; a < n; ++a)
        for (int b = a+1; b < n; ++b)
            cand.push_back({a,b,dist(pts[a].first,pts[a].second,pts[b].first,pts[b].second)});
    std::sort(cand.begin(), cand.end(), [](const Pair& x, const Pair& y){ return x.d < y.d; });

    int added = 0;
    for (const auto& p : cand) {
        if (added >= kExtraEdgesTarget) break;
        if (hasEdge(p.a,p.b)) continue;
        if (degreeOf(p.a) >= kMaxDegree || degreeOf(p.b) >= kMaxDegree) continue;
        edges.emplace_back(p.a,p.b);
        ++added;
    }
}

// Randomly place 20 points on grid with spacing
std::vector<std::pair<int,int>> placePoints(std::mt19937& rng) {
    std::uniform_int_distribution<int> rd(0, kRows - 1);
    std::uniform_int_distribution<int> cd(0, kCols - 1);

    std::vector<std::pair<int,int>> pts;
    pts.reserve(kN);

    int tries = 0;
    while ((int)pts.size() < kN && tries < 10000) {
        ++tries;
        int r = rd(rng), c = cd(rng);
        bool ok = true;
        for (auto [pr, pc] : pts)
            if (cheb(r,c,pr,pc) < kMinSep) { ok = false; break; }
        if (ok) pts.emplace_back(r,c);
    }

    // Fallback if spacing fails
    if ((int)pts.size() < kN) {
        pts.clear();
        while ((int)pts.size() < kN) {
            int r = rd(rng), c = cd(rng);
            bool used = false;
            for (auto [pr, pc] : pts)
                if (pr==r && pc==c) { used=true; break; }
            if (!used) pts.emplace_back(r,c);
        }
    }
    return pts;
}

} // anonymous namespace

// ------------------------------------------------------------
namespace MapSpec {

std::vector<Territory> build20() {
    std::random_device rd;
    return build20(rd());
}

std::vector<Territory> build20(unsigned seed) {
    std::mt19937 rng(seed);

    // 1) positions
    auto pts = placePoints(rng);

    // 2) MST connectivity
    auto edges = buildMST(pts);

    // 3) add near-neighbor edges
    addExtraEdges(pts, edges);

    // 4) build Territory list
    std::vector<Territory> t(kN);
    const char* codes = "ABCDEFGHIJKLMNOPQRST";
    for (int i = 0; i < kN; ++i) {
        t[i].code = codes[i];
        t[i].name = std::string(1, codes[i]);
        t[i].owner = PlayerId::None;
        t[i].armies = 0;
        t[i].r = pts[i].first;
        t[i].c = pts[i].second;
    }

    // 5) fill adjacency
    for (auto [a,b] : edges) {
        t[a].adj.push_back(b);
        t[b].adj.push_back(a);
    }
    return t;
}

} // namespace MapSpec
