/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <Standard.hxx>
#endif

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Mod/Part/App/PartFeature.h>

#include "RayFeature.h"
#include "PovTools.h"


using namespace Raytracing;

PROPERTY_SOURCE(Raytracing::RayFeature, Raytracing::RaySegment)

//===========================================================================
// Feature
//===========================================================================

RayFeature::RayFeature(void)
{
    ADD_PROPERTY(Source,(0));
}

App::DocumentObjectExecReturn *RayFeature::execute(void)
{
    std::stringstream result;
    std::string ViewName = Label.getValue();

    App::DocumentObject* link = Source.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape()._Shape;
    std::string Name(std::string("Pov_")+static_cast<Part::Feature*>(link)->Label.getValue());
    if (shape.IsNull())
        return new App::DocumentObjectExecReturn("Linked shape object is empty");

    PovTools::writeShape(result,Name.c_str(),shape);

    // Apply the resulting fragment
    Result.setValue(result.str().c_str());

    return App::DocumentObject::StdReturn;
}
