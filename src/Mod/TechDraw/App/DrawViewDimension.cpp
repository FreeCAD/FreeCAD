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
#endif

#include <QLocale>

#include <App/Application.h>
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
                                                NULL};

const char* DrawViewDimension::MeasureTypeEnums[]= {"True",
                                                    "Projected",
                                                    NULL};

enum RefType{
        invalidRef,
        oneEdge,
        twoEdge,
        twoVertex,
        vertexEdge
    };

DrawViewDimension::DrawViewDimension(void)
{
    ADD_PROPERTY_TYPE(References2D,(0,0),"",(App::PropertyType)(App::Prop_None),"Projected Geometry References");
    References2D.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(References3D,(0,0),"",(App::PropertyType)(App::Prop_None),"3D Geometry References");
    References3D.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(FormatSpec,(getDefaultFormatSpec().c_str()) ,
                  "Format",(App::PropertyType)(App::Prop_None),"Dimension Format");

    Type.setEnums(TypeEnums);                                          //dimension type: length, radius etc
    ADD_PROPERTY(Type,((long)0));
    MeasureType.setEnums(MeasureTypeEnums);
    ADD_PROPERTY(MeasureType, ((long)1));                             //Projected (or True) measurement


    //hide the properties the user can't edit in the property editor
    References2D.setStatus(App::Property::Hidden,true);
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
                Base::Console().Warning("Dimension %s missing Reference to 3D model. Must be Projected.\n", getNameInDocument());
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

    DrawView::onChanged(prop);
    }

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
    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawViewDimension::execute(void)
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    if (!has2DReferences()) {                                            //too soon
        return App::DocumentObject::StdReturn;
    }

    //TODO: if MeasureType = Projected and the Projected shape changes, the Dimension may become invalid (see tilted Cube example)
    requestPaint();

    return App::DocumentObject::execute();;
}

std::string  DrawViewDimension::getFormatedValue()
{
    std::string result;
    QString specStr = QString::fromUtf8(FormatSpec.getStrValue().data(),FormatSpec.getStrValue().size());
    double val = std::abs(getDimValue());    //internal units!

    Base::Quantity qVal;
    qVal.setValue(val);
    if (Type.isValue("Angle")) {
        qVal.setUnit(Base::Unit::Angle);
    } else {
        qVal.setUnit(Base::Unit::Length);
    }
    QString userStr = qVal.getUserString();                           //this handles mm to inch/km/parsec etc and decimal positions
                                                                      //but won't give more than Global_Decimals precision
                                                                      //really should be able to ask units for value in appropriate UoM!!
    QRegExp rxUnits(QString::fromUtf8(" \\D*$"));                     //space + any non digits at end of string

    QString userVal = userStr;
    userVal.remove(rxUnits);                                           //getUserString(defaultDecimals) without units

    QLocale loc;
    double userValNum = loc.toDouble(userVal);

    QString userUnits;
    int pos = 0;
    if ((pos = rxUnits.indexIn(userStr, 0)) != -1)  {
        userUnits = rxUnits.cap(0);                                       //entire capture - non numerics at end of userString
    }

    std::string prefixSym = getPrefix();                                  //get Radius/Diameter/... symbol

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
    }

    QString repl = userVal;
    if (useDecimals()) {
        if (showUnits()) {
            repl = userStr;
        } else {
            repl = userVal;
        }
    } else {
        if (showUnits()) {
            repl = specVal + userUnits;
        } else {
            repl = specVal;
        }
    }

    repl = Base::Tools::fromStdString(getPrefix()) + repl;
    specStr.replace(match,repl);
    //this next bit is so inelegant!!!
    QChar dp = QChar::fromLatin1('.');
    if (loc.decimalPoint() != dp) {
        specStr.replace(dp,loc.decimalPoint());
    }

    return specStr.toUtf8().constData();
}

