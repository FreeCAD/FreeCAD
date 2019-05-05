/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <QString>
# include <QStringList>
# include <QRegExp>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#endif

#include <QLocale>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Mod/Measure/App/Measurement.h>

#include "Geometry.h"
#include "DrawViewPart.h"
#include "DrawViewDimension.h"
#include "DrawUtil.h"
#include "LineGroup.h"


#include <Mod/TechDraw/App/DrawViewDimensionPy.h>  // generated from DrawViewDimensionPy.xml

using namespace TechDraw;

//===========================================================================
// DrawViewDimension
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDimension, TechDraw::DrawView)

const char* DrawViewDimension::TypeEnums[]= {"Distance",
                                                "DistanceX",
                                                "DistanceY",
                                                "DistanceZ",
                                                "Radius",
                                                "Diameter",
                                                "Angle",
                                                "Angle3Pt",
                                                NULL};

const char* DrawViewDimension::MeasureTypeEnums[]= {"True",
                                                    "Projected",
                                                    NULL};

enum RefType{
        invalidRef,
        oneEdge,
        twoEdge,
        twoVertex,
        vertexEdge,
        threeVertex
    };

DrawViewDimension::DrawViewDimension(void)
{
    ADD_PROPERTY_TYPE(References2D,(0,0),"",(App::PropertyType)(App::Prop_None),"Projected Geometry References");
    References2D.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(References3D,(0,0),"",(App::PropertyType)(App::Prop_None),"3D Geometry References");
    References3D.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(FormatSpec,("") , "Format",(App::PropertyType)(App::Prop_None),"Dimension Format");
    ADD_PROPERTY_TYPE(Arbitrary,(false) ,"Format",(App::PropertyType)(App::Prop_None),"Value overridden by user");

    Type.setEnums(TypeEnums);                                          //dimension type: length, radius etc
    ADD_PROPERTY(Type,((long)0));
    MeasureType.setEnums(MeasureTypeEnums);
    ADD_PROPERTY(MeasureType, ((long)1));                             //Projected (or True) measurement
    ADD_PROPERTY_TYPE(OverTolerance ,(0.0),"",App::Prop_None,"+ Tolerance value");
    ADD_PROPERTY_TYPE(UnderTolerance ,(0.0),"",App::Prop_None,"- Tolerance value");

    //hide the properties the user can't edit in the property editor
//    References2D.setStatus(App::Property::Hidden,true);
    References3D.setStatus(App::Property::Hidden,true);

    //hide the DrawView properties that don't apply to Dimensions
    ScaleType.setStatus(App::Property::ReadOnly,true);
    ScaleType.setStatus(App::Property::Hidden,true);
    Scale.setStatus(App::Property::ReadOnly,true);
    Scale.setStatus(App::Property::Hidden,true);
    Rotation.setStatus(App::Property::ReadOnly,true);
    Rotation.setStatus(App::Property::Hidden,true);

    measurement = new Measure::Measurement();
    //TODO: should have better initial datumLabel position than (0,0) in the DVP?? something closer to the object being measured? 

    //initialize the descriptive geometry. 
    //TODO: should this be more like DVP with a "geometry object"?
    m_linearPoints.first  = Base::Vector3d(0,0,0);
    m_linearPoints.second = Base::Vector3d(0,0,0);

    m_anglePoints.ends.first = Base::Vector3d(0,0,0);
    m_anglePoints.ends.second = Base::Vector3d(0,0,0);
    m_anglePoints.vertex = Base::Vector3d(0,0,0);

    m_arcPoints.isArc = false;
    m_arcPoints.center = Base::Vector3d(0,0,0);
    m_arcPoints.onCurve.first = Base::Vector3d(0,0,0);
    m_arcPoints.onCurve.second = Base::Vector3d(0,0,0);
    m_arcPoints.arcEnds.first = Base::Vector3d(0,0,0);
    m_arcPoints.arcEnds.second = Base::Vector3d(0,0,0);
    m_arcPoints.midArc = Base::Vector3d(0,0,0);
    m_arcPoints.arcCW = false;
    m_hasGeometry = false;
}

DrawViewDimension::~DrawViewDimension()
{
    delete measurement;
    measurement = 0;
}

void DrawViewDimension::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &MeasureType) {
            if (MeasureType.isValue("True") && !measurement->has3DReferences()) {
                Base::Console().Warning("%s has no 3D References but is Type: True\n", getNameInDocument());
                MeasureType.setValue("Projected");
            }
        }
        if (prop == &References3D) {                                       //have to rebuild the Measurement object
            clear3DMeasurements();                                                             //Measurement object
            if (!(References3D.getValues()).empty()) {
                setAll3DMeasurement();
            } else {
                if (MeasureType.isValue("True")) {                                 //empty 3dRefs, but True
                    MeasureType.touch();                                          //run MeasureType logic for this case
                }
            }
        }
        if (prop == &Type) {
            FormatSpec.setValue(getDefaultFormatSpec().c_str());
        }
    }

    DrawView::onChanged(prop);
}

void DrawViewDimension::onDocumentRestored()
{
    if (has3DReferences()) {
        setAll3DMeasurement();
    }
}


