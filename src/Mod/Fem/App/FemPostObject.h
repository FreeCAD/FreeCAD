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


#pragma once

#include "PropertyPostDataObject.h"
#include <App/GeoFeature.h>
#include <App/PropertyStandard.h>

#include <vtkBoundingBox.h>
#include <vtkTransformFilter.h>
#include <vtkSmartPointer.h>

class vtkDataSet;

namespace Fem
{

// poly data is the only data we can visualize, hence every post
// processing object needs to expose it
class FemExport FemPostObject: public App::GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostObject);

public:
    /// Constructor
    FemPostObject();
    ~FemPostObject() override;

    Fem::PropertyPostDataObject Data;

    // returns the DataSet from the data property. Better use this
    // instead of casting Data.getValue(), as data does not need to be a dataset,
    // but could for example also be a composite data structure.
    // Could return NULL if no dataset is available
    virtual vtkDataSet* getDataSet();

    PyObject* getPyObject() override;

    vtkBoundingBox getBoundingBox();
    void writeVTK(const char* filename) const;

protected:
    // placement is applied via transform filter. However, we do not know
    // how this filter should be used to create data. This is to be implemented
    // by the derived classes.
    vtkSmartPointer<vtkTransformFilter> m_transform_filter;

    void onChanged(const App::Property* prop) override;
};

}  // namespace Fem
