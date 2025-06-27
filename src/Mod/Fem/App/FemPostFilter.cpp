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
#include <Python.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkUnstructuredGrid.h>
#endif

#include <App/FeaturePythonPyImp.h>
#include <App/Document.h>
#include <Base/Console.h>

#include "FemPostFilter.h"
#include "FemPostFilterPy.h"

#include "FemPostPipeline.h"
#include "FemPostBranchFilter.h"


using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostFilter, Fem::FemPostObject)


FemPostFilter::FemPostFilter()
{
    ADD_PROPERTY_TYPE(Frame,
                      ((long)0),
                      "Data",
                      App::Prop_ReadOnly,
                      "The step used to calculate the data");

    // the default pipeline: just a passthrough
    // this is used to simplify the python filter handling,
    // as those do not have filter pipelines setup till later
    // in the document loading process.
    auto filter = vtkPassThrough::New();
    auto pipeline = FemPostFilter::FilterPipeline();
    pipeline.algorithmStorage.push_back(filter);
    pipeline.source = filter;
    pipeline.target = filter;
    addFilterPipeline(pipeline, "__passthrough__");
}

FemPostFilter::~FemPostFilter() = default;


void FemPostFilter::addFilterPipeline(const FemPostFilter::FilterPipeline& p, std::string name)
{
    m_pipelines[name] = p;

    if (m_activePipeline.empty()) {
        m_activePipeline = name;
    }
}

FemPostFilter::FilterPipeline& FemPostFilter::getFilterPipeline(std::string name)
{
    return m_pipelines.at(name);
}

void FemPostFilter::setActiveFilterPipeline(std::string name)
{
    if (m_pipelines.count(name) == 0) {
        throw Base::ValueError("Not a filter pipeline name");
    }

    if (m_activePipeline != name && isValid()) {

        // disable all inputs of current pipeline
        if (m_activePipeline != "" && m_pipelines.find(m_activePipeline) != m_pipelines.end()) {
            m_pipelines[m_activePipeline].source->RemoveAllInputConnections(0);
        }

        // handle the transform
        if (m_use_transform) {
            m_transform_filter->RemoveAllInputConnections(0);
            if (m_transform_location == TransformLocation::output) {
                m_transform_filter->SetInputConnection(m_pipelines[name].target->GetOutputPort(0));
            }
            else {
                m_pipelines[name].source->SetInputConnection(m_transform_filter->GetOutputPort(0));
            }
        }

        // set the new pipeline active
        m_activePipeline = name;
        pipelineChanged();
    }
}

vtkSmartPointer<vtkAlgorithm> FemPostFilter::getFilterInput()
{
    if (m_use_transform && m_transform_location == TransformLocation::input) {

        return m_transform_filter;
    }

    return m_pipelines[m_activePipeline].source;
}

vtkSmartPointer<vtkAlgorithm> FemPostFilter::getFilterOutput()
{
    if (m_use_transform && m_transform_location == TransformLocation::output) {

        return m_transform_filter;
    }

    return m_pipelines[m_activePipeline].target;
}

void FemPostFilter::pipelineChanged()
{
    // inform our parent, that we need to be reconnected
    App::DocumentObject* group = FemPostGroupExtension::getGroupOfObject(this);
    if (!group) {
        return;
    }
    if (group->hasExtension(FemPostGroupExtension::getExtensionClassTypeId())) {
        auto postgroup = group->getExtensionByType<FemPostGroupExtension>();
        postgroup->filterPipelineChanged(this);
    }
}

void FemPostFilter::onChanged(const App::Property* prop)
{

    if (prop == &Placement) {

        if (Placement.getValue().isIdentity() && m_use_transform) {
            // remove transform from pipeline
            if (m_transform_location == TransformLocation::output) {
                m_transform_filter->RemoveAllInputConnections(0);
            }
            else {
                m_pipelines[m_activePipeline].source->RemoveAllInputConnections(0);
            }
            m_use_transform = false;
            pipelineChanged();
        }
        if (!Placement.getValue().isIdentity() && !m_use_transform) {
            // add transform to pipeline
            if (m_transform_location == TransformLocation::output) {
                m_transform_filter->SetInputConnection(
                    m_pipelines[m_activePipeline].target->GetOutputPort(0));
            }
            else {
                m_pipelines[m_activePipeline].source->SetInputConnection(
                    m_transform_filter->GetOutputPort(0));
            }
            m_use_transform = true;
            pipelineChanged();
        }
    }

    // make sure we inform our parent object that we changed, it then can inform others if needed
    App::DocumentObject* group = FemPostGroupExtension::getGroupOfObject(this);
    if (group && group->hasExtension(FemPostGroupExtension::getExtensionClassTypeId())) {
        auto postgroup = group->getExtensionByType<FemPostGroupExtension>();
        postgroup->filterChanged(this);
    }

    return FemPostObject::onChanged(prop);
}


DocumentObjectExecReturn* FemPostFilter::execute()
{
    // the pipelines are setup correctly, all we need to do is to update and take out the data.
    if (!m_pipelines.empty() && !m_activePipeline.empty()) {

        auto active = m_pipelines[m_activePipeline];
        if (active.source->GetNumberOfInputConnections(0) == 0) {
            return nullptr;
        }

        auto output = getFilterOutput();

        if (output->GetNumberOfInputConnections(0) == 0) {
            return StdReturn;
        }

        if (Frame.getValue() > 0) {
            output->UpdateTimeStep(Frame.getValue());
        }
        else {
            output->Update();
        }

        Data.setValue(output->GetOutputDataObject(0));
    }
    return StdReturn;
}

vtkSmartPointer<vtkDataSet> FemPostFilter::getInputData()
{
    auto active = m_pipelines[m_activePipeline];
    if (active.source->GetNumberOfInputConnections(0) == 0) {
        return nullptr;
    }

    vtkAlgorithmOutput* output = active.source->GetInputConnection(0, 0);
    if (!output) {
        return nullptr;
    }
    vtkAlgorithm* algo = output->GetProducer();
    if (!algo) {
        return nullptr;
    }
    if (Frame.getValue() > 0) {
        algo->UpdateTimeStep(Frame.getValue());
    }
    else {
        algo->Update();
    }
    return vtkDataSet::SafeDownCast(algo->GetOutputDataObject(0));
}

