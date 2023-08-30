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
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <gp_Lin.hxx>
# include <gp_Pnt.hxx>
#endif

#include "DatumLine.h"

using namespace PartDesign;
using namespace Attacher;

// ============================================================================

const char* Line::ResizeModeEnums[]= {"Automatic","Manual",nullptr};

PROPERTY_SOURCE(PartDesign::Line, Part::Datum)

Line::Line()
{
    // These properties are only relevant for the visual appearance.
    // Since they are getting changed from within its view provider
    // their type is set to "Output" to avoid that they are marked as
    // touched all the time.
    ADD_PROPERTY_TYPE(ResizeMode,(static_cast<long>(0)), "Size", App::Prop_Output, "Automatic or manual resizing");
    ResizeMode.setEnums(ResizeModeEnums);
    ADD_PROPERTY_TYPE(Length,(20), "Size", App::Prop_Output, "Length of the line");
    Length.setReadOnly(true);

    this->setAttacher(new AttachEngineLine);
    // Create a shape, which will be used by the Sketcher. Them main function is to avoid a dependency of
    // Sketcher on the PartDesign module
    BRepBuilderAPI_MakeEdge builder(gp_Lin(gp_Pnt(0,0,0), gp_Dir(0,0,1)));
    if (!builder.IsDone())
        return;
    TopoDS_Shape myShape = builder.Shape();
    myShape.Infinite(Standard_True);
    Shape.setValue(myShape);

    Support.touch();
}

Line::~Line() = default;

Base::Vector3d Line::getDirection() const
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d dir;
    rot.multVec(Base::Vector3d(0,0,1), dir);
    return dir;
}

void Line::onChanged(const App::Property *prop)
{
    if (prop == &ResizeMode) {
        if (ResizeMode.getValue() == 0) {
            Length.setReadOnly(true);
        }
        else {
            Length.setReadOnly(false);
        }
    }
    Datum::onChanged(prop);
}
