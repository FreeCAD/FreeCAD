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

#ifndef Fem_FemPostBranch_H
#define Fem_FemPostBranch_H

#include "App/GroupExtension.h"

#include "FemPostFilter.h"
#include "FemPostFunction.h"
#include "FemPostObject.h"
#include "FemResultObject.h"

#include <vtkSmartPointer.h>
#include <vtkAppendFilter.h>
#include <vtkPassThrough.h>


namespace Fem
{

class FemExport FemPostBranch: public Fem::FemPostFilter, public App::GroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Fem::FemPostBranch);

public:
    /// Constructor
    FemPostBranch();
    ~FemPostBranch() override;

    App::PropertyEnumeration Mode;
    App::PropertyEnumeration Output;


    short mustExecute() const override;
    PyObject* getPyObject() override;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostBranch";
    }

    // load data from files
    static bool canRead(Base::FileInfo file);
    void read(Base::FileInfo file);
    void scale(double s);

    // load from results
    void load(FemResultObject* res);

    // Branch handling
    void filterChanged(FemPostFilter* filter);
    void pipelineChanged(FemPostFilter* filter);
    void recomputeChildren();
    FemPostObject* getLastPostObject();
    bool holdsPostObject(FemPostObject* obj);

protected:
    void onChanged(const App::Property* prop) override;

private:
    static const char* ModeEnums[];
    static const char* OutputEnums[];

    vtkSmartPointer<vtkAppendFilter> m_append;
    vtkSmartPointer<vtkPassThrough>  m_passthrough;

    template<class TReader>
    void readXMLFile(std::string file)
    {

        vtkSmartPointer<TReader> reader = vtkSmartPointer<TReader>::New();
        reader->SetFileName(file.c_str());
        reader->Update();
        Data.setValue(reader->GetOutput());
    }
};

}  // namespace Fem


#endif  // Fem_FemPostBranch_H
