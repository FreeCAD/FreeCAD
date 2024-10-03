/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include <vtkDataSet.h>
#include <vtkXMLDataSetWriter.h>
#endif

#include <Base/Exception.h>

#include "FemPostObject.h"
#include "FemPostObjectPy.h"


using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostObject, App::GeoFeature)


FemPostObject::FemPostObject()
{
    ADD_PROPERTY(Data, (nullptr));
}

FemPostObject::~FemPostObject() = default;

vtkBoundingBox FemPostObject::getBoundingBox()
{

    vtkBoundingBox box;

    vtkDataSet* dset = vtkDataSet::SafeDownCast(Data.getValue());
    if (dset) {
        box.AddBounds(dset->GetBounds());
    }

    // TODO: add calculation of multiblock and Multipiece datasets

    return box;
}

PyObject* FemPostObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FemPostObjectPy(this), true);
    }

    return Py::new_reference_to(PythonObject);
}

namespace
{

template<typename T>
void femVTKWriter(const char* filename, const vtkSmartPointer<vtkDataObject>& dataObject)
{
    if (vtkDataSet::SafeDownCast(dataObject)->GetNumberOfPoints() <= 0) {
        throw Base::ValueError("Empty data object");
    }

    vtkSmartPointer<T> writer = vtkSmartPointer<T>::New();
    writer->SetFileName(filename);
    writer->SetDataModeToBinary();
    writer->SetInputDataObject(dataObject);
    writer->Write();
}

std::string vtkWriterExtension(const vtkSmartPointer<vtkDataObject>& dataObject)
{
    std::string extension;
    switch (dataObject->GetDataObjectType()) {
        case VTK_POLY_DATA:
            extension = "vtp";
            break;
        case VTK_STRUCTURED_GRID:
            extension = "vts";
            break;
        case VTK_RECTILINEAR_GRID:
            extension = "vtr";
            break;
        case VTK_UNSTRUCTURED_GRID:
            extension = "vtu";
            break;
        case VTK_UNIFORM_GRID:
            extension = "vti";
            break;
        default:
            break;
    }

    return extension;
}

}  // namespace

void FemPostObject::writeVTK(const char* filename) const
{
    const vtkSmartPointer<vtkDataObject>& data = Data.getValue();

    // set appropriate filename extension
    std::string name(filename);
    std::string extension = vtkWriterExtension(data);
    if (extension.empty()) {
        throw Base::TypeError("Unsupported data type");
    }

    std::string::size_type pos = name.find_last_of('.');
    if (pos != std::string::npos) {
        name = name.substr(0, pos + 1).append(extension);
    }
    else {
        name = name.append(".").append(extension);
    }

    femVTKWriter<vtkXMLDataSetWriter>(name.c_str(), data);
}
