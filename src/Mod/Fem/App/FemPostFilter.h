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

#include <vtkArrayCalculator.h>
#include <vtkContourFilter.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkCutter.h>
#include <vtkExtractGeometry.h>
#include <vtkExtractVectorComponents.h>
#include <vtkLineSource.h>
#include <vtkPointSource.h>
#include <vtkProbeFilter.h>
#include <vtkAppendArcLength.h>
#include <vtkSmartPointer.h>
#include <vtkTableBasedClipDataSet.h>
#include <vtkVectorNorm.h>
#include <vtkWarpVector.h>
#include <vtkImplicitFunction.h>

#include <App/PropertyUnits.h>
#include <App/DocumentObjectExtension.h>
#include <App/FeaturePython.h>

#include "FemPostObject.h"


namespace Fem
{

enum class TransformLocation : size_t
{
    input,
    output
};

class FemPostFilterPy;

class FemExport FemPostFilter: public Fem::FemPostObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostFilter);

protected:
    bool dataIsAvailable();
    vtkSmartPointer<vtkDataSet> getInputData();
    std::vector<std::string> getInputVectorFields();
    std::vector<std::string> getInputScalarFields();

    // pipeline handling for derived filter
    struct FilterPipeline
    {
        vtkSmartPointer<vtkAlgorithm> source, target;
        std::vector<vtkSmartPointer<vtkAlgorithm>> algorithmStorage;
    };

    // pipeline handling
    void addFilterPipeline(const FilterPipeline& p, std::string name);
    FilterPipeline& getFilterPipeline(std::string name);
    void setActiveFilterPipeline(std::string name);

    // Transformation handling
    void setTransformLocation(TransformLocation loc);

    friend class FemPostFilterPy;

public:
    /// Constructor
    FemPostFilter();
    ~FemPostFilter() override;

    App::PropertyFloat Frame;

    void onChanged(const App::Property* prop) override;
    App::DocumentObjectExecReturn* execute() override;

    vtkSmartPointer<vtkAlgorithm> getFilterInput();
    vtkSmartPointer<vtkAlgorithm> getFilterOutput();

    PyObject* getPyObject() override;

private:
    // handling of multiple pipelines which can be the filter
    std::map<std::string, FilterPipeline> m_pipelines;
    std::string m_activePipeline;
    bool m_use_transform = false;
    bool m_running_setup = false;
    TransformLocation m_transform_location = TransformLocation::output;

    void pipelineChanged();  // inform parents that the pipeline changed
};

using PostFilterPython = App::FeaturePythonT<FemPostFilter>;

class FemExport FemPostSmoothFilterExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostSmoothFilterExtension);

public:
    FemPostSmoothFilterExtension();
    ~FemPostSmoothFilterExtension() override = default;

    App::PropertyBool BoundarySmoothing;
    App::PropertyAngle EdgeAngle;
    App::PropertyBool EdgeSmoothing;
    App::PropertyBool EnableSmoothing;
    App::PropertyAngle FeatureAngle;
    App::PropertyIntegerConstraint Iterations;
    App::PropertyFloatConstraint RelaxationFactor;

    vtkSmartPointer<vtkSmoothPolyDataFilter> getFilter() const
    {
        return m_smooth;
    }

protected:
    void extensionOnChanged(const App::Property* prop) override;

private:
    vtkSmartPointer<vtkSmoothPolyDataFilter> m_smooth;
    static const App::PropertyQuantityConstraint::Constraints angleRange;
    static const App::PropertyIntegerConstraint::Constraints iterationRange;
    static const App::PropertyFloatConstraint::Constraints relaxationRange;
};

// ***************************************************************************
// in the following, the different filters sorted alphabetically
// ***************************************************************************


// ***************************************************************************
// data along line filter
class FemExport FemPostDataAlongLineFilter: public FemPostFilter
{

    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostDataAlongLineFilter);

public:
    FemPostDataAlongLineFilter();
    ~FemPostDataAlongLineFilter() override;

    App::PropertyVectorDistance Point1;
    App::PropertyVectorDistance Point2;
    App::PropertyInteger Resolution;
    App::PropertyFloatList XAxisData;
    App::PropertyFloatList YAxisData;
    App::PropertyString PlotData;
    App::PropertyEnumeration PlotDataComponent;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostDataAlongLine";
    }
    short int mustExecute() const override;
    void GetAxisData();

protected:
    App::DocumentObjectExecReturn* execute() override;
    void onChanged(const App::Property* prop) override;
    void handleChangedPropertyType(
        Base::XMLReader& reader,
        const char* TypeName,
        App::Property* prop
    ) override;

private:
    vtkSmartPointer<vtkLineSource> m_line;
    vtkSmartPointer<vtkAppendArcLength> m_arclength;
    vtkSmartPointer<vtkProbeFilter> m_probe;
};


// ***************************************************************************
// data at point filter
class FemExport FemPostDataAtPointFilter: public FemPostFilter
{

    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostDataAtPointFilter);

