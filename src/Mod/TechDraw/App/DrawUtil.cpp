
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
# include <exception>
# include <boost/regex.hpp>
# include <QString>
# include <QStringList>
# include <QRegExp>
#include <QChar>


#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>
#include <BRepLProp_CLProps.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepLProp_CurveTool.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>

#include "DrawUtil.h"

using namespace TechDraw;

/*static*/ int DrawUtil::getIndexFromName(std::string geomName)
{
   boost::regex re("\\d+$"); // one of more digits at end of string
   boost::match_results<std::string::const_iterator> what;
   boost::match_flag_type flags = boost::match_default;
   char* endChar;
   std::string::const_iterator begin = geomName.begin();
   std::string::const_iterator end = geomName.end();
   std::stringstream ErrorMsg;

   if (!geomName.empty()) {
      if (boost::regex_search(begin, end, what, re, flags)) {
         return int (std::strtol(what.str().c_str(), &endChar, 10));         //TODO: use std::stoi() in c++11
      } else {
         ErrorMsg << "getIndexFromName: malformed geometry name - " << geomName;
         throw Base::Exception(ErrorMsg.str());
      }
   } else {
         throw Base::Exception("getIndexFromName - empty geometry name");
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
         throw Base::Exception(ErrorMsg.str());
      }
   } else {
         throw Base::Exception("getGeomTypeFromName - empty geometry name");
   }
}

std::string DrawUtil::makeGeomName(std::string geomType, int index)
{
    std::stringstream newName;
    newName << geomType << index;
    return newName.str();
}


bool DrawUtil::isSamePoint(TopoDS_Vertex v1, TopoDS_Vertex v2)
{
    bool result = false;
    gp_Pnt p1 = BRep_Tool::Pnt(v1);
    gp_Pnt p2 = BRep_Tool::Pnt(v2);
    if (p1.IsEqual(p2,Precision::Confusion())) {
        result = true;
    }
    return result;
}

bool DrawUtil::isZeroEdge(TopoDS_Edge e)
{
    TopoDS_Vertex vStart = TopExp::FirstVertex(e);
    TopoDS_Vertex vEnd = TopExp::LastVertex(e);
    bool result = isSamePoint(vStart,vEnd);
    if (result) {
        //closed edge will have same V's but non-zero length
        GProp_GProps props;
        BRepGProp::LinearProperties(e, props);
        double len = props.Mass();
        if (len > Precision::Confusion()) {
            result = false;
        }
    }
    return result;
}

//based on Function provided by Joe Dowsett, 2014
double DrawUtil::sensibleScale(double working_scale)
{
    double result = 1.0;
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
