/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <gp_Circ.hxx>
#include <Geom_Circle.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#endif  // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Tools2D.h>
#include <Base/Vector3D.h>

#include <App/Application.h>
#include <App/Material.h>

#include "DrawUtil.h"
#include "GeometryObject.h"
#include "Geometry.h"

#include "Cosmetic.h"

using namespace TechDraw;

LineFormat::LineFormat()
{
    m_style = getDefEdgeStyle();
    m_weight = getDefEdgeWidth();
    m_color = getDefEdgeColor();
    m_visible = true;
}

LineFormat::LineFormat(int style,
               double weight,
               App::Color color,
               bool visible ) :
    m_style(style),
    m_weight(weight),
    m_color(color),
    m_visible(visible)
{
}

void LineFormat::dump(char* title)
{
    Base::Console().Message("LF::dump - %s \n",title);
    Base::Console().Message("LF::dump - %s \n",toCSV().c_str());
}

std::string LineFormat::toCSV(void) const
{
    std::stringstream ss;
    ss << m_style << "," <<
          m_weight << "," <<
          m_color.asHexString() << "," <<
          m_visible;
    return ss.str();
}

bool LineFormat::fromCSV(std::string& lineSpec)
{
    unsigned int maxCells = 4;
    if (lineSpec.length() == 0) {
        Base::Console().Message( "LineFormat::fromCSV - lineSpec empty\n");
        return false;
    }
    std::vector<std::string> values = DrawUtil::split(lineSpec);
    if (values.size() < maxCells) {
        Base::Console().Message( "LineFormat::fromCSV(%s) invalid CSV entry\n",lineSpec.c_str() );
        return false;
    }
    m_style = atoi(values[0].c_str());
    m_weight= atof(values[1].c_str());
    m_color.fromHexString(values[2]);
    m_visible = atoi(values[3].c_str());
    return true;
}

//static preference getters.
double LineFormat::getDefEdgeWidth()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                    GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    std::string lgName = hGrp->GetASCII("LineGroup","FC 0.70mm");
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgName);

    double width = lg->getWeight("Graphic");
    delete lg; 
    return width;
}

App::Color LineFormat::getDefEdgeColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("NormalColor", 0x00000000));  //black
    return fcColor;
}

int LineFormat::getDefEdgeStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    int style = hGrp->GetInt("CosmoCLStyle", 2);   //dashed
    return style;
}

//****************************************************************************************

CosmeticVertex::CosmeticVertex() : TechDraw::Vertex()
{
    point(Base::Vector3d(0.0, 0.0, 0.0));
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("VertexColor", 0x00000000));

    linkGeom = -1;
    color = fcColor;
    size  = 3.0;
    style = 1;
    visible = true;
}

//CosmeticVertex::CosmeticVertex(Base::Vector3d loc) : TechDraw::Vertex(DrawUtil::invertY(loc))
CosmeticVertex::CosmeticVertex(Base::Vector3d loc) : TechDraw::Vertex(loc)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("VertexColor", 0xff000000));

    linkGeom = -1;
    color = fcColor;
    //TODO: size = hGrp->getFloat("VertexSize",30.0);
    size  = 30.0;
    style = 1;        //TODO: implement styled vertexes
    visible = true;
}

std::string CosmeticVertex::toCSV(void) const
{
    std::stringstream ss;
    ss << point().x << "," <<
          point().y << "," <<
          point().z << "," <<

          linkGeom << "," << 
          color.asHexString() << ","  <<
          size << "," <<
          style << ","  <<
          visible << 
          std::endl;
    return ss.str();
}

bool CosmeticVertex::fromCSV(std::string& lineSpec)
{
    unsigned int maxCells = 8;
    if (lineSpec.length() == 0) {
        Base::Console().Message( "CosmeticVertex::fromCSV - lineSpec empty\n");
        return false;
    }
    std::vector<std::string> values = DrawUtil::split(lineSpec);
    if (values.size() < maxCells) {
        Base::Console().Message( "CosmeticVertex::fromCSV(%s) invalid CSV entry\n",lineSpec.c_str() );
        return false;
    }
    double x = atof(values[0].c_str());
    double y = atof(values[1].c_str());
    double z = atof(values[2].c_str());
    point(Base::Vector3d (x,y,z));
    linkGeom = atoi(values[3].c_str());
    color.fromHexString(values[4]);
    size = atof(values[5].c_str());
    style = atoi(values[6].c_str());
    visible = atoi(values[7].c_str());
    return true;
}

void CosmeticVertex::dump(char* title)
{
    Base::Console().Message("CV::dump - %s \n",title);
    Base::Console().Message("CV::dump - %s \n",toCSV().c_str());
}

//******************************************

//note this ctor has no occEdge or first/last point for geometry!
CosmeticEdge::CosmeticEdge()
{
    m_geometry = new TechDraw::BaseGeom();
    m_geometry->geomType = GENERIC;
    m_geometry->classOfEdge = ecHARD;
    m_geometry->visible = true;
    m_geometry->cosmetic = true;

    m_linkGeom = -1;
    m_format = LineFormat();
}

