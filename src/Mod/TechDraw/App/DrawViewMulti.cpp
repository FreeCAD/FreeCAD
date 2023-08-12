/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
# include <sstream>
# include <gp_Ax2.hxx>
# include <gp_Pnt.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Shape.hxx>
#endif

#include <Base/Console.h>

#include "GeometryObject.h"
#include "DrawUtil.h"
#include "DrawViewMulti.h"


using namespace TechDraw;

//===========================================================================
// DrawViewMulti
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewMulti, TechDraw::DrawViewPart)

DrawViewMulti::DrawViewMulti()
{
    static const char *group = "Projection";

    //properties that affect Geometry
    ADD_PROPERTY_TYPE(Sources ,(nullptr), group, App::Prop_None, "3D Shapes to view");
    Sources.setScope(App::LinkScope::Global);
    //Source is replaced by Sources in Multi
    Source.setStatus(App::Property::ReadOnly, true);
    Source.setStatus(App::Property::Hidden, true);

    geometryObject = nullptr;
}

DrawViewMulti::~DrawViewMulti()
{
}

short DrawViewMulti::mustExecute() const
{
    if (!isRestoring() && Sources.isTouched()) {
        return true;
    }

    return TechDraw::DrawViewPart::mustExecute();
}

void DrawViewMulti::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        //Base::Console().Message("TRACE - DVM::onChanged(%s) - %s\n", prop->getName(), Label.getValue());
        if (prop == &Sources) {
            const std::vector<App::DocumentObject*>& links = Sources.getValues();
            if (!links.empty()) {
                Source.setValue(links.front());
            }
        }
    }

    DrawViewPart::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewMulti::execute()
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    const std::vector<App::DocumentObject*>& links = Sources.getValues();
    if (links.empty())  {
        Base::Console().Log("INFO - DVM::execute - No Sources - creation?\n");
        return DrawView::execute();
    }

    m_compound = TopoDS::Compound(getSourceShape());
    if (m_compound.IsNull()) {
        return new App::DocumentObjectExecReturn("DVP - Linked shape object(s) is invalid");
    }
    TopoDS_Compound comp = m_compound;

    gp_Pnt inputCenter;
    try {
        inputCenter = ShapeUtils::findCentroid(comp,
                                                     Direction.getValue());
        shapeCentroid = Base::Vector3d(inputCenter.X(), inputCenter.Y(), inputCenter.Z());
        TopoDS_Shape mirroredShape = ShapeUtils::mirrorShape(comp,
                                                    inputCenter,
                                                    getScale());
        gp_Ax2 viewAxis = getViewAxis(Base::Vector3d(inputCenter.X(), inputCenter.Y(), inputCenter.Z()), Direction.getValue());
        if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
            mirroredShape = ShapeUtils::rotateShape(mirroredShape,
                                                          viewAxis,
                                                          Rotation.getValue());
        }
        geometryObject = buildGeometryObject(mirroredShape, viewAxis);

#if MOD_TECHDRAW_HANDLE_FACES
        extractFaces();
#endif //#if MOD_TECHDRAW_HANDLE_FACES
    }
    catch (Standard_Failure& e1) {
        return new App::DocumentObjectExecReturn(e1.GetMessageString());
    }

    requestPaint();
    return App::DocumentObject::StdReturn;
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewMultiPython, TechDraw::DrawViewMulti)
template<> const char* TechDraw::DrawViewMultiPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewMulti>;
}
