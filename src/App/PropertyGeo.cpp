/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
#	include <assert.h>
#endif

#include <boost/algorithm/string/predicate.hpp>

/// Here the FreeCAD includes sorted by Base,App,Gui......

#include <Base/Exception.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Rotation.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <Base/VectorPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/QuantityPy.h>

#include "Document.h"
#include "DocumentObject.h"
#include "Placement.h"
#include "PropertyGeo.h"
#include "ObjectIdentifier.h"

using namespace App;
using namespace Base;
using namespace std;




//**************************************************************************
//**************************************************************************
// PropertyVector
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVector , App::Property)

//**************************************************************************
// Construction/Destruction


PropertyVector::PropertyVector()
{

}


PropertyVector::~PropertyVector()
{

}

//**************************************************************************
// Base class implementer


void PropertyVector::setValue(const Base::Vector3d &vec)
{
    aboutToSetValue();
    _cVec=vec;
    hasSetValue();
}

void PropertyVector::setValue(double x, double y, double z)
{
    aboutToSetValue();
    _cVec.Set(x,y,z);
    hasSetValue();
}

const Base::Vector3d & PropertyVector::getValue(void)const
{
    return _cVec;
}

PyObject *PropertyVector::getPyObject(void)
{
    return new Base::VectorPy(_cVec);
}

void PropertyVector::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(Base::VectorPy::Type))) {
        Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(value);
        Base::Vector3d* val = pcObject->getVectorPtr();
        setValue(*val);
    }
    else if (PyTuple_Check(value)&&PyTuple_Size(value)==3) {
        PyObject* item;
        Base::Vector3d cVec;
        // x
        item = PyTuple_GetItem(value,0);
        if (PyFloat_Check(item))
            cVec.x = PyFloat_AsDouble(item);
#if PY_MAJOR_VERSION < 3
        else if (PyInt_Check(item))
            cVec.x = (double)PyInt_AsLong(item);
#else
        else if (PyLong_Check(item))
            cVec.x = (double)PyLong_AsLong(item);
#endif
        else
            throw Base::TypeError("Not allowed type used in tuple (float expected)...");
        // y
        item = PyTuple_GetItem(value,1);
        if (PyFloat_Check(item))
            cVec.y = PyFloat_AsDouble(item);
#if PY_MAJOR_VERSION < 3
        else if (PyInt_Check(item))
            cVec.y = (double)PyInt_AsLong(item);
#else
        else if (PyLong_Check(item))
            cVec.y = (double)PyLong_AsLong(item);
#endif
        else
            throw Base::TypeError("Not allowed type used in tuple (float expected)...");
        // z
        item = PyTuple_GetItem(value,2);
        if (PyFloat_Check(item))
            cVec.z = PyFloat_AsDouble(item);
#if PY_MAJOR_VERSION < 3
        else if (PyInt_Check(item))
            cVec.z = (double)PyInt_AsLong(item);
#else
        else if (PyLong_Check(item))
            cVec.z = (double)PyLong_AsLong(item);
#endif
        else
            throw Base::TypeError("Not allowed type used in tuple (float expected)...");
        setValue( cVec );
    }
    else {
        std::string error = std::string("type must be 'Vector' or tuple of three floats, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyVector::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<PropertyVector valueX=\"" <<  _cVec.x << "\" valueY=\"" <<  _cVec.y << "\" valueZ=\"" <<  _cVec.z <<"\"/>\n";
}

void PropertyVector::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("PropertyVector");
    // get the value of my Attribute
    aboutToSetValue();
    _cVec.x = reader.getAttributeAsFloat("valueX");
    _cVec.y = reader.getAttributeAsFloat("valueY");
    _cVec.z = reader.getAttributeAsFloat("valueZ");
    hasSetValue();
}


Property *PropertyVector::Copy(void) const
{
    PropertyVector *p= new PropertyVector();
    p->_cVec = _cVec;
    return p;
}

void PropertyVector::Paste(const Property &from)
{
    aboutToSetValue();
    _cVec = dynamic_cast<const PropertyVector&>(from)._cVec;
    hasSetValue();
}

void PropertyVector::getPaths(std::vector<ObjectIdentifier> &paths) const
{
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("z")));
}

