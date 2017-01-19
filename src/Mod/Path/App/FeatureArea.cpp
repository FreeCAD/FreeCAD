/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>

#include "FeatureArea.h"
#include <App/DocumentObjectPy.h>
#include <Base/Placement.h>
#include <Mod/Part/App/PartFeature.h>

using namespace Path;

PROPERTY_SOURCE(Path::FeatureArea, Part::Feature)

PARAM_ENUM_STRING_DECLARE(static const char *Enums,AREA_PARAMS_ALL)

FeatureArea::FeatureArea()
{
    ADD_PROPERTY(Sources,(0));
    ADD_PROPERTY(WorkPlane,(TopoDS_Shape()));

    PARAM_PROP_ADD("Area",AREA_PARAMS_OPCODE);
    PARAM_PROP_ADD("Area",AREA_PARAMS_BASE);
    PARAM_PROP_ADD("Offset",AREA_PARAMS_OFFSET);
    PARAM_PROP_ADD("Pocket",AREA_PARAMS_POCKET);
    PARAM_PROP_ADD("Offset Settings", AREA_PARAMS_OFFSET_CONF);
    PARAM_PROP_ADD("libarea Settings",AREA_PARAMS_CAREA);

    PARAM_PROP_SET_ENUM(Enums,AREA_PARAMS_ALL);
    PocketMode.setValue((long)0);
}

FeatureArea::~FeatureArea()
{
}

App::DocumentObjectExecReturn *FeatureArea::execute(void)
{
    std::vector<App::DocumentObject*> links = Sources.getValues();
    if (links.empty())
        return new App::DocumentObjectExecReturn("No shapes linked");

    for (std::vector<App::DocumentObject*>::iterator it = links.begin(); it != links.end(); ++it) {
        if (!(*it && (*it)->isDerivedFrom(Part::Feature::getClassTypeId())))
            return new App::DocumentObjectExecReturn("Linked object is not a Part object (has no Shape).");
        TopoDS_Shape shape = static_cast<Part::Feature*>(*it)->Shape.getShape().getShape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Linked shape object is empty");
    }

    AreaParams params;

#define AREA_PROP_GET(_param) \
    params.PARAM_FNAME(_param) = PARAM_FNAME(_param).getValue();

    PARAM_FOREACH(AREA_PROP_GET,AREA_PARAMS_CONF)

    Area area(&params);

    TopoDS_Shape workPlane = WorkPlane.getShape().getShape();
    if(!workPlane.IsNull())
        area.setPlane(workPlane);

    area.clean(true);
    for (std::vector<App::DocumentObject*>::iterator it = links.begin(); it != links.end(); ++it) {
        area.add(static_cast<Part::Feature*>(*it)->Shape.getShape().getShape(),
                PARAM_PROP_ARGS(AREA_PARAMS_OPCODE));
    }

    std::list<TopoDS_Shape> shapes;
    if(fabs(Offset.getValue())>Precision::Confusion())
        area.makeOffset(shapes,PARAM_PROP_ARGS(AREA_PARAMS_OFFSET));

    if(PocketMode.getValue()) {
        Area areaPocket(&params);
        if(shapes.empty())
            areaPocket.add(area.getShape());
        else{
            bool front = true;
            if(shapes.size()>1) {
                double step = Stepover.getValue();
                if(fabs(step)<Precision::Confusion())
                    step = Offset.getValue();
                front = step>0;
            }
            areaPocket.add(front?shapes.front():shapes.back());
        }
        shapes.push_back(areaPocket.makePocket(PARAM_PROP_ARGS(AREA_PARAMS_POCKET)));
    }

    if(shapes.empty())
        this->Shape.setValue(area.getShape());
    else if(shapes.size()==1)
        this->Shape.setValue(shapes.front());
    else {
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        for(const TopoDS_Shape &s : shapes)
            builder.Add(compound,s);
        this->Shape.setValue(compound);
    }
    return Part::Feature::execute();
}

short FeatureArea::mustExecute(void) const
{
    if (Sources.isTouched())
        return 1;
    if (WorkPlane.isTouched())
        return 1;

    PARAM_PROP_TOUCHED(AREA_PARAMS_ALL)

    return Part::Feature::mustExecute();
}

// Python Area feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Path::FeatureAreaPython, Path::FeatureArea)

template<> const char* Path::FeatureAreaPython::getViewProviderName(void) const {
    return "PathGui::ViewProviderArea";
}
/// @endcond

// explicit template instantiation
template class PathExport FeaturePythonT<Path::FeatureArea>;
}

