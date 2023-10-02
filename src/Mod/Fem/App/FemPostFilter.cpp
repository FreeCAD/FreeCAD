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
#endif

#include <App/Document.h>

#include "FemPostFilter.h"
#include "FemPostPipeline.h"


using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostFilter, Fem::FemPostObject)


FemPostFilter::FemPostFilter()
{
    ADD_PROPERTY(Input, (nullptr));
}

FemPostFilter::~FemPostFilter() = default;

void FemPostFilter::addFilterPipeline(const FemPostFilter::FilterPipeline& p, std::string name)
{
    m_pipelines[name] = p;
}

FemPostFilter::FilterPipeline& FemPostFilter::getFilterPipeline(std::string name)
{
    return m_pipelines[name];
}

void FemPostFilter::setActiveFilterPipeline(std::string name)
{
    if (m_activePipeline != name && isValid()) {
        m_activePipeline = name;
    }
}

DocumentObjectExecReturn* FemPostFilter::execute()
{
    if (!m_pipelines.empty() && !m_activePipeline.empty()) {
        FemPostFilter::FilterPipeline& pipe = m_pipelines[m_activePipeline];
        vtkSmartPointer<vtkDataObject> data = getInputData();
        if (!data || !data->IsA("vtkDataSet")) {
            return StdReturn;
        }

        if ((m_activePipeline == "DataAlongLine") || (m_activePipeline == "DataAtPoint")) {
            pipe.filterSource->SetSourceData(getInputData());
            pipe.filterTarget->Update();
            Data.setValue(pipe.filterTarget->GetOutputDataObject(0));
        }
        else {
            pipe.source->SetInputDataObject(data);
            pipe.target->Update();
            Data.setValue(pipe.target->GetOutputDataObject(0));
        }
    }

    return StdReturn;
}

vtkDataObject* FemPostFilter::getInputData()
{
    if (Input.getValue()) {
        if (Input.getValue()->getTypeId().isDerivedFrom(
                Base::Type::fromName("Fem::FemPostObject"))) {
            return Input.getValue<FemPostObject*>()->Data.getValue();
        }
        else {
            throw std::runtime_error(
                "The filter's Input object is not a 'Fem::FemPostObject' object!");
        }
    }
    else {
        // get the pipeline and use the pipelinedata
        std::vector<App::DocumentObject*> objs =
            getDocument()->getObjectsOfType(FemPostPipeline::getClassTypeId());
        for (auto it : objs) {
            if (static_cast<FemPostPipeline*>(it)->holdsPostObject(this)) {
                return static_cast<FemPostObject*>(it)->Data.getValue();
            }
        }
    }

    return nullptr;
}

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

    PlotData.setStatus(App::Property::ReadOnly, true);
    XAxisData.setStatus(App::Property::Output, true);
    YAxisData.setStatus(App::Property::Output, true);

    FilterPipeline clip;

    m_line = vtkSmartPointer<vtkLineSource>::New();
    const Base::Vector3d& vec1 = Point1.getValue();
    m_line->SetPoint1(vec1.x, vec1.y, vec1.z);
    const Base::Vector3d& vec2 = Point2.getValue();
    m_line->SetPoint2(vec2.x, vec2.y, vec2.z);
    m_line->SetResolution(Resolution.getValue());


    m_probe = vtkSmartPointer<vtkProbeFilter>::New();
    m_probe->SetInputConnection(m_line->GetOutputPort());
    m_probe->SetValidPointMaskArrayName("ValidPointArray");
    m_probe->SetPassPointArrays(1);
    m_probe->SetPassCellArrays(1);
    // needs vtk > 6.1
#if (VTK_MAJOR_VERSION > 6) && (VTK_MINOR_VERSION > 1)
    m_probe->ComputeToleranceOff();
    m_probe->SetTolerance(0.01);
