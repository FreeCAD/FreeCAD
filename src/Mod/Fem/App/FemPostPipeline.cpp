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

#include <cmath>

#include <Python.h>
#include <vtkAppendFilter.h>
#include <vtkDataSetReader.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLPUnstructuredGridReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLMultiBlockDataReader.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkFloatArray.h>
#include <vtkStringArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>

#include <Base/Console.h>
#include <Base/Reader.h>

#include "FemMesh.h"
#include "FemMeshObject.h"
#include "FemPostFilter.h"
#include "FemPostPipeline.h"
#include "FemPostPipelinePy.h"
#include "FemVTKTools.h"


using namespace Fem;
using namespace App;


PROPERTY_SOURCE_WITH_EXTENSIONS(Fem::FemPostPipeline, Fem::FemPostObject)

FemPostPipeline::FemPostPipeline()
{

    FemPostGroupExtension::initExtension(this);

    ADD_PROPERTY_TYPE(
        Frame,
        (long(0)),
        "Pipeline",
        App::Prop_None,
        "The frame used to calculate the data in the pipeline processing (read only, "
        "set via pipeline object)."
    );
    ADD_PROPERTY_TYPE(MergeDuplicate, (false), "Pipeline", App::Prop_None, "Remove coindent elements.");

    // create our source algorithm
    m_source_algorithm = vtkSmartPointer<vtkFemFrameSourceAlgorithm>::New();
    m_clean_filter = vtkSmartPointer<vtkCleanUnstructuredGrid>::New();

    m_clean_filter->SetPointDataWeighingStrategy(vtkCleanUnstructuredGrid::AVERAGING);
    m_transform_filter->SetInputConnection(m_source_algorithm->GetOutputPort(0));
}

vtkDataSet* FemPostPipeline::getDataSet()
{
    if (!m_source_algorithm->isValid()) {
        return nullptr;
    }

    vtkDataObject* data = m_transform_filter->GetOutputDataObject(0);
    if (!data) {
        return nullptr;
    }

    if (data->IsA("vtkDataSet")) {
        return vtkDataSet::SafeDownCast(data);
    }

    return nullptr;
}

Fem::FemPostFunctionProvider* FemPostPipeline::getFunctionProvider()
{

    // see if we have one
    for (auto obj : Group.getValues()) {
        if (obj->isDerivedFrom<FemPostFunctionProvider>()) {
            return static_cast<FemPostFunctionProvider*>(obj);
        }
    }
    return nullptr;
}

bool FemPostPipeline::allowObject(App::DocumentObject* obj)
{
    // we additionally allow FunctionPRoviders to be added
    if (obj->isDerivedFrom<FemPostFunctionProvider>()) {
        return true;
    }

    // and all standard Post objects the group can handle
    return FemPostGroupExtension::allowObject(obj);
}


bool FemPostPipeline::canRead(Base::FileInfo File)
{

    // from FemResult only unstructural mesh is supported in femvtktoools.cpp
    return File.hasExtension({"vtk", "vtp", "vts", "vtr", "vti", "vtu", "pvtu", "vtm", "pvd"});
}

vtkSmartPointer<vtkDataObject> FemPostPipeline::dataObjectFromFile(const Base::FileInfo& File)
{
    // checking on the file
    if (!File.isReadable()) {
        throw Base::FileException("File to load not existing or not readable", File);
    }

    if (File.hasExtension("vtu")) {
        return readXMLFile<vtkXMLUnstructuredGridReader>(File.filePath());
    }
    else if (File.hasExtension("pvtu")) {
        return readXMLFile<vtkXMLPUnstructuredGridReader>(File.filePath());
    }
    else if (File.hasExtension("vtp")) {
        return readXMLFile<vtkXMLPolyDataReader>(File.filePath());
    }
    else if (File.hasExtension("vts")) {
        return readXMLFile<vtkXMLStructuredGridReader>(File.filePath());
    }
    else if (File.hasExtension("vtr")) {
        return readXMLFile<vtkXMLRectilinearGridReader>(File.filePath());
    }
    else if (File.hasExtension("vti")) {
        return readXMLFile<vtkXMLImageDataReader>(File.filePath());
    }
    else if (File.hasExtension("vtk")) {
        return readXMLFile<vtkDataSetReader>(File.filePath());
    }
    else if (File.hasExtension("vtm")) {
        return readXMLFile<vtkXMLMultiBlockDataReader>(File.filePath());
    }
    else if (File.hasExtension("pvd")) {
        return readPVD(File);
    }

    throw Base::FileException("Unknown extension");
}

