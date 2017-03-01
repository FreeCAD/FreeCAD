/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <Bnd_Box.hxx>
# include <BRepTools.hxx>
# include <BRepTools_ShapeSet.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <TopTools_MapOfShape.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopExp.hxx>
# include <Standard_Failure.hxx>
# include <gp_GTrsf.hxx>
# include <gp_Trsf.hxx>
#endif


#include <Base/Console.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <App/Application.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>

#include "PropertyTopoShape.h"
#include "TopoShapePy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeWirePy.h"
#include "TopoShapeVertexPy.h"
#include "TopoShapeSolidPy.h"
#include "TopoShapeShellPy.h"
#include "TopoShapeCompSolidPy.h"
#include "TopoShapeCompoundPy.h"

using namespace Part;

TYPESYSTEM_SOURCE(Part::PropertyPartShape , App::PropertyComplexGeoData);

PropertyPartShape::PropertyPartShape()
{
}

PropertyPartShape::~PropertyPartShape()
{
}

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

const TopoDS_Shape& PropertyPartShape::getValue(void)const 
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
    catch (Standard_Failure) {
    }

    return box;
}

void PropertyPartShape::transformGeometry(const Base::Matrix4D &rclTrf)
{
    aboutToSetValue();
    _Shape.transformGeometry(rclTrf);
    hasSetValue();
}

PyObject *PropertyPartShape::getPyObject(void)
{
    Base::PyObjectBase* prop;
    const TopoDS_Shape& sh = _Shape.getShape();
    if (sh.IsNull()) {
        prop = new TopoShapePy(new TopoShape(sh));
    }
    else {
        TopAbs_ShapeEnum type = sh.ShapeType();
        switch (type)
        {
        case TopAbs_COMPOUND:
            prop = new TopoShapeCompoundPy(new TopoShape(sh));
            break;
        case TopAbs_COMPSOLID:
            prop = new TopoShapeCompSolidPy(new TopoShape(sh));
            break;
        case TopAbs_SOLID:
            prop = new TopoShapeSolidPy(new TopoShape(sh));
            break;
        case TopAbs_SHELL:
            prop = new TopoShapeShellPy(new TopoShape(sh));
            break;
        case TopAbs_FACE:
            prop = new TopoShapeFacePy(new TopoShape(sh));
            break;
        case TopAbs_WIRE:
            prop = new TopoShapeWirePy(new TopoShape(sh));
            break;
        case TopAbs_EDGE:
            prop = new TopoShapeEdgePy(new TopoShape(sh));
            break;
        case TopAbs_VERTEX:
            prop = new TopoShapeVertexPy(new TopoShape(sh));
            break;
        case TopAbs_SHAPE:
        default:
            prop = new TopoShapePy(new TopoShape(sh));
            break;
        }
    }

    if (prop) prop->setConst();
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

App::Property *PropertyPartShape::Copy(void) const
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

unsigned int PropertyPartShape::getMemSize (void) const
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
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
}

void PropertyPartShape::SaveDocFile (Base::Writer &writer) const
{
    // If the shape is empty we simply store nothing. The file size will be 0 which
    // can be checked when reading in the data.
    if (_Shape.getShape().IsNull())
        return;
    // NOTE: Cleaning the triangulation may cause problems on some algorithms like BOP
    // Before writing to the project we clean all triangulation data to save memory
    BRepBuilderAPI_Copy copy(_Shape.getShape());
    const TopoDS_Shape& myShape = copy.Shape();
    BRepTools::Clean(myShape); // remove triangulation

    if (writer.getMode("BinaryBrep")) {
        TopoShape shape;
        shape.setShape(myShape);
        shape.exportBinary(writer.Stream());
    }
    else {
        bool direct = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Mod/Part/General")->GetBool("DirectAccess", true);
        if (!direct) {
            // create a temporary file and copy the content to the zip stream
            // once the tmp. filename is known use always the same because otherwise
            // we may run into some problems on the Linux platform
            static Base::FileInfo fi(App::Application::getTempFileName());

            if (!BRepTools::Write(myShape,(const Standard_CString)fi.filePath().c_str())) {
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
                //unsigned long ulSize = 0; 
                std::streambuf* buf = file.rdbuf();
                //if (buf) {
                //    unsigned long ulCurr;
                //    ulCurr = buf->pubseekoff(0, std::ios::cur, std::ios::in);
                //    ulSize = buf->pubseekoff(0, std::ios::end, std::ios::in);
                //    buf->pubseekoff(ulCurr, std::ios::beg, std::ios::in);
                //}

                // read in the ASCII file and write back to the stream
                //std::strstreambuf sbuf(ulSize);
                //file >> &sbuf;
                //writer.Stream() << &sbuf;
                writer.Stream() << buf;
            }

            file.close();
            // remove temp file
            fi.deleteFile();
        }
        else {
            BRepTools::Write(myShape, writer.Stream());
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
                if (!BRepTools::Read(shape, (const Standard_CString)fi.filePath().c_str(), builder)) {
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
        else {
            BRep_Builder builder;
            TopoDS_Shape shape;
            BRepTools::Read(shape, reader, builder);
            setValue(shape);
        }
    }
}

// -------------------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::PropertyShapeHistory , App::PropertyLists);

PropertyShapeHistory::PropertyShapeHistory()
{
}

PropertyShapeHistory::~PropertyShapeHistory()
{
}

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

PyObject *PropertyShapeHistory::getPyObject(void)
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

App::Property *PropertyShapeHistory::Copy(void) const
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

TYPESYSTEM_SOURCE(Part::PropertyFilletEdges , App::PropertyLists);

PropertyFilletEdges::PropertyFilletEdges()
{
}

PropertyFilletEdges::~PropertyFilletEdges()
{
}

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

PyObject *PropertyFilletEdges::getPyObject(void)
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
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
}

void PropertyFilletEdges::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (std::vector<FilletElement>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        str << it->edgeid << it->radius1 << it->radius2;
    }
}

void PropertyFilletEdges::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader);
    uint32_t uCt=0;
    str >> uCt;
    std::vector<FilletElement> values(uCt);
    for (std::vector<FilletElement>::iterator it = values.begin(); it != values.end(); ++it) {
        str >> it->edgeid >> it->radius1 >> it->radius2;
    }
    setValues(values);
}

App::Property *PropertyFilletEdges::Copy(void) const
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
