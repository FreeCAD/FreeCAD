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


#include <Python.h>
#include <vtkCompositeDataSet.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkUniformGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkTable.h>
#include <vtkXMLTableWriter.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLMultiBlockDataWriter.h>
#include <vtkXMLTableReader.h>
#include <vtkXMLMultiBlockDataReader.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLUnstructuredGridReader.h>


#ifdef FC_USE_VTK_PYTHON
# include <vtkPythonUtil.h>
#else
# include <Base/PyObjectBase.h>
#endif

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Writer.h>
#include <CXX/Objects.hxx>


#ifdef _MSC_VER
# include <zipios++/zipios-config.h>
#endif
#include <zipios++/zipoutputstream.h>
#include <zipios++/zipinputstream.h>

#include "PropertyPostDataObject.h"


using namespace Fem;

TYPESYSTEM_SOURCE(Fem::PropertyPostDataObject, App::Property)

PropertyPostDataObject::PropertyPostDataObject() = default;

PropertyPostDataObject::~PropertyPostDataObject() = default;

void PropertyPostDataObject::scaleDataObject(vtkDataObject* dataObject, double s)
{
    auto scalePoints = [](vtkPoints* points, double s) {
        for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++) {
            double xyz[3];
            points->GetPoint(i, xyz);
            for (double& j : xyz) {
                j *= s;
            }
            points->SetPoint(i, xyz);
        }
    };

    if (dataObject->GetDataObjectType() == VTK_POLY_DATA) {
        vtkPolyData* dataSet = vtkPolyData::SafeDownCast(dataObject);
        scalePoints(dataSet->GetPoints(), s);
    }
    else if (dataObject->GetDataObjectType() == VTK_STRUCTURED_GRID) {
        vtkStructuredGrid* dataSet = vtkStructuredGrid::SafeDownCast(dataObject);
        scalePoints(dataSet->GetPoints(), s);
    }
    else if (dataObject->GetDataObjectType() == VTK_UNSTRUCTURED_GRID) {
        vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::SafeDownCast(dataObject);
        scalePoints(dataSet->GetPoints(), s);
    }
    else if (dataObject->GetDataObjectType() == VTK_MULTIBLOCK_DATA_SET) {
        vtkMultiBlockDataSet* dataSet = vtkMultiBlockDataSet::SafeDownCast(dataObject);
        for (unsigned int i = 0; i < dataSet->GetNumberOfBlocks(); i++) {
            scaleDataObject(dataSet->GetBlock(i), s);
        }
    }
    else if (dataObject->GetDataObjectType() == VTK_MULTIPIECE_DATA_SET) {
        vtkMultiPieceDataSet* dataSet = vtkMultiPieceDataSet::SafeDownCast(dataObject);
        for (unsigned int i = 0; i < dataSet->GetNumberOfPieces(); i++) {
            scaleDataObject(dataSet->GetPiece(i), s);
        }
    }
}

void PropertyPostDataObject::scale(double s)
{
    if (m_dataObject) {
        aboutToSetValue();
        scaleDataObject(m_dataObject, s);
        hasSetValue();
    }
}

void PropertyPostDataObject::setValue(const vtkSmartPointer<vtkDataObject>& ds)
{
    aboutToSetValue();

    if (ds) {
        createDataObjectByExternalType(ds);
        m_dataObject->DeepCopy(ds);
    }
    else {
        m_dataObject = nullptr;
    }

    hasSetValue();
}

const vtkSmartPointer<vtkDataObject>& PropertyPostDataObject::getValue() const
{
    return m_dataObject;
}

bool PropertyPostDataObject::isComposite()
{

    return m_dataObject && !m_dataObject->IsA("vtkDataSet");
}

bool PropertyPostDataObject::isDataSet()
{

    return m_dataObject && m_dataObject->IsA("vtkDataSet");
}

int PropertyPostDataObject::getDataType()
{

    if (!m_dataObject) {
        return -1;
    }

    return m_dataObject->GetDataObjectType();
}


PyObject* PropertyPostDataObject::getPyObject()
{
#ifdef FC_USE_VTK_PYTHON
    // create a copy first
    auto copy = static_cast<PropertyPostDataObject*>(Copy());

    // get the data python wrapper
    PyObject* py_dataset = vtkPythonUtil::GetObjectFromPointer(copy->getValue());
    auto result = Py::new_reference_to(py_dataset);
    delete copy;

    return result;
#else
    PyErr_SetString(PyExc_NotImplementedError, "VTK python wrapper not available");
    Py_Return;
#endif
}