vtkSmartPointer<vtkDataObject> FemPostPipeline::readPVD(const Base::FileInfo& file)
{
    std::string path = file.filePath();

    std::ifstream ifstr(path, std::ios::in | std::ios::binary);
    Base::XMLReader reader(path.c_str(), ifstr);
    reader.readElement("DataSet");
    std::map<double, std::string> values;
    std::vector<std::string> files;
    while (strcmp(reader.localName(), "DataSet") == 0) {
        values.emplace(
            std::make_pair(
                reader.getAttribute<double>("timestep"),
                reader.getAttribute<std::string>("file")
            )
        );
        reader.readNextElement();
    }

    auto timeInfo = vtkSmartPointer<vtkStringArray>::New();
    timeInfo->SetName("TimeInfo");
    timeInfo->InsertNextValue("TimeStep");
    // set unit to empty string
    timeInfo->InsertNextValue("");

    auto multiBlock = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    multiBlock->GetFieldData()->AddArray(timeInfo);

    int i = 0;
    std::string dir = file.dirPath();
    for (auto v : values) {
        Base::FileInfo fi(dir + "/" + v.second);
        auto data = dataObjectFromFile(fi);
        auto time = vtkSmartPointer<vtkFloatArray>::New();
        time->SetName("TimeValue");
        time->InsertNextValue(v.first);
        data->GetFieldData()->AddArray(time);
        data->GetFieldData()->AddArray(timeInfo);

        multiBlock->SetBlock(i, data);
        ++i;
    }

    return multiBlock;
}

void FemPostPipeline::read(Base::FileInfo File)
{
    Data.setValue(dataObjectFromFile(File));
}

void FemPostPipeline::read(
    std::vector<Base::FileInfo>& files,
    std::vector<double>& values,
    Base::Unit unit,
    std::string& frame_type
)
{
    if (files.size() != values.size()) {
        throw Base::ValueError("Result files and frame values have different length");
    }

    // make sure we do not have invalid values
    for (auto& value : values) {
        if (!std::isfinite(value)) {
            throw Base::ValueError("Values need to be finite");
        }
    }

    // ensure no double values for frames
    std::set<double> value_set(values.begin(), values.end());
    if (value_set.size() != values.size()) {
        throw Base::ValueError("Values need to be unique");
    }


    // setup the time information for the multiblock
    vtkStringArray* TimeInfo = vtkStringArray::New();
    TimeInfo->SetName("TimeInfo");
    TimeInfo->InsertNextValue(frame_type);
    TimeInfo->InsertNextValue(unit.getString());

    auto multiblock = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    for (ulong i = 0; i < files.size(); i++) {


        // add time information
        vtkFloatArray* TimeValue = vtkFloatArray::New();
        TimeValue->SetNumberOfComponents(1);
        TimeValue->SetName("TimeValue");
        TimeValue->InsertNextValue(values[i]);

        // checking on the file
        auto File = files[i];
        if (!File.isReadable()) {
            throw Base::FileException("File to load not existing or not readable", File);
        }

        auto data = dataObjectFromFile(File);
        data->GetFieldData()->AddArray(TimeValue);
        data->GetFieldData()->AddArray(TimeInfo);

        multiblock->SetBlock(i, data);
    }

    multiblock->GetFieldData()->AddArray(TimeInfo);
    Data.setValue(multiblock);
}

void FemPostPipeline::scale(double s)
{
    Data.scale(s);
    onChanged(&Data);
}

App::DocumentObjectExecReturn* FemPostPipeline::execute()
{
    // we fake a recalculated data object, so that the viewprovider updates
    // the visualization. We do not want to do this in onChange, as it
    // could theoretically be long running
    if (m_data_updated) {

        auto frames = getFrameValues();
        if (!frames.empty() && Frame.getValue() < long(frames.size())) {

            double time = frames[Frame.getValue()];
            m_transform_filter->UpdateTimeStep(time);
        }
        else {
            m_transform_filter->Update();
        }

        m_block_property = true;
        FemPostObject::onChanged(&Data);
        m_block_property = false;
        m_data_updated = false;
    }
    return FemPostObject::execute();
}

void FemPostPipeline::updateData()
{
    m_data_updated = true;
}

