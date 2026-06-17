// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Lesser General Public             *
 *   License as published by the Free Software Foundation; either           *
 *   version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA 02111-1307, USA                                  *
 *                                                                         *
 ***************************************************************************/

#include "LinkArrayPolar.h"

#include <optional>

#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

#include <App/Datums.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <Base/Exception.h>
#include <Base/Rotation.h>

using namespace Part;

PROPERTY_SOURCE_WITH_EXTENSIONS(Part::LinkArrayPolar, Part::LinkArray)

namespace
{

std::string roleFromSubName(const std::string& subName)
{
    std::string role = subName;
    if (!role.empty() && role.back() == '.') {
        role.pop_back();
    }

    const auto dot = role.rfind('.');
    if (dot != std::string::npos) {
        role = role.substr(dot + 1);
    }

    return role;
}

std::optional<gp_Ax2> axisFromLine(App::Line* line)
{
    if (!line) {
        return {};
    }

    Base::Vector3d base = line->getBasePoint();
    Base::Vector3d dir = line->getDirection();
    return gp_Ax2(gp_Pnt(base.x, base.y, base.z), gp_Dir(dir.x, dir.y, dir.z));
}

std::optional<gp_Ax2> axisFromLocalCoordinateSystem(App::LocalCoordinateSystem* lcs,
                                                    const std::string& role)
{
    if (!lcs) {
        return {};
    }

    if (role == App::LocalCoordinateSystem::AxisRoles[0]
        || role == App::LocalCoordinateSystem::AxisRoles[1]
        || role == App::LocalCoordinateSystem::AxisRoles[2]) {
        return axisFromLine(lcs->getAxis(role.c_str()));
    }

    return {};
}

App::DocumentObject* resolveSubObject(App::DocumentObject* refObject, const std::string& subName)
{
    if (!refObject || subName.empty()) {
        return nullptr;
    }

    std::string sub = subName;
    if (sub.back() != '.') {
        sub += '.';
    }

    try {
        if (auto* resolved = refObject->getSubObject(sub.c_str())) {
            return resolved;
        }
    }
    catch (const Base::Exception&) {
    }

    if (auto* doc = refObject->getDocument()) {
        return doc->getObject(roleFromSubName(subName).c_str());
    }

    return nullptr;
}

}  // namespace

LinkArrayPolar::LinkArrayPolar()
{
    Part::PolarPatternExtension::initExtension(this);
    setAxisLinkScope();
}

void LinkArrayPolar::onDocumentRestored()
{
    inherited::onDocumentRestored();
    setAxisLinkScope();
}

void LinkArrayPolar::setAxisLinkScope()
{
    Axis.setScope(App::LinkScope::Global);
}

std::vector<Base::Placement> LinkArrayPolar::getElementPlacements()
{
    updateSpacings();
    return placementsFromTransforms(calculateTransformations());
}

gp_Ax2 LinkArrayPolar::getRotation() const
{
    App::DocumentObject* refObject = Axis.getValue();
    std::vector<std::string> subStrings = Axis.getSubValues();
    std::string role;
    if (!subStrings.empty()) {
        role = roleFromSubName(subStrings.front());
    }

    if (!refObject && (role == "X_Axis" || role == "Y_Axis" || role == "Z_Axis")) {
        gp_Dir direction(0.0, 0.0, 1.0);
        if (role == "X_Axis") {
            direction = gp_Dir(1.0, 0.0, 0.0);
        }
        else if (role == "Y_Axis") {
            direction = gp_Dir(0.0, 1.0, 0.0);
        }
        gp_Ax2 localAxis(gp_Pnt(), direction);
        if (Reversed.getValue()) {
            localAxis.SetDirection(localAxis.Direction().Reversed());
        }
        return localAxis;
    }
    if (!refObject) {
        return gp_Ax2();
    }

    std::optional<gp_Ax2> axis;
    if (auto* lcs = freecad_cast<App::LocalCoordinateSystem*>(refObject)) {
        axis = axisFromLocalCoordinateSystem(lcs, role);
    }

    if (!axis && !subStrings.empty()) {
        App::DocumentObject* resolved = resolveSubObject(refObject, subStrings.front());
        if (auto* line = freecad_cast<App::Line*>(resolved)) {
            axis = axisFromLine(line);
        }
        else if (auto* lcs = freecad_cast<App::LocalCoordinateSystem*>(resolved)) {
            axis = axisFromLocalCoordinateSystem(lcs, role);
        }
    }

    if (!axis) {
        axis = axisFromLine(freecad_cast<App::Line*>(refObject));
    }

    if (!axis) {
        return PolarPatternExtension::getRotation();
    }

    gp_Ax2 transformed = *axis;
    transformAxisToLocal(transformed);

    if (Reversed.getValue()) {
        transformed.SetDirection(transformed.Direction().Reversed());
    }

    return transformed;
}

void LinkArrayPolar::transformAxisToLocal(gp_Ax2& axis) const
{
    auto* prop = freecad_cast<App::PropertyPlacement*>(getPropertyByName("Placement"));
    if (!prop) {
        return;
    }

    Base::Placement pl = prop->getValue();
    Base::Rotation rot(pl.getRotation());
    Base::Vector3d rotAxis;
    double angle;
    rot.getValue(rotAxis, angle);

    gp_Trsf trf;
    trf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(rotAxis.x, rotAxis.y, rotAxis.z)), angle);
    trf.SetTranslationPart(gp_Vec(pl.getPosition().x, pl.getPosition().y, pl.getPosition().z));
    trf.Invert();

    gp_Pnt base = axis.Location();
    gp_Dir direction = axis.Direction();
    base.Transform(trf);
    direction.Transform(trf);
    axis = gp_Ax2(base, direction);
}
