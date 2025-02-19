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

#include <Base/PyObjectBase.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Writer.h>

#include "MaterialPy.h"
#include "PropertyMaterial.h"

namespace App {

TYPESYSTEM_SOURCE(App::PropertyMaterial, App::Property)

PropertyMaterial::PropertyMaterial() = default;

PropertyMaterial::~PropertyMaterial() = default;

void PropertyMaterial::setValue(const Material& mat)
{
    aboutToSetValue();
    _cMat = mat;
    hasSetValue();
}

void PropertyMaterial::setValue(const Color& col)
{
    setDiffuseColor(col);
}

void PropertyMaterial::setValue(float r, float g, float b, float a)
{
    setDiffuseColor(r, g, b, a);
}

void PropertyMaterial::setValue(uint32_t rgba)
{
    setDiffuseColor(rgba);
}

const Material& PropertyMaterial::getValue() const
{
    return _cMat;
}

void PropertyMaterial::setAmbientColor(const Color& col)
{
    aboutToSetValue();
    _cMat.ambientColor = col;
    hasSetValue();
}

void PropertyMaterial::setAmbientColor(float r, float g, float b, float a)
{
    aboutToSetValue();
    _cMat.ambientColor.set(r, g, b, a);
    hasSetValue();
}

void PropertyMaterial::setAmbientColor(uint32_t rgba)
{
    aboutToSetValue();
    _cMat.ambientColor.setPackedValue(rgba);
    hasSetValue();
}

void PropertyMaterial::setDiffuseColor(const Color& col)
{
    aboutToSetValue();
    _cMat.diffuseColor = col;
    hasSetValue();
}

void PropertyMaterial::setDiffuseColor(float r, float g, float b, float a)
{
    aboutToSetValue();
    _cMat.diffuseColor.set(r, g, b, a);
    hasSetValue();
}

void PropertyMaterial::setDiffuseColor(uint32_t rgba)
{
    aboutToSetValue();
    _cMat.diffuseColor.setPackedValue(rgba);
    hasSetValue();
}

void PropertyMaterial::setSpecularColor(const Color& col)
{
    aboutToSetValue();
    _cMat.specularColor = col;
    hasSetValue();
}

void PropertyMaterial::setSpecularColor(float r, float g, float b, float a)
{
    aboutToSetValue();
    _cMat.specularColor.set(r, g, b, a);
    hasSetValue();
}

void PropertyMaterial::setSpecularColor(uint32_t rgba)
{
    aboutToSetValue();
    _cMat.specularColor.setPackedValue(rgba);
    hasSetValue();
}

void PropertyMaterial::setEmissiveColor(const Color& col)
{
    aboutToSetValue();
    _cMat.emissiveColor = col;
    hasSetValue();
}

void PropertyMaterial::setEmissiveColor(float r, float g, float b, float a)
{
    aboutToSetValue();
    _cMat.emissiveColor.set(r, g, b, a);
    hasSetValue();
}

void PropertyMaterial::setEmissiveColor(uint32_t rgba)
{
    aboutToSetValue();
    _cMat.emissiveColor.setPackedValue(rgba);
    hasSetValue();
}

void PropertyMaterial::setShininess(float val)
{
    aboutToSetValue();
    _cMat.shininess = val;
    hasSetValue();
}

void PropertyMaterial::setTransparency(float val)
{
    aboutToSetValue();
    _cMat.transparency = val;
    hasSetValue();
}

const Color& PropertyMaterial::getAmbientColor() const
{
    return _cMat.ambientColor;
}

const Color& PropertyMaterial::getDiffuseColor() const
{
    return _cMat.diffuseColor;
}

const Color& PropertyMaterial::getSpecularColor() const
{
    return _cMat.specularColor;
}

const Color& PropertyMaterial::getEmissiveColor() const
{
    return _cMat.emissiveColor;
}

double PropertyMaterial::getShininess() const
{
    return _cMat.shininess;
}

double PropertyMaterial::getTransparency() const
{
    return _cMat.transparency;
}

PyObject* PropertyMaterial::getPyObject()
{
    return new MaterialPy(new Material(_cMat));
}

void PropertyMaterial::setPyObject(PyObject* value)
{
    if (PyObject_TypeCheck(value, &(MaterialPy::Type))) {
        setValue(*static_cast<MaterialPy*>(value)->getMaterialPtr());
    }
    else {
        setValue(MaterialPy::toColor(value));
    }
}

void PropertyMaterial::Save(Base::Writer& writer) const
{
    // clang-format off
    writer.Stream() << writer.ind()
                    << "<PropertyMaterial ambientColor=\"" << _cMat.ambientColor.getPackedValue()
                    << "\" diffuseColor=\"" << _cMat.diffuseColor.getPackedValue()
                    << "\" specularColor=\"" << _cMat.specularColor.getPackedValue()
                    << "\" emissiveColor=\"" << _cMat.emissiveColor.getPackedValue()
                    << "\" shininess=\"" << _cMat.shininess
                    << "\" transparency=\"" << _cMat.transparency
                    << "\" image=\"" << _cMat.image
                    << "\" imagePath=\"" << _cMat.imagePath
                    << "\" uuid=\"" << _cMat.uuid
                    << "\"/>" << std::endl;
    // clang-format on
}

void PropertyMaterial::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("PropertyMaterial");
    // get the value of my Attribute
    aboutToSetValue();
    _cMat.ambientColor.setPackedValue(reader.getAttributeAsUnsigned("ambientColor"));
    _cMat.diffuseColor.setPackedValue(reader.getAttributeAsUnsigned("diffuseColor"));
    _cMat.specularColor.setPackedValue(reader.getAttributeAsUnsigned("specularColor"));
    _cMat.emissiveColor.setPackedValue(reader.getAttributeAsUnsigned("emissiveColor"));
    _cMat.shininess = (float)reader.getAttributeAsFloat("shininess");
    _cMat.transparency = (float)reader.getAttributeAsFloat("transparency");
    if (reader.hasAttribute("image")) {
        _cMat.image = reader.getAttribute("image");
    }
    if (reader.hasAttribute("imagePath")) {
        _cMat.imagePath = reader.getAttribute("imagePath");
    }
    if (reader.hasAttribute("uuid")) {
        _cMat.uuid = reader.getAttribute("uuid");
    }
    hasSetValue();
}