App::any PropertyVector::getPathValue(const ObjectIdentifier &path) const
{
    Base::Unit unit = getUnit();
    if(!unit.isEmpty()) {
        std::string p = path.getSubPathStr();
        if (p == ".x" || p == ".y" || p == ".z") {
            // Convert double to quantity
            return Base::Quantity(App::any_cast<double>(Property::getPathValue(path)), unit);
        }
    }
    return Property::getPathValue(path);
}

bool PropertyVector::getPyPathValue(const ObjectIdentifier &path, Py::Object &res) const
{
    Base::Unit unit = getUnit();
    if(unit.isEmpty())
        return false;

    int i=0;
    for(auto &c : path.getPropertyComponents(1)) {
        if(i == 0) {
            if(!c.isSimple())
                return false;
            if(c.getName() == "x") {
                res = Py::asObject(new QuantityPy(new Quantity(getValue().x,unit)));
            } else if(c.getName() == "y") {
                res = Py::asObject(new QuantityPy(new Quantity(getValue().y,unit)));
            } else if(c.getName() == "z") {
                res = Py::asObject(new QuantityPy(new Quantity(getValue().z,unit)));
            } else
                return false;
            continue;
        }
        res = c.get(res);
    }
    return true;
}


//**************************************************************************
// PropertyVectorDistance
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVectorDistance , App::PropertyVector)

//**************************************************************************
// Construction/Destruction


PropertyVectorDistance::PropertyVectorDistance()
{

}

PropertyVectorDistance::~PropertyVectorDistance()
{

}

//**************************************************************************
// PropertyPosition
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPosition , App::PropertyVector)

//**************************************************************************
// Construction/Destruction


PropertyPosition::PropertyPosition()
{

}

PropertyPosition::~PropertyPosition()
{

}

//**************************************************************************
// PropertyPosition
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyDirection , App::PropertyVector)

//**************************************************************************
// Construction/Destruction


PropertyDirection::PropertyDirection()
{

}

PropertyDirection::~PropertyDirection()
{

}

//**************************************************************************
// PropertyVectorList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVectorList , App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyVectorList::PropertyVectorList()
{

}

PropertyVectorList::~PropertyVectorList()
{

}

//**************************************************************************
// Base class implementer

void PropertyVectorList::setValue(double x, double y, double z)
{
    setValue(Base::Vector3d(x,y,z));
}

PyObject *PropertyVectorList::getPyObject(void)
{
    PyObject* list = PyList_New(	getSize() );

    for (int i = 0;i<getSize(); i++)
        PyList_SetItem( list, i, new VectorPy(	_lValueList[i]));

    return list;
}

Base::Vector3d PropertyVectorList::getPyValue(PyObject *item) const {
    PropertyVector val;
    val.setPyObject( item );
    return val.getValue();
}

bool PropertyVectorList::saveXML(Base::Writer &writer) const
{
    writer.Stream() << ">\n";
    for(const auto &v : _lValueList)
        writer.Stream() << v.x << ' ' << v.y << ' ' << v.z << '\n';
    return false;
}

void PropertyVectorList::restoreXML(Base::XMLReader &reader)
{
    unsigned count = reader.getAttributeAsUnsigned("count");
    auto &s = reader.beginCharStream(false);
    std::vector<Base::Vector3d> values(count);
    for(auto &v : values) 
        s >> v.x >> v.y >> v.z;
    reader.endCharStream();
    setValues(std::move(values));
}

void PropertyVectorList::saveStream(Base::OutputStream &str) const
{
    if (!isSinglePrecision()) {
        for (std::vector<Base::Vector3d>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
            str << it->x << it->y << it->z;
        }
    }
    else {
        for (std::vector<Base::Vector3d>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
            float x = (float)it->x;
            float y = (float)it->y;
            float z = (float)it->z;
            str << x << y << z;
        }
    }
}

void PropertyVectorList::restoreStream(Base::InputStream &str, unsigned uCt)
{
    std::vector<Base::Vector3d> values(uCt);
    if (!isSinglePrecision()) {
        for (std::vector<Base::Vector3d>::iterator it = values.begin(); it != values.end(); ++it) {
            str >> it->x >> it->y >> it->z;
        }
    }
    else {
        float x,y,z;
        for (std::vector<Base::Vector3d>::iterator it = values.begin(); it != values.end(); ++it) {
            str >> x >> y >> z;
            it->Set(x, y, z);
        }
    }
    setValues(std::move(values));
}

