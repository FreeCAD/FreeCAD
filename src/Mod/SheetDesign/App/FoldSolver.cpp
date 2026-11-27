// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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


#include <algorithm>
#include <cmath>
#include <map>
#include <set>

#include <Eigen/Dense>

#include <Base/Exception.h>

#include "FoldSolver.h"
#include "SheetModel.h"

using namespace SheetDesign;

namespace
{

using Vec = Eigen::Vector3d;
using Mat = Eigen::Matrix4d;

Mat rotationAboutLine(const Vec& point, const Vec& direction, double angle)
{
    const Eigen::Matrix3d r = Eigen::AngleAxisd(angle, direction.normalized()).toRotationMatrix();
    Mat m = Mat::Identity();
    m.block<3, 3>(0, 0) = r;
    m.block<3, 1>(0, 3) = point - r * point;
    return m;
}

}  // namespace

struct FoldSolver::Data
{
    struct Hinge
    {
        int parent {-1};
        int child {-1};
        Vec a;  // The hinge line in flat space.
        Vec b;
        Vec axis;
        double final {0.0};
        bool closing {false};
    };

    std::vector<Hinge> hinges;
    std::vector<std::vector<int>> scores;
    std::vector<int> order;
    int root {0};
    int panels {0};
    double thickness {1.0};
    std::vector<int> tree;  // Bends that give a panel its position, parent before child.
    std::vector<int> closing;
    std::vector<bool> inLoop;             // A tree bend that is on the path of a closing bend.
    std::vector<std::vector<int>> loops;  // For each closing bend, the tree bends of its loop.
    std::set<int> locked;

    double length(int i) const
    {
        return (hinges[i].b - hinges[i].a).norm();
    }

    std::vector<Mat> transforms(const Eigen::VectorXd& theta) const
    {
        std::vector<Mat> t(panels, Mat::Identity());
        for (int i : tree) {
            const Hinge& h = hinges[i];
            t[h.child] = t[h.parent] * rotationAboutLine(h.a, h.axis, theta[i]);
        }
        return t;
    }

    Eigen::VectorXd residual(const Eigen::VectorXd& theta) const
    {
        const std::vector<Mat> t = transforms(theta);
        Eigen::VectorXd r(6 * static_cast<Eigen::Index>(closing.size()));
        Eigen::Index row = 0;
        for (int c : closing) {
            const Hinge& h = hinges[c];
            for (const Vec* p : {&h.a, &h.b}) {
                const Eigen::Vector4d q(p->x(), p->y(), p->z(), 1.0);
                const Eigen::Vector4d diff = t[h.parent] * q - t[h.child] * q;
                r.segment<3>(row) = diff.head<3>();
                row += 3;
            }
        }
        return r;
    }

    double worst(const Eigen::VectorXd& theta) const
    {
        if (closing.empty()) {
            return 0.0;
        }
        return residual(theta).cwiseAbs().maxCoeff();
    }

    /// The bends that can move with a bend: those in a loop with it, through bends that are
    /// not locked. A locked bend divides the part into mechanisms that do not depend on
    /// each other.
    std::set<int> mechanism(int bend) const
    {
        std::set<int> result {bend};
        bool grown = true;
        while (grown) {
            grown = false;
            for (const std::vector<int>& loop : loops) {
                bool touches = false;
                for (int i : loop) {
                    touches = touches || (result.count(i) && !locked.count(i));
                }
                if (!touches) {
                    continue;
                }
                for (int i : loop) {
                    if (!locked.count(i) && result.insert(i).second) {
                        grown = true;
                    }
                }
            }
        }
        return result;
    }

