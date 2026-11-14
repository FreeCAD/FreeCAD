// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QPointF>
#include <QRectF>

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace TechDrawGui
{
// Bounds and obstacles share page coordinates; obstacles already include clearance.
// Return the nearest free position on the sheet, or no result if none fits.
inline std::optional<QPointF> findAlignedViewPlacement(const QRectF& bounds,
                                       const QPointF& direction,
                                       const QRectF& paper,
                                       const std::vector<QRectF>& obstacles)
{
    // Every free interval begins at a page edge or an obstacle edge.
    // Testing these events avoids a grid search and also finds narrow gaps.
    std::vector<double> candidates{0.0};
    auto addCandidate = [&](double distance) {
        candidates.push_back(distance);
        // Include both sides of an edge to tolerate floating point rounding,
        // while retaining exact fits and positions already clear of obstacles.
        candidates.push_back(distance - 1.0e-5);
        candidates.push_back(distance + 1.0e-5);
    };
    auto addEdges = [&](const QRectF& rect, bool inside) {
        const double x1 = rect.left() - (inside ? bounds.left() : bounds.right());
        const double x2 = rect.right() - (inside ? bounds.right() : bounds.left());
        const double y1 = rect.top() - (inside ? bounds.top() : bounds.bottom());
        const double y2 = rect.bottom() - (inside ? bounds.bottom() : bounds.top());
        if (std::abs(direction.x()) > 1.0e-9) {
            addCandidate(x1 / direction.x());
            addCandidate(x2 / direction.x());
        }
        if (std::abs(direction.y()) > 1.0e-9) {
            addCandidate(y1 / direction.y());
            addCandidate(y2 / direction.y());
        }
    };
    addEdges(paper, true);
    for (const auto& obstacle : obstacles) {
        addEdges(obstacle, false);
    }
    std::sort(candidates.begin(), candidates.end(), [](double a, double b) {
        return std::abs(a) == std::abs(b) ? a > b : std::abs(a) < std::abs(b);
    });
    for (double distance : candidates) {
        const QPointF movement = direction * distance;
        const QRectF proposed = bounds.translated(movement);
        if (!paper.contains(proposed)) {
            continue;
        }
        if (std::any_of(obstacles.begin(), obstacles.end(),
                        [&](const QRectF& obstacle) {
                            return proposed.intersects(obstacle);
                        })) {
            continue;
        }
        return movement;
    }
    return {};
}
}  // namespace TechDrawGui
