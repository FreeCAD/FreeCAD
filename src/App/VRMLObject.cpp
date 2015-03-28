/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#endif

#include "VRMLObject.h"
#include "Document.h"
#include "DocumentObjectPy.h"
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

using namespace App;

PROPERTY_SOURCE(App::VRMLObject, App::GeoFeature)


VRMLObject::VRMLObject() 
{
    ADD_PROPERTY_TYPE(VrmlFile,(0),"",Prop_None,"Included file with the VRML definition");
    ADD_PROPERTY_TYPE(Urls,(""),"",static_cast<PropertyType>(Prop_ReadOnly|Prop_Output),"Included file with the VRML definition");
    Urls.setSize(0);
}

VRMLObject::~VRMLObject()
{
}

short VRMLObject::mustExecute(void) const
{
    return 0;
}

PyObject *VRMLObject::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

void VRMLObject::Save (Base::Writer &writer) const
{
    App::GeoFeature::Save(writer);

    // if the VRML file has some inline files then store them inside the project file
    if (Urls.getSize() > 0) {
        const std::vector<std::string>& urls = Urls.getValues();
        writer.incInd();
        writer.Stream() << writer.ind() << "<UrlList count=\"" << urls.size() <<"\">" << std::endl;
        writer.incInd();
        std::string intname = this->getNameInDocument();
        for (std::vector<std::string>::const_iterator it = urls.begin(); it != urls.end(); ++it) {
            Base::FileInfo fi(*it);
            // make sure to put the VRML files into a sub-directory
            std::string output = intname + "/" + fi.fileName();
            output = writer.addFile(output.c_str(), this);
            writer.Stream() << writer.ind() << "<Url value=\"" << output <<"\"/>" << std::endl;
        }
        writer.decInd();
        writer.Stream() << writer.ind() << "</UrlList>" << std::endl;
        writer.decInd();
    }

    this->index = 0;
}

void VRMLObject::Restore(Base::XMLReader &reader)
{
    App::GeoFeature::Restore(reader);

    // are there inline files
    if (Urls.getSize() > 0) {
        reader.readElement("UrlList");
        int count = reader.getAttributeAsInteger("count");
        for(int i = 0; i < count; i++) {
            reader.readElement("Url");
            std::string value = reader.getAttribute("value");
            reader.addFile(value.c_str(), this);
        }

        reader.readEndElement("UrlList");
    }

    this->index = 0;
}

void VRMLObject::SaveDocFile (Base::Writer &writer) const
{
    // store the inline files of the VRML file
    if (this->index < Urls.getSize()) {
        std::string url = Urls[this->index];
        this->index++;

        Base::FileInfo fi(url);
        Base::ifstream file(fi, std::ios::in | std::ios::binary);
        if (file) {
            writer.Stream() << file.rdbuf();
        }
    }
}

void VRMLObject::RestoreDocFile(Base::Reader &reader)
{
    if (this->index < Urls.getSize()) {
        std::string path = getDocument()->TransientDir.getValue();
        std::string url = Urls[this->index];
        std::string intname = this->getNameInDocument();

        Base::FileInfo fi(url);
        std::string subdir = path + "/" + intname;
        Base::FileInfo(subdir).createDirectory();
        url = subdir + "/" + fi.fileName();
        fi.setFile(url);
        Urls.set1Value(this->index, url);
        this->index++;

        Base::ofstream file(fi, std::ios::out | std::ios::binary);
        if (file) {
            reader >> file.rdbuf();
            file.close();
        }

        // after restoring all inline files reload the VRML file
        if (this->index == Urls.getSize()) {
            VrmlFile.touch();
        }
    }
}