std::vector<std::string> FemPostFilter::getInputVectorFields()
{
    vtkDataSet* dset = getInputData();
    if (!dset) {
        return std::vector<std::string>();
    }
    vtkPointData* pd = dset->GetPointData();

    // get all vector fields
    std::vector<std::string> VectorArray;
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        if (pd->GetArray(i)->GetNumberOfComponents() == 3) {
            VectorArray.emplace_back(pd->GetArrayName(i));
        }
    }

    return VectorArray;
}

std::vector<std::string> FemPostFilter::getInputScalarFields()
{
    vtkDataSet* dset = getInputData();
    if (!dset) {
        return std::vector<std::string>();
    }
    vtkPointData* pd = dset->GetPointData();

    // get all scalar fields
    std::vector<std::string> ScalarArray;
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        if (pd->GetArray(i)->GetNumberOfComponents() == 1) {
            ScalarArray.emplace_back(pd->GetArrayName(i));
        }
    }

    return ScalarArray;
}

void FemPostFilter::setTransformLocation(TransformLocation loc)
{
    m_transform_location = loc;
}

PyObject* FemPostFilter::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FemPostFilterPy(this), true);
    }

    return Py::new_reference_to(PythonObject);
}


// Python Filter feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Fem::PostFilterPython, Fem::FemPostFilter)
template<>
const char* Fem::PostFilterPython::getViewProviderName(void) const
{
    return "FemGui::ViewProviderPostFilterPython";
}
template<>
PyObject* Fem::PostFilterPython::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new App::FeaturePythonPyT<FemPostFilterPy>(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

/// @endcond

// explicit template instantiation
template class FemExport FeaturePythonT<Fem::FemPostFilter>;
}  // namespace App


// ***************************************************************************
// in the following, the different filters sorted alphabetically
// ***************************************************************************


// ***************************************************************************
// data along line filter
PROPERTY_SOURCE(Fem::FemPostDataAlongLineFilter, Fem::FemPostFilter)

FemPostDataAlongLineFilter::FemPostDataAlongLineFilter()
    : FemPostFilter()
{
    ADD_PROPERTY_TYPE(Point1,
                      (Base::Vector3d(0.0, 0.0, 0.0)),
                      "DataAlongLine",
                      App::Prop_None,
                      "The point 1 used to define end point of line");
    ADD_PROPERTY_TYPE(Point2,
                      (Base::Vector3d(0.0, 0.0, 1.0)),
                      "DataAlongLine",
                      App::Prop_None,
                      "The point 2 used to define end point of line");
    ADD_PROPERTY_TYPE(Resolution,
                      (100),
                      "DataAlongLine",
                      App::Prop_None,
                      "The number of intervals between the 2 end points of line");
    ADD_PROPERTY_TYPE(XAxisData,
                      (0),
                      "DataAlongLine",
                      App::Prop_None,
                      "X axis data values used for plotting");
    ADD_PROPERTY_TYPE(YAxisData,
                      (0),
                      "DataAlongLine",
                      App::Prop_None,
                      "Y axis data values used for plotting");
    ADD_PROPERTY_TYPE(PlotData, (""), "DataAlongLine", App::Prop_None, "Field used for plotting");
    ADD_PROPERTY_TYPE(PlotDataComponent,
                      ((long)0),
                      "DataAlongLine",
                      App::Prop_None,
                      "Field component used for plotting");

    PlotData.setStatus(App::Property::ReadOnly, true);
    PlotDataComponent.setStatus(App::Property::ReadOnly, true);
    XAxisData.setStatus(App::Property::Output, true);
    YAxisData.setStatus(App::Property::Output, true);

    FilterPipeline clip;

    m_line = vtkSmartPointer<vtkLineSource>::New();
    const Base::Vector3d& vec1 = Point1.getValue();
    m_line->SetPoint1(vec1.x, vec1.y, vec1.z);
    const Base::Vector3d& vec2 = Point2.getValue();
    m_line->SetPoint2(vec2.x, vec2.y, vec2.z);
    m_line->SetResolution(Resolution.getValue());

    m_arclength = vtkSmartPointer<vtkAppendArcLength>::New();
    m_arclength->SetInputConnection(m_line->GetOutputPort(0));


    auto passthrough = vtkSmartPointer<vtkPassThrough>::New();
    m_probe = vtkSmartPointer<vtkProbeFilter>::New();
    m_probe->SetSourceConnection(passthrough->GetOutputPort(0));
    m_probe->SetInputConnection(m_arclength->GetOutputPort());
    m_probe->SetPassPointArrays(1);
    m_probe->SetPassCellArrays(1);
    m_probe->ComputeToleranceOff();
    m_probe->SetTolerance(0.01);

    clip.source = passthrough;
    clip.algorithmStorage.push_back(m_arclength);
    clip.target = m_probe;

    addFilterPipeline(clip, "DataAlongLine");
    setActiveFilterPipeline("DataAlongLine");
}

FemPostDataAlongLineFilter::~FemPostDataAlongLineFilter() = default;

DocumentObjectExecReturn* FemPostDataAlongLineFilter::execute()
{
    // recalculate the filter
    return Fem::FemPostFilter::execute();
}

void FemPostDataAlongLineFilter::handleChangedPropertyType(Base::XMLReader& reader,
                                                           const char* TypeName,
                                                           App::Property* prop)