const char* PropertyMaterial::getEditorName() const
{
    if (testStatus(MaterialEdit)) {
        return "Gui::PropertyEditor::PropertyMaterialItem";
    }
    return "";
}

Property* PropertyMaterial::Copy() const
{
    PropertyMaterial* p = new PropertyMaterial();
    p->_cMat = _cMat;
    return p;
}

void PropertyMaterial::Paste(const Property& from)
{
    aboutToSetValue();
    _cMat = dynamic_cast<const PropertyMaterial&>(from)._cMat;
    hasSetValue();
}

//**************************************************************************
// PropertyMaterialList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMaterialList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyMaterialList::PropertyMaterialList()
{
    setMinimumSizeOne();
}

PropertyMaterialList::~PropertyMaterialList() = default;

//**************************************************************************
// Base class implementer

void PropertyMaterialList::setValues(const std::vector<App::Material>& newValues)
{
    if (!newValues.empty()) {
        PropertyListsT<Material>::setValues(newValues);
    }
    else {
        aboutToSetValue();
        setSize(1);
        hasSetValue();
    }
}

PyObject* PropertyMaterialList::getPyObject()
{
    Py::Tuple tuple(getSize());

    for (int i = 0; i < getSize(); i++) {
        tuple.setItem(i, Py::asObject(new MaterialPy(new Material(_lValueList[i]))));
    }

    return Py::new_reference_to(tuple);
}

