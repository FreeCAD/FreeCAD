// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include <limits>
#include <algorithm>
#include <cmath>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include <App/Datums.h>
#include <App/Document.h>
#include <Base/Axis.h>
#include <Base/Tools.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Part2DObject.h>

#include "FeatureLinearPattern.h"
#include "DatumLine.h"
#include "DatumPlane.h"


using namespace PartDesign;

namespace PartDesign
{

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::LinearPattern, PartDesign::Transformed)

LinearPattern::LinearPattern()
{
    Part::LinearPatternExtension::initExtension(this);
    SuppressedIndices.setStatus(App::Property::Hidden, true);
}

bool LinearPattern::isTransformationSuppressed(int index) const
{
    return isInstanceSuppressed(index);
}

void LinearPattern::setTransformationSuppressed(int index, bool suppressed)
{
    setInstanceSuppressed(index, suppressed);
}

void LinearPattern::importSuppressedIndices()
{
    std::vector<Base::Vector3d> positions;
    for (long index : SuppressedIndices.getValues()) {
        if (index >= 0) {
            positions.push_back(getInstancePosition(index));
        }
    }
    SuppressedPositions.setValues(positions);
}

void LinearPattern::updateSuppressedIndices()
{
    // Keep the legacy Python property as a projection onto the current grid.
    std::vector<long> indices;
    for (const auto& position : SuppressedPositions.getValues()) {
        if (position.x >= 0 && position.x < Occurrences.getValue() && position.y >= 0
            && position.y < Occurrences2.getValue() && position.x == std::floor(position.x)
            && position.y == std::floor(position.y) && position.z == 0) {
            const double index = position.x * Occurrences2.getValue() + position.y;
            if (index <= std::numeric_limits<long>::max()) {
                indices.push_back(static_cast<long>(index));
            }
        }
    }
    std::ranges::sort(indices);
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    if (indices != SuppressedIndices.getValues()) {
        SuppressedIndices.setValues(indices);
    }
}

void LinearPattern::onChanged(const App::Property* prop)
{
    if (!syncingSuppression && !isRestoring()
        && !(getDocument() && getDocument()->isPerformingTransaction())) {
        Base::StateLocker guard(syncingSuppression);
        if (prop == &SuppressedIndices) {
            importSuppressedIndices();
        }
        else if (prop == &SuppressedPositions || prop == &Occurrences || prop == &Occurrences2) {
            updateSuppressedIndices();
        }
    }
    Transformed::onChanged(prop);
}

void LinearPattern::onDocumentRestored()
{
    Transformed::onDocumentRestored();
    Base::StateLocker guard(syncingSuppression);
    // Older documents only contain flat indices. All occurrence properties are restored now.
    if (SuppressedPositions.getSize() == 0 && SuppressedIndices.getSize() != 0) {
        importSuppressedIndices();
    }
    updateSuppressedIndices();
    SuppressedIndices.setStatus(App::Property::Hidden, true);
}

gp_Dir LinearPattern::getDirectionFromProperty(const App::PropertyLinkSub& dirProp) const
{
    App::DocumentObject* refObject = dirProp.getValue();
    if (!refObject) {
        throw Base::ValueError("No direction reference specified");
    }

    if (auto* plane = freecad_cast<PartDesign::Plane*>(refObject)) {
        Base::Vector3d d = plane->getNormal();
        gp_Dir dir(d.x, d.y, d.z);

        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        dir.Transform(invObjLoc.Transformation());
        return Base::convertTo<gp_Vec>(dir);
    }
    else if (auto* line = freecad_cast<PartDesign::Line*>(refObject)) {
        Base::Vector3d d = line->getDirection();
        gp_Dir dir(d.x, d.y, d.z);

        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        dir.Transform(invObjLoc.Transformation());
        return Base::convertTo<gp_Vec>(dir);
    }

    return Part::LinearPatternExtension::getDirectionFromProperty(dirProp);
}

const std::list<gp_Trsf> LinearPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    return calculateTransformations();
}

void LinearPattern::handleChangedPropertyType(
    Base::XMLReader& reader,
    const char* TypeName,
    App::Property* prop
)
// transforms properties that had been changed
{
    // property Occurrences had the App::PropertyInteger and was changed to
    // App::PropertyIntegerConstraint
    if (prop == &Occurrences && strcmp(TypeName, "App::PropertyInteger") == 0) {
        App::PropertyInteger OccurrencesProperty;
        // restore the PropertyInteger to be able to set its value
        OccurrencesProperty.Restore(reader);
        Occurrences.setValue(OccurrencesProperty.getValue());
    }
    else {
        Transformed::handleChangedPropertyType(reader, TypeName, prop);
    }
}

}  // namespace PartDesign