// transforms properties that had been changed
{
    // property Point1 had the App::PropertyVector and was changed to App::PropertyVectorDistance
    if (prop == &Point1 && strcmp(TypeName, "App::PropertyVector") == 0) {
        App::PropertyVector Point1Property;
        // restore the PropertyFloat to be able to set its value
        Point1Property.Restore(reader);
        Point1.setValue(Point1Property.getValue());
    }
    // property Point2 had the App::PropertyVector and was changed to App::PropertyVectorDistance
    else if (prop == &Point2 && strcmp(TypeName, "App::PropertyVector") == 0) {
        App::PropertyVector Point2Property;
        Point2Property.Restore(reader);
        Point2.setValue(Point2Property.getValue());
    }
    else {
        FemPostFilter::handleChangedPropertyType(reader, TypeName, prop);
    }
}

void FemPostDataAlongLineFilter::onChanged(const Property* prop)
{
    if (prop == &Point1) {
        const Base::Vector3d& vec1 = Point1.getValue();
        m_line->SetPoint1(vec1.x, vec1.y, vec1.z);
    }
    else if (prop == &Point2) {
        const Base::Vector3d& vec2 = Point2.getValue();
        m_line->SetPoint2(vec2.x, vec2.y, vec2.z);
    }
    else if (prop == &Resolution) {
        m_line->SetResolution(Resolution.getValue());
    }
    else if (prop == &PlotData) {
        GetAxisData();
    }
    else if (prop == &PlotDataComponent) {
        GetAxisData();
    }

    Fem::FemPostFilter::onChanged(prop);
}

short int FemPostDataAlongLineFilter::mustExecute() const
{
    if (Point1.isTouched() || Point2.isTouched() || Resolution.isTouched()) {
        return 1;
    }
    else {
        return App::DocumentObject::mustExecute();
    }
}

void FemPostDataAlongLineFilter::GetAxisData()
{
    std::vector<double> coords;
    std::vector<double> values;

    vtkSmartPointer<vtkDataObject> data = m_probe->GetOutputDataObject(0);
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
    if (!dset) {
        return;
    }
    vtkDataArray* pdata = dset->GetPointData()->GetArray(PlotData.getValue());
    // VTK cannot deliver data when the filer relies e.g. on a scalar clip filter
    // whose value is set so that all data are clipped
    if (!pdata) {
        return;
    }

    // expected "Magnitude" -> 0; "X" -> 1; "Y" -> 2, "Z" -> 3
    vtkIdType component = PlotDataComponent.getValue();
    // prevent selecting a component out of range
    if (!PlotDataComponent.isValid() || component > pdata->GetNumberOfComponents()) {
        return;
    }

    vtkDataArray* alength = dset->GetPointData()->GetArray("arc_length");

    for (vtkIdType i = 0; i < dset->GetNumberOfPoints(); ++i) {
        double value = 0;
        if (pdata) {
            if (pdata->GetNumberOfComponents() == 1) {
                value = pdata->GetComponent(i, 0);
            }
            else if (pdata->GetNumberOfComponents() > 1) {
                if (component) {
                    value = pdata->GetComponent(i, component - 1);
                }
                else {
                    // compute magnitude
                    for (vtkIdType j = 0; j < pdata->GetNumberOfComponents(); ++j) {
                        value += std::pow(pdata->GetComponent(i, j), 2);
                    }

                    value = std::sqrt(value);
                }
            }
        }

        values.push_back(value);
        coords.push_back(alength->GetTuple1(i));
    }

    YAxisData.setValues(values);
    XAxisData.setValues(coords);
}


// ***************************************************************************
// data at point filter
PROPERTY_SOURCE(Fem::FemPostDataAtPointFilter, Fem::FemPostFilter)

FemPostDataAtPointFilter::FemPostDataAtPointFilter()
    : FemPostFilter()
{
    ADD_PROPERTY_TYPE(Center,
                      (Base::Vector3d(0.0, 0.0, 0.0)),
                      "DataAtPoint",
                      App::Prop_None,
                      "Center of the point");
    ADD_PROPERTY_TYPE(PointData, (0), "DataAtPoint", App::Prop_None, "Point data values");
    ADD_PROPERTY_TYPE(FieldName, (""), "DataAtPoint", App::Prop_None, "Field used for plotting");
    ADD_PROPERTY_TYPE(Unit, (""), "DataAtPoint", App::Prop_None, "Unit used for the field");

    PointData.setStatus(App::Property::Output, true);
    FieldName.setStatus(App::Property::ReadOnly, true);
    Unit.setStatus(App::Property::ReadOnly, true);

    FilterPipeline clip;

    m_point = vtkSmartPointer<vtkPointSource>::New();
    const Base::Vector3d& vec = Center.getValue();
    m_point->SetCenter(vec.x, vec.y, vec.z);
    m_point->SetRadius(0);

    auto passthrough = vtkSmartPointer<vtkPassThrough>::New();
    m_probe = vtkSmartPointer<vtkProbeFilter>::New();
    m_probe->SetSourceConnection(passthrough->GetOutputPort(0));
    m_probe->SetInputConnection(m_point->GetOutputPort());
    m_probe->SetValidPointMaskArrayName("ValidPointArray");
    m_probe->SetPassPointArrays(1);
    m_probe->SetPassCellArrays(1);
    m_probe->ComputeToleranceOff();
    m_probe->SetTolerance(0.01);

    clip.source = passthrough;
    clip.target = m_probe;

    addFilterPipeline(clip, "DataAtPoint");
    setActiveFilterPipeline("DataAtPoint");
}

FemPostDataAtPointFilter::~FemPostDataAtPointFilter() = default;

DocumentObjectExecReturn* FemPostDataAtPointFilter::execute()
{
    // recalculate the filter
    return Fem::FemPostFilter::execute();
}

void FemPostDataAtPointFilter::onChanged(const Property* prop)
{
    if (prop == &Center) {
        const Base::Vector3d& vec = Center.getValue();
        m_point->SetCenter(vec.x, vec.y, vec.z);
    }
    GetPointData();
    Fem::FemPostFilter::onChanged(prop);
}

short int FemPostDataAtPointFilter::mustExecute() const
{
    if (Center.isTouched()) {
        return 1;
    }
    else {
        return App::DocumentObject::mustExecute();
    }
}