    /// Put the bends of each score on one line, and move a single score to a meeting point.
    void idealize()
    {
        struct Line
        {
            Vec p;
            Vec dir;
        };
        std::vector<Line> lines;
        for (const std::vector<int>& group : scores) {
            int main = group.front();
            for (int i : group) {
                if (length(i) > length(main)) {
                    main = i;
                }
            }
            const Vec p = hinges[main].a;
            const Vec dir = (hinges[main].b - hinges[main].a).normalized();
            lines.push_back({p, dir});
            for (int i : group) {
                Hinge& h = hinges[i];
                h.a = p + dir * (h.a - p).dot(dir);
                h.b = p + dir * (h.b - p).dot(dir);
            }
        }
        // The points where two scores meet, if each has a bend that ends near the point.
        std::vector<std::pair<Vec, std::pair<int, int>>> points;
        const double reach = 6.0 * thickness;
        auto near = [&](const std::vector<int>& group, const Vec& x) {
            for (int i : group) {
                if ((x - hinges[i].a).norm() < reach || (x - hinges[i].b).norm() < reach) {
                    return true;
                }
            }
            return false;
        };
        for (std::size_t i = 0; i < lines.size(); ++i) {
            for (std::size_t k = i + 1; k < lines.size(); ++k) {
                const Vec& d1 = lines[i].dir;
                const Vec& d2 = lines[k].dir;
                const double cross = d1.x() * d2.y() - d1.y() * d2.x();
                if (std::fabs(cross) < 0.2) {
                    continue;
                }
                const Vec w = lines[k].p - lines[i].p;
                const double s = (w.x() * d2.y() - w.y() * d2.x()) / cross;
                const Vec x = lines[i].p + d1 * s;
                if (near(scores[i], x) && near(scores[k], x)) {
                    points.push_back({x, {static_cast<int>(i), static_cast<int>(k)}});
                }
            }
        }
        for (std::size_t g = 0; g < scores.size(); ++g) {
            if (scores[g].size() != 1) {
                continue;
            }
            Hinge& h = hinges[scores[g].front()];
            const Vec dir = (h.b - h.a).normalized();
            bool found = false;
            Vec best = Vec::Zero();
            for (const auto& point : points) {
                if (point.second.first == static_cast<int>(g)
                    || point.second.second == static_cast<int>(g)) {
                    continue;
                }
                const Vec to = point.first - h.a;
                const Vec off = to - dir * to.dot(dir);
                const double end = std::min((point.first - h.a).norm(), (point.first - h.b).norm());
                if (off.norm() < 3.0 * thickness && end < reach
                    && (!found || off.norm() < best.norm())) {
                    best = off;
                    found = true;
                }
            }
            if (found) {
                h.a += best;
                h.b += best;
            }
        }
    }

    /// Find the free angles that close the loops, near a reference. Returns the new angles.
    Eigen::VectorXd close(
        const Eigen::VectorXd& theta,
        const std::vector<int>& free,
        const Eigen::VectorXd& ref,
        const Eigen::VectorXd& weight,
        int iterations = 150
    ) const
    {
        const Eigen::Index n = static_cast<Eigen::Index>(free.size());
        if (n == 0 || closing.empty()) {
            return theta;
        }
        Eigen::VectorXd x = theta;
        const Eigen::Index m = 6 * static_cast<Eigen::Index>(closing.size());
        auto cost = [&](const Eigen::VectorXd& v) {
            Eigen::VectorXd t = x;
            Eigen::VectorXd r(m + n);
            for (Eigen::Index k = 0; k < n; ++k) {
                t[free[k]] = v[k];
                r[m + k] = weight[free[k]] * (v[k] - ref[free[k]]);
            }
            r.head(m) = residual(t);
            return r;
        };
        Eigen::VectorXd v(n);
        Eigen::VectorXd low(n);
        Eigen::VectorXd high(n);
        const double bound = 0.05;
        for (Eigen::Index k = 0; k < n; ++k) {
            v[k] = x[free[k]];
            low[k] = -bound;
            high[k] = hinges[free[k]].final + bound;
        }
        Eigen::VectorXd r = cost(v);
        double damp = 1.0e-3;
        const double h = 1.0e-6;
        for (int iteration = 0; iteration < iterations; ++iteration) {
            Eigen::MatrixXd jac(m + n, n);
            for (Eigen::Index k = 0; k < n; ++k) {
                Eigen::VectorXd moved = v;
                moved[k] += h;
                jac.col(k) = (cost(moved) - r) / h;
            }
            const Eigen::VectorXd gradient = jac.transpose() * r;
            if (gradient.norm() < 1.0e-9) {
                break;
            }
            const Eigen::MatrixXd normal = jac.transpose() * jac;
            bool better = false;
            double stepSize = 0.0;
            for (int attempt = 0; attempt < 8; ++attempt) {
                const Eigen::MatrixXd system = normal + damp * Eigen::MatrixXd::Identity(n, n);
                const Eigen::VectorXd step = system.ldlt().solve(-gradient);
                const Eigen::VectorXd candidate = (v + step).cwiseMax(low).cwiseMin(high);
                const Eigen::VectorXd r2 = cost(candidate);
                if (r2.squaredNorm() < r.squaredNorm()) {
                    stepSize = (candidate - v).norm();
                    v = candidate;
                    r = r2;
                    damp = std::max(damp / 3.0, 1.0e-9);
                    better = true;
                    break;
                }
                damp *= 5.0;
            }
            if (!better || stepSize < 1.0e-9) {
                break;
            }
        }
        for (Eigen::Index k = 0; k < n; ++k) {
            x[free[k]] = v[k];
        }
        return x;
    }
};