//CosmeticEdge::CosmeticEdge(Base::Vector3d pt1, Base::Vector3d pt2)
CosmeticEdge::CosmeticEdge(Base::Vector3d pt1, Base::Vector3d pt2)
{
//    Base:: Console().Message("CE::CE(%s, %s) \n",
//                             DrawUtil::formatVector(p1).c_str(),
//                             DrawUtil::formatVector(p2).c_str());
    Base::Vector3d p1 = DrawUtil::invertY(pt1);
    Base::Vector3d p2 = DrawUtil::invertY(pt2);
    gp_Pnt gp1(p1.x,p1.y,p1.z);
    gp_Pnt gp2(p2.x,p2.y,p2.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
    m_geometry = TechDraw::BaseGeom::baseFactory(e);
    m_geometry->classOfEdge = ecHARD;
    m_geometry->visible = true;
    m_geometry->cosmetic = true;

    if (m_geometry->geomType == TechDraw::GeomType::GENERIC) {
        //
    } else if (m_geometry->geomType == TechDraw::GeomType::CIRCLE) {
        //
    }
    m_linkGeom = -1;
    m_format = LineFormat();
}

//CosmeticEdge::CosmeticEdge(TopoDS_Edge e, Base::Vector3d mirrorPoint)
CosmeticEdge::CosmeticEdge(TopoDS_Edge e)
{
//    Base:: Console().Message("CE::CE(occEdge) \n");
    m_geometry = TechDraw::BaseGeom::baseFactory(e);
    m_geometry->classOfEdge = ecHARD;
    m_geometry->visible = true;
    m_geometry->cosmetic = true;

    m_linkGeom = -1;
    m_format = LineFormat();
}

CosmeticEdge::CosmeticEdge(TechDraw::BaseGeom* g)
{
//    Base:: Console().Message("CE::CE(Base::Geom) \n");
    m_geometry = g;
    m_geometry->classOfEdge = ecHARD;
    m_geometry->visible = true;
    m_geometry->cosmetic = true;
    m_linkGeom = -1;
    m_format = LineFormat();
}

TechDraw::BaseGeom* CosmeticEdge::scaledGeometry(double scale)
{
//    Base::Console().Message("CE::getScaledGeometry(%.3f)\n",scale);
    TechDraw::BaseGeom* newGeom = nullptr;
    TopoDS_Edge e = m_geometry->occEdge;
    TopoDS_Shape s = TechDraw::scaleShape(e, scale);
    TopoDS_Edge newEdge = TopoDS::Edge(s);
    newGeom = TechDraw::BaseGeom::baseFactory(newEdge);
    newGeom->classOfEdge = ecHARD;
    newGeom->visible = true;
    newGeom->cosmetic = true;
    return newGeom;
}

std::string CosmeticEdge::toCSV(void) const
{
//    Base::Console().Message( "CosmeticEdge::toCSV()\n");
    std::stringstream ss;
    if (m_geometry != nullptr) {
        ss << m_geometry->geomType << "," <<
            m_linkGeom <<
            ",$$$," <<
            m_geometry->toCSV() <<
            ",$$$," <<
            m_format.toCSV() <<
            std::endl;
    }
    return ss.str();
}

bool CosmeticEdge::fromCSV(std::string& lineSpec)
{
//    Base::Console().Message( "CosmeticEdge::fromCSV() - lineSpec: %s\n", lineSpec.c_str());
    std::vector<std::string> tokens = DrawUtil::tokenize(lineSpec);
    if (tokens.empty()) {
        Base::Console().Message("CosmeticEdge::fromCSV - tokenize failed - no tokens\n");
        return false;
    }

    if (tokens[0].length() == 0) {
        Base::Console().Message( "CosmeticEdge::fromCSV - token0 empty\n");
        return false;
    }
    
    std::vector<std::string> values = DrawUtil::split(tokens[0]);
    unsigned int maxCells = 2;
    if (values.size() < maxCells) {
        Base::Console().Message( "CosmeticEdge::fromCSV(%s) invalid CSV entry\n",lineSpec.c_str() );
        return false;
    }
    int geomType = atoi(values[0].c_str());
    m_linkGeom = atoi(values[1].c_str());

    int lastToken = 0;
    if (geomType == TechDraw::GeomType::GENERIC) {
        if (tokens.size() != 4) {
            Base::Console().Message("CE::fromCSV - wrong number of tokens\n");
            return false;
        }
        TechDraw::Generic* tempGeom = new TechDraw::Generic();
        tempGeom->fromCSV(tokens[1] + ",$$$," + tokens[2]);
        lastToken = 3;
        m_geometry = tempGeom;
        m_geometry->occEdge = GeometryUtils::edgeFromGeneric(tempGeom);
    } else if (geomType == TechDraw::GeomType::CIRCLE) {
        if (tokens.size() != 4) {
            Base::Console().Message("CE::fromCSV - wrong number of tokens\n");
            return false;
        }
        TechDraw::Circle* tempGeom = new TechDraw::Circle();
        tempGeom->fromCSV(tokens[1] + ",$$$," + tokens[2]);
        lastToken = 3;
        m_geometry = tempGeom;
        m_geometry->occEdge = GeometryUtils::edgeFromCircle(tempGeom);
    } else if (geomType == TechDraw::GeomType::ARCOFCIRCLE) {
        if (tokens.size() != 5) {
            Base::Console().Message("CE::fromCSV - wrong number of tokens\n");
            return false;
        }
        TechDraw::AOC* tempGeom = new TechDraw::AOC();
        tempGeom->fromCSV(tokens[1] + ",$$$," + tokens[2] + ",$$$," + tokens[3]);
        lastToken = 4;
        m_geometry = tempGeom;
        m_geometry->occEdge = GeometryUtils::edgeFromCircleArc(tempGeom);
    } else {
        Base::Console().Message("Cosmetic::fromCSV - unimplemented geomType: %d\n", geomType);
        return false;
    }

    m_format.fromCSV(tokens[lastToken]);

    m_geometry->classOfEdge = ecHARD;
    m_geometry->visible = true;
    m_geometry->cosmetic = true;
    return true;
}

//duplicate of CV routine.  make static? or base class?
void CosmeticEdge::dump(char* title)
{
    Base::Console().Message("CE::dump - %s \n",title);
    Base::Console().Message("CE::dump - %s \n",toCSV().c_str());
}


