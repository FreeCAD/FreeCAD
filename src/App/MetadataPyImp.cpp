/***************************************************************************
 *   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
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
 *   License along with this library; see the file LICENSE.html. If not,   *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include "Metadata.h"

// inclusion of the generated files (generated out of MetadataPy.xml)
#include "MetadataPy.h"
#include "MetadataPy.cpp"

using namespace Base;

// Returns a string which represents the object e.g. when printed in Python
std::string MetadataPy::representation(void) const
{
    MetadataPy::PointerType ptr = reinterpret_cast<MetadataPy::PointerType>(_pcTwinPointer);
    std::stringstream str;
    str << "Metadata [Name=(";
    str << ptr->name();
    str << "), Description=(";
    str << ptr->description();
    str << "), Maintainer=(";
    str << ptr->maintainer().front().name << ")]";

    return str.str();
}

PyObject* MetadataPy::PyMake(struct _typeobject*, PyObject* args, PyObject*)  // Python wrapper
{
    // create a new instance of MetadataPy and the Twin object 
    const char* filename;
    if (!PyArg_ParseTuple(args, "s!", &filename))
        return nullptr;

    return new MetadataPy(new Metadata(filename));
}

// constructor method
int MetadataPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    // Main class constructor -- takes a file path, loads the metadata from it
    PyErr_Clear();
    const char* file;
    if (PyArg_ParseTuple(args, "s!", &file)) {
        App::Metadata* a = new Metadata(file);
        *(getMetadataPtr()) = *a;
        return 0;
    }

    // Copy constructor
    PyErr_Clear();    
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(App::MetadataPy::Type), &o)) {
        App::Metadata* a = static_cast<App::MetadataPy*>(o)->getMetadataPtr();
        *(getMetadataPtr()) = *a;
        return 0;
    }

    PyErr_SetString(Base::BaseExceptionFreeCADError, "path to metadata file expected");
    return -1;
}

Py::Object MetadataPy::getName(void) const
{
    return Py::String(getMetadataPtr()->name());
}

Py::Object MetadataPy::getVersion(void) const
{
    return Py::String(getMetadataPtr()->version().str());
}

Py::Object MetadataPy::getDescription(void) const
{
    return Py::String(getMetadataPtr()->description());
}

Py::Object MetadataPy::getMaintainer(void) const
{
    auto maintainers = getMetadataPtr()->maintainer();
    Py::List pyMaintainers;
    for (const auto& m : maintainers) {
        Py::Object pyMaintainer;
        pyMaintainer.setAttr("name", Py::String(m.name));
        pyMaintainer.setAttr("email", Py::String(m.email));
        pyMaintainers.append(pyMaintainer);
    }
    return pyMaintainers;
}

Py::Object MetadataPy::getAuthor(void) const
{
    auto authors = getMetadataPtr()->author();
    Py::List pyAuthors;
    for (const auto& a : authors) {
        Py::Object pyAuthor;
        pyAuthor.setAttr("name", Py::String(a.name));
        pyAuthor.setAttr("email", Py::String(a.email));
        pyAuthors.append(pyAuthor);
    }
    return pyAuthors;
}

Py::Object MetadataPy::getLicense(void) const
{
    auto licenses = getMetadataPtr()->license();
    Py::List pyLicenses;
    for (const auto& lic : licenses) {
        Py::Object pyLicense;
        pyLicense.setAttr("name", Py::String(lic.name));
        pyLicense.setAttr("file", Py::String(lic.file.string()));
        pyLicenses.append(pyLicense);
    }
    return pyLicenses;
}

Py::Object MetadataPy::getUrl(void) const
{
    auto urls = getMetadataPtr()->url ();
    Py::List pyUrls;
    for (const auto& url : urls) {
        Py::Object pyUrl;
        pyUrl.setAttr("location", Py::String(url.location));
        switch (url.type) {
        case Meta::UrlType::website: pyUrl.setAttr("type", Py::String("website")); break;
        case Meta::UrlType::repository: pyUrl.setAttr("type", Py::String("repository")); break;
        case Meta::UrlType::bugtracker: pyUrl.setAttr("type", Py::String("bugtracker")); break;
        case Meta::UrlType::readme: pyUrl.setAttr("type", Py::String("readme")); break;
        case Meta::UrlType::documentation: pyUrl.setAttr("type", Py::String("documentation")); break;
        }
        pyUrls.append(pyUrl);
    }
    return pyUrls;
}

Py::Object dependencyToPyObject(const Meta::Dependency& d)
{
    Py::Object pyDependency;
    pyDependency.setAttr("package",Py::String(d.package));
    pyDependency.setAttr("version_lt", Py::String(d.version_lt));
    pyDependency.setAttr("version_lte", Py::String(d.version_lte));
    pyDependency.setAttr("version_eq", Py::String(d.version_eq));
    pyDependency.setAttr("version_gt", Py::String(d.version_gt));
    pyDependency.setAttr("version_gte", Py::String(d.version_gte));
    pyDependency.setAttr("condition", Py::String(d.condition));
    return pyDependency;
}

Py::Object MetadataPy::getDepend(void) const
{
    auto dependencies = getMetadataPtr()->depend();
    Py::List pyDependencies;
    for (const auto& d : dependencies) {
        pyDependencies.append(dependencyToPyObject(d));
    }
    return pyDependencies;
}

Py::Object MetadataPy::getConflict(void) const
{
    auto dependencies = getMetadataPtr()->conflict();
    Py::List pyDependencies;
    for (const auto& d : dependencies) {
        pyDependencies.append(dependencyToPyObject(d));
    }
    return pyDependencies;
}

Py::Object MetadataPy::getReplace(void) const
{
    auto dependencies = getMetadataPtr()->replace();
    Py::List pyDependencies;
    for (const auto& d : dependencies) {
        pyDependencies.append(dependencyToPyObject(d));
    }
    return pyDependencies;
}

// Tag, icon, classname, file

Py::Object MetadataPy::getTag(void) const
{
    auto tags = getMetadataPtr()->tag();
    Py::List pyTags;
    for (const auto& t : tags) {
        pyTags.append(Py::String(t));
    }
    return pyTags;
}

Py::Object MetadataPy::getIcon(void) const
{
    return Py::String(getMetadataPtr()->icon().string());
}

Py::Object MetadataPy::getClassname(void) const
{
    return Py::String(getMetadataPtr()->classname());
}

Py::Object MetadataPy::getFile(void) const
{
    auto files = getMetadataPtr()->file();
    Py::List pyFiles;
    for (const auto& f : files) {
        pyFiles.append(Py::String(f.string()));
    }
    return pyFiles;
}

Py::Object MetadataPy::getContent(void) const
{
    auto content = getMetadataPtr()->content();
    std::set<std::string> keys;
    for (const auto& item : content) {
        keys.insert(item.first);
    }

    // For the Python, we'll use a dictionary of lists to store the content components:
    Py::Dict pyContent;
    for (const auto& key : keys) {
        Py::List pyContentForKey;
        auto elements = content.equal_range(key);
        for (auto element = elements.first; element != elements.second; ++element) {
            auto contentMetadataItem = new MetadataPy(new Metadata(element->second));
            pyContentForKey.append(Py::asObject(contentMetadataItem));
        }
        pyContent[key] = pyContentForKey;
    }
    return pyContent;
}

PyObject* MetadataPy::getGenericMetadata(PyObject* args)
{
    const char* name;
    if (!PyArg_ParseTuple(args, "s!", &name))
        return NULL;
    auto gm = (*getMetadataPtr())[name];
    auto pyGenericMetadata = new Py::List;
    for (const auto& item : gm) {
        Py::Object pyItem;
        pyItem.setAttr("contents", Py::String(item.contents));
        Py::Dict pyAttributes;
        for (const auto& attribute : item.attributes) {
            pyAttributes[attribute.first] = Py::String(attribute.second);
        }
        pyItem.setAttr("attributes", pyAttributes);
        pyGenericMetadata->append(pyItem);
    }
    return pyGenericMetadata->ptr();
}

PyObject* MetadataPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int MetadataPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}