void FemPostDataAtPointFilter::GetPointData()
{
    std::vector<double> values;

    vtkSmartPointer<vtkDataObject> data = m_probe->GetOutputDataObject(0);
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
    if (!dset) {
        return;
    }
    vtkDataArray* pdata = dset->GetPointData()->GetArray(FieldName.getValue());
    // VTK cannot deliver data when the filer relies e.g. on a scalar clip filter
    // whose value is set so that all data are clipped
    if (!pdata) {
        return;
    }

    int component = 0;

    for (int i = 0; i < dset->GetNumberOfPoints(); ++i) {

        double value = 0;
        if (pdata->GetNumberOfComponents() == 1) {
            value = pdata->GetComponent(i, component);
        }
        else {
            for (int j = 0; j < pdata->GetNumberOfComponents(); ++j) {
                value += std::pow(pdata->GetComponent(i, j), 2);
            }

            value = std::sqrt(value);
        }
        values.push_back(value);
    }
    PointData.setValues(values);
}


// ***************************************************************************
// clip filter
PROPERTY_SOURCE(Fem::FemPostClipFilter, Fem::FemPostFilter)

FemPostClipFilter::FemPostClipFilter()
    : FemPostFilter()
{
    ADD_PROPERTY_TYPE(Function,
                      (nullptr),
                      "Clip",
                      App::Prop_None,
                      "The function object which defines the clip regions");
    ADD_PROPERTY_TYPE(InsideOut, (false), "Clip", App::Prop_None, "Invert the clip direction");
    ADD_PROPERTY_TYPE(
        CutCells,
        (false),
        "Clip",
        App::Prop_None,
        "Decides if cells are cut and interpolated or if the cells are kept as a whole");

    auto sphere = vtkSmartPointer<vtkSphere>::New();
    sphere->SetRadius(1e12);
    m_defaultFunction = sphere;

    FilterPipeline clip;
    m_clipper = vtkSmartPointer<vtkTableBasedClipDataSet>::New();
    m_clipper->SetClipFunction(m_defaultFunction);
    clip.source = m_clipper;
    clip.target = m_clipper;
    addFilterPipeline(clip, "clip");

    FilterPipeline extr;
    m_extractor = vtkSmartPointer<vtkExtractGeometry>::New();
    m_extractor->SetImplicitFunction(m_defaultFunction);
    extr.source = m_extractor;
    extr.target = m_extractor;
    addFilterPipeline(extr, "extract");

    m_extractor->SetExtractInside(0);
    setActiveFilterPipeline("extract");
}

FemPostClipFilter::~FemPostClipFilter() = default;

void FemPostClipFilter::onChanged(const Property* prop)
{
    if (prop == &Function) {

        if (auto* value = freecad_cast<FemPostFunction*>(Function.getValue())) {
            m_clipper->SetClipFunction(value->getImplicitFunction());
            m_extractor->SetImplicitFunction(value->getImplicitFunction());
        }
        else {
            m_clipper->SetClipFunction(m_defaultFunction);
            m_extractor->SetImplicitFunction(m_defaultFunction);
        }
    }
    else if (prop == &InsideOut) {

        m_clipper->SetInsideOut(InsideOut.getValue());
        m_extractor->SetExtractInside((InsideOut.getValue()) ? 1 : 0);
    }
    else if (prop == &CutCells) {

        if (!CutCells.getValue()) {
            setActiveFilterPipeline("extract");
        }
        else {
            setActiveFilterPipeline("clip");
        }
    };

    Fem::FemPostFilter::onChanged(prop);
}

short int FemPostClipFilter::mustExecute() const
{
    if (Function.isTouched() || InsideOut.isTouched() || CutCells.isTouched()) {

        return 1;
    }
    else {
        return App::DocumentObject::mustExecute();
    }
}

DocumentObjectExecReturn* FemPostClipFilter::execute()
{
    if (!m_extractor->GetImplicitFunction()) {
        return StdReturn;
    }

    return Fem::FemPostFilter::execute();
}

// ***************************************************************************
// smoothing filter extension
const App::PropertyQuantityConstraint::Constraints FemPostSmoothFilterExtension::angleRange = {
    0.0,
    180.0,
    1.0};
const App::PropertyIntegerConstraint::Constraints FemPostSmoothFilterExtension::iterationRange = {
    0,
    VTK_INT_MAX,
    1};
const App::PropertyFloatConstraint::Constraints FemPostSmoothFilterExtension::relaxationRange = {
    0,
    1.0,
    0.01};

EXTENSION_PROPERTY_SOURCE(Fem::FemPostSmoothFilterExtension, App::DocumentObjectExtension)

FemPostSmoothFilterExtension::FemPostSmoothFilterExtension()
{
    EXTENSION_ADD_PROPERTY_TYPE(BoundarySmoothing,
                                (true),
                                "Smoothing",
                                App::Prop_None,
                                "Smooth vertices on the boundary");
    EXTENSION_ADD_PROPERTY_TYPE(EdgeAngle,
                                (15),
                                "Smoothing",
                                App::Prop_None,
                                "Angle to control smoothing along edges");
    EXTENSION_ADD_PROPERTY_TYPE(EnableSmoothing,
                                (false),
                                "Smoothing",
                                App::Prop_None,
                                "Enable Laplacian smoothing");
    EXTENSION_ADD_PROPERTY_TYPE(FeatureAngle,
                                (45),
                                "Smoothing",
                                App::Prop_None,
                                "Angle for sharp edge identification");
    EXTENSION_ADD_PROPERTY_TYPE(EdgeSmoothing,
                                (false),
                                "Smoothing",
                                App::Prop_None,
                                "Smooth align sharp interior edges");
    EXTENSION_ADD_PROPERTY_TYPE(RelaxationFactor,
                                (0.05),
                                "Smoothing",
                                App::Prop_None,
                                "Factor to control vertex displacement");
    EXTENSION_ADD_PROPERTY_TYPE(Iterations,
                                (20),
                                "Smoothing",
                                App::Prop_None,
                                "Number of smoothing iterations");

    EdgeAngle.setConstraints(&angleRange);
    FeatureAngle.setConstraints(&angleRange);
    Iterations.setConstraints(&iterationRange);
    RelaxationFactor.setConstraints(&relaxationRange);

    m_smooth = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
    // override default VTK values
    m_smooth->SetNumberOfIterations(EnableSmoothing.getValue() ? Iterations.getValue() : 0);
    m_smooth->SetBoundarySmoothing(BoundarySmoothing.getValue());
    m_smooth->SetEdgeAngle(EdgeAngle.getValue());
    m_smooth->SetFeatureAngle(FeatureAngle.getValue());
    m_smooth->SetFeatureEdgeSmoothing(EdgeSmoothing.getValue());
    m_smooth->SetRelaxationFactor(RelaxationFactor.getValue());

    initExtensionType(FemPostSmoothFilterExtension::getExtensionClassTypeId());
}