void PropertyMaterialList::verifyIndex(int index) const
{
    int size = getSize();
    if (index < -1 || index > size) {
        throw Base::RuntimeError("index out of bound");
    }
}

void PropertyMaterialList::setMinimumSizeOne()
{
    int size = getSize();
    if (size < 1) {
        setSize(1);
    }
}

int PropertyMaterialList::resizeByOneIfNeeded(int index)
{
    int size = getSize();
    if (index == -1 || index == size) {
        index = size;
        setSize(size + 1);
    }

    return index;
}

void PropertyMaterialList::setValue()
{
    Material empty;
    setValue(empty);
}

void PropertyMaterialList::setValue(const Material& mat)
{
    aboutToSetValue();
    setSize(1);
    for (auto& material : _lValueList) {
        material = mat;
    }
    hasSetValue();
}

void PropertyMaterialList::setValue(int index, const Material& mat)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index] = mat;
    hasSetValue();
}

void PropertyMaterialList::setAmbientColor(const Color& col)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.ambientColor = col;
    }
    hasSetValue();
}

void PropertyMaterialList::setAmbientColor(float r, float g, float b, float a)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.ambientColor.set(r, g, b, a);
    }
    hasSetValue();
}

void PropertyMaterialList::setAmbientColor(uint32_t rgba)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.ambientColor.setPackedValue(rgba);
    }
    hasSetValue();
}

void PropertyMaterialList::setAmbientColor(int index, const Color& col)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].ambientColor = col;
    hasSetValue();
}

void PropertyMaterialList::setAmbientColor(int index, float r, float g, float b, float a)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].ambientColor.set(r, g, b, a);
    hasSetValue();
}

void PropertyMaterialList::setAmbientColor(int index, uint32_t rgba)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].ambientColor.setPackedValue(rgba);
    hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(const Color& col)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.diffuseColor = col;
    }
    hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(float r, float g, float b, float a)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.diffuseColor.set(r, g, b, a);
    }
    hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(uint32_t rgba)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.diffuseColor.setPackedValue(rgba);
    }
    hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(int index, const Color& col)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].diffuseColor = col;
    hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(int index, float r, float g, float b, float a)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].diffuseColor.set(r, g, b, a);
    hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(int index, uint32_t rgba)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].diffuseColor.setPackedValue(rgba);
    hasSetValue();
}

void PropertyMaterialList::setDiffuseColors(const std::vector<App::Color>& colors)
{
    aboutToSetValue();
    setSize(colors.size(), _lValueList[0]);

    for (std::size_t i = 0; i < colors.size(); i++) {
        _lValueList[i].diffuseColor = colors[i];
    }
    hasSetValue();
}

void PropertyMaterialList::setSpecularColor(const Color& col)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.specularColor = col;
    }
    hasSetValue();
}

void PropertyMaterialList::setSpecularColor(float r, float g, float b, float a)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.specularColor.set(r, g, b, a);
    }
    hasSetValue();
}

void PropertyMaterialList::setSpecularColor(uint32_t rgba)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.specularColor.setPackedValue(rgba);
    }
    hasSetValue();
}

void PropertyMaterialList::setSpecularColor(int index, const Color& col)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].specularColor = col;
    hasSetValue();
}

void PropertyMaterialList::setSpecularColor(int index, float r, float g, float b, float a)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].specularColor.set(r, g, b, a);
    hasSetValue();
}

void PropertyMaterialList::setSpecularColor(int index, uint32_t rgba)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].specularColor.setPackedValue(rgba);
    hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(const Color& col)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.emissiveColor = col;
    }
    hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(float r, float g, float b, float a)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.emissiveColor.set(r, g, b, a);
    }
    hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(uint32_t rgba)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.emissiveColor.setPackedValue(rgba);
    }
    hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(int index, const Color& col)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].emissiveColor = col;
    hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(int index, float r, float g, float b, float a)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].emissiveColor.set(r, g, b, a);
    hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(int index, uint32_t rgba)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].emissiveColor.setPackedValue(rgba);
    hasSetValue();
}

