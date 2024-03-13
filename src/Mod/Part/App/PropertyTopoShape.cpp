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
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Writer.h>

#include "PartFeature.h"
#include "PartPyCXX.h"
#include "PropertyTopoShape.h"
#include "TopoShapePy.h"
#include "PartFeature.h"

FC_LOG_LEVEL_INIT("App", true, true)

namespace sp = std::placeholders;
using namespace Part;

TYPESYSTEM_SOURCE(Part::PropertyPartShape , App::PropertyComplexGeoData)

PropertyPartShape::PropertyPartShape() = default;

PropertyPartShape::~PropertyPartShape() = default;

void PropertyPartShape::setValue(const TopoShape& sh)
{
    aboutToSetValue();
    _Shape = sh;
    auto obj = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    if(obj) {
        auto tag = obj->getID();
        if(_Shape.Tag && tag!=_Shape.Tag) {
            auto hasher = _Shape.Hasher?_Shape.Hasher:obj->getDocument()->getStringHasher();
            _Shape.reTagElementMap(tag,hasher);
        } else
            _Shape.Tag = obj->getID();
        if (!_Shape.Hasher && _Shape.hasChildElementMap()) {
            _Shape.Hasher = obj->getDocument()->getStringHasher();
            _Shape.hashChildMaps();
        }
    }
    hasSetValue();
    _Ver.clear();
}

void PropertyPartShape::setValue(const TopoDS_Shape& sh, bool resetElementMap)
{
    aboutToSetValue();
    auto obj = dynamic_cast<App::DocumentObject*>(getContainer());
    if(obj)
        _Shape.Tag = obj->getID();
    _Shape.setShape(sh,resetElementMap);
    hasSetValue();
    _Ver.clear();
}

const TopoDS_Shape& PropertyPartShape::getValue() const
{
    return _Shape.getShape();
}

TopoShape PropertyPartShape::getShape() const
{
    _Shape.initCache(-1);
    auto res = _Shape;
    // March, 2024 Toponaming project:  There was originally an unused feature to disable
    // elementMapping that has not been kept:
    //    if (Feature::isElementMappingDisabled(getContainer()))
    //        res.Tag = -1;
    //    else if (!res.Tag) {
    if (!res.Tag) {
        if (auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer())) {
            res.Tag = parent->getID();
        }
    }
    return res;
}

const Data::ComplexGeoData* PropertyPartShape::getComplexData() const
{
    _Shape.initCache(-1);
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
        auto shape = *static_cast<TopoShapePy*>(value)->getTopoShapePtr();
        auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
        if(owner && owner->getDocument()) {
            if(shape.Tag || shape.getElementMapSize()) {
                // We can't trust the meaning of the input shape tag, so we
                // remap anyway
                TopoShape res(owner->getID(),owner->getDocument()->getStringHasher(),shape.getShape());
                res.mapSubElement(shape);
                shape = res;
            }else{
                shape.Tag = owner->getID();
                if ( shape.Hasher ) {   // TODO: This null guard added during TNP transition
                    shape.Hasher->clear();
                }
            }
        }
        setValue(shape);
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

    // March, 2024 Toponaming project:  There was originally a feature to enable making an element
    // copy ( new geometry and map ) that has not been kept:
//    if (PartParams::getShapePropertyCopy()) {
//        // makeElementCopy() consume too much memory for complex geometry.
//        prop->_Shape = this->_Shape.makeElementCopy();
//    } else
//        prop->_Shape = this->_Shape;
    prop->_Shape = this->_Shape;
    prop->_Ver = this->_Ver;
    return prop;
}

