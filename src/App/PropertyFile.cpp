/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2008                        *
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
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Stream.h>
#include <Base/Console.h>
#include <Base/PyObjectBase.h>

#include "PropertyFile.h"
#include "Document.h"
#include "PropertyContainer.h"
#include "DocumentObject.h"
#define new DEBUG_CLIENTBLOCK
using namespace App;
using namespace Base;
using namespace std;



//**************************************************************************
// PropertyFileIncluded
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFileIncluded , App::Property);

PropertyFileIncluded::PropertyFileIncluded()
{

}

PropertyFileIncluded::~PropertyFileIncluded()
{
    // clean up
    if (!_cValue.empty()) {
        Base::FileInfo file(_cValue.c_str());
        file.deleteFile();
    }
}

std::string PropertyFileIncluded::getDocTransientPath(void) const
{
    PropertyContainer *co = getContainer();
    if (co->isDerivedFrom(DocumentObject::getClassTypeId()))
        return dynamic_cast<DocumentObject*>(co)->getDocument()->TransientDir.getValue();

    return std::string();
}

std::string PropertyFileIncluded::getExchangeTempFile(void) const
{
    return Base::FileInfo::getTempFileName(Base::FileInfo
        (getValue()).fileName().c_str(), getDocTransientPath().c_str());
}

void PropertyFileIncluded::setValue(const char* sFile, const char* sName)
{
    if (sFile) {
        if (_cValue == sFile)
            throw Base::Exception("Not possible to set the same file!");

        std::string pathTrans = getDocTransientPath();
        Base::FileInfo file(sFile);
        std::string path = file.dirPath();
        if (!file.exists()) {
            std::stringstream str;
            str << "File " << file.filePath() << " does not exist.";
            throw Base::Exception(str.str());
        }

        aboutToSetValue(); // undo redo by move the file away with temp name

        // remove old file (if not moved by undo)
        Base::FileInfo value(_cValue);
        std::string pathAct = value.dirPath();
        if (value.exists())
            value.deleteFile();

        // if a special name given, use this instead
        if (sName) {
            Base::FileInfo ExtraName(path + "/" + sName);
            if (ExtraName.exists() ) {
                // if a file with this name already exists search for a new one
                int i=0;
                
                do {
                    i++;
                    std::stringstream str;
                    str << path << "/" << sName << i;
                    ExtraName.setFile(str.str());
                }
                while (ExtraName.exists());
                _cValue = ExtraName.filePath();
                _BaseFileName = ExtraName.fileName();

            }
            else {
                _cValue = path + "/" + sName;
                _BaseFileName = sName;
            }
        }
        else if (value.fileName().empty()) {
            _cValue = pathTrans + "/" + file.fileName();
            _BaseFileName = file.fileName();
        }

        // if the files is already in transient dir of the document, just use it
        if (path == pathTrans) {
            bool done = file.renameFile(_cValue.c_str());
            //assert(done);
            if (!done) {
                std::stringstream str;
                str << "Cannot rename file " << file.filePath() << " to " << _cValue;
                throw Base::Exception(str.str());
            }
        }
        // otherwise copy from origin location 
        else {
            // if file already exists in transient dir make a new unique name
            Base::FileInfo fi(_cValue);
            if (fi.exists()) {
                Base::FileInfo fi2(Base::FileInfo::getTempFileName());
                std::stringstream str;
                str << fi.dirPath() << "/" << fi2.fileNamePure();
                std::string ext = fi.extension(false);
                if (!ext.empty())
                    str << "." << ext;
                Base::FileInfo fi3(str.str());
                _cValue = fi3.filePath();
                _BaseFileName = fi3.fileName();
            }

            bool done = file.copyTo(_cValue.c_str());
            //assert(done); 
            if (!done) {
                std::stringstream str;
                str << "Cannot copy file from " << file.filePath() << " to " << _cValue;
                throw Base::Exception(str.str());
            }
        }

        hasSetValue();
    }
}

const char* PropertyFileIncluded::getValue(void) const
{
     return _cValue.c_str();
}

PyObject *PropertyFileIncluded::getPyObject(void)
{
    PyObject *p = PyUnicode_DecodeUTF8(_cValue.c_str(),_cValue.size(),0);
    if (!p) throw Base::Exception("UTF8 conversion failure at PropertyString::getPyObject()");
    return p;
}

