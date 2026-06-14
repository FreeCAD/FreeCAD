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

#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Lin.hxx>
#include <gp_Circ.hxx>
#include <gp_Ax2.hxx>
#include <BRepAdaptor_Curve.hxx>

#include "DatumLine.h"
#include <Base/Axis.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Part2DObject.h>
#include <App/Datums.h>

#include "FeaturePolarPattern.h"

using namespace PartDesign;

namespace PartDesign
{

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::PolarPattern, PartDesign::Transformed)

PolarPattern::PolarPattern()
{
    Part::PolarPatternExtension::initExtension(this);
}

gp_Ax2 PolarPattern::getRotation() const
{
    App::DocumentObject* refObject = Axis.getValue();
    if (!refObject) {
        return gp_Ax2();
    }

    if (refObject->isDerivedFrom<PartDesign::Line>()) {
        PartDesign::Line* line = static_cast<PartDesign::Line*>(refObject);
        Base::Vector3d base = line->getBasePoint();
        gp_Pnt axbase(base.x, base.y, base.z);
        Base::Vector3d dir = line->getDirection();
        gp_Dir axdir(dir.x, dir.y, dir.z);

        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        axbase.Transform(invObjLoc.Transformation());
        axdir.Transform(invObjLoc.Transformation());

        gp_Ax2 axis(axbase, axdir);
        if (Reversed.getValue()) {
            axis.SetDirection(axis.Direction().Reversed());
        }
        return axis;
    }

    return Part::PolarPatternExtension::getRotation();
}

const std::list<gp_Trsf> PolarPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    return calculateTransformations();
}

void PolarPattern::handleChangedPropertyType(
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
