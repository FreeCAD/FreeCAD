/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2015              *
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

#include "DraftDxf.h"

#include <gp_Pnt.hxx>
#include <gp_Circ.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Elips.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>

#include <Base/Parameter.h>
#include <Base/Matrix.h>
#include <Base/Vector3D.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Annotation.h>
#include <Mod/Part/App/PartFeature.h>

using namespace DraftUtils;

DraftDxfRead::DraftDxfRead(std::string filepath, App::Document *pcDoc) : CDxfRead(filepath.c_str())
{
    document = pcDoc;
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Draft");
    optionGroupLayers = hGrp->GetBool("groupLayers",false);
    optionImportAnnotations = hGrp->GetBool("dxftext",false);
}


void DraftDxfRead::OnReadLine(const double* s, const double* e, bool hidden)
{
    gp_Pnt p0(s[0], s[1], s[2]);
    gp_Pnt p1(e[0], e[1], e[2]);
    if (p0.IsEqual(p1,0.00000001))
        return;
    BRepBuilderAPI_MakeEdge makeEdge(p0, p1);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void DraftDxfRead::OnReadPoint(const double* s)
{
    BRepBuilderAPI_MakeVertex makeVertex(gp_Pnt(s[0], s[1], s[2]));
    TopoDS_Vertex vertex = makeVertex.Vertex();
    AddObject(new Part::TopoShape(vertex));
}


void DraftDxfRead::OnReadArc(const double* s, const double* e, const double* c, bool dir, bool hidden)
{
    gp_Pnt p0(s[0], s[1], s[2]);
    gp_Pnt p1(e[0], e[1], e[2]);
    gp_Dir up(0, 0, 1);
    if (!dir)
        up = -up;
    gp_Pnt pc(c[0], c[1], c[2]);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    BRepBuilderAPI_MakeEdge makeEdge(circle, p0, p1);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void DraftDxfRead::OnReadCircle(const double* s, const double* c, bool dir, bool hidden)
{
    gp_Pnt p0(s[0], s[1], s[2]);
    gp_Dir up(0, 0, 1);
    if (!dir)
        up = -up;
    gp_Pnt pc(c[0], c[1], c[2]);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    BRepBuilderAPI_MakeEdge makeEdge(circle);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void DraftDxfRead::OnReadSpline(struct SplineData& sd)
{
    // not yet implemented
}


void DraftDxfRead::OnReadEllipse(const double* c, double major_radius, double minor_radius, double rotation, double start_angle, double end_angle, bool dir)
{
    gp_Dir up(0, 0, 1);
    if(!dir)
        up = -up;
    gp_Pnt pc(c[0], c[1], c[2]);
    gp_Elips ellipse(gp_Ax2(pc, up), major_radius, minor_radius);
    ellipse.Rotate(gp_Ax1(pc,up),rotation);
    BRepBuilderAPI_MakeEdge makeEdge(ellipse);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void DraftDxfRead::OnReadText(const double *point, const double height, const char* text)
{
    if (optionImportAnnotations) {
        Base::Vector3d pt(point[0],point[1],point[2]);
        if(LayerName().substr(0, 6) != "BLOCKS") {
            App::Annotation *pcFeature = (App::Annotation *)document->addObject("App::Annotation", "Text");
            pcFeature->LabelText.setValue(Deformat(text));
            pcFeature->Position.setValue(pt);
        }
        //else std::cout << "skipped text in block: " << LayerName() << std::endl;
    }
}


void DraftDxfRead::OnReadInsert(const double* point, const double* scale, const char* name, double rotation)
{
    //std::cout << "Inserting block " << name << " rotation " << rotation << " pos " << point[0] << "," << point[1] << "," << point[2] << " scale " << scale[0] << "," << scale[1] << "," << scale[2] << std::endl;
    for(std::map<std::string,std::vector<Part::TopoShape*> > ::const_iterator i = layers.begin(); i != layers.end(); ++i) {
        std::string k = i->first;
        std::string prefix = "BLOCKS ";
        prefix += name;
        prefix += " ";
        if(k.substr(0, prefix.size()) == prefix) {
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            std::vector<Part::TopoShape*> v = i->second;
            for(std::vector<Part::TopoShape*>::const_iterator j = v.begin(); j != v.end(); ++j) { 
                const TopoDS_Shape& sh = (*j)->_Shape;
                if (!sh.IsNull())
                    builder.Add(comp, sh);
            }
            if (!comp.IsNull()) {
                Part::TopoShape* pcomp = new Part::TopoShape(comp);
                Base::Matrix4D mat;
                mat.scale(scale[0],scale[1],scale[2]);
                mat.rotZ(rotation);
                mat.move(point[0],point[1],point[2]);
                pcomp->transformShape(mat,true);
                AddObject(pcomp);
            }
        }
    } 
}


void DraftDxfRead::OnReadDimension(const double* s, const double* e, const double* point, double rotation)
{
    if (optionImportAnnotations) {
        Base::Interpreter().runString("import Draft");
        Base::Interpreter().runStringArg("p1=FreeCAD.Vector(%f,%f,%f)",s[0],s[1],s[2]);
        Base::Interpreter().runStringArg("p2=FreeCAD.Vector(%f,%f,%f)",e[0],e[1],e[2]);
        Base::Interpreter().runStringArg("p3=FreeCAD.Vector(%f,%f,%f)",point[0],point[1],point[2]);
        Base::Interpreter().runString("Draft.makeDimension(p1,p2,p3)");
    }
}


void DraftDxfRead::AddObject(Part::TopoShape *shape)
{
    //std::cout << "layer:" << LayerName() << std::endl;
    std::vector <Part::TopoShape*> vec;
    if (layers.count(LayerName()))
        vec = layers[LayerName()];
    vec.push_back(shape);
    layers[LayerName()] = vec;
    if (!optionGroupLayers) {
        Part::Feature *pcFeature = (Part::Feature *)document->addObject("Part::Feature", "Shape");
        pcFeature->Shape.setValue(*shape);
    }
}


std::string DraftDxfRead::Deformat(const char* text)
{
    // this function removes DXF formatting from texts
    std::stringstream ss;
    bool escape = false; // turned on when finding an escape character
    bool longescape = false; // turned on for certain escape codes that expect additional chars
    for(unsigned int i = 0; i<strlen(text); i++) {
        if (text[i] == '\\')
            escape = true;
        else if (escape) {
            if (longescape) {
                if (text[i] == ';') {
                    escape = false;
                    longescape = false;
                }
            } else {
                if ( (text[i] == 'H') || (text[i] == 'h') ||
                     (text[i] == 'Q') || (text[i] == 'q') ||
                     (text[i] == 'W') || (text[i] == 'w') ||
                     (text[i] == 'F') || (text[i] == 'f') ||
                     (text[i] == 'A') || (text[i] == 'a') ||
                     (text[i] == 'C') || (text[i] == 'c') ||
                     (text[i] == 'T') || (text[i] == 't') )
                    longescape = true;
                else {
                    if ( (text[i] == 'P') || (text[i] == 'p') )
                        ss << "\n";
                    escape = false;
                }
            }
        }
        else if ( (text[i] != '{') && (text[i] != '}') ) {
            ss << text[i];
        }
    }
    return ss.str();
}


void DraftDxfRead::AddGraphics() const
{
    if (optionGroupLayers) {
        for(std::map<std::string,std::vector<Part::TopoShape*> > ::const_iterator i = layers.begin(); i != layers.end(); ++i) {
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            std::string k = i->first;
            std::vector<Part::TopoShape*> v = i->second;
            if(k.substr(0, 6) != "BLOCKS") {
                for(std::vector<Part::TopoShape*>::const_iterator j = v.begin(); j != v.end(); ++j) { 
                    const TopoDS_Shape& sh = (*j)->_Shape;
                    if (!sh.IsNull())
                        builder.Add(comp, sh);
                }
                if (!comp.IsNull()) {
                    Part::Feature *pcFeature = (Part::Feature *)document->addObject("Part::Feature", k.c_str());
                    pcFeature->Shape.setValue(comp);
                } 
            }
        }
    }
}


