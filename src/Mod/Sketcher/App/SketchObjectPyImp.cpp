/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2010     *
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
# include <Geom_TrimmedCurve.hxx>
#endif

#include <boost/shared_ptr.hpp>

#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Part/App/LinePy.h>
#include <Mod/Part/App/Geometry.h>
#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>
#include <Base/AxisPy.h>
#include <Base/Tools.h>
#include <Base/QuantityPy.h>
#include <App/Document.h>
#include <CXX/Objects.hxx>

// inclusion of the generated files (generated out of SketchObjectSFPy.xml)
#include "SketchObjectPy.h"
#include "SketchObjectPy.cpp"
// other python types
#include "ConstraintPy.h"

using namespace Sketcher;

// returns a string which represents the object e.g. when printed in python
std::string SketchObjectPy::representation(void) const
{
    return "<Sketcher::SketchObject>";
}


PyObject* SketchObjectPy::solve(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    int ret = this->getSketchObjectPtr()->solve();
    return Py_BuildValue("i", ret);
}

PyObject* SketchObjectPy::addGeometry(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))
        return 0;

    if (PyObject_TypeCheck(pcObj, &(Part::GeometryPy::Type))) {
        Part::Geometry *geo = static_cast<Part::GeometryPy*>(pcObj)->getGeometryPtr();
        int ret;
        // An arc created with Part.Arc will be converted into a Part.ArcOfCircle
        if (geo->getTypeId() == Part::GeomTrimmedCurve::getClassTypeId()) {
            Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast(geo->handle());
            Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(trim->BasisCurve());
            Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(trim->BasisCurve());
            if (!circle.IsNull()) {
                // create the definition struct for that geom
                Part::GeomArcOfCircle aoc;
                aoc.setHandle(trim);
                ret = this->getSketchObjectPtr()->addGeometry(&aoc);
            }
            else if (!ellipse.IsNull()) {
                // create the definition struct for that geom
                Part::GeomArcOfEllipse aoe;
                aoe.setHandle(trim);
                ret = this->getSketchObjectPtr()->addGeometry(&aoe);
            }             
            else {
                std::stringstream str;
                str << "Unsupported geometry type: " << geo->getTypeId().getName();
                PyErr_SetString(PyExc_TypeError, str.str().c_str());
                return 0;
            }
        }
        else if (geo->getTypeId() == Part::GeomPoint::getClassTypeId() ||
                 geo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                 geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                 geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
                 geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                 geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            ret = this->getSketchObjectPtr()->addGeometry(geo);
        }
        else {
            std::stringstream str;
            str << "Unsupported geometry type: " << geo->getTypeId().getName();
            PyErr_SetString(PyExc_TypeError, str.str().c_str());
            return 0;
        }
        return Py::new_reference_to(Py::Int(ret));
    }
    else if (PyObject_TypeCheck(pcObj, &(PyList_Type)) ||
             PyObject_TypeCheck(pcObj, &(PyTuple_Type))) {
        std::vector<Part::Geometry *> geoList;
        std::vector<boost::shared_ptr <Part::Geometry> > tmpList;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::GeometryPy::Type))) {
                Part::Geometry *geo = static_cast<Part::GeometryPy*>((*it).ptr())->getGeometryPtr();

                // An arc created with Part.Arc will be converted into a Part.ArcOfCircle
                if (geo->getTypeId() == Part::GeomTrimmedCurve::getClassTypeId()) {
                    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast(geo->handle());
                    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(trim->BasisCurve());
                    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(trim->BasisCurve());
                    if (!circle.IsNull()) {
                        // create the definition struct for that geom
                        boost::shared_ptr<Part::GeomArcOfCircle> aoc(new Part::GeomArcOfCircle());
                        aoc->setHandle(trim);
                        geoList.push_back(aoc.get());
                        tmpList.push_back(aoc);
                    }
                    else if (!ellipse.IsNull()) {
                        // create the definition struct for that geom
                        boost::shared_ptr<Part::GeomArcOfEllipse> aoe(new Part::GeomArcOfEllipse());
                        aoe->setHandle(trim);
                        geoList.push_back(aoe.get());
                        tmpList.push_back(aoe);
                    }
                    else {
                        std::stringstream str;
                        str << "Unsupported geometry type: " << geo->getTypeId().getName();
                        PyErr_SetString(PyExc_TypeError, str.str().c_str());
                        return 0;
                    }
                }
                else if (geo->getTypeId() == Part::GeomPoint::getClassTypeId() ||
                         geo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                         geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                         geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
                         geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                         geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                    geoList.push_back(geo);
                }
                else {
                    std::stringstream str;
                    str << "Unsupported geometry type: " << geo->getTypeId().getName();
                    PyErr_SetString(PyExc_TypeError, str.str().c_str());
                    return 0;
                }
            }
        }

        int ret = this->getSketchObjectPtr()->addGeometry(geoList) + 1;
        std::size_t numGeo = geoList.size();
        Py::Tuple tuple(numGeo);
        for (std::size_t i=0; i<numGeo; ++i) {
            int geoId = ret - int(numGeo - i);
            tuple.setItem(i, Py::Int(geoId));
        }

        return Py::new_reference_to(tuple);
    }

    std::string error = std::string("type must be 'Geometry' or list of 'Geometry', not ");
    error += pcObj->ob_type->tp_name;
    throw Py::TypeError(error);
}

