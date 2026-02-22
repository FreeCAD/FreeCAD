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

#include "Base/Unit.h"
#include "FemPostGroupExtension.h"

#include "FemPostFilter.h"
#include "FemPostFunction.h"
#include "FemPostObject.h"
#include "FemResultObject.h"
#include "VTKExtensions/vtkFemFrameSourceAlgorithm.h"

#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 2, 20230125)
# include "VTKExtensions/vtkCleanUnstructuredGrid.h"
#else
# include <vtkCleanUnstructuredGrid.h>
#endif
#include <vtkSmartPointer.h>


namespace Fem
{

class FemExport FemPostPipeline: public Fem::FemPostObject, public Fem::FemPostGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Fem::FemPostPipeline);

public:
    /// Constructor
    FemPostPipeline();

    App::PropertyEnumeration Frame;
    App::PropertyBool MergeDuplicate;

    virtual vtkDataSet* getDataSet() override;
    Fem::FemPostFunctionProvider* getFunctionProvider();

    App::DocumentObjectExecReturn* execute() override;
    PyObject* getPyObject() override;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostPipeline";
    }

    // load data from files (single or as multiframe)
    static bool canRead(Base::FileInfo file);
    void read(Base::FileInfo file);
    void read(
        std::vector<Base::FileInfo>& files,
        std::vector<double>& values,
        Base::Unit unit,
        std::string& frame_type
    );
    void scale(double s);
    void renameArrays(const std::map<std::string, std::string>& names);

    // load from results
    void load(FemResultObject* res);
    void load(
        std::vector<FemResultObject*>& res,
        std::vector<double>& values,
        Base::Unit unit,
        std::string& frame_type
    );

    // Group pipeline handling
    void filterChanged(FemPostFilter* filter) override;
    void filterPipelineChanged(FemPostFilter* filter) override;

    // frame handling
    bool hasFrames();
    std::string getFrameType();
    Base::Unit getFrameUnit();
    unsigned int getFrameNumber();
    std::vector<double> getFrameValues();

    // output algorithm handling
    vtkSmartPointer<vtkAlgorithm> getOutputAlgorithm()
    {
        return m_source_algorithm;
    }

protected:
    void onChanged(const App::Property* prop) override;
    bool allowObject(App::DocumentObject* obj) override;

    // update documents
    void handleChangedPropertyName(
        Base::XMLReader& reader,
        const char* TypeName,
        const char* PropName
    ) override;
    void onDocumentRestored() override;

private:
    App::Enumeration m_frameEnum;

    vtkSmartPointer<vtkFemFrameSourceAlgorithm> m_source_algorithm;
    vtkSmartPointer<vtkCleanUnstructuredGrid> m_clean_filter;

    bool m_block_property = false;
    bool m_data_updated = false;
    void updateData();


    template<class TReader>
    vtkSmartPointer<vtkDataObject> readXMLFile(std::string file)
    {

        vtkSmartPointer<TReader> reader = vtkSmartPointer<TReader>::New();
        reader->SetFileName(file.c_str());
        reader->Update();
        return reader->GetOutput();
    }
    vtkSmartPointer<vtkDataObject> dataObjectFromFile(const Base::FileInfo& File);
    // read .pvd file into multiblock dataset
    vtkSmartPointer<vtkDataObject> readPVD(const Base::FileInfo& file);
};

}  // namespace Fem
