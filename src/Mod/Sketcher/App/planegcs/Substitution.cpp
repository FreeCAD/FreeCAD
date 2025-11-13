
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

#include <vector>
#include <iostream>
#include <queue>
#include <optional>

#include <Precision.hxx>

namespace GCS
{

struct SubstitutionFactory
{
    std::unordered_map<double*, std::unordered_map<double*, double>> adjacencyList;

    std::unordered_map<double*, double> constUnknowns;

    // Each unknown that will be kept as a parameter to a list of unknowns that are equal,
    // will be used to build the reduction map which will go the otherway around
    std::unordered_map<double*, std::vector<double*>> unknownsReductionList;

    // Updaters that are are in function of unknowns,
    // will be used to build the updaters in function of parameters
    std::vector<SubstitutionUpdater> constSubstUpdaters;
    std::vector<SubstitutionUpdater> substUpdaters;


    enum class RelationshipOptions
    {
        Relative,  // [default] Adds a relationship of the form A = B + offset iif the initial
                   // condition are B <= A and A = B - offset otherwise
        Absolute   // Adds a relationship of the form A = B + offset
    };
    // Offset is assumed to be positive
    bool addRelationship(double* unknownA,
                         double* unknownB,
                         double offset,
                         RelationshipOptions option = RelationshipOptions::Relative)
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
    bool isVertical(const Line& line) const
    {
        return areEqual(line.p1.x, line.p2.x);
    }
    bool isHorizontal(const Line& line) const
    {
        return areEqual(line.p1.y, line.p2.y);
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
        if (foundB == foundA->second.end()) {
            return false;
        }
        return true;
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
    bool hasARelationship(double* unknown) const
    {
        const auto& foundAdj = adjacencyList.find(unknown);
        const auto& foundConst = constUnknowns.find(unknown);
        if (foundAdj == adjacencyList.end() && foundConst == constUnknowns.end()) {
            return false;
        }

        return true;
    }
    void compile()
    {
        std::unordered_set<double*> visited;

        for (auto constUnknown : constUnknowns) {
            if (visited.count(constUnknown.first)) {
                std::cerr << "Unexpected! const unknown " << constUnknown.first
                          << " is already visited :(\n";
            }
            constSubstUpdaters.push_back(SubstitutionUpdater {.follower = constUnknown.first,
                                                              .offset = constUnknown.second});
            unknownsReductionList[constUnknown.first] = {};
            makeComponent(constUnknown.first, visited, constSubstUpdaters);
        }
        for (const auto& unknownVert : adjacencyList) {
            if (!visited.count(unknownVert.first)) {
                makeComponent(unknownVert.first, visited, substUpdaters);
            }
        }
    }
    void makeComponent(double* root,
                       std::unordered_set<double*>& visited,
                       std::vector<SubstitutionUpdater>& updaterBucket)
    {
        std::vector<std::pair<double*, double>> rootOffsets;
        std::queue<std::pair<double*, double>> toExplore;
        toExplore.push(std::make_pair(root, 0.0));
        visited.insert(root);

        while (!toExplore.empty()) {
            rootOffsets.push_back(toExplore.front());
            findNeighbours(toExplore, visited);
        }

        std::ranges::sort(rootOffsets, {}, &std::pair<double*, double>::second);
        double* currentSubst = root;
        double currentOffset = rootOffsets[0].second;
        if (rootOffsets[0].first != root) {
            updaterBucket.push_back(SubstitutionUpdater {.root = root,
                                                         .follower = rootOffsets[0].first,
                                                         .offset = rootOffsets[0].second});
            currentSubst = rootOffsets[0].first;
            unknownsReductionList[currentSubst] = {};
        }

        for (size_t i = 1; i < rootOffsets.size(); ++i) {
            auto unknownAndOffset = rootOffsets[i];
            if (unknownAndOffset.second != currentOffset) {
                if (unknownAndOffset.first != root) {
                    updaterBucket.push_back(
                        SubstitutionUpdater {.root = root,
                                             .follower = unknownAndOffset.first,
                                             .offset = unknownAndOffset.second});
                    currentSubst = unknownAndOffset.first;
                }
                else {
                    currentSubst = root;
                }
                unknownsReductionList[currentSubst] = {};
            }
            else {
                unknownsReductionList[currentSubst].push_back(unknownAndOffset.first);
            }
        }
    }
    void findNeighbours(std::queue<std::pair<double*, double>>& toExplore,
                        std::unordered_set<double*>& visited)
    {
        std::pair<double*, double> search = toExplore.front();
        toExplore.pop();
        auto foundAdj = adjacencyList.find(search.first);
        if (foundAdj == adjacencyList.end()) {
            return;
        }
        if (toExplore.size() > 500) {
            std::cerr << "Waht\n";
        }
        for (auto neighbour : foundAdj->second) {
            if (!visited.count(neighbour.first)) {
                visited.insert(neighbour.first);
                toExplore.push(std::make_pair(neighbour.first, -neighbour.second + search.second));
            }
        }
    }
};


enum class ConstraintSubstitutionAttempt
{
    Yes,     // Substitution is possible
    No,      // Substitution is not possible
    Maybe,   // Substitution could be possible if some parameters are right
    Unknown  // Not checked yet
};

ConstraintSubstitutionAttempt
substitutionForEqual(Constraint* constr, const USET_pD& unknowns, SubstitutionFactory& factory)
{
    double* p1 = constr->origParams()[0];
    double* p2 = constr->origParams()[1];

    bool foundP1 = unknowns.count(p1);
    bool foundP2 = unknowns.count(p2);

    if (!foundP1 && foundP2) {
        factory.addConst(p2, *p1);
        return ConstraintSubstitutionAttempt::Yes;
    }
    if (foundP1 && !foundP2) {
        factory.addConst(p1, *p2);
        return ConstraintSubstitutionAttempt::Yes;
    }

    if (!foundP1 && !foundP2) {
        return ConstraintSubstitutionAttempt::No;
    }

    if (factory.addEqual(p1, p2)) {
        return ConstraintSubstitutionAttempt::Yes;
    }
    return ConstraintSubstitutionAttempt::No;
}
ConstraintSubstitutionAttempt substitutionForPerpendicular(Constraint* constr,
                                                           const USET_pD& unknowns,
                                                           SubstitutionFactory& factory)
{
    double* l1x1 = constr->origParams()[0];
    double* l1y1 = constr->origParams()[1];
    double* l1x2 = constr->origParams()[2];
    double* l1y2 = constr->origParams()[3];
    double* l2x1 = constr->origParams()[4];
    double* l2y1 = constr->origParams()[5];
    double* l2x2 = constr->origParams()[6];
    double* l2y2 = constr->origParams()[7];

    if (!unknowns.count(l1x1) || !unknowns.count(l1y1) || !unknowns.count(l1x2)
        || !unknowns.count(l1y2) || !unknowns.count(l2x1) || !unknowns.count(l2y1)
        || !unknowns.count(l2x2) || !unknowns.count(l2y2)) {
        return ConstraintSubstitutionAttempt::No;
    }

    // First line is vertical, second line should be horizontal
    if (factory.areEqual(l1x1, l1x2)) {
        factory.addEqual(l2y1, l2y2);
        return ConstraintSubstitutionAttempt::Yes;
    }

    // First line is horizontal, second line should be vertical
    if (factory.areEqual(l1y1, l1y2)) {
        factory.addEqual(l2x1, l2x2);
        return ConstraintSubstitutionAttempt::Yes;
    }

    // Second line is vertical, first line should be horizontal
    if (factory.areEqual(l2x1, l2x2)) {
        factory.addEqual(l1y1, l1y2);
        return ConstraintSubstitutionAttempt::Yes;
    }

    // Second line is horizontal, fist line should be vertical
    if (factory.areEqual(l2y1, l2y2)) {
        factory.addEqual(l1x1, l1x2);
        return ConstraintSubstitutionAttempt::Yes;
    }
    return ConstraintSubstitutionAttempt::Maybe;
}
ConstraintSubstitutionAttempt substitutionForPointOnLine(Constraint* constr,
                                                         const USET_pD& unknowns,
                                                         SubstitutionFactory& factory)
{
    double* p0x = constr->origParams()[0];
    double* p0y = constr->origParams()[1];
    double* lp1x = constr->origParams()[2];
    double* lp1y = constr->origParams()[3];
    double* lp2x = constr->origParams()[4];
    double* lp2y = constr->origParams()[5];

    if (!unknowns.count(p0x) || !unknowns.count(p0y) || !unknowns.count(lp1y)
        || !unknowns.count(lp1x) || !unknowns.count(lp2x) || !unknowns.count(lp2y)) {
        return ConstraintSubstitutionAttempt::No;
    }

    // Line is vertical, constraint can be translated to a vertical constraint
    if (factory.areEqual(lp1x, lp2x)) {
        factory.addEqual(lp1x, p0x);
        return ConstraintSubstitutionAttempt::Yes;
    }

    // Line is horizontal, constraint can be translated to a horizontal constraint
    if (factory.areEqual(lp1y, lp2y)) {
        factory.addEqual(lp1y, p0y);
        return ConstraintSubstitutionAttempt::Yes;
    }
    return ConstraintSubstitutionAttempt::Maybe;
}
ConstraintSubstitutionAttempt
substitutionForP2PDist(Constraint* constr, const USET_pD& unknowns, SubstitutionFactory& factory)
{
    double* lp1x = constr->origParams()[0];
    double* lp1y = constr->origParams()[1];
    double* lp2x = constr->origParams()[2];
    double* lp2y = constr->origParams()[3];
    double* dist = constr->origParams()[4];

    if (!unknowns.count(lp1x) || !unknowns.count(lp1y) || !unknowns.count(lp2x)
        || !unknowns.count(lp2y) || unknowns.count(dist)) {
        return ConstraintSubstitutionAttempt::No;
    }

    // Line is vertical, constraint can be translated to a vertical distance constraint
    if (factory.areEqual(lp1x, lp2x)) {
        factory.addRelationship(lp1y, lp2y, *dist);
        return ConstraintSubstitutionAttempt::Yes;
    }

    // Line is horizontal, constraint can be translated to a horizontal distance constraint
    if (factory.areEqual(lp1y, lp2y)) {
        factory.addRelationship(lp1x, lp2x, *dist);
        return ConstraintSubstitutionAttempt::Yes;
    }
    return ConstraintSubstitutionAttempt::Maybe;
}
ConstraintSubstitutionAttempt
substitutionForP2LDist(Constraint* constr, const USET_pD& unknowns, SubstitutionFactory& factory)
{
    double* p0x = constr->origParams()[0];
    double* p0y = constr->origParams()[1];
    double* lp1x = constr->origParams()[2];
    double* lp1y = constr->origParams()[3];
    double* lp2x = constr->origParams()[4];
    double* lp2y = constr->origParams()[5];
    double* dist = constr->origParams()[6];

    if (!unknowns.count(p0x) || !unknowns.count(p0y) || !unknowns.count(lp1y)
        || !unknowns.count(lp1x) || !unknowns.count(lp2x) || !unknowns.count(lp2y)
        || unknowns.count(dist)) {
        return ConstraintSubstitutionAttempt::No;
    }

    // Line is vertical, constraint can be translated to a horizontal distance constraint
    if (factory.areEqual(lp1x, lp2x)) {
        factory.addRelationship(p0x, lp1x, *dist);
        return ConstraintSubstitutionAttempt::Yes;
    }

    // Line is horizontal, constraint can be translated to a vertical distance constraint
    if (factory.areEqual(lp1y, lp2y)) {
        factory.addRelationship(p0y, lp1y, *dist);
        return ConstraintSubstitutionAttempt::Yes;
    }
    return ConstraintSubstitutionAttempt::Maybe;
}

void addCircleDistance(double* centerPos,
                       double* refPos,
                       double dist,
                       double radius,
                       SubstitutionFactory& factory)
{
    if (std::abs(*centerPos - *refPos) < radius) {
        factory.addRelationship(centerPos, refPos, std::abs(radius - dist));
    }
    else {
        factory.addRelationship(centerPos, refPos, radius + dist);
    }
}
ConstraintSubstitutionAttempt substitutionForC2LDist(ConstraintC2LDistance* constr,
                                                     const USET_pD& unknowns,
                                                     SubstitutionFactory& factory)
{
    double* dist = constr->origParams()[0];
    Circle circle = constr->circle;
    Line line = constr->line;


    if (!unknowns.count(circle.center.x) || !unknowns.count(circle.center.y)
        || !unknowns.count(circle.rad) || !unknowns.count(line.p1.x) || !unknowns.count(line.p1.y)
        || !unknowns.count(line.p2.x) || !unknowns.count(line.p2.y) || unknowns.count(dist)) {
        return ConstraintSubstitutionAttempt::No;
    }
    std::optional<double> rad = factory.value(circle.rad);
    if (!rad.has_value()) {
        return ConstraintSubstitutionAttempt::No;
    }

    // Line is vertical, the constraint can be translated into a horizontal distance
    if (factory.areEqual(line.p1.x, line.p2.x)) {
        addCircleDistance(circle.center.x, line.p1.x, *dist, *rad, factory);
        return ConstraintSubstitutionAttempt::Yes;
    }

    // Line is horizontal, the constraint can be translated into a vertical distance
    if (factory.areEqual(line.p1.y, line.p2.y)) {
        addCircleDistance(circle.center.y, line.p1.y, *dist, *rad, factory);
        return ConstraintSubstitutionAttempt::Yes;
    }

    return ConstraintSubstitutionAttempt::Maybe;
}
ConstraintSubstitutionAttempt substitutionForEqualLineLength(ConstraintEqualLineLength* constr,
                                                             const USET_pD& unknowns,
                                                             SubstitutionFactory& factory)
{
    Line line1 = constr->l1;
    Line line2 = constr->l2;

    if (!unknowns.count(line1.p1.x) || !unknowns.count(line1.p1.y) || !unknowns.count(line1.p2.x)
        || !unknowns.count(line1.p2.y) || !unknowns.count(line2.p1.x) || !unknowns.count(line2.p1.y)
        || !unknowns.count(line2.p2.x) || !unknowns.count(line2.p2.y)) {
        return ConstraintSubstitutionAttempt::No;
    }
    bool l1horiz = false;
    std::optional<double> l1length = std::nullopt;

    bool l2horiz = false;
    std::optional<double> l2length = std::nullopt;

    if (factory.isHorizontal(line1)) {
        l1horiz = true;
        l1length = factory.distance(line1.p1.x, line1.p2.x);
    }
    else if (factory.isVertical(line1)) {
        l1horiz = false;
        l1length = factory.distance(line1.p1.y, line1.p2.y);
    }
    else {
        return ConstraintSubstitutionAttempt::Maybe;
    }

    if (factory.isHorizontal(line2)) {
        l2horiz = true;
        l2length = factory.distance(line2.p1.x, line2.p2.x);
    }
    else if (factory.isVertical(line2)) {
        l2horiz = false;
        l2length = factory.distance(line2.p1.y, line2.p2.y);
    }
    else {
        return ConstraintSubstitutionAttempt::Maybe;
    }

    if (l1length.has_value() && l2length.has_value()) {
        return ConstraintSubstitutionAttempt::No;
    }
    else if (l1length.has_value()) {
        if (l2horiz) {
            factory.addRelationship(line2.p1.x, line2.p2.x, *l1length);
        }
        else {
            factory.addRelationship(line2.p1.y, line2.p2.y, *l1length);
        }
        return ConstraintSubstitutionAttempt::Yes;
    }
    else if (l2length.has_value()) {
        if (l1horiz) {
            factory.addRelationship(line1.p1.x, line1.p2.x, *l2length);
        }
        else {
            factory.addRelationship(line1.p1.y, line1.p2.y, *l2length);
        }
        return ConstraintSubstitutionAttempt::Yes;
    }
    return ConstraintSubstitutionAttempt::Maybe;
}

Substitution::Substitution(const VEC_pD& initialUnknowns,
                           const std::vector<Constraint*>& initialConstraints)
{
    USET_pD unknownsSet(initialUnknowns.begin(), initialUnknowns.end());
    SubstitutionFactory factory;

    std::vector<ConstraintSubstitutionAttempt> attempts(initialConstraints.size(),
                                                        ConstraintSubstitutionAttempt::Unknown);


    bool done = true;
    for (size_t i = 0; i < initialConstraints.size(); ++i) {
        auto constr = initialConstraints[i];

        if (constr->getTypeId() != Equal) {
            continue;
        }

        attempts[i] = substitutionForEqual(constr, unknownsSet, factory);
        if (attempts[i] == ConstraintSubstitutionAttempt::Yes) {
            done = false;  // If at least one equality was substituted, we can try to translate
                           // other constraints
        }
    }

    while (!done) {
        done = true;  // Done until proven false by having a successful substituttion
        for (size_t i = 0; i < initialConstraints.size(); ++i) {
            auto constr = initialConstraints[i];

            // No use trying again
            if (attempts[i] == ConstraintSubstitutionAttempt::No
                || attempts[i] == ConstraintSubstitutionAttempt::Yes) {
                continue;
            }

            // No substitution for temporary constraints
            if (constr->getTag() < 0) {
                continue;
            }

            ConstraintSubstitutionAttempt attempt = ConstraintSubstitutionAttempt::No;
            switch (constr->getTypeId()) {
                case Perpendicular:
                    attempt = substitutionForPerpendicular(constr, unknownsSet, factory);
                    break;
                case PointOnLine:
                    attempt = substitutionForPointOnLine(constr, unknownsSet, factory);
                    break;
                case C2LDistance:
                    attempt = substitutionForC2LDist(static_cast<ConstraintC2LDistance*>(constr),
                                                     unknownsSet,
                                                     factory);
                    break;
                case P2LDistance:
                    attempt = substitutionForP2LDist(constr, unknownsSet, factory);
                    break;
                case P2PDistance:
                    attempt = substitutionForP2PDist(constr, unknownsSet, factory);
                    break;
                case EqualLineLength:
                    attempt = substitutionForEqualLineLength(
                        static_cast<ConstraintEqualLineLength*>(constr),
                        unknownsSet,
                        factory);
            }
            attempts[i] = attempt;
            if (attempt == ConstraintSubstitutionAttempt::Yes) {
                done = false;
            }
        }
    }

    factory.compile();

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
    parameters.resize(untouchedUnknowns.size() + factory.unknownsReductionList.size());
    unknowns.resize(parameters.size());

    for (double* unknown : untouchedUnknowns) {
        unknowns[parameterIndex] = unknown;
        parameters[parameterIndex] = *unknown;
        reductionMap[unknown] = &parameters[parameterIndex];
        parameterIndex++;
    }
    for (const auto& reduction : factory.unknownsReductionList) {
        // A single unkown is promoted to parameter for every reduction group
        unknowns[parameterIndex] = reduction.first;
        parameters[parameterIndex] = *reduction.first;
        reductionMap[reduction.first] = &parameters[parameterIndex];
        for (double* reduced : reduction.second) {
            reductionMap[reduced] = &parameters[parameterIndex];
        }
        parameterIndex++;
    }

    substitutionUpdaters = std::move(factory.substUpdaters);
    constUpdaters = std::move(factory.constSubstUpdaters);
    for (auto& upd : substitutionUpdaters) {
        auto foundRoot = reductionMap.find(upd.root);
        auto foundFollower = reductionMap.find(upd.follower);

        if (foundRoot == reductionMap.end()) {
            std::cerr << "Unexpected! Could not find root for substitution updater :(\n";
            continue;
        }
        if (foundFollower == reductionMap.end()) {
            std::cerr << "Unexpected! Could not find follower for substitution updater :(\n";
            continue;
        }

        upd.root = foundRoot->second;
        upd.follower = foundFollower->second;
        substitutionMap[upd.follower] = upd.root;
    }
    for (auto& upd : constUpdaters) {
        auto foundRoot = reductionMap.find(upd.root);
        auto foundFollower = reductionMap.find(upd.follower);

        if (upd.root != nullptr && foundRoot == reductionMap.end()) {
            std::cerr << "Unexpected! Could not find root for substitution updater :(\n";
            continue;
        }
        if (foundFollower == reductionMap.end()) {
            std::cerr << "Unexpected! Could not find follower for substitution updater :(\n";
            continue;
        }

        upd.root = upd.root == nullptr ? nullptr : foundRoot->second;
        upd.follower = foundFollower->second;

        if (upd.root != nullptr) {
            substitutionMap[upd.follower] = upd.root;
        }
    }

    // Put all constraints which were not reduced in the constraints vector
    for (size_t i = 0; i < initialConstraints.size(); ++i) {
        if (attempts[i] != ConstraintSubstitutionAttempt::Yes) {
            constraints.push_back(initialConstraints[i]);
            initialConstraints[i]->redirectParams(reductionMap, substitutionMap);
        }
    }
}

void Substitution::initParams()
{
    for (size_t i = 0; i < unknowns.size(); ++i) {
        parameters[i] = *unknowns[i];
    }
}

void Substitution::applyConst() const
{
    for (const auto& upd : constUpdaters) {
        upd.apply();
    }
}
void Substitution::applySubst() const
{
    for (const auto& upd : substitutionUpdaters) {
        upd.apply();
    }
}
void Substitution::applyReduction() const
{
    for (const auto& red : reductionMap) {
        *red.first = *red.second;
    }
}

}  // namespace GCS