Property *PropertyVectorList::Copy(void) const
{
    PropertyVectorList *p= new PropertyVectorList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyVectorList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyVectorList&>(from)._lValueList);
}

unsigned int PropertyVectorList::getMemSize (void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Base::Vector3d));
}

//**************************************************************************
// _PropertyVectorList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void _PropertyVectorList::setValue(float x, float y, float z)
{
    setValue(Base::Vector3f(x,y,z));
}

PyObject *_PropertyVectorList::getPyObject(void)
{
    Py::List res;
    for(const auto &v : _lValueList)
        res.append(Py::Object(new VectorPy(Vector3d(v.x,v.y,v.z))));
    return Py::new_reference_to(res);
}

Base::Vector3f _PropertyVectorList::getPyValue(PyObject *item) const {
    PropertyVector val;
    val.setPyObject( item );
    const auto &v = val.getValue();
    return Base::Vector3f(v.x,v.y,v.z);
}

bool _PropertyVectorList::saveXML(Base::Writer &writer) const
{
    writer.Stream() << ">\n";
    for(const auto &v : _lValueList)
        writer.Stream() << v.x << ' ' << v.y << ' ' << v.z << '\n';
    return false;
}

void _PropertyVectorList::restoreXML(Base::XMLReader &reader)
{
    unsigned count = reader.getAttributeAsUnsigned("count");
    auto &s = reader.beginCharStream(false);
    std::vector<Base::Vector3f> values(count);
    for(auto &v : values) 
        s >> v.x >> v.y >> v.z;
    reader.endCharStream();
    setValues(std::move(values));
}

void _PropertyVectorList::saveStream(Base::OutputStream &str) const
{
    for(const auto &v : _lValueList)
        str << v.x << v.y << v.z;
}

void _PropertyVectorList::restoreStream(Base::InputStream &str, unsigned uCt)
{
    std::vector<Base::Vector3f> values(uCt);
    for(auto &v : _lValueList)
        str >> v.x >> v.y >> v.z;
    setValues(std::move(values));
}

Property *_PropertyVectorList::Copy(void) const
{
    _PropertyVectorList *p= new _PropertyVectorList();
    p->_lValueList = _lValueList;
    return p;
}

void _PropertyVectorList::Paste(const Property &from)
{
    setValues(dynamic_cast<const _PropertyVectorList&>(from)._lValueList);
}

unsigned int _PropertyVectorList::getMemSize (void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Base::Vector3f));
}

//**************************************************************************
//**************************************************************************
// PropertyMatrix
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMatrix , App::Property)

//**************************************************************************
// Construction/Destruction


PropertyMatrix::PropertyMatrix()
{

}


PropertyMatrix::~PropertyMatrix()
{

}

//**************************************************************************
// Base class implementer


void PropertyMatrix::setValue(const Base::Matrix4D &mat)
{
    aboutToSetValue();
    _cMat=mat;
    hasSetValue();
}


const Base::Matrix4D & PropertyMatrix::getValue(void)const
{
    return _cMat;
}

PyObject *PropertyMatrix::getPyObject(void)
{
    return new Base::MatrixPy(_cMat);
}

