// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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


#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Writer.h>
#include <Base/Uuid.h>
#include <Base/Tools.h>

#include "PropertyFile.h"
#include "Document.h"
#include "DocumentObject.h"
#include "PropertyContainer.h"


using namespace App;
using namespace Base;
using namespace std;


//**************************************************************************
// PropertyFileIncluded
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFileIncluded, App::Property)


PropertyFileIncluded::PropertyFileIncluded() = default;

PropertyFileIncluded::~PropertyFileIncluded()
{
    // clean up
    if (!_cValue.empty()) {
        Base::FileInfo file(_cValue.c_str());
        file.setPermissions(Base::FileInfo::ReadWrite);
        file.deleteFile();
    }
}

void PropertyFileIncluded::aboutToSetValue()
{
    // This is a trick to check in Copy() if it is called
    // directly from outside or by the Undo/Redo mechanism.
    // In the latter case it is sufficient to rename the file
    // because another file will be assigned afterwards.
    // If Copy() is directly called (e.g. to copy the file to
    // another document) a copy of the file needs to be created.
    // This copy will be deleted again in the class destructor.
    this->StatusBits.set(10);
    Property::aboutToSetValue();
    this->StatusBits.reset(10);
}

std::string PropertyFileIncluded::getDocTransientPath() const
{
    std::string path;
    PropertyContainer* co = getContainer();
    if (co->isDerivedFrom<DocumentObject>()) {
        path = static_cast<DocumentObject*>(co)->getDocument()->TransientDir.getValue();
        std::replace(path.begin(), path.end(), '\\', '/');
    }
    return path;
}

std::string PropertyFileIncluded::getUniqueFileName(const std::string& path,
                                                    const std::string& filename) const
{
    Base::Uuid uuid;
    Base::FileInfo fi(path + "/" + filename);
    while (fi.exists()) {
        fi.setFile(path + "/" + filename + "." + uuid.getValue());
    }

    return fi.filePath();
}

std::string PropertyFileIncluded::getExchangeTempFile() const
{
    return Base::FileInfo::getTempFileName(Base::FileInfo(getValue()).fileName().c_str(),
                                           getDocTransientPath().c_str());
}

std::string PropertyFileIncluded::getOriginalFileName() const
{
    return _OriginalName;
}

void PropertyFileIncluded::setValue(const char* sFile, const char* sName)
{
    if (!Base::Tools::isNullOrEmpty(sFile)) {
        if (_cValue == sFile) {
            throw Base::FileSystemError("Not possible to set the same file!");
        }

        // keep the path to the original file
        _OriginalName = sFile;

        std::string pathTrans = getDocTransientPath();
        Base::FileInfo file(sFile);
        std::string path = file.dirPath();
        if (!file.exists()) {
            std::stringstream str;
            str << "File " << file.filePath() << " does not exist.";
            throw Base::FileSystemError(str.str());
        }

        aboutToSetValue();  // undo/redo by moving the file away with temp name

        // remove old file (if not moved by undo)
        Base::FileInfo value(_cValue);
        std::string pathAct = value.dirPath();
        if (value.exists()) {
            value.setPermissions(Base::FileInfo::ReadWrite);
            value.deleteFile();
        }

        // if a special name given, use this instead
        if (sName) {
            Base::FileInfo fi(pathTrans + "/" + sName);
            if (fi.exists()) {
                // if a file with this name already exists search for a new one
                std::string dir = pathTrans;
                std::string fnp = fi.fileNamePure();
                std::string ext = fi.extension();
                int i = 0;
                do {
                    i++;
                    std::stringstream str;
                    str << dir << "/" << fnp << i;
                    if (!ext.empty()) {
                        str << "." << ext;
                    }
                    fi.setFile(str.str());
                } while (fi.exists());

                _cValue = fi.filePath();
                _BaseFileName = fi.fileName();
            }
            else {
                _cValue = pathTrans + "/" + sName;
                _BaseFileName = sName;
            }
        }
        else if (value.fileName().empty()) {
            _cValue = pathTrans + "/" + file.fileName();
            _BaseFileName = file.fileName();
        }

        // The following applies only on files that are inside the transient
        // directory:
        // When a file is read-only it is supposed to be assigned to a
        // PropertyFileIncluded instance. In this case we must copy the
        // file because otherwise the above instance loses its data.
        // If the file is writable it is supposed to be of free use and
        // it can be simply renamed.

        // if the file is already in transient dir of the document, just use it
        if (path == pathTrans && file.isWritable()) {
            bool done = file.renameFile(_cValue.c_str());
            if (!done) {
                std::stringstream str;
                str << "Cannot rename file " << file.filePath() << " to " << _cValue;
                throw Base::FileSystemError(str.str());
            }

            // make the file read-only
            Base::FileInfo dst(_cValue);
            dst.setPermissions(Base::FileInfo::ReadOnly);
        }
        // otherwise copy from origin location
        else {
            // if file already exists in transient dir make a new unique name
            Base::FileInfo fi(_cValue);
            if (fi.exists()) {
                // if a file with this name already exists search for a new one
                std::string dir = fi.dirPath();
                std::string fnp = fi.fileNamePure();
                std::string ext = fi.extension();
                int i = 0;
                do {
                    i++;
                    std::stringstream str;
                    str << dir << "/" << fnp << i;
                    if (!ext.empty()) {
                        str << "." << ext;
                    }
                    fi.setFile(str.str());
                } while (fi.exists());

                _cValue = fi.filePath();
                _BaseFileName = fi.fileName();
            }

            bool done = file.copyTo(_cValue.c_str());
            if (!done) {
                std::stringstream str;
                str << "Cannot copy file from " << file.filePath() << " to " << _cValue;
                throw Base::FileSystemError(str.str());
            }

            // make the file read-only
            Base::FileInfo dst(_cValue);
            dst.setPermissions(Base::FileInfo::ReadOnly);
        }

        hasSetValue();
    }
}

