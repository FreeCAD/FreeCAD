/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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
# include <cstring>
# include <cstdlib>
#include <cmath>
#include <string>
# include <exception>
# include <boost/regex.hpp>
# include <QString>
# include <QStringList>
# include <QRegExp>
#include <QChar>
#include <QPointF>

#include <BRep_Tool.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <BRep_Builder.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepLProp_CurveTool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <GProp_GProps.hxx>
#include <GeomLProp_SLProps.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepLProp_SLProps.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepTools.hxx>

#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "GeometryObject.h"
#include "DrawUtil.h"

using namespace TechDraw;

/*static*/ int DrawUtil::getIndexFromName(std::string geomName)
{
//   Base::Console().Message("DU::getIndexFromName(%s)\n", geomName.c_str());
   boost::regex re("\\d+$"); // one of more digits at end of string
   boost::match_results<std::string::const_iterator> what;
   boost::match_flag_type flags = boost::match_default;
//   char* endChar;
   std::string::const_iterator begin = geomName.begin();
   std::string::const_iterator end = geomName.end();
   std::stringstream ErrorMsg;

   if (!geomName.empty()) {
      if (boost::regex_search(begin, end, what, re, flags)) {
           return int (std::stoi(what.str()));
      } else {
         ErrorMsg << "getIndexFromName: malformed geometry name - " << geomName;
         throw Base::ValueError(ErrorMsg.str());
      }
   } else {
         Base::Console().Log("DU::getIndexFromName(%s) - empty geometry name\n",geomName.c_str());
         throw Base::ValueError("getIndexFromName - empty geometry name");
   }
}

std::string DrawUtil::getGeomTypeFromName(std::string geomName)
{
   boost::regex re("^[a-zA-Z]*");                                           //one or more letters at start of string
   boost::match_results<std::string::const_iterator> what;
   boost::match_flag_type flags = boost::match_default;
   std::string::const_iterator begin = geomName.begin();
   std::string::const_iterator end = geomName.end();
   std::stringstream ErrorMsg;

   if (!geomName.empty()) {
      if (boost::regex_search(begin, end, what, re, flags)) {
         return what.str();         //TODO: use std::stoi() in c++11
      } else {
         ErrorMsg << "In getGeomTypeFromName: malformed geometry name - " << geomName;
         throw Base::ValueError(ErrorMsg.str());
      }
   } else {
         throw Base::ValueError("getGeomTypeFromName - empty geometry name");
   }
}

std::string DrawUtil::makeGeomName(std::string geomType, int index)
{
    std::stringstream newName;
    newName << geomType << index;
    return newName.str();
}

//! true if v1 and v2 are the same geometric point within tolerance
bool DrawUtil::isSamePoint(TopoDS_Vertex v1, TopoDS_Vertex v2, double tolerance)
{
    bool result = false;
    gp_Pnt p1 = BRep_Tool::Pnt(v1);
    gp_Pnt p2 = BRep_Tool::Pnt(v2);
    if (p1.IsEqual(p2,tolerance)) {
        result = true;
    }
    return result;
}

bool DrawUtil::isZeroEdge(TopoDS_Edge e, double tolerance)
{
    TopoDS_Vertex vStart = TopExp::FirstVertex(e);
    TopoDS_Vertex vEnd = TopExp::LastVertex(e);
    bool result = isSamePoint(vStart,vEnd, tolerance);
    if (result) {
        //closed edge will have same V's but non-zero length
        GProp_GProps props;
        BRepGProp::LinearProperties(e, props);
        double len = props.Mass();
        if (len > tolerance) {
            result = false;
        }
    }
    return result;
}
double DrawUtil::simpleMinDist(TopoDS_Shape s1, TopoDS_Shape s2)
{
    Standard_Real minDist = -1;

    BRepExtrema_DistShapeShape extss(s1, s2);
    if (!extss.IsDone()) {
        Base::Console().Message("DU::simpleMinDist - BRepExtrema_DistShapeShape failed");
        return -1;
    }
    int count = extss.NbSolution();
    if (count != 0) {
        minDist = extss.Value();
    } else {
        minDist = -1;
    }
    return minDist;
}