void PropertyMatrix::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(Base::MatrixPy::Type))) {
        Base::MatrixPy  *pcObject = (Base::MatrixPy*)value;
        setValue( pcObject->value() );
    }
    else if (PyTuple_Check(value)&&PyTuple_Size(value)==16) {
        PyObject* item;
        Base::Matrix4D cMatrix;

        for (int x=0; x<4;x++) {
            for (int y=0; y<4;y++) {
                item = PyTuple_GetItem(value,x+y*4);
                if (PyFloat_Check(item))
                    cMatrix[x][y] = PyFloat_AsDouble(item);
#if PY_MAJOR_VERSION < 3
                else if (PyInt_Check(item))
                    cMatrix[x][y] = (double)PyInt_AsLong(item);
#else
                else if (PyLong_Check(item))
                    cMatrix[x][y] = (double)PyLong_AsLong(item);
#endif
                else
                    throw Base::TypeError("Not allowed type used in matrix tuple (a number expected)...");
            }
        }

        setValue( cMatrix );
    }
    else {
        std::string error = std::string("type must be 'Matrix' or tuple of 16 float or int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyMatrix::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<PropertyMatrix";
    writer.Stream() << " a11=\"" <<  _cMat[0][0] << "\" a12=\"" <<  _cMat[0][1] << "\" a13=\"" <<  _cMat[0][2] << "\" a14=\"" <<  _cMat[0][3] << "\"";
    writer.Stream() << " a21=\"" <<  _cMat[1][0] << "\" a22=\"" <<  _cMat[1][1] << "\" a23=\"" <<  _cMat[1][2] << "\" a24=\"" <<  _cMat[1][3] << "\"";
    writer.Stream() << " a31=\"" <<  _cMat[2][0] << "\" a32=\"" <<  _cMat[2][1] << "\" a33=\"" <<  _cMat[2][2] << "\" a34=\"" <<  _cMat[2][3] << "\"";
    writer.Stream() << " a41=\"" <<  _cMat[3][0] << "\" a42=\"" <<  _cMat[3][1] << "\" a43=\"" <<  _cMat[3][2] << "\" a44=\"" <<  _cMat[3][3] << "\"";
    writer.Stream() <<"/>\n";
}

void PropertyMatrix::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("PropertyMatrix");
    // get the value of my Attribute
    aboutToSetValue();
    _cMat[0][0] = reader.getAttributeAsFloat("a11");
    _cMat[0][1] = reader.getAttributeAsFloat("a12");
    _cMat[0][2] = reader.getAttributeAsFloat("a13");
    _cMat[0][3] = reader.getAttributeAsFloat("a14");

    _cMat[1][0] = reader.getAttributeAsFloat("a21");
    _cMat[1][1] = reader.getAttributeAsFloat("a22");
    _cMat[1][2] = reader.getAttributeAsFloat("a23");
    _cMat[1][3] = reader.getAttributeAsFloat("a24");

    _cMat[2][0] = reader.getAttributeAsFloat("a31");
    _cMat[2][1] = reader.getAttributeAsFloat("a32");
    _cMat[2][2] = reader.getAttributeAsFloat("a33");
    _cMat[2][3] = reader.getAttributeAsFloat("a34");

    _cMat[3][0] = reader.getAttributeAsFloat("a41");
    _cMat[3][1] = reader.getAttributeAsFloat("a42");
    _cMat[3][2] = reader.getAttributeAsFloat("a43");
    _cMat[3][3] = reader.getAttributeAsFloat("a44");
    hasSetValue();
}


Property *PropertyMatrix::Copy(void) const
{
    PropertyMatrix *p= new PropertyMatrix();
    p->_cMat = _cMat;
    return p;
}

void PropertyMatrix::Paste(const Property &from)
{
    aboutToSetValue();
    _cMat = dynamic_cast<const PropertyMatrix&>(from)._cMat;
    hasSetValue();
}

//**************************************************************************
//**************************************************************************
// PropertyPlacement
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPlacement , App::Property)

//**************************************************************************
// Construction/Destruction


PropertyPlacement::PropertyPlacement()
{

}


PropertyPlacement::~PropertyPlacement()
{

}

//**************************************************************************
// Base class implementer


void PropertyPlacement::setValue(const Base::Placement &pos)
{
    aboutToSetValue();
    _cPos=pos;
    hasSetValue();
}

bool PropertyPlacement::setValueIfChanged(const Base::Placement &pos,double tol,double atol)
{
    if(_cPos.getPosition().IsEqual(pos.getPosition(),tol)
            && _cPos.getRotation().isSame(pos.getRotation(),atol))
    {
        return false;
    }
    setValue(pos);
    return true;
}


const Base::Placement & PropertyPlacement::getValue(void)const
{
    return _cPos;
}

void PropertyPlacement::getPaths(std::vector<ObjectIdentifier> &paths) const
{
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Base"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Base"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Base"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("z")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Angle")));

    // Unlike the above path (which provides units that are not available
    // through python, the following paths provides the same value. They are no
    // longer needed, because the expression completer will now dig into all
    // python attributes.

    // paths.push_back(ObjectIdentifier(*this)
    //                 << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
    //                 << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
    //                 << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    // paths.push_back(ObjectIdentifier(*this)
    //                 << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
    //                 << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
    //                 << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    // paths.push_back(ObjectIdentifier(*this)
    //                 << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
    //                 << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
    //                 << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("z")));
}

