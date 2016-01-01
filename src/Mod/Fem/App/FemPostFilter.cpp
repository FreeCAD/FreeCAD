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
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <vtkFieldData.h>
#include <vtkPointData.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostFilter, Fem::FemPostObject)


FemPostFilter::FemPostFilter()
{
    ADD_PROPERTY(Input,(0));
}

FemPostFilter::~FemPostFilter()
{
 
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
    }
}

DocumentObjectExecReturn* FemPostFilter::execute(void) {
    
    if(!m_pipelines.empty() && !m_activePipeline.empty()) {
    
        FemPostFilter::FilterPipeline& pipe = m_pipelines[m_activePipeline];
        pipe.source->SetInputDataObject(getInputData());
        pipe.target->Update();
        
        Data.setValue(pipe.target->GetOutputDataObject(0));
    }
    return StdReturn;
}

vtkDataObject* FemPostFilter::getInputData() {

    if(Input.getValue()) {
        return Input.getValue<FemPostObject*>()->Data.getValue();
    }
    else {
        //get the pipeline and use the pipelinedata
        std::vector<App::DocumentObject*> objs = getDocument()->getObjectsOfType(FemPostPipeline::getClassTypeId());
        for(std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        
            if(static_cast<FemPostPipeline*>(*it)->holdsPostObject(this)) {
            
                return static_cast<FemPostObject*>(*it)->Data.getValue();
            }
        }
    }
    
    return NULL;
}


PROPERTY_SOURCE(Fem::FemPostClipFilter, Fem::FemPostFilter)

FemPostClipFilter::FemPostClipFilter(void) : FemPostFilter() {

    ADD_PROPERTY_TYPE(Function, (0), "Clip", App::Prop_None, "The function object which defines the clip regions");
    ADD_PROPERTY_TYPE(InsideOut, (false), "Clip", App::Prop_None, "Invert the clip direction"); 
    ADD_PROPERTY_TYPE(CutCells, (false), "Clip", App::Prop_None, "Decides if cells are cuttet and interpolated or if the cells are kept as a whole"); 
    
    FilterPipeline clip;  
    m_clipper           = vtkSmartPointer<vtkTableBasedClipDataSet>::New();
    clip.source         = m_clipper;
    clip.target         = m_clipper;
    addFilterPipeline(clip, "clip");
    
    FilterPipeline extr;
    m_extractor         = vtkSmartPointer<vtkExtractGeometry>::New();
    extr.source         = m_extractor;
    extr.target         = m_extractor;
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

short int FemPostClipFilter::mustExecute(void) const {
    
    if(Function.isTouched() ||
       InsideOut.isTouched() ||
       CutCells.isTouched()) {
        
        return 1;
    }
    else return App::DocumentObject::mustExecute();
}

DocumentObjectExecReturn* FemPostClipFilter::execute(void) {
    
    if(!m_extractor->GetImplicitFunction())
        return StdReturn;
    
    return Fem::FemPostFilter::execute();
}



PROPERTY_SOURCE(Fem::FemPostScalarClipFilter, Fem::FemPostFilter)

FemPostScalarClipFilter::FemPostScalarClipFilter(void) : FemPostFilter() {

    ADD_PROPERTY_TYPE(Value, (0), "Clip", App::Prop_None, "The scalar value used to clip the selected field");
    ADD_PROPERTY_TYPE(Scalars, (long(0)), "Clip", App::Prop_None, "The field used to clip");
    ADD_PROPERTY_TYPE(InsideOut, (false), "Clip", App::Prop_None, "Invert the clip direction"); 
  
    Value.setConstraints(&m_constraints);
       
    FilterPipeline clip;  
    m_clipper           = vtkSmartPointer<vtkTableBasedClipDataSet>::New();
    clip.source         = m_clipper;
    clip.target         = m_clipper;
    addFilterPipeline(clip, "clip");
    setActiveFilterPipeline("clip"); 
}

FemPostScalarClipFilter::~FemPostScalarClipFilter() {
    
}

DocumentObjectExecReturn* FemPostScalarClipFilter::execute(void) {
 
    std::string val;
    if(m_scalarFields.getEnums() && Scalars.getValue() >= 0)
        val = Scalars.getValueAsString();
    
    std::vector<std::string> array;
    
    vtkSmartPointer<vtkDataObject> data = getInputData();
    if(!data || !data->IsA("vtkDataSet"))
        return StdReturn;
    
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);    
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

short int FemPostScalarClipFilter::mustExecute(void) const {
    
    if(Value.isTouched() ||
       InsideOut.isTouched() ||
       Scalars.isTouched()) {
        
        return 1;
    }
    else return App::DocumentObject::mustExecute();
}

void FemPostScalarClipFilter::setConstraintForField() {

    vtkSmartPointer<vtkDataObject> data = getInputData();
    if(!data || !data->IsA("vtkDataSet"))
        return;
    
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
    
    vtkDataArray* pdata = dset->GetPointData()->GetArray(Scalars.getValueAsString());
    double p[2];
    pdata->GetRange(p);
    m_constraints.LowerBound = p[0];
    m_constraints.UpperBound = p[1];
    m_constraints.StepSize = (p[1]-p[0])/100.;
}


PROPERTY_SOURCE(Fem::FemPostWarpVectorFilter, Fem::FemPostFilter)

FemPostWarpVectorFilter::FemPostWarpVectorFilter(void): FemPostFilter() {

    ADD_PROPERTY_TYPE(Factor, (0), "Warp", App::Prop_None, "The factor by which the vector is added to the node positions");
    ADD_PROPERTY_TYPE(Vector, (long(0)), "Warp", App::Prop_None, "The field added to the node position");
       
    FilterPipeline warp;  
    m_warp              = vtkSmartPointer<vtkWarpVector>::New();
    warp.source         = m_warp;
    warp.target         = m_warp;
    addFilterPipeline(warp, "warp");
    setActiveFilterPipeline("warp"); 
}

FemPostWarpVectorFilter::~FemPostWarpVectorFilter() {

}


DocumentObjectExecReturn* FemPostWarpVectorFilter::execute(void) {
 
    std::string val;
    if(m_vectorFields.getEnums() && Vector.getValue() >= 0)
        val = Vector.getValueAsString();
    
    std::vector<std::string> array;
    
    vtkSmartPointer<vtkDataObject> data = getInputData();
    if(!data || !data->IsA("vtkDataSet"))
        return StdReturn;
    
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);      
    vtkPointData* pd = dset->GetPointData();
    
    for(int i=0; i<pd->GetNumberOfArrays(); ++i) {
        if(pd->GetArray(i)->GetNumberOfComponents()==3)
            array.push_back(pd->GetArrayName(i));
    }

    App::Enumeration empty;
    Vector.setValue(empty);
    m_vectorFields.setEnums(array);
    Vector.setValue(m_vectorFields);
    
    std::vector<std::string>::iterator it = std::find(array.begin(), array.end(), val);
    if(!val.empty() && it != array.end())
        Vector.setValue(val.c_str());
    
    //recalculate the filter
    return Fem::FemPostFilter::execute();
}


