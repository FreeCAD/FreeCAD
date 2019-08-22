/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2010     *
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
# include <boost/shared_ptr.hpp>
#endif

#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Part/App/LinePy.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>
#include <Base/AxisPy.h>
#include <Base/Tools.h>
#include <Base/QuantityPy.h>
#include <App/Document.h>
#include <App/OriginFeature.h>
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
    PyObject* construction; // this is an optional argument default false
    bool isConstruction;
    if (!PyArg_ParseTuple(args, "OO!", &pcObj, &PyBool_Type, &construction)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "O", &pcObj))
            return 0;
        else
            isConstruction=false;
    }
    else {
        isConstruction = PyObject_IsTrue(construction) ? true : false;
    }

    if (PyObject_TypeCheck(pcObj, &(Part::GeometryPy::Type))) {
        Part::Geometry *geo = static_cast<Part::GeometryPy*>(pcObj)->getGeometryPtr();
        int ret;
        // An arc created with Part.Arc will be converted into a Part.ArcOfCircle
        if (geo->getTypeId() == Part::GeomTrimmedCurve::getClassTypeId()) {
            Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast(geo->handle());
            Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(trim->BasisCurve());
            Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(trim->BasisCurve());
            if (!circle.IsNull()) {
                // create the definition struct for that geom
                Part::GeomArcOfCircle aoc;
                aoc.setHandle(trim);
                ret = this->getSketchObjectPtr()->addGeometry(&aoc,isConstruction);
            }
            else if (!ellipse.IsNull()) {
                // create the definition struct for that geom
                Part::GeomArcOfEllipse aoe;
                aoe.setHandle(trim);
                ret = this->getSketchObjectPtr()->addGeometry(&aoe,isConstruction);
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
                 geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                 geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ||
                 geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ||
                 geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            ret = this->getSketchObjectPtr()->addGeometry(geo,isConstruction);
        }
        else {
            std::stringstream str;
            str << "Unsupported geometry type: " << geo->getTypeId().getName();
            PyErr_SetString(PyExc_TypeError, str.str().c_str());
            return 0;
        }
        return Py::new_reference_to(Py::Long(ret));
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
                    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast(geo->handle());
                    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(trim->BasisCurve());
                    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(trim->BasisCurve());
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
                         geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                         geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ||
                         geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ||
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

        int ret = this->getSketchObjectPtr()->addGeometry(geoList,isConstruction) + 1;
        std::size_t numGeo = geoList.size();
        Py::Tuple tuple(numGeo);
        for (std::size_t i=0; i<numGeo; ++i) {
            int geoId = ret - int(numGeo - i);
            tuple.setItem(i, Py::Long(geoId));
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

PyObject* SketchObjectPy::deleteAllGeometry(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    if (this->getSketchObjectPtr()->deleteAllGeometry()) {
        std::stringstream str;
        str << "Unable to delete Geometry";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::deleteAllConstraints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    if (this->getSketchObjectPtr()->deleteAllConstraints()) {
        std::stringstream str;
        str << "Unable to delete Constraints";
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
        if (!this->getSketchObjectPtr()->evaluateConstraint(constr)) {
            PyErr_SetString(PyExc_IndexError, "Constraint has invalid indexes");
            return 0;
        }
        int ret = this->getSketchObjectPtr()->addConstraint(constr);
        // this solve is necessary because:
        // 1. The addition of constraint is part of a command addition
        // 2. This solve happens before the command is committed
        // 3. A constraint, may effect a geometry change (think of coincident,
        // a line's point moves to meet the other line's point
        // 4. The transaction is committed before any other solve, for example
        // the one of execute() triggered by a recompute (UpdateActive) is generated.
        // 5. Upon "undo", the constraint is removed (it was before the command was committed)
        //    however, the geometry changed after the command was committed, so the point that
        //    moved do not go back to the position where it was.
        //
        // N.B.: However, the solve itself may be inhibited in cases where groups of geometry/constraints
        //      are added together, because in that case undoing will also make the geometry disappear.
        this->getSketchObjectPtr()->solve();
        // if the geometry moved during the solve, then the initial solution is invalid
        // at this point, so a point movement may not work in cases where redundant constraints exist.
        // this forces recalculation of the initial solution (not a full solve)
        if(this->getSketchObjectPtr()->noRecomputes) {
            this->getSketchObjectPtr()->setUpSketch();
            this->getSketchObjectPtr()->Constraints.touch(); // update solver information
        }
        return Py::new_reference_to(Py::Long(ret));
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

        for (std::vector<Constraint*>::iterator it = values.begin(); it != values.end(); ++it) {
            if (!this->getSketchObjectPtr()->evaluateConstraint(*it)) {
                PyErr_SetString(PyExc_IndexError, "Constraint has invalid indexes");
                return 0;
            }
        }
        int ret = getSketchObjectPtr()->addConstraints(values) + 1;
        std::size_t numCon = values.size();
        Py::Tuple tuple(numCon);
        for (std::size_t i=0; i<numCon; ++i) {
            int conId = ret - int(numCon - i);
            tuple.setItem(i, Py::Long(conId));
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
    char* utf8Name;
    if (!PyArg_ParseTuple(args, "iet", &Index, "utf-8", &utf8Name))
        return 0;

    std::string Name = utf8Name;
    PyMem_Free(utf8Name);

    if (this->getSketchObjectPtr()->Constraints.getSize() <= Index) {
        std::stringstream str;
        str << "Not able to rename a constraint with the given index: " << Index;
        PyErr_SetString(PyExc_IndexError, str.str().c_str());
        return 0;
    }

    if (!Name.empty()) {

        if (!Sketcher::PropertyConstraintList::validConstraintName(Name)) {
            std::stringstream str;
            str << "Invalid constraint name with the given index: " << Index;
            PyErr_SetString(PyExc_IndexError, str.str().c_str());
            return 0;
        }

        const std::vector< Sketcher::Constraint * > &vals = getSketchObjectPtr()->Constraints.getValues();
        for (std::size_t i = 0; i < vals.size(); ++i) {
            if (static_cast<int>(i) != Index && Name == vals[i]->Name) {
                PyErr_SetString(PyExc_ValueError, "Duplicate constraint not allowed");
                return 0;
            }
        }
    }

    // only change the constraint item if the names are different
    const Constraint* item = this->getSketchObjectPtr()->Constraints[Index];
    if (item->Name != Name) {
        Constraint* copy = item->clone();
        copy->Name = Name;
        this->getSketchObjectPtr()->Constraints.set1Value(Index, copy);
        delete copy;
    }
    Py_Return;
}

PyObject* SketchObjectPy::carbonCopy(PyObject *args)
{
    char *ObjectName;
    PyObject *construction = Py_True;
    if (!PyArg_ParseTuple(args, "s|O!:Give an object", &ObjectName, &PyBool_Type, &construction))
        return 0;

    Sketcher::SketchObject* skObj = this->getSketchObjectPtr();
    App::DocumentObject * Obj = skObj->getDocument()->getObject(ObjectName);

    if (!Obj) {
        std::stringstream str;
        str << ObjectName << " does not exist in the document";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }
    // check if this type of external geometry is allowed
    if (!skObj->isExternalAllowed(Obj->getDocument(), Obj) && (Obj->getTypeId() != Sketcher::SketchObject::getClassTypeId())) {
        std::stringstream str;
        str << ObjectName << " is not allowed for a carbon copy operation in this sketch";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    // add the external
    if (skObj->carbonCopy(Obj, PyObject_IsTrue(construction) ? true : false) < 0) {
        std::stringstream str;
        str << "Not able to add the requested geometry";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::addExternal(PyObject *args)
{
    char *ObjectName;
    char *SubName;
    if (!PyArg_ParseTuple(args, "ss:Give an object and subelement name", &ObjectName,&SubName))
        return 0;

    // get the target object for the external link
    Sketcher::SketchObject* skObj = this->getSketchObjectPtr();
    App::DocumentObject * Obj = skObj->getDocument()->getObject(ObjectName);
    if (!Obj) {
        std::stringstream str;
        str << ObjectName << " does not exist in the document";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }
    // check if this type of external geometry is allowed
    if (!skObj->isExternalAllowed(Obj->getDocument(), Obj)) {
        std::stringstream str;
        str << ObjectName << " is not allowed as external geometry of this sketch";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    // add the external
    if (skObj->addExternal(Obj,SubName) < 0) {
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
    int Index, pos=-1;
    if (!PyArg_ParseTuple(args, "i|i", &Index, &pos))
        return 0;

    if (pos>=0 && pos<3) { // Sketcher::none Sketcher::mid
        if (this->getSketchObjectPtr()->delConstraintOnPoint(Index,(Sketcher::PointPos)pos)) {
            std::stringstream str;
            str << "Not able to delete a constraint on point with the given index: " << Index
                << " and position: " << pos;
            PyErr_SetString(PyExc_ValueError, str.str().c_str());
            return 0;
        }
    }
    else if (pos==-1) {
        if (this->getSketchObjectPtr()->delConstraintOnPoint(Index)) {
            std::stringstream str;
            str << "Not able to delete a constraint on point with the given index: " << Index;
            PyErr_SetString(PyExc_ValueError, str.str().c_str());
            return 0;
        }
    }
    else {
        PyErr_SetString(PyExc_ValueError, "Wrong PointPos argument");
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
        else if (err == -6)
            str << "Cannot set the datum because of invalid geometry";
        else
            str << "Unexpected problem at setting datum " << (const char*)Quantity.getUserString().toUtf8() << " for the constraint with index " << Index;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::getDatum(PyObject *args)
{
    const std::vector<Constraint*>& vals = this->getSketchObjectPtr()->Constraints.getValues();
    Constraint* constr = 0;

    do {
        int index = 0;
        if (PyArg_ParseTuple(args,"i", &index)) {
            if (index < 0 || index >= static_cast<int>(vals.size())) {
                PyErr_SetString(PyExc_IndexError, "index out of range");
                return 0;
            }

            constr = vals[index];
            break;
        }

        PyErr_Clear();
        char* name;
        if (PyArg_ParseTuple(args,"s", &name)) {
            int id = 0;
            for (std::vector<Constraint*>::const_iterator it = vals.begin(); it != vals.end(); ++it, ++id) {
                if (Sketcher::PropertyConstraintList::getConstraintName((*it)->Name, id) == name) {
                    constr = *it;
                    break;
                }
            }

            if (!constr) {
                std::stringstream str;
                str << "Invalid constraint name: '" << name << "'";
                PyErr_SetString(PyExc_NameError, str.str().c_str());
                return 0;
            }
            else {
                break;
            }
        }

        // error handling
        PyErr_SetString(PyExc_TypeError, "Wrong arguments");
        return 0;
    }
    while (false);

    ConstraintType type = constr->Type;
    if (type != Distance &&
        type != DistanceX &&
        type != DistanceY &&
        type != Radius &&
        type != Diameter &&
        type != Angle) {
        PyErr_SetString(PyExc_TypeError, "Constraint is not a datum");
        return 0;
    }

    Base::Quantity datum;
    datum.setValue(constr->getValue());
    if (type == Angle) {
        datum.setValue(Base::toDegrees<double>(datum.getValue()));
        datum.setUnit(Base::Unit::Angle);
    }
    else {
        datum.setUnit(Base::Unit::Length);
    }

    return new Base::QuantityPy(new Base::Quantity(datum));
}

PyObject* SketchObjectPy::setDriving(PyObject *args)
{
    PyObject* driving;
    int constrid;

    if (!PyArg_ParseTuple(args, "iO!", &constrid, &PyBool_Type, &driving))
        return 0;

    if (this->getSketchObjectPtr()->setDriving(constrid, PyObject_IsTrue(driving) ? true : false)) {
        std::stringstream str;
        str << "Not able set Driving/reference for constraint with the given index: " << constrid;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::setDatumsDriving(PyObject *args)
{
    PyObject* driving;

    if (!PyArg_ParseTuple(args, "O!", &PyBool_Type, &driving))
        return 0;

    if (this->getSketchObjectPtr()->setDatumsDriving(PyObject_IsTrue(driving) ? true : false)) {
        std::stringstream str;
        str << "Not able set all dimensionals driving/reference";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::moveDatumsToEnd(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    if (this->getSketchObjectPtr()->moveDatumsToEnd()) {
        std::stringstream str;
        str << "Not able move all dimensionals to end";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}


PyObject* SketchObjectPy::getDriving(PyObject *args)
{
    int constrid;
    bool driving;

    if (!PyArg_ParseTuple(args, "i", &constrid))
        return 0;

    if (this->getSketchObjectPtr()->getDriving(constrid, driving)) {
        PyErr_SetString(PyExc_ValueError, "Invalid constraint id");
        return 0;
    }

    return Py::new_reference_to(Py::Boolean(driving));
}

PyObject* SketchObjectPy::toggleDriving(PyObject *args)
{
    int constrid;

    if (!PyArg_ParseTuple(args, "i", &constrid))
        return 0;

    if (this->getSketchObjectPtr()->toggleDriving(constrid)) {
        std::stringstream str;
        str << "Not able toggle Driving for constraint with the given index: " << constrid;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::setVirtualSpace(PyObject *args)
{
    PyObject* invirtualspace;
    int constrid;

    if (!PyArg_ParseTuple(args, "iO!", &constrid, &PyBool_Type, &invirtualspace))
        return 0;

    if (this->getSketchObjectPtr()->setVirtualSpace(constrid, PyObject_IsTrue(invirtualspace) ? true : false)) {
        std::stringstream str;
        str << "Not able set virtual space for constraint with the given index: " << constrid;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::getVirtualSpace(PyObject *args)
{
    int constrid;
    bool invirtualspace;

    if (!PyArg_ParseTuple(args, "i", &constrid))
        return 0;

    if (this->getSketchObjectPtr()->getVirtualSpace(constrid, invirtualspace)) {
        PyErr_SetString(PyExc_ValueError, "Invalid constraint id");
        return 0;
    }

    return Py::new_reference_to(Py::Boolean(invirtualspace));
}

PyObject* SketchObjectPy::toggleVirtualSpace(PyObject *args)
{
    int constrid;

    if (!PyArg_ParseTuple(args, "i", &constrid))
        return 0;

    if (this->getSketchObjectPtr()->toggleVirtualSpace(constrid)) {
        std::stringstream str;
        str << "Not able toggle virtual space for constraint with the given index: " << constrid;
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
            str << "Not able to fillet curves with ids : (" << geoId1 << ", " << geoId2 << ") and points (" << v1.x << ", " << v1.y << ", " << v1.z << ") & "
            << "(" << v2.x << ", " << v2.y << ", " << v2.z << ")";
            THROWM(Base::ValueError, str.str().c_str())
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

PyObject* SketchObjectPy::extend(PyObject *args)
{
    double increment;
    int endPoint;
    int GeoId;

    if (PyArg_ParseTuple(args, "idi", &GeoId, &increment, &endPoint)) {
        if (this->getSketchObjectPtr()->extend(GeoId, increment, endPoint)) {
            std::stringstream str;
            str << "Not able to extend geometry with id : (" << GeoId  << ") for increment (" << increment << ") and point position (" << endPoint << ")";
            PyErr_SetString(PyExc_ValueError, str.str().c_str());
            return 0;
        }
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "extend() method accepts:\n"
        "-- int,float,int\n");
    return 0;
}

PyObject* SketchObjectPy::addSymmetric(PyObject *args)
{
    PyObject *pcObj;
    int refGeoId;
    int refPosId = Sketcher::none;

    if (!PyArg_ParseTuple(args, "Oi|i", &pcObj, &refGeoId, &refPosId))
        return 0;

    if (PyObject_TypeCheck(pcObj, &(PyList_Type)) ||
             PyObject_TypeCheck(pcObj, &(PyTuple_Type))) {
        std::vector<int> geoIdList;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
            if (PyLong_Check((*it).ptr()))
                geoIdList.push_back(PyLong_AsLong((*it).ptr()));
#else
            if (PyInt_Check((*it).ptr()))
                geoIdList.push_back(PyInt_AsLong((*it).ptr()));
#endif
        }

        int ret = this->getSketchObjectPtr()->addSymmetric(geoIdList,refGeoId,(Sketcher::PointPos) refPosId) + 1;

        if(ret == -1)
            throw Py::TypeError("Symmetric operation unsuccessful!");

        std::size_t numGeo = geoIdList.size();
        Py::Tuple tuple(numGeo);
        for (std::size_t i=0; i<numGeo; ++i) {
            int geoId = ret - int(numGeo - i);
            tuple.setItem(i, Py::Long(geoId));
        }

        return Py::new_reference_to(tuple);
    }

    std::string error = std::string("type must be list of GeoIds, not ");
    error += pcObj->ob_type->tp_name;
    throw Py::TypeError(error);
}

PyObject* SketchObjectPy::addCopy(PyObject *args)
{
    PyObject *pcObj, *pcVect;
    PyObject* clone= Py_False;

    if (!PyArg_ParseTuple(args, "OO!|O!", &pcObj, &(Base::VectorPy::Type), &pcVect, &PyBool_Type, &clone))
        return 0;

    Base::Vector3d vect = static_cast<Base::VectorPy*>(pcVect)->value();

    if (PyObject_TypeCheck(pcObj, &(PyList_Type)) ||
             PyObject_TypeCheck(pcObj, &(PyTuple_Type))) {
        std::vector<int> geoIdList;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
            if (PyLong_Check((*it).ptr()))
                geoIdList.push_back(PyLong_AsLong((*it).ptr()));
#else
            if (PyInt_Check((*it).ptr()))
                geoIdList.push_back(PyInt_AsLong((*it).ptr()));
#endif
        }

        int ret = this->getSketchObjectPtr()->addCopy(geoIdList, vect, false, PyObject_IsTrue(clone) ? true : false) + 1;

        if(ret == -1)
            throw Py::TypeError("Copy operation unsuccessful!");

        std::size_t numGeo = geoIdList.size();
        Py::Tuple tuple(numGeo);
        for (std::size_t i=0; i<numGeo; ++i) {
            int geoId = ret - int(numGeo - i);
            tuple.setItem(i, Py::Long(geoId));
        }

        return Py::new_reference_to(tuple);
    }

    std::string error = std::string("type must be list of GeoIds, not ");
    error += pcObj->ob_type->tp_name;
    throw Py::TypeError(error);
}

PyObject* SketchObjectPy::addMove(PyObject *args)
{
    PyObject *pcObj, *pcVect;

    if (!PyArg_ParseTuple(args, "OO!", &pcObj, &(Base::VectorPy::Type), &pcVect))
        return 0;

    Base::Vector3d vect = static_cast<Base::VectorPy*>(pcVect)->value();

    if (PyObject_TypeCheck(pcObj, &(PyList_Type)) ||
        PyObject_TypeCheck(pcObj, &(PyTuple_Type))) {
        std::vector<int> geoIdList;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            #if PY_MAJOR_VERSION >= 3
            if (PyLong_Check((*it).ptr()))
                geoIdList.push_back(PyLong_AsLong((*it).ptr()));
            #else
            if (PyInt_Check((*it).ptr()))
                geoIdList.push_back(PyInt_AsLong((*it).ptr()));
            #endif
        }

    this->getSketchObjectPtr()->addCopy(geoIdList, vect, true);

    Py_Return;
    }

    std::string error = std::string("type must be list of GeoIds, not ");
    error += pcObj->ob_type->tp_name;
    throw Py::TypeError(error);
}

PyObject* SketchObjectPy::addRectangularArray(PyObject *args)
{
    PyObject *pcObj, *pcVect;
    int rows,cols;
    double perpscale=1.0;
    PyObject* constraindisplacement= Py_False;
    PyObject* clone= Py_False;

    if (!PyArg_ParseTuple(args, "OO!O!ii|O!d", &pcObj, &(Base::VectorPy::Type), &pcVect,
            &PyBool_Type, &clone, &rows, &cols, &PyBool_Type, &constraindisplacement,&perpscale))
        return 0;

    Base::Vector3d vect = static_cast<Base::VectorPy*>(pcVect)->value();

    if (PyObject_TypeCheck(pcObj, &(PyList_Type)) ||
             PyObject_TypeCheck(pcObj, &(PyTuple_Type))) {
        std::vector<int> geoIdList;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
	    if (PyLong_Check((*it).ptr()))
		geoIdList.push_back(PyLong_AsLong((*it).ptr()));
#else
            if (PyInt_Check((*it).ptr()))
                geoIdList.push_back(PyInt_AsLong((*it).ptr()));
#endif
        }

        int ret = this->getSketchObjectPtr()->addCopy(geoIdList,vect, false, PyObject_IsTrue(clone) ? true : false,
                                                      rows, cols, PyObject_IsTrue(constraindisplacement) ? true : false, perpscale) + 1;

        if(ret == -1)
            throw Py::TypeError("Copy operation unsuccessful!");
        Py_Return;
    }

    std::string error = std::string("type must be list of GeoIds, not ");
    error += pcObj->ob_type->tp_name;
    throw Py::TypeError(error);
}

PyObject* SketchObjectPy::calculateAngleViaPoint(PyObject *args)
{
    int GeoId1=0, GeoId2=0;
    double px=0, py=0;
    if (!PyArg_ParseTuple(args, "iidd", &GeoId1, &GeoId2, &px, &py))
        return 0;

    SketchObject* obj = this->getSketchObjectPtr();
    if (GeoId1 > obj->getHighestCurveIndex() || -GeoId1 > obj->getExternalGeometryCount() ||
        GeoId2 > obj->getHighestCurveIndex() || -GeoId2 > obj->getExternalGeometryCount()    ) {
        PyErr_SetString(PyExc_ValueError, "Invalid geometry Id");
        return 0;
    }
    double ang = obj->calculateAngleViaPoint(GeoId1, GeoId2, px, py);

    return Py::new_reference_to(Py::Float(ang));
}

PyObject* SketchObjectPy::isPointOnCurve(PyObject *args)
{
    int GeoId=Constraint::GeoUndef;
    double px=0, py=0;
    if (!PyArg_ParseTuple(args, "idd", &GeoId, &px, &py))
        return 0;

    SketchObject* obj = this->getSketchObjectPtr();
    if (GeoId > obj->getHighestCurveIndex() || -GeoId > obj->getExternalGeometryCount()) {
        PyErr_SetString(PyExc_ValueError, "Invalid geometry Id");
        return 0;
    }

    return Py::new_reference_to(Py::Boolean(obj->isPointOnCurve(GeoId, px, py)));
}

PyObject* SketchObjectPy::calculateConstraintError(PyObject *args)
{
    int ic=-1;
    if (!PyArg_ParseTuple(args, "i", &ic))
        return 0;

    SketchObject* obj = this->getSketchObjectPtr();
    if (ic >= obj->Constraints.getSize() || ic < 0) {
        PyErr_SetString(PyExc_ValueError, "Invalid constraint Id");
        return 0;
    }
    double err = obj->calculateConstraintError(ic);

    return Py::new_reference_to(Py::Float(err));
}

PyObject* SketchObjectPy::changeConstraintsLocking(PyObject *args)
{
    int bLock=0;
    if (!PyArg_ParseTuple(args, "i", &bLock))
        return 0;

    SketchObject* obj = this->getSketchObjectPtr();

    int naff = obj->changeConstraintsLocking((bool)bLock);

    return Py::new_reference_to(Py::Long(naff));
}

//Deprecated
PyObject* SketchObjectPy::ExposeInternalGeometry(PyObject *args)
{
    int GeoId;

    if (!PyArg_ParseTuple(args, "i", &GeoId))
        return 0;

    if (this->getSketchObjectPtr()->exposeInternalGeometry(GeoId)==-1) {
        std::stringstream str;
        str << "Object does not support internal geometry: " << GeoId;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

//Deprecated
PyObject* SketchObjectPy::DeleteUnusedInternalGeometry(PyObject *args)
{
    int GeoId;

    if (!PyArg_ParseTuple(args, "i", &GeoId))
        return 0;

    if (this->getSketchObjectPtr()->deleteUnusedInternalGeometry(GeoId)==-1) {
        std::stringstream str;
        str << "Object does not support internal geometry: " << GeoId;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::exposeInternalGeometry(PyObject *args)
{
    int GeoId;

    if (!PyArg_ParseTuple(args, "i", &GeoId))
        return 0;

    if (this->getSketchObjectPtr()->exposeInternalGeometry(GeoId)==-1) {
        std::stringstream str;
        str << "Object does not support internal geometry: " << GeoId;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::deleteUnusedInternalGeometry(PyObject *args)
{
    int GeoId;

    if (!PyArg_ParseTuple(args, "i", &GeoId))
        return 0;

    if (this->getSketchObjectPtr()->deleteUnusedInternalGeometry(GeoId)==-1) {
        std::stringstream str;
        str << "Object does not support internal geometry: " << GeoId;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::convertToNURBS(PyObject *args)
{
    int GeoId;

    if (!PyArg_ParseTuple(args, "i", &GeoId))
        return 0;

    if (this->getSketchObjectPtr()->convertToNURBS(GeoId)==false) {
        std::stringstream str;
        str << "Object does not support NURBS conversion: " << GeoId;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::increaseBSplineDegree(PyObject *args)
{
    int GeoId;
    int incr = 1;

    if (!PyArg_ParseTuple(args, "i|i", &GeoId, &incr))
        return 0;

    if (this->getSketchObjectPtr()->increaseBSplineDegree(GeoId, incr)==false) {
        std::stringstream str;
        str << "Degree increase failed for: " << GeoId;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::modifyBSplineKnotMultiplicity(PyObject *args)
{
    int GeoId;
    int knotIndex;
    int multiplicity = 1;

    if (!PyArg_ParseTuple(args, "ii|i", &GeoId, &knotIndex, &multiplicity))
        return 0;

    if (this->getSketchObjectPtr()->modifyBSplineKnotMultiplicity(GeoId, knotIndex, multiplicity)==false) {
        std::stringstream str;
        str << "Multiplicity modification failed for: " << GeoId;
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::autoconstraint(PyObject *args)
{
    double precision = Precision::Confusion() * 1000;
    double angleprecision = M_PI/8;
    PyObject* includeconstruction = Py_True;


    if (!PyArg_ParseTuple(args, "|ddO!", &precision, &angleprecision, &PyBool_Type, &includeconstruction))
        return 0;

    if (this->getSketchObjectPtr()->autoConstraint(precision, angleprecision, PyObject_IsTrue(includeconstruction) ? true : false)) {
        std::stringstream str;
        str << "Unable to autoconstraint";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return 0;
    }

    Py_Return;
}

PyObject* SketchObjectPy::detectMissingPointOnPointConstraints(PyObject *args)
{
    double precision = Precision::Confusion() * 1000;
    PyObject* includeconstruction = Py_True;

    if (!PyArg_ParseTuple(args, "|dO!", &precision, &PyBool_Type, &includeconstruction))
        return 0;

    return Py::new_reference_to(Py::Long(this->getSketchObjectPtr()->detectMissingPointOnPointConstraints(precision, PyObject_IsTrue(includeconstruction) ? true : false)));
}

PyObject* SketchObjectPy::detectMissingVerticalHorizontalConstraints(PyObject *args)
{
    double angleprecision = M_PI/8;

    if (!PyArg_ParseTuple(args, "|d", &angleprecision))
        return 0;

    return Py::new_reference_to(Py::Long(this->getSketchObjectPtr()->detectMissingVerticalHorizontalConstraints(angleprecision)));
}

PyObject* SketchObjectPy::detectMissingEqualityConstraints(PyObject *args)
{
    double precision = Precision::Confusion() * 1000;

    if (!PyArg_ParseTuple(args, "|d", &precision))
        return 0;

    return Py::new_reference_to(Py::Long(this->getSketchObjectPtr()->detectMissingEqualityConstraints(precision)));
}

PyObject* SketchObjectPy::analyseMissingPointOnPointCoincident(PyObject *args)
{
    double angleprecision = M_PI/8;

    if (!PyArg_ParseTuple(args, "|d", &angleprecision))
        return 0;

    this->getSketchObjectPtr()->analyseMissingPointOnPointCoincident(angleprecision);

    Py_Return;
}

PyObject* SketchObjectPy::makeMissingPointOnPointCoincident(PyObject *args)
{

    PyObject* onebyone = Py_False;

    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &onebyone))
        return 0;

    this->getSketchObjectPtr()->makeMissingPointOnPointCoincident(PyObject_IsTrue(onebyone) ? true : false);

    Py_Return;
}

PyObject* SketchObjectPy::makeMissingVerticalHorizontal(PyObject *args)
{
    PyObject* onebyone = Py_False;

    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &onebyone))
        return 0;

    this->getSketchObjectPtr()->makeMissingVerticalHorizontal(PyObject_IsTrue(onebyone) ? true : false);

    Py_Return;
}

PyObject* SketchObjectPy::makeMissingEquality(PyObject *args)
{
    PyObject* onebyone = Py_True;

    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &onebyone))
        return 0;

    this->getSketchObjectPtr()->makeMissingEquality(PyObject_IsTrue(onebyone) ? true : false);

    Py_Return;
}

PyObject* SketchObjectPy::autoRemoveRedundants(PyObject *args)
{
    PyObject* updategeo = Py_True;

    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &updategeo))
        return 0;

    this->getSketchObjectPtr()->autoRemoveRedundants(PyObject_IsTrue(updategeo) ? true : false);

    Py_Return;
}

Py::List SketchObjectPy::getMissingPointOnPointConstraints(void) const
{
    std::vector<ConstraintIds> constraints = this->getSketchObjectPtr()->getMissingPointOnPointConstraints();

    Py::List list;
    for (auto c : constraints) {
        Py::Tuple t(5);
        t.setItem(0, Py::Long(c.First));
        t.setItem(1, Py::Long(((c.FirstPos == Sketcher::none)?0:(c.FirstPos == Sketcher::start)?1:(c.FirstPos == Sketcher::end)?2:3)));
        t.setItem(2, Py::Long(c.Second));
        t.setItem(3, Py::Long(((c.SecondPos == Sketcher::none)?0:(c.SecondPos == Sketcher::start)?1:(c.SecondPos == Sketcher::end)?2:3)));
        t.setItem(4, Py::Long(c.Type));
        list.append(t);
    }
    return list;
}

void SketchObjectPy::setMissingPointOnPointConstraints(Py::List arg)
{
    std::vector<ConstraintIds> constraints;

    auto checkpos = [](Py::Tuple &t,int i) {
        auto checkitem = [](Py::Tuple &t,int i, int val) {return long(Py::Long(t.getItem(i)))==val;};
        return (checkitem(t,i,0)?Sketcher::none:(checkitem(t,i,1)?Sketcher::start:(checkitem(t,i,2)?Sketcher::end:Sketcher::mid)));
    };

    for (auto ti : arg) {
        Py::Tuple t(ti);
        ConstraintIds c;
        c.First = (long)Py::Long(t.getItem(0));
        c.FirstPos = checkpos(t,1);
        c.Second = (long)Py::Long(t.getItem(2));
        c.SecondPos = checkpos(t,3);
        c.Type = (Sketcher::ConstraintType)(long)Py::Long(t.getItem(4));

        constraints.push_back(c);
    }

    this->getSketchObjectPtr()->setMissingPointOnPointConstraints(constraints);
}

Py::List SketchObjectPy::getMissingVerticalHorizontalConstraints(void) const
{
    std::vector<ConstraintIds> constraints = this->getSketchObjectPtr()->getMissingVerticalHorizontalConstraints();

    Py::List list;
    for (auto c : constraints) {
        Py::Tuple t(5);
        t.setItem(0, Py::Long(c.First));
        t.setItem(1, Py::Long(((c.FirstPos == Sketcher::none)?0:(c.FirstPos == Sketcher::start)?1:(c.FirstPos == Sketcher::end)?2:3)));
        t.setItem(2, Py::Long(c.Second));
        t.setItem(3, Py::Long(((c.SecondPos == Sketcher::none)?0:(c.SecondPos == Sketcher::start)?1:(c.SecondPos == Sketcher::end)?2:3)));
        t.setItem(4, Py::Long(c.Type));
        list.append(t);
    }
    return list;
}

void SketchObjectPy::setMissingVerticalHorizontalConstraints(Py::List arg)
{
    std::vector<ConstraintIds> constraints;

    auto checkpos = [](Py::Tuple &t,int i) {
        auto checkitem = [](Py::Tuple &t,int i, int val) {return long(Py::Long(t.getItem(i)))==val;};
        return (checkitem(t,i,0)?Sketcher::none:(checkitem(t,i,1)?Sketcher::start:(checkitem(t,i,2)?Sketcher::end:Sketcher::mid)));
    };

    for (auto ti : arg) {
        Py::Tuple t(ti);
        ConstraintIds c;
        c.First = (long)Py::Long(t.getItem(0));
        c.FirstPos = checkpos(t,1);
        c.Second = (long)Py::Long(t.getItem(2));
        c.SecondPos = checkpos(t,3);
        c.Type = (Sketcher::ConstraintType)(long)Py::Long(t.getItem(4));

        constraints.push_back(c);
    }

    this->getSketchObjectPtr()->setMissingVerticalHorizontalConstraints(constraints);
}

Py::List SketchObjectPy::getMissingLineEqualityConstraints(void) const
{
    std::vector<ConstraintIds> constraints = this->getSketchObjectPtr()->getMissingLineEqualityConstraints();

    Py::List list;
    for (auto c : constraints) {
        Py::Tuple t(4);
        t.setItem(0, Py::Long(c.First));
        t.setItem(1, Py::Long(((c.FirstPos == Sketcher::none)?0:(c.FirstPos == Sketcher::start)?1:(c.FirstPos == Sketcher::end)?2:3)));
        t.setItem(2, Py::Long(c.Second));
        t.setItem(3, Py::Long(((c.SecondPos == Sketcher::none)?0:(c.SecondPos == Sketcher::start)?1:(c.SecondPos == Sketcher::end)?2:3)));
        list.append(t);
    }
    return list;
}

void SketchObjectPy::setMissingLineEqualityConstraints(Py::List arg)
{
    std::vector<ConstraintIds> constraints;

    auto checkpos = [](Py::Tuple &t,int i) {
        auto checkitem = [](Py::Tuple &t,int i, int val) {return long(Py::Long(t.getItem(i)))==val;};
        return (checkitem(t,i,0)?Sketcher::none:(checkitem(t,i,1)?Sketcher::start:(checkitem(t,i,2)?Sketcher::end:Sketcher::mid)));
    };

    for (auto ti : arg) {
        Py::Tuple t(ti);
        ConstraintIds c;
        c.First = (long)Py::Long(t.getItem(0));
        c.FirstPos = checkpos(t,1);
        c.Second = (long)Py::Long(t.getItem(2));
        c.SecondPos = checkpos(t,3);
        c.Type = Sketcher::Equal;

        constraints.push_back(c);
    }

    this->getSketchObjectPtr()->setMissingLineEqualityConstraints(constraints);
}

Py::List SketchObjectPy::getMissingRadiusConstraints(void) const
{
    std::vector<ConstraintIds> constraints = this->getSketchObjectPtr()->getMissingRadiusConstraints();

    Py::List list;
    for (auto c : constraints) {
        Py::Tuple t(4);
        t.setItem(0, Py::Long(c.First));
        t.setItem(1, Py::Long(((c.FirstPos == Sketcher::none)?0:(c.FirstPos == Sketcher::start)?1:(c.FirstPos == Sketcher::end)?2:3)));
        t.setItem(2, Py::Long(c.Second));
        t.setItem(3, Py::Long(((c.SecondPos == Sketcher::none)?0:(c.SecondPos == Sketcher::start)?1:(c.SecondPos == Sketcher::end)?2:3)));
        list.append(t);
    }
    return list;
}

void SketchObjectPy::setMissingRadiusConstraints(Py::List arg)
{
    std::vector<ConstraintIds> constraints;

    auto checkpos = [](Py::Tuple &t,int i) {
        auto checkitem = [](Py::Tuple &t,int i, int val) {return long(Py::Long(t.getItem(i)))==val;};
        return (checkitem(t,i,0)?Sketcher::none:(checkitem(t,i,1)?Sketcher::start:(checkitem(t,i,2)?Sketcher::end:Sketcher::mid)));
    };

    for (auto ti : arg) {
        Py::Tuple t(ti);
        ConstraintIds c;
        c.First = (long)Py::Long(t.getItem(0));
        c.FirstPos = checkpos(t,1);
        c.Second = (long)Py::Long(t.getItem(2));
        c.SecondPos = checkpos(t,3);
        c.Type = Sketcher::Equal;

        constraints.push_back(c);
    }

    this->getSketchObjectPtr()->setMissingRadiusConstraints(constraints);
}

PyObject* SketchObjectPy::getGeometryWithDependentParameters(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    std::vector<std::pair<int,PointPos>> geometrymap;

    this->getSketchObjectPtr()->getGeometryWithDependentParameters(geometrymap);

    Py::List list;
    for (auto pair : geometrymap) {
        Py::Tuple t(2);
        t.setItem(0, Py::Long(pair.first));
        t.setItem(1, Py::Long(((pair.second == Sketcher::none)?0:(pair.second == Sketcher::start)?1:(pair.second == Sketcher::end)?2:3)));
        list.append(t);
    }
    return Py::new_reference_to(list);
}

Py::List SketchObjectPy::getOpenVertices(void) const
{
    std::vector<Base::Vector3d> points = this->getSketchObjectPtr()->getOpenVertices();

    Py::List list;
    for (auto p : points) {
        Py::Tuple t(3);
        t.setItem(0, Py::Float(p.x));
        t.setItem(1, Py::Float(p.y));
        t.setItem(2, Py::Float(p.z));
        list.append(t);
    }
    return list;
}

Py::Long SketchObjectPy::getConstraintCount(void) const
{
    return Py::Long(this->getSketchObjectPtr()->Constraints.getSize());
}

Py::Long SketchObjectPy::getGeometryCount(void) const
{
    return Py::Long(this->getSketchObjectPtr()->Geometry.getSize());
}

Py::Long SketchObjectPy::getAxisCount(void) const
{
    return Py::Long(this->getSketchObjectPtr()->getAxisCount());
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