void PropertyPlacement::setPathValue(const ObjectIdentifier &path, const App::any &value)
{
    if (path.getSubPathStr() == ".Rotation.Angle") {
        double avalue;

        if (value.type() == typeid(Base::Quantity))
            avalue = App::any_cast<const Base::Quantity&>(value).getValue();
        else if (value.type() == typeid(double))
            avalue = App::any_cast<double>(value);
        else if (value.type() == typeid(int))
            avalue =  App::any_cast<int>(value);
        else if (value.type() == typeid(unsigned int))
            avalue =  App::any_cast<unsigned int >(value);
        else if (value.type() == typeid(short))
            avalue =  App::any_cast<short>(value);
        else if (value.type() == typeid(unsigned short))
            avalue =  App::any_cast<unsigned short>(value);
        else if (value.type() == typeid(long))
            avalue =  App::any_cast<long>(value);
        else if (value.type() == typeid(unsigned long))
            avalue =  App::any_cast<unsigned long>(value);
        else
            throw std::bad_cast();

        Property::setPathValue(path, Base::toRadians(avalue));
    }
    else
        Property::setPathValue(path, value);
}

App::any PropertyPlacement::getPathValue(const ObjectIdentifier &path) const
{
    std::string p = path.getSubPathStr();

    if (p == ".Rotation.Angle") {
        // Convert angle to degrees
        return Base::Quantity(Base::toDegrees(App::any_cast<double>(Property::getPathValue(path))), Unit::Angle);
    }
    else if (p == ".Base.x" || p == ".Base.y" || p == ".Base.z") {
        // Convert double to quantity
        return Base::Quantity(App::any_cast<double>(Property::getPathValue(path)), Unit::Length);
    }
    else
        return Property::getPathValue(path);
}

bool PropertyPlacement::getPyPathValue(const ObjectIdentifier &path, Py::Object &res) const
{
    auto components = path.getPropertyComponents(1);
    if(components.size() < 2)
        return false;
    if(components[0].isSimple() && components[0].getName() == "Rotation") {
        if(components[1].isSimple() && components[1].getName() != "Angle")
            return false;
        Base::Vector3d axis; double angle;
        _cPos.getRotation().getValue(axis,angle);
        res = Py::asObject(new QuantityPy(new Quantity(Base::toDegrees(angle),Unit::Angle)));
    }
    else if(components[0].isSimple() && components[0].getName() == "Base") {
        if(!components[1].isSimple())
            return false;
        if(components[1].getName() == "x") {
            res = Py::asObject(new QuantityPy(new Quantity(_cPos.getPosition().x,Unit::Length)));
        } else if(components[1].getName() == "y") {
            res = Py::asObject(new QuantityPy(new Quantity(_cPos.getPosition().y,Unit::Length)));
        } else if(components[1].getName() == "z") {
            res = Py::asObject(new QuantityPy(new Quantity(_cPos.getPosition().z,Unit::Length)));
        } else
            return false;
    }

    for(size_t i=2;i<components.size();++i)
        res = components[i].get(res);
    return true;
}

PyObject *PropertyPlacement::getPyObject(void)
{
    return new Base::PlacementPy(new Base::Placement(_cPos));
}

