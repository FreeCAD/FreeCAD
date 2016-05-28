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
# include <boost/regex.hpp>
# include <QString>
# include <QStringList>
# include <QRegExp>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>

#include <Mod/Measure/App/Measurement.h>

#include "DrawViewPart.h"
#include "DrawViewDimension.h"
#include "DrawUtil.h"

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepExtrema_DistShapeShape.hxx>

#include "DrawViewDimensionPy.h"  // generated from DrawViewDimensionPy.xml

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
        twoVertex
    };

DrawViewDimension::DrawViewDimension(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                         .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");

    ADD_PROPERTY_TYPE(References2D,(0,0),"Dimension",(App::PropertyType)(App::Prop_None),"Projected Geometry References");
    ADD_PROPERTY_TYPE(References3D,(0,0),"Dimension",(App::PropertyType)(App::Prop_None),"3D Geometry References");
    ADD_PROPERTY_TYPE(Precision,(2)   ,"Dimension",(App::PropertyType)(App::Prop_None),"Dimension Precision");
    ADD_PROPERTY_TYPE(Font ,(fontName.c_str()),"Dimension",App::Prop_None, "The name of the font to use");
    ADD_PROPERTY_TYPE(Fontsize,(4)    ,"Dimension",(App::PropertyType)(App::Prop_None),"Dimension text size in mm");
    ADD_PROPERTY_TYPE(CentreLines,(0) ,"Dimension",(App::PropertyType)(App::Prop_None),"Dimension Center Lines");
    ADD_PROPERTY_TYPE(ProjDirection ,(0.,0.,1.0), "Dimension",App::Prop_None,"Projection normal direction");
    ADD_PROPERTY_TYPE(FormatSpec,("%value%") ,"Dimension",(App::PropertyType)(App::Prop_None),"Dimension Format");

    Type.setEnums(TypeEnums);                                          //dimension type: length, radius etc
    ADD_PROPERTY(Type,((long)0));

    MeasureType.setEnums(MeasureTypeEnums);
    ADD_PROPERTY(MeasureType, ((long)0));                           //True or Projected measurement

    //hide the DrawView properties that don't apply to Dimensions
    ScaleType.setStatus(App::Property::ReadOnly,true);
    ScaleType.setStatus(App::Property::Hidden,true);
    Scale.setStatus(App::Property::ReadOnly,true);
    Scale.setStatus(App::Property::Hidden,true);
    Rotation.setStatus(App::Property::ReadOnly,true);
    Rotation.setStatus(App::Property::Hidden,true);

    measurement = new Measure::Measurement();
}

DrawViewDimension::~DrawViewDimension()
{
    delete measurement;
    measurement = 0;
}

void DrawViewDimension::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &References2D  ||
            prop == &Precision   ||
            prop == &Font        ||
            prop == &Fontsize    ||
            prop == &CentreLines ||
            prop == &FormatSpec) {
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
        if (prop == &MeasureType) {
            if (MeasureType.isValue("True") && !measurement->has3DReferences()) {
                Base::Console().Warning("Dimension %s missing Reference to 3D model. Must be Projected.\n", getNameInDocument());
                MeasureType.setValue("Projected");
            }
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
        if (prop == &References3D) {                                       //have to rebuild the Measurement object
            clear3DMeasurements();
            set3DMeasurement(References3D.getValues().at(0),References3D.getSubValues());
        }

    DrawView::onChanged(prop);
    }

}

void DrawViewDimension::onDocumentRestored()
{
    if (has3DReferences()) {
        clear3DMeasurements();
        set3DMeasurement(References3D.getValues().at(0),References3D.getSubValues());
    }
}


short DrawViewDimension::mustExecute() const
{
    bool result = 0;
    if (References2D.isTouched() ||
        Type.isTouched() ||
        MeasureType.isTouched()) {
        result =  1;
    } else {
        result = 0;
    }
    return result;
}

App::DocumentObjectExecReturn *DrawViewDimension::execute(void)
{
    if (!has2DReferences()) {                                            //too soon
        return App::DocumentObject::StdReturn;
    }

    //TODO: why not just use View's property directly?
    ProjDirection.setValue(getViewPart()->Direction.getValue());
    XAxisDirection.setValue(getViewPart()->XAxisDirection.getValue());

    //TODO: if MeasureType = Projected and the Projected shape changes, the Dimension may become invalid (see tilted Cube example)

    return App::DocumentObject::StdReturn;
}