void PropertyPartShape::Paste(const App::Property &from)
{
    auto prop = Base::freecad_dynamic_cast<const PropertyPartShape>(&from);
    if(prop) {
        setValue(prop->_Shape);
        _Ver = prop->_Ver;
    }
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

void PropertyPartShape::beforeSave() const
{
    _HasherIndex = 0;
    _SaveHasher = false;
    auto owner = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    if(owner && !_Shape.isNull() && _Shape.getElementMapSize()>0) {
        auto ret = owner->getDocument()->addStringHasher(_Shape.Hasher);
        _HasherIndex = ret.second;
        _SaveHasher = ret.first;
        _Shape.beforeSave();
    }
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

#ifdef NOT_YET_AND_MAYBE_NEVER
void PropertyPartShape::Save (Base::Writer &writer) const
{
    //See SaveDocFile(), RestoreDocFile()
    writer.Stream() << writer.ind() << "<Part";
    auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
    if(owner && !_Shape.isNull()
        && _Shape.getElementMapSize()>0
        && !_Shape.Hasher.isNull()) {
        writer.Stream() << " HasherIndex=\"" << _HasherIndex << '"';
        if(_SaveHasher)
            writer.Stream() << " SaveHasher=\"1\"";
    }
    std::string version;
    // If exporting, do not export mapped element name, but still make a mark
    if(owner) {
        if(!owner->isExporting())
            version = _Ver.size()?_Ver:owner->getElementMapVersion(this);
    }else
        version = _Ver.size()?_Ver:_Shape.getElementMapVersion();
    writer.Stream() << " ElementMap=\"" << version << '"';

    bool binary = writer.getMode("BinaryBrep");
    bool toXML = writer.getFileVersion()>1 && writer.isForceXML()>=(binary?3:2);
    if(!toXML) {
        writer.Stream() << " file=\""
                        << writer.addFile(getFileName(binary?".bin":".brp"), this)
                        << "\"/>\n";
    } else if(binary) {
        writer.Stream() << " binary=\"1\">\n";
        TopoShape shape;
        shape.setShape(_Shape.getShape());
        shape.exportBinary(writer.beginCharStream(true));
        writer.endCharStream() <<  writer.ind() << "</Part>\n";
    } else {
        writer.Stream() << " brep=\"1\">\n";
        _Shape.exportBrep(writer.beginCharStream(false)<<'\n');
        writer.endCharStream() << '\n' << writer.ind() << "</Part>\n";
    }

    if(_SaveHasher) {
        if(!toXML)
            _Shape.Hasher->setPersistenceFileName(getFileName(".Table").c_str());
        else
            _Shape.Hasher->setPersistenceFileName(0);
        _Shape.Hasher->Save(writer);
    }
    if(version.size()) {
        if(!toXML)
            _Shape.setPersistenceFileName(getFileName(".Map").c_str());
        else
            _Shape.setPersistenceFileName(0);
        _Shape.Save(writer);
    }
}
#endif

std::string PropertyPartShape::getElementMapVersion(bool restored) const {
    if(restored)
        return _Ver;
    return PropertyComplexGeoData::getElementMapVersion(false);
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

#ifdef NOT_YET_AND_MAYBE_NEVER
void PropertyPartShape::Restore(Base::XMLReader &reader)
{
    reader.readElement("Part");

    auto owner = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    _Ver = "?";
    bool has_ver = reader.hasAttribute("ElementMap");
    if(has_ver)
        _Ver = reader.getAttribute("ElementMap");

    int hasher_idx = reader.getAttributeAsInteger("HasherIndex","-1");
    int save_hasher = reader.getAttributeAsInteger("SaveHasher","");

    TopoDS_Shape sh;

    if(reader.hasAttribute("file")) {
        std::string file = reader.getAttribute("file");
        if (!file.empty()) {
            // initiate a file read
            reader.addFile(file.c_str(),this);
        }
    } else if(reader.getAttributeAsInteger("binary","")) {
        TopoShape shape;
        shape.importBinary(reader.beginCharStream(true));
        sh = shape.getShape();
    } else if(reader.getAttributeAsInteger("brep","")) {
        BRep_Builder builder;
        BRepTools::Read(sh, reader.beginCharStream(false), builder);
    }

    reader.readEndElement("Part");

    if(owner && hasher_idx>=0) {
        _Shape.Hasher = owner->getDocument()->getStringHasher(hasher_idx);
        if(save_hasher)
            _Shape.Hasher->Restore(reader);
    }

    if(has_ver) {
        // The file name here is not used for restore, but just a way to get
        // more useful error message if something wrong when restoring
        _Shape.setPersistenceFileName(getFileName().c_str());
        if(owner && owner->getDocument()->testStatus(App::Document::PartialDoc))
            _Shape.Restore(reader);
        else if(_Ver == "?" || _Ver.empty()) {
            // This indicate the shape is saved by legacy version without
            // element map info.
            if(owner) {
                // This will ask user for recompute after import
                owner->getDocument()->addRecomputeObject(owner);
            }
        }else{
            _Shape.Restore(reader);
            if (owner ? owner->checkElementMapVersion(this, _Ver.c_str())
                      : _Shape.checkElementMapVersion(_Ver.c_str())) {
                auto ver = owner?owner->getElementMapVersion(this):_Shape.getElementMapVersion();
                if(!owner || !owner->getNameInDocument() || !_Shape.getElementMapSize()) {
                    _Ver = ver;
                } else {
                    // version mismatch, signal for regenerating.
                    static const char *warnedDoc=0;
                    if(warnedDoc != owner->getDocument()->getName()) {
                        warnedDoc = owner->getDocument()->getName();
                        FC_WARN("Recomputation required for document '" << warnedDoc
                                                                        << "' on geo element version change in " << getFullName()
                                                                        << ": " << _Ver << " -> " << ver);
                    }
                    owner->getDocument()->addRecomputeObject(owner);
                }
            }
        }
    } else if(owner && !owner->getDocument()->testStatus(App::Document::PartialDoc)) {
        if(App::DocumentParams::getWarnRecomputeOnRestore()) {
            FC_WARN("Pending recompute for generating element map: " << owner->getFullName());
            owner->getDocument()->addRecomputeObject(owner);
        }
    }

    if (!sh.IsNull() || !_Shape.isNull()) {
        aboutToSetValue();
        _Shape.setShape(sh,false);
        hasSetValue();
    }
}

void PropertyPartShape::afterRestore()
{
    if (_Shape.isRestoreFailed()) {
        // this cause GeoFeature::updateElementReference() to call
        // PropertyLinkBase::updateElementReferences() with reverse = true, in
        // order to try to regenerate the element map
        _Ver = "?";
    }
    else if (_Shape.getElementMapSize() == 0)
        _Shape.Hasher->clear(); //reset();
    PropertyComplexGeoData::afterRestore();
}
#endif

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

ShapeHistory::ShapeHistory(BRepBuilderAPI_MakeShape& mkShape, TopAbs_ShapeEnum type,
                           const TopoDS_Shape& newS, const TopoDS_Shape& oldS)
{
    reset(mkShape,type,newS,oldS);
}

void ShapeHistory::reset(BRepBuilderAPI_MakeShape& mkShape, TopAbs_ShapeEnum type,
                         const TopoDS_Shape& newS, const TopoDS_Shape& oldS)
{
    shapeMap.clear();
    this->type = type;

    TopTools_IndexedMapOfShape newM, oldM;
    TopExp::MapShapes(newS, type, newM); // map containing all old objects of type "type"
    TopExp::MapShapes(oldS, type, oldM); // map containing all new objects of type "type"

    // Look at all objects in the old shape and try to find the modified object in the new shape
    for (int i=1; i<=oldM.Extent(); i++) {
        bool found = false;
        TopTools_ListIteratorOfListOfShape it;
        // Find all new objects that are a modification of the old object (e.g. a face was resized)
        for (it.Initialize(mkShape.Modified(oldM(i))); it.More(); it.Next()) {
            found = true;
            for (int j=1; j<=newM.Extent(); j++) { // one old object might create several new ones!
                if (newM(j).IsPartner(it.Value())) {
                    shapeMap[i-1].push_back(j-1); // adjust indices to start at zero
                    break;
                }
            }
        }

        // Find all new objects that were generated from an old object (e.g. a face generated from an edge)
        for (it.Initialize(mkShape.Generated(oldM(i))); it.More(); it.Next()) {
            found = true;
            for (int j=1; j<=newM.Extent(); j++) {
                if (newM(j).IsPartner(it.Value())) {
                    shapeMap[i-1].push_back(j-1);
                    break;
                }
            }
        }

        if (!found) {
            // Find all old objects that don't exist any more (e.g. a face was completely cut away)
            if (mkShape.IsDeleted(oldM(i))) {
                shapeMap[i-1] = std::vector<int>();
            }
            else {
                // Mop up the rest (will this ever be reached?)
                for (int j=1; j<=newM.Extent(); j++) {
                    if (newM(j).IsPartner(oldM(i))) {
                        shapeMap[i-1].push_back(j-1);
                        break;
                    }
                }
            }
        }
    }
}

void ShapeHistory::join(const ShapeHistory& newH)
{
    ShapeHistory join;

    for (ShapeHistory::MapList::const_iterator it = shapeMap.begin(); it != shapeMap.end(); ++it) {
        int old_shape_index = it->first;
        if (it->second.empty())
            join.shapeMap[old_shape_index] = ShapeHistory::List();
        for (ShapeHistory::List::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
            ShapeHistory::MapList::const_iterator kt = newH.shapeMap.find(*jt);
            if (kt != newH.shapeMap.end()) {
                ShapeHistory::List& ary = join.shapeMap[old_shape_index];
                ary.insert(ary.end(), kt->second.begin(), kt->second.end());
            }
        }
    }

    shapeMap.swap(join.shapeMap);
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

// -------------------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::PropertyShapeCache, App::Property);

App::Property *PropertyShapeCache::Copy(void) const {
    return new PropertyShapeCache();
}

void PropertyShapeCache::Paste(const App::Property &) {
    cache.clear();
}

void PropertyShapeCache::Save (Base::Writer &) const
{
}

void PropertyShapeCache::Restore(Base::XMLReader &)
{
}

/**
 * Make a new python List with a tuple for each cache entry containing the key and the shape
 * @return the python list
 */
PyObject *PropertyShapeCache::getPyObject() {
    Py::List res;
    for(auto &v : cache)
        res.append(Py::TupleN(Py::String(v.first),shape2pyshape(v.second)));
    return Py::new_reference_to(res);
}

/**
 * Remove the cache entries for every element in the list
 * @param value A python list of entry names
 */
void PropertyShapeCache::setPyObject(PyObject *value) {
    if(!value)
        return;
    if(value == Py_None) {
        cache.clear();
        return;
    }
    App::PropertyStringList prop;
    prop.setPyObject(value);
    for(const auto &sub : prop.getValues())
        cache.erase(sub);
}

#define SHAPE_CACHE_NAME "_Part_ShapeCache"
/**
 * Find or create the shape cache for a document object
 * @param obj The document object
 * @param create True if we should create the cache if it doesn't exist
 * @return The shape cache, or null if we aren't creating and it doesn't exist
 */
PropertyShapeCache *PropertyShapeCache::get(const App::DocumentObject *obj, bool create) {
    auto prop = Base::freecad_dynamic_cast<PropertyShapeCache>(
        obj->getDynamicPropertyByName(SHAPE_CACHE_NAME));
    if(prop && prop->getContainer()==obj)
        return prop;
    if(!create)
        return 0;

    prop = static_cast<PropertyShapeCache*>(
        const_cast<App::DocumentObject*>(obj)->addDynamicProperty("Part::PropertyShapeCache",
                                                                  SHAPE_CACHE_NAME,"Part","Shape cache",
                                                                  App::Prop_NoPersist|App::Prop_Output|App::Prop_Hidden));
    if(!prop)
        FC_ERR("Failed to add shape cache for " << obj->getFullName());
    else
        prop->connChanged = const_cast<App::DocumentObject*>(obj)->signalEarlyChanged.connect(
            std::bind(&PropertyShapeCache::slotChanged,prop,sp::_1,sp::_2));
    return prop;
}

/**
 * Look up and return a shape in the cache
 * @param obj The document object to look in
 * @param shape The found shape is returned here
 * @param subname The key to look up
 * @return True if the name was found
 */
bool PropertyShapeCache::getShape(const App::DocumentObject *obj, TopoShape &shape, const char *subname) {
// March, 2024 Toponaming project:  There was originally a feature to disable shape cache
// that has not been kept:
//    if (PartParams::getDisableShapeCache())
//        return false;
    auto prop = get(obj,false);
    if(!prop)
        return false;
    if(!subname) subname = "";
    auto it = prop->cache.find(subname);
    if(it!=prop->cache.end()) {
        shape = it->second;
        return !shape.isNull();
    }
    return false;
}

/**
 * Find or create the property shape cache in a document object and then add an entry
 * @param obj The Object
 * @param shape The shape to cache
 * @param subname The key to point at that shape
 */
void PropertyShapeCache::setShape(
    const App::DocumentObject *obj, const TopoShape &shape, const char *subname)
{
// March, 2024 Toponaming project:  There was originally a feature to disable shape cache
// that has not been kept:
//    if (PartParams::getDisableShapeCache())
//        return;
    auto prop = get(obj,true);
    if(!prop)
        return;
    if(!subname) subname = "";
    prop->cache[subname] = shape;
}

void PropertyShapeCache::slotChanged(const App::DocumentObject &, const App::Property &prop) {
    auto propName = prop.getName();
    if(!propName) return;
    if(strcmp(propName,"Group")==0 ||
        strcmp(propName,"Shape")==0 ||
        strstr(propName,"Touched")!=0)
    {
        FC_LOG("clear shape cache on changed " << prop.getFullName());
        cache.clear();
    }
}

