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

#include <Base/MatrixPy.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyMatrix.h"

namespace App {

TYPESYSTEM_SOURCE(App::PropertyMatrix, App::Property)

PropertyMatrix::PropertyMatrix() = default;

PropertyMatrix::~PropertyMatrix() = default;

//**************************************************************************
// Base class implementer


void PropertyMatrix::setValue(const Base::Matrix4D& mat)
{
    aboutToSetValue();
    _cMat = mat;
    hasSetValue();
}


const Base::Matrix4D& PropertyMatrix::getValue() const
{
    return _cMat;
}

PyObject* PropertyMatrix::getPyObject()
{
    return new Base::MatrixPy(_cMat);
}

void PropertyMatrix::setPyObject(PyObject* value)
{
    if (PyObject_TypeCheck(value, &(Base::MatrixPy::Type))) {
        Base::MatrixPy* pcObject = static_cast<Base::MatrixPy*>(value);
        setValue(pcObject->value());
    }
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 16) {
        PyObject* item;
        Base::Matrix4D cMatrix;

        const int dim = 4;
        for (int x = 0; x < dim; x++) {
            for (int y = 0; y < dim; y++) {
                item = PyTuple_GetItem(value, x + y * dim);
                if (PyFloat_Check(item)) {
                    cMatrix[x][y] = PyFloat_AsDouble(item);
                }
                else if (PyLong_Check(item)) {
                    cMatrix[x][y] = (double)PyLong_AsLong(item);
                }
                else {
                    throw Base::TypeError(
                        "Not allowed type used in matrix tuple (a number expected)...");
                }
            }
        }

        setValue(cMatrix);
    }
    else {
        std::string error = std::string("type must be 'Matrix' or tuple of 16 float or int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyMatrix::Save(Base::Writer& writer) const
{
    // clang-format off
    writer.Stream() << writer.ind() << "<PropertyMatrix";
    writer.Stream() << " a11=\"" <<  _cMat[0][0] << "\" a12=\"" <<  _cMat[0][1] << "\" a13=\"" <<  _cMat[0][2] << "\" a14=\"" <<  _cMat[0][3] << "\"";
    writer.Stream() << " a21=\"" <<  _cMat[1][0] << "\" a22=\"" <<  _cMat[1][1] << "\" a23=\"" <<  _cMat[1][2] << "\" a24=\"" <<  _cMat[1][3] << "\"";
    writer.Stream() << " a31=\"" <<  _cMat[2][0] << "\" a32=\"" <<  _cMat[2][1] << "\" a33=\"" <<  _cMat[2][2] << "\" a34=\"" <<  _cMat[2][3] << "\"";
    writer.Stream() << " a41=\"" <<  _cMat[3][0] << "\" a42=\"" <<  _cMat[3][1] << "\" a43=\"" <<  _cMat[3][2] << "\" a44=\"" <<  _cMat[3][3] << "\"";
    writer.Stream() <<"/>" << std::endl;
    // clang-format on
}

void PropertyMatrix::Restore(Base::XMLReader& reader)
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


Property* PropertyMatrix::Copy() const
{
    PropertyMatrix* p = new PropertyMatrix();
    p->_cMat = _cMat;
    return p;
}

void PropertyMatrix::Paste(const Property& from)
{
    aboutToSetValue();
    _cMat = dynamic_cast<const PropertyMatrix&>(from)._cMat;
    hasSetValue();
}

}  // namespace App
