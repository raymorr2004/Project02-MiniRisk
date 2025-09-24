#include "MapSpec.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

// Canvas size for ASCII rendering (rows, cols)
constexpr int kRows = 15;
constexpr int kCols = 25;

// Territory count
constexpr int kN = 20;

// Minimum Chebyshev spacing between placed nodes (avoid visual overlap)
constexpr int kMinSep = 2;

// After MST, try to add up to this many extra edges total (keeps it Risk-like)
constexpr int kExtraEdgesTarget = 10;

// Hard cap on node degree (to avoid hairballs)
constexpr int kMaxDegree = 4;

// Euclidean distance (double) for layout heuristics
inline double dist(int r1, int c1, int r2, int c2) {
    const double dr = static_cast<double>(r1 - r2);
    const double dc = static_cast<double>(c1 - c2);
    return std::sqrt(dr*dr + dc*dc);
}

// Chebyshev distance for spacing check (king-move distance)
inline int cheb(int r1, int c1, int r2, int c2) {
    return std::max(std::abs(r1 - r2), std::abs(c1 - c2));
}

// Build a Prim MST over points; returns undirected edge list (u,v), u<v.
std::vector<std::pair<int,int>> buildMST(const std::vector<std::pair<int,int>>& pts) {
    const int n = static_cast<int>(pts.size());
    std::vector<double> best(n, std::numeric_limits<double>::infinity());
    std::vector<int> parent(n, -1);
    std::vector<char> used(n, 0);
    best[0] = 0.0;

    for (int it = 0; it < n; ++it) {
        int v = -1;
        for (int i = 0; i < n; ++i) {
            if (!used[i] && (v == -1 || best[i] < best[v])) v = i;
        }
        used[v] = 1;
        for (int u = 0; u < n; ++u) {
            if (used[u]) continue;
            double d = dist(pts[v].first, pts[v].second, pts[u].first, pts[u].second);
            if (d < best[u]) {
                best[u] = d;
                parent[u] = v;
            }
        }
    }

    std::vector<std::pair<int,int>> edges;
    edges.reserve(n - 1);
    for (int u = 1; u < n; ++u) {
        int v = parent[u];
        if (v >= 0) {
            int a = std::min(u, v), b = std::max(u, v);
            edges.emplace_back(a, b);
        }
    }
    return edges;
}

// Add extra near-neighbor edges until targets are met, respecting degree caps.
void addExtraEdges(const std::vector<std::pair<int,int>>& pts,
                   std::vector<std::pair<int,int>>& edges) {
    const int n = static_cast<int>(pts.size());

    auto hasEdge = [&](int a, int b) {
        if (a > b) std::swap(a, b);
        return std::find(edges.begin(), edges.end(), std::pair<int,int>(a,b)) != edges.end();
    };

    auto degreeOf = [&](int v) {
        int d = 0;
        for (auto& e : edges) d += (e.first == v || e.second == v);
        return d;
    };

    // Precompute all candidate pairs sorted by distance
    struct Pair { int a, b; double d; };
    std::vector<Pair> cand;
    cand.reserve(n*(n-1)/2);
    for (int a = 0; a < n; ++a) {
        for (int b = a+1; b < n; ++b) {
            cand.push_back({a, b, dist(pts[a].first, pts[a].second, pts[b].first, pts[b].second)});
        }
    }
    std::sort(cand.begin(), cand.end(), [](const Pair& x, const Pair& y){ return x.d < y.d; });

    int added = 0;
    for (const auto& p : cand) {
        if (added >= kExtraEdgesTarget) break;
        if (hasEdge(p.a, p.b)) continue;
        if (degreeOf(p.a) >= kMaxDegree || degreeOf(p.b) >= kMaxDegree) continue;
        edges.emplace_back(p.a, p.b);
        ++added;
    }
}

// Place 20 points on the canvas with spacing; returns vector of (r,c).
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
        for (auto [pr, pc] : pts) {
            if (cheb(r, c, pr, pc) < kMinSep) { ok = false; break; }
        }
        if (ok) pts.emplace_back(r, c);
    }

    // If we fail due to unlucky spacing, fall back to a looser placement (reduce spacing).
    if ((int)pts.size() < kN) {
        pts.clear();
        while ((int)pts.size() < kN) {
            int r = rd(rng), c = cd(rng);
            bool used = false;
            for (auto [pr, pc] : pts) {
                if (pr == r && pc == c) { used = true; break; }
            }
            if (!used) pts.emplace_back(r, c);
        }
    }
    return pts;
}

} // anonymous namespace

namespace MapSpec {

std::vector<Territory> build20() {
    std::random_device rd;
    return build20(rd());
}

std::vector<Territory> build20(unsigned seed) {
    std::mt19937 rng(seed);

    // 1) Random positions for 20 territories on a big canvas
    auto pts = placePoints(rng);

    // 2) Ensure connectivity with an MST
    auto edges = buildMST(pts);

    // 3) Add near-neighbor extra edges to avoid a skinny tree
    addExtraEdges(pts, edges);

    // 4) Construct Territory vector (A..T)
    std::vector<Territory> t(kN);
    const char* codes = "ABCDEFGHIJKLMNOPQRST";
    for (int i = 0; i < kN; ++i) {
        t[i].code  = codes[i];
        t[i].name  = std::string(1, codes[i]);
        t[i].owner = PlayerId::None;
        t[i].armies = 0;
        t[i].r = pts[i].first;
        t[i].c = pts[i].second;
    }

    // 5) Fill undirected adjacency from edge list
    for (auto [a,b] : edges) {
        t[a].adj.push_back(b);
        t[b].adj.push_back(a);
    }

    return t;
}

} // namespace MapSpec
