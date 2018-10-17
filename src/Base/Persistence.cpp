/***************************************************************************
 *   Copyright (c) Riegel         <juergen.riegel@web.de>                  *
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
#include "Writer.h"
#include "Reader.h"
#include "PyObjectBase.h"

#ifndef _PreComp_
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Persistence.h"

using namespace Base;

TYPESYSTEM_SOURCE_ABSTRACT(Base::Persistence,Base::BaseClass);


//**************************************************************************
// Construction/Destruction



//**************************************************************************
// separator for other implementation aspects

unsigned int Persistence::getMemSize (void) const
{
    // you have to implement this method in all descending classes!
    assert(0);
    return 0;
}

void Persistence::Save (Writer &/*writer*/) const
{
    // you have to implement this method in all descending classes!
    assert(0);
}

void Persistence::Restore(XMLReader &/*reader*/)
{
    // you have to implement this method in all descending classes!
    assert(0);
}

void Persistence::SaveDocFile (Writer &/*writer*/) const
{
}

void Persistence::RestoreDocFile(Reader &/*reader*/)
{
}

std::string Persistence::encodeAttribute(const std::string& str)
{
    std::string tmp;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (*it == '<')
            tmp += "&lt;";
        else if (*it == '\"')
            tmp += "&quot;";
        else if (*it == '\'')
            tmp += "&apos;";
        else if (*it == '&')
            tmp += "&amp;";
        else if (*it == '>')
            tmp += "&gt;";
        else if (*it == '\r')
            tmp += "&#13;";
        else if (*it == '\n')
            tmp += "&#10;";
        else if (*it == '\t')
            tmp += "&#9;";
        else
            tmp += *it;
    }

    return tmp;
}

PyObject* Persistence::dumpToPython(int compression) {
 
    //setup the stream. the in flag is needed to make "read" work^
    std::stringstream stream(std::stringstream::out | std::stringstream::in | std::stringstream::binary);

    //we need to close the zipstream to get a good result, the only way to do this is to delete the ZipWriter. 
    //Hence the scope...
    {
        //create the writer
        Base::ZipWriter writer(stream);
        writer.setLevel(compression);
        writer.putNextEntry("Document.xml");
        writer.setMode("BinaryBrep");    
        
        //save the content (we need to encapsulte it with xml tags to be able to read single element xmls like happen for properties)
        writer.Stream() << "<Content>" << std::endl;
        Save(writer);
        writer.Stream() << "</Content>";
        writer.writeFiles();
    }
     
    //build the byte array with correct size
    if(!stream.seekp(0, stream.end)) {
        PyErr_SetString(PyExc_IOError, "Unable to find end of stream");
        return NULL;
    }        
    std::stringstream::pos_type offset = stream.tellp();
    if(!stream.seekg(0, stream.beg)) {
        PyErr_SetString(PyExc_IOError, "Unable to find begin of stream");
        return NULL;
    }
    
    PyObject* ba = PyByteArray_FromStringAndSize(NULL, offset);
    
    //use the buffer protocol to access the underlying array and write into it
    Py_buffer buf = Py_buffer();
    PyObject_GetBuffer(ba, &buf, PyBUF_WRITABLE);
    try {
        if(!stream.read((char*)buf.buf, offset)) {
            PyErr_SetString(PyExc_IOError, "Error copying data into byte array");
            return NULL;
        }
        PyBuffer_Release(&buf);
    }
    catch(...) {
        PyBuffer_Release(&buf);
        PyErr_SetString(PyExc_IOError, "Error copying data into byte array");
        return NULL;
    }
    
    return ba;    
}

PyObject* Persistence::restoreFromPython(PyObject *buffer) {

    //check if it really is a buffer
    if( !PyObject_CheckBuffer(buffer) ) {
        PyErr_SetString(PyExc_TypeError, "Must be a buffer object");
        return NULL;
    }
    
    Py_buffer buf;
    if(PyObject_GetBuffer(buffer, &buf, PyBUF_SIMPLE) < 0)
        return NULL;
    
    if(!PyBuffer_IsContiguous(&buf, 'C')) {
        PyErr_SetString(PyExc_TypeError, "Buffer must be contiguous");
        return NULL;
    }
    
    try {
        
        //TODO: this mkes a stupid copy, we should make a stream directly from the buffer
        std::stringstream stream(std::string((char*)buf.buf, buf.len), std::stringstream::in | std::stringstream::binary);
        
        zipios::ZipInputStream zipstream(stream);
        Base::XMLReader reader("", zipstream);

        if (!reader.isValid()) {
            PyErr_SetString(PyExc_IOError, "Unable to read file");
            return NULL;
        }
        
        reader.readElement("Content");
        Restore(reader);
        reader.readFiles(zipstream);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    } 
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }   
    
    return Py_None;
}