void FemPostSmoothFilterExtension::extensionOnChanged(const App::Property* prop)
{
    if (prop == &EnableSmoothing || prop == &Iterations) {
        // if disabled, set iterations to zero to do nothing
        m_smooth->SetNumberOfIterations(EnableSmoothing.getValue() ? Iterations.getValue() : 0);
    }
    else if (prop == &BoundarySmoothing) {
        m_smooth->SetBoundarySmoothing(static_cast<const App::PropertyBool*>(prop)->getValue());
    }
    else if (prop == &EdgeAngle) {
        m_smooth->SetEdgeAngle(static_cast<const App::PropertyAngle*>(prop)->getValue());
    }
    else if (prop == &FeatureAngle) {
        m_smooth->SetFeatureAngle(static_cast<const App::PropertyAngle*>(prop)->getValue());
    }
    else if (prop == &EdgeSmoothing) {
        m_smooth->SetFeatureEdgeSmoothing(static_cast<const App::PropertyBool*>(prop)->getValue());
    }
    else if (prop == &RelaxationFactor) {
        m_smooth->SetRelaxationFactor(static_cast<const App::PropertyFloat*>(prop)->getValue());
    }
    else {
        DocumentObjectExtension::extensionOnChanged(prop);
    }
}

// ***************************************************************************
// contours filter
PROPERTY_SOURCE(Fem::FemPostContoursFilter, Fem::FemPostFilter)

FemPostContoursFilter::FemPostContoursFilter()
    : FemPostFilter()
{
    ADD_PROPERTY_TYPE(NumberOfContours, (10), "Contours", App::Prop_None, "The number of contours");
    ADD_PROPERTY_TYPE(Field, (long(0)), "Clip", App::Prop_None, "The field used to clip");
    ADD_PROPERTY_TYPE(VectorMode,
                      ((long)0),
                      "Contours",
                      App::Prop_None,
                      "Select what vector field");
    ADD_PROPERTY_TYPE(NoColor,
                      (false),
                      "Contours",
                      PropertyType(Prop_Hidden),
                      "Don't color the contours");

    m_contourConstraints.LowerBound = 1;
    m_contourConstraints.UpperBound = 1000;
    m_contourConstraints.StepSize = 1;
    NumberOfContours.setConstraints(&m_contourConstraints);

    FilterPipeline contours;
    m_contours = vtkSmartPointer<vtkContourFilter>::New();
    m_contours->ComputeScalarsOn();
    smoothExtension.getFilter()->SetInputConnection(m_contours->GetOutputPort());
    contours.source = m_contours;
    contours.target = smoothExtension.getFilter();
    addFilterPipeline(contours, "contours");
    setActiveFilterPipeline("contours");

    smoothExtension.initExtension(this);
}

FemPostContoursFilter::~FemPostContoursFilter() = default;

DocumentObjectExecReturn* FemPostContoursFilter::execute()
{

    // update list of available fields and their vectors
    if (!m_blockPropertyChanges) {
        refreshFields();
        refreshVectors();
    }

    // recalculate the filter
    return Fem::FemPostFilter::execute();
}

void FemPostContoursFilter::onChanged(const Property* prop)
{
    if (m_blockPropertyChanges) {
        return;
    }

    if (prop == &Field && (Field.isValid())) {
        refreshVectors();
    }

    // note that we need to calculate also in case of a Data change
    // otherwise the contours output would be empty and the ViewProviderFemPostObject
    // would not get any data
    if ((prop == &Field || prop == &VectorMode || prop == &NumberOfContours || prop == &Data)
        && (Field.isValid())) {
        double p[2];

        // get the field and its data
        vtkDataSet* dset = getInputData();
        if (!dset) {
            return;
        }
        vtkDataArray* pdata = dset->GetPointData()->GetArray(Field.getValueAsString());
        if (!pdata) {
            return;
        }
        if (pdata->GetNumberOfComponents() == 1) {
            // if we have a scalar, we can directly use the array
            m_contours->SetInputArrayToProcess(0,
                                               0,
                                               0,
                                               vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                               Field.getValueAsString());
            pdata->GetRange(p);
            recalculateContours(p[0], p[1]);
        }
        else {
            // The contour filter handles vectors by taking always its first component.
            // There is no other solution than to make the desired vectorn component a
            // scalar array and append this temporarily to the data. (vtkExtractVectorComponents
            // does not work because our data is an unstructured data set.)
            int component = -1;
            if (VectorMode.getValue() == 1) {
                component = 0;
            }
            else if (VectorMode.getValue() == 2) {
                component = 1;
            }
            else if (VectorMode.getValue() == 3) {
                component = 2;
            }
            // extract the component to a new array
            vtkSmartPointer<vtkDoubleArray> componentArray = vtkSmartPointer<vtkDoubleArray>::New();
            componentArray->SetNumberOfComponents(1);
            vtkIdType numTuples = pdata->GetNumberOfTuples();
            componentArray->SetNumberOfTuples(numTuples);

            if (component >= 0) {
                for (vtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx) {
                    componentArray->SetComponent(tupleIdx,
                                                 0,
                                                 pdata->GetComponent(tupleIdx, component));
                }
            }
            else {
                for (vtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx) {
                    componentArray->SetComponent(
                        tupleIdx,
                        0,
                        std::sqrt(
                            pdata->GetComponent(tupleIdx, 0) * pdata->GetComponent(tupleIdx, 0)
                            + pdata->GetComponent(tupleIdx, 1) * pdata->GetComponent(tupleIdx, 1)
                            + pdata->GetComponent(tupleIdx, 2) * pdata->GetComponent(tupleIdx, 2)));
                }
            }
            // name the array
            contourFieldName = std::string(Field.getValueAsString()) + "_contour";
            componentArray->SetName(contourFieldName.c_str());

            // add the array as new field and use it for the contour filter
            dset->GetPointData()->AddArray(componentArray);
            m_contours->SetInputArrayToProcess(0,
                                               0,
                                               0,
                                               vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                               contourFieldName.c_str());
            componentArray->GetRange(p);
            recalculateContours(p[0], p[1]);
            if (prop == &Data) {
                // we must recalculate to pass the new created contours field
                // to ViewProviderFemPostObject
                m_blockPropertyChanges = true;
                execute();
                m_blockPropertyChanges = false;
            }
        }
    }

    Fem::FemPostFilter::onChanged(prop);
}

