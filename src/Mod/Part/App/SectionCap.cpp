// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Gregg Jaskiewicz
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "PreCompiled.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <unordered_map>

#include <Base/Console.h>

#include "SectionCap.h"


using namespace Part;

namespace
{

/// When slicing, segments are scattered without a clear order.
/// ChainLoops must repeatedly determine the end of each unused segment.
/// Each scan is inefficient, like a quadratic search.
/// To simplify, we store endpoints by position.
/// A point is rounded to a cell tolerance wide. ...
/// Instead of checking every cell, we look for nearby ones.
/// The cell narrows down possibilities, but the exact distance test determines a match.

struct EndpointCell
{
    long long x = 0;
    long long y = 0;
    long long z = 0;

    bool operator==(const EndpointCell& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct EndpointCellHash
{
    std::size_t operator()(const EndpointCell& k) const noexcept
    {
        // three way mix, adequate for the handful of points a section produces
        std::size_t h = std::hash<long long> {}(k.x);
        h ^= std::hash<long long> {}(k.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<long long> {}(k.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

EndpointCell cellOf(const Base::Vector3d& p, double tolerance)
{
    const double inv = 1.0 / tolerance;
    return EndpointCell {
        static_cast<long long>(std::llround(p.x * inv)),
        static_cast<long long>(std::llround(p.y * inv)),
        static_cast<long long>(std::llround(p.z * inv))
    };
}

/// The 27 cells enclosing a point - itself and its 26 neighbours - so a lookup
/// still finds a partner that rounded into an adjacent cell.
void forEachNeighbouringCell(const EndpointCell& cell, const std::function<void(const EndpointCell&)>& fn)
{
    for (long long dx = -1; dx <= 1; ++dx) {
        for (long long dy = -1; dy <= 1; ++dy) {
            for (long long dz = -1; dz <= 1; ++dz) {
                fn(EndpointCell {cell.x + dx, cell.y + dy, cell.z + dz});
            }
        }
    }
}

}  // namespace


std::optional<Part::SectionCap::Segment> SectionCap::planeTriangleIntersection(
    const Base::Vector3d& a,
    const Base::Vector3d& b,
    const Base::Vector3d& c,
    const Base::Vector3d& normal,
    double offset
)
{
    const Base::Vector3d* p[3] = {&a, &b, &c};

    // signed distance from the plane
    const double s[3] = {a * normal - offset, b * normal - offset, c * normal - offset};

    // Half open test: exactly zero or two edges cross, so a vertex sitting
    // on the plane cannot yield a duplicate or a dangling segment.
    const bool above[3] = {s[0] > 0.0, s[1] > 0.0, s[2] > 0.0};
    if (above[0] == above[1] && above[1] == above[2]) {
        return std::nullopt;  // all on one side, or all on the plane
    }

    Base::Vector3d hit[2];
    int hits = 0;
    for (int e = 0; e < 3 && hits < 2; ++e) {
        const int i = e;
        const int j = (e + 1) % 3;
        if (above[i] == above[j]) {
            continue;
        }
        const double t = s[i] / (s[i] - s[j]);
        hit[hits++] = *p[i] + (*p[j] - *p[i]) * t;
    }
    // A triangle resting one vertex on the plane produces two crossings that
    // collapse onto that vertex. It does not cross the plane, and the zero
    // length segment would only confuse the chaining below.
    if (hits != 2 || Base::DistanceP2(hit[0], hit[1]) <= 0.0) {
        return std::nullopt;
    }

    return SectionCap::Segment {hit[0], hit[1]};
}


std::vector<SectionCap::Segment> SectionCap::sliceTriangles(
    const TriangleSoup& soup,
    const Base::Vector3d& normal,
    double offset
)
{
    std::vector<Segment> segments;
    if (soup.indices.size() < 3 || soup.points.empty()) {
        return segments;
    }

    const std::size_t pointCount = soup.points.size();
    segments.reserve(soup.indices.size() / 6);

    for (std::size_t i = 0; i + 2 < soup.indices.size(); i += 3) {
        const int ia = soup.indices[i];
        const int ib = soup.indices[i + 1];
        const int ic = soup.indices[i + 2];
        if (ia < 0 || ib < 0 || ic < 0 || static_cast<std::size_t>(ia) >= pointCount
            || static_cast<std::size_t>(ib) >= pointCount
            || static_cast<std::size_t>(ic) >= pointCount) {
            continue;
        }

        auto segment = planeTriangleIntersection(
            soup.points[ia],
            soup.points[ib],
            soup.points[ic],
            normal,
            offset
        );
        if (segment.has_value()) {
            segments.push_back(segment.value());
        }
    }

    return segments;
}


std::vector<std::vector<Base::Vector3d>> SectionCap::chainLoops(
    const std::vector<Segment>& segments,
    double tolerance
)
{
    std::vector<std::vector<Base::Vector3d>> loops;
    if (segments.empty() || tolerance <= 0.0) {
        return loops;
    }

    // Every segment filed under the cell of each of its two ends, so growing a
    // chain is a lookup rather than a scan over everything still unused.
    std::unordered_map<EndpointCell, std::vector<std::size_t>, EndpointCellHash> segmentsByEndpoint;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        segmentsByEndpoint[cellOf(segments[i].start, tolerance)].push_back(i);
        segmentsByEndpoint[cellOf(segments[i].end, tolerance)].push_back(i);
    }

    std::vector<bool> used(segments.size(), false);
    const double tolSq = tolerance * tolerance;

    auto findNext = [&](const Base::Vector3d& from) -> std::size_t {
        std::size_t found = segments.size();
        forEachNeighbouringCell(cellOf(from, tolerance), [&](const EndpointCell& k) {
            if (found != segments.size()) {
                return;
            }
            auto it = segmentsByEndpoint.find(k);
            if (it == segmentsByEndpoint.end()) {
                return;
            }
            for (std::size_t idx : it->second) {
                if (used[idx]) {
                    continue;
                }
                if (Base::DistanceP2(segments[idx].start, from) <= tolSq
                    || Base::DistanceP2(segments[idx].end, from) <= tolSq) {
                    found = idx;
                    return;
                }
            }
        });
        return found;
    };

    for (std::size_t seed = 0; seed < segments.size(); ++seed) {
        if (used[seed]) {
            continue;
        }
        used[seed] = true;

        std::vector<Base::Vector3d> loop {segments[seed].start, segments[seed].end};
        Base::Vector3d tail = segments[seed].end;

        while (true) {
            const std::size_t next = findNext(tail);
            if (next >= segments.size()) {
                break;
            }
            used[next] = true;
            // walk on from whichever end of the found segment is further away
            const bool startMatches = Base::DistanceP2(segments[next].start, tail) <= tolSq;
            tail = startMatches ? segments[next].end : segments[next].start;
            loop.push_back(tail);

            if (Base::DistanceP2(tail, loop.front()) <= tolSq) {
                break;  // closed
            }
        }

        if (loop.size() >= 3) {
            loops.push_back(std::move(loop));
        }
    }

    return loops;
}


SectionCap::TriangleSoup SectionCap::fillLoops(
    const std::vector<std::vector<Base::Vector3d>>& loops,
    const Base::Vector3d& u,
    const Base::Vector3d& v
)
{
    TriangleSoup soup;
    if (loops.empty()) {
        return soup;
    }

    // Same projection as the hatching, without the rotation: the fill has no
    // direction of its own.
    Base::Vector3d offsetFromOrigin(0, 0, 0);
    for (const auto& loop : loops) {
        if (!loop.empty()) {
            const Base::Vector3d& p = loop.front();
            offsetFromOrigin = p - u * (p * u) - v * (p * v);
            break;
        }
    }

    // An edge of the boundary in plane coordinates, kept low end first so a
    // band can ask whether it is inside the edge's span without reordering.
    struct Edge
    {
        double bLow, bHigh;
        double aLow, aHigh;
    };
    std::vector<Edge> edges;
    std::vector<double> levels;
    for (const auto& loop : loops) {
        if (loop.size() < 3) {
            continue;
        }
        for (std::size_t i = 0; i < loop.size(); ++i) {
            const Base::Vector3d& p = loop[i];
            const Base::Vector3d& q = loop[(i + 1) % loop.size()];
            const double a0 = p * u;
            const double b0 = p * v;
            const double a1 = q * u;
            const double b1 = q * v;
            if (!std::isfinite(a0) || !std::isfinite(b0) || !std::isfinite(a1) || !std::isfinite(b1)) {
                continue;
            }
            levels.push_back(b0);
            // An edge lying along a level is crossed by no band and would only
            // divide by zero below. Its ends still count as levels.
            if (b0 == b1) {
                continue;
            }
            edges.push_back(b0 < b1 ? Edge {b0, b1, a0, a1} : Edge {b1, b0, a1, a0});
        }
    }
    if (edges.empty()) {
        return soup;
    }

    std::sort(levels.begin(), levels.end());
    levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
    if (levels.size() < 2) {
        return soup;
    }

    // Reached in the order their low end comes up, so a band takes the edges
    // that have started without looking at the rest.
    std::sort(edges.begin(), edges.end(), [](const Edge& l, const Edge& r) {
        return l.bLow < r.bLow;
    });

    /// Where an edge sits at one level. Only asked for levels inside its span.
    auto crossingAt = [&edges](std::size_t index, double level) {
        const Edge& e = edges[index];
        return e.aLow + (e.aHigh - e.aLow) * (level - e.bLow) / (e.bHigh - e.bLow);
    };

    // One quad per span, so neighbouring bands meet exactly and leave no gaps.
    auto addQuad = [&soup, &u, &v, &offsetFromOrigin](
                       double leftLower,
                       double rightLower,
                       double leftUpper,
                       double rightUpper,
                       double lower,
                       double upper
                   ) {
        const int base = static_cast<int>(soup.points.size());
        soup.points.push_back(offsetFromOrigin + u * leftLower + v * lower);
        soup.points.push_back(offsetFromOrigin + u * rightLower + v * lower);
        soup.points.push_back(offsetFromOrigin + u * rightUpper + v * upper);
        soup.points.push_back(offsetFromOrigin + u * leftUpper + v * upper);
        soup.indices.insert(soup.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    };

    std::vector<std::size_t> active;
    std::size_t entering = 0;

    for (std::size_t band = 0; band + 1 < levels.size(); ++band) {
        const double lower = levels[band];
        const double upper = levels[band + 1];

        while (entering < edges.size() && edges[entering].bLow <= lower) {
            active.push_back(entering++);
        }

        // Left to right. No vertex falls inside a band, so no two edges meet
        // inside one either, and the order halfway up is the order throughout.
        const double middle = 0.5 * (lower + upper);
        std::sort(active.begin(), active.end(), [&](std::size_t l, std::size_t r) {
            return crossingAt(l, middle) < crossingAt(r, middle);
        });

        for (std::size_t i = 0; i + 1 < active.size(); i += 2) {
            const double leftLower = crossingAt(active[i], lower);
            const double rightLower = crossingAt(active[i + 1], lower);
            const double leftUpper = crossingAt(active[i], upper);
            const double rightUpper = crossingAt(active[i + 1], upper);
            if (rightLower > leftLower || rightUpper > leftUpper) {
                addQuad(leftLower, rightLower, leftUpper, rightUpper, lower, upper);
            }
        }

        // Retired once the band that ends them has been drawn.
        std::erase_if(active, [&edges, upper](std::size_t index) {
            return edges[index].bHigh <= upper;
        });
    }

    return soup;
}


bool SectionCap::extentAlong(
    const Base::BoundBox3d& bounds,
    const Base::Vector3d& normal,
    double& lo,
    double& hi
)
{
    if (!bounds.IsValid()) {
        return false;
    }

    // The corner furthest along the normal is the one picked axis by axis, so
    // the whole extent falls out of the centre plus a support radius. No need to
    // enumerate the eight corners, let alone the points inside them.
    const Base::Vector3d centre = bounds.GetCenter();
    const double reach = 0.5
        * (bounds.LengthX() * std::abs(normal.x) + bounds.LengthY() * std::abs(normal.y)
           + bounds.LengthZ() * std::abs(normal.z));

    const double middle = centre * normal;
    lo = middle - reach;
    hi = middle + reach;
    return true;
}


bool SectionCap::isClosed(const std::vector<Base::Vector3d>& loop, double tolerance)
{
    if (loop.size() < 3) {
        return false;
    }
    return Base::DistanceP2(loop.front(), loop.back()) <= tolerance * tolerance;
}

std::vector<SectionCap::Segment> SectionCap::hatchTriangles(
    const TriangleSoup& cap,
    const Base::Vector3d& levelDir,
    double spacing,
    std::size_t maxSegments
)
{
    std::vector<Segment> hatch;
    if (cap.indices.size() < 3 || cap.points.empty() || !std::isfinite(spacing) || spacing <= 0.0) {
        return hatch;
    }

    const double dirLength = levelDir.Length();
    if (!std::isfinite(dirLength) || dirLength <= 0.0) {
        return hatch;
    }
    const Base::Vector3d dir = levelDir / dirLength;

    const std::size_t pointCount = cap.points.size();

    for (std::size_t i = 0; i + 2 < cap.indices.size(); i += 3) {
        if (hatch.size() >= maxSegments) {
            break;
        }

        const int ia = cap.indices[i];
        const int ib = cap.indices[i + 1];
        const int ic = cap.indices[i + 2];
        if (ia < 0 || ib < 0 || ic < 0 || static_cast<std::size_t>(ia) >= pointCount
            || static_cast<std::size_t>(ib) >= pointCount
            || static_cast<std::size_t>(ic) >= pointCount) {
            continue;
        }

        const Base::Vector3d p[3] = {cap.points[ia], cap.points[ib], cap.points[ic]};
        const double s[3] = {p[0] * dir, p[1] * dir, p[2] * dir};
        if (!std::isfinite(s[0]) || !std::isfinite(s[1]) || !std::isfinite(s[2])) {
            continue;
        }

        // Only the levels this triangle actually spans, which is what makes the
        // cost follow the output rather than the level count.
        const double lowest = std::min({s[0], s[1], s[2]});
        const double highest = std::max({s[0], s[1], s[2]});

        // Bounded while still in floating point, where overflow saturates
        // instead of being undefined as an out of range cast would be.
        const double firstD = std::ceil(lowest / spacing);
        const double lastD = std::floor(highest / spacing);
        if (!(lastD >= firstD) || !(lastD - firstD < static_cast<double>(maxSegments))) {
            continue;
        }

        for (auto k = static_cast<std::int64_t>(firstD); k <= static_cast<std::int64_t>(lastD); ++k) {
            if (hatch.size() >= maxSegments) {
                break;
            }
            const double level = static_cast<double>(k) * spacing;

            // Half open sign test, as everywhere else here: a triangle yields
            // exactly zero or two crossings, so a vertex sitting on a hatch
            // line cannot produce a duplicate or a dangling segment.
            const bool above[3] = {s[0] > level, s[1] > level, s[2] > level};
            if (above[0] == above[1] && above[1] == above[2]) {
                continue;
            }

            Base::Vector3d hit[2];
            int hits = 0;
            for (int e = 0; e < 3 && hits < 2; ++e) {
                const int a = e;
                const int b = (e + 1) % 3;
                if (above[a] == above[b]) {
                    continue;
                }
                const double t = (level - s[a]) / (s[b] - s[a]);
                hit[hits++] = p[a] + (p[b] - p[a]) * t;
            }
            // A triangle touching the line at one vertex gives two crossings
            // that collapse onto it. That is a touch, not a crossing, and a
            // zero length line would only be drawn as nothing.
            if (hits != 2 || Base::DistanceP2(hit[0], hit[1]) <= 0.0) {
                continue;
            }

            hatch.push_back(Segment {hit[0], hit[1]});
        }
    }

    return hatch;
}