void PropertyFileIncluded::setPyObject(PyObject *value)
{
    std::string string;
    if (PyUnicode_Check(value)) {
        PyObject* unicode = PyUnicode_AsUTF8String(value);
        string = PyString_AsString(unicode);
        Py_DECREF(unicode);
    }
    else if (PyString_Check(value)) {
        string = PyString_AsString(value);
    }
    else if (PyFile_Check(value)) {
        PyObject* FileName = PyFile_Name(value);
        string = PyString_AsString(FileName);
    }
    else if (PyTuple_Check(value)) {
        if (PyTuple_Size(value) != 2)
            throw Py::TypeError("Tuple need size of (filePath,newFileName)"); 
        PyObject* file = PyTuple_GetItem(value,0);
        PyObject* name = PyTuple_GetItem(value,1);

        // decoding file
        std::string fileStr;
        if (PyUnicode_Check(file)) {
            PyObject* unicode = PyUnicode_AsUTF8String(file);
            fileStr = PyString_AsString(unicode);
            Py_DECREF(unicode);
        }
        else if (PyString_Check(file)) {
            fileStr = PyString_AsString(file);
        }
        else if (PyFile_Check(file)) {
            PyObject* FileName = PyFile_Name(file);
            fileStr = PyString_AsString(FileName);
        }
        else {
            std::string error = std::string("first in tuple must be a file or string");
            error += value->ob_type->tp_name;
            throw Py::TypeError(error);
        }

        // decoding name
        std::string nameStr;
        if (PyString_Check(name)) {
            nameStr = PyString_AsString(name);
        }
        else if (PyFile_Check(name)) {
            PyObject* FileName = PyFile_Name(name);
            nameStr = PyString_AsString(FileName);
        }
        else {
            std::string error = std::string("second in tuple must be a string");
            error += value->ob_type->tp_name;
            throw Py::TypeError(error);
        }

        setValue(fileStr.c_str(),nameStr.c_str());
        return;

    }
    else {
        std::string error = std::string("type must be str or file");
        error += value->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    // assign the string
    setValue(string.c_str());
}

void PropertyFileIncluded::Save (Base::Writer &writer) const
{
    if (writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<FileIncluded file=\"\">" << endl;

        // write the file in the XML stream
        if (!_cValue.empty())
            writer.insertBinFile(_cValue.c_str());

        writer.Stream() << writer.ind() <<"</FileIncluded>" << endl ;
    }
    else {
        // instead initiate an extra file 
        if (!_cValue.empty()) {
            Base::FileInfo file(_cValue.c_str());
            writer.Stream() << writer.ind() << "<FileIncluded file=\"" << 
            writer.addFile(file.fileName().c_str(), this) << "\"/>" << std::endl;
        }
        else
            writer.Stream() << writer.ind() << "<FileIncluded file=\"\"/>" << std::endl;
    }
}

void PropertyFileIncluded::Restore(Base::XMLReader &reader)
{
    reader.readElement("FileIncluded");
    string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initate a file read
        reader.addFile(file.c_str(),this);

        // is in the document transient path
        aboutToSetValue();
        _cValue = getDocTransientPath() + "/" + file;
        _BaseFileName = file;
        hasSetValue();
    }
}

void PropertyFileIncluded::SaveDocFile (Base::Writer &writer) const
{
    std::ifstream from(_cValue.c_str());
    if (!from)
        throw Base::Exception("PropertyFileIncluded::SaveDocFile() "
        "File in document transient dir deleted");

    // copy plain data
    unsigned char c;
    std::ostream& to = writer.Stream();
    while (from.get((char&)c)) {
        to.put((const char)c);
    }
}

void PropertyFileIncluded::RestoreDocFile(Base::Reader &reader)
{
    std::ofstream to(_cValue.c_str());
    if (!to) 
        throw Base::Exception("PropertyFileIncluded::RestoreDocFile() "
        "File in document transient dir deleted");

    // copy plain data
    aboutToSetValue();
    unsigned char c;
    while (reader.get((char&)c)) {
        to.put((const char)c);
    }
    to.close();
    hasSetValue();
}

Property *PropertyFileIncluded::Copy(void) const
{
    PropertyFileIncluded *p= new PropertyFileIncluded();

    // remember the base name
    p->_BaseFileName = _BaseFileName;

    if (!_cValue.empty()) {
        Base::FileInfo file(_cValue);

        // create a new name in the document transient directory
        Base::FileInfo NewName(Base::FileInfo::getTempFileName(file.fileName().c_str(),file.dirPath().c_str()));
        NewName.deleteFile();
        // move the file 
        bool done = file.renameFile(NewName.filePath().c_str());
        assert(done);
        // remember the new name for the Undo
        Base::Console().Log("Copy this=%p Befor=%s After=%s\n",p,p->_cValue.c_str(),NewName.filePath().c_str());
        p->_cValue = NewName.filePath().c_str();
    }

    return p;
}

void PropertyFileIncluded::Paste(const Property &from)
{
    aboutToSetValue();
    Base::FileInfo file(_cValue);
    // delete old file (if still there)
    file.deleteFile();
    const PropertyFileIncluded &fileInc = dynamic_cast<const PropertyFileIncluded&>(from);

    if (!fileInc._cValue.empty()) {
        // move the saved files back in place
        Base::FileInfo NewFile(fileInc._cValue);
        _cValue = NewFile.dirPath() + "/" + fileInc._BaseFileName;
        bool done = NewFile.renameFile(_cValue.c_str());
        assert(done);
    }
    else
        _cValue.clear();
    hasSetValue();
}


//**************************************************************************
// PropertyFile
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFile , App::PropertyString);

PropertyFile::PropertyFile()
{

}

PropertyFile::~PropertyFile()
{

}

