/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <BRepBuilderAPI_MakeFace.hxx>
# include <gp_Pln.hxx>
# include <gp_Pnt.hxx>
#endif

#include "DatumPlane.h"

using namespace PartDesign;
using namespace Attacher;

// ============================================================================

const char* Plane::ResizeModeEnums[]= {"Automatic","Manual",nullptr};

PROPERTY_SOURCE(PartDesign::Plane, Part::Datum)

Plane::Plane()
{
    // These properties are only relevant for the visual appearance.
    // Since they are getting changed from within its view provider
    // their type is set to "Output" to avoid that they are marked as
    // touched all the time.
    ADD_PROPERTY_TYPE(ResizeMode,(static_cast<long>(0)), "Size", App::Prop_Output, "Automatic or manual resizing");
    ResizeMode.setEnums(ResizeModeEnums);
    ADD_PROPERTY_TYPE(Length,(20), "Size", App::Prop_Output, "Length of the plane");
    ADD_PROPERTY_TYPE(Width,(20), "Size", App::Prop_Output, "Width of the plane");

    Length.setReadOnly(true);
    Width.setReadOnly(true);

    this->setAttacher(new AttachEnginePlane);
    // Create a shape, which will be used by the Sketcher. Them main function is to avoid a dependency of
    // Sketcher on the PartDesign module
    BRepBuilderAPI_MakeFace builder(gp_Pln(gp_Pnt(0,0,0), gp_Dir(0,0,1)));
    if (!builder.IsDone())
        return;
    TopoDS_Shape myShape = builder.Shape();
    myShape.Infinite(Standard_True);
    Shape.setValue(myShape);
}

Plane::~Plane() = default;

Base::Vector3d Plane::getNormal()
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d normal;
    rot.multVec(Base::Vector3d(0,0,1), normal);
    return normal;
}

void Plane::Restore(Base::XMLReader& reader)
{
    // set it to Manual to avoid to automatically adjust
    // Length because it will be read before ResizeMode
    // See bug #0004540
    ResizeMode.setValue("Manual");
    Datum::Restore(reader);
}

void Plane::onChanged(const App::Property *prop)
{
    if (prop == &ResizeMode) {
        if (ResizeMode.getValue() == 0) {
            Length.setReadOnly(true);
            Width.setReadOnly(true);
        }
        else {
            Length.setReadOnly(false);
            Width.setReadOnly(false);
        }
    }

    Datum::onChanged(prop);
}
