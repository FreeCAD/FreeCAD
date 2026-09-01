// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>    *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "LinkArrayLinear.h"

#include <optional>
#include <algorithm>

#include <App/Document.h>
#include <App/Datums.h>
#include <App/PropertyLinks.h>
#include <App/SuppressibleExtension.h>
#include <Base/Tools.h>
#include <Precision.hxx>
#include <gp_Trsf.hxx>

using namespace Part;

PROPERTY_SOURCE_WITH_EXTENSIONS(Part::LinkArrayLinear, Part::LinkArray)

LinkArrayLinear::LinkArrayLinear()
{
    Part::LinearPatternExtension::initExtension(this);
    ADD_PROPERTY_TYPE(
        GeneratedOccurrences2,
        (0),
        "Pattern",
        static_cast<App::PropertyType>(App::Prop_Hidden | App::Prop_ReadOnly | App::Prop_Output),
        "Second-direction occurrence count used to generate the current elements"
    );
    setDirectionLinkScopes();
}

App::DocumentObjectExecReturn* LinkArrayLinear::execute()
{
    Base::StateLocker guard(syncingSuppression);
    auto* result = inherited::execute();
    applyElementSuppression();
    return result;
}

void LinkArrayLinear::connectElementSuppression()
{
    suppressionConnections.clear();
    const auto& elements = ElementList.getValues();
    for (size_t i = 0; i < elements.size(); ++i) {
        if (!elements[i]) {
            continue;
        }
        suppressionConnections.emplace_back(
            elements[i]->signalChanged.connect([this,
                                                i](const App::DocumentObject& obj,
                                                   const App::Property& prop) {
                if (syncingSuppression || isRestoring()
                    || (getDocument() && getDocument()->isPerformingTransaction())) {
                    return;
                }
                const auto* suppressible = obj.getExtensionByType<App::SuppressibleExtension>(true);
                if (!suppressible || &prop != &suppressible->Suppressed) {
                    return;
                }
                const auto stride = static_cast<size_t>(std::max(1L, GeneratedOccurrences2.getValue()));
                setPositionSuppressed(
                    Base::Vector3d(i / stride, i % stride, 0),
                    suppressible->Suppressed.getValue()
                );
            })
        );
    }
}

void LinkArrayLinear::restoreElementSuppression()
{
    // Legacy files only store Suppressed on the generated links.
    if (GeneratedOccurrences2.getValue() == 0) {
        GeneratedOccurrences2.setValue(Occurrences2.getValue());
        const auto& elements = ElementList.getValues();
        for (size_t i = 0; i < elements.size(); ++i) {
            const auto* suppressible = elements[i]
                ? elements[i]->getExtensionByType<App::SuppressibleExtension>(true)
                : nullptr;
            if (suppressible && suppressible->Suppressed.getValue()) {
                setInstanceSuppressed(static_cast<long>(i), true);
            }
        }
    }
    connectElementSuppression();
}

void LinkArrayLinear::applyElementSuppression()
{
    Base::StateLocker guard(syncingSuppression);
    const auto& positions = SuppressedPositions.getValues();
    const auto& elements = ElementList.getValues();
    const auto stride = static_cast<size_t>(std::max(1L, GeneratedOccurrences2.getValue()));
    for (size_t i = 0; i < elements.size(); ++i) {
        auto* suppressible = elements[i]
            ? elements[i]->getExtensionByType<App::SuppressibleExtension>(true)
            : nullptr;
        if (!suppressible) {
            continue;
        }
        const Base::Vector3d position(i / stride, i % stride, 0);
        const bool suppressed = std::ranges::find(positions, position) != positions.end();
        if (suppressible->Suppressed.getValue() != suppressed) {
            suppressible->Suppressed.setValue(suppressed);
        }
    }
}

void LinkArrayLinear::onChanged(const App::Property* prop)
{
    inherited::onChanged(prop);
    if (isRestoring()) {
        return;
    }
    if (prop == &ElementList) {
        connectElementSuppression();
    }
    else if (
        prop == &SuppressedPositions && !syncingSuppression
        && !(getDocument() && getDocument()->isPerformingTransaction())
    ) {
        applyElementSuppression();
    }
}

void LinkArrayLinear::onDocumentRestored()
{
    inherited::onDocumentRestored();
    setDirectionLinkScopes();
    Base::StateLocker guard(syncingSuppression);
    restoreElementSuppression();
}

void LinkArrayLinear::setDirectionLinkScopes()
{
    Direction.setScope(App::LinkScope::Global);
    Direction2.setScope(App::LinkScope::Global);
}