const char* PropertyFileIncluded::getValue() const
{
    return _cValue.c_str();
}

PyObject* PropertyFileIncluded::getPyObject()
{
    PyObject* p = PyUnicode_DecodeUTF8(_cValue.c_str(), _cValue.size(), nullptr);
    if (!p) {
        throw Base::UnicodeError("PropertyFileIncluded: UTF-8 conversion failure");
    }
    return p;
}

namespace App
{
const char* getNameFromFile(PyObject* value)
{
    const char* string = nullptr;
    PyObject* oname = PyObject_GetAttrString(value, "name");
    if (oname) {
        if (PyUnicode_Check(oname)) {
            string = PyUnicode_AsUTF8(oname);
        }
        else if (PyBytes_Check(oname)) {
            string = PyBytes_AsString(oname);
        }
        Py_DECREF(oname);
    }

    if (!string) {
        throw Base::TypeError("Unable to get filename");
    }
    return string;
}


bool isIOFile(PyObject* file)
{
    PyObject* io = PyImport_ImportModule("io");
    PyObject* IOBase_Class = PyObject_GetAttrString(io, "IOBase");
    bool isFile = PyObject_IsInstance(file, IOBase_Class);
    Py_DECREF(IOBase_Class);
    Py_DECREF(io);
    return isFile;
}
}  // namespace App

