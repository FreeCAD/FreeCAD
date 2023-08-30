/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <sstream>
# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepTools.hxx>
# include <BRepTools_ShapeSet.hxx>
# include <OSD_OpenFile.hxx>
# include <Standard_Failure.hxx>
# include <Standard_Version.hxx>
# include <TopoDS.hxx>
#endif // _PreComp_

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Writer.h>

#include "PropertyTopoShape.h"
#include "TopoShapePy.h"


using namespace Part;

TYPESYSTEM_SOURCE(Part::PropertyPartShape , App::PropertyComplexGeoData)

PropertyPartShape::PropertyPartShape() = default;

PropertyPartShape::~PropertyPartShape() = default;

void PropertyPartShape::setValue(const TopoShape& sh)
{
    aboutToSetValue();
    _Shape = sh;
    hasSetValue();
}

void PropertyPartShape::setValue(const TopoDS_Shape& sh)
{
    aboutToSetValue();
    _Shape.setShape(sh);
    hasSetValue();
}

const TopoDS_Shape& PropertyPartShape::getValue() const
{
    return _Shape.getShape();
}

const TopoShape& PropertyPartShape::getShape() const
{
    return this->_Shape;
}

const Data::ComplexGeoData* PropertyPartShape::getComplexData() const
{
    return &(this->_Shape);
}

Base::BoundBox3d PropertyPartShape::getBoundingBox() const
{
    Base::BoundBox3d box;
    if (_Shape.getShape().IsNull())
        return box;
    try {
        // If the shape is empty an exception may be thrown
        Bnd_Box bounds;
        BRepBndLib::Add(_Shape.getShape(), bounds);
        bounds.SetGap(0.0);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

        box.MinX = xMin;
        box.MaxX = xMax;
        box.MinY = yMin;
        box.MaxY = yMax;
        box.MinZ = zMin;
        box.MaxZ = zMax;
    }
    catch (Standard_Failure&) {
    }

    return box;
}

void PropertyPartShape::setTransform(const Base::Matrix4D &rclTrf)
{
    _Shape.setTransform(rclTrf);
}

Base::Matrix4D PropertyPartShape::getTransform() const
{
    return _Shape.getTransform();
}

void PropertyPartShape::transformGeometry(const Base::Matrix4D &rclTrf)
{
    aboutToSetValue();
    _Shape.transformGeometry(rclTrf);
    hasSetValue();
}

PyObject *PropertyPartShape::getPyObject()
{
    Base::PyObjectBase* prop = static_cast<Base::PyObjectBase*>(_Shape.getPyObject());
    if (prop)
        prop->setConst();
    return prop;
}

void PropertyPartShape::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(TopoShapePy::Type))) {
        TopoShapePy *pcObject = static_cast<TopoShapePy*>(value);
        setValue(*pcObject->getTopoShapePtr());
    }
    else {
        std::string error = std::string("type must be 'Shape', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

App::Property *PropertyPartShape::Copy() const
{
    PropertyPartShape *prop = new PropertyPartShape();
    prop->_Shape = this->_Shape;
    if (!_Shape.getShape().IsNull()) {
        BRepBuilderAPI_Copy copy(_Shape.getShape());
        prop->_Shape.setShape(copy.Shape());
    }

    return prop;
}

void PropertyPartShape::Paste(const App::Property &from)
{
    aboutToSetValue();
    _Shape = dynamic_cast<const PropertyPartShape&>(from)._Shape;
    hasSetValue();
}

unsigned int PropertyPartShape::getMemSize () const
{
    return _Shape.getMemSize();
}

void PropertyPartShape::getPaths(std::vector<App::ObjectIdentifier> &paths) const
{
    paths.push_back(App::ObjectIdentifier(getContainer()) << App::ObjectIdentifier::Component::SimpleComponent(getName())
                    << App::ObjectIdentifier::Component::SimpleComponent(App::ObjectIdentifier::String("ShapeType")));
    paths.push_back(App::ObjectIdentifier(getContainer()) << App::ObjectIdentifier::Component::SimpleComponent(getName())
                    << App::ObjectIdentifier::Component::SimpleComponent(App::ObjectIdentifier::String("Orientation")));
    paths.push_back(App::ObjectIdentifier(getContainer()) << App::ObjectIdentifier::Component::SimpleComponent(getName())
                    << App::ObjectIdentifier::Component::SimpleComponent(App::ObjectIdentifier::String("Length")));
    paths.push_back(App::ObjectIdentifier(getContainer()) << App::ObjectIdentifier::Component::SimpleComponent(getName())
                    << App::ObjectIdentifier::Component::SimpleComponent(App::ObjectIdentifier::String("Area")));
    paths.push_back(App::ObjectIdentifier(getContainer()) << App::ObjectIdentifier::Component::SimpleComponent(getName())
                    << App::ObjectIdentifier::Component::SimpleComponent(App::ObjectIdentifier::String("Volume")));
}

void PropertyPartShape::Save (Base::Writer &writer) const
{
    if(!writer.isForceXML()) {
        //See SaveDocFile(), RestoreDocFile()
        if (writer.getMode("BinaryBrep")) {
            writer.Stream() << writer.ind() << "<Part file=\""
                            << writer.addFile("PartShape.bin", this)
                            << "\"/>" << std::endl;
        }
        else {
            writer.Stream() << writer.ind() << "<Part file=\""
                            << writer.addFile("PartShape.brp", this)
                            << "\"/>" << std::endl;
        }
    }
}

void PropertyPartShape::Restore(Base::XMLReader &reader)
{
    reader.readElement("Part");
    std::string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(),this);
    }
}

