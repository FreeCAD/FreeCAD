/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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
#endif

#include "FeaturePathShape.h"
#include "Command.h"

#include <App/DocumentObjectPy.h>
#include <Base/Placement.h>
#include <Mod/Part/App/TopoShape.h>

#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Lin.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_CompCurve.hxx>
#include <Handle_BRepAdaptor_HCompCurve.hxx>
#include <Approx_Curve3d.hxx>
#include <BRepAdaptor_HCurve.hxx>
#include <Handle_BRepAdaptor_HCurve.hxx>


using namespace Path;

PROPERTY_SOURCE(Path::FeatureShape, Path::Feature)


FeatureShape::FeatureShape()
{
    ADD_PROPERTY_TYPE(Shape,(TopoDS_Shape()),"Path",App::Prop_None,"The shape data of this feature");
}

FeatureShape::~FeatureShape()
{
}

short FeatureShape::mustExecute(void) const
{
    return Path::Feature::mustExecute();
}

App::DocumentObjectExecReturn *FeatureShape::execute(void)
{
    TopoDS_Shape shape = Shape.getValue();
    if (!shape.IsNull()) {
        if (shape.ShapeType() == TopAbs_WIRE) {
            Path::Toolpath result;
            bool first = true;
            Base::Placement last;
            
            TopExp_Explorer ExpEdges (shape,TopAbs_EDGE);
            while (ExpEdges.More()) {
                const TopoDS_Edge& edge = TopoDS::Edge(ExpEdges.Current());
                TopExp_Explorer ExpVerts(edge,TopAbs_VERTEX);
                bool vfirst = true;
                while (ExpVerts.More()) {
                    const TopoDS_Vertex& vert = TopoDS::Vertex(ExpVerts.Current());
                    gp_Pnt pnt = BRep_Tool::Pnt(vert);
                    Base::Placement tpl;
                    tpl.setPosition(Base::Vector3d(pnt.X(),pnt.Y(),pnt.Z()));
                    if (first) {
                        // add first point as a G0 move
                        Path::Command cmd;
                        std::ostringstream ctxt;
                        ctxt << "G0 X" << tpl.getPosition().x << " Y" << tpl.getPosition().y << " Z" << tpl.getPosition().z;
                        cmd.setFromGCode(ctxt.str());
                        result.addCommand(cmd);
                        first = false;
                        vfirst = false;
                    } else {
                        if (vfirst)
                            vfirst = false;
                        else {
                            Path::Command cmd;
                            cmd.setFromPlacement(tpl);
                
                            // write arc data if needed
                            BRepAdaptor_Curve adapt(edge);
                            if (adapt.GetType() == GeomAbs_Circle) {
                                gp_Circ circ = adapt.Circle();
                                gp_Pnt c = circ.Location();
                                bool clockwise = false;
                                gp_Dir n = circ.Axis().Direction();
                                if (n.Z() < 0)
                                    clockwise = true;
                                Base::Vector3d center = Base::Vector3d(c.X(),c.Y(),c.Z());
                                // center coords must be relative to last point
                                center -= last.getPosition();
                                cmd.setCenter(center,clockwise);
                            }
                            result.addCommand(cmd);
                        }
                    }
                    ExpVerts.Next();
                    last = tpl;
                }
                ExpEdges.Next();
            }
            
            Path.setValue(result);
        }
    }
    return App::DocumentObject::StdReturn;
}

// Python Path Shape feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Path::FeatureShapePython, Path::FeatureShape)
template<> const char* Path::FeatureShapePython::getViewProviderName(void) const {
    return "PathGui::ViewProviderPathShape";
}
/// @endcond

// explicit template instantiation
template class PathExport FeaturePythonT<Path::FeatureShape>;
}