//! assumes 2d on XY
//! quick angle for straight edges
double DrawUtil::angleWithX(TopoDS_Edge e, bool reverse)
{
    double result = 0;
    gp_Pnt gstart  = BRep_Tool::Pnt(TopExp::FirstVertex(e));
    Base::Vector3d start(gstart.X(),gstart.Y(),gstart.Z());
    gp_Pnt gend    = BRep_Tool::Pnt(TopExp::LastVertex(e));
    Base::Vector3d end(gend.X(),gend.Y(),gend.Z());
    Base::Vector3d u;
    if (reverse) {
        u = start - end;
    } else {
        u = end - start;
    }
    result = atan2(u.y,u.x);
    if (result < 0) {
         result += 2.0 * M_PI;
    }

    return result;
}

//! find angle of edge with x-Axis at First/LastVertex 
double DrawUtil::angleWithX(TopoDS_Edge e, TopoDS_Vertex v, double tolerance)
{
    double result = 0;
    double param = 0;

    //find tangent @ v
    double adjust = 1.0;            //occ tangent points in direction of curve. at lastVert we need to reverse it.
    BRepAdaptor_Curve adapt(e);
    if (isFirstVert(e,v,tolerance)) {
        param = adapt.FirstParameter();
    } else if (isLastVert(e,v,tolerance)) {
        param = adapt.LastParameter();
        adjust = -1;
    } else {
        //TARFU
        Base::Console().Message("Error: DU::angleWithX - v is neither first nor last \n");
        //must be able to get non-terminal point parm from curve/
    }

    Base::Vector3d uVec(0.0,0.0,0.0);
    gp_Dir uDir;
    BRepLProp_CLProps prop(adapt,param,2,tolerance);
    if (prop.IsTangentDefined()) {
        prop.Tangent(uDir);
        uVec = Base::Vector3d(uDir.X(),uDir.Y(),uDir.Z()) * adjust;
    } else {
        //this bit is a little sketchy
        gp_Pnt gstart  = BRep_Tool::Pnt(TopExp::FirstVertex(e));
        Base::Vector3d start(gstart.X(),gstart.Y(),gstart.Z());
        gp_Pnt gend    = BRep_Tool::Pnt(TopExp::LastVertex(e));
        Base::Vector3d end(gend.X(),gend.Y(),gend.Z());
        if (isFirstVert(e,v,tolerance)) {
            uVec = end - start;
        } else if (isLastVert(e,v,tolerance)) {
            uVec = end - start;
        } else {
          gp_Pnt errPnt = BRep_Tool::Pnt(v);
          Base::Console().Warning("angleWithX: Tangent not defined at (%.3f,%.3f,%.3f)\n",errPnt.X(),errPnt.Y(),errPnt.Z());
          //throw ??????
        }
    }
    result = atan2(uVec.y,uVec.x);
    if (result < 0) {                               //map from [-PI:PI] to [0:2PI]
         result += 2.0 * M_PI;
    }
    return result;
}

bool DrawUtil::isFirstVert(TopoDS_Edge e, TopoDS_Vertex v, double tolerance)
{
    bool result = false;
    TopoDS_Vertex first = TopExp::FirstVertex(e);
    if (isSamePoint(first,v, tolerance)) {
        result = true;
    }
    return result;
}

bool DrawUtil::isLastVert(TopoDS_Edge e, TopoDS_Vertex v, double tolerance)
{
    bool result = false;
    TopoDS_Vertex last = TopExp::LastVertex(e);
    if (isSamePoint(last,v, tolerance)) {
        result = true;
    }
    return result;
}

bool DrawUtil::fpCompare(const double& d1, const double& d2, double tolerance)
{
    bool result = false;
    if (std::fabs(d1 - d2) < tolerance) {
        result = true;
    }
    return result;
}

Base::Vector3d DrawUtil::vertex2Vector(const TopoDS_Vertex& v)
{
    gp_Pnt gp  = BRep_Tool::Pnt(v);
    Base::Vector3d result(gp.X(),gp.Y(),gp.Z());
    return result;
}

