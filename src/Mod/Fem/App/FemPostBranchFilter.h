/***************************************************************************
 *   Copyright (c) 2024 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include "FemPostFilter.h"
#include "FemPostGroupExtension.h"

#include <vtkSmartPointer.h>
#include <vtkAppendFilter.h>
#include <vtkPassThrough.h>


namespace Fem
{

class FemExport FemPostBranchFilter: public Fem::FemPostFilter, public FemPostGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Fem::FemPostBranchFilter);

public:
    /// Constructor
    FemPostBranchFilter();

    App::PropertyEnumeration Output;


    short mustExecute() const override;
    PyObject* getPyObject() override;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostBranchFilter";
    }

    // Branch handling
    void filterChanged(FemPostFilter* filter) override;
    void filterPipelineChanged(FemPostFilter* filter) override;

protected:
    void onChanged(const App::Property* prop) override;

private:
    static const char* OutputEnums[];

    void setupPipeline();

    vtkSmartPointer<vtkAppendFilter> m_append;
    vtkSmartPointer<vtkPassThrough> m_passthrough;
};

}  // namespace Fem