short int FemPostContoursFilter::mustExecute() const
{
    if (Field.isTouched() || VectorMode.isTouched() || NumberOfContours.isTouched()
        || Data.isTouched()) {
        return 1;
    }
    else {
        return App::DocumentObject::mustExecute();
    }
}

void FemPostContoursFilter::recalculateContours(double min, double max)
{
    // As the min and max contours are not visible, an input of "3" leads
    // to 1 visible contour. To not confuse the user, take the visible contours
    // for NumberOfContours
    int visibleNum = NumberOfContours.getValue() + 2;
    m_contours->GenerateValues(visibleNum, min, max);
}

void FemPostContoursFilter::refreshFields()
{
    m_blockPropertyChanges = true;

    std::string fieldName;
    if (Field.isValid()) {
        fieldName = Field.getValueAsString();
    }

    std::vector<std::string> FieldsArray;

    vtkDataSet* dset = getInputData();
    if (!dset || !dset->GetPointData()) {
        // no valid data: no fields to choose!
        App::Enumeration empty;
        Field.setValue(empty);
        m_fields.setEnums(FieldsArray);
        Field.setValue(m_fields);
        m_blockPropertyChanges = false;
        return;
    }

    // get all fields
    auto pd = dset->GetPointData();
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        FieldsArray.emplace_back(pd->GetArrayName(i));
    }

    App::Enumeration empty;
    Field.setValue(empty);
    m_fields.setEnums(FieldsArray);
    Field.setValue(m_fields);

    // search if the current field is in the available ones and set it
    // Note: list could be empty and hence Field invalid
    if (Field.isValid()) {
        const auto it = std::ranges::find(FieldsArray, fieldName);
        if (!fieldName.empty() && it != FieldsArray.end()) {
            Field.setValue(fieldName.c_str());
        }
        else {
            m_blockPropertyChanges = false;
            // select the first field
            Field.setValue(long(0));
        }
    }

    m_blockPropertyChanges = false;
}

void FemPostContoursFilter::refreshVectors()
{
    // refreshes the list of available vectors for the current Field
    m_blockPropertyChanges = true;
    std::vector<std::string> vectorArray;

    vtkDataSet* dset = getInputData();
    if (!dset || !dset->GetPointData() || !Field.isValid()) {
        vectorArray.emplace_back("Not a vector");
        App::Enumeration empty;
        VectorMode.setValue(empty);
        m_vectors.setEnums(vectorArray);
        VectorMode.setValue(m_vectors);
        m_blockPropertyChanges = false;
        return;
    }

    vtkDataArray* fieldArray = dset->GetPointData()->GetArray(Field.getValueAsString());
    if (!fieldArray) {
        vectorArray.emplace_back("Not a vector");
        App::Enumeration empty;
        VectorMode.setValue(empty);
        m_vectors.setEnums(vectorArray);
        VectorMode.setValue(m_vectors);
        m_blockPropertyChanges = false;
        return;
    }

    // store name if already set
    std::string vectorName;
    if (VectorMode.isValid()) {
        vectorName = VectorMode.getValueAsString();
    }

    if (fieldArray->GetNumberOfComponents() == 1) {
        vectorArray.emplace_back("Not a vector");
    }
    else {
        vectorArray.emplace_back("Magnitude");
        if (fieldArray->GetNumberOfComponents() >= 2) {
            vectorArray.emplace_back("X");
            vectorArray.emplace_back("Y");
        }
        if (fieldArray->GetNumberOfComponents() >= 3) {
            vectorArray.emplace_back("Z");
        }
    }
    App::Enumeration empty;
    VectorMode.setValue(empty);
    m_vectors.setEnums(vectorArray);
    VectorMode.setValue(m_vectors);

    // apply stored name
    const auto it = std::ranges::find(vectorArray, vectorName);
    if (!vectorName.empty() && it != vectorArray.end()) {
        VectorMode.setValue(vectorName.c_str());
    }

    m_blockPropertyChanges = false;
}


// ***************************************************************************
// cut filter
PROPERTY_SOURCE(Fem::FemPostCutFilter, Fem::FemPostFilter)

FemPostCutFilter::FemPostCutFilter()
    : FemPostFilter()
{
    ADD_PROPERTY_TYPE(Function,
                      (nullptr),
                      "Cut",
                      App::Prop_None,
                      "The function object which defines the cut function");

    auto sphere = vtkSmartPointer<vtkSphere>::New();
    sphere->SetRadius(1e12);
    m_defaultFunction = sphere;


    FilterPipeline cut;
    m_cutter = vtkSmartPointer<vtkCutter>::New();
    m_cutter->SetCutFunction(m_defaultFunction);
    cut.source = m_cutter;
    cut.target = m_cutter;
    addFilterPipeline(cut, "cut");
    setActiveFilterPipeline("cut");
}