// The following function is copied from OCCT BRepTools.cxx and modified
// to disable saving of triangulation
//

static Standard_Boolean  BRepTools_Write(const TopoDS_Shape& Sh, const Standard_CString File)
{
  std::ofstream os;
  OSD_OpenStream(os, File, std::ios::out);

  if (!os.rdbuf()->is_open())
      return Standard_False;

  Standard_Boolean isGood = (os.good() && !os.eof());
  if(!isGood)
    return isGood;

  // See TopTools_FormatVersion of OCCT 7.6
  enum {
      VERSION_1 = 1,
      VERSION_2 = 2,
      VERSION_3 = 3
  };

  BRepTools_ShapeSet SS(Standard_False);
  SS.SetFormatNb(VERSION_1);
  // SS.SetProgress(PR);
  SS.Add(Sh);

  os << "DBRep_DrawableShape\n";  // for easy Draw read
  SS.Write(os);
  isGood = os.good();
  if(isGood )
    SS.Write(Sh,os);
  os.flush();
  isGood = os.good();

  errno = 0;
  os.close();
  isGood = os.good() && isGood && !errno;

  return isGood;
}

void PropertyPartShape::saveToFile(Base::Writer &writer) const
{
    // create a temporary file and copy the content to the zip stream
    // once the tmp. filename is known use always the same because otherwise
    // we may run into some problems on the Linux platform
    static Base::FileInfo fi(App::Application::getTempFileName());

    TopoDS_Shape myShape = _Shape.getShape();
    if (!BRepTools_Write(myShape,static_cast<Standard_CString>(fi.filePath().c_str()))) {
        // Note: Do NOT throw an exception here because if the tmp. file could
        // not be created we should not abort.
        // We only print an error message but continue writing the next files to the
        // stream...
        App::PropertyContainer* father = this->getContainer();
        if (father && father->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
            App::DocumentObject* obj = static_cast<App::DocumentObject*>(father);
            Base::Console().Error("Shape of '%s' cannot be written to BRep file '%s'\n",
                obj->Label.getValue(),fi.filePath().c_str());
        }
        else {
            Base::Console().Error("Cannot save BRep file '%s'\n", fi.filePath().c_str());
        }

        std::stringstream ss;
        ss << "Cannot save BRep file '" << fi.filePath() << "'";
        writer.addError(ss.str());
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

void PropertyPartShape::loadFromFile(Base::Reader &reader)
{
    BRep_Builder builder;
    // create a temporary file and copy the content from the zip stream
    Base::FileInfo fi(App::Application::getTempFileName());

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

    // Read the shape from the temp file, if the file is empty the stored shape was already empty.
    // If it's still empty after reading the (non-empty) file there must occurred an error.
    TopoDS_Shape shape;
    if (ulSize > 0) {
        if (!BRepTools::Read(shape, static_cast<Standard_CString>(fi.filePath().c_str()), builder)) {
            // Note: Do NOT throw an exception here because if the tmp. created file could
            // not be read it's NOT an indication for an invalid input stream 'reader'.
            // We only print an error message but continue reading the next files from the
            // stream...
            App::PropertyContainer* father = this->getContainer();
            if (father && father->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
                App::DocumentObject* obj = static_cast<App::DocumentObject*>(father);
                Base::Console().Error("BRep file '%s' with shape of '%s' seems to be empty\n",
                    fi.filePath().c_str(),obj->Label.getValue());
            }
            else {
                Base::Console().Warning("Loaded BRep file '%s' seems to be empty\n", fi.filePath().c_str());
            }
        }
    }

    // delete the temp file
    fi.deleteFile();
    setValue(shape);
}

void PropertyPartShape::loadFromStream(Base::Reader &reader)
{
    try {
        reader.exceptions(std::istream::failbit | std::istream::badbit);
        BRep_Builder builder;
        TopoDS_Shape shape;
        BRepTools::Read(shape, reader, builder);
        setValue(shape);
    }
    catch (const std::exception&) {
        if (!reader.eof())
            Base::Console().Warning("Failed to load BRep file %s\n", reader.getFileName().c_str());
    }
}

void PropertyPartShape::SaveDocFile (Base::Writer &writer) const
{
    // If the shape is empty we simply store nothing. The file size will be 0 which
    // can be checked when reading in the data.
    if (_Shape.getShape().IsNull())
        return;
    TopoDS_Shape myShape = _Shape.getShape();
    if (writer.getMode("BinaryBrep")) {
        TopoShape shape;
        shape.setShape(myShape);
        shape.exportBinary(writer.Stream());
    }
    else {
        bool direct = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Mod/Part/General")->GetBool("DirectAccess", true);
        if (!direct) {
            saveToFile(writer);
        }
        else {
            TopoShape shape;
            shape.setShape(myShape);
            shape.exportBrep(writer.Stream());
        }
    }
}

void PropertyPartShape::RestoreDocFile(Base::Reader &reader)
{
    Base::FileInfo brep(reader.getFileName());
    if (brep.hasExtension("bin")) {
        TopoShape shape;
        shape.importBinary(reader);
        setValue(shape);
    }
    else {
        bool direct = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Mod/Part/General")->GetBool("DirectAccess", true);
        if (!direct) {
            loadFromFile(reader);
        }
        else {
            auto iostate = reader.exceptions();
            loadFromStream(reader);
            reader.exceptions(iostate);
        }
    }
}

// -------------------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::PropertyShapeHistory , App::PropertyLists)

PropertyShapeHistory::PropertyShapeHistory() = default;

PropertyShapeHistory::~PropertyShapeHistory() = default;

void PropertyShapeHistory::setValue(const ShapeHistory& sh)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0] = sh;
    hasSetValue();
}

