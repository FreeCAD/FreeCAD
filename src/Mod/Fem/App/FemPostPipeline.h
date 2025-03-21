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

#ifndef Fem_FemPostPipeline_H
#define Fem_FemPostPipeline_H

#include "Base/Unit.h"
#include "FemPostGroupExtension.h"

#include "FemPostFilter.h"
#include "FemPostFunction.h"
#include "FemPostObject.h"
#include "FemResultObject.h"

#include <vtkSmartPointer.h>
#include <vtkUnstructuredGridAlgorithm.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>


namespace Fem
{

// algorithm that allows multi frame handling: if data is stored in MultiBlock dataset
// this source enables the downstream filters to query the blocks as different time frames
class FemFrameSourceAlgorithm : public vtkUnstructuredGridAlgorithm
{
public:
    static FemFrameSourceAlgorithm* New();
    vtkTypeMacro(FemFrameSourceAlgorithm, vtkUnstructuredGridAlgorithm);

    bool isValid();
    void setDataObject(vtkSmartPointer<vtkDataObject> data);
    std::vector<double> getFrameValues();

protected:
    FemFrameSourceAlgorithm();
    ~FemFrameSourceAlgorithm() override;

    vtkSmartPointer<vtkDataObject> m_data;

    int RequestInformation(vtkInformation* reqInfo, vtkInformationVector** inVector, vtkInformationVector* outVector) override;
    int RequestData(vtkInformation* reqInfo, vtkInformationVector** inVector, vtkInformationVector* outVector) override;
};


class FemExport FemPostPipeline: public Fem::FemPostObject, public Fem::FemPostGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Fem::FemPostPipeline);

public:
    /// Constructor
    FemPostPipeline();
    ~FemPostPipeline() override;

    App::PropertyEnumeration Frame;


    virtual vtkDataSet* getDataSet() override;
    Fem::FemPostFunctionProvider* getFunctionProvider();

    App::DocumentObjectExecReturn* execute() override;
    PyObject* getPyObject() override;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostPipeline";
    }

    // load data from files
    static bool canRead(Base::FileInfo file);
    void read(Base::FileInfo file);
    void scale(double s);

    // load from results
    void load(FemResultObject* res);
    void load(std::vector<FemResultObject*> res, std::vector<double> values, Base::Unit unit, std::string frame_type);

    // Group pipeline handling
    void filterChanged(FemPostFilter* filter) override;
    void filterPipelineChanged(FemPostFilter* filter) override;

    // frame handling
    bool hasFrames();
    std::string getFrameType();
    Base::Unit getFrameUnit();
    unsigned int getFrameNumber();
    std::vector<double> getFrameValues();

protected:
    void onChanged(const App::Property* prop) override;
    bool allowObject(App::DocumentObject* obj) override;

    // update documents
    void handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName) override;
    void onDocumentRestored() override;

private:
    App::Enumeration  m_frameEnum;
    vtkSmartPointer<FemFrameSourceAlgorithm> m_source_algorithm;

    bool m_block_property = false;
    bool m_data_updated   = false;
    bool m_use_transform  = false;
    void updateData();


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


#endif  // Fem_FemPostPipeline_H
