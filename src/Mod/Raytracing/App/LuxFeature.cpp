/***************************************************************************
 *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
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

#include "LuxFeature.h"
#include "LuxTools.h"

#include <ShapeAnalysis_ShapeContents.hxx>


using namespace Raytracing;
using namespace std;

PROPERTY_SOURCE(Raytracing::LuxFeature, Raytracing::RaySegment)

//===========================================================================
// Feature
//===========================================================================

LuxFeature::LuxFeature(void)
{
    ADD_PROPERTY(Source,(nullptr));
    ADD_PROPERTY(Color,(App::Color(0.5f,0.5f,0.5f)));
    ADD_PROPERTY(Transparency,(0));
}

short LuxFeature::mustExecute() const
{
    if (Source.isTouched())
        return 1;
    if (Color.isTouched())
        return 1;
    if (Transparency.isTouched())
        return 1;
    return RaySegment::mustExecute();
}

App::DocumentObjectExecReturn *LuxFeature::execute(void)
{
    std::stringstream result;
    std::string ViewName = getNameInDocument();

    App::DocumentObject* link = Source.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape().getShape();
    std::string Name(std::string("Lux_")+static_cast<Part::Feature*>(link)->getNameInDocument());
    if (shape.IsNull())
        return new App::DocumentObjectExecReturn("Linked shape object is empty");
    ShapeAnalysis_ShapeContents test;
    test.Clear();
    test.Perform(shape);
    if (test.NbFaces() < 1)
        return new App::DocumentObjectExecReturn("Shape contains no face to render");

    // write a material entry
    // This must not be done in LuxTools::writeShape!
    const App::Color& c = Color.getValue();
    long t = Transparency.getValue();
    if (t == 0) {
        result << "MakeNamedMaterial \"FreeCADMaterial_" << Name << "\"" << endl
               << "    \"color Kd\" [" << c.r << " " << c.g << " " << c.b << "]" << endl
               << "    \"float sigma\" [0.000000000000000]" << endl
               << "    \"string type\" [\"matte\"]" << endl << endl;
    } else {
        float trans = t/100.0f;
        result << "MakeNamedMaterial \"FreeCADMaterial_Base_" << Name << "\"" << endl
               << "    \"color Kd\" [" << c.r << " " << c.g << " " << c.b << "]" << endl
               << "    \"float sigma\" [0.000000000000000]" << endl
               << "    \"string type\" [\"matte\"]" << endl << endl
               << "MakeNamedMaterial \"FreeCADMaterial_Null_" << Name << "\"" << endl
               << "    \"string type\" [\"null\"]" << endl << endl
               << "MakeNamedMaterial \"FreeCADMaterial_" << Name << "\"" << endl
               << "    \"string namedmaterial1\" [\"FreeCADMaterial_Null_" << Name << "\"]" << endl
               << "    \"string namedmaterial2\" [\"FreeCADMaterial_Base_" << Name << "\"]" << endl
               << "    \"float amount\" [" << trans << "]" << endl
               << "    \"string type\" [\"mix\"]" << endl << endl;
    }
    
    LuxTools::writeShape(result,Name.c_str(),shape);
    
    // Apply the resulting fragment
    Result.setValue(result.str().c_str());

    return App::DocumentObject::StdReturn;
}
