/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
 *   Copyright (c) 2017 Lei Zheng <realthunder.dev@gmail.com>             *
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
# include <Standard_Version.hxx>
# include <TopoDS_Shape.hxx>
#endif

#include "FeaturePathShape.h"


using namespace Path;

PROPERTY_SOURCE(Path::FeatureShape, Path::Feature)

PARAM_ENUM_STRING_DECLARE(static const char *Enums,AREA_PARAMS_PATH)

FeatureShape::FeatureShape()
{
    ADD_PROPERTY(Sources,(nullptr));
    ADD_PROPERTY_TYPE(StartPoint,(Base::Vector3d()),"Path",App::Prop_None,"Feed start position");
    ADD_PROPERTY_TYPE(UseStartPoint,(false),"Path",App::Prop_None,"Enable feed start position");
    PARAM_PROP_ADD("Path",AREA_PARAMS_PATH);
    PARAM_PROP_SET_ENUM(Enums,AREA_PARAMS_PATH);
}

FeatureShape::~FeatureShape()
{
}

App::DocumentObjectExecReturn *FeatureShape::execute()
{
    Toolpath path;
    std::vector<App::DocumentObject*> links = Sources.getValues();
    if (links.empty()) {
        Path.setValue(path);
        return new App::DocumentObjectExecReturn("No shapes linked");
    }

    const Base::Vector3d &v = StartPoint.getValue();
    gp_Pnt pstart(v.x,v.y,v.z);

    std::list<TopoDS_Shape> shapes;
    for (std::vector<App::DocumentObject*>::iterator it = links.begin(); it != links.end(); ++it) {
        if (!(*it && (*it)->isDerivedFrom(Part::Feature::getClassTypeId())))
            continue;
        const TopoDS_Shape &shape = static_cast<Part::Feature*>(*it)->Shape.getShape().getShape();
        if (shape.IsNull())
            continue;
        shapes.push_back(shape);
    }

    Area::toPath(path,shapes,UseStartPoint.getValue()?&pstart:nullptr,nullptr,PARAM_PROP_ARGS(AREA_PARAMS_PATH));

    Path.setValue(path);
    return App::DocumentObject::StdReturn;
}

// Python Path Shape feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Path::FeatureShapePython, Path::FeatureShape)
template<> const char* Path::FeatureShapePython::getViewProviderName() const {
    return "PathGui::ViewProviderPathShape";
}
/// @endcond

// explicit template instantiation
template class PathExport FeaturePythonT<Path::FeatureShape>;
}