void FemPostWarpVectorFilter::onChanged(const Property* prop) {
    
    if(prop == &Factor) {
        m_warp->SetScaleFactor(Factor.getValue());     
    }
    else if(prop == &Vector && (Vector.getValue() >= 0)) {
        m_warp->SetInputArrayToProcess(0, 0, 0, 
                                          vtkDataObject::FIELD_ASSOCIATION_POINTS, Vector.getValueAsString() );
    }
    
    Fem::FemPostFilter::onChanged(prop);
}

short int FemPostWarpVectorFilter::mustExecute(void) const {
    
    if(Factor.isTouched() ||
       Vector.isTouched()) {
        
        return 1;
    }
    else return App::DocumentObject::mustExecute();
}


PROPERTY_SOURCE(Fem::FemPostCutFilter, Fem::FemPostFilter)

FemPostCutFilter::FemPostCutFilter(void) : FemPostFilter() {

    ADD_PROPERTY_TYPE(Function, (0), "Cut", App::Prop_None, "The function object which defines the clip cut function");
    
    FilterPipeline clip;  
    m_cutter            = vtkSmartPointer<vtkCutter>::New();
    clip.source         = m_cutter;
    clip.target         = m_cutter;
    addFilterPipeline(clip, "cut");
    setActiveFilterPipeline("cut"); 
}

FemPostCutFilter::~FemPostCutFilter() {

}

void FemPostCutFilter::onChanged(const Property* prop) {

    if(prop == &Function) {
     
        if(Function.getValue() && Function.getValue()->isDerivedFrom(FemPostFunction::getClassTypeId())) {
            m_cutter->SetCutFunction(static_cast<FemPostFunction*>(Function.getValue())->getImplicitFunction());
         }
    }      
    
    Fem::FemPostFilter::onChanged(prop);
}

short int FemPostCutFilter::mustExecute(void) const {
    
    if(Function.isTouched()) {
        
        return 1;
    }
    else return App::DocumentObject::mustExecute();
}

DocumentObjectExecReturn* FemPostCutFilter::execute(void) {
    
    if(!m_cutter->GetCutFunction())
        return StdReturn;
    
    return Fem::FemPostFilter::execute();
}


