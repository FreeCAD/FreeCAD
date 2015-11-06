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
#endif

#include "FemPostFilter.h"
#include "FemPostPipeline.h"
#include <Base/Console.h>
#include <App/DocumentObjectPy.h>
#include <vtkFieldData.h>
#include <vtkPointData.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostFilter, Fem::FemPostObject)


FemPostFilter::FemPostFilter()
{
    m_pass = vtkPassThrough::New();
}

FemPostFilter::~FemPostFilter()
{
    //we need to make sure that all vtk filters are disconnected
    //as the would stay alive and connected otherwise
    clearInput();    
}

bool FemPostFilter::valid() {
    return polyDataSource && !m_pipelines.empty() && !m_activePipeline.empty();
}

bool FemPostFilter::isConnected() {
    return valid() && (m_pipelines[m_activePipeline].source->GetTotalNumberOfInputConnections() > 0);
}

bool FemPostFilter::providesPolyData() {
    return isConnected();
}


DocumentObjectExecReturn* FemPostFilter::execute(void) {

    if(isConnected()) {
        
        FilterPipeline& pipe = m_pipelines[m_activePipeline];
        if(pipe.source->GetTotalNumberOfInputConnections() > 0) {
            pipe.target->Update();
            return Fem::FemPostObject::execute();
        }
    }
    
    return StdReturn;
}


void FemPostFilter::clearInput() {

    if(isConnected()) {
        for(std::map<std::string, FilterPipeline>::iterator it = m_pipelines.begin(); it != m_pipelines.end(); ++it) {
            it->second.source->RemoveAllInputConnections(0);
            it->second.source->RemoveAllInputs();
        }
        polyDataSource->RemoveAllInputConnections(0);
    }
}

bool FemPostFilter::hasInputAlgorithmConnected() {
    
    return isConnected();
}

void FemPostFilter::connectInputAlgorithm(vtkSmartPointer< vtkAlgorithm > algo) {

    clearInput();
    if(isValid()) {
        for(std::map<std::string, FilterPipeline>::iterator it = m_pipelines.begin(); it != m_pipelines.end(); ++it) {
            it->second.source->SetInputConnection(algo->GetOutputPort());
        }
        polyDataSource->SetInputConnection(m_pipelines[m_activePipeline].visualisation->GetOutputPort());
        touch();
    }
}

vtkSmartPointer< vtkAlgorithm > FemPostFilter::getConnectedInputAlgorithm() {
    
    if(!isConnected())
        return vtkSmartPointer< vtkAlgorithm >();
    
    return m_pipelines[m_activePipeline].source->GetInputAlgorithm(0,0);
}

bool FemPostFilter::hasInputDataConnected() {

    if(!isValid())
        return false;
    
    return (m_pipelines[m_activePipeline].source->GetInputDataObject(0,0) != NULL);
}


void FemPostFilter::connectInputData(vtkSmartPointer< vtkDataSet > data) {

    clearInput();
    if(isValid()) {

        for(std::map<std::string, FilterPipeline>::iterator it = m_pipelines.begin(); it != m_pipelines.end(); ++it) {
            it->second.source->SetInputDataObject(data);
        }
        polyDataSource->SetInputConnection(m_pipelines[m_activePipeline].visualisation->GetOutputPort());
        touch();
    }
}

vtkSmartPointer< vtkDataObject > FemPostFilter::getConnectedInputData() {

    if(!isValid())
        return vtkSmartPointer< vtkDataSet >();
    
    return m_pipelines[m_activePipeline].source->GetInputDataObject(0,0);
}

vtkSmartPointer< vtkAlgorithm > FemPostFilter::getOutputAlgorithm() {
    return m_pass;
}


void FemPostFilter::addFilterPipeline(const FemPostFilter::FilterPipeline& p, std::string name) {
    m_pipelines[name] = p;
}

FemPostFilter::FilterPipeline& FemPostFilter::getFilterPipeline(std::string name) {
    return m_pipelines[name];
}

void FemPostFilter::setActiveFilterPipeline(std::string name) {
    
    if(m_activePipeline != name && isValid()) {
        m_activePipeline = name;
        m_pass->RemoveAllInputConnections(0);
        polyDataSource->RemoveAllInputConnections(0);
        polyDataSource->SetInputConnection(m_pipelines[m_activePipeline].visualisation->GetOutputPort());
        m_pass->SetInputConnection(m_pipelines[m_activePipeline].target->GetOutputPort());
    }
}



PROPERTY_SOURCE(Fem::FemPostClipFilter, Fem::FemPostFilter)

FemPostClipFilter::FemPostClipFilter(void) : FemPostFilter() {

    ADD_PROPERTY_TYPE(Function, (0), "Clip", App::Prop_None, "The function object which defines the clip regions");
    ADD_PROPERTY_TYPE(InsideOut, (false), "Clip", App::Prop_None, "Invert the clip direction"); 
    ADD_PROPERTY_TYPE(CutCells, (false), "Clip", App::Prop_None, "Decides if cells are cuttet and interpolated or if the cells are kept as a whole"); 
  
    polyDataSource = vtkGeometryFilter::New();
    
    FilterPipeline clip;  
    m_clipper           = vtkTableBasedClipDataSet::New();
    clip.source         = m_clipper;
    clip.target         = m_clipper;
    clip.visualisation  = m_clipper;
    addFilterPipeline(clip, "clip");
    
    FilterPipeline extr;
    m_extractor         = vtkExtractGeometry::New();
    extr.source         = m_extractor;
    extr.target         = m_extractor;
    extr.visualisation  = m_extractor;
    addFilterPipeline(extr, "extract");

    m_extractor->SetExtractInside(0);
    setActiveFilterPipeline("extract"); 
}