void FemPostPipeline::onChanged(const Property* prop)
{
    /* onChanged handles the Pipeline setup: we connect the inputs and outputs
     * of our child filters correctly according to the new settings
     */

    FemPostObject::onChanged(prop);

    // update placement
    if (prop == &Placement) {
        // pipeline data updated!
        updateData();
        recomputeChildren();
    }

    if (prop == &MergeDuplicate) {
        if (MergeDuplicate.getValue()) {
            m_clean_filter->SetInputConnection(m_source_algorithm->GetOutputPort(0));
            m_transform_filter->SetInputConnection(m_clean_filter->GetOutputPort(0));
        }
        else {
            m_transform_filter->SetInputConnection(m_source_algorithm->GetOutputPort(0));
        }
        m_transform_filter->Update();
        updateData();
        recomputeChildren();
    }

    // use the correct data as source
    if (prop == &Data && !m_block_property) {
        m_source_algorithm->setDataObject(Data.getValue());
        m_transform_filter->Update();

        // change the frame enum to correct values
        std::string val;
        if (Frame.hasEnums() && Frame.getValue() >= 0) {
            val = Frame.getValueAsString();
        }

        std::vector<double> frames = m_source_algorithm->getFrameValues();
        std::vector<std::string> frame_values;
        if (frames.empty()) {
            frame_values.push_back("No frames available");
        }
        else {
            auto unit = getFrameUnit();
            for (const double& frame : frames) {
                auto quantity = Base::Quantity(frame, unit);
                frame_values.push_back(quantity.getUserString());
            }
        }

        App::Enumeration empty;
        m_block_property = true;
        Frame.setValue(empty);
        m_frameEnum.setEnums(frame_values);
        Frame.setValue(m_frameEnum);
        Frame.purgeTouched();
        m_block_property = false;

        std::vector<std::string>::iterator it
            = std::find(frame_values.begin(), frame_values.end(), val);
        if (!val.empty() && it != frame_values.end()) {
            // frame stays the same
            m_block_property = true;
            Frame.setValue(val.c_str());
            m_block_property = false;
        }
        else {
            // frame gets updated
            Frame.setValue(long(0));
        }

        updateData();
        recomputeChildren();
    }

    if (prop == &Frame && !m_block_property) {

        // Update all children with the new frame
        double value = 0;
        auto frames = m_source_algorithm->getFrameValues();
        if (!frames.empty() && frames.size() > ulong(Frame.getValue())) {
            value = frames[Frame.getValue()];
        }
        for (const auto& obj : Group.getValues()) {
            if (auto* postFilter = freecad_cast<FemPostFilter*>(obj)) {
                postFilter->Frame.setValue(value);
            }
        }
        // pipeline data updated!
        updateData();
        recomputeChildren();
    }


    // connect all filters correctly to the source
    if (prop == &Group || prop == &Mode) {

        // we check if all connections are right and add new ones if needed
        std::vector<FemPostFilter*> objs = getFilter();

        if (objs.empty()) {
            return;
        }

        FemPostFilter* filter = nullptr;
        for (auto& obj : objs) {

            // prepare the filter: make all connections new
            FemPostFilter* nextFilter = obj;
            nextFilter->getFilterInput()->RemoveAllInputConnections(0);

            // handle input modes (Parallel is separated, all other settings are serial, just in
            // case an old document is loaded with "custom" mode, idx 2)
            if (Mode.getValue() == Fem::PostGroupMode::Parallel) {
                // parallel: all filters get out input
                nextFilter->getFilterInput()->SetInputConnection(m_transform_filter->GetOutputPort(0));
            }
            else {
                // serial: the next filter gets the previous output, the first one gets our input
                if (!filter) {
                    nextFilter->getFilterInput()->SetInputConnection(
                        m_transform_filter->GetOutputPort(0)
                    );
                }
                else {
                    nextFilter->getFilterInput()->SetInputConnection(
                        filter->getFilterOutput()->GetOutputPort()
                    );
                }
            }

            filter = nextFilter;
        };

        // inform the downstream pipeline
        recomputeChildren();
    }
}

void FemPostPipeline::filterChanged(FemPostFilter* filter)
{
    // we only need to update the following children if we are in serial mode
    if (Mode.getValue() == Fem::PostGroupMode::Serial) {

        std::vector<App::DocumentObject*> objs = Group.getValues();

        if (objs.empty()) {
            return;
        }
        bool started = false;
        for (auto& obj : objs) {

            if (started) {
                obj->touch();
                if (obj->hasExtension(Fem::FemPostGroupExtension::getExtensionClassTypeId())) {
                    obj->getExtension<FemPostGroupExtension>()->recomputeChildren();
                }
            }

            if (obj == filter) {
                started = true;
            }
        }
    }
}

