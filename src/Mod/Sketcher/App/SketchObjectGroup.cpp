// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Base/Exception.h>
#include <Base/Tools.h>
#include "GeometryFacade.h"
#include "GroupGeometry.h"
#include "SketchObject.h"

int Sketcher::SketchObject::replaceGroupGeometry(
    int constraintId,
    const std::vector<Part::Geometry*>& source
)
{
    const auto& constraints = Constraints.getValues();
    if (constraintId < 0 || constraintId >= static_cast<int>(constraints.size())
        || constraints[constraintId]->Type != Group) {
        throw Base::ValueError("Expected a Group constraint");
    }
    const auto* group = constraints[constraintId];
    const int handle = group->getGeoId(0);
    const auto* line = dynamic_cast<const Part::GeomLineSegment*>(getGeometry(handle));
    const auto& geometry = getInternalGeometry();
    auto removed = getGroupGeometries(handle);
    removed.erase(GeoEnum::GeoUndef);
    if (!line || handle < 0 || removed.empty()) {
        throw Base::ValueError("Invalid group handle or members");
    }
    for (int member : removed) {
        if (member < 0 || member >= static_cast<int>(geometry.size()) || member == handle
            || isGroupHandle(member)) {
            throw Base::ValueError("Invalid group member");
        }
    }
    auto replacements = transformGroupGeometry(
        source,
        line->getStartPoint(),
        line->getEndPoint(),
        group->getFileHeight()
    );

    // Retain every geometry outside this group, including its handle. Constraint tags
    // are cloned so named dimensions and expressions follow any changed indices.
    std::vector<int> mapping(geometry.size(), GeoEnum::GeoUndef);
    std::vector<Part::Geometry*> newGeometry;
    for (size_t i = 0; i < geometry.size(); ++i) {
        if (!removed.contains(static_cast<int>(i))) {
            mapping[i] = static_cast<int>(newGeometry.size());
            newGeometry.push_back(geometry[i]);
        }
    }
    const int first = static_cast<int>(newGeometry.size());
    bool construction = !removed.empty();
    for (int member : removed) {
        construction = construction && GeometryFacade::getConstruction(getGeometry(member));
    }
    for (auto& geo : replacements) {
        if (construction) {
            GeometryFacade::setConstruction(geo.get(), true);
        }
        generateId(geo.get());
        newGeometry.push_back(geo.get());
    }

    std::vector<std::unique_ptr<Constraint>> ownedConstraints;
    std::vector<Constraint*> newConstraints;
    int result = -1;
    for (const auto* original : constraints) {
        if (original != group) {
            bool touchesRemoved = false;
            for (int i = 0; original->hasElement(i); ++i) {
                touchesRemoved |= removed.contains(original->getGeoId(i));
            }
            if (touchesRemoved) {
                continue;
            }
        }
        auto copy = std::unique_ptr<Constraint>(original->clone());
        if (original == group) {
            result = static_cast<int>(newConstraints.size());
            copy->truncateElements(1);
        }
        for (int i = 0; copy->hasElement(i); ++i) {
            const int id = copy->getGeoId(i);
            if (id >= 0) {
                copy->setGeoId(i, mapping.at(id));
            }
        }
        if (original == group) {
            for (int i = first; i < static_cast<int>(newGeometry.size()); ++i) {
                copy->addElement(GeoElementId(i));
            }
        }
        newConstraints.push_back(copy.get());
        ownedConstraints.push_back(std::move(copy));
    }

    Base::StateLocker lock(managedoperation, true);
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        Geometry.setValues(newGeometry);
        Constraints.setValues(newConstraints);
    }
    Geometry.touch();
    return result;
}