gp_Dir LinkArrayLinear::getDirectionFromProperty(const App::PropertyLinkSub& dirProp) const
{
    const auto datumDirection = [](App::DocumentObject* obj) -> std::optional<gp_Dir> {
        if (auto* line = freecad_cast<App::Line*>(obj)) {
            Base::Vector3d d = line->getDirection();
            return gp_Dir(d.x, d.y, d.z);
        }

        if (auto* plane = freecad_cast<App::Plane*>(obj)) {
            Base::Vector3d d = plane->getDirection();
            return gp_Dir(d.x, d.y, d.z);
        }

        return {};
    };

    auto* refObject = dirProp.getValue();
    auto* lcs = freecad_cast<App::LocalCoordinateSystem*>(refObject);

    std::vector<std::string> subStrings = dirProp.getSubValues();
    if (!lcs && !subStrings.empty() && !subStrings.front().empty()) {
        std::string sub = subStrings.front();
        if (sub.back() != '.') {
            sub += '.';
        }

        App::DocumentObject* resolved = nullptr;
        try {
            resolved = refObject->getSubObject(sub.c_str());
        }
        catch (const Base::Exception&) {
            resolved = nullptr;
        }

        if (!resolved && refObject->getDocument()) {
            std::string role = subStrings.front();
            const auto dot = role.rfind('.');
            if (dot != std::string::npos) {
                role = role.substr(dot + 1);
            }
            resolved = refObject->getDocument()->getObject(role.c_str());
        }

        if (resolved && resolved != refObject) {
            if (auto direction = datumDirection(resolved)) {
                return *direction;
            }

            lcs = freecad_cast<App::LocalCoordinateSystem*>(resolved);
        }
    }

    if (!lcs) {
        if (auto direction = datumDirection(refObject)) {
            return *direction;
        }
        return LinearPatternExtension::getDirectionFromProperty(dirProp);
    }

    if (subStrings.empty() || subStrings.front().empty()) {
        throw Base::ValueError("No direction reference specified");
    }

    std::string role = subStrings.front();
    const auto dot = role.rfind('.');
    if (dot != std::string::npos) {
        role = role.substr(dot + 1);
    }

    if (role == App::LocalCoordinateSystem::AxisRoles[0]
        || role == App::LocalCoordinateSystem::AxisRoles[1]
        || role == App::LocalCoordinateSystem::AxisRoles[2]) {
        Base::Vector3d d = lcs->getAxis(role.c_str())->getDirection();
        return gp_Dir(d.x, d.y, d.z);
    }

    if (role == App::LocalCoordinateSystem::PlaneRoles[0]
        || role == App::LocalCoordinateSystem::PlaneRoles[1]
        || role == App::LocalCoordinateSystem::PlaneRoles[2]) {
        Base::Vector3d d = lcs->getPlane(role.c_str())->getDirection();
        return gp_Dir(d.x, d.y, d.z);
    }

    return LinearPatternExtension::getDirectionFromProperty(dirProp);
}

std::vector<Base::Placement> LinkArrayLinear::getElementPlacements()
{
    int occurrences = Occurrences.getValue();
    int occurrences2 = Occurrences2.getValue();
    if (occurrences < 1 || occurrences2 < 1) {
        throw Base::ValueError("At least one occurrence required");
    }

    updateSpacings();

    gp_Vec offset1 = calculateOffsetVectorWithDefault(LinearPatternDirection::First);
    std::vector<gp_Vec> steps1 = calculateSteps(LinearPatternDirection::First, offset1);

    gp_Vec offset2 = calculateOffsetVectorWithDefault(LinearPatternDirection::Second);
    std::vector<gp_Vec> steps2 = calculateSteps(LinearPatternDirection::Second, offset2);

    std::list<gp_Trsf> transformations;
    for (const auto& step1 : steps1) {
        for (const auto& step2 : steps2) {
            gp_Trsf trans;
            trans.SetTranslation(step1 + step2);
            transformations.push_back(trans);
        }
    }

    auto placements = placementsFromTransforms(transformations);
    if (GeneratedOccurrences2.getValue() != occurrences2) {
        GeneratedOccurrences2.setValue(occurrences2);
    }
    return placements;
}

gp_Vec LinkArrayLinear::calculateOffsetVectorWithDefault(LinearPatternDirection dir) const
{
    const auto objectAxisDirection = [](const std::string& role) -> std::optional<gp_Vec> {
        if (role == "X_Axis") {
            return gp_Vec(1.0, 0.0, 0.0);
        }
        if (role == "Y_Axis") {
            return gp_Vec(0.0, 1.0, 0.0);
        }
        if (role == "Z_Axis") {
            return gp_Vec(0.0, 0.0, 1.0);
        }

        return {};
    };

    const bool isSecondDir = dir == LinearPatternDirection::Second;
    const auto& directionProp = isSecondDir ? Direction2 : Direction;
    if (directionProp.getValue()) {
        return calculateOffsetVector(dir);
    }

    std::vector<std::string> subStrings = directionProp.getSubValues();
    gp_Vec offset;
    if (!subStrings.empty()) {
        std::string role = subStrings.front();
        const auto dot = role.rfind('.');
        if (dot != std::string::npos) {
            role = role.substr(dot + 1);
        }
        if (auto direction = objectAxisDirection(role)) {
            offset = *direction;
        }
    }
    if (offset.Magnitude() <= Precision::Confusion()) {
        offset = isSecondDir ? gp_Vec(0.0, 1.0, 0.0) : gp_Vec(1.0, 0.0, 0.0);
    }

    const auto& occurrencesProp = isSecondDir ? Occurrences2 : Occurrences;
    int occurrences = occurrencesProp.getValue();
    if (occurrences <= 1) {
        return gp_Vec();
    }

    const auto& reversedProp = isSecondDir ? Reversed2 : Reversed;
    if (reversedProp.getValue()) {
        offset.Reverse();
    }

    const auto& modeProp = isSecondDir ? Mode2 : Mode;
    if (static_cast<LinearPatternMode>(modeProp.getValue()) == LinearPatternMode::Extent) {
        const auto& lengthProp = isSecondDir ? Length2 : Length;
        double distance = lengthProp.getValue();
        if (distance < Precision::Confusion()) {
            throw Base::ValueError("Pattern length too small");
        }
        offset *= distance / (occurrences - 1);
    }

    return offset;
}