void FemPostPipeline::filterPipelineChanged(FemPostFilter*)
{
    // one of our filters has changed its active pipeline. We need to reconnect it properly.
    // As we are cheap we just reconnect everything
    // TODO: Do more efficiently
    onChanged(&Group);
}

bool FemPostPipeline::hasFrames()
{
    // lazy implementation
    return !m_source_algorithm->getFrameValues().empty();
}

std::string FemPostPipeline::getFrameType()
{
    vtkSmartPointer<vtkDataObject> data = Data.getValue();

    // check if we have frame data
    if (!data || !data->IsA("vtkMultiBlockDataSet")) {
        return std::string("no frames");
    }

    // we have multiple frames! let's check the amount and times
    vtkSmartPointer<vtkMultiBlockDataSet> multiblock = vtkMultiBlockDataSet::SafeDownCast(data);
    if (!multiblock->GetFieldData()->HasArray("TimeInfo")) {
        return std::string("unknown");
    }

    vtkAbstractArray* TimeInfo = multiblock->GetFieldData()->GetAbstractArray("TimeInfo");
    if (!TimeInfo || !TimeInfo->IsA("vtkStringArray") || TimeInfo->GetNumberOfTuples() < 2) {

        return std::string("unknown");
    }

    return vtkStringArray::SafeDownCast(TimeInfo)->GetValue(0);
}

Base::Unit FemPostPipeline::getFrameUnit()
{
    vtkSmartPointer<vtkDataObject> data = Data.getValue();

    // check if we have frame data
    if (!data || !data->IsA("vtkMultiBlockDataSet")) {
        // units cannot be undefined, so use time
        return Base::Unit::TimeSpan;
    }

    // we have multiple frames! let's check the amount and times
    vtkSmartPointer<vtkMultiBlockDataSet> multiblock = vtkMultiBlockDataSet::SafeDownCast(data);
    if (!multiblock->GetFieldData()->HasArray("TimeInfo")) {
        // units cannot be undefined, so use time
        return Base::Unit::TimeSpan;
    }

    vtkAbstractArray* TimeInfo = multiblock->GetFieldData()->GetAbstractArray("TimeInfo");
    if (!TimeInfo->IsA("vtkStringArray") || TimeInfo->GetNumberOfTuples() < 2) {
        // units cannot be undefined, so use time
        return Base::Unit::TimeSpan;
    }
    auto qty = Base::Quantity(0, vtkStringArray::SafeDownCast(TimeInfo)->GetValue(1));
    return qty.getUnit();
}

std::vector<double> FemPostPipeline::getFrameValues()
{
    return m_source_algorithm->getFrameValues();
}

unsigned int FemPostPipeline::getFrameNumber()
{
    // lazy implementation
    return getFrameValues().size();
}

void FemPostPipeline::load(FemResultObject* res)
{
    if (!res->Mesh.getValue()) {
        Base::Console().log("Result mesh object is empty.\n");
        return;
    }
    if (!res->Mesh.getValue()->isDerivedFrom<Fem::FemMeshObject>()) {
        Base::Console().log("Result mesh object is not derived from Fem::FemMeshObject.\n");
        return;
    }

    // first copy the mesh over
    // ***************************
    const FemMesh& mesh = static_cast<FemMeshObject*>(res->Mesh.getValue())->FemMesh.getValue();
    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    FemVTKTools::exportVTKMesh(&mesh, grid);

    // Now copy the point data over
    // ***************************
    FemVTKTools::exportFreeCADResult(res, grid);

    Data.setValue(grid);
}