public:
    FemPostDataAtPointFilter();
    ~FemPostDataAtPointFilter() override;

    App::PropertyVectorDistance Center;
    App::PropertyString FieldName;
    App::PropertyFloatList PointData;
    App::PropertyString Unit;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostDataAtPoint";
    }
    short int mustExecute() const override;

protected:
    App::DocumentObjectExecReturn* execute() override;
    void onChanged(const App::Property* prop) override;
    void GetPointData();

private:
    vtkSmartPointer<vtkPointSource> m_point;
    vtkSmartPointer<vtkProbeFilter> m_probe;
};


// ***************************************************************************
// clip filter
class FemExport FemPostClipFilter: public FemPostFilter
{

    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostClipFilter);

public:
    FemPostClipFilter();
    ~FemPostClipFilter() override;

    App::PropertyLink Function;
    App::PropertyBool InsideOut;
    App::PropertyBool CutCells;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostClip";
    }
    short int mustExecute() const override;
    App::DocumentObjectExecReturn* execute() override;

protected:
    void onChanged(const App::Property* prop) override;

private:
    vtkSmartPointer<vtkTableBasedClipDataSet> m_clipper;
    vtkSmartPointer<vtkExtractGeometry> m_extractor;
    vtkSmartPointer<vtkImplicitFunction> m_defaultFunction;
};


// ***************************************************************************
// contours filter
class FemExport FemPostContoursFilter: public FemPostFilter
{

    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostContoursFilter);

public:
    FemPostContoursFilter();
    ~FemPostContoursFilter() override;

    App::PropertyEnumeration Field;
    App::PropertyIntegerConstraint NumberOfContours;
    App::PropertyEnumeration VectorMode;
    App::PropertyBool NoColor;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostContours";
    }
    short int mustExecute() const override;

protected:
    App::DocumentObjectExecReturn* execute() override;
    void onChanged(const App::Property* prop) override;

    void recalculateContours(double min, double max);
    void refreshFields();
    void refreshVectors();
    bool m_blockPropertyChanges = false;

    std::string contourFieldName;
    FemPostSmoothFilterExtension smoothExtension;

private:
    vtkSmartPointer<vtkContourFilter> m_contours;
    vtkSmartPointer<vtkExtractVectorComponents> m_extractor;
    vtkSmartPointer<vtkVectorNorm> m_norm;
    App::Enumeration m_fields;
    App::Enumeration m_vectors;
    App::PropertyIntegerConstraint::Constraints m_contourConstraints;
};


// ***************************************************************************
// cut filter
class FemExport FemPostCutFilter: public FemPostFilter
{

    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostCutFilter);

public:
    FemPostCutFilter();
    ~FemPostCutFilter() override;

    App::PropertyLink Function;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostCut";
    }
    short int mustExecute() const override;
    App::DocumentObjectExecReturn* execute() override;

protected:
    void onChanged(const App::Property* prop) override;

private:
    vtkSmartPointer<vtkCutter> m_cutter;
    vtkSmartPointer<vtkImplicitFunction> m_defaultFunction;
};


// ***************************************************************************
// scalar clip filter
class FemExport FemPostScalarClipFilter: public FemPostFilter
{

    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostScalarClipFilter);

public:
    FemPostScalarClipFilter();
    ~FemPostScalarClipFilter() override;

    App::PropertyBool InsideOut;
    App::PropertyFloatConstraint Value;
    App::PropertyEnumeration Scalars;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostScalarClip";
    }
    short int mustExecute() const override;

protected:
    App::DocumentObjectExecReturn* execute() override;
    void onChanged(const App::Property* prop) override;
    void setConstraintForField();

private:
    vtkSmartPointer<vtkTableBasedClipDataSet> m_clipper;
    App::Enumeration m_scalarFields;
    App::PropertyFloatConstraint::Constraints m_constraints;
};


// ***************************************************************************
// warp vector filter
class FemExport FemPostWarpVectorFilter: public FemPostFilter
{

    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostWarpVectorFilter);

public:
    FemPostWarpVectorFilter();
    ~FemPostWarpVectorFilter() override;

    App::PropertyFloat Factor;
    App::PropertyEnumeration Vector;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostWarpVector";
    }
    short int mustExecute() const override;

protected:
    App::DocumentObjectExecReturn* execute() override;
    void onChanged(const App::Property* prop) override;

private:
    vtkSmartPointer<vtkWarpVector> m_warp;
    App::Enumeration m_vectorFields;
};

// ***************************************************************************
// calculator filter
class FemExport FemPostCalculatorFilter: public FemPostFilter
{

    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostCalculatorFilter);

public:
    FemPostCalculatorFilter();
    ~FemPostCalculatorFilter() override;

    App::PropertyString FieldName;
    App::PropertyString Function;
    App::PropertyFloat ReplacementValue;
    App::PropertyBool ReplaceInvalid;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostCalculator";
    }
    short int mustExecute() const override;

    const std::vector<std::string> getScalarVariables();
    const std::vector<std::string> getVectorVariables();

protected:
    App::DocumentObjectExecReturn* execute() override;
    void onChanged(const App::Property* prop) override;

    void updateAvailableFields();

private:
    vtkSmartPointer<vtkArrayCalculator> m_calculator;
};

}  // namespace Fem