FoldSolver::FoldSolver(
    const SheetModel& model,
    const std::vector<std::vector<int>>& scores,
    const std::vector<int>& order
)
    : d(new Data)
{
    const int count = static_cast<int>(model.bends.size());
    d->panels = static_cast<int>(model.panels.size());
    d->thickness = model.params.thickness;
    for (int i = 0; i < d->panels; ++i) {
        if (model.panels[i].parentBend < 0) {
            d->root = i;
            break;
        }
    }
    for (const Bend& bend : model.bends) {
        Data::Hinge h;
        h.parent = bend.parentPanel;
        h.child = bend.childPanel;
        const Vec normal(bend.normal.X(), bend.normal.Y(), 0.0);
        const double half = bend.allowance / 2.0;
        h.a = Vec(bend.start.X(), bend.start.Y(), 0.0) + normal * half;
        h.b = Vec(bend.end.X(), bend.end.Y(), 0.0) + normal * half;
        h.axis = Vec(normal.y(), -normal.x(), 0.0) * (bend.up ? 1.0 : -1.0);
        h.final = bend.angle;
        h.closing = bend.closing;
        d->hinges.push_back(h);
    }
    // The scores. A bend that is in no score is a score of its own.
    std::vector<bool> used(count, false);
    for (std::size_t g = 0; g < scores.size(); ++g) {
        std::vector<int> group;
        for (int i : scores[g]) {
            if (i >= 0 && i < count && !used[i]) {
                group.push_back(i);
                used[i] = true;
            }
        }
        if (!group.empty()) {
            d->scores.push_back(group);
            d->order.push_back(g < order.size() ? order[g] : static_cast<int>(g) + 1);
        }
    }
    for (int i = 0; i < count; ++i) {
        if (!used[i]) {
            d->scores.push_back({i});
            d->order.push_back(static_cast<int>(d->scores.size()));
        }
    }
    // The tree, with each parent before its children.
    std::map<int, std::vector<int>> children;
    std::map<int, int> parentBend;
    for (int i = 0; i < count; ++i) {
        if (d->hinges[i].closing) {
            d->closing.push_back(i);
        }
        else {
            children[d->hinges[i].parent].push_back(i);
            parentBend[d->hinges[i].child] = i;
        }
    }
    std::vector<int> queue {d->root};
    for (std::size_t head = 0; head < queue.size(); ++head) {
        for (int i : children[queue[head]]) {
            d->tree.push_back(i);
            queue.push_back(d->hinges[i].child);
        }
    }
    // The tree bends on the path between the two panels of each closing bend.
    d->inLoop.assign(count, false);
    auto path = [&](int panel) {
        std::set<int> result;
        while (panel != d->root && parentBend.count(panel)) {
            const int i = parentBend[panel];
            result.insert(i);
            panel = d->hinges[i].parent;
        }
        return result;
    };
    for (int c : d->closing) {
        const std::set<int> first = path(d->hinges[c].parent);
        const std::set<int> second = path(d->hinges[c].child);
        std::vector<int> loop;
        for (int i : first) {
            if (!second.count(i)) {
                d->inLoop[i] = true;
                loop.push_back(i);
            }
        }
        for (int i : second) {
            if (!first.count(i)) {
                d->inLoop[i] = true;
                loop.push_back(i);
            }
        }
        d->loops.push_back(loop);
    }
    d->idealize();
}

FoldSolver::~FoldSolver()
{
    delete d;
}

bool FoldSolver::hasLoops() const
{
    return !d->closing.empty();
}

double FoldSolver::mismatch(const std::vector<double>& angles) const
{
    Eigen::VectorXd theta = Eigen::VectorXd::Zero(static_cast<Eigen::Index>(d->hinges.size()));
    for (std::size_t i = 0; i < angles.size() && i < d->hinges.size(); ++i) {
        theta[static_cast<Eigen::Index>(i)] = angles[i];
    }
    return d->worst(theta);
}