void PropertyShapeHistory::setValues(const std::vector<ShapeHistory>& values)
{
    aboutToSetValue();
    _lValueList = values;
    hasSetValue();
}

PyObject *PropertyShapeHistory::getPyObject()
{
    return Py::new_reference_to(Py::None());
}

void PropertyShapeHistory::setPyObject(PyObject *)
{
}

void PropertyShapeHistory::Save (Base::Writer &) const
{
}

void PropertyShapeHistory::Restore(Base::XMLReader &)
{
}

void PropertyShapeHistory::SaveDocFile (Base::Writer &) const
{
}

void PropertyShapeHistory::RestoreDocFile(Base::Reader &)
{
}

App::Property *PropertyShapeHistory::Copy() const
{
    PropertyShapeHistory *p= new PropertyShapeHistory();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyShapeHistory::Paste(const Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyShapeHistory&>(from)._lValueList;
    hasSetValue();
}

// -------------------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::PropertyFilletEdges , App::PropertyLists)

PropertyFilletEdges::PropertyFilletEdges() = default;

PropertyFilletEdges::~PropertyFilletEdges() = default;

void PropertyFilletEdges::setValue(int id, double r1, double r2)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0].edgeid = id;
    _lValueList[0].radius1 = r1;
    _lValueList[0].radius2 = r2;
    hasSetValue();
}

void PropertyFilletEdges::setValues(const std::vector<FilletElement>& values)
{
    aboutToSetValue();
    _lValueList = values;
    hasSetValue();
}

PyObject *PropertyFilletEdges::getPyObject()
{
    Py::List list(getSize());
    std::vector<FilletElement>::const_iterator it;
    int index = 0;
    for (it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        Py::Tuple ent(3);
        ent.setItem(0, Py::Long(it->edgeid));
        ent.setItem(1, Py::Float(it->radius1));
        ent.setItem(2, Py::Float(it->radius2));
        list[index++] = ent;
    }

    return Py::new_reference_to(list);
}

void PropertyFilletEdges::setPyObject(PyObject *value)
{
    Py::Sequence list(value);
    std::vector<FilletElement> values;
    values.reserve(list.size());
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        FilletElement fe;
        Py::Tuple ent(*it);
        fe.edgeid = (int)Py::Long(ent.getItem(0));
        fe.radius1 = (double)Py::Float(ent.getItem(1));
        fe.radius2 = (double)Py::Float(ent.getItem(2));
        values.push_back(fe);
    }

    setValues(values);
}

void PropertyFilletEdges::Save (Base::Writer &writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<FilletEdges file=\"" << writer.addFile(getName(), this) << "\"/>" << std::endl;
    }
}

void PropertyFilletEdges::Restore(Base::XMLReader &reader)
{
    reader.readElement("FilletEdges");
    std::string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(),this);
    }
}

void PropertyFilletEdges::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (const auto & it : _lValueList) {
        str << it.edgeid << it.radius1 << it.radius2;
    }
}

void PropertyFilletEdges::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader);
    uint32_t uCt=0;
    str >> uCt;
    std::vector<FilletElement> values(uCt);
    for (auto & it : values) {
        str >> it.edgeid >> it.radius1 >> it.radius2;
    }
    setValues(values);
}

App::Property *PropertyFilletEdges::Copy() const
{
    PropertyFilletEdges *p= new PropertyFilletEdges();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyFilletEdges::Paste(const Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyFilletEdges&>(from)._lValueList;
    hasSetValue();
}