void PropertyPlacement::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(Base::MatrixPy::Type))) {
        Base::MatrixPy  *pcObject = (Base::MatrixPy*)value;
        Base::Matrix4D mat = pcObject->value();
        Base::Placement p;
        p.fromMatrix(mat);
        setValue(p);
    }
    else if (PyObject_TypeCheck(value, &(Base::PlacementPy::Type))) {
        setValue(*static_cast<Base::PlacementPy*>(value)->getPlacementPtr());
    }
    else {
        std::string error = std::string("type must be 'Matrix' or 'Placement', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyPlacement::Save (Base::Writer &writer) const
{
    Vector3d axis;
    double rfAngle;
    _cPos.getRotation().getValue(axis, rfAngle);

    writer.Stream() << writer.ind() << "<PropertyPlacement"
                    << " Px=\"" <<  _cPos.getPosition().x 
                    << "\" Py=\"" <<  _cPos.getPosition().y
                    << "\" Pz=\"" <<  _cPos.getPosition().z

                    << "\" Q0=\"" <<  _cPos.getRotation()[0]
                    << "\" Q1=\"" <<  _cPos.getRotation()[1]
                    << "\" Q2=\"" <<  _cPos.getRotation()[2]
                    << "\" Q3=\"" <<  _cPos.getRotation()[3]

                    << "\" A=\"" <<  rfAngle
                    << "\" Ox=\"" <<  axis.x
                    << "\" Oy=\"" <<  axis.y
                    << "\" Oz=\"" <<  axis.z 

                    << "\"/>\n";
}

void PropertyPlacement::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("PropertyPlacement");
    // get the value of my Attribute
    aboutToSetValue();

    if (reader.hasAttribute("A")) {
        _cPos = Base::Placement(Vector3d(reader.getAttributeAsFloat("Px"),
                                         reader.getAttributeAsFloat("Py"),
                                         reader.getAttributeAsFloat("Pz")),
                       Rotation(Vector3d(reader.getAttributeAsFloat("Ox"),
                                         reader.getAttributeAsFloat("Oy"),
                                         reader.getAttributeAsFloat("Oz")),
                                reader.getAttributeAsFloat("A")));
    }
    else {
        _cPos = Base::Placement(Vector3d(reader.getAttributeAsFloat("Px"),
                                         reader.getAttributeAsFloat("Py"),
                                         reader.getAttributeAsFloat("Pz")),
                                Rotation(reader.getAttributeAsFloat("Q0"),
                                         reader.getAttributeAsFloat("Q1"),
                                         reader.getAttributeAsFloat("Q2"),
                                         reader.getAttributeAsFloat("Q3")));
    }

    hasSetValue();
}


Property *PropertyPlacement::Copy(void) const
{
    PropertyPlacement *p= new PropertyPlacement();
    p->_cPos = _cPos;
    return p;
}

void PropertyPlacement::Paste(const Property &from)
{
    aboutToSetValue();
    _cPos = dynamic_cast<const PropertyPlacement&>(from)._cPos;
    hasSetValue();
}


//**************************************************************************
// PropertyPlacementList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPlacementList , App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyPlacementList::PropertyPlacementList()
{

}

PropertyPlacementList::~PropertyPlacementList()
{

}

//**************************************************************************
// Base class implementer

PyObject *PropertyPlacementList::getPyObject(void)
{
    PyObject* list = PyList_New( getSize() );

    for (int i = 0;i<getSize(); i++)
        PyList_SetItem( list, i, new Base::PlacementPy(new Base::Placement(_lValueList[i])));

    return list;
}

Base::Placement PropertyPlacementList::getPyValue(PyObject *item) const {
    PropertyPlacement val;
    val.setPyObject( item );
    return val.getValue();
}

bool PropertyPlacementList::saveXML(Base::Writer &writer) const
{
    writer.Stream() << ">\n";
    for(const auto &v : _lValueList) {
        Vector3d axis;
        double fAngle;
        v.getRotation().getValue(axis, fAngle);
        writer.Stream() << v.getPosition().x  << ' '
                        << v.getPosition().y << ' '
                        << v.getPosition().z << ' '
                        << fAngle << ' '
                        << axis.x << ' '
                        << axis.y << ' '
                        << axis.z << '\n';
    }
    return false;
}

void PropertyPlacementList::restoreXML(Base::XMLReader &reader)
{
    unsigned count = reader.getAttributeAsUnsigned("count");
    auto &s = reader.beginCharStream(false);
    std::vector<Base::Placement> values(count);
    for(auto &v : values) {
        Base::Vector3d pos,axis;
        double rfAngle;
        s >> pos.x >> pos.y >> pos.z >> rfAngle >> axis.x >> axis.y >> axis.z;
        v.setRotation(Base::Rotation(axis,rfAngle));
        v.setPosition(pos);
    }
    reader.endCharStream();
    setValues(std::move(values));
}

void PropertyPlacementList::saveStream(Base::OutputStream &str) const
{
    if (!isSinglePrecision()) {
        for (std::vector<Base::Placement>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
            str << it->getPosition().x << it->getPosition().y << it->getPosition().z
                << it->getRotation()[0] << it->getRotation()[1] << it->getRotation()[2] << it->getRotation()[3] ;
        }
    }
    else {
        for (std::vector<Base::Placement>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
            float x = (float)it->getPosition().x;
            float y = (float)it->getPosition().y;
            float z = (float)it->getPosition().z;
            float q0 = (float)it->getRotation()[0];
            float q1 = (float)it->getRotation()[1];
            float q2 = (float)it->getRotation()[2];
            float q3 = (float)it->getRotation()[3];
            str << x << y << z << q0 << q1 << q2 << q3;
        }
    }
}

void PropertyPlacementList::restoreStream(Base::InputStream &str, unsigned uCt)
{
    std::vector<Base::Placement> values(uCt);
    if (!isSinglePrecision()) {
        for (std::vector<Base::Placement>::iterator it = values.begin(); it != values.end(); ++it) {
            Base::Vector3d pos;
            double q0, q1, q2, q3;
            str >> pos.x >> pos.y >> pos.z >> q0 >> q1 >> q2 >> q3;
            Base::Rotation rot(q0,q1,q2,q3);
            it->setPosition(pos);
            it->setRotation(rot);
        }
    }
    else {
        float x,y,z,q0,q1,q2,q3;
        for (std::vector<Base::Placement>::iterator it = values.begin(); it != values.end(); ++it) {
            str >> x >> y >> z >> q0 >> q1 >> q2 >> q3;
            Base::Vector3d pos(x, y, z);
            Base::Rotation rot(q0,q1,q2,q3);
            it->setPosition(pos);
            it->setRotation(rot);
        }
    }
    setValues(std::move(values));
}

Property *PropertyPlacementList::Copy(void) const
{
    PropertyPlacementList *p= new PropertyPlacementList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyPlacementList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyPlacementList&>(from)._lValueList);
}