void PropertyMaterialList::setShininess(float val)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.shininess = val;
    }
    hasSetValue();
}

void PropertyMaterialList::setShininess(int index, float val)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].shininess = val;
    hasSetValue();
}

void PropertyMaterialList::setTransparency(float val)
{
    aboutToSetValue();
    setMinimumSizeOne();
    for (auto& material : _lValueList) {
        material.transparency = val;
    }
    hasSetValue();
}

void PropertyMaterialList::setTransparency(int index, float val)
{
    verifyIndex(index);

    aboutToSetValue();
    index = resizeByOneIfNeeded(index);
    _lValueList[index].transparency = val;
    hasSetValue();
}

void PropertyMaterialList::setTransparencies(const std::vector<float>& transparencies)
{
    aboutToSetValue();
    setSize(transparencies.size(), _lValueList[0]);

    for (std::size_t i = 0; i < transparencies.size(); i++) {
        _lValueList[i].transparency = transparencies[i];
    }
    hasSetValue();
}

const Color& PropertyMaterialList::getAmbientColor() const
{
    return _lValueList[0].ambientColor;
}

const Color& PropertyMaterialList::getAmbientColor(int index) const
{
    return _lValueList[index].ambientColor;
}

const Color& PropertyMaterialList::getDiffuseColor() const
{
    return _lValueList[0].diffuseColor;
}

const Color& PropertyMaterialList::getDiffuseColor(int index) const
{
    return _lValueList[index].diffuseColor;
}

std::vector<App::Color> PropertyMaterialList::getDiffuseColors() const
{
    std::vector<App::Color> list;
    for (auto& material : _lValueList) {
        list.push_back(material.diffuseColor);
    }

    return list;
}

const Color& PropertyMaterialList::getSpecularColor() const
{
    return _lValueList[0].specularColor;
}

const Color& PropertyMaterialList::getSpecularColor(int index) const
{
    return _lValueList[index].specularColor;
}

const Color& PropertyMaterialList::getEmissiveColor() const
{
    return _lValueList[0].emissiveColor;
}

const Color& PropertyMaterialList::getEmissiveColor(int index) const
{
    return _lValueList[index].emissiveColor;
}

float PropertyMaterialList::getShininess() const
{
    return _lValueList[0].shininess;
}

float PropertyMaterialList::getShininess(int index) const
{
    return _lValueList[index].shininess;
}

float PropertyMaterialList::getTransparency() const
{
    return _lValueList[0].transparency;
}

float PropertyMaterialList::getTransparency(int index) const
{
    return _lValueList[index].transparency;
}

std::vector<float> PropertyMaterialList::getTransparencies() const
{
    std::vector<float> list;
    for (auto& material : _lValueList) {
        list.push_back(material.transparency);
    }

    return list;
}

Material PropertyMaterialList::getPyValue(PyObject* value) const
{
    if (PyObject_TypeCheck(value, &(MaterialPy::Type))) {
        return *static_cast<MaterialPy*>(value)->getMaterialPtr();
    }
    else {
        std::string error = std::string("type must be 'Material', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyMaterialList::Save(Base::Writer& writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<MaterialList file=\""
                        << (getSize() ? writer.addFile(getName(), this) : "") << "\""
                        << " version=\"3\"/>" << std::endl;
    }
}

void PropertyMaterialList::Restore(Base::XMLReader& reader)
{
    reader.readElement("MaterialList");
    if (reader.hasAttribute("file")) {
        std::string file(reader.getAttribute("file"));
        if (reader.hasAttribute("version")) {
            formatVersion = static_cast<Format>(reader.getAttributeAsInteger("version"));
        }

        if (!file.empty()) {
            // initiate a file read
            reader.addFile(file.c_str(), this);
        }
    }
}

void PropertyMaterialList::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (const auto& it : _lValueList) {
        str << it.ambientColor.getPackedValue();
        str << it.diffuseColor.getPackedValue();
        str << it.specularColor.getPackedValue();
        str << it.emissiveColor.getPackedValue();
        str << it.shininess;
        str << it.transparency;
    }

    // Apply the latest changes last for backwards compatibility
    for (const auto& it : _lValueList) {
        writeString(str, it.image);
        writeString(str, it.imagePath);
        writeString(str, it.uuid);
    }
}

