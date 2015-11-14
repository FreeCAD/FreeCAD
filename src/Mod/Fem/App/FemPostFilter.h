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


#ifndef Fem_FemPostFilter_H
#define Fem_FemPostFilter_H

#include "FemPostObject.h"
#include <App/PropertyUnits.h>

#include <vtkSmartPointer.h>
#include <vtkTableBasedClipDataSet.h>
#include <vtkExtractGeometry.h>
#include <vtkGeometryFilter.h>
#include <vtkPassThrough.h>
#include <vtkPlane.h>
#include <vtkWarpVector.h>

namespace Fem
{

class AppFemExport FemPostFilter : public Fem::FemPostObject
{
    PROPERTY_HEADER(Fem::FemPostFilter);

public:
    /// Constructor
    FemPostFilter(void);
    virtual ~FemPostFilter();
   
    virtual App::DocumentObjectExecReturn* execute(void);
    
    vtkSmartPointer<vtkAlgorithm> getOutputAlgorithm();
    
    bool hasInputAlgorithmConnected();
    void connectInputAlgorithm(vtkSmartPointer<vtkAlgorithm> algo);
    vtkSmartPointer<vtkAlgorithm> getConnectedInputAlgorithm();
    
    bool hasInputDataConnected();
    void connectInputData(vtkSmartPointer<vtkDataSet> data);
    vtkSmartPointer<vtkDataObject> getConnectedInputData();
    void clearInput();
    
    //returns true if the pipelines are set up correctly
    bool valid();
    //returns true if the filter is valid and connected
    bool isConnected();
    //override poly data providing to let the object know we only provide poly data if connected 
    //to something
    virtual bool providesPolyData();

protected:       
    //pipeline handling for derived filter
    struct FilterPipeline {
       vtkSmartPointer<vtkAlgorithm>                    source, target, visualisation;
       std::vector<vtkSmartPointer<vtkAlgorithm> >      algorithmStorage;
    };
    
    void addFilterPipeline(const FilterPipeline& p, std::string name);
    void setActiveFilterPipeline(std::string name);
    FilterPipeline& getFilterPipeline(std::string name);
    
private:
    //handling of multiple pipelines which can be the filter 
    std::map<std::string, FilterPipeline> m_pipelines;
    std::string m_activePipeline;
    vtkSmartPointer<vtkPassThrough> m_pass;
};

class AppFemExport FemPostClipFilter : public FemPostFilter {
  
    PROPERTY_HEADER(Fem::FemPostClipFilter);
    
public:
    FemPostClipFilter(void);        
    virtual ~FemPostClipFilter();
    
    App::PropertyLink           Function;
    App::PropertyBool           InsideOut;
    App::PropertyBool           CutCells;
    
    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemPostClip";
    }
    
protected:
    virtual void onChanged(const App::Property* prop);
    
private:    
    vtkSmartPointer<vtkTableBasedClipDataSet>     m_clipper;
    vtkSmartPointer<vtkExtractGeometry> m_extractor;
};


class AppFemExport FemPostScalarClipFilter : public FemPostFilter {
  
    PROPERTY_HEADER(Fem::FemPostScalarClipFilter);
    
public:
    FemPostScalarClipFilter(void);        
    virtual ~FemPostScalarClipFilter();

    App::PropertyBool            InsideOut;
    App::PropertyFloatConstraint Value;
    App::PropertyEnumeration     Scalars;
    
    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemPostScalarClip";
    }
    
protected:
    virtual App::DocumentObjectExecReturn* execute(void);
    virtual void onChanged(const App::Property* prop);
    void setConstraintForField();
    
private:    
    vtkSmartPointer<vtkTableBasedClipDataSet>   m_clipper;
    App::Enumeration                            m_scalarFields;
    App::PropertyFloatConstraint::Constraints   m_constraints;
};

class AppFemExport FemPostWarpVectorFilter : public FemPostFilter {
  
    PROPERTY_HEADER(Fem::FemPostWarpVectorFilter);
    
public:
    FemPostWarpVectorFilter(void);        
    virtual ~FemPostWarpVectorFilter();

    App::PropertyFloat        Factor;
    App::PropertyEnumeration  Vector;
    
    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemPostWarpVector";
    }
    
protected:
    virtual App::DocumentObjectExecReturn* execute(void);
    virtual void onChanged(const App::Property* prop);
    
private:    
    vtkSmartPointer<vtkWarpVector>   m_warp;
    App::Enumeration                 m_vectorFields;
};

} //namespace Fem


#endif // Fem_FemPostFilter_H