short DrawViewDimension::mustExecute() const
{
    bool result = 0;
    if (!isRestoring()) {
        result =  (References2D.isTouched() ||
                  Type.isTouched() ||
                  FormatSpec.isTouched() ||
                  MeasureType.isTouched());
    }
    if (result) {
        return result;
    }
    
    auto dvp = getViewPart();
    if (dvp != nullptr) {
        result = dvp->isTouched();
    }
    if (result) {
        return result;
    }

    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawViewDimension::execute(void)
{
//    Base::Console().Message("DVD::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    //any empty Reference2D??
    if (!has2DReferences()) {                                            //too soon?
        if (isRestoring() ||
            getDocument()->testStatus(App::Document::Status::Restoring)) {
            return App::DocumentObject::StdReturn;
        } else {
            Base::Console().Warning("%s has no 2D References\n", getNameInDocument());
        }
        return App::DocumentObject::StdReturn;
    }

    //can't do anything until Source has geometry
    if (!getViewPart()->hasGeometry()) {                              //happens when loading saved document
        if (isRestoring() ||
            getDocument()->testStatus(App::Document::Status::Restoring)) {
            return App::DocumentObject::StdReturn;
        } else {
            Base::Console().Warning("%s - target has no geometry\n", getNameInDocument());
            return App::DocumentObject::StdReturn;
        }
    }

    //now we can check if Reference2ds have valid targets.
    if (!checkReferences2D()) {
        return App::DocumentObject::StdReturn;
    }

    const std::vector<std::string> &subElements = References2D.getSubValues();

    if ( Type.isValue("Distance")  ||
         Type.isValue("DistanceX") ||
         Type.isValue("DistanceY") )  {

        if (getRefType() == oneEdge) {
            m_linearPoints = getPointsOneEdge();
        }else if (getRefType() == twoEdge) {
            m_linearPoints = getPointsTwoEdges();
        } else if (getRefType() == twoVertex) {
            m_linearPoints = getPointsTwoVerts();
        } else if (getRefType() == vertexEdge) {
            m_linearPoints = getPointsEdgeVert();
        }  //else tarfu
        m_hasGeometry = true;
    } else if(Type.isValue("Radius")){
        int idx = DrawUtil::getIndexFromName(subElements[0]);
        TechDrawGeometry::BaseGeom* base = getViewPart()->getProjEdgeByIndex(idx);
        TechDrawGeometry::Circle* circle;
        arcPoints pts;
        pts.center = Base::Vector3d(0.0,0.0,0.0);
        pts.radius = 0.0;
        if( (base && base->geomType == TechDrawGeometry::GeomType::CIRCLE) || 
           (base && base->geomType == TechDrawGeometry::GeomType::ARCOFCIRCLE))  {
            circle = static_cast<TechDrawGeometry::Circle*> (base);
            pts.center = Base::Vector3d(circle->center.x,circle->center.y,0.0);
            pts.radius = circle->radius;
            if (base->geomType == TechDrawGeometry::GeomType::ARCOFCIRCLE) {
                TechDrawGeometry::AOC* aoc = static_cast<TechDrawGeometry::AOC*> (circle);
                pts.isArc = true;
                pts.onCurve.first  = Base::Vector3d(aoc->midPnt.x,aoc->midPnt.y,0.0);
                pts.midArc         = Base::Vector3d(aoc->midPnt.x,aoc->midPnt.y,0.0);
                pts.arcEnds.first  = Base::Vector3d(aoc->startPnt.x,aoc->startPnt.y,0.0);
                pts.arcEnds.second = Base::Vector3d(aoc->endPnt.x,aoc->endPnt.y,0.0);
                pts.arcCW          = aoc->cw;
            } else {
                pts.isArc = false;
                pts.onCurve.first  = pts.center + Base::Vector3d(1,0,0) * circle->radius;   //arbitrary point on edge
                pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * circle->radius;  //arbitrary point on edge
            }
        } else if (base && base->geomType == TechDrawGeometry::GeomType::BSPLINE) {
            TechDrawGeometry::BSpline* spline = static_cast<TechDrawGeometry::BSpline*> (base);
            if (spline->isCircle()) {
                bool circ,arc;
                double rad;
                Base::Vector3d center;
                spline->getCircleParms(circ,rad,center,arc);
                pts.center = Base::Vector3d(center.x,center.y,0.0);
                pts.radius = rad;
                pts.arcEnds.first  = Base::Vector3d(spline->startPnt.x,spline->startPnt.y,0.0);
                pts.arcEnds.second = Base::Vector3d(spline->endPnt.x,spline->endPnt.y,0.0);
                pts.midArc         = Base::Vector3d(spline->midPnt.x,spline->midPnt.y,0.0);
                pts.isArc = arc;
                pts.arcCW          = spline->cw;
                if (arc) {
                    pts.onCurve.first  = Base::Vector3d(spline->midPnt.x,spline->midPnt.y,0.0);
                } else {
                    pts.onCurve.first  = pts.center + Base::Vector3d(1,0,0) * rad;   //arbitrary point on edge
                    pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * rad;  //arbitrary point on edge
                }
            } else {
                //fubar - can't have non-circular spline as target of Diameter dimension
                Base::Console().Error("Dimension %s refers to invalid BSpline\n",getNameInDocument());
                return App::DocumentObject::StdReturn;
            }
        } else {
            Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
            return App::DocumentObject::StdReturn;
        }
        m_arcPoints = pts;
        m_hasGeometry = true;
    } else if(Type.isValue("Diameter")){
        int idx = DrawUtil::getIndexFromName(subElements[0]);
        TechDrawGeometry::BaseGeom* base = getViewPart()->getProjEdgeByIndex(idx);
        TechDrawGeometry::Circle* circle;
        arcPoints pts;
        pts.center = Base::Vector3d(0.0,0.0,0.0);
        pts.radius = 0.0;
        if ((base && base->geomType == TechDrawGeometry::GeomType::CIRCLE) || 
           (base && base->geomType == TechDrawGeometry::GeomType::ARCOFCIRCLE)) {
            circle = static_cast<TechDrawGeometry::Circle*> (base);
            pts.center = Base::Vector3d(circle->center.x,circle->center.y,0.0);
            pts.radius = circle->radius;
            if (base->geomType == TechDrawGeometry::GeomType::ARCOFCIRCLE) {
                TechDrawGeometry::AOC* aoc = static_cast<TechDrawGeometry::AOC*> (circle);
                pts.isArc = true;
                pts.onCurve.first  = Base::Vector3d(aoc->midPnt.x,aoc->midPnt.y,0.0);
                pts.midArc         = Base::Vector3d(aoc->midPnt.x,aoc->midPnt.y,0.0);
                pts.arcEnds.first  = Base::Vector3d(aoc->startPnt.x,aoc->startPnt.y,0.0);
                pts.arcEnds.second = Base::Vector3d(aoc->endPnt.x,aoc->endPnt.y,0.0);
                pts.arcCW          = aoc->cw;
            } else {
                pts.isArc = false;
                pts.onCurve.first  = pts.center + Base::Vector3d(1,0,0) * circle->radius;   //arbitrary point on edge
                pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * circle->radius;  //arbitrary point on edge
            }
        } else if (base && base->geomType == TechDrawGeometry::GeomType::BSPLINE) {
            TechDrawGeometry::BSpline* spline = static_cast<TechDrawGeometry::BSpline*> (base);
            if (spline->isCircle()) {
                bool circ,arc;
                double rad;
                Base::Vector3d center;
                spline->getCircleParms(circ,rad,center,arc);
                pts.center = Base::Vector3d(center.x,center.y,0.0);
                pts.radius = rad;
                pts.arcEnds.first  = Base::Vector3d(spline->startPnt.x,spline->startPnt.y,0.0);
                pts.arcEnds.second = Base::Vector3d(spline->endPnt.x,spline->endPnt.y,0.0);
                pts.midArc         = Base::Vector3d(spline->midPnt.x,spline->midPnt.y,0.0);
                pts.isArc = arc;
                pts.arcCW          = spline->cw;
                if (arc) {
                    pts.onCurve.first  = Base::Vector3d(spline->midPnt.x,spline->midPnt.y,0.0);
                } else {
                    pts.onCurve.first  = pts.center + Base::Vector3d(1,0,0) * rad;   //arbitrary point on edge
                    pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * rad;  //arbitrary point on edge
                }
            } else {
                //fubar - can't have non-circular spline as target of Diameter dimension
                Base::Console().Error("Dimension %s refers to invalid BSpline\n",getNameInDocument());
                return App::DocumentObject::StdReturn;
            }
        } else {
            Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
            return App::DocumentObject::StdReturn;
        }
        m_arcPoints = pts;
        m_hasGeometry = true;
    } else if(Type.isValue("Angle")){
        if (getRefType() != twoEdge) {
             Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
             return App::DocumentObject::StdReturn;
        }
        int idx0 = DrawUtil::getIndexFromName(subElements[0]);
        int idx1 = DrawUtil::getIndexFromName(subElements[1]);
        TechDrawGeometry::BaseGeom* edge0 = getViewPart()->getProjEdgeByIndex(idx0);
        TechDrawGeometry::BaseGeom* edge1 = getViewPart()->getProjEdgeByIndex(idx1);
        TechDrawGeometry::Generic *gen0;
        TechDrawGeometry::Generic *gen1;
        if (edge0 && edge0->geomType == TechDrawGeometry::GeomType::GENERIC) {
             gen0 = static_cast<TechDrawGeometry::Generic*>(edge0);
        } else {
             Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
             return App::DocumentObject::StdReturn;
        }
        if (edge1 && edge1->geomType == TechDrawGeometry::GeomType::GENERIC) {
             gen1 = static_cast<TechDrawGeometry::Generic*>(edge1);
        } else {
             Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
             return App::DocumentObject::StdReturn;
        }

        anglePoints pts;
        Base::Vector3d apex = DrawUtil::vector23(gen0->apparentInter(gen1));
        Base::Vector3d extPoint0,extPoint1;
        if ((DrawUtil::vector23(gen0->getStartPoint()) - apex).Length() >
            (DrawUtil::vector23(gen0->getEndPoint()) - apex).Length()) {
            extPoint0 = DrawUtil::vector23(gen0->getStartPoint());
        } else {
            extPoint0 = DrawUtil::vector23(gen0->getEndPoint());
        }
        if ((DrawUtil::vector23(gen1->getStartPoint()) - apex).Length() >
            (DrawUtil::vector23(gen1->getEndPoint()) - apex).Length()) {
            extPoint1 = DrawUtil::vector23(gen1->getStartPoint());
        } else {
            extPoint1 = DrawUtil::vector23(gen1->getEndPoint());
        }
        pts.ends.first  = extPoint0;
        pts.ends.second = extPoint1;
        pts.vertex = apex;
        m_anglePoints = pts;
        m_hasGeometry = true;
    } else if(Type.isValue("Angle3Pt")){
        if (getRefType() != threeVertex) {
             Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
             return App::DocumentObject::StdReturn;
        }
        int idx0 = DrawUtil::getIndexFromName(subElements[0]);
        int idx1 = DrawUtil::getIndexFromName(subElements[1]);
        int idx2 = DrawUtil::getIndexFromName(subElements[2]);
                               
        TechDrawGeometry::Vertex* vert0 = getViewPart()->getProjVertexByIndex(idx0);
        TechDrawGeometry::Vertex* vert1 = getViewPart()->getProjVertexByIndex(idx1);
        TechDrawGeometry::Vertex* vert2 = getViewPart()->getProjVertexByIndex(idx2);
        if (!vert0 || !vert1 || !vert2) {
             Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
             return App::DocumentObject::StdReturn;
        }

        anglePoints pts;
        Base::Vector3d apex =  vert1->getAs3D();
        Base::Vector3d extPoint0 = vert0->getAs3D();
        Base::Vector3d extPoint2 = vert2->getAs3D();
        pts.ends.first  = extPoint0;
        pts.ends.second = extPoint2;
        pts.vertex = apex;
        m_anglePoints = pts;
        m_hasGeometry = true;
    }

    //TODO: if MeasureType = Projected and the Projected shape changes, the Dimension may become invalid (see tilted Cube example)

    return DrawView::execute();
}

std::string  DrawViewDimension::getFormatedValue(bool obtuse)
{
//    Base::Console().Message("DVD::getFormatedValue()\n");
    std::string result;
    if (Arbitrary.getValue()) {
        return FormatSpec.getStrValue();
    }

    QString specStr = QString::fromUtf8(FormatSpec.getStrValue().data(),FormatSpec.getStrValue().size());
    double val = std::abs(getDimValue());    //internal units!
    
    bool angularMeasure = false;
    Base::Quantity qVal;
    qVal.setValue(val);
    if ( (Type.isValue("Angle")) ||
         (Type.isValue("Angle3Pt")) ) {
        angularMeasure = true;
        qVal.setUnit(Base::Unit::Angle);
        if (obtuse) {
            qVal.setValue(fabs(360.0 - val));
        }
    } else {
        qVal.setUnit(Base::Unit::Length);
    }

    QString userStr = qVal.getUserString();                           // this handles mm to inch/km/parsec etc
                                                                      // and decimal positions but won't give more than
                                                                      // Global_Decimals precision
                                                                      // really should be able to ask units for value
                                                                      // in appropriate UoM!!

    //units api: get schema to figure out if this is multi-value schema(Imperial1, ImperialBuilding, etc)
    //if it is multi-unit schema, don't even try to use Alt Decimals or format per format spec
    Base::UnitSystem uniSys = Base::UnitsApi::getSchema();

//handle multi value schemes
    std::string pre = getPrefix();
    QString qPre = QString::fromUtf8(pre.data(),pre.size());
    if (((uniSys == Base::UnitSystem::Imperial1) ||
         (uniSys == Base::UnitSystem::ImperialBuilding) ) &&
         !angularMeasure) {
        specStr = userStr;
        if (!pre.empty()) {
            specStr = qPre + userStr;
        }
    } else if ((uniSys == Base::UnitSystem::ImperialCivil) &&
         angularMeasure) {
        QString dispMinute = QString::fromUtf8("\'");
        QString dispSecond = QString::fromUtf8("\"");
        QString schemeMinute = QString::fromUtf8("M");
        QString schemeSecond = QString::fromUtf8("S");
        specStr = userStr.replace(schemeMinute,dispMinute);
        specStr = specStr.replace(schemeSecond,dispSecond);
        if (!pre.empty()) {
            specStr = qPre + userStr;
        }
    } else {
//handle single value schemes
        QRegExp rxUnits(QString::fromUtf8(" \\D*$"));                     //space + any non digits at end of string

        QString userVal = userStr;
        userVal.remove(rxUnits);                                          //getUserString(defaultDecimals) without units

        QLocale loc;
        double userValNum = loc.toDouble(userVal);

        QString userUnits;
        int pos = 0;
        if ((pos = rxUnits.indexIn(userStr, 0)) != -1)  {
            userUnits = rxUnits.cap(0);                                       //entire capture - non numerics at end of userString
        }

        //find the %x.y tag in FormatSpec
        QRegExp rxFormat(QString::fromUtf8("%[0-9]*\\.*[0-9]*[aefgAEFG]"));     //printf double format spec 
        QString match;
        QString specVal = userVal;                                             //sensible default
        pos = 0;
        if ((pos = rxFormat.indexIn(specStr, 0)) != -1)  {
            match = rxFormat.cap(0);                                          //entire capture of rx
    #if QT_VERSION >= 0x050000
            specVal = QString::asprintf(Base::Tools::toStdString(match).c_str(),userValNum);
    #else
            QString qs2;
            specVal = qs2.sprintf(Base::Tools::toStdString(match).c_str(),userValNum);
    #endif
        } else {       //printf format not found!
            Base::Console().Warning("Warning - no numeric format in formatSpec - %s\n",getNameInDocument());
            return Base::Tools::toStdString(specStr);
        }

        QString repl = userVal;
        if (useDecimals()) {
            if (showUnits() || (Type.isValue("Angle")) ||(Type.isValue("Angle3Pt")) ) {
                repl = userStr;
            } else {
                repl = userVal;
            }
        } else {
            if (showUnits() || (Type.isValue("Angle")) || (Type.isValue("Angle3Pt"))) {
                repl = specVal + userUnits;
            } else {
                repl = specVal;
            }
        }

        specStr.replace(match,repl);
        //this next bit is so inelegant!!!
        QChar dp = QChar::fromLatin1('.');
        if (loc.decimalPoint() != dp) {
            specStr.replace(dp,loc.decimalPoint());
        }
        //Remove space between dimension and degree sign
        if ((Type.isValue("Angle")) || (Type.isValue("Angle3Pt"))) {
            QRegExp space(QString::fromUtf8("\\s"));
            specStr.remove(space);
        }
    }

    return specStr.toUtf8().constData();
}

//!NOTE: this returns the Dimension value in internal units (ie mm)!!!!
double DrawViewDimension::getDimValue()
{
//    Base::Console().Message("DVD::getDimValue()\n");
    double result = 0.0;
    if (!has2DReferences()) {                                            //too soon?
        if (isRestoring() ||
            getDocument()->testStatus(App::Document::Status::Restoring)) {
            return result;
        } else {
            Base::Console().Warning("%s has no 2D References\n", getNameInDocument());
        }
        return result;
    }

    if (!getViewPart()->hasGeometry()) {                              //happens when loading saved document
        return result;
    }

    if (MeasureType.isValue("True")) {
        // True Values
        if (!measurement->has3DReferences()) {
            Base::Console().Warning("%s - True dimension has no 3D References\n", getNameInDocument());
            return result;
        }
        if ( Type.isValue("Distance")  ||
             Type.isValue("DistanceX") ||
             Type.isValue("DistanceY") )  {
            result = measurement->length();
        } else if(Type.isValue("Radius")){
            result = measurement->radius();
        } else if(Type.isValue("Diameter")){
            result = 2.0 * measurement->radius();
        } else if(Type.isValue("Angle")){
            result = measurement->angle();
        } else {  //tarfu
            throw Base::ValueError("getDimValue() - Unknown Dimension Type (3)");
        }
    } else {
        // Projected Values
        if (!checkReferences2D()) {
            Base::Console().Warning("DVD::getDimValue - %s - 2D references are corrupt\n",getNameInDocument());
            return result;
        }
        if ( Type.isValue("Distance")  ||
             Type.isValue("DistanceX") ||
             Type.isValue("DistanceY") )  {
            pointPair pts = m_linearPoints;
            Base::Vector3d dimVec = pts.first - pts.second;
            if (Type.isValue("Distance")) {
                result = dimVec.Length() / getViewPart()->getScale();
            } else if (Type.isValue("DistanceX")) {
                result = fabs(dimVec.x) / getViewPart()->getScale();
            } else {
                result = fabs(dimVec.y) / getViewPart()->getScale();
            }

        } else if(Type.isValue("Radius")){
            arcPoints pts = m_arcPoints;
            result = pts.radius / getViewPart()->getScale();            //Projected BaseGeom is scaled for drawing

            
        } else if(Type.isValue("Diameter")){
            arcPoints pts = m_arcPoints;
            result = (pts.radius  * 2.0) / getViewPart()->getScale();   //Projected BaseGeom is scaled for drawing

        } else if(Type.isValue("Angle")){
            anglePoints pts = m_anglePoints;
            Base::Vector3d vertex = pts.vertex;
            Base::Vector3d leg0 = pts.ends.first - vertex;
            Base::Vector3d leg1 = pts.ends.second - vertex;
            double legAngle =  leg0.GetAngle(leg1) * 180.0 / M_PI;
            result = legAngle;

        } else if(Type.isValue("Angle3Pt")){    //same as case "Angle"?
            anglePoints pts = m_anglePoints;
            Base::Vector3d vertex = pts.vertex;
            Base::Vector3d leg0 = pts.ends.first - vertex;
            Base::Vector3d leg1 = pts.ends.second - vertex;
            double legAngle =  leg0.GetAngle(leg1) * 180.0 / M_PI;
            result = legAngle;
        }
    }
    return result;
}

pointPair DrawViewDimension::getPointsOneEdge()
{
//    Base::Console().Message("DVD::getPointsOneEdge() - %s\n",getNameInDocument());
    pointPair result;
    const std::vector<std::string> &subElements      = References2D.getSubValues();

    //TODO: Check for straight line Edge?
    int idx = DrawUtil::getIndexFromName(subElements[0]);
    TechDrawGeometry::BaseGeom* geom = getViewPart()->getProjEdgeByIndex(idx);
    TechDrawGeometry::Generic* gen;
    if (geom && geom->geomType == TechDrawGeometry::GeomType::GENERIC) {
        gen = static_cast<TechDrawGeometry::Generic*>(geom);
    } else {
        Base::Console().Error("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
        return result;
    }
    result.first = DrawUtil::vector23(gen->points[0]);
    result.second = DrawUtil::vector23(gen->points[1]);
    return result;
}

pointPair DrawViewDimension::getPointsTwoEdges()
{
//    Base::Console().Message("DVD::getPointsTwoEdges() - %s\n",getNameInDocument());
    pointPair result;
    const std::vector<std::string> &subElements      = References2D.getSubValues();

    int idx0 = DrawUtil::getIndexFromName(subElements[0]);
    int idx1 = DrawUtil::getIndexFromName(subElements[1]);
    TechDrawGeometry::BaseGeom* geom0 = getViewPart()->getProjEdgeByIndex(idx0);
    TechDrawGeometry::BaseGeom* geom1 = getViewPart()->getProjEdgeByIndex(idx1);
    if ((geom0 == nullptr) ||
        (geom1 == nullptr) ) {
        Base::Console().Error("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
        return result;
    }
    result = closestPoints(geom0->occEdge,geom1->occEdge);
    return result;
}

pointPair DrawViewDimension::getPointsTwoVerts()
{
//    Base::Console().Message("DVD::getPointsTwoVerts() - %s\n",getNameInDocument());
    pointPair result;
    const std::vector<std::string> &subElements      = References2D.getSubValues();

    int idx0 = DrawUtil::getIndexFromName(subElements[0]);
    int idx1 = DrawUtil::getIndexFromName(subElements[1]);
    TechDrawGeometry::Vertex* v0 = getViewPart()->getProjVertexByIndex(idx0);
    TechDrawGeometry::Vertex* v1 = getViewPart()->getProjVertexByIndex(idx1);
    if ((v0 == nullptr) ||
        (v1 == nullptr) ) {
        Base::Console().Error("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
        return result;
    }
    result.first  = DrawUtil::vector23(v0->pnt);
    result.second = DrawUtil::vector23(v1->pnt);
    return result;
}    

pointPair DrawViewDimension::getPointsEdgeVert()
{
//    Base::Console().Message("DVD::getPointsEdgeVert() - %s\n",getNameInDocument());
    pointPair result;
    const std::vector<std::string> &subElements      = References2D.getSubValues();

    int idx0 = DrawUtil::getIndexFromName(subElements[0]);
    int idx1 = DrawUtil::getIndexFromName(subElements[1]);
    TechDrawGeometry::BaseGeom* e;
    TechDrawGeometry::Vertex* v;
    if (DrawUtil::getGeomTypeFromName(subElements[0]) == "Edge") {
        e = getViewPart()->getProjEdgeByIndex(idx0);
        v = getViewPart()->getProjVertexByIndex(idx1);
    } else {
        e = getViewPart()->getProjEdgeByIndex(idx1);
        v = getViewPart()->getProjVertexByIndex(idx0);
    }
    if ((v == nullptr) ||
        (e == nullptr) ) {
        Base::Console().Error("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
        return result;
    }
    result = closestPoints(e->occEdge,v->occVertex);
    return result;
}

DrawViewPart* DrawViewDimension::getViewPart() const
{
    if (References2D.getValues().empty()) {
        return nullptr;
    }
    return dynamic_cast<TechDraw::DrawViewPart * >(References2D.getValues().at(0));
}

int DrawViewDimension::getRefType() const
{
    int refType = invalidRef;
    const std::vector<std::string> &subElements      = References2D.getSubValues();
    if (subElements.size() == 1) {
        refType = getRefType1(subElements[0]);
    } else if (subElements.size() == 2) {
        refType = getRefType2(subElements[0],subElements[1]);
    } else if (subElements.size() == 3) {
        refType = getRefType3(subElements[0],subElements[1],subElements[2]);
    }
    return refType;
}

//static
int DrawViewDimension::getRefType1(const std::string g1)
{
    int refType = invalidRef;
    if (DrawUtil::getGeomTypeFromName(g1) == "Edge") {
        refType = oneEdge;
    }
    return refType;
}

//static
int DrawViewDimension::getRefType2(const std::string g1, const std::string g2)
{
    int refType = invalidRef;
    if ((DrawUtil::getGeomTypeFromName(g1) == "Edge") &&
        (DrawUtil::getGeomTypeFromName(g2) == "Edge")) {
        refType = twoEdge;
    } else if ((DrawUtil::getGeomTypeFromName(g1) == "Vertex") &&
               (DrawUtil::getGeomTypeFromName(g2) == "Vertex")) {
        refType = twoVertex;
    } else if (((DrawUtil::getGeomTypeFromName(g1) == "Vertex") &&
                (DrawUtil::getGeomTypeFromName(g2) == "Edge"))   ||
               ((DrawUtil::getGeomTypeFromName(g1) == "Edge") &&
               (DrawUtil::getGeomTypeFromName(g2) == "Vertex")) ) {
        refType = vertexEdge;
    }
    //} else add different types here - Vertex-Face, ...

    return refType;
}

int DrawViewDimension::getRefType3(const std::string g1,
                                   const std::string g2,
                                   const std::string g3)
{
    int refType = invalidRef;
    if ((DrawUtil::getGeomTypeFromName(g1) == "Vertex") &&
        (DrawUtil::getGeomTypeFromName(g2) == "Vertex") && 
        (DrawUtil::getGeomTypeFromName(g3) == "Vertex") ) {
        refType = threeVertex;
    }

    return refType;
}


//! validate 2D references - only checks if the target exists
bool DrawViewDimension::checkReferences2D() const
{
//    Base::Console().Message("DVD::checkReFerences2d() - %s\n",getNameInDocument());
    bool result = true;
    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    if (!objects.empty()) {
        const std::vector<std::string> &subElements      = References2D.getSubValues();
        if (!subElements.empty()) {
            for (auto& s: subElements) {
                if (!s.empty()) { 
                    int idx = DrawUtil::getIndexFromName(s);
                    if (DrawUtil::getGeomTypeFromName(s) == "Edge") {
                        TechDrawGeometry::BaseGeom* geom = getViewPart()->getProjEdgeByIndex(idx);
                        if (geom == nullptr) {
                            result = false;
                            break;
                        }
                    } else if (DrawUtil::getGeomTypeFromName(s) == "Vertex") {
                        TechDrawGeometry::Vertex* v = getViewPart()->getProjVertexByIndex(idx);
                        if (v == nullptr) {
                            result = false;
                            break;
                        }
                    }
                } else {
                    result = false;
                }
            }
        } else {
            Base::Console().Log("DVD::checkRegerences2d() - %s - subelements empty!\n",getNameInDocument());
            result = false;
        }
    } else {
        Base::Console().Log("DVD::checkRegerences2d() - %s - objects empty!\n",getNameInDocument());
        result = false;
    }
    return result;
}

pointPair DrawViewDimension::closestPoints(TopoDS_Shape s1,
                                           TopoDS_Shape s2) const
{
    pointPair result;
    BRepExtrema_DistShapeShape extss(s1, s2);
    if (!extss.IsDone()) {
        throw Base::RuntimeError("DVD::closestPoints - BRepExtrema_DistShapeShape failed");
    }
    int count = extss.NbSolution();
    if (count != 0) {
        gp_Pnt p = extss.PointOnShape1(1);
        result.first = Base::Vector3d(p.X(),p.Y(),p.Z());
        p = extss.PointOnShape2(1);
        result.second = Base::Vector3d(p.X(),p.Y(),p.Z());
    } //TODO: else { explode }

    return result;
}

//!add Dimension 3D references to measurement
void DrawViewDimension::setAll3DMeasurement()
{
    measurement->clear();
    const std::vector<App::DocumentObject*> &Objs = References3D.getValues();
    const std::vector<std::string> &Subs      = References3D.getSubValues();
    int end = Objs.size();
    int i = 0;
    for ( ; i < end; i++) {
        static_cast<void> (measurement->addReference3D(Objs.at(i), Subs.at(i)));
    }
}

//delete all previous measurements
void DrawViewDimension::clear3DMeasurements()
{
    //set sublinklist to empty?
    measurement->clear();
}

void DrawViewDimension::dumpRefs2D(char* text) const
{
    Base::Console().Message("DUMP - %s\n",text);
    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    const std::vector<std::string> &subElements = References2D.getSubValues();
    std::vector<App::DocumentObject*>::const_iterator objIt = objects.begin();
    std::vector<std::string>::const_iterator subIt = subElements.begin();
    int i = 0;
    for( ;objIt != objects.end();objIt++,subIt++,i++) {
        Base::Console().Message("DUMP - ref: %d object: %s subElement: %s\n",i,(*objIt)->getNameInDocument(),(*subIt).c_str());
    }
}

double DrawViewDimension::dist2Segs(Base::Vector2d s1,
                                       Base::Vector2d e1,
                                       Base::Vector2d s2,
                                       Base::Vector2d e2) const
{
    gp_Pnt start(s1.x,s1.y,0.0);
    gp_Pnt end(e1.x,e1.y,0.0);
    TopoDS_Vertex v1 = BRepBuilderAPI_MakeVertex(start);
    TopoDS_Vertex v2 = BRepBuilderAPI_MakeVertex(end);
    BRepBuilderAPI_MakeEdge makeEdge1(v1,v2);
    TopoDS_Edge edge1 = makeEdge1.Edge();

    start = gp_Pnt(s2.x,s2.y,0.0);
    end = gp_Pnt(e2.x,e2.y,0.0);
    v1 = BRepBuilderAPI_MakeVertex(start);
    v2 = BRepBuilderAPI_MakeVertex(end);
    BRepBuilderAPI_MakeEdge makeEdge2(v1,v2);
    TopoDS_Edge edge2 = makeEdge2.Edge();

    BRepExtrema_DistShapeShape extss(edge1, edge2);
    if (!extss.IsDone()) {
        throw Base::RuntimeError("DVD::dist2Segs - BRepExtrema_DistShapeShape failed");
    }
    int count = extss.NbSolution();
    double minDist = 0.0;
    if (count != 0) {
        minDist = extss.Value();
    } //TODO: else { explode }

    return minDist;
}

bool DrawViewDimension::leaderIntersectsArc(Base::Vector3d s, Base::Vector3d pointOnCircle) {
    bool result = false;
    const std::vector<std::string> &subElements      = References2D.getSubValues();
    int idx = DrawUtil::getIndexFromName(subElements[0]);
    TechDrawGeometry::BaseGeom* base = getViewPart()->getProjEdgeByIndex(idx);
    if( base && base->geomType == TechDrawGeometry::GeomType::ARCOFCIRCLE )  {
        TechDrawGeometry::AOC* aoc = static_cast<TechDrawGeometry::AOC*> (base);
        if (aoc->intersectsArc(s,pointOnCircle)) {
            result = true;
        }
    } else if( base && base->geomType == TechDrawGeometry::GeomType::BSPLINE )  {
        TechDrawGeometry::BSpline* spline = static_cast<TechDrawGeometry::BSpline*> (base);
        if (spline->isCircle()) {
            if (spline->intersectsArc(s,pointOnCircle)) {
                result = true;
            }
        }
    }

    return result;
}

//are there non-blank references?
bool DrawViewDimension::has2DReferences(void) const
{
//    Base::Console().Message("DVD::has2DReferences() - %s\n",getNameInDocument());
    bool result = false;

    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    const std::vector<std::string> &SubNames         = References2D.getSubValues();
    if (!objects.empty()) {
        App::DocumentObject* testRef = objects.at(0);
        if (testRef != nullptr) {
            if (!SubNames.empty()) {
                result = true;              //not empty is good
                for (auto& s: SubNames) {   //but check individual entries
                    if (s.empty()) {
                        result = false;
                        break;
                    }
                }
            }
        }
    }
    return result;
}

bool DrawViewDimension::has3DReferences(void) const
{
    return (References3D.getSize() > 0);
}

bool DrawViewDimension::hasTolerance(void) const
{
    bool result = true;
    double overTol = OverTolerance.getValue();
    double underTol = UnderTolerance.getValue();
    if (DrawUtil::fpCompare(overTol,0.0) &&
        DrawUtil::fpCompare(underTol,0.0) ) {
        result = false;
    }
    return result;
}

bool DrawViewDimension::showUnits() const
{
    bool result = false;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    result = hGrp->GetBool("ShowUnits", true);
    return result;
}

bool DrawViewDimension::useDecimals() const
{
    bool result = false;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    result = hGrp->GetBool("UseGlobalDecimals", true);
    return result;
}

std::string DrawViewDimension::getPrefix() const
{
    std::string result = "";
    if(Type.isValue("Distance")) {
        result = "";
    } else if(Type.isValue("DistanceX")){
        result = "";
    } else if(Type.isValue("DistanceY")){
        result = "";
    } else if(Type.isValue("DistanceZ")){
        result = "";
    } else if(Type.isValue("Radius")){
        result =  "R";
    } else if(Type.isValue("Diameter")){
        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
            .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
        std::string diamSym = hGrp->GetASCII("DiameterSymbol","\xe2\x8c\x80");
        result = diamSym;
    } else if(Type.isValue("Angle")){
        result = "";
    }
    return result;
}

std::string DrawViewDimension::getDefaultFormatSpec() const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                         .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    std::string prefFormat = hGrp->GetASCII("formatSpec","");
    QString formatSpec;
    QString qPrefix;
    if (prefFormat.empty()) {
        QString format1 = Base::Tools::fromStdString("%.");
        QString format2 = Base::Tools::fromStdString("f");
        int precision;
        if (useDecimals()) {
            precision = Base::UnitsApi::getDecimals();
        } else {
            precision = hGrp->GetInt("AltDecimals", 2);
        }
        QString formatPrecision = QString::number(precision);

        std::string prefix = getPrefix();
        
        if (!prefix.empty()) {
            qPrefix = QString::fromUtf8(prefix.data(),prefix.size());
        }

        formatSpec = qPrefix + format1 + formatPrecision + format2;
    } else {
        
        std::string prefix = getPrefix();
        qPrefix = QString::fromUtf8(prefix.data(),prefix.size());
        formatSpec = qPrefix + QString::fromStdString(prefFormat);
        
    }
    
    return Base::Tools::toStdString(formatSpec);
}

PyObject *DrawViewDimension::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewDimensionPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