std::string  DrawViewDimension::getFormatedValue() const
{

    QString str = QString::fromUtf8(FormatSpec.getStrValue().c_str());

    QRegExp rx(QString::fromAscii("%(\\w+)%"));                        //any word bracketed by %
    QStringList list;
    int pos = 0;

    while ((pos = rx.indexIn(str, pos)) != -1) {
        list << rx.cap(0);
        pos += rx.matchedLength();
    }

    for(QStringList::const_iterator it = list.begin(); it != list.end(); ++it) {
        if(*it == QString::fromAscii("%value%")){
            double val = std::abs(getDimValue());
            str.replace(*it, QString::number(val, 'f', Precision.getValue()) );
        } else {                                                       //insert new placeholder replacement logic here
            str.replace(*it, QString::fromAscii(""));
        }
    }

    return str.toStdString();
}

double DrawViewDimension::getDimValue() const
{
    double result = 0.0;
    if (!has2DReferences()) {                                            //happens during Dimension creation
        Base::Console().Message("INFO - DVD::getDimValue - Dimension has no References\n");
        return result;
    }

    if (!getViewPart()->hasGeometry()) {                              //happens when loading saved document
        Base::Console().Message("INFO - DVD::getDimValue ViewPart has no Geometry yet\n");
        return result;
    }

    if (MeasureType.isValue("True")) {
        // True Values
        if (!measurement->has3DReferences()) {
            return result;
        }
        if(Type.isValue("Distance")) {
            result = measurement->delta().Length();
        } else if(Type.isValue("DistanceX")){
            Base::Vector3d delta = measurement->delta();
            result = delta.x;
        } else if(Type.isValue("DistanceY")){
            Base::Vector3d delta = measurement->delta();
            result = delta.y;
        } else if(Type.isValue("DistanceZ")){
            Base::Vector3d delta = measurement->delta();
            result = delta.z;
        } else if(Type.isValue("Radius")){
            result =  measurement->radius();
        } else if(Type.isValue("Diameter")){
            result =  measurement->radius() * 2.0;
        } else if(Type.isValue("Angle")){
            result = measurement->angle();
        } else {
            throw Base::Exception("getDimValue() - Unknown Dimension Type (1)");
        }
    } else {
        // Projected Values
        const std::vector<App::DocumentObject*> &objects = References2D.getValues();
        const std::vector<std::string> &subElements      = References2D.getSubValues();
        if (Type.isValue("Distance") && getRefType() == oneEdge) {
            //TODO: Check for straight line Edge?
            int idx = DrawUtil::getIndexFromName(subElements[0]);
            TechDrawGeometry::BaseGeom* geom = getViewPart()->getProjEdgeByIndex(idx);
            TechDrawGeometry::Generic* gen = static_cast<TechDrawGeometry::Generic*>(geom);
            Base::Vector2D start = gen->points[0];
            Base::Vector2D end = gen->points[1];
            Base::Vector2D line = end - start;
            result = line.Length() / getViewPart()->Scale.getValue();
        } else if (Type.isValue("Distance") && getRefType() == twoEdge) {
            //only works for straight line edges
            int idx0 = DrawUtil::getIndexFromName(subElements[0]);
            int idx1 = DrawUtil::getIndexFromName(subElements[1]);
            TechDrawGeometry::BaseGeom* geom0 = getViewPart()->getProjEdgeByIndex(idx0);
            TechDrawGeometry::BaseGeom* geom1 = getViewPart()->getProjEdgeByIndex(idx1);
            TechDrawGeometry::Generic* gen0 = static_cast<TechDrawGeometry::Generic*>(geom0);
            TechDrawGeometry::Generic* gen1 = static_cast<TechDrawGeometry::Generic*>(geom1);
            Base::Vector2D s0 = gen0->points[0];
            Base::Vector2D e0 = gen0->points[1];
            Base::Vector2D s1 = gen1->points[0];
            Base::Vector2D e1 = gen1->points[1];
            result = dist2Segs(s0,e0,s1,e1) / getViewPart()->Scale.getValue();
        } else if (Type.isValue("Distance") && getRefType() == twoVertex) {
            int idx0 = DrawUtil::getIndexFromName(subElements[0]);
            int idx1 = DrawUtil::getIndexFromName(subElements[1]);
            TechDrawGeometry::Vertex* v0 = getViewPart()->getProjVertexByIndex(idx0);
            TechDrawGeometry::Vertex* v1 = getViewPart()->getProjVertexByIndex(idx1);
            Base::Vector2D start = v0->pnt;
            Base::Vector2D end = v1->pnt;
            Base::Vector2D line = end - start;
            result = line.Length() / getViewPart()->Scale.getValue();
        } else if (Type.isValue("DistanceX") && getRefType() == oneEdge) {
            int idx = DrawUtil::getIndexFromName(subElements[0]);
            TechDrawGeometry::BaseGeom* geom = getViewPart()->getProjEdgeByIndex(idx);
            TechDrawGeometry::Generic* gen = static_cast<TechDrawGeometry::Generic*>(geom);
            Base::Vector2D start = gen->points[0];
            Base::Vector2D end = gen->points[1];
            Base::Vector2D line = end - start;
            return fabs(line.fX) / getViewPart()->Scale.getValue();
        } else if (Type.isValue("DistanceY") && getRefType() == oneEdge) {
            int idx = DrawUtil::getIndexFromName(subElements[0]);
            TechDrawGeometry::BaseGeom* geom = getViewPart()->getProjEdgeByIndex(idx);
            TechDrawGeometry::Generic* gen = static_cast<TechDrawGeometry::Generic*>(geom);
            Base::Vector2D start = gen->points[0];
            Base::Vector2D end = gen->points[1];
            Base::Vector2D line = end - start;
            result = fabs(line.fY) / getViewPart()->Scale.getValue();
        } else if (Type.isValue("DistanceX") && getRefType() == twoVertex) {
            int idx0 = DrawUtil::getIndexFromName(subElements[0]);
            int idx1 = DrawUtil::getIndexFromName(subElements[1]);
            TechDrawGeometry::Vertex* v0 = getViewPart()->getProjVertexByIndex(idx0);
            TechDrawGeometry::Vertex* v1 = getViewPart()->getProjVertexByIndex(idx1);
            Base::Vector2D start = v0->pnt;
            Base::Vector2D end = v1->pnt;
            Base::Vector2D line = end - start;
            result = fabs(line.fX) / getViewPart()->Scale.getValue();
        } else if (Type.isValue("DistanceY") && getRefType() == twoVertex) {
            int idx0 = DrawUtil::getIndexFromName(subElements[0]);
            int idx1 = DrawUtil::getIndexFromName(subElements[1]);
            TechDrawGeometry::Vertex* v0 = getViewPart()->getProjVertexByIndex(idx0);
            TechDrawGeometry::Vertex* v1 = getViewPart()->getProjVertexByIndex(idx1);
            Base::Vector2D start = v0->pnt;
            Base::Vector2D end = v1->pnt;
            Base::Vector2D line = end - start;
            result = fabs(line.fY) / getViewPart()->Scale.getValue();
        } else if(Type.isValue("Radius")){
            //only 1 reference for a Radius
            int idx = DrawUtil::getIndexFromName(subElements[0]);
            TechDrawGeometry::BaseGeom* base = getViewPart()->getProjEdgeByIndex(idx);
            TechDrawGeometry::Circle* circle = static_cast<TechDrawGeometry::Circle*> (base);
            result = circle->radius / getViewPart()->Scale.getValue();            //Projected BaseGeom is scaled for drawing
        } else if(Type.isValue("Diameter")){
            //only 1 reference for a Diameter
            int idx = DrawUtil::getIndexFromName(subElements[0]);
            TechDrawGeometry::BaseGeom* base = getViewPart()->getProjEdgeByIndex(idx);
            TechDrawGeometry::Circle* circle = static_cast<TechDrawGeometry::Circle*> (base);
            result = (circle->radius  * 2.0) / getViewPart()->Scale.getValue();   //Projected BaseGeom is scaled for drawing
        } else if(Type.isValue("Angle")){
            // Must project lines to 2D so cannot use measurement framework this time
            //Relcalculate the measurement based on references stored.
            //WF: why not use projected geom in GeomObject and Vector2D.GetAngle? intersection pt & direction issues?
            //TODO: do we need to distinguish inner vs outer angle? -wf
//            if(subElements.size() != 2) {
//                throw Base::Exception("FVD - Two references required for angle measurement");
//            }
            if (getRefType() != twoEdge) {
                throw Base::Exception("FVD - Two edge references required for angle measurement");
            }
            int idx0 = DrawUtil::getIndexFromName(subElements[0]);
            int idx1 = DrawUtil::getIndexFromName(subElements[1]);
            TechDraw::DrawViewPart *viewPart = dynamic_cast<TechDraw::DrawViewPart *>(objects[0]);
            TechDrawGeometry::BaseGeom* edge0 = viewPart->getProjEdgeByIndex(idx0);
            TechDrawGeometry::BaseGeom* edge1 = viewPart->getProjEdgeByIndex(idx1);

            // Only can find angles with straight line edges
            if(edge0->geomType == TechDrawGeometry::GENERIC &&
               edge1->geomType == TechDrawGeometry::GENERIC) {
                TechDrawGeometry::Generic *gen1 = static_cast<TechDrawGeometry::Generic *>(edge0);
                TechDrawGeometry::Generic *gen2 = static_cast<TechDrawGeometry::Generic *>(edge1);

                Base::Vector3d p1S(gen1->points.at(0).fX, gen1->points.at(0).fY, 0.);
                Base::Vector3d p1E(gen1->points.at(1).fX, gen1->points.at(1).fY, 0.);

                Base::Vector3d p2S(gen2->points.at(0).fX, gen2->points.at(0).fY, 0.);
                Base::Vector3d p2E(gen2->points.at(1).fX, gen2->points.at(1).fY, 0.);

                Base::Vector3d dir1 = p1E - p1S;
                Base::Vector3d dir2 = p2E - p2S;

                // Line Intersetion (taken from ViewProviderSketch.cpp)
                double det = dir1.x*dir2.y - dir1.y*dir2.x;
                if ((det > 0 ? det : -det) < 1e-10)
                    throw Base::Exception("Invalid selection - Det = 0");

                double c1 = dir1.y*gen1->points.at(0).fX - dir1.x*gen1->points.at(0).fY;
                double c2 = dir2.y*gen2->points.at(1).fX - dir2.x*gen2->points.at(1).fY;
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
            }
        }  //endif Angle
    } //endif Projected
    return result;
}