FoldPath FoldSolver::solve(int steps)
{
    steps = std::max(steps, 2);
    const Eigen::Index count = static_cast<Eigen::Index>(d->hinges.size());
    FoldPath path;
    Eigen::VectorXd theta = Eigen::VectorXd::Zero(count);
    d->locked.clear();
    const double closed = 1.0e-3;  // mm

    auto record = [&](int score) {
        path.states.emplace_back(theta.data(), theta.data() + count);
        path.drivers.push_back(score);
        path.mismatch = std::max(path.mismatch, d->worst(theta));
    };
    record(0);

    // The scores in their sequence, each with its main bend: the longest bend of the tree.
    std::vector<std::size_t> sequence(d->scores.size());
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        sequence[i] = i;
    }
    std::stable_sort(sequence.begin(), sequence.end(), [&](std::size_t a, std::size_t b) {
        return d->order[a] < d->order[b];
    });
    std::vector<int> mains(d->scores.size(), -1);
    for (std::size_t g = 0; g < d->scores.size(); ++g) {
        for (int i : d->scores[g]) {
            if (!d->hinges[i].closing && (mains[g] < 0 || d->length(i) > d->length(mains[g]))) {
                mains[g] = i;
            }
        }
    }

    std::vector<bool> done(d->scores.size(), false);
    std::vector<std::string> notes(d->scores.size());
    auto foldScore = [&](std::size_t g) -> bool {
        const int main = mains[g];
        const std::string name = "Score " + std::to_string(d->order[g]);
        if (main < 0) {
            notes[g] = name + ": closes a loop";
            return true;
        }
        const double target = d->hinges[main].final;
        if (!d->inLoop[main]) {
            // No loop: the bends of this score fold alone.
            for (int k = 1; k <= steps; ++k) {
                for (int i : d->scores[g]) {
                    if (!d->hinges[i].closing && !d->inLoop[i]) {
                        theta[i] = d->hinges[i].final * k / steps;
                    }
                }
                record(d->order[g]);
            }
            d->locked.insert(main);
            notes[g] = name + ": driver";
            return true;
        }
        if (std::fabs(theta[main] - target) < 1.0e-3) {
            d->locked.insert(main);
            notes[g] = name + ": followed an earlier score";
            return true;
        }
        // The main bends of the later scores. First they stay. If the loops cannot close
        // then, they are free and move as little as possible.
        std::set<int> later;
        for (std::size_t other = 0; other < mains.size(); ++other) {
            if (other != g && !done[other] && mains[other] >= 0) {
                later.insert(mains[other]);
            }
        }
        const std::set<int> active = d->mechanism(main);
        const double first = std::min(target - theta[main], 2.0 * M_PI / 180.0);
        bool started = false;
        bool laterFree = false;
        std::vector<int> free;
        Eigen::VectorXd weight = Eigen::VectorXd::Zero(count);
        Eigen::VectorXd start = theta;
        for (int attempt = 0; attempt < 2 && !started; ++attempt) {
            laterFree = attempt == 1;
            free.clear();
            weight.setZero();
            for (Eigen::Index i = 0; i < count; ++i) {
                const int index = static_cast<int>(i);
                if (!d->inLoop[index] || index == main || d->locked.count(index)
                    || !active.count(index)) {
                    continue;
                }
                if (later.count(index)) {
                    if (!laterFree) {
                        continue;
                    }
                    weight[i] = 0.3;
                }
                free.push_back(index);
            }
            // The start of a coupled fold is a special point with two ways. Try some first
            // guesses, and take the one that closes the loops with all angles on the side of
            // their end angle.
            for (double part : {0.0, 0.2, 0.1, 0.3, 0.05, 0.45}) {
                Eigen::VectorXd guess = theta;
                guess[main] += first;
                for (int i : free) {
                    guess[i] += part * (d->hinges[i].final - guess[i]);
                }
                // The weights keep the later scores at rest where the loops permit it. The
                // second call has no weights: the loops close exactly, near the first result.
                Eigen::VectorXd result = d->close(guess, free, theta, weight);
                result = d->close(result, free, result, Eigen::VectorXd::Zero(count));
                bool side = true;
                for (int i : free) {
                    side = side && result[i] > -1.0e-3;
                }
                if (side && d->worst(result) < closed) {
                    start = result;
                    started = true;
                    break;
                }
            }
        }
        if (!started) {
            notes[g] = name + ": cannot move at this place of the sequence";
            return false;
        }
        theta = start;
        record(d->order[g]);
        const double from = theta[main];
        for (int k = 1; k <= steps; ++k) {
            theta[main] = from + (target - from) * k / steps;
            Eigen::VectorXd stay = weight;
            for (int i : free) {
                stay[i] = std::max(stay[i], 0.02);
            }
            theta = d->close(theta, free, theta, stay);
            theta = d->close(theta, free, theta, Eigen::VectorXd::Zero(count));
            record(d->order[g]);
        }
        d->locked.insert(main);
        notes[g] = name + (laterFree ? ": driver, with later scores" : ": driver");
        return true;
    };

    // A score that cannot move waits. It gets a new attempt after a later score moved,
    // because that score can free it.
    bool progress = true;
    while (progress) {
        progress = false;
        for (std::size_t g : sequence) {
            if (!done[g] && foldScore(g)) {
                done[g] = true;
                progress = true;
            }
        }
    }
    for (std::size_t g : sequence) {
        path.notes.push_back(notes[g]);
    }

    for (int i : d->tree) {
        path.endError = std::max(path.endError, std::fabs(theta[i] - d->hinges[i].final));
    }
    path.complete = path.endError < 0.02 && path.mismatch < 0.05;
    return path;
}