#endif

    clip.filterSource = m_probe;
    clip.filterTarget = m_probe;

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
    vtkDataArray* tcoords = dset->GetPointData()->GetTCoords("Texture Coordinates");

    vtkIdType component = 0;

    const Base::Vector3d& vec1 = Point1.getValue();
    const Base::Vector3d& vec2 = Point2.getValue();
    const Base::Vector3d diff = vec1 - vec2;
    double Len = diff.Length();

    for (vtkIdType i = 0; i < dset->GetNumberOfPoints(); ++i) {

        double value = 0;
        if (pdata) {
            if (pdata->GetNumberOfComponents() == 1) {
                value = pdata->GetComponent(i, component);
            }
            else {
                for (vtkIdType j = 0; j < pdata->GetNumberOfComponents(); ++j) {
                    value += std::pow(pdata->GetComponent(i, j), 2);
                }

                value = std::sqrt(value);
            }
        }

        values.push_back(value);
        double tcoord = tcoords->GetComponent(i, component);
        coords.push_back(tcoord * Len);
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
    ADD_PROPERTY_TYPE(Radius,
                      (0),
                      "DataAtPoint",
                      App::Prop_None,
                      "Radius around the point (unused)");
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

    m_probe = vtkSmartPointer<vtkProbeFilter>::New();
    m_probe->SetInputConnection(m_point->GetOutputPort());
    m_probe->SetValidPointMaskArrayName("ValidPointArray");
    m_probe->SetPassPointArrays(1);
    m_probe->SetPassCellArrays(1);
    // needs vtk > 6.1
#if (VTK_MAJOR_VERSION > 6) && (VTK_MINOR_VERSION > 1)
    m_probe->ComputeToleranceOff();
    m_probe->SetTolerance(0.01);
#endif

    clip.filterSource = m_probe;
    clip.filterTarget = m_probe;

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

    FilterPipeline clip;
    m_clipper = vtkSmartPointer<vtkTableBasedClipDataSet>::New();
    clip.source = m_clipper;
    clip.target = m_clipper;
    addFilterPipeline(clip, "clip");

    FilterPipeline extr;
    m_extractor = vtkSmartPointer<vtkExtractGeometry>::New();
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

        if (Function.getValue()
            && Function.getValue()->isDerivedFrom(FemPostFunction::getClassTypeId())) {
            m_clipper->SetClipFunction(
                static_cast<FemPostFunction*>(Function.getValue())->getImplicitFunction());
            m_extractor->SetImplicitFunction(
                static_cast<FemPostFunction*>(Function.getValue())->getImplicitFunction());
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
    contours.source = m_contours;
    contours.target = m_contours;
    addFilterPipeline(contours, "contours");
    setActiveFilterPipeline("contours");
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
    auto returnObject = Fem::FemPostFilter::execute();

    // delete contour field
    vtkSmartPointer<vtkDataObject> data = getInputData();
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
    if (!dset) {
        return returnObject;
    }
    dset->GetPointData()->RemoveArray(contourFieldName.c_str());
    // refresh fields to reflect the deletion
    if (!m_blockPropertyChanges) {
        refreshFields();
    }

    return returnObject;
}

void FemPostContoursFilter::onChanged(const Property* prop)
{
    if (m_blockPropertyChanges) {
        return;
    }

    if (prop == &Field && (Field.getValue() >= 0)) {
        refreshVectors();
    }

    // note that we need to calculate also in case of a Data change
    // otherwise the contours output would be empty and the ViewProviderFemPostObject
    // would not get any data
    if ((prop == &Field || prop == &VectorMode || prop == &NumberOfContours || prop == &Data)
        && (Field.getValue() >= 0)) {
        double p[2];

        // get the field and its data
        vtkSmartPointer<vtkDataObject> data = getInputData();
        vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
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
    if (Field.getValue() >= 0) {
        fieldName = Field.getValueAsString();
    }

    std::vector<std::string> FieldsArray;

    vtkSmartPointer<vtkDataObject> data = getInputData();
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
    if (!dset) {
        m_blockPropertyChanges = false;
        return;
    }
    vtkPointData* pd = dset->GetPointData();

    // get all fields
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        FieldsArray.emplace_back(pd->GetArrayName(i));
    }

    App::Enumeration empty;
    Field.setValue(empty);
    m_fields.setEnums(FieldsArray);
    Field.setValue(m_fields);

    // search if the current field is in the available ones and set it
    std::vector<std::string>::iterator it =
        std::find(FieldsArray.begin(), FieldsArray.end(), fieldName);
    if (!fieldName.empty() && it != FieldsArray.end()) {
        Field.setValue(fieldName.c_str());
    }
    else {
        m_blockPropertyChanges = false;
        // select the first field
        Field.setValue(long(0));
        fieldName = Field.getValueAsString();
    }

    m_blockPropertyChanges = false;
}

void FemPostContoursFilter::refreshVectors()
{
    // refreshes the list of available vectors for the current Field
    m_blockPropertyChanges = true;

    vtkSmartPointer<vtkDataObject> data = getInputData();
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
    if (!dset) {
        m_blockPropertyChanges = false;
        return;
    }
    vtkDataArray* fieldArray = dset->GetPointData()->GetArray(Field.getValueAsString());
    if (!fieldArray) {
        m_blockPropertyChanges = false;
        return;
    }

    // store name if already set
    std::string vectorName;
    if (VectorMode.hasEnums() && VectorMode.getValue() >= 0) {
        vectorName = VectorMode.getValueAsString();
    }

    std::vector<std::string> vectorArray;
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
    auto it = std::find(vectorArray.begin(), vectorArray.end(), vectorName);
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

    FilterPipeline cut;
    m_cutter = vtkSmartPointer<vtkCutter>::New();
    cut.source = m_cutter;
    cut.target = m_cutter;
    addFilterPipeline(cut, "cut");
    setActiveFilterPipeline("cut");
}

FemPostCutFilter::~FemPostCutFilter() = default;

void FemPostCutFilter::onChanged(const Property* prop)
{
    if (prop == &Function) {
        if (Function.getValue()
            && Function.getValue()->isDerivedFrom(FemPostFunction::getClassTypeId())) {
            m_cutter->SetCutFunction(
                static_cast<FemPostFunction*>(Function.getValue())->getImplicitFunction());
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
    std::string val;
    if (Scalars.getValue() >= 0) {
        val = Scalars.getValueAsString();
    }

    std::vector<std::string> ScalarsArray;

    vtkSmartPointer<vtkDataObject> data = getInputData();
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
    if (!dset) {
        return StdReturn;
    }
    vtkPointData* pd = dset->GetPointData();

    // get all scalar fields
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        if (pd->GetArray(i)->GetNumberOfComponents() == 1) {
            ScalarsArray.emplace_back(pd->GetArrayName(i));
        }
    }

    App::Enumeration empty;
    Scalars.setValue(empty);
    m_scalarFields.setEnums(ScalarsArray);
    Scalars.setValue(m_scalarFields);

    // search if the current field is in the available ones and set it
    std::vector<std::string>::iterator it =
        std::find(ScalarsArray.begin(), ScalarsArray.end(), val);
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
    else if (prop == &Scalars && (Scalars.getValue() >= 0)) {
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
    vtkSmartPointer<vtkDataObject> data = getInputData();
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
    if (!dset) {
        return;
    }

    vtkDataArray* pdata = dset->GetPointData()->GetArray(Scalars.getValueAsString());
    // VTK cannot deliver data when the filer relies e.g. on a cut clip filter
    // whose value is set so that all data are cut
    if (!pdata) {
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
    if (Vector.getValue() >= 0) {
        val = Vector.getValueAsString();
    }

    std::vector<std::string> VectorArray;

    vtkSmartPointer<vtkDataObject> data = getInputData();
    vtkDataSet* dset = vtkDataSet::SafeDownCast(data);
    if (!dset) {
        return StdReturn;
    }
    vtkPointData* pd = dset->GetPointData();

    // get all vector fields
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        if (pd->GetArray(i)->GetNumberOfComponents() == 3) {
            VectorArray.emplace_back(pd->GetArrayName(i));
        }
    }

    App::Enumeration empty;
    Vector.setValue(empty);
    m_vectorFields.setEnums(VectorArray);
    Vector.setValue(m_vectorFields);

    // search if the current field is in the available ones and set it
    std::vector<std::string>::iterator it = std::find(VectorArray.begin(), VectorArray.end(), val);
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
    else if (prop == &Vector && (Vector.getValue() >= 0)) {
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