std::string DrawUtil::formatVector(const Base::Vector3d& v)
{
    std::string result;
    std::stringstream builder;
    builder << std::fixed << std::setprecision(3) ;
    builder << " (" << v.x  << "," << v.y << "," << v.z << ") ";
//    builder << " (" << setw(6) << v.x  << "," << setw(6) << v.y << "," << setw(6) << v.z << ") ";
    result = builder.str();
    return result;
}

std::string DrawUtil::formatVector(const gp_Dir& v)
{
    std::string result;
    std::stringstream builder;
    builder << std::fixed << std::setprecision(3) ;
    builder << " (" << v.X()  << "," << v.Y() << "," << v.Z() << ") ";
    result = builder.str();
    return result;
}

std::string DrawUtil::formatVector(const gp_Vec& v)
{
    std::string result;
    std::stringstream builder;
    builder << std::fixed << std::setprecision(3) ;
    builder << " (" << v.X()  << "," << v.Y() << "," << v.Z() << ") ";
    result = builder.str();
    return result;
}

std::string DrawUtil::formatVector(const gp_Pnt& v)
{
    std::string result;
    std::stringstream builder;
    builder << std::fixed << std::setprecision(3) ;
    builder << " (" << v.X()  << "," << v.Y() << "," << v.Z() << ") ";
    result = builder.str();
    return result;
}

std::string DrawUtil::formatVector(const QPointF& v)
{
    std::string result;
    std::stringstream builder;
    builder << std::fixed << std::setprecision(3) ;
    builder << " (" << v.x()  << "," << v.y() << ") ";
    result = builder.str();
    return result;
}

//! compare 2 vectors for sorting - true if v1 < v2
bool DrawUtil::vectorLess(const Base::Vector3d& v1, const Base::Vector3d& v2)  
{
    bool result = false;
    if ((v1 - v2).Length() > Precision::Confusion()) {      //ie v1 != v2
        if (!DrawUtil::fpCompare(v1.x,v2.x)) {
            result = v1.x < v2.x;
        } else if (!DrawUtil::fpCompare(v1.y,v2.y)) {
            result = v1.y < v2.y;
        } else {
            result = v1.z < v2.z;
        }
    }
    return result;
}  

//!convert fromPoint in coordinate system fromSystem to reference coordinate system
Base::Vector3d DrawUtil::toR3(const gp_Ax2 fromSystem, const Base::Vector3d fromPoint)
{
    gp_Pnt gFromPoint(fromPoint.x,fromPoint.y,fromPoint.z);
    gp_Pnt gToPoint;
    gp_Trsf T;
    gp_Ax3 gRef;
    gp_Ax3 gFrom(fromSystem);
    T.SetTransformation (gFrom, gRef);
    gToPoint = gFromPoint.Transformed(T);
    Base::Vector3d toPoint(gToPoint.X(),gToPoint.Y(),gToPoint.Z());
    return toPoint;
}

//! check if two vectors are parallel
bool DrawUtil::checkParallel(const Base::Vector3d v1, Base::Vector3d v2, double tolerance)
{
    bool result = false;
    double dot = fabs(v1.Dot(v2));
    double mag = v1.Length() * v2.Length();
    if (DrawUtil::fpCompare(dot,mag,tolerance)) {
        result = true;
    }
    return result;
}

//! rotate vector by angle radians around axis through org
Base::Vector3d DrawUtil::vecRotate(Base::Vector3d vec,
                                   double angle,
                                   Base::Vector3d axis,
                                   Base::Vector3d org)
{
    Base::Vector3d result;
    Base::Matrix4D xForm;
    xForm.rotLine(org,axis,angle);
    result = xForm * (vec);
    return result;
}