FemPostClipFilter::~FemPostClipFilter() {

}

void FemPostClipFilter::onChanged(const Property* prop) {

    if(prop == &Function) {
     
        if(Function.getValue() && Function.getValue()->isDerivedFrom(FemPostFunction::getClassTypeId())) {
            m_clipper->SetClipFunction(static_cast<FemPostFunction*>(Function.getValue())->getImplicitFunction());
            m_extractor->SetImplicitFunction(static_cast<FemPostFunction*>(Function.getValue())->getImplicitFunction());
        }
    }
    else if(prop == &InsideOut) {
    
        m_clipper->SetInsideOut(InsideOut.getValue());
        m_extractor->SetExtractInside( (InsideOut.getValue()) ? 1 : 0 ); 
    }
    else if(prop == &CutCells) {
        
        if(!CutCells.getValue()) 
            setActiveFilterPipeline("extract");
        else 
            setActiveFilterPipeline("clip");
    };              
    
    Fem::FemPostFilter::onChanged(prop);
}




PROPERTY_SOURCE(Fem::FemPostScalarClipFilter, Fem::FemPostFilter)

FemPostScalarClipFilter::FemPostScalarClipFilter(void) : FemPostFilter() {

    ADD_PROPERTY_TYPE(Value, (0), "Clip", App::Prop_None, "The scalar value used to clip the selected field");
    ADD_PROPERTY_TYPE(Scalars, (long(0)), "Clip", App::Prop_None, "The field used to clip");
    ADD_PROPERTY_TYPE(InsideOut, (false), "Clip", App::Prop_None, "Invert the clip direction"); 
  
    Value.setConstraints(&m_constraints);
    
    polyDataSource = vtkGeometryFilter::New();
    
    FilterPipeline clip;  
    m_clipper           = vtkTableBasedClipDataSet::New();
    clip.source         = m_clipper;
    clip.target         = m_clipper;
    clip.visualisation  = m_clipper;
    addFilterPipeline(clip, "clip");
    setActiveFilterPipeline("clip"); 
}

FemPostScalarClipFilter::~FemPostScalarClipFilter() {
    
}

DocumentObjectExecReturn* FemPostScalarClipFilter::execute(void) {
        
    //update the available fields and set the correct input field data for clipping
    if(!isConnected())
        return StdReturn;
 
    std::string val;
    if(m_scalarFields.getEnums() && Scalars.getValue() >= 0)
        val = Scalars.getValueAsString();
    
    std::vector<std::string> array;
    
    vtkDataObject* data;
    if(hasInputAlgorithmConnected()) {
        getConnectedInputAlgorithm()->Update();
        data = getConnectedInputAlgorithm()->GetOutputDataObject(0);
    }
    else 
        data = getConnectedInputData();
   
    vtkDataSet* dset = dynamic_cast<vtkDataSet*>(data);
    if(!dset)
        return StdReturn;
    
    vtkPointData* pd = dset->GetPointData();
    
    for(int i=0; i<pd->GetNumberOfArrays(); ++i) {
        if(pd->GetArray(i)->GetNumberOfComponents()==1)
            array.push_back(pd->GetArrayName(i));
    }

    App::Enumeration empty;
    Scalars.setValue(empty);
    m_scalarFields.setEnums(array);
    Scalars.setValue(m_scalarFields);
    
    std::vector<std::string>::iterator it = std::find(array.begin(), array.end(), val);
    if(!val.empty() && it != array.end())
        Scalars.setValue(val.c_str());
    
    //recalculate the filter
    return Fem::FemPostFilter::execute();
}


void FemPostScalarClipFilter::onChanged(const Property* prop) {
    
    if(prop == &Value) {
        m_clipper->SetValue(Value.getValue());     
    }
    else if(prop == &InsideOut) {
        m_clipper->SetInsideOut(InsideOut.getValue());
    }
    else if(prop == &Scalars && (Scalars.getValue() >= 0)) {
        m_clipper->SetInputArrayToProcess(0, 0, 0, 
                                          vtkDataObject::FIELD_ASSOCIATION_POINTS, Scalars.getValueAsString() );
        setConstraintForField();
    }
    
    Fem::FemPostFilter::onChanged(prop);
}

void FemPostScalarClipFilter::setConstraintForField() {

    vtkDataObject* data;
    if(hasInputAlgorithmConnected()) {
        getConnectedInputAlgorithm()->Update();
        data = getConnectedInputAlgorithm()->GetOutputDataObject(0);
    }
    else 
        data = getConnectedInputData();
   
    vtkDataSet* dset = dynamic_cast<vtkDataSet*>(data);
    if(!dset)
        return;
    
    vtkDataArray* pdata = dset->GetPointData()->GetArray(Scalars.getValueAsString());
    double p[2];
    pdata->GetRange(p);
    m_constraints.LowerBound = p[0];
    m_constraints.UpperBound = p[1];
    m_constraints.StepSize = (p[1]-p[0])/100.;
}


