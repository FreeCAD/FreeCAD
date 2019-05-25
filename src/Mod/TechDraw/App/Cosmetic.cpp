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
# include <TopoDS_Edge.hxx>
# include <gp_Pnt.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
#endif  // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Tools2D.h>
#include <Base/Vector3D.h>

#include <App/Application.h>
#include <App/Material.h>

#include "DrawUtil.h"
#include "Geometry.h"

#include "Cosmetic.h"

using namespace TechDraw;

CosmeticVertex::CosmeticVertex()
{
    pageLocation = Base::Vector3d(0.0, 0.0, 0.0);
    modelLocation = Base::Vector3d(0.0, 0.0, 0.0);
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

CosmeticVertex::CosmeticVertex(Base::Vector3d loc)
{
    pageLocation = loc;
    modelLocation = loc;

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
    ss << pageLocation.x << "," <<
          pageLocation.y << "," <<
          pageLocation.z << "," <<

          modelLocation.x << "," <<
          modelLocation.y << "," <<
          modelLocation.z << "," <<

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
    unsigned int maxCells = 11;
    if (lineSpec.length() == 0) {
        Base::Console().Message( "CosmeticVertex::fromCSV - lineSpec empty\n");
        return false;
    }
    std::vector<std::string> values = split(lineSpec);
    if (values.size() < maxCells) {
        Base::Console().Message( "CosmeticVertex::fromCSV(%s) invalid CSV entry\n",lineSpec.c_str() );
        return false;
    }
    double x = atof(values[0].c_str());
    double y = atof(values[1].c_str());
    double z = atof(values[2].c_str());
    pageLocation = Base::Vector3d (x,y,z);
    x = atof(values[3].c_str());
    y = atof(values[4].c_str());
    z = atof(values[5].c_str());
    modelLocation = Base::Vector3d (x,y,z);
    linkGeom = atoi(values[6].c_str());
    color.fromHexString(values[7]);
    size = atof(values[8].c_str());
    style = atoi(values[9].c_str());
    visible = atoi(values[10].c_str());
    return true;
}

std::vector<std::string> CosmeticVertex::split(std::string csvLine)
{
//    Base::Console().Message("CV::split - csvLine: %s\n",csvLine.c_str());
    std::vector<std::string>  result;
    std::stringstream     lineStream(csvLine);
    std::string           cell;

    while(std::getline(lineStream,cell, ','))
    {
        result.push_back(cell);
    }
    return result;
}

void CosmeticVertex::dump(char* title)
{
    Base::Console().Message("CV::dump - %s \n",title);
    Base::Console().Message("CV::dump - %s \n",toCSV().c_str());
}

//******************************************

CosmeticEdge::CosmeticEdge()
{
    geometry = new TechDrawGeometry::BaseGeom();
    linkGeom = -1;
    color = getDefEdgeColor();
    width  = getDefEdgeWidth();
    style = getDefEdgeStyle();
    visible = true;
}

CosmeticEdge::CosmeticEdge(Base::Vector3d p1, Base::Vector3d p2)
{
    gp_Pnt gp1(p1.x,p1.y,p1.z);
    gp_Pnt gp2(p2.x,p2.y,p2.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
    geometry = TechDrawGeometry::BaseGeom::baseFactory(e);
    linkGeom = -1;
    color = getDefEdgeColor();
    width  = getDefEdgeWidth();
    style = getDefEdgeStyle();
    visible = true;
}

CosmeticEdge::CosmeticEdge(TopoDS_Edge e)
{
    geometry = TechDrawGeometry::BaseGeom::baseFactory(e);
    linkGeom = -1;
    color = getDefEdgeColor();
    width  = getDefEdgeWidth();
    style = getDefEdgeStyle();
    visible = true;
}

double CosmeticEdge::getDefEdgeWidth()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                    GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    std::string lgName = hGrp->GetASCII("LineGroup","FC 0.70mm");
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgName);

    double width = lg->getWeight("Graphic");
    delete lg; 
    return width;
}

App::Color CosmeticEdge::getDefEdgeColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("NormalColor", 0x00000000));
    return fcColor;
}

int CosmeticEdge::getDefEdgeStyle()
{
    return 1;
}

std::string CosmeticEdge::toCSV(void) const
{
    std::stringstream ss;
    Base::Vector3d start, end;
    if (geometry != nullptr) {
        Base::Vector2d p2d = geometry->getStartPoint();
        start = Base::Vector3d(p2d.x, p2d.y, 0.0);
        p2d = geometry->getEndPoint();
        end = Base::Vector3d(p2d.x, p2d.y, 0.0);
    }
    ss << start.x << "," <<
        start.y << "," <<
        start.z << "," <<
        end.x << "," <<
        end.y << "," <<
        end.z << "," <<
        linkGeom << "," << 
        color.asHexString() << ","  <<
        width << "," <<
        style << ","  <<
        visible << 
        std::endl;
    return ss.str();
}

bool CosmeticEdge::fromCSV(std::string& lineSpec)
{
    unsigned int maxCells = 11;
    if (lineSpec.length() == 0) {
        Base::Console().Message( "CosmeticEdge::fromCSV - lineSpec empty\n");
        return false;
    }
    std::vector<std::string> values = split(lineSpec);
    Base::Console().Message("CE::fromCSV - values: %d\n",values.size());
    if (values.size() < maxCells) {
        Base::Console().Message( "CosmeticEdge::fromCSV(%s) invalid CSV entry\n",lineSpec.c_str() );
        return false;
    }
    Base::Vector3d start, end;
    double x = atof(values[0].c_str());
    double y = atof(values[1].c_str());
    double z = atof(values[2].c_str());
    start = Base::Vector3d (x,y,z);
    x = atof(values[3].c_str());
    y = atof(values[4].c_str());
    z = atof(values[5].c_str());
    end = Base::Vector3d (x,y,z);

    linkGeom = atoi(values[6].c_str());
    color.fromHexString(values[7]);
    width = atof(values[8].c_str());
    style = atoi(values[9].c_str());
    visible = atoi(values[10].c_str());
    return true;
}

//duplicate of CV routine.  make static? or base class?
std::vector<std::string> CosmeticEdge::split(std::string csvLine)
{
    Base::Console().Message("CE::split - csvLine: %s\n",csvLine.c_str());
    std::vector<std::string>  result;
    std::stringstream     lineStream(csvLine);
    std::string           cell;

    while(std::getline(lineStream,cell, ','))
    {
        result.push_back(cell);
    }
    return result;
}

//duplicate of CV routine.  make static? or base class?
void CosmeticEdge::dump(char* title)
{
    Base::Console().Message("CE::dump - %s \n",title);
    Base::Console().Message("CE::dump - %s \n",toCSV().c_str());
}