Base::Vector3d  DrawUtil::closestBasis(Base::Vector3d v)
{
    Base::Vector3d result(0.0,-1,0);
    Base::Vector3d  stdX(1.0,0.0,0.0);
    Base::Vector3d  stdY(0.0,1.0,0.0);
    Base::Vector3d  stdZ(0.0,0.0,1.0);
    Base::Vector3d  stdXr(-1.0,0.0,0.0);
    Base::Vector3d  stdYr(0.0,-1.0,0.0);
    Base::Vector3d  stdZr(0.0,0.0,-1.0);
    double angleX,angleY,angleZ,angleXr,angleYr,angleZr, angleMin;
    
    //first check if already a basis
    if (checkParallel(v,stdZ)) {
        return v;
    } else if (checkParallel(v,stdY)) {
        return v;
    } else if (checkParallel(v,stdX)) {
        return v;
    }
    
    //not a basis. find smallest angle with a basis.
    angleX = stdX.GetAngle(v);
    angleY = stdY.GetAngle(v);
    angleZ = stdZ.GetAngle(v);
    angleXr = stdXr.GetAngle(v);
    angleYr = stdYr.GetAngle(v);
    angleZr = stdZr.GetAngle(v);

    angleMin = angleX;
    result = stdX;
    if (angleY < angleMin) {
        angleMin = angleY;
        result = stdY;
    }
    if (angleZ < angleMin) {
        angleMin = angleZ;
        result = stdZ;
    }
    if (angleXr < angleMin) {
        angleMin = angleXr;
        result = stdXr;
    }
    if (angleYr < angleMin) {
        angleMin = angleYr;
        result = stdYr;
    }
    if (angleZr < angleMin) {
        angleMin = angleZr;
        result = stdZr;
    }
    return result;
}

//based on Function provided by Joe Dowsett, 2014
double DrawUtil::sensibleScale(double working_scale)
{
    double result = 1.0;
    if (!(working_scale > 0.0)) {
        return result;
    }
    //which gives the largest scale for which the min_space requirements can be met, but we want a 'sensible' scale, rather than 0.28457239...
    //eg if working_scale = 0.115, then we want to use 0.1, similarly 7.65 -> 5, and 76.5 -> 50

    float exponent = std::floor(std::log10(working_scale));                  //if working_scale = a * 10^b, what is b?
    working_scale *= std::pow(10, -exponent);                                //now find what 'a' is.

    //int choices = 10;
    float valid_scales[2][10] =
                          {{1.0, 1.25, 2.0, 2.5, 3.75, 5.0, 7.5, 10.0, 50.0, 100.0},   //equate to 1:10, 1:8, 1:5, 1:4, 3:8, 1:2, 3:4, 1:1
                                                                                       //          .1   .125            .375      .75
                           {1.0, 1.5 , 2.0, 3.0, 4.0 , 5.0, 8.0, 10.0, 50.0, 100.0}};  //equate to 1:1, 3:2, 2:1, 3:1, 4:1, 5:1, 8:1, 10:1
                                                                                       //              1.5:1
    //int i = choices - 1;
    int i = 9;
    while (valid_scales[(exponent >= 0)][i] > working_scale)                 //choose closest value smaller than 'a' from list.
        i -= 1;                                                              //choosing top list if exponent -ve, bottom list for +ve exponent

    //now have the appropriate scale, reapply the *10^b
    result = valid_scales[(exponent >= 0)][i] * pow(10, exponent);
    return result;
}

double DrawUtil::getDefaultLineWeight(std::string lineType)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                    GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    std::string lgName = hGrp->GetASCII("LineGroup","FC 0.70mm");
    auto lg = LineGroup::lineGroupFactory(lgName);
    
    double weight = lg->getWeight(lineType);
    delete lg;                                    //Coverity CID 174671
    return weight;
}

bool DrawUtil::isBetween(const Base::Vector3d pt, const Base::Vector3d end1, const Base::Vector3d end2)
{
    bool result = false;
    double segLength = (end2 - end1).Length();
    double l1        = (pt - end1).Length();
    double l2        = (pt - end2).Length();
    if (fpCompare(segLength,l1 + l2)) {
        result = true;
    }
    return result;
}