DrawViewPart* DrawViewDimension::getViewPart() const
{
    //TODO: range_check here if no References.  valid situation during Dimension creation.  what happens if return NULL??
    //need checks everywhere?
    return dynamic_cast<TechDraw::DrawViewPart * >(References2D.getValues().at(0));
}

int DrawViewDimension::getRefType() const
{
    int refType = invalidRef;
    const std::vector<std::string> &subElements      = References2D.getSubValues();
    if ((subElements.size() == 1) &&
        (DrawUtil::getGeomTypeFromName(subElements[0]) == "Edge")) {
        refType = oneEdge;
    } else if (subElements.size() == 2) {
        if ((DrawUtil::getGeomTypeFromName(subElements[0]) == "Edge") &&
            (DrawUtil::getGeomTypeFromName(subElements[1]) == "Edge")) {
            refType = twoEdge;
        } else if ((DrawUtil::getGeomTypeFromName(subElements[0]) == "Vertex") &&
                   (DrawUtil::getGeomTypeFromName(subElements[1]) == "Vertex")) {
            refType = twoVertex;
        }
    //} else add different types here - Vertex-Edge, Vertex-Face, ...
    }
    return refType;
}

//!add 1 3D measurement Reference
void DrawViewDimension::set3DMeasurement(DocumentObject* const &obj, const std::vector<std::string>& subElements)
{
   std::vector<std::string>::const_iterator itSub = subElements.begin();
   for (; itSub != subElements.end(); itSub++) {
       //int rc =
       static_cast<void> (measurement->addReference3D(obj,(*itSub).c_str()));
   }
}