PyObject* SketchObjectPy::delGeometry(PyObject *args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return 0;

    if (this->getSketchObjectPtr()->delGeometry(Index)) {
        std::stringstream str;
        str << "Not able to delete a geometry with the given index: " << Index;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::toggleConstruction(PyObject *args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return 0;

    if (this->getSketchObjectPtr()->toggleConstruction(Index)) {
        std::stringstream str;
        str << "Not able to toggle a geometry with the given index: " << Index;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::setConstruction(PyObject *args)
{
    int Index;
    PyObject *Mode;
    if (!PyArg_ParseTuple(args, "iO!", &Index, &PyBool_Type, &Mode))
        return 0;

    if (this->getSketchObjectPtr()->setConstruction(Index, PyObject_IsTrue(Mode) ? true : false)) {
        std::stringstream str;
        str << "Not able to set construction mode of a geometry with the given index: " << Index;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::addConstraint(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))
        return 0;

    if (PyObject_TypeCheck(pcObj, &(Sketcher::ConstraintPy::Type))) {
        Sketcher::Constraint *constr = static_cast<Sketcher::ConstraintPy*>(pcObj)->getConstraintPtr();
        int ret = this->getSketchObjectPtr()->addConstraint(constr);
        this->getSketchObjectPtr()->solve();
        return Py::new_reference_to(Py::Int(ret));
    }
    else if (PyObject_TypeCheck(pcObj, &(PyList_Type)) ||
             PyObject_TypeCheck(pcObj, &(PyTuple_Type))) {
        std::vector<Constraint*> values;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(ConstraintPy::Type))) {
                Constraint *con = static_cast<ConstraintPy*>((*it).ptr())->getConstraintPtr();
                values.push_back(con);
            }
        }

        int ret = getSketchObjectPtr()->addConstraints(values) + 1;
        std::size_t numCon = values.size();
        Py::Tuple tuple(numCon);
        for (std::size_t i=0; i<numCon; ++i) {
            int conId = ret - int(numCon - i);
            tuple.setItem(i, Py::Int(conId));
        }
        return Py::new_reference_to(tuple);
    }

    std::string error = std::string("type must be 'Constraint' or list of 'Constraint', not ");
    error += pcObj->ob_type->tp_name;
    throw Py::TypeError(error);
}

PyObject* SketchObjectPy::delConstraint(PyObject *args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return 0;

    if (this->getSketchObjectPtr()->delConstraint(Index)) {
        std::stringstream str;
        str << "Not able to delete a constraint with the given index: " << Index;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::renameConstraint(PyObject *args)
{
    int Index;
    char* Name;
    if (!PyArg_ParseTuple(args, "is", &Index, &Name))
        return 0;

    if (this->getSketchObjectPtr()->Constraints.getSize() <= Index) {
        std::stringstream str;
        str << "Not able to rename a constraint with the given index: " << Index;
        PyErr_SetString(PyExc_IndexError, str.str().c_str());
        return 0;
    }

    Constraint* copy = this->getSketchObjectPtr()->Constraints[Index]->clone();
    copy->Name = Name;
    this->getSketchObjectPtr()->Constraints.set1Value(Index, copy);
    delete copy;

    Py_Return;
}

PyObject* SketchObjectPy::addExternal(PyObject *args)
{
    char *ObjectName;
    char *SubName;
    if (!PyArg_ParseTuple(args, "ss:Give an object and subelement name", &ObjectName,&SubName))
        return 0;

    // get the target object for the external link
    App::DocumentObject * Obj = this->getSketchObjectPtr()->getDocument()->getObject(ObjectName);
    if (!Obj) {
        std::stringstream str;
        str << ObjectName << "does not exist in the document";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }
    // check if it belongs to the sketch support
    if (this->getSketchObjectPtr()->Support.getValue() != Obj) {
        std::stringstream str;
        str << ObjectName << "is not supported by this sketch";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    // add the external
    if (this->getSketchObjectPtr()->addExternal(Obj,SubName) < 0) {
        std::stringstream str;
        str << "Not able to add external shape element";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::delExternal(PyObject *args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return 0;

    if (this->getSketchObjectPtr()->delExternal(Index)) {
        std::stringstream str;
        str << "Not able to delete an external geometry with the given index: " << Index;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::delConstraintOnPoint(PyObject *args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return 0;

    if (this->getSketchObjectPtr()->delConstraintOnPoint(Index)) {
        std::stringstream str;
        str << "Not able to delete a constraint on point with the given index: " << Index;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::setDatum(PyObject *args)
{
    double Datum;
    int    Index;
    PyObject* object;
    Base::Quantity Quantity;

    do {
        // handle (int,Quantity)
        if (PyArg_ParseTuple(args,"iO!", &Index, &(Base::QuantityPy::Type), &object)) {
            Quantity = *(static_cast<Base::QuantityPy*>(object)->getQuantityPtr());
            if (Quantity.getUnit() == Base::Unit::Angle) {
                Datum = Base::toRadians<double>(Quantity.getValue());
                break;
            }
            else {
                Datum = Quantity.getValue();
                break;
            }
        }

        // handle (int,double)
        PyErr_Clear();
        if (PyArg_ParseTuple(args, "id", &Index, &Datum)) {
            Quantity.setValue(Datum);
            break;
        }

        // handle (string,Quantity)
        char* constrName;
        PyErr_Clear();
        if (PyArg_ParseTuple(args,"sO!", &constrName, &(Base::QuantityPy::Type), &object)) {
            Quantity = *(static_cast<Base::QuantityPy*>(object)->getQuantityPtr());
            if (Quantity.getUnit() == Base::Unit::Angle) {
                Datum = Base::toRadians<double>(Quantity.getValue());
            }
            else {
                Datum = Quantity.getValue();
            }

            int i = 0;
            Index = -1;
            const std::vector<Constraint*>& vals = this->getSketchObjectPtr()->Constraints.getValues();
            for (std::vector<Constraint*>::const_iterator it = vals.begin(); it != vals.end(); ++it, ++i) {
                if ((*it)->Name == constrName) {
                    Index = i;
                    break;
                }
            }

            if (Index >= 0) {
                break;
            }
            else {
                std::stringstream str;
                str << "Invalid constraint name: '" << constrName << "'";
                PyErr_SetString(PyExc_ValueError, str.str().c_str());
                return 0;
            }
        }

        // handle (string,double)
        PyErr_Clear();
        if (PyArg_ParseTuple(args, "sd", &constrName, &Datum)) {
            Quantity.setValue(Datum);
            int i = 0;
            Index = -1;
            const std::vector<Constraint*>& vals = this->getSketchObjectPtr()->Constraints.getValues();
            for (std::vector<Constraint*>::const_iterator it = vals.begin(); it != vals.end(); ++it, ++i) {
                if ((*it)->Name == constrName) {
                    Index = i;
                    break;
                }
            }

            if (Index >= 0) {
                break;
            }
            else {
                std::stringstream str;
                str << "Invalid constraint name: '" << constrName << "'";
                PyErr_SetString(PyExc_ValueError, str.str().c_str());
                return 0;
            }
        }

        // error handling
        PyErr_SetString(PyExc_TypeError, "Wrong arguments");
        return 0;
    }
    while (false);

    int err=this->getSketchObjectPtr()->setDatum(Index, Datum);
    if (err) {
        std::stringstream str;
        if (err == -1)
            str << "Invalid constraint index: " << Index;
        else if (err == -3)
            str << "Cannot set the datum because the sketch contains conflicting constraints";
        else if (err == -2)
            str << "Datum " << (const char*)Quantity.getUserString().toUtf8() << " for the constraint with index " << Index << " is invalid";
        else if (err == -4)
            str << "Negative datum values are not valid for the constraint with index " << Index;
        else if (err == -5)
            str << "Zero is not a valid datum for the constraint with index " << Index;
        else
            str << "Unexpected problem at setting datum " << (const char*)Quantity.getUserString().toUtf8() << " for the constraint with index " << Index;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::movePoint(PyObject *args)
{
    PyObject *pcObj;
    int GeoId, PointType;
    int relative=0;

    if (!PyArg_ParseTuple(args, "iiO!|i", &GeoId, &PointType, &(Base::VectorPy::Type), &pcObj, &relative))
        return 0;

    Base::Vector3d v1 = static_cast<Base::VectorPy*>(pcObj)->value();

    if (this->getSketchObjectPtr()->movePoint(GeoId,(Sketcher::PointPos)PointType,v1,(relative>0))) {
        std::stringstream str;
        str << "Not able to move point with the id and type: (" << GeoId << ", " << PointType << ")";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;

}

PyObject* SketchObjectPy::getPoint(PyObject *args)
{
    int GeoId, PointType;
    if (!PyArg_ParseTuple(args, "ii", &GeoId, &PointType))
        return 0;

    if (PointType < 0 || PointType > 3) {
        PyErr_SetString(PyExc_ValueError, "Invalid point type");
        return 0;
    }

    SketchObject* obj = this->getSketchObjectPtr();
    if (GeoId > obj->getHighestCurveIndex() || -GeoId > obj->getExternalGeometryCount()) {
        PyErr_SetString(PyExc_ValueError, "Invalid geometry Id");
        return 0;
    }

    return new Base::VectorPy(new Base::Vector3d(obj->getPoint(GeoId,(Sketcher::PointPos)PointType)));
}

PyObject* SketchObjectPy::getAxis(PyObject *args)
{
    int AxId;
    if (!PyArg_ParseTuple(args, "i", &AxId))
        return 0;

    return new Base::AxisPy(new Base::Axis(this->getSketchObjectPtr()->getAxis(AxId)));
}

PyObject* SketchObjectPy::fillet(PyObject *args)
{
    PyObject *pcObj1, *pcObj2;
    int geoId1, geoId2, posId1, trim=1;
    double radius;

    // Two Lines, radius
    if (PyArg_ParseTuple(args, "iiO!O!d|i", &geoId1, &geoId2, &(Base::VectorPy::Type), &pcObj1, &(Base::VectorPy::Type), &pcObj2, &radius, &trim)) {

        Base::Vector3d v1 = static_cast<Base::VectorPy*>(pcObj1)->value();
        Base::Vector3d v2 = static_cast<Base::VectorPy*>(pcObj2)->value();

        if (this->getSketchObjectPtr()->fillet(geoId1, geoId2, v1, v2, radius, trim?true:false)) {
            std::stringstream str;
            str << "Not able to fillet lineSegments with ids : (" << geoId1 << ", " << geoId2 << ") and points (" << v1.x << ", " << v1.y << ", " << v1.z << ") & "
            << "(" << v2.x << ", " << v2.y << ", " << v2.z << ")";
            PyErr_SetString(PyExc_ValueError, str.str().c_str());
            return 0;
        }
        Py_Return;
    }

    PyErr_Clear();
    // Point, radius
    if (PyArg_ParseTuple(args, "iid|i", &geoId1, &posId1, &radius, &trim)) {
        if (this->getSketchObjectPtr()->fillet(geoId1, (Sketcher::PointPos) posId1, radius, trim?true:false)) {
            std::stringstream str;
            str << "Not able to fillet point with ( geoId: " << geoId1 << ", PointPos: " << posId1 << " )";
            PyErr_SetString(PyExc_ValueError, str.str().c_str());
            return 0;
        }
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "fillet() method accepts:\n"
    "-- int,int,Vector,Vector,float,[int]\n"
    "-- int,int,float,[int]\n");
    return 0;
}

PyObject* SketchObjectPy::trim(PyObject *args)
{
    PyObject *pcObj;
    int GeoId;

    if (!PyArg_ParseTuple(args, "iO!", &GeoId, &(Base::VectorPy::Type), &pcObj))
        return 0;

    Base::Vector3d v1 = static_cast<Base::VectorPy*>(pcObj)->value();

    if (this->getSketchObjectPtr()->trim(GeoId,v1)) {
        std::stringstream str;
        str << "Not able to trim curve with the given index: " << GeoId;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;

}

Py::Int SketchObjectPy::getConstraintCount(void) const
{
    return Py::Int(this->getSketchObjectPtr()->Constraints.getSize());
}

Py::Int SketchObjectPy::getGeometryCount(void) const
{
    return Py::Int(this->getSketchObjectPtr()->Geometry.getSize());
}

Py::Int SketchObjectPy::getAxisCount(void) const
{
    return Py::Int(this->getSketchObjectPtr()->getAxisCount());
}

PyObject *SketchObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int SketchObjectPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    // search in PropertyList
    App::Property *prop = getSketchObjectPtr()->getPropertyByName(attr);
    if (prop) {
        // Read-only attributes must not be set over its Python interface
        short Type =  getSketchObjectPtr()->getPropertyType(prop);
        if (Type & App::Prop_ReadOnly) {
            std::stringstream s;
            s << "Object attribute '" << attr << "' is read-only";
            throw Py::AttributeError(s.str());
        }

        prop->setPyObject(obj);

        if (strcmp(attr,"Geometry") == 0)
            getSketchObjectPtr()->rebuildVertexIndex();

        return 1;
    }

    return 0;
}