void PropertyPostDataObject::setPyObject([[maybe_unused]] PyObject* value)
{
#ifdef FC_USE_VTK_PYTHON
    vtkObjectBase* obj = vtkPythonUtil::GetPointerFromObject(value, "vtkDataObject");
    if (!obj) {
        throw Base::TypeError("Can only set vtkDataObject");
    }
    auto dobj = static_cast<vtkDataObject*>(obj);
    createDataObjectByExternalType(dobj);

    aboutToSetValue();
    m_dataObject->DeepCopy(dobj);
    hasSetValue();
#else
    throw Base::NotImplementedError();
#endif
}

App::Property* PropertyPostDataObject::Copy() const
{
    PropertyPostDataObject* prop = new PropertyPostDataObject();
    if (m_dataObject) {

        prop->createDataObjectByExternalType(m_dataObject);
        prop->m_dataObject->DeepCopy(m_dataObject);
    }

    return prop;
}

void PropertyPostDataObject::createDataObjectByExternalType(vtkSmartPointer<vtkDataObject> ex)
{

    switch (ex->GetDataObjectType()) {

        case VTK_POLY_DATA:
            m_dataObject = vtkSmartPointer<vtkPolyData>::New();
            break;
        case VTK_STRUCTURED_GRID:
            m_dataObject = vtkSmartPointer<vtkStructuredGrid>::New();
            break;
        case VTK_RECTILINEAR_GRID:
            m_dataObject = vtkSmartPointer<vtkRectilinearGrid>::New();
            break;
        case VTK_UNSTRUCTURED_GRID:
            m_dataObject = vtkSmartPointer<vtkUnstructuredGrid>::New();
            break;
        case VTK_UNIFORM_GRID:
            m_dataObject = vtkSmartPointer<vtkUniformGrid>::New();
            break;
        case VTK_COMPOSITE_DATA_SET:
            m_dataObject = vtkCompositeDataSet::New();
            break;
        case VTK_MULTIBLOCK_DATA_SET:
            m_dataObject = vtkSmartPointer<vtkMultiBlockDataSet>::New();
            break;
        case VTK_MULTIPIECE_DATA_SET:
            m_dataObject = vtkSmartPointer<vtkMultiPieceDataSet>::New();
            break;
        case VTK_TABLE:
            m_dataObject = vtkSmartPointer<vtkTable>::New();
            break;
        default:
            throw Base::TypeError("Unsupported VTK data type");
    };
}


void PropertyPostDataObject::Paste(const App::Property& from)
{
    aboutToSetValue();
    m_dataObject = dynamic_cast<const PropertyPostDataObject&>(from).m_dataObject;
    hasSetValue();
}

unsigned int PropertyPostDataObject::getMemSize() const
{
    return m_dataObject ? m_dataObject->GetActualMemorySize() : 0;
}

void PropertyPostDataObject::getPaths(std::vector<App::ObjectIdentifier>& /*paths*/) const
{
    /* paths.push_back(App::ObjectIdentifier(getContainer())
                       << App::ObjectIdentifier::Component::SimpleComponent(getName())
                       << App::ObjectIdentifier::Component::SimpleComponent(
                              App::ObjectIdentifier::String("ShapeType")));
       paths.push_back(App::ObjectIdentifier(getContainer())
                       << App::ObjectIdentifier::Component::SimpleComponent(getName())
                       << App::ObjectIdentifier::Component::SimpleComponent(
                              App::ObjectIdentifier::String("Orientation")));
       paths.push_back(App::ObjectIdentifier(getContainer())
                       << App::ObjectIdentifier::Component::SimpleComponent(getName())
                       << App::ObjectIdentifier::Component::SimpleComponent(
                              App::ObjectIdentifier::String("Length")));
       paths.push_back(App::ObjectIdentifier(getContainer())
                       << App::ObjectIdentifier::Component::SimpleComponent(getName())
                       << App::ObjectIdentifier::Component::SimpleComponent(
                              App::ObjectIdentifier::String("Area")));
       paths.push_back(App::ObjectIdentifier(getContainer())
                       << App::ObjectIdentifier::Component::SimpleComponent(getName())
                       << App::ObjectIdentifier::Component::SimpleComponent(
                              App::ObjectIdentifier::String("Volume")));
       */
}

void PropertyPostDataObject::Save(Base::Writer& writer) const
{
    if (!m_dataObject) {
        return;
    }

    std::string extension;
    switch (m_dataObject->GetDataObjectType()) {

        case VTK_POLY_DATA:
            extension = "vtp";
            break;
        case VTK_STRUCTURED_GRID:
            extension = "vts";
            break;
        case VTK_RECTILINEAR_GRID:
            extension = "vtr";
            break;
        case VTK_UNSTRUCTURED_GRID:
            extension = "vtu";
            break;
        case VTK_UNIFORM_GRID:
            extension = "vti";  // image data
            break;
        case VTK_MULTIBLOCK_DATA_SET:
            extension = "zip";
            break;
        case VTK_TABLE:
            extension = ".vtt";
            break;
        default:
            break;
    };

    if (!writer.isForceXML()) {
        std::string file = "Data." + extension;
        writer.Stream() << writer.ind() << "<Data file=\"" << writer.addFile(file.c_str(), this)
                        << "\"/>" << std::endl;
    }
}