Base::Vector3d DrawUtil::Intersect2d(Base::Vector3d p1, Base::Vector3d d1,
                                     Base::Vector3d p2, Base::Vector3d d2)
{
    Base::Vector3d result(0,0,0);
    Base::Vector3d p12(p1.x+d1.x, p1.y+d1.y, 0.0);
    double A1 = d1.y;
    double B1 = -d1.x;
    double C1 = A1*p1.x + B1*p1.y;

    Base::Vector3d p22(p2.x+d2.x, p2.y+d2.y, 0.0);
    double A2 = d2.y;
    double B2 = -d2.x;
    double C2 = A2*p2.x + B2*p2.y;

    double det = A1*B2 - A2*B1;
    if(det == 0){
        Base::Console().Message("Lines are parallel\n");
    }else{
        double x = (B2*C1 - B1*C2)/det;
        double y = (A1*C2 - A2*C1)/det;
        result.x = x;
        result.y = y;
    }

    return result;
}

std::string DrawUtil::shapeToString(TopoDS_Shape s)
{
    std::ostringstream buffer; 
    BRepTools::Write(s, buffer);
    return buffer.str();
}

TopoDS_Shape DrawUtil::shapeFromString(std::string s)
{
    TopoDS_Shape result;
    BRep_Builder builder;
    std::istringstream buffer(s);
    BRepTools::Read(result, buffer, builder);
    return result;
}

Base::Vector3d DrawUtil::invertY(Base::Vector3d v)
{
    Base::Vector3d result(v.x, -v.y, v.z);
    return result;
}

//obs? was used in CSV prototype of Cosmetics
std::vector<std::string> DrawUtil::split(std::string csvLine)
{
//    Base::Console().Message("DU::split - csvLine: %s\n",csvLine.c_str());
    std::vector<std::string>  result;
    std::stringstream     lineStream(csvLine);
    std::string           cell;

    while(std::getline(lineStream,cell, ','))
    {
        result.push_back(cell);
    }
    return result;
}

//obs? was used in CSV prototype of Cosmetics
std::vector<std::string> DrawUtil::tokenize(std::string csvLine, std::string delimiter)
{
//    Base::Console().Message("DU::tokenize - csvLine: %s delimit: %s\n",csvLine.c_str(), delimiter.c_str());
    std::string s(csvLine);
    size_t pos = 0;
    std::vector<std::string> tokens;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        tokens.push_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }
    if (!s.empty()) {     
        tokens.push_back(s);
    }
    return tokens;
}

App::Color DrawUtil::pyTupleToColor(PyObject* pColor)
{
//    Base::Console().Message("DU::pyTupleToColor()\n");
    double red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
    App::Color c(red, blue, green, alpha);
    if (PyTuple_Check(pColor)) {
        int tSize = (int) PyTuple_Size(pColor);
        if (tSize > 2) {
            PyObject* pRed = PyTuple_GetItem(pColor,0);
            red = PyFloat_AsDouble(pRed);
            PyObject* pGreen = PyTuple_GetItem(pColor,1);
            green = PyFloat_AsDouble(pGreen);
            PyObject* pBlue = PyTuple_GetItem(pColor,2);
            blue = PyFloat_AsDouble(pBlue);
        }
        if (tSize > 3) {
            PyObject* pAlpha = PyTuple_GetItem(pColor,3);
            alpha = PyFloat_AsDouble(pAlpha);
        }
        c = App::Color(red, blue, green, alpha);
    }
    return c;
}

PyObject* DrawUtil::colorToPyTuple(App::Color color)
{
//    Base::Console().Message("DU::pyTupleToColor()\n");
    PyObject* pTuple = PyTuple_New(4);
    PyObject* pRed = PyFloat_FromDouble(color.r);
    PyObject* pGreen = PyFloat_FromDouble(color.g);
    PyObject* pBlue = PyFloat_FromDouble(color.b);
    PyObject* pAlpha = PyFloat_FromDouble(color.a);

    PyTuple_SET_ITEM(pTuple, 0,pRed);
    PyTuple_SET_ITEM(pTuple, 1,pGreen);
    PyTuple_SET_ITEM(pTuple, 2,pBlue);
    PyTuple_SET_ITEM(pTuple, 3,pAlpha);

    return pTuple;
}


