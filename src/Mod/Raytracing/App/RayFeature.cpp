/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
using namespace std;

PROPERTY_SOURCE(Raytracing::RayFeature, Raytracing::RaySegment)

//===========================================================================
// Feature
//===========================================================================

RayFeature::RayFeature(void)
{
    ADD_PROPERTY(Source,(nullptr));
    ADD_PROPERTY(Color,(App::Color(0.5f,0.5f,0.5f)));
    ADD_PROPERTY(Transparency,(0));
}

short RayFeature::mustExecute() const
{
    if (Source.isTouched())
        return 1;
    if (Color.isTouched())
        return 1;
    if (Transparency.isTouched())
        return 1;
    return RaySegment::mustExecute();
}

App::DocumentObjectExecReturn *RayFeature::execute(void)
{
    std::stringstream result;
    std::string ViewName = getNameInDocument();

    App::DocumentObject* link = Source.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape().getShape();
    std::string Name(std::string("Pov_")+static_cast<Part::Feature*>(link)->getNameInDocument());
    if (shape.IsNull())
        return new App::DocumentObjectExecReturn("Linked shape object is empty");

    PovTools::writeShape(result,Name.c_str(),shape);

    // This must not be done in PovTools::writeShape!
    long t = Transparency.getValue();
    const App::Color& c = Color.getValue();
    result << "// instance to render" << endl
           << "object {" << Name << endl
           << " texture {" << endl;
    if (t == 0) {
        result << "      pigment {color rgb <"<<c.r<<","<<c.g<<","<<c.b<<">}" << endl;
    }
    else {
        float trans = t/100.0f;
        result << "      pigment {color rgb <"<<c.r<<","<<c.g<<","<<c.b<<"> transmit "<<trans<<"}" << endl;
    }
    result << "      finish {StdFinish } //definition on top of the project" << endl
           << "  }" << endl
           << "}" << endl   ;

    // Apply the resulting fragment
    Result.setValue(result.str().c_str());

    return App::DocumentObject::StdReturn;
}
