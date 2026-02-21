
/***************************************************************************
 *   Copyright (c) 2025 Théo Veilleux-Trinh <theo.veilleux.trinh@proton.me>*
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "Substitution.h"

#include "Constraints.h"
#include "Util.h"

#include <cstdint>
#include <utility>
#include <vector>
#include <iostream>
#include <queue>
#include <optional>

#include <Precision.hxx>

namespace GCS
{

// The substitutiton factory does not deal with
// parameters, only unknowns (and constants)
struct SubstitutionFactory
{
    enum class Attempt : std::uint8_t
    {
        Yes,     // Substitution is possible
        No,      // Substitution is not possible
        Maybe,   // Substitution could be possible if some parameters are right
        Unknown  // Not checked yet
    };
    enum class Orientation : std::uint8_t
    {
        None,
        Vertical,
        Horizontal
    };

    struct LineDesc
    {
        bool isExternal {false};
        Orientation orientation {Orientation::None};
    };

    std::unordered_map<double*, std::unordered_map<double*, double>> adjacencyList;

    std::unordered_map<double*, double> constUnknowns;

    // Each unknown that will be kept as a parameter to a list of unknowns that are equal,
    // will be used to build the reduction map which will go the otherway around
    // one-to-many to keep track while building => many-to-one for solving
    std::unordered_map<double*, std::vector<double*>> reductionList;

    std::vector<ConstraintDifference*> differenceConstraints;
    std::vector<double> differences;

    std::vector<ConstraintEqual*> constantConstraints;
    std::vector<double> constants;

    std::vector<std::pair<Line, double>> linesOfKnownLength;
    USET_pD unknowns;
    std::vector<Attempt> attempts;

    enum class RelationshipOptions : std::uint8_t
    {
        Relative,  // [default] Adds a relationship of the form A = B + offset iif the initial
                   // condition are B <= A and A = B - offset otherwise
        Absolute   // Adds a relationship of the form A = B + offset
    };

    SubstitutionFactory() = default;
    explicit SubstitutionFactory(
        const std::vector<double*>& unknowns_,
        const std::vector<Constraint*>& initialConstraints
    )
        : unknowns(unknowns_.begin(), unknowns_.end())
        , attempts(initialConstraints.size(), Attempt::Unknown)
    {
        bool done = false;
        size_t numConstraintsDone = 0;

        // First try substitutions on constraints which cannot
        // yield an ambiguous result to resolve other constraints
        // in less iterations in the second loop
        for (size_t i = 0; i < initialConstraints.size(); ++i) {
            auto constr = initialConstraints[i];

            // No substitution for temporary constraints,
            // non driving constraints do not help the solve
            if (constr->getTag() < 0 || !constr->isDriving()) {
                attempts[i] = Attempt::No;
                numConstraintsDone++;
                continue;
            }

            if (constr->getTypeId() == Equal) {
                attempts[i] = trySubstitute(static_cast<ConstraintEqual*>(constr));
            }
            else if (constr->getTypeId() == Difference) {
                attempts[i] = trySubstitute(static_cast<ConstraintDifference*>(constr));
            }
            else {
                continue;
            }

            if (attempts[i] != Attempt::Maybe) {
                numConstraintsDone++;
            }
        }
        // As new substitution & reductions are discovered, previously incompatible
        // constraints may appear to be substitutable so we try until no constraint
        // can be substituted
        while (!done && numConstraintsDone != initialConstraints.size()) {
            done = true;  // Done until proven false by having a successful substituttion
            for (size_t i = 0; i < initialConstraints.size(); ++i) {
                auto constr = initialConstraints[i];

                // No use trying again
                if (attempts[i] == Attempt::No || attempts[i] == Attempt::Yes) {
                    continue;
                }

                Attempt attempt = Attempt::No;
                switch (constr->getTypeId()) {
                    case P2PDistance:
                        attempt
                            = trySubstitute(static_cast<ConstraintP2PDistance*>(constr), attempts[i]);
                        break;
                    case PointOnLine:
                        attempt = trySubstitute(static_cast<ConstraintPointOnLine*>(constr));
                        break;
                    case Parallel:
                        attempt = trySubstitute(static_cast<ConstraintParallel*>(constr));
                        break;
                    case Perpendicular:
                        attempt = trySubstitute(static_cast<ConstraintPerpendicular*>(constr));
                        break;
                    case C2CDistance:
                        attempt = trySubstitute(static_cast<ConstraintC2CDistance*>(constr));
                        break;
                    case C2LDistance:
                        attempt = trySubstitute(static_cast<ConstraintC2LDistance*>(constr));
                        break;
                    case P2CDistance:
                        attempt = trySubstitute(static_cast<ConstraintP2CDistance*>(constr));
                        break;
                    case P2LDistance:
                        attempt = trySubstitute(static_cast<ConstraintP2LDistance*>(constr));
                        break;
                    case EqualLineLength:
                        attempt = trySubstitute(static_cast<ConstraintEqualLineLength*>(constr));
                        break;
                    case None:
                    case Equal:       // Already handled in first loop
                    case Difference:  // Already handled in first loop
                    case P2PAngle:
                    case PointOnPerpBisector:
                    case L2LAngle:  // Not handled, if the user specified angles such as 90deg or
                                    // 180deg, perhaps they are trying to prevent a flip, let's not
                                    // disturb that
                    case MidpointOnLine:
                    case TangentCircumf:
                    case PointOnEllipse:
                    case TangentEllipseLine:
                    case InternalAlignmentPoint2Ellipse:
                    case EqualMajorAxesConic:
                    case EllipticalArcRangeToEndPoints:
                    case AngleViaPoint:
                    case Snell:
                    case CurveValue:
                    case PointOnHyperbola:
                    case InternalAlignmentPoint2Hyperbola:
                    case PointOnParabola:
                    case EqualFocalDistance:
                    case CenterOfGravity:
                    case WeightedLinearCombination:
                    case SlopeAtBSplineKnot:
                    case PointOnBSpline:
                    case AngleViaPointAndParam:
                    case AngleViaPointAndTwoParams:
                    case AngleViaTwoPoints:
                    case ArcLength:
                        break;
                }
                attempts[i] = attempt;
                if (attempt == Attempt::Yes) {
                    done = false;
                    numConstraintsDone++;
                }
                else if (attempt == Attempt::No) {
                    numConstraintsDone++;
                }
            }
        }

        compile();
    }
    // Offset is assumed to be positive
    bool addRelationship(
        double* unknownA,
        double* unknownB,
        double offset,
        RelationshipOptions option = RelationshipOptions::Relative
    )
    {
        if (haveRelationship(unknownA, unknownB)) {
            std::cerr << "Unexpected! unknowns " << unknownA << " and " << unknownB
                      << " are already linked :(\n";
            return false;
        }
        if (option == RelationshipOptions::Relative && *unknownB > *unknownA) {
            offset = -offset;
        }

        adjacencyList[unknownA][unknownB] = offset;
        adjacencyList[unknownB][unknownA] = -offset;

        return true;
    }
    bool addEqual(double* unknownA, double* unknownB)
    {
        return addRelationship(unknownA, unknownB, 0.0);
    }
    void addConst(double* unknown, double value)
    {
        constUnknowns[unknown] = value;
    }
    // adds a directed difference between the unknown and the ref
    // so that *unknown > ref returns the same after solve
    // diff must be positive
    void addConstDifference(double* unknown, double ref, double diff)
    {
        if (*unknown > ref) {
            addConst(unknown, ref + diff);
        }
        else {
            addConst(unknown, ref - diff);
        }
    }
    std::optional<double> value(double* unknown) const
    {
        auto foundConst = constUnknowns.find(unknown);
        if (foundConst == constUnknowns.end()) {
            return std::nullopt;
        }
        return foundConst->second;
    }
    bool areEqual(double* unknownA, double* unknownB) const
    {
        const auto& foundA = adjacencyList.find(unknownA);
        if (foundA == adjacencyList.end()) {
            return areBothConstAndEqual(unknownA, unknownB);
        }

        const auto& foundB = foundA->second.find(unknownB);
        if (foundB == foundA->second.end()) {
            return areBothConstAndEqual(unknownA, unknownB);
        }
        return foundB->second == 0.0;
    }
    bool areBothConstAndEqual(double* unknownA, double* unknownB) const
    {
        std::optional<double> maybeDist = constDistance(unknownA, unknownB);

        return maybeDist.has_value() && *maybeDist < Precision::Confusion();
    }
    bool haveRelationship(double* unknownA, double* unknownB) const
    {
        const auto& foundA = adjacencyList.find(unknownA);
        if (foundA == adjacencyList.end()) {
            return false;
        }

        const auto& foundB = foundA->second.find(unknownB);
        return foundB != foundA->second.end();
    }
    std::optional<double> constDistance(double* unknownA, double* unknownB) const
    {
        std::optional<double> valueA = value(unknownA);
        std::optional<double> valueB = value(unknownB);

        if (valueA.has_value() && valueB.has_value()) {
            return std::abs(*valueA - *valueB);
        }
        return std::nullopt;
    }
    std::optional<double> distance(double* unknownA, double* unknownB) const
    {
        const auto& foundA = adjacencyList.find(unknownA);
        if (foundA == adjacencyList.end()) {
            return constDistance(unknownA, unknownB);
        }

        const auto& foundB = foundA->second.find(unknownB);
        if (foundB == foundA->second.end()) {
            return constDistance(unknownA, unknownB);
        }
        return std::abs(foundB->second);
    }
    std::optional<double> length(Line line)
    {
        auto foundLineLength = std::ranges::find_if(linesOfKnownLength, [&](const auto& lokl) {
            return (line.p1 == lokl.first.p1 && line.p2 == lokl.first.p2)
                || (line.p1 == lokl.first.p2 && line.p2 == lokl.first.p1);
        });
        if (foundLineLength != linesOfKnownLength.end()) {
            return foundLineLength->second;
        }
        std::optional<double> x1v = value(line.p1.x);
        std::optional<double> y1v = value(line.p1.y);
        std::optional<double> x2v = value(line.p2.x);
        std::optional<double> y2v = value(line.p2.y);

        if (x1v.has_value() && y1v.has_value() && x2v.has_value() && y2v.has_value()) {
            double xl = *x1v - *x2v;
            double yl = *y1v - *y2v;
            double length = std::sqrt((xl * xl) + (yl * yl));
            linesOfKnownLength.emplace_back(line, length);
            return length;
        }

        return std::nullopt;
    }
    bool hasARelationship(double* unknown) const
    {
        const auto& foundAdj = adjacencyList.find(unknown);
        const auto& foundConst = constUnknowns.find(unknown);
        return foundAdj != adjacencyList.end() || foundConst != constUnknowns.end();
    }
    LineDesc lineDescription(const Line& line) const
    {
        // Is it possible to have some but not all parameters not unknown??
        if (!unknowns.contains(line.p1.x) || !unknowns.contains(line.p1.y)
            || !unknowns.contains(line.p2.x) || !unknowns.contains(line.p2.y)) {

            if (std::abs(*line.p1.x - *line.p2.x) < Precision::Confusion()) {
                return LineDesc {.isExternal = true, .orientation = Orientation::Vertical};
            }
            if (std::abs(*line.p1.y - *line.p2.y) < Precision::Confusion()) {
                return LineDesc {.isExternal = true, .orientation = Orientation::Horizontal};
            }
            return LineDesc {.isExternal = true};
        }

        if (areEqual(line.p1.x, line.p2.x)) {
            return LineDesc {.orientation = Orientation::Vertical};
        }
        if (areEqual(line.p1.y, line.p2.y)) {
            return LineDesc {.orientation = Orientation::Horizontal};
        }
        return LineDesc {};
    }
    bool isExternal(const Point& point) const
    {
        return !unknowns.contains(point.x) || !unknowns.contains(point.y);
    }
    bool isExternal(const Circle& circle) const
    {
        return isExternal(circle.center) || !unknowns.contains(circle.rad);
    }
    void compile()
    {
        std::unordered_set<double*> visited;

        for (auto constUnknown : constUnknowns) {
            if (visited.contains(constUnknown.first)) {
                std::cerr << "Unexpected! const unknown " << constUnknown.first
                          << " is already visited :(\n";
            }
            std::vector<std::pair<double*, double>> comp = component(constUnknown.first, visited);
            comp.push_back(std::pair<double*, double>(constUnknown.first, 0.0));

            // Sort offsets so that unknowns with the same const value are contiguous
            std::ranges::sort(comp, {}, &std::pair<double*, double>::second);

            makeConstantConstraints(comp, constUnknown.second);
        }

        for (const auto& unknownVert : adjacencyList) {
            if (!visited.contains(unknownVert.first)) {
                std::vector<std::pair<double*, double>> comp = component(unknownVert.first, visited);

                // Sort offsets so that unknowns with the same offset value are contiguous
                std::ranges::sort(comp, {}, &std::pair<double*, double>::second);

                makeDifferenceConstraints(comp, unknownVert.first);
            }
        }
    }

    void makeDifferenceConstraints(const std::vector<std::pair<double*, double>>& comp, double* baseUnkown)
    {
        if (comp.empty()) {
            return;
        }

        double currentOffset = comp[0].second;
        double* currentReducer = nullptr;

        if (currentOffset != 0.0) {
            currentReducer = comp[0].first;
            // Set the difference to nullptr for now because the constants vector is
            // still growing so references to it's element are not stable
            differenceConstraints.push_back(
                new ConstraintDifference(comp[0].first, baseUnkown, nullptr)
            );
            differences.push_back(-currentOffset);
        }
        else {
            currentReducer = baseUnkown;
        }
        reductionList[currentReducer] = {};

        for (size_t i = 1; i < comp.size(); ++i) {
            if (comp[i].second != currentOffset) {
                currentOffset = comp[i].second;

                if (currentOffset != 0.0) {
                    currentReducer = comp[i].first;
                    // Set the difference to nullptr for now because the constants vector is
                    // still growing so references to it's element are not stable
                    differenceConstraints.push_back(
                        new ConstraintDifference(comp[i].first, baseUnkown, nullptr)
                    );
                    differences.push_back(-currentOffset);
                }
                else {
                    currentReducer = baseUnkown;
                }
                reductionList[currentReducer] = {};
            }
            else {
                reductionList[currentReducer].push_back(comp[i].first);
            }
        }
    }

    void makeConstantConstraints(const std::vector<std::pair<double*, double>>& comp, double baseValue)
    {
        if (comp.empty()) {
            return;
        }

        double currentOffset = comp[0].second;
        double* currentReducer = comp[0].first;

        // Set the equality to nullptr for now because the constants vector is
        // still growing so references to it's element are not stable
        constantConstraints.push_back(new ConstraintEqual(currentReducer, nullptr));
        constants.push_back(baseValue + currentOffset);

        for (size_t i = 1; i < comp.size(); ++i) {
            if (comp[i].second != currentOffset) {
                currentReducer = comp[i].first;
                currentOffset = comp[i].second;

                constantConstraints.push_back(new ConstraintEqual(comp[i].first, nullptr));
                constants.push_back(baseValue + currentOffset);
                reductionList[currentReducer] = {};
            }
            else {
                reductionList[currentReducer].push_back(comp[i].first);
            }
        }
    }

    std::vector<std::pair<double*, double>> component(double* root, std::unordered_set<double*>& visited)
    {
        std::vector<std::pair<double*, double>> comp;
        std::queue<std::pair<double*, double>> toExplore;
        toExplore.push(std::make_pair(root, 0.0));
        visited.insert(root);

        while (!toExplore.empty()) {
            comp.push_back(toExplore.front());
            findNeighbours(toExplore, visited);
        }
        return comp;
    }
    void findNeighbours(
        std::queue<std::pair<double*, double>>& toExplore,
        std::unordered_set<double*>& visited
    )
    {
        std::pair<double*, double> search = toExplore.front();
        toExplore.pop();
        auto foundAdj = adjacencyList.find(search.first);
        if (foundAdj == adjacencyList.end()) {
            return;
        }

        for (auto neighbour : foundAdj->second) {
            if (!visited.contains(neighbour.first)) {
                visited.insert(neighbour.first);
                toExplore.push(std::make_pair(neighbour.first, -neighbour.second + search.second));
            }
        }
    }

    Attempt trySubstitute(ConstraintEqual* constr)
    {
        double* p1 = constr->origParams()[0];
        double* p2 = constr->origParams()[1];

        bool foundP1 = unknowns.contains(p1);
        bool foundP2 = unknowns.contains(p2);

        if (!foundP1 && foundP2) {
            addConst(p2, *p1 * constr->ratio);
            return Attempt::Yes;
        }
        if (foundP1 && !foundP2) {
            addConst(p1, *p2 * constr->ratio);
            return Attempt::Yes;
        }

        if (!foundP1 && !foundP2) {
            return Attempt::No;
        }

        if (constr->ratio == 1.0 && addEqual(p1, p2)) {
            return Attempt::Yes;
        }
        return Attempt::No;
    }
    Attempt trySubstitute(ConstraintPerpendicular* constr)
    {
        Line line1(
            Point(constr->origParams()[0], constr->origParams()[1]),
            Point(constr->origParams()[2], constr->origParams()[3])
        );
        Line line2(
            Point(constr->origParams()[4], constr->origParams()[5]),
            Point(constr->origParams()[6], constr->origParams()[7])
        );

        LineDesc line1Desc = lineDescription(line1);
        LineDesc line2Desc = lineDescription(line2);

        if (line1Desc.isExternal && line2Desc.isExternal) {
            return Attempt::No;
        }

        if (line1Desc.orientation != Orientation::None && line2Desc.orientation != Orientation::None) {
            return Attempt::No;
        }

        if (line2Desc.isExternal) {
            std::swap(line1, line2);
            std::swap(line1Desc, line2Desc);
        }

        if (line1Desc.isExternal) {
            if (line1Desc.orientation == Orientation::Vertical) {
                addEqual(line2.p1.y, line2.p2.y);  // Make line2 horizontal
                return Attempt::Yes;
            }
            if (line2Desc.orientation == Orientation::Horizontal) {
                addEqual(line2.p1.x, line2.p2.x);  // Make line2 vertical
                return Attempt::Yes;
            }
            return Attempt::No;  // Line1 is external, so will never become horizontal/vertical
        }
        if (line1Desc.orientation == Orientation::Vertical) {
            addEqual(line2.p1.y, line2.p2.y);  // Make line2 horizontal
            return Attempt::Yes;
        }
        if (line1Desc.orientation == Orientation::Horizontal) {
            addEqual(line2.p1.x, line2.p2.x);  // Make line2 vertical
            return Attempt::Yes;
        }
        if (line2Desc.orientation == Orientation::Vertical) {
            addEqual(line1.p1.y, line1.p2.y);  // Make line1 horizontal
            return Attempt::Yes;
        }
        if (line2Desc.orientation == Orientation::Horizontal) {
            addEqual(line1.p1.x, line1.p2.x);  // Make line1 vertical
            return Attempt::Yes;
        }
        return Attempt::Maybe;
    }
    Attempt trySubstitute(ConstraintParallel* constr)
    {
        Line line1(
            Point(constr->origParams()[0], constr->origParams()[1]),
            Point(constr->origParams()[2], constr->origParams()[3])
        );
        Line line2(
            Point(constr->origParams()[4], constr->origParams()[5]),
            Point(constr->origParams()[6], constr->origParams()[7])
        );

        LineDesc line1Desc = lineDescription(line1);
        LineDesc line2Desc = lineDescription(line2);

        if (line1Desc.isExternal && line2Desc.isExternal) {
            return Attempt::No;
        }

        if (line1Desc.orientation != Orientation::None && line2Desc.orientation != Orientation::None) {
            return Attempt::No;
        }

        if (line2Desc.isExternal) {
            std::swap(line1, line2);
            std::swap(line1Desc, line2Desc);
        }

        if (line1Desc.isExternal) {
            if (line1Desc.orientation == Orientation::Vertical) {
                addEqual(line2.p1.x, line2.p2.x);  // Make line2 vertical
                return Attempt::Yes;
            }
            if (line2Desc.orientation == Orientation::Horizontal) {
                addEqual(line2.p1.y, line2.p2.y);  // Make line2 horizontal
                return Attempt::Yes;
            }
            return Attempt::No;  // Line1 is external, so will never become horizontal/vertical
        }

        if (line1Desc.orientation == Orientation::Vertical) {
            addEqual(line2.p1.x, line2.p2.x);  // Make line2 vertical
            return Attempt::Yes;
        }
        if (line1Desc.orientation == Orientation::Horizontal) {
            addEqual(line2.p1.y, line2.p2.y);  // Make line2 horizontal
            return Attempt::Yes;
        }
        if (line2Desc.orientation == Orientation::Vertical) {
            addEqual(line1.p1.x, line1.p2.x);  // Make line1 vertical
            return Attempt::Yes;
        }
        if (line2Desc.orientation == Orientation::Horizontal) {
            addEqual(line1.p1.y, line1.p2.y);  // Make line1 horizontal
            return Attempt::Yes;
        }
        return Attempt::Maybe;
    }

    Attempt p2pDistHelper(
        const Point& p1,
        const Point& p2,
        double dist,
        Orientation orientation,
        bool p1_external
    )
    {
        if (p1_external) {
            if (orientation == Orientation::Vertical) {
                addConstDifference(p2.y, *p1.y, dist);
                return Attempt::Yes;
            }
            if (orientation == Orientation::Horizontal) {
                addConstDifference(p2.x, *p1.x, dist);
                return Attempt::Yes;
            }
            return Attempt::Maybe;
        }

        // both p1 and p2 are sketcher geometries
        if (orientation == Orientation::Vertical) {
            addRelationship(p1.y, p2.y, dist);
            return Attempt::Yes;
        }
        if (orientation == Orientation::Horizontal) {
            addRelationship(p1.x, p2.x, dist);
            return Attempt::Yes;
        }
        return Attempt::Maybe;
    }
    Attempt trySubstitute(ConstraintP2PDistance* constr, Attempt previousAttempt)
    {
        Line line(
            Point(constr->origParams()[0], constr->origParams()[1]),
            Point(constr->origParams()[2], constr->origParams()[3])
        );
        double* dist = constr->origParams()[4];

        // The distance is not fixed, solved for even
        if (unknowns.contains(dist)) {
            return Attempt::No;
        }

        if (previousAttempt == Attempt::Unknown) {
            linesOfKnownLength.emplace_back(line, *dist);
        }

        LineDesc lineDesc = lineDescription(line);
        if (lineDesc.isExternal) {
            return Attempt::No;
        }

        return p2pDistHelper(line.p1, line.p2, *dist, lineDesc.orientation, false);
    }

    Attempt p2lDistanceHelper(const Point& point, const Line& line, double dist)
    {
        LineDesc lineDesc = lineDescription(line);
        bool pointExternal = isExternal(point);

        // Invalid constraint, between 2 external geometries
        if (lineDesc.isExternal && pointExternal) {
            return Attempt::No;
        }

        if (lineDesc.orientation == Orientation::None) {
            return lineDesc.isExternal ? Attempt::No : Attempt::Maybe;
        }

        Point p1 = point;
        Point p2 = line.p1;

        if (lineDesc.isExternal) {
            std::swap(p1, p2);
            std::swap(lineDesc.isExternal, pointExternal);
        }

        // We swap the orientation because if the line is horizontal,
        // the the distance with the point will be defined on the y axis
        // and conversly
        return p2pDistHelper(
            p1,
            p2,
            dist,
            lineDesc.orientation == Orientation::Horizontal ? Orientation::Vertical
                                                            : Orientation::Horizontal,
            pointExternal
        );
    }
    Attempt trySubstitute(ConstraintP2LDistance* constr)
    {
        Point point(constr->origParams()[0], constr->origParams()[1]);
        Line line(
            Point(constr->origParams()[2], constr->origParams()[3]),
            Point(constr->origParams()[4], constr->origParams()[5])
        );

        double* dist = constr->origParams()[6];

        // The distance is not fixed, solved for even
        if (unknowns.contains(dist)) {
            return Attempt::No;
        }

        return p2lDistanceHelper(point, line, *dist);
    }
    Attempt trySubstitute(ConstraintPointOnLine* constr)
    {
        Point point(constr->origParams()[0], constr->origParams()[1]);
        Line line(
            Point(constr->origParams()[2], constr->origParams()[3]),
            Point(constr->origParams()[4], constr->origParams()[5])
        );

        return p2lDistanceHelper(point, line, 0.0);
    }
    std::optional<double> circleDistanceToCenterDistance(
        double centerPos,
        double refPos,
        double dist,
        double radius
    )
    {
        if (std::abs(centerPos - refPos) < radius) {
            if (dist > radius) {
                return std::nullopt;  // Invalid!
            }
            return std::abs(radius - dist);
        }
        return radius + dist;
    }
    Attempt p2cDistanceHelper(
        const Point& point,
        const Circle& circle,
        double dist,
        Orientation alignment,
        bool pointExternal
    )
    {
        std::optional<double> rad = std::nullopt;
        if (!pointExternal) {
            rad = value(circle.rad);
            if (!rad.has_value()) {
                return Attempt::No;
            }
        }
        else {
            rad = *circle.rad;
        }

        if (pointExternal) {
            if (alignment == Orientation::Vertical) {
                std::optional<double> maybeDist
                    = circleDistanceToCenterDistance(*circle.center.y, *point.y, dist, *circle.rad);
                if (!maybeDist.has_value()) {
                    return Attempt::No;  // Not dealing with invalid constraint
                }

                addConstDifference(circle.center.y, *point.y, *maybeDist);
                return Attempt::Yes;
            }
            if (alignment == Orientation::Horizontal) {
                std::optional<double> maybeDist
                    = circleDistanceToCenterDistance(*circle.center.x, *point.x, dist, *circle.rad);

                if (!maybeDist.has_value()) {
                    return Attempt::No;  // Not dealing with invalid constraint
                }
                addConstDifference(circle.center.x, *point.x, *maybeDist);
                return Attempt::Yes;
            }
            return Attempt::Maybe;
        }

        if (alignment == Orientation::Vertical) {
            std::optional<double> maybeDist
                = circleDistanceToCenterDistance(*circle.center.y, *point.y, dist, *circle.rad);
            if (!maybeDist.has_value()) {
                return Attempt::No;
            }
            addRelationship(circle.center.y, point.y, *maybeDist);
            return Attempt::Yes;
        }
        if (alignment == Orientation::Vertical) {
            std::optional<double> maybeDist
                = circleDistanceToCenterDistance(*circle.center.y, *point.y, dist, *circle.rad);
            if (!maybeDist.has_value()) {
                return Attempt::No;
            }
            addRelationship(circle.center.y, point.y, *maybeDist);
            return Attempt::Yes;
        }
        return Attempt::Maybe;
    }
    Attempt trySubstitute(ConstraintP2CDistance* constr)
    {
        double* dist = constr->distance();
        Circle circle = constr->circle;
        Point point = constr->pt;

        bool circleExternal = isExternal(circle);
        bool pointExternal = isExternal(point);

        if (circleExternal && pointExternal) {
            return Attempt::No;
        }

        LineDesc relPos = lineDescription(Line(circle.center, point));
        if (relPos.orientation == Orientation::None) {
            return Attempt::Maybe;
        }


        std::optional<double> rad = std::nullopt;
        if (!circleExternal) {
            rad = value(circle.rad);
            if (!rad.has_value()) {
                return Attempt::No;
            }
        }
        else {
            rad = *circle.rad;
        }
        if (pointExternal) {
            std::swap(circle.center, point);
            std::swap(pointExternal, circleExternal);
        }

        return p2cDistanceHelper(point, circle, *dist, relPos.orientation, pointExternal);
    }
    Attempt trySubstitute(ConstraintC2LDistance* constr)
    {
        double* dist = constr->origParams()[0];
        Circle circle = constr->circle;
        Line line = constr->line;

        LineDesc lineDesc = lineDescription(line);
        bool circleExternal = isExternal(circle);

        if (lineDesc.isExternal && circleExternal) {
            return Attempt::No;
        }

        std::optional<double> rad = std::nullopt;
        if (!circleExternal) {
            rad = value(circle.rad);
            if (!rad.has_value()) {
                return Attempt::No;
            }
        }
        else {
            rad = *circle.rad;
        }

        Point point = line.p1;
        if (lineDesc.isExternal) {
            std::swap(circleExternal, lineDesc.isExternal);
            std::swap(point, circle.center);
        }

        // We swap the orientation because if the line is horizontal,
        // the the distance with the point will be defined on the y axis
        // and conversly
        return p2cDistanceHelper(
            point,
            circle,
            *dist,
            lineDesc.orientation == Orientation::Horizontal ? Orientation::Vertical
                                                            : Orientation::Horizontal,
            lineDesc.isExternal
        );
    }
    Attempt trySubstitute(ConstraintC2CDistance* constr)
    {
        double* dist = constr->distance();
        Circle c1 = constr->c1;
        Circle c2 = constr->c2;

        bool c1External = isExternal(c1);
        bool c2External = isExternal(c2);

        if (c1External && c2External) {
            return Attempt::No;
        }

        LineDesc relPos = lineDescription(Line(c1.center, c2.center));

        // No substitution can be made yet
        if (relPos.orientation == Orientation::None) {
            return Attempt::Maybe;
        }

        std::optional<double> c1Rad = std::nullopt;
        std::optional<double> c2Rad = std::nullopt;
        if (!c1External) {
            c1Rad = value(c1.rad);
            if (!c1Rad.has_value()) {
                return Attempt::No;
            }
        }
        else {
            c1Rad = *c1.rad;
        }
        if (!c2External) {
            c2Rad = value(c2.rad);
            if (!c2Rad.has_value()) {
                return Attempt::No;
            }
        }
        else {
            c2Rad = *c2.rad;
        }
        if (c2External) {
            std::swap(c1, c2);
            std::swap(c1Rad, c2Rad);
            std::swap(c1External, c2External);
        }

        // Invalid constraint, not dealing with this
        if (-*dist > *c1Rad || -*dist > *c2Rad) {
            return Attempt::No;
        }

        double centerDist = *dist + *c1Rad + *c2Rad;
        return p2pDistHelper(c1.center, c2.center, centerDist, relPos.orientation, c1External);
    }

    Attempt trySubstitute(ConstraintEqualLineLength* constr)
    {
        Line line1 = constr->l1;
        Line line2 = constr->l2;

        LineDesc line1Desc = lineDescription(line1);
        LineDesc line2Desc = lineDescription(line2);

        std::optional<double> l1length = std::nullopt;
        std::optional<double> l2length = std::nullopt;

        if (line1Desc.isExternal) {
            l1length = std::sqrt(
                std::pow(*line1.p1.x - *line1.p2.x, 2.0) + std::pow(*line1.p1.y - *line1.p2.y, 2.0)
            );  // This is a constant line, we can just measure it's length
        }
        else if (line1Desc.orientation == Orientation::Vertical) {
            l1length = distance(line1.p1.y, line1.p2.y);  // May return nullopt!
        }
        else if (line1Desc.orientation == Orientation::Horizontal) {
            l1length = distance(line1.p1.x, line1.p2.x);  // May return nullopt!
        }
        else {
            l1length = length(line1);  // Search in our little database if we have seen it before,
                                       // may return nullopt
        }

        if (line2Desc.isExternal) {
            l2length = std::sqrt(
                std::pow(*line2.p1.x - *line2.p2.x, 2.0) + std::pow(*line2.p1.y - *line2.p2.y, 2.0)
            );  // This is a constant line, we can just measure it's length
        }
        else if (line2Desc.orientation == Orientation::Vertical) {
            l2length = distance(line2.p1.y, line2.p2.y);  // May return nullopt!
        }
        else if (line2Desc.orientation == Orientation::Horizontal) {
            l2length = distance(line2.p1.x, line2.p2.x);  // May return nullopt!
        }
        else {
            l2length = length(line2);  // Search in our little database if we have seen it before,
                                       // may return nullopt
        }

        if (l1length.has_value() && l2length.has_value()) {
            return Attempt::No;  // Hopefully they are the same but we are not getting into this
        }

        if (l2length.has_value()) {
            std::swap(line1, line2);
            std::swap(line1Desc, line2Desc);
            std::swap(l1length, l2length);
        }

        if (l1length.has_value()) {
            linesOfKnownLength.emplace_back(line2, *l1length);

            // At this point we know l2 is not external
            if (line2Desc.orientation == Orientation::Vertical) {
                addRelationship(line2.p1.y, line2.p2.y, *l1length);
                return Attempt::Yes;
            }
            if (line2Desc.orientation == Orientation::Horizontal) {
                addRelationship(line2.p1.x, line2.p2.x, *l1length);
                return Attempt::Yes;
            }
            return Attempt::Maybe;
        }
        return Attempt::Maybe;
    }
    Attempt trySubstitute(ConstraintDifference* constr)
    {
        double* p1 = constr->origParams()[0];
        double* p2 = constr->origParams()[1];
        double* diff = constr->origParams()[2];

        bool p1External = !unknowns.contains(p1);
        bool p2External = !unknowns.contains(p2);

        if ((p1External && p2External) || unknowns.contains(diff)) {
            return Attempt::No;
        }

        // Using absolute because the constraint was created with an order
        // in mind
        // however this is a bit confusing because relationships in substitutions
        // are defined from left to rigth such that p1 = p2 + diff
        // but in the constraint it is defined such that p2 = p1 + diff
        // so we have to negate the difference
        if (p1External) {
            addConst(p2, *p1 + *diff);
        }
        else if (p2External) {
            addConst(p1, *p2 - *diff);
        }
        else {
            addRelationship(p1, p2, -*diff, RelationshipOptions::Absolute);
        }
        return Attempt::Yes;
    }
};


Substitution::Substitution(
    const VEC_pD& initialUnknowns,
    const std::vector<Constraint*>& initialConstraints
)
{
    SubstitutionFactory factory(initialUnknowns, initialConstraints);

    // Build the parameters vector, the size must not change after it is built
    // so that pointers to it's elements are still valid and can be used by the solver
    std::vector<double*> untouchedUnknowns;
    for (double* unknown : initialUnknowns) {
        // If the unknown has not even been touched by the substitution
        // factory, then surely it needs to be solved
        if (!factory.hasARelationship(unknown)) {
            untouchedUnknowns.push_back(unknown);
        }
    }

    // Resize now because, some pointers will refer to elements of this vector
    // when creating the reduction map next
    size_t parameterIndex = 0;
    parameters.resize(untouchedUnknowns.size() + factory.reductionList.size());
    unknowns.resize(parameters.size());

    for (double* unknown : untouchedUnknowns) {
        unknowns[parameterIndex] = unknown;
        parameters[parameterIndex] = *unknown;
        reductionMap[unknown] = &parameters[parameterIndex];
        parameterIndex++;
    }
    for (const auto& reduction : factory.reductionList) {
        // A single unkown is promoted to parameter for every reduction group
        unknowns[parameterIndex] = reduction.first;
        parameters[parameterIndex] = *reduction.first;
        reductionMap[reduction.first] = &parameters[parameterIndex];
        for (double* reduced : reduction.second) {
            reductionMap[reduced] = &parameters[parameterIndex];
        }
        parameterIndex++;
    }

    // Put all constraints which were not reduced in the constraints vector
    for (size_t i = 0; i < initialConstraints.size(); ++i) {
        if (factory.attempts[i] != SubstitutionFactory::Attempt::Yes
            && initialConstraints[i]->redirectParams(reductionMap)) {
            constraints.push_back(initialConstraints[i]);
        }
    }
    // Add all the constraints we created by substitution
    constantConstraints = std::move(factory.constantConstraints);
    constants = std::move(factory.constants);
    differenceConstraints = std::move(factory.differenceConstraints);
    differences = std::move(factory.differences);
    for (size_t i = 0; i < constantConstraints.size(); ++i) {
        constantConstraints[i]->setOrigParam(1, &constants[i]);
        constantConstraints[i]->redirectParams(reductionMap);
        constraints.push_back(constantConstraints[i]);
    }
    for (size_t i = 0; i < differenceConstraints.size(); ++i) {
        differenceConstraints[i]->setOrigParam(2, &differences[i]);
        differenceConstraints[i]->redirectParams(reductionMap);
        constraints.push_back(differenceConstraints[i]);
    }
}
Substitution Substitution::makeTrivial(
    const VEC_pD& initialUnknowns,
    const std::vector<Constraint*>& initialConstraints
)
{
    Substitution dst;
    dst.unknowns = initialUnknowns;
    dst.constraints = initialConstraints;
    dst.parameters.resize(initialUnknowns.size());

    for (size_t i = 0; i < initialUnknowns.size(); ++i) {
        dst.parameters[i] = *initialUnknowns[i];
        dst.reductionMap[initialUnknowns[i]] = &dst.parameters[i];
    }
    for (auto constr : dst.constraints) {
        constr->redirectParams(dst.reductionMap);
    }
    return dst;
}
Substitution::Substitution(Substitution&& other) noexcept
    : constraints(std::move(other.constraints))
    , parameters(std::move(other.parameters))
    , unknowns(std::move(other.unknowns))
    , reductionMap(std::move(other.reductionMap))
    , constantConstraints(std::move(other.constantConstraints))
    , constants(std::move(other.constants))
    , differenceConstraints(std::move(other.differenceConstraints))
    , differences(std::move(other.differences))
{}
Substitution& Substitution::operator=(Substitution&& other) noexcept
{
    constraints = std::move(other.constraints);
    parameters = std::move(other.parameters);
    unknowns = std::move(other.unknowns);

    reductionMap = std::move(other.reductionMap);

    constantConstraints = std::move(other.constantConstraints);
    constants = std::move(other.constants);

    differenceConstraints = std::move(other.differenceConstraints);
    differences = std::move(other.differences);

    return *this;
}
Substitution::~Substitution()
{
    for (auto constConstr : constantConstraints) {
        delete constConstr;
    }
    for (auto diffConstr : differenceConstraints) {
        delete diffConstr;
    }
}

void Substitution::initParams()
{
    for (size_t i = 0; i < unknowns.size(); ++i) {
        parameters[i] = *unknowns[i];
    }
}

}  // namespace GCS