FemPostCutFilter::~FemPostCutFilter() = default;

void FemPostCutFilter::onChanged(const Property* prop)
{
    if (prop == &Function) {
        if (auto* value = freecad_cast<FemPostFunction*>(Function.getValue())) {
            m_cutter->SetCutFunction(value->getImplicitFunction());
        }
        else {
            m_cutter->SetCutFunction(m_defaultFunction);
        }
    }

    Fem::FemPostFilter::onChanged(prop);
}

short int FemPostCutFilter::mustExecute() const
{
    if (Function.isTouched()) {
        return 1;
    }
    else {
        return App::DocumentObject::mustExecute();
    }
}

DocumentObjectExecReturn* FemPostCutFilter::execute()
{
    if (!m_cutter->GetCutFunction()) {
        return StdReturn;
    }

    return Fem::FemPostFilter::execute();
}


// ***************************************************************************
// scalar clip filter
PROPERTY_SOURCE(Fem::FemPostScalarClipFilter, Fem::FemPostFilter)

FemPostScalarClipFilter::FemPostScalarClipFilter()
    : FemPostFilter()
{

    ADD_PROPERTY_TYPE(Value,
                      (0),
                      "Clip",
                      App::Prop_None,
                      "The scalar value used to clip the selected field");
    ADD_PROPERTY_TYPE(Scalars, (long(0)), "Clip", App::Prop_None, "The field used to clip");
    ADD_PROPERTY_TYPE(InsideOut, (false), "Clip", App::Prop_None, "Invert the clip direction");

    Value.setConstraints(&m_constraints);

    FilterPipeline clip;
    m_clipper = vtkSmartPointer<vtkTableBasedClipDataSet>::New();
    clip.source = m_clipper;
    clip.target = m_clipper;
    addFilterPipeline(clip, "clip");
    setActiveFilterPipeline("clip");
}

FemPostScalarClipFilter::~FemPostScalarClipFilter() = default;

DocumentObjectExecReturn* FemPostScalarClipFilter::execute()
{
    std::string val = "";
    if (Scalars.isValid()) {
        val = Scalars.getValueAsString();
    }

    std::vector<std::string> ScalarsArray = getInputScalarFields();

    App::Enumeration empty;
    Scalars.setValue(empty);
    m_scalarFields.setEnums(ScalarsArray);
    Scalars.setValue(m_scalarFields);

    // search if the current field is in the available ones and set it
    const auto it = std::ranges::find(ScalarsArray, val);
    if (!val.empty() && it != ScalarsArray.end()) {
        Scalars.setValue(val.c_str());
    }

    // recalculate the filter
    return Fem::FemPostFilter::execute();
}

void FemPostScalarClipFilter::onChanged(const Property* prop)
{
    if (prop == &Value) {
        m_clipper->SetValue(Value.getValue());
    }
    else if (prop == &InsideOut) {
        m_clipper->SetInsideOut(InsideOut.getValue());
    }
    else if (prop == &Scalars && (Scalars.isValid())) {
        m_clipper->SetInputArrayToProcess(0,
                                          0,
                                          0,
                                          vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                          Scalars.getValueAsString());
        setConstraintForField();
    }

    Fem::FemPostFilter::onChanged(prop);
}

short int FemPostScalarClipFilter::mustExecute() const
{
    if (Value.isTouched() || InsideOut.isTouched() || Scalars.isTouched()) {
        return 1;
    }
    else {
        return App::DocumentObject::mustExecute();
    }
}

void FemPostScalarClipFilter::setConstraintForField()
{
    vtkDataSet* dset = getInputData();
    if (!dset || !dset->GetPointData() || !Scalars.isValid()) {
        m_constraints.LowerBound = 0;
        m_constraints.UpperBound = 1;
        m_constraints.StepSize = 1;
        return;
    }

    vtkDataArray* pdata = dset->GetPointData()->GetArray(Scalars.getValueAsString());
    // VTK cannot deliver data when the filer relies e.g. on a cut clip filter
    // whose value is set so that all data are cut
    if (!pdata) {
        m_constraints.LowerBound = 0;
        m_constraints.UpperBound = 1;
        m_constraints.StepSize = 1;
        return;
    }
    double p[2];
    pdata->GetRange(p);
    m_constraints.LowerBound = p[0];
    m_constraints.UpperBound = p[1];
    m_constraints.StepSize = (p[1] - p[0]) / 100.;
}


// ***************************************************************************
// warp vector filter
PROPERTY_SOURCE(Fem::FemPostWarpVectorFilter, Fem::FemPostFilter)

FemPostWarpVectorFilter::FemPostWarpVectorFilter()
    : FemPostFilter()
{
    ADD_PROPERTY_TYPE(Factor,
                      (0),
                      "Warp",
                      App::Prop_None,
                      "The factor by which the vector is added to the node positions");
    ADD_PROPERTY_TYPE(Vector,
                      (long(0)),
                      "Warp",
                      App::Prop_None,
                      "The field added to the node position");

    FilterPipeline warp;
    m_warp = vtkSmartPointer<vtkWarpVector>::New();
    warp.source = m_warp;
    warp.target = m_warp;
    addFilterPipeline(warp, "warp");
    setActiveFilterPipeline("warp");
}

FemPostWarpVectorFilter::~FemPostWarpVectorFilter() = default;

DocumentObjectExecReturn* FemPostWarpVectorFilter::execute()
{

    std::string val;
    if (Vector.isValid()) {
        val = Vector.getValueAsString();
    }

    std::vector<std::string> VectorArray = getInputVectorFields();

    App::Enumeration empty;
    Vector.setValue(empty);
    m_vectorFields.setEnums(VectorArray);
    Vector.setValue(m_vectorFields);

    // search if the current field is in the available ones and set it
    const auto it = std::ranges::find(VectorArray, val);
    if (!val.empty() && it != VectorArray.end()) {
        Vector.setValue(val.c_str());
    }

    // recalculate the filter
    return Fem::FemPostFilter::execute();
}