void PropertyFileIncluded::setPyObject(PyObject* value)
{
    if (PyUnicode_Check(value)) {
        std::string string = PyUnicode_AsUTF8(value);
        setValue(string.c_str());
    }
    else if (PyBytes_Check(value)) {
        std::string string = PyBytes_AsString(value);
        setValue(string.c_str());
    }
    else if (isIOFile(value)) {
        std::string string = getNameFromFile(value);
        setValue(string.c_str());
    }
    else if (PyTuple_Check(value)) {
        if (PyTuple_Size(value) != 2) {
            throw Base::TypeError("Tuple needs size of (filePath,newFileName)");
        }
        PyObject* file = PyTuple_GetItem(value, 0);
        PyObject* name = PyTuple_GetItem(value, 1);

        // decoding file
        std::string fileStr;
        if (PyUnicode_Check(file)) {
            fileStr = PyUnicode_AsUTF8(file);
        }
        else if (PyBytes_Check(file)) {
            fileStr = PyBytes_AsString(file);
        }
        else if (isIOFile(value)) {
            fileStr = getNameFromFile(file);
        }
        else {
            std::string error = std::string("First item in tuple must be a file or string, not ");
            error += file->ob_type->tp_name;
            throw Base::TypeError(error);
        }

        // decoding name
        std::string nameStr;
        if (PyUnicode_Check(name)) {
            nameStr = PyUnicode_AsUTF8(name);
        }
        else if (PyBytes_Check(name)) {
            nameStr = PyBytes_AsString(name);
        }
        else if (isIOFile(value)) {
            nameStr = getNameFromFile(name);
        }
        else {
            std::string error = std::string("Second item in tuple must be a string, not ");
            error += name->ob_type->tp_name;
            throw Base::TypeError(error);
        }

        setValue(fileStr.c_str(), nameStr.c_str());
    }
    else if (PyDict_Check(value)) {
        Py::Dict dict(value);
        if (dict.hasKey("filter")) {
            setFilter(Py::String(dict.getItem("filter")));
        }
        if (dict.hasKey("filename")) {
            std::string string = static_cast<std::string>(Py::String(dict.getItem("filename")));
            setValue(string.c_str());
        }
    }
    else {
        std::string error = std::string("Type must be string or file, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyFileIncluded::Save(Base::Writer& writer) const
{
    // when saving a document under a new file name the transient directory
    // name changes and thus the stored file name doesn't work any more.
    if (!_cValue.empty() && !Base::FileInfo(_cValue).exists()) {
        Base::FileInfo fi(getDocTransientPath() + "/" + _BaseFileName);
        if (fi.exists()) {
            _cValue = fi.filePath();
        }
    }

    if (writer.isForceXML()) {
        if (!_cValue.empty()) {
            Base::FileInfo file(_cValue.c_str());
            writer.Stream() << writer.ind() << "<FileIncluded data=\"" << file.fileName() << "\">"
                            << std::endl;
            // write the file in the XML stream
            writer.incInd();
            writer.insertBinFile(_cValue.c_str());
            writer.decInd();
            writer.Stream() << writer.ind() << "</FileIncluded>" << endl;
        }
        else {
            writer.Stream() << writer.ind() << "<FileIncluded data=\"\"/>" << std::endl;
        }
    }
    else {
        // instead initiate an extra file
        if (!_cValue.empty()) {
            Base::FileInfo file(_cValue.c_str());
            std::string filename = writer.addFile(file.fileName().c_str(), this);
            filename = encodeAttribute(filename);
            writer.Stream() << writer.ind() << "<FileIncluded file=\"" << filename << "\"/>"
                            << std::endl;
        }
        else {
            writer.Stream() << writer.ind() << "<FileIncluded file=\"\"/>" << std::endl;
        }
    }
}

void PropertyFileIncluded::Restore(Base::XMLReader& reader)
{
    reader.readElement("FileIncluded");
    if (reader.hasAttribute("file")) {
        string file(reader.getAttribute<const char*>("file"));
        if (!file.empty()) {
            // initiate a file read
            reader.addFile(file.c_str(), this);
            // is in the document transient path
            aboutToSetValue();
            _cValue = getDocTransientPath() + "/" + file;
            _BaseFileName = file;
            hasSetValue();
        }
    }
    // section is XML stream
    else if (reader.hasAttribute("data")) {
        string file(reader.getAttribute<const char*>("data"));
        if (!file.empty()) {
            // is in the document transient path
            aboutToSetValue();
            _cValue = getDocTransientPath() + "/" + file;
            reader.readBinFile(_cValue.c_str());
            reader.readEndElement("FileIncluded");
            _BaseFileName = file;
            // set read-only after restoring the file
            Base::FileInfo fi(_cValue.c_str());
            fi.setPermissions(Base::FileInfo::ReadOnly);
            hasSetValue();
        }
    }
}

void PropertyFileIncluded::SaveDocFile(Base::Writer& writer) const
{
    if (_cValue.empty()) {
        Base::Console().warning("Empty filename in PropertyFileIncluded::SaveDocFile. "
                                "Nothing to do.\n");
        return;
    }

    Base::ifstream from(Base::FileInfo(_cValue), std::ios::in | std::ios::binary);
    if (!from) {
        std::stringstream str;
        str << "PropertyFileIncluded::SaveDocFile(): "
            << "File '" << _cValue << "' in transient directory doesn't exist.";
        throw Base::FileSystemError(str.str());
    }

    // copy plain data
    unsigned char c;
    std::ostream& to = writer.Stream();
    while (from.get((char&)c)) {
        to.put((char)c);
    }
}

void PropertyFileIncluded::RestoreDocFile(Base::Reader& reader)
{
    Base::FileInfo fi(_cValue.c_str());
    if (fi.exists() && !fi.isWritable()) {
        // This happens when an object is being restored and tries to reference the
        // same file of another object (e.g. for copy&paste of objects inside the same document).
        return;
    }
    Base::ofstream to(fi, std::ios::out | std::ios::binary);
    if (!to) {
        std::stringstream str;
        str << "PropertyFileIncluded::RestoreDocFile(): "
            << "File '" << _cValue << "' in transient directory cannot be created.";
        throw Base::FileSystemError(str.str());
    }

    // copy plain data
    aboutToSetValue();
    unsigned char c;
    while (reader.get((char&)c)) {
        to.put((char)c);
    }
    to.close();

    // set read-only after restoring the file
    fi.setPermissions(Base::FileInfo::ReadOnly);
    hasSetValue();
}

Property* PropertyFileIncluded::Copy() const
{
    std::unique_ptr<PropertyFileIncluded> prop(new PropertyFileIncluded());

    // remember the base name
    prop->_BaseFileName = _BaseFileName;

    Base::FileInfo file(_cValue);
    if (file.exists()) {
        // create a new name in the document transient directory
        Base::FileInfo newName(getUniqueFileName(file.dirPath(), file.fileName()));
        if (this->StatusBits.test(10)) {
            // rename the file
            bool done = file.renameFile(newName.filePath().c_str());
            if (!done) {
                std::stringstream str;
                str << "PropertyFileIncluded::Copy(): "
                    << "Renaming the file '" << file.filePath() << "' to '" << newName.filePath()
                    << "' failed.";
                throw Base::FileSystemError(str.str());
            }
        }
        else {
            // copy the file
            bool done = file.copyTo(newName.filePath().c_str());
            if (!done) {
                std::stringstream str;
                str << "PropertyFileIncluded::Copy(): "
                    << "Copying the file '" << file.filePath() << "' to '" << newName.filePath()
                    << "' failed.";
                throw Base::FileSystemError(str.str());
            }
        }

        // remember the new name for the Undo
        Base::Console().log("Copy '%s' to '%s'\n", _cValue.c_str(), newName.filePath().c_str());
        prop->_cValue = newName.filePath().c_str();

        // make backup files writable to avoid copying them again on undo/redo
        newName.setPermissions(Base::FileInfo::ReadWrite);
    }

    return prop.release();
}

void PropertyFileIncluded::Paste(const Property& from)
{
    aboutToSetValue();
    const PropertyFileIncluded& prop = dynamic_cast<const PropertyFileIncluded&>(from);
    // make sure that source and destination file are different
    if (_cValue != prop._cValue) {
        // delete old file (if still there)
        Base::FileInfo fi(_cValue);
        fi.setPermissions(Base::FileInfo::ReadWrite);
        fi.deleteFile();

        // get path to destination which can be the transient directory
        // of another document
        std::string pathTrans = getDocTransientPath();
        Base::FileInfo fiSrc(prop._cValue);
        Base::FileInfo fiDst(pathTrans + "/" + prop._BaseFileName);
        std::string path = fiSrc.dirPath();

        if (fiSrc.exists()) {
            fiDst.setFile(getUniqueFileName(fiDst.dirPath(), fiDst.fileName()));

            // if the file is already in transient dir of the document, just use it
            if (path == pathTrans) {
                if (!fiSrc.renameFile(fiDst.filePath().c_str())) {
                    std::stringstream str;
                    str << "PropertyFileIncluded::Paste(): "
                        << "Renaming the file '" << fiSrc.filePath() << "' to '" << fiDst.filePath()
                        << "' failed.";
                    throw Base::FileSystemError(str.str());
                }
            }
            else {
                if (!fiSrc.copyTo(fiDst.filePath().c_str())) {
                    std::stringstream str;
                    str << "PropertyFileIncluded::Paste(): "
                        << "Copying the file '" << fiSrc.filePath() << "' to '" << fiDst.filePath()
                        << "' failed.";
                    throw Base::FileSystemError(str.str());
                }
            }

            // set the file again read-only
            fiDst.setPermissions(Base::FileInfo::ReadOnly);
            _cValue = fiDst.filePath();
        }
        else {
            _cValue.clear();
        }

        // set the base name
        _BaseFileName = prop._BaseFileName;
    }
    hasSetValue();
}

unsigned int PropertyFileIncluded::getMemSize() const
{
    unsigned int mem = Property::getMemSize();
    mem += _cValue.size();
    mem += _BaseFileName.size();
    return mem;
}

void PropertyFileIncluded::setFilter(std::string filter)
{
    m_filter = std::move(filter);
}

std::string PropertyFileIncluded::getFilter() const
{
    return m_filter;
}

//**************************************************************************
// PropertyFile
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFile, App::PropertyString)

PropertyFile::PropertyFile()
{
    m_filter = "";
}

PropertyFile::~PropertyFile() = default;

void PropertyFile::setFilter(const std::string f)
{
    m_filter = f;
}

std::string PropertyFile::getFilter() const
{
    return m_filter;
}

void PropertyFile::setPyObject(PyObject* value)
{
    if (PyDict_Check(value)) {
        Py::Dict dict(value);
        if (dict.hasKey("filter")) {
            setFilter(Py::String(dict.getItem("filter")));
        }
        if (dict.hasKey("filename")) {
            std::string string = static_cast<std::string>(Py::String(dict.getItem("filename")));
            setValue(string.c_str());
        }
    }
    else {
        PropertyString::setPyObject(value);
    }
}