//!NOTE: this returns the Dimension value in internal units (ie mm)!!!!
double DrawViewDimension::getDimValue()
{
    double result = 0.0;
    if (!has2DReferences()) {                                            //happens during Dimension creation
        Base::Console().Log("INFO - DVD::getDimValue - Dimension has no References\n");
        return result;
    }

    if (!getViewPart()->hasGeometry()) {                              //happens when loading saved document
        Base::Console().Log("INFO - DVD::getDimValue ViewPart has no Geometry yet\n");
        return result;
    }

    if (MeasureType.isValue("True")) {
        // True Values
        if (!measurement->has3DReferences()) {
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
            throw Base::Exception("getDimValue() - Unknown Dimension Type (3)");
        }
    } else {
        // Projected Values
        const std::vector<App::DocumentObject*> &objects = References2D.getValues();
        const std::vector<std::string> &subElements      = References2D.getSubValues();

        if (!checkReferences2D()) {
            Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
            References2D.setValue(nullptr,"");
            return result;
        }
        if ( Type.isValue("Distance")  ||
             Type.isValue("DistanceX") ||
             Type.isValue("DistanceY") )  {
            if (getRefType() == oneEdge) {
                //TODO: Check for straight line Edge?
                int idx = DrawUtil::getIndexFromName(subElements[0]);
                TechDrawGeometry::BaseGeom* geom = getViewPart()->getProjEdgeByIndex(idx);
                TechDrawGeometry::Generic* gen;
                if (geom && geom->geomType == TechDrawGeometry::GeomType::GENERIC) {
                    gen = static_cast<TechDrawGeometry::Generic*>(geom);
                } else {
                    Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
                    References2D.setValue(nullptr,"");
                    return result;
                }
                Base::Vector2d start = gen->points[0];
                Base::Vector2d end = gen->points[1];
                Base::Vector2d line = end - start;
                if (Type.isValue("Distance")) {
                    result = line.Length() / getViewPart()->getScale();
                } else if (Type.isValue("DistanceX")) {
                    return fabs(line.x) / getViewPart()->getScale();
                } else {
                    result = fabs(line.y) / getViewPart()->getScale();
                }
            }else if (getRefType() == twoEdge) {
                //only works for straight line edges
                int idx0 = DrawUtil::getIndexFromName(subElements[0]);
                int idx1 = DrawUtil::getIndexFromName(subElements[1]);
                TechDrawGeometry::BaseGeom* geom0 = getViewPart()->getProjEdgeByIndex(idx0);
                TechDrawGeometry::Generic* gen0;
                if (geom0 && geom0->geomType == TechDrawGeometry::GeomType::GENERIC) {
                    gen0 = static_cast<TechDrawGeometry::Generic*>(geom0);
                } else {
                    Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
                    References2D.setValue(nullptr,"");
                    return result;
                }
                TechDrawGeometry::BaseGeom* geom1 = getViewPart()->getProjEdgeByIndex(idx1);
                TechDrawGeometry::Generic* gen1;
                if (geom1 && geom1->geomType == TechDrawGeometry::GeomType::GENERIC) {
                    gen1 = static_cast<TechDrawGeometry::Generic*>(geom1);
                } else {
                    Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
                    References2D.setValue(nullptr,"");
                    return result;
                }
                Base::Vector2d s0 = gen0->points[0];
                Base::Vector2d e0 = gen0->points[1];
                Base::Vector2d s1 = gen1->points[0];
                Base::Vector2d e1 = gen1->points[1];
                if (Type.isValue("Distance")) {
                    result = dist2Segs(s0,e0,s1,e1) / getViewPart()->getScale();
                } else if (Type.isValue("DistanceX")) {
                    Base::Vector2d p1 = geom0->nearPoint(geom1);
                    Base::Vector2d p2 = geom1->nearPoint(geom0);
                    result = fabs(p1.x - p2.x) / getViewPart()->getScale();
                } else if (Type.isValue("DistanceY")) {
                    Base::Vector2d p1 = geom0->nearPoint(geom1);
                    Base::Vector2d p2 = geom1->nearPoint(geom0);
                    result = fabs(p1.y - p2.y) / getViewPart()->getScale();
                }
            } else if (getRefType() == twoVertex) {
                int idx0 = DrawUtil::getIndexFromName(subElements[0]);
                int idx1 = DrawUtil::getIndexFromName(subElements[1]);
                TechDrawGeometry::Vertex* v0 = getViewPart()->getProjVertexByIndex(idx0);
                TechDrawGeometry::Vertex* v1 = getViewPart()->getProjVertexByIndex(idx1);
                if ((v0 == nullptr) ||
                    (v1 == nullptr) ) {
                    Base::Console().Error("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
                    References2D.setValue(nullptr,"");
                    return result;
                }
                Base::Vector2d start = v0->pnt;    //v0 != nullptr, but v0->pnt is invalid
                Base::Vector2d end = v1->pnt;
                Base::Vector2d line = end - start;
                if (Type.isValue("Distance")) {
                    result = line.Length() / getViewPart()->getScale();
                } else if (Type.isValue("DistanceX")) {
                    result = fabs(line.x) / getViewPart()->getScale();
                } else {
                    result = fabs(line.y) / getViewPart()->getScale();
                }
            } else if (getRefType() == vertexEdge) {
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
                    Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
                    References2D.setValue(nullptr,"");
                    return result;
                }
                Base::Vector2d nearPoint = e->nearPoint(v->pnt);
                Base::Vector2d line = nearPoint - v->pnt;
                if (Type.isValue("Distance")) {
                    result = e->minDist(v->pnt) / getViewPart()->getScale();
                } else if (Type.isValue("DistanceX")) {
                    result = fabs(line.x) / getViewPart()->getScale();
                } else {
                    result = fabs(line.y) / getViewPart()->getScale();
                }
            }  //else tarfu
        } else if(Type.isValue("Radius")){
            //only 1 reference for a Radius
            int idx = DrawUtil::getIndexFromName(subElements[0]);
            TechDrawGeometry::BaseGeom* base = getViewPart()->getProjEdgeByIndex(idx);
            TechDrawGeometry::Circle* circle;
                if( (base && base->geomType == TechDrawGeometry::GeomType::CIRCLE) || 
                   (base && base->geomType == TechDrawGeometry::GeomType::ARCOFCIRCLE))  {
                    circle = static_cast<TechDrawGeometry::Circle*> (base);
                } else {
                    Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
                    References2D.setValue(nullptr,"");
                    return result;
                }
            result = circle->radius / getViewPart()->getScale();            //Projected BaseGeom is scaled for drawing
            
        } else if(Type.isValue("Diameter")){
            //only 1 reference for a Diameter
            int idx = DrawUtil::getIndexFromName(subElements[0]);
            TechDrawGeometry::BaseGeom* base = getViewPart()->getProjEdgeByIndex(idx);
            TechDrawGeometry::Circle* circle;
            if ((base && base->geomType == TechDrawGeometry::GeomType::CIRCLE) || 
               (base && base->geomType == TechDrawGeometry::GeomType::ARCOFCIRCLE)) {
                circle = static_cast<TechDrawGeometry::Circle*> (base);
            } else {
                return result;
            }
            result = (circle->radius  * 2.0) / getViewPart()->getScale();   //Projected BaseGeom is scaled for drawing
        } else if(Type.isValue("Angle")){
            // Must project lines to 2D so cannot use measurement framework this time
            //Relcalculate the measurement based on references stored.
            //WF: why not use projected geom in GeomObject and Vector2d.GetAngle? intersection pt & direction issues?
            //TODO: do we need to distinguish inner vs outer angle? -wf
            if (getRefType() != twoEdge) {
                 Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
                 References2D.setValue(nullptr,"");
                 return result;
            }
            int idx0 = DrawUtil::getIndexFromName(subElements[0]);
            int idx1 = DrawUtil::getIndexFromName(subElements[1]);
            auto viewPart( dynamic_cast<TechDraw::DrawViewPart *>(objects[0]) );
            if( viewPart == nullptr ) {
                Base::Console().Log("INFO - DVD::getDimValue - References2D not DrawViewPart\n");
                return result;
            }
            TechDrawGeometry::BaseGeom* edge0 = viewPart->getProjEdgeByIndex(idx0);
            TechDrawGeometry::BaseGeom* edge1 = viewPart->getProjEdgeByIndex(idx1);
            TechDrawGeometry::Generic *gen1;
            TechDrawGeometry::Generic *gen2;
            if (edge0 && edge0->geomType == TechDrawGeometry::GeomType::GENERIC) {
                 gen1 = static_cast<TechDrawGeometry::Generic*>(edge0);
            } else {
                 Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
                 References2D.setValue(nullptr,"");
                 return result;
            }
            if (edge1 && edge1->geomType == TechDrawGeometry::GeomType::GENERIC) {
                 gen2 = static_cast<TechDrawGeometry::Generic*>(edge1);
            } else {
                 Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
                 References2D.setValue(nullptr,"");
                 return result;
            }

            Base::Vector3d p1S(gen1->points.at(0).x, gen1->points.at(0).y, 0.);
            Base::Vector3d p1E(gen1->points.at(1).x, gen1->points.at(1).y, 0.);

            Base::Vector3d p2S(gen2->points.at(0).x, gen2->points.at(0).y, 0.);
            Base::Vector3d p2E(gen2->points.at(1).x, gen2->points.at(1).y, 0.);

            Base::Vector3d dir1 = p1E - p1S;
            Base::Vector3d dir2 = p2E - p2S;

            // Line Intersetion (taken from ViewProviderSketch.cpp)
            double det = dir1.x*dir2.y - dir1.y*dir2.x;
            if ((det > 0 ? det : -det) < 1e-10)
                throw Base::Exception("Invalid selection - Det = 0");

            double c1 = dir1.y*gen1->points.at(0).x - dir1.x*gen1->points.at(0).y;
            double c2 = dir2.y*gen2->points.at(1).x - dir2.x*gen2->points.at(1).y;
            double x = (dir1.x*c2 - dir2.x*c1)/det;
            double y = (dir1.y*c2 - dir2.y*c1)/det;

            // Intersection point
            Base::Vector3d p0 = Base::Vector3d(x,y,0);

            Base::Vector3d lPos((double) X.getValue(), (double) Y.getValue(), 0.);
            //Base::Vector3d delta = lPos - p0;

            // Create vectors point towards intersection always
            Base::Vector3d a = -p0, b = -p0;
            a += ((p1S - p0).Length() < FLT_EPSILON) ? p1E : p1S;
            b += ((p2S - p0).Length() < FLT_EPSILON) ? p2E : p2S;

            double angle2 = atan2( a.x*b.y - a.y*b.x, a.x*b.x + a.y*b.y );
            result = angle2 * 180. / M_PI;
        } else {
            throw Base::Exception("getDimValue() - Unknown Dimension Type (2)");
        }  //endif Angle
    } //endif Projected
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

//! validate 2D references - only checks if they exist, not if they are the right type
bool DrawViewDimension::checkReferences2D() const
{
    bool result = true;
    //const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    const std::vector<std::string> &subElements      = References2D.getSubValues();

    for (auto& s: subElements) {
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
    }
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
        throw Base::Exception("DVD - BRepExtrema_DistShapeShape failed");
    }
    int count = extss.NbSolution();
    double minDist = 0.0;
    if (count != 0) {
        minDist = extss.Value();
    } //TODO: else { explode }

    return minDist;
}

bool DrawViewDimension::has2DReferences(void) const
{
    bool result = false;
    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    const std::vector<std::string> &SubNames         = References2D.getSubValues();
    if (!objects.empty()) {
        App::DocumentObject* testRef = objects.at(0);
        if (testRef != nullptr) {
            if (!SubNames.empty()) {
                result = true;
            }
        }
    }
    return result;
}

bool DrawViewDimension::has3DReferences(void) const
{
    return (References3D.getSize() > 0);
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
    QString format1 = Base::Tools::fromStdString("%.");
    QString format2 = Base::Tools::fromStdString("f");
    int precision;
    if (useDecimals()) {
        precision = Base::UnitsApi::getDecimals();
    } else {
        precision = hGrp->GetInt("AltDecimals", 2);
    }
    QString formatPrecision = QString::number(precision);
    QString formatSpec = format1 + formatPrecision + format2;
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