void FemPostWarpVectorFilter::onChanged(const Property* prop)
{
    if (prop == &Factor) {
        // since our mesh is in mm, we must scale the factor
        m_warp->SetScaleFactor(1000 * Factor.getValue());
    }
    else if (prop == &Vector && Vector.isValid()) {
        m_warp->SetInputArrayToProcess(0,
                                       0,
                                       0,
                                       vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                       Vector.getValueAsString());
    }

    Fem::FemPostFilter::onChanged(prop);
}

short int FemPostWarpVectorFilter::mustExecute() const
{
    if (Factor.isTouched() || Vector.isTouched()) {
        return 1;
    }
    else {
        return App::DocumentObject::mustExecute();
    }
}


// ***************************************************************************
// calculator filter
PROPERTY_SOURCE(Fem::FemPostCalculatorFilter, Fem::FemPostFilter)

FemPostCalculatorFilter::FemPostCalculatorFilter()
    : FemPostFilter()
{
    ADD_PROPERTY_TYPE(FieldName,
                      ("Calculator"),
                      "Calculator",
                      App::Prop_None,
                      "Name of the calculated field");
    ADD_PROPERTY_TYPE(Function,
                      (""),
                      "Calculator",
                      App::Prop_None,
                      "Expression of the unction to evaluate");
    ADD_PROPERTY_TYPE(ReplacementValue,
                      (0.0f),
                      "Calculator",
                      App::Prop_None,
                      "Value used to replace invalid operations");
    ADD_PROPERTY_TYPE(ReplaceInvalid,
                      (false),
                      "Calculator",
                      App::Prop_None,
                      "Replace invalid values");

    FilterPipeline calculator;
    m_calculator = vtkSmartPointer<vtkArrayCalculator>::New();
    m_calculator->SetResultArrayName(FieldName.getValue());
    calculator.source = m_calculator;
    calculator.target = m_calculator;
    addFilterPipeline(calculator, "calculator");
    setActiveFilterPipeline("calculator");
}

FemPostCalculatorFilter::~FemPostCalculatorFilter() = default;

DocumentObjectExecReturn* FemPostCalculatorFilter::execute()
{
    updateAvailableFields();

    return FemPostFilter::execute();
}

void FemPostCalculatorFilter::onChanged(const Property* prop)
{
    if (prop == &Function) {
        m_calculator->SetFunction(Function.getValue());
    }
    else if (prop == &FieldName) {
        m_calculator->SetResultArrayName(FieldName.getValue());
    }
    else if (prop == &ReplaceInvalid) {
        m_calculator->SetReplaceInvalidValues(ReplaceInvalid.getValue());
    }
    else if (prop == &ReplacementValue) {
        m_calculator->SetReplacementValue(ReplacementValue.getValue());
    }
    else if (prop == &Data) {
        updateAvailableFields();
    }
    Fem::FemPostFilter::onChanged(prop);
}

short int FemPostCalculatorFilter::mustExecute() const
{
    if (Function.isTouched() || FieldName.isTouched()) {
        return 1;
    }
    else {
        return FemPostFilter::mustExecute();
    }
}

void FemPostCalculatorFilter::updateAvailableFields()
{
    // clear all variables
    m_calculator->RemoveAllVariables();
    m_calculator->AddCoordinateScalarVariable("coordsX", 0);
    m_calculator->AddCoordinateScalarVariable("coordsY", 1);
    m_calculator->AddCoordinateScalarVariable("coordsZ", 2);
    m_calculator->AddCoordinateVectorVariable("coords");

    std::vector<std::string> scalars;
    std::vector<std::string> vectors;

    vtkSmartPointer<vtkDataObject> data = getInputData();
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
    if (!dset) {
        return;
    }
    vtkPointData* pd = dset->GetPointData();

    // get all vector fields
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        std::string name1 = pd->GetArrayName(i);
        std::string name2 = name1;
        std::replace(name2.begin(), name2.end(), ' ', '_');
        if (pd->GetArray(i)->GetNumberOfComponents() == 3) {
            m_calculator->AddVectorVariable(name2.c_str(), name1.c_str());
            // add components as scalar variable
            m_calculator->AddScalarVariable((name2 + "_X").c_str(), name1.c_str(), 0);
            m_calculator->AddScalarVariable((name2 + "_Y").c_str(), name1.c_str(), 1);
            m_calculator->AddScalarVariable((name2 + "_Z").c_str(), name1.c_str(), 2);
        }
        else if (pd->GetArray(i)->GetNumberOfComponents() == 1) {
            m_calculator->AddScalarVariable(name2.c_str(), name1.c_str());
        }
    }
}

const std::vector<std::string> FemPostCalculatorFilter::getScalarVariables()
{
#if (VTK_MAJOR_VERSION >= 9) && (VTK_MINOR_VERSION > 0)
    std::vector<std::string> scalars = m_calculator->GetScalarVariableNames();
#else
    std::vector<std::string> scalars(m_calculator->GetScalarVariableNames(),
                                     m_calculator->GetScalarVariableNames()
                                         + m_calculator->GetNumberOfScalarArrays());
#endif

    scalars.insert(scalars.begin(), {"coordsX", "coordsY", "coordsZ"});
    return scalars;
}

const std::vector<std::string> FemPostCalculatorFilter::getVectorVariables()
{
#if (VTK_MAJOR_VERSION >= 9) && (VTK_MINOR_VERSION > 0)
    std::vector<std::string> vectors = m_calculator->GetVectorVariableNames();
#else
    std::vector<std::string> vectors(m_calculator->GetVectorVariableNames(),
                                     m_calculator->GetVectorVariableNames()
                                         + m_calculator->GetNumberOfVectorArrays());
#endif

    vectors.insert(vectors.begin(), "coords");
    return vectors;
}