//============================
// various debugging routines.
void DrawUtil::dumpVertexes(const char* text, const TopoDS_Shape& s)
{
    Base::Console().Message("DUMP - %s\n",text);
    TopExp_Explorer expl(s, TopAbs_VERTEX);
    int i;
    for (i = 1 ; expl.More(); expl.Next(),i++) {
        const TopoDS_Vertex& v = TopoDS::Vertex(expl.Current());
        gp_Pnt pnt = BRep_Tool::Pnt(v);
        Base::Console().Message("v%d: (%.3f,%.3f,%.3f)\n",i,pnt.X(),pnt.Y(),pnt.Z());
    }
}

void DrawUtil::countFaces(const char* text, const TopoDS_Shape& s)
{
    TopTools_IndexedMapOfShape mapOfFaces;
    TopExp::MapShapes(s, TopAbs_FACE, mapOfFaces);
    int num = mapOfFaces.Extent();
    Base::Console().Message("COUNT - %s has %d Faces\n",text,num);
}

//count # of unique Wires in shape.
void DrawUtil::countWires(const char* text, const TopoDS_Shape& s)
{
    TopTools_IndexedMapOfShape mapOfWires;
    TopExp::MapShapes(s, TopAbs_WIRE, mapOfWires);
    int num = mapOfWires.Extent();
    Base::Console().Message("COUNT - %s has %d wires\n",text,num);
}

void DrawUtil::countEdges(const char* text, const TopoDS_Shape& s)
{
    TopTools_IndexedMapOfShape mapOfEdges;
    TopExp::MapShapes(s, TopAbs_EDGE, mapOfEdges);
    int num = mapOfEdges.Extent();
    Base::Console().Message("COUNT - %s has %d edges\n",text,num);
}

void DrawUtil::dump1Vertex(const char* text, const TopoDS_Vertex& v)
{
    Base::Console().Message("DUMP - dump1Vertex - %s\n",text);
    gp_Pnt pnt = BRep_Tool::Pnt(v);
    Base::Console().Message("%s: (%.3f,%.3f,%.3f)\n",text,pnt.X(),pnt.Y(),pnt.Z());
}

void DrawUtil::dumpEdge(char* label, int i, TopoDS_Edge e)
{
    BRepAdaptor_Curve adapt(e);
    double start = BRepLProp_CurveTool::FirstParameter(adapt);
    double end = BRepLProp_CurveTool::LastParameter(adapt);
    BRepLProp_CLProps propStart(adapt,start,0,Precision::Confusion());
    const gp_Pnt& vStart = propStart.Value();
    BRepLProp_CLProps propEnd(adapt,end,0,Precision::Confusion());
    const gp_Pnt& vEnd = propEnd.Value();
    //Base::Console().Message("%s edge:%d start:(%.3f,%.3f,%.3f)/%0.3f end:(%.2f,%.3f,%.3f)/%.3f\n",label,i,
    //                        vStart.X(),vStart.Y(),vStart.Z(),start,vEnd.X(),vEnd.Y(),vEnd.Z(),end);
    Base::Console().Message("%s edge:%d start:(%.3f,%.3f,%.3f)  end:(%.2f,%.3f,%.3f)\n",label,i,
                            vStart.X(),vStart.Y(),vStart.Z(),vEnd.X(),vEnd.Y(),vEnd.Z());
}
const char* DrawUtil::printBool(bool b)
{
    return (b ? "True" : "False");
}

QString DrawUtil::qbaToDebug(const QByteArray & line)
{
    QString s;
    uchar c;

    for ( int i=0 ; i < line.size() ; i++ ){
        c = line[i];
        if (( c >= 0x20) && (c <= 126) ) {
            s.append(QChar::fromLatin1(c));
        } else {
            s.append(QString::fromUtf8("<%1>").arg(c, 2, 16, QChar::fromLatin1('0')));
        }
    }
    return s;
}

//==================================
