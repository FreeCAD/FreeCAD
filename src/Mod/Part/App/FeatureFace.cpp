/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepBuilderAPI_Copy.hxx>
# include <TopoDS.hxx>
#endif

#include "FeatureFace.h"
#include "FaceMaker.h"


using namespace Part;


PROPERTY_SOURCE(Part::Face, Part::Feature)

Face::Face()
{
    ADD_PROPERTY(Sources,(nullptr));
    ADD_PROPERTY(FaceMakerClass,("Part::FaceMakerCheese"));//default value here is for legacy documents. Default for new objects is set in setupObject.
    Sources.setSize(0);
}

short Face::mustExecute() const
{
    if (FaceMakerClass.isTouched())
        return 1;
    if (Sources.isTouched())
        return 1;
    return Part::Feature::mustExecute();
}

void Face::setupObject()
{
    this->FaceMakerClass.setValue("Part::FaceMakerBullseye");
    Feature::setupObject();
}

App::DocumentObjectExecReturn *Face::execute()
{
    std::vector<App::DocumentObject*> links = Sources.getValues();
    if (links.empty())
        return new App::DocumentObjectExecReturn("No shapes linked");

    std::unique_ptr<FaceMaker> facemaker = FaceMaker::ConstructFromType(this->FaceMakerClass.getValue());

    for (std::vector<App::DocumentObject*>::iterator it = links.begin(); it != links.end(); ++it) {
        if (!(*it))
            return new App::DocumentObjectExecReturn("Linked object is not a Part object (has no Shape).");
        TopoDS_Shape shape = Feature::getShape(*it);
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Linked shape object is empty");

        // this is a workaround for an obscure OCC bug which leads to empty tessellations
        // for some faces. Making an explicit copy of the linked shape seems to fix it.
        // The error only happens when re-computing the shape.
        /*if (!this->Shape.getValue().IsNull()) {
            BRepBuilderAPI_Copy copy(shape);
            shape = copy.Shape();
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Linked shape object is empty");
        }*/

        if(links.size() == 1 && shape.ShapeType() == TopAbs_COMPOUND)
            facemaker->useCompound(TopoDS::Compound(shape));
        else
            facemaker->addShape(shape);
    }

    facemaker->Build();

    TopoDS_Shape aFace = facemaker->Shape();
    if (aFace.IsNull())
        return new App::DocumentObjectExecReturn("Creating face failed (null shape result)");
    this->Shape.setValue(aFace);

    return App::DocumentObject::StdReturn;
}

