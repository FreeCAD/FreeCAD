/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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

/// Here the FreeCAD includes sorted by Base,App,Gui......

#include <Base/Exception.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Rotation.h>
#include <Base/VectorPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>

#include "Placement.h"

#include "PropertyGeo.h"

using namespace App;
using namespace Base;
using namespace std;




//**************************************************************************
//**************************************************************************
// PropertyVector
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVector , App::Property);

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
        else if (PyInt_Check(item))
            cVec.x = (double)PyInt_AsLong(item);
        else
            throw Base::TypeError("Not allowed type used in tuple (float expected)...");
        // y
        item = PyTuple_GetItem(value,1);
        if (PyFloat_Check(item))
            cVec.y = PyFloat_AsDouble(item);
        else if (PyInt_Check(item))
            cVec.y = (double)PyInt_AsLong(item);
        else
            throw Base::TypeError("Not allowed type used in tuple (float expected)...");
        // z
        item = PyTuple_GetItem(value,2);
        if (PyFloat_Check(item))
            cVec.z = PyFloat_AsDouble(item);
        else if (PyInt_Check(item))
            cVec.z = (double)PyInt_AsLong(item);
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
    writer.Stream() << writer.ind() << "<PropertyVector valueX=\"" <<  _cVec.x << "\" valueY=\"" <<  _cVec.y << "\" valueZ=\"" <<  _cVec.z <<"\"/>" << endl;
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


//**************************************************************************
// PropertyVectorList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVectorList , App::PropertyLists);

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

void PropertyVectorList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyVectorList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyVectorList::setValue(const Base::Vector3d& lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0]=lValue;
    hasSetValue();
}

void PropertyVectorList::setValue(double x, double y, double z)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0].Set(x,y,z);
    hasSetValue();
}

void PropertyVectorList::setValues(const std::vector<Base::Vector3d>& values)
{
    aboutToSetValue();
    _lValueList = values;
    hasSetValue();
}

PyObject *PropertyVectorList::getPyObject(void)
{
    PyObject* list = PyList_New(	getSize() );

    for (int i = 0;i<getSize(); i++)
        PyList_SetItem( list, i, new VectorPy(	_lValueList[i]));

    return list;
}

void PropertyVectorList::setPyObject(PyObject *value)
{
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<Base::Vector3d> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(value, i);
            PropertyVector val;
            val.setPyObject( item );
            values[i] = val.getValue();
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(VectorPy::Type))) {
        Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(value);
        Base::Vector3d* val = pcObject->getVectorPtr();
        setValue(*val);
    }
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 3) {
        PropertyVector val;
        val.setPyObject( value );
        setValue( val.getValue() );
    }
    else {
        std::string error = std::string("type must be 'Vector' or list of 'Vector', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyVectorList::Save (Base::Writer &writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<VectorList file=\"" << writer.addFile(getName(), this) << "\"/>" << std::endl;
    }
}

void PropertyVectorList::Restore(Base::XMLReader &reader)
{
    reader.readElement("VectorList");
    std::string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
}

void PropertyVectorList::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    if (writer.getFileVersion() > 0) {
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

void PropertyVectorList::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader);
    uint32_t uCt=0;
    str >> uCt;
    std::vector<Base::Vector3d> values(uCt);
    if (reader.getFileVersion() > 0) {
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
    setValues(values);
}

Property *PropertyVectorList::Copy(void) const
{
    PropertyVectorList *p= new PropertyVectorList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyVectorList::Paste(const Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyVectorList&>(from)._lValueList;
    hasSetValue();
}

unsigned int PropertyVectorList::getMemSize (void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Base::Vector3d));
}

//**************************************************************************
//**************************************************************************
// PropertyMatrix
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMatrix , App::Property);

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
                else if (PyInt_Check(item))
                    cMatrix[x][y] = (double)PyInt_AsLong(item);
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
    writer.Stream() <<"/>" << endl;
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

TYPESYSTEM_SOURCE(App::PropertyPlacement , App::Property);

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

const Base::Placement & PropertyPlacement::getValue(void)const
{
    return _cPos;
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
    writer.Stream() << writer.ind() << "<PropertyPlacement";
    writer.Stream() << " Px=\"" <<  _cPos.getPosition().x 
                    << "\" Py=\"" <<  _cPos.getPosition().y
                    << "\" Pz=\"" <<  _cPos.getPosition().z << "\"";
    writer.Stream() << " Q0=\"" <<  _cPos.getRotation()[0]
                    << "\" Q1=\"" <<  _cPos.getRotation()[1]
                    << "\" Q2=\"" <<  _cPos.getRotation()[2]
                    << "\" Q3=\"" <<  _cPos.getRotation()[3] << "\"";
    writer.Stream() <<"/>" << endl;
}

void PropertyPlacement::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("PropertyPlacement");
    // get the value of my Attribute
    aboutToSetValue();
    _cPos = Base::Placement(Vector3d(reader.getAttributeAsFloat("Px"),
                                     reader.getAttributeAsFloat("Py"),
                                     reader.getAttributeAsFloat("Pz")),
                            Rotation(reader.getAttributeAsFloat("Q0"),
                                     reader.getAttributeAsFloat("Q1"),
                                     reader.getAttributeAsFloat("Q2"),
                                     reader.getAttributeAsFloat("Q3")));
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
//**************************************************************************
// PropertyPlacement
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPlacementLink , App::PropertyLink);

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

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyGeometry , App::Property);

PropertyGeometry::PropertyGeometry()
{

}

PropertyGeometry::~PropertyGeometry()
{

}

// ------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyComplexGeoData , App::PropertyGeometry);

PropertyComplexGeoData::PropertyComplexGeoData()
{

}

PropertyComplexGeoData::~PropertyComplexGeoData()
{

}
