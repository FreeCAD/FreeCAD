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
#include <cmath>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <gp_Circ.hxx>
#include <Geom_Circle.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#endif  // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>

#include <App/Application.h>
#include <App/Material.h>

#include "DrawUtil.h"
#include "GeometryObject.h"
#include "Geometry.h"
#include "DrawViewPart.h"

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
          visible;
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
    init();
}

CosmeticEdge::CosmeticEdge(Base::Vector3d pt1, Base::Vector3d pt2)
{
    Base::Vector3d p1 = DrawUtil::invertY(pt1);
    Base::Vector3d p2 = DrawUtil::invertY(pt2);
    gp_Pnt gp1(p1.x,p1.y,p1.z);
    gp_Pnt gp2(p2.x,p2.y,p2.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
    m_geometry = TechDraw::BaseGeom::baseFactory(e);
    init();
}

CosmeticEdge::CosmeticEdge(TopoDS_Edge e)
{
    m_geometry = TechDraw::BaseGeom::baseFactory(e);
    init();
}

CosmeticEdge::CosmeticEdge(TechDraw::BaseGeom* g)
{
    m_geometry = g;
    init();
}

CosmeticEdge::~CosmeticEdge(void)
{
    delete m_geometry;
}

void CosmeticEdge::init(void)
{
    m_geometry->classOfEdge = ecHARD;
    m_geometry->visible = true;
    m_geometry->cosmetic = true;
}

TechDraw::BaseGeom* CosmeticEdge::scaledGeometry(double scale)
{
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
    std::stringstream ss;
    if (m_geometry != nullptr) {
        ss << m_geometry->geomType << 
            ",$$$," <<
            m_geometry->toCSV() <<
            ",$$$," <<
            m_format.toCSV();
    }
    return ss.str();
}

bool CosmeticEdge::fromCSV(std::string& lineSpec)
{
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
    unsigned int maxCells = 1;
    if (values.size() < maxCells) {
        Base::Console().Message( "CosmeticEdge::fromCSV(%s) invalid CSV entry\n",lineSpec.c_str() );
        return false;
    }
    int geomType = atoi(values[0].c_str());

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

void CosmeticEdge::replaceGeometry(TechDraw::BaseGeom* g)
{
    delete m_geometry;
    m_geometry = g;
}

void CosmeticEdge::dump(char* title)
{
    Base::Console().Message("CE::dump - %s \n",title);
    Base::Console().Message("CE::dump - %s \n",toCSV().c_str());
}

//*********************************************************
CenterLine::CenterLine(void)
{
    m_start = Base::Vector3d(0.0, 0.0, 0.0);
    m_end = Base::Vector3d(0.0, 0.0, 0.0);
    m_mode = 0;
    m_hShift = 0.0;
    m_vShift = 0.0;
    m_rotate = 0.0;
    m_extendBy = 0.0;
}

CenterLine::CenterLine(Base::Vector3d p1, Base::Vector3d p2)
{
    m_start = p1;
    m_end = p2;
    m_mode = 0;
    m_hShift = 0.0;
    m_vShift = 0.0;
    m_rotate = 0.0;
    m_extendBy = 0.0;
}

CenterLine::CenterLine(Base::Vector3d p1, Base::Vector3d p2,
                       int m, 
                       double h,
                       double v,
                       double r,
                       double x)
{
    m_start = p1;
    m_end = p2;
    m_mode = m;
    m_hShift = h;
    m_vShift = v;
    m_rotate = r;
    m_extendBy = x;
}

CenterLine::~CenterLine()
{
}

TechDraw::BaseGeom* CenterLine::scaledGeometry(TechDraw::DrawViewPart* partFeat)
{
    double scale = partFeat->getScale();
    if (m_faces.empty() ) {
        Base::Console().Message("CL::scaledGeometry - no Faces!\n");
        return nullptr;
    }
    
    std::pair<Base::Vector3d, Base::Vector3d> ends = 
                             calcEndPoints(partFeat,
                                          m_faces,
                                          m_mode, m_extendBy,
                                          m_hShift,m_vShift, m_rotate);
    TechDraw::BaseGeom* newGeom = nullptr;
    Base::Vector3d p1 = DrawUtil::invertY(ends.first);
    Base::Vector3d p2 = DrawUtil::invertY(ends.second);
    gp_Pnt gp1(p1.x,p1.y,p1.z);
    gp_Pnt gp2(p2.x,p2.y,p2.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
    TopoDS_Shape s = TechDraw::scaleShape(e, scale);
    TopoDS_Edge newEdge = TopoDS::Edge(s);
    newGeom = TechDraw::BaseGeom::baseFactory(newEdge);
    newGeom->classOfEdge = ecHARD;
    newGeom->visible = true;
    newGeom->cosmetic = true;
    return newGeom;
}

std::string CenterLine::toCSV(void) const
{
    std::stringstream ss;
    ss << m_start.x << "," <<   //0
          m_start.y << "," <<   //1
          m_start.z << "," <<   //2
          m_end.x << "," <<     //3
          m_end.y << "," <<     //4
          m_end.z << "," <<     //5
          m_mode << "," <<      //6
          m_hShift << "," <<    //7
          m_vShift << "," <<    //8
          m_rotate << "," <<    //9
          m_extendBy << "," <<  //10
          m_faces.size();     //11
    if (!m_faces.empty()) {
        for (auto& f: m_faces) {
            if (!f.empty()) {
                ss << "," << f ;
            }
        }
    }

    std::string clCSV = ss.str();
    std::string fmtCSV = m_format.toCSV();
    return clCSV + ",$$$," + fmtCSV;
}

bool CenterLine::fromCSV(std::string& lineSpec)
{
    if (lineSpec.length() == 0) {
        Base::Console().Message( "CenterLine::fromCSV - lineSpec empty\n");
        return false;
    }
    
    std::vector<std::string> tokens = DrawUtil::tokenize(lineSpec);
    if (tokens.size() != 2) {
        Base::Console().Message("CenterLine::fromCSV - tokenize failed - size: %d\n",tokens.size());
    }

    if (tokens[0].length() == 0) {
        Base::Console().Message( "CenterLine::fromCSV - token0 empty\n");
        return false;
    }

    std::vector<std::string> values = DrawUtil::split(tokens[0]);

// variable length record, can't determine maxCells in advance.
    double x = atof(values[0].c_str());
    double y = atof(values[1].c_str());
    double z = atof(values[2].c_str());
    m_start = Base::Vector3d (x,y,z);
    x = atof(values[3].c_str());
    y = atof(values[4].c_str());
    z = atof(values[5].c_str());
    m_end = Base::Vector3d (x,y,z);
    m_mode = atoi(values[6].c_str());
    m_hShift = atof(values[7].c_str());
    m_vShift = atof(values[8].c_str());
    m_rotate = atof(values[9].c_str());
    m_extendBy = atof(values[10].c_str());
    int m_faceCount = atoi(values[11].c_str());
    int i = 0;
    for ( ; i < m_faceCount; i++ ) {
        m_faces.push_back(values[12 + i]);
    }
    m_format.fromCSV(tokens[1]);
    return true;
}

void CenterLine::dump(char* title)
{
    Base::Console().Message("CL::dump - %s \n",title);
    Base::Console().Message("CL::dump - %s \n",toCSV().c_str());
}

std::pair<Base::Vector3d, Base::Vector3d> CenterLine::calcEndPoints(DrawViewPart* partFeat,
                                                      std::vector<std::string> faceNames, 
                                                      int vert, double ext,
                                                      double m_hShift, double m_vShift,
                                                      double rotate)
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    if (faceNames.empty()) {
        Base::Console().Message("CL::calcEndPoints - no faces!\n");
        return result;
    }

    Bnd_Box faceBox;
    faceBox.SetGap(0.0);

    double scale = partFeat->getScale();
    double hss = m_hShift * scale;
    double vss = m_vShift * scale;

    for (auto& fn: faceNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(fn) != "Face") {
            continue;
        }
        int idx = TechDraw::DrawUtil::getIndexFromName(fn);
        std::vector<TechDraw::BaseGeom*> faceEdges = 
                                                partFeat->getFaceEdgesByIndex(idx);
        for (auto& fe: faceEdges) {
            if (!fe->cosmetic) {
                BRepBndLib::Add(fe->occEdge, faceBox);
            }
        }
    }

    double Xmin,Ymin,Zmin,Xmax,Ymax,Zmax;
    faceBox.Get(Xmin,Ymin,Zmin,Xmax,Ymax,Zmax);

    double Xspan = fabs(Xmax - Xmin);
//    Xspan = (Xspan / 2.0) + (ext * scale);    //this should be right? edges in GO are scaled!
//    Xspan = (Xspan / 2.0) + ext;
    Xspan = (Xspan / 2.0);
    double Xmid = Xmin + fabs(Xmax - Xmin) / 2.0;

    double Yspan = fabs(Ymax - Ymin);
//    Yspan = (Yspan / 2.0) + (ext * scale);
//    Yspan = (Yspan / 2.0) + ext;
    Yspan = (Yspan / 2.0);
    double Ymid = Ymin + fabs(Ymax - Ymin) / 2.0;

    Base::Vector3d p1, p2;
    if (vert == 0) {                    //vertical
        Base::Vector3d top(Xmid + hss, (Ymid - Yspan - ext) + vss, 0.0);
        Base::Vector3d bottom(Xmid + hss, (Ymid + Yspan + ext) + vss, 0.0);
        p1 = top;
        p2 = bottom;
    } else if (vert == 1) {            //horizontal
        Base::Vector3d left((Xmid - Xspan - ext) + hss, Ymid + vss, 0.0);
        Base::Vector3d right((Xmid + Xspan + ext) + hss, Ymid + vss, 0.0);
        p1 = left;
        p2 = right;
    } else {      //vert == 2 //aligned
        Base::Console().Message("CL::calcEndPoints - aligned is not implemented yet\n");
        Base::Vector3d top(Xmid + hss, (Ymid - Yspan - ext) + vss, 0.0);
        Base::Vector3d bottom(Xmid + hss, (Ymid + Yspan + ext) + vss, 0.0);
        p1 = top;
        p2 = bottom;
    }
    
    Base::Vector3d bbxCenter(Xmid, Ymid, 0.0);
    if (!DrawUtil::fpCompare(rotate, 0.0)) {
        //rotate p1, p2 about bbxCenter
        double cosTheta = cos(rotate * M_PI / 180.0);
        double sinTheta = sin(rotate * M_PI / 180.0);
        Base::Vector3d toOrg = p1 - bbxCenter;
        double xRot = toOrg.x * cosTheta - toOrg.y * sinTheta;
        double yRot = toOrg.y * cosTheta + toOrg.x * sinTheta;
        p1 = Base::Vector3d(xRot, yRot, 0.0) + bbxCenter;
        toOrg = p2 - bbxCenter;
        xRot = toOrg.x * cosTheta - toOrg.y * sinTheta;
        yRot = toOrg.y * cosTheta + toOrg.x * sinTheta;
        p2 = Base::Vector3d(xRot, yRot, 0.0) + bbxCenter;
    }

    result.first = p1 / scale;
    result.second = p2 / scale;
    return result;
}

GeomFormat::GeomFormat() :
    m_geomIndex(-1)
{
    m_format.m_style = LineFormat::getDefEdgeStyle();
    m_format.m_weight = LineFormat::getDefEdgeWidth();
    m_format.m_color = LineFormat::getDefEdgeColor();
    m_format.m_visible = true;
}

GeomFormat::GeomFormat(int idx,
                       TechDraw::LineFormat fmt) :
    m_geomIndex(idx)
{
    m_format.m_style = fmt.m_style;
    m_format.m_weight = fmt.m_weight;
    m_format.m_color = fmt.m_color;
    m_format.m_visible = fmt.m_visible;
    //m_format = fmt;  //???
}

GeomFormat::~GeomFormat()
{
}

void GeomFormat::dump(char* title)
{
    Base::Console().Message("GF::dump - %s \n",title);
    Base::Console().Message("GF::dump - %s \n",toCSV().c_str());
}

std::string GeomFormat::toCSV(void) const
{
    std::stringstream ss;
    ss << m_geomIndex << ",$$$," <<
          m_format.toCSV();
    return ss.str();
}

bool GeomFormat::fromCSV(std::string& lineSpec)
{
    std::vector<std::string> tokens = DrawUtil::tokenize(lineSpec);
    if (tokens.empty()) {
        Base::Console().Message("GeomFormat::fromCSV - tokenize failed - no tokens\n");
        return false;
    }

    if (tokens[0].length() == 0) {
        Base::Console().Message( "GeomFormat::fromCSV - token0 empty\n");
        return false;
    }

    std::vector<std::string> values = DrawUtil::split(tokens[0]);
    unsigned int maxCells = 1;
    if (values.size() < maxCells) {
        Base::Console().Message( "GeomFormat::fromCSV(%s) invalid CSV entry\n",lineSpec.c_str() );
        return false;
    }
    m_geomIndex = atoi(values[0].c_str());

    int lastToken = 1;
    if (tokens.size() != 2) {
        Base::Console().Message("CE::fromCSV - wrong number of tokens\n");
        return false;
    }
    m_format.fromCSV(tokens[lastToken]);
    return true;
}