unsigned int PropertyPlacementList::getMemSize (void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Base::Vector3d));
}




//**************************************************************************
//**************************************************************************
// PropertyPlacement
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPlacementLink , App::PropertyLink)

//**************************************************************************
// Construction/Destruction


PropertyPlacementLink::PropertyPlacementLink()
{

}


PropertyPlacementLink::~PropertyPlacementLink()
{

}

App::Placement * PropertyPlacementLink::getPlacementObject(void) const
{
    if (_pcLink->getTypeId().isDerivedFrom(App::Placement::getClassTypeId()))
        return dynamic_cast<App::Placement*>(_pcLink);
    else
        return 0;

}

//**************************************************************************
// Base class implementer

Property *PropertyPlacementLink::Copy(void) const
{
    PropertyPlacementLink *p= new PropertyPlacementLink();
    p->_pcLink = _pcLink;
    return p;
}

void PropertyPlacementLink::Paste(const Property &from)
{
    aboutToSetValue();
    _pcLink = dynamic_cast<const PropertyPlacementLink&>(from)._pcLink;
    hasSetValue();
}

// ------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyGeometry , App::Property)

PropertyGeometry::PropertyGeometry()
{

}

PropertyGeometry::~PropertyGeometry()
{

}

// ------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyComplexGeoData , App::PropertyGeometry)

PropertyComplexGeoData::PropertyComplexGeoData()
{

}

PropertyComplexGeoData::~PropertyComplexGeoData()
{

}

std::string PropertyComplexGeoData::getElementMapVersion(bool) const {
    auto data = getComplexData();
    if(!data)
        return std::string();
    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    std::ostringstream ss;
    if(owner && owner->getDocument() && owner->getDocument()->Hasher==data->Hasher)
        ss << "1.";
    else
        ss << "0.";
    ss << data->getElementMapVersion();
    return ss.str();
}

bool PropertyComplexGeoData::isSame(const Property &_other) const
{
    if(!_other.isDerivedFrom(PropertyComplexGeoData::getClassTypeId()))
        return false;
    auto data = getComplexData();
    auto other = static_cast<const PropertyComplexGeoData&>(_other).getComplexData();

    if(other == data)
        return true;
    if(!data || !other)
        return false;
    return data->isSame(*other);
}