void PropertyPostDataObject::Restore(Base::XMLReader& reader)
{
    reader.readElement("Data");
    if (!reader.hasAttribute("file")) {
        return;
    }

    std::string file(reader.getAttribute<const char*>("file"));
    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
}

void add_to_zip(Base::FileInfo path, int zip_path_idx, zipios::ZipOutputStream& ZipWriter)
{

    if (path.isDir()) {
        for (auto file : path.getDirectoryContent()) {
            add_to_zip(file, zip_path_idx, ZipWriter);
        }
    }
    else {
        ZipWriter.putNextEntry(path.filePath().substr(zip_path_idx));
        Base::ifstream file(path, std::ios::in | std::ios::binary);
        if (file) {
            std::streambuf* buf = file.rdbuf();
            ZipWriter << buf;
        }
    }
}

void PropertyPostDataObject::SaveDocFile(Base::Writer& writer) const
{
    // If the shape is empty we simply store nothing. The file size will be 0 which
    // can be checked when reading in the data.
    if (!m_dataObject) {
        return;
    }

    // create a temporary file and copy the content to the zip stream
    // once the tmp. filename is known use always the same because otherwise
    // we may run into some problems on the Linux platform
    static Base::FileInfo fi = Base::FileInfo(App::Application::getTempFileName());

    Base::FileInfo datafolder;
    vtkSmartPointer<vtkXMLWriter> xmlWriter;
    if (m_dataObject->IsA("vtkMultiBlockDataSet")) {

        // create a tmp directory to write in
        datafolder = Base::FileInfo(App::Application::getTempPath() + "vtk_datadir");
        datafolder.createDirectories();
        auto datafile = Base::FileInfo(datafolder.filePath() + "/datafile.vtm");

        // create the data: vtm file and subfolder with the subsequent data files
        xmlWriter = vtkSmartPointer<vtkXMLMultiBlockDataWriter>::New();
        xmlWriter->SetInputDataObject(m_dataObject);
        xmlWriter->SetFileName(datafile.filePath().c_str());
    }
    else if (m_dataObject->IsA("vtkTable")) {
        xmlWriter = vtkSmartPointer<vtkXMLTableWriter>::New();
        xmlWriter->SetInputDataObject(m_dataObject);
        xmlWriter->SetFileName(fi.filePath().c_str());
    }
    else {
        xmlWriter = vtkSmartPointer<vtkXMLDataSetWriter>::New();
        xmlWriter->SetInputDataObject(m_dataObject);
        xmlWriter->SetFileName(fi.filePath().c_str());

        // Looks like an invalid data object that causes a crash with vtk9
        vtkUnstructuredGrid* dataGrid = vtkUnstructuredGrid::SafeDownCast(m_dataObject);
        if (dataGrid && (dataGrid->GetPiece() < 0 || dataGrid->GetNumberOfPoints() <= 0)) {
            std::cerr << "PropertyPostDataObject::SaveDocFile: ignore empty vtkUnstructuredGrid\n";
            return;
        }
    }
    xmlWriter->SetDataModeToBinary();

    if (xmlWriter->Write() != 1) {
        // Note: Do NOT throw an exception here because if the tmp. file could
        // not be created we should not abort.
        // We only print an error message but continue writing the next files to the
        // stream...
        App::PropertyContainer* father = this->getContainer();
        if (father && father->isDerivedFrom<App::DocumentObject>()) {
            App::DocumentObject* obj = static_cast<App::DocumentObject*>(father);
            Base::Console().error(
                "Dataset of '%s' cannot be written to vtk file '%s'\n",
                obj->Label.getValue(),
                fi.filePath().c_str()
            );
        }
        else {
            Base::Console().error("Cannot save vtk file '%s'\n", fi.filePath().c_str());
        }

        std::stringstream ss;
        ss << "Cannot save vtk file '" << fi.filePath() << "'";
        writer.addError(ss.str());
    }
    else if (m_dataObject->IsA("vtkMultiBlockDataSet")) {
        // ZIP file we store all data in
        zipios::ZipOutputStream ZipWriter(fi.filePath());
        ZipWriter.putNextEntry("dummy");  // need to add a dummy first, as the read stream preloads
                                          // the first entry, and we cannot get the file name...
        add_to_zip(datafolder, datafolder.filePath().length(), ZipWriter);
        ZipWriter.close();
        datafolder.deleteDirectoryRecursive();
    }

    Base::ifstream file(fi, std::ios::in | std::ios::binary);
    if (file) {
        std::streambuf* buf = file.rdbuf();
        writer.Stream() << buf;
    }

    file.close();
    // remove temp file
    fi.deleteFile();
}