// set multiple result objects as frames for one pipeline
// Notes:
//      1. values vector must contain growing value, smallest first
void FemPostPipeline::load(
    std::vector<FemResultObject*>& res,
    std::vector<double>& values,
    Base::Unit unit,
    std::string& frame_type
)
{

    if (res.size() != values.size()) {
        throw Base::ValueError("Result values and frame values have different length");
    }

    // make sure we do not have invalid values
    for (auto& value : values) {
        if (!std::isfinite(value)) {
            throw Base::ValueError("Values need to be finite");
        }
    }

    // ensure no double values for frames
    std::set<double> value_set(values.begin(), values.end());
    if (value_set.size() != values.size()) {
        throw Base::ValueError("Values need to be unique");
    }

    // setup the time information for the multiblock
    vtkStringArray* TimeInfo = vtkStringArray::New();
    TimeInfo->SetName("TimeInfo");
    TimeInfo->InsertNextValue(frame_type);
    TimeInfo->InsertNextValue(unit.getString());

    auto multiblock = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    for (ulong i = 0; i < res.size(); i++) {

        if (!res[i]->Mesh.getValue()->isDerivedFrom<FemMeshObject>()) {
            throw Base::ValueError("Result mesh object is not derived from Fem::FemMeshObject");
        }

        // first copy the mesh over
        const FemMesh& mesh = static_cast<FemMeshObject*>(res[i]->Mesh.getValue())->FemMesh.getValue();
        vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
        FemVTKTools::exportVTKMesh(&mesh, grid);

        // Now copy the point data over
        FemVTKTools::exportFreeCADResult(res[i], grid);

        // add time information
        vtkFloatArray* TimeValue = vtkFloatArray::New();
        TimeValue->SetNumberOfComponents(1);
        TimeValue->SetName("TimeValue");
        TimeValue->InsertNextValue(values[i]);
        grid->GetFieldData()->AddArray(TimeValue);
        grid->GetFieldData()->AddArray(TimeInfo);

        multiblock->SetBlock(i, grid);
    }

    multiblock->GetFieldData()->AddArray(TimeInfo);
    Data.setValue(multiblock);
}

void FemPostPipeline::handleChangedPropertyName(
    Base::XMLReader& reader,
    const char* typeName,
    const char* propName
)
{
    if (strcmp(propName, "Filter") == 0
        && Base::Type::fromName(typeName) == App::PropertyLinkList::getClassTypeId()) {

        // add the formerly filter values to the group
        App::PropertyLinkList filter;
        filter.setContainer(this);
        filter.Restore(reader);
        auto group_filter = filter.getValues();
        auto group = Group.getValues();
        group.insert(group.end(), group_filter.begin(), group_filter.end());
        Group.setValues(group);
    }
    else if (strcmp(propName, "Functions") == 0
             && Base::Type::fromName(typeName) == App::PropertyLink::getClassTypeId()) {

        // add the formerly Functions values to the group
        App::PropertyLink functions;
        functions.setContainer(this);
        functions.Restore(reader);
        if (functions.getValue()) {
            auto group = Group.getValues();
            group.push_back(functions.getValue());
            Group.setValues(group);
        }
    }
    else {
        FemPostObject::handleChangedPropertyName(reader, typeName, propName);
    }
}

void FemPostPipeline::onDocumentRestored()
{
    // if a old document was loaded with "custom" mode setting, the current value
    // would be out of range. Reset it to "serial"
    if (Mode.getValue() > Fem::PostGroupMode::Parallel
        || Mode.getValue() < Fem::PostGroupMode::Serial) {
        Mode.setValue(Fem::PostGroupMode::Serial);
    }
}

void FemPostPipeline::renameArrays(const std::map<std::string, std::string>& names)
{
    std::vector<vtkSmartPointer<vtkDataSet>> fields;
    auto data = Data.getValue();
    if (!data) {
        return;
    }

    if (auto dataSet = vtkDataSet::SafeDownCast(data)) {
        fields.emplace_back(dataSet);
    }
    else if (auto blocks = vtkMultiBlockDataSet::SafeDownCast(data)) {
        for (unsigned int i = 0; i < blocks->GetNumberOfBlocks(); ++i) {
            if (auto dataSet = vtkDataSet::SafeDownCast(blocks->GetBlock(i))) {
                fields.emplace_back(dataSet);
            }
        }
    }

    for (auto f : fields) {
        auto pointData = f->GetPointData();
        for (const auto& name : names) {
            auto array = pointData->GetAbstractArray(name.first.c_str());
            if (array) {
                array->SetName(name.second.c_str());
            }
        }
    }

    Data.touch();
}

void FemPostPipeline::addArrayFromFunction(const std::map<std::string, std::string>& functions)
{
    auto data = Data.getValue();
    FemVTKTools::addArrayFromFunction(data, functions);
    Data.setValue(data);
}

PyObject* FemPostPipeline::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FemPostPipelinePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