//delete all previous measurements
void DrawViewDimension::clear3DMeasurements()
{
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

double DrawViewDimension::dist2Segs(Base::Vector2D s1,
                                       Base::Vector2D e1,
                                       Base::Vector2D s2,
                                       Base::Vector2D e2) const
{
    gp_Pnt start(s1.fX,s1.fY,0.0);
    gp_Pnt end(e1.fX,e1.fY,0.0);
    TopoDS_Vertex v1 = BRepBuilderAPI_MakeVertex(start);
    TopoDS_Vertex v2 = BRepBuilderAPI_MakeVertex(end);
    BRepBuilderAPI_MakeEdge makeEdge1(v1,v2);
    TopoDS_Edge edge1 = makeEdge1.Edge();

    start = gp_Pnt(s2.fX,s2.fY,0.0);
    end = gp_Pnt(e2.fX,e2.fY,0.0);
    v1 = BRepBuilderAPI_MakeVertex(start);
    v2 = BRepBuilderAPI_MakeVertex(end);
    BRepBuilderAPI_MakeEdge makeEdge2(v1,v2);
    TopoDS_Edge edge2 = makeEdge2.Edge();

    BRepExtrema_DistShapeShape extss(edge1, edge2);
    if (!extss.IsDone()) {
        throw Base::Exception("FVD - BRepExtrema_DistShapeShape failed");
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
    return (References2D.getSize() > 0);
}

bool DrawViewDimension::has3DReferences(void) const
{
    return (References3D.getSize() > 0);
}

PyObject *DrawViewDimension::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewDimensionPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