void PropertyMaterialList::writeString(Base::OutputStream& str, const std::string& value) const
{
    uint32_t uCt = (uint32_t)value.size();
    str << uCt;
    str.write(value.c_str(), uCt);
}

void PropertyMaterialList::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream str(reader);
    if (formatVersion == Version_2) {
        // V2 is same as V0
        uint32_t count = 0;
        str >> count;
        RestoreDocFileV0(count, reader);
    }
    else if (formatVersion == Version_3) {
        // Default to the latest
        RestoreDocFileV3(reader);
    }
    else {
        int32_t version;
        str >> version;
        if (version < 0) {
            // This was a failed attempt at versioning, but is included
            // to support files created during development. In can be removed
            // in a future release once dev files are migrated.
            uint32_t count = 0;
            str >> count;
            RestoreDocFileV0(count, reader);
        }
        else {
            uint32_t uCt = static_cast<uint32_t>(version);
            RestoreDocFileV0(uCt, reader);
        }
    }
}

void PropertyMaterialList::RestoreDocFileV0(uint32_t count, Base::Reader& reader)
{
    Base::InputStream str(reader);
    std::vector<Material> values(count);
    uint32_t value {};  // must be 32 bit long
    float valueF {};
    for (auto& it : values) {
        str >> value;
        it.ambientColor.setPackedValue(value);
        str >> value;
        it.diffuseColor.setPackedValue(value);
        str >> value;
        it.specularColor.setPackedValue(value);
        str >> value;
        it.emissiveColor.setPackedValue(value);
        str >> valueF;
        it.shininess = valueF;
        str >> valueF;
        it.transparency = valueF;
    }
    setValues(values);
}

void PropertyMaterialList::RestoreDocFileV3(Base::Reader& reader)
{
    Base::InputStream str(reader);
    uint32_t count = 0;
    str >> count;
    std::vector<Material> values(count);
    uint32_t value {};  // must be 32 bit long
    float valueF {};
    for (auto& it : values) {
        str >> value;
        it.ambientColor.setPackedValue(value);
        str >> value;
        it.diffuseColor.setPackedValue(value);
        str >> value;
        it.specularColor.setPackedValue(value);
        str >> value;
        it.emissiveColor.setPackedValue(value);
        str >> valueF;
        it.shininess = valueF;
        str >> valueF;
        it.transparency = valueF;
    }
    for (auto& it : values) {
        readString(str, it.image);
        readString(str, it.imagePath);
        readString(str, it.uuid);
    }
    setValues(values);
}

void PropertyMaterialList::readString(Base::InputStream& str, std::string& value)
{
    uint32_t uCt {};
    str >> uCt;

    std::vector<char> temp(uCt);
    str.read(temp.data(), uCt);
    value.assign(temp.data(), temp.size());
}

const char* PropertyMaterialList::getEditorName() const
{
    if (testStatus(NoMaterialListEdit)) {
        return "";
    }
    return "Gui::PropertyEditor::PropertyMaterialListItem";
}

Property* PropertyMaterialList::Copy() const
{
    PropertyMaterialList* p = new PropertyMaterialList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyMaterialList::Paste(const Property& from)
{
    setValues(dynamic_cast<const PropertyMaterialList&>(from)._lValueList);
}

unsigned int PropertyMaterialList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Material));
}

}  // namespace App