void PropertyPostDataObject::RestoreDocFile(Base::Reader& reader)
{
    Base::FileInfo xml(reader.getFileName());
    // create a temporary file and copy the content from the zip stream
    Base::FileInfo fi(App::Application::getTempFileName());
    Base::FileInfo fo;

    // read in the ASCII file and write back to the file stream
    Base::ofstream file(fi, std::ios::out | std::ios::binary);
    unsigned long ulSize = 0;
    if (reader) {
        std::streambuf* buf = file.rdbuf();
        reader >> buf;
        file.flush();
        ulSize = buf->pubseekoff(0, std::ios::cur, std::ios::in);
    }
    file.close();

    // Read the data from the temp file
    if (ulSize > 0) {
        std::string extension = xml.extension();

        // TODO: read in of composite data structures need to be coded,
        // including replace of "GetOutputAsDataSet()"
        vtkSmartPointer<vtkXMLReader> xmlReader = nullptr;
        if (extension == "vtp") {
            xmlReader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
        }
        else if (extension == "vts") {
            xmlReader = vtkSmartPointer<vtkXMLStructuredGridReader>::New();
        }
        else if (extension == "vtr") {
            xmlReader = vtkSmartPointer<vtkXMLRectilinearGridReader>::New();
        }
        else if (extension == "vtu") {
            xmlReader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        }
        else if (extension == "vti") {
            xmlReader = vtkSmartPointer<vtkXMLImageDataReader>::New();
        }
        else if (extension == "vtt") {
            xmlReader = vtkSmartPointer<vtkXMLTableReader>::New();
        }
        else if (extension == "zip") {

            // first unzip the file into a datafolder
            zipios::ZipInputStream ZipReader(fi.filePath());
            fo = Base::FileInfo(App::Application::getTempPath() + "vtk_extract_datadir");
            fo.createDirectories();

            try {
                zipios::ConstEntryPointer entry = ZipReader.getNextEntry();
                while (entry->isValid()) {
                    Base::FileInfo entry_path(fo.filePath() + entry->getName());
                    if (entry->isDirectory()) {
                        // seems not to be called
                        entry_path.createDirectories();
                    }
                    else {
                        auto entry_dir = Base::FileInfo(entry_path.dirPath());
                        if (!entry_dir.exists()) {
                            entry_dir.createDirectories();
                        }

                        Base::ofstream file(entry_path, std::ios::out | std::ios::binary);
                        std::streambuf* buf = file.rdbuf();
                        ZipReader >> buf;
                        file.flush();
                        file.close();
                    }
                    entry = ZipReader.getNextEntry();
                }
            }
            catch (const std::exception&) {
                // there is no further entry
            }

            // create the reader, and change the file for it to read. Also delete zip file, not
            // needed anymore
            fi.deleteFile();
            fi = Base::FileInfo(fo.filePath() + "/datafile.vtm");
            xmlReader = vtkSmartPointer<vtkXMLMultiBlockDataReader>::New();
        }

        if (xmlReader) {
            xmlReader->SetFileName(fi.filePath().c_str());
            xmlReader->Update();

            if (!xmlReader->GetOutputDataObject(0)) {
                // Note: Do NOT throw an exception here because if the tmp. created file could
                // not be read it's NOT an indication for an invalid input stream 'reader'.
                // We only print an error message but continue reading the next files from the
                // stream...
                App::PropertyContainer* father = this->getContainer();
                if (father && father->isDerivedFrom<App::DocumentObject>()) {
                    App::DocumentObject* obj = static_cast<App::DocumentObject*>(father);
                    Base::Console().error(
                        "Dataset file '%s' with data of '%s' seems to be empty\n",
                        fi.filePath().c_str(),
                        obj->Label.getValue()
                    );
                }
                else {
                    Base::Console().warning(
                        "Loaded Dataset file '%s' seems to be empty\n",
                        fi.filePath().c_str()
                    );
                }
            }
            else {
                aboutToSetValue();
                createDataObjectByExternalType(xmlReader->GetOutputDataObject(0));
                m_dataObject->DeepCopy(xmlReader->GetOutputDataObject(0));
                hasSetValue();
            }
        }
        else {
            Base::Console().error(
                "Dataset file '%s' is of unsupported type: %s. Data not loaded.\n",
                fi.filePath().c_str(),
                extension
            );
        }
    }

    // delete the temp file
    fi.deleteFile();
    if (xml.extension() == "zip") {
        fo.deleteDirectoryRecursive();
    }
}
