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
#include <Base/FileInfo.h>

// inclusion of the generated files (generated out of MetadataPy.xml)
#include "MetadataPy.h"
#include "MetadataPy.cpp"

using namespace Base;
XERCES_CPP_NAMESPACE_USE

// Returns a string which represents the object e.g. when printed in Python
std::string MetadataPy::representation(void) const
{
    MetadataPy::PointerType ptr = getMetadataPtr();
    std::stringstream str;
    str << "Metadata [Name=(";
    str << ptr->name();
    str << "), Description=(";
    str << ptr->description();
    if (!ptr->maintainer().empty()) {
        str << "), Maintainer=(";
        str << ptr->maintainer().front().name;
    }
    str << ")]";

    return str.str();
}

PyObject* MetadataPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    return new MetadataPy(nullptr);
}

// constructor method
int MetadataPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        setTwinPointer(new Metadata());
        return 0;
    }

    // Main class constructor -- takes a file path, loads the metadata from it
    PyErr_Clear();
    char* filename;
    if (PyArg_ParseTuple(args, "et", "utf-8", &filename)) {
        try {
            std::string utf8Name = std::string(filename);
            PyMem_Free(filename);

            auto md = new Metadata(Base::FileInfo::stringToPath(utf8Name));
            setTwinPointer(md);
            return 0;
        }
        catch (const Base::XMLBaseException& e) {
            e.setPyException();
            return -1;
        }
        catch (const XMLException& toCatch) {
            char* message = XMLString::transcode(toCatch.getMessage());
            std::string what = message;
            XMLString::release(&message);
            PyErr_SetString(Base::PyExc_FC_XMLBaseException, what.c_str());
            return -1;
        }
        catch (const DOMException& toCatch) {
            char* message = XMLString::transcode(toCatch.getMessage());
            std::string what = message;
            XMLString::release(&message);
            PyErr_SetString(Base::PyExc_FC_XMLBaseException, what.c_str());
            return -1;
        }
        catch (...) {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Failed to create Metadata object");
            return -1;
        }
    }

    // Copy constructor
    PyErr_Clear();
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(App::MetadataPy::Type), &o)) {
        App::Metadata* a = static_cast<App::MetadataPy*>(o)->getMetadataPtr();
        setTwinPointer(new Metadata(*a));
        return 0;
    }

    PyErr_SetString(Base::PyExc_FC_GeneralError, "metadata object or path to metadata file expected");
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
        Py::Dict pyMaintainer;
        pyMaintainer["name"] = Py::String(m.name);
        pyMaintainer["email"] = Py::String(m.email);
        pyMaintainers.append(pyMaintainer);
    }
    return pyMaintainers;
}

Py::Object MetadataPy::getAuthor(void) const
{
    auto authors = getMetadataPtr()->author();
    Py::List pyAuthors;
    for (const auto& a : authors) {
        Py::Dict pyAuthor;
        pyAuthor["name"] = Py::String(a.name);
        pyAuthor["email"] = Py::String(a.email);
        pyAuthors.append(pyAuthor);
    }
    return pyAuthors;
}

Py::Object MetadataPy::getLicense(void) const
{
    auto licenses = getMetadataPtr()->license();
    Py::List pyLicenses;
    for (const auto& lic : licenses) {
        Py::Dict pyLicense;
        pyLicense["name"] = Py::String(lic.name);
        pyLicense["file"] = Py::String(lic.file.string());
        pyLicenses.append(pyLicense);
    }
    return pyLicenses;
}

Py::Object MetadataPy::getUrls(void) const
{
    auto urls = getMetadataPtr()->url ();
    Py::List pyUrls;
    for (const auto& url : urls) {
        Py::Dict pyUrl;
        pyUrl["location"] = Py::String(url.location);
        switch (url.type) {
        case Meta::UrlType::website: pyUrl["type"] = Py::String("website"); break;
        case Meta::UrlType::repository: pyUrl["type"] = Py::String("repository"); break;
        case Meta::UrlType::bugtracker: pyUrl["type"] = Py::String("bugtracker"); break;
        case Meta::UrlType::readme: pyUrl["type"] = Py::String("readme"); break;
        case Meta::UrlType::documentation: pyUrl["type"] = Py::String("documentation"); break;
        }
        if (url.type == Meta::UrlType::repository)
            pyUrl["branch"] = Py::String(url.branch);
        pyUrls.append(pyUrl);
    }
    return pyUrls;
}

Py::Object dependencyToPyObject(const Meta::Dependency& d)
{
    Py::Dict pyDependency;
    pyDependency["package"] = Py::String(d.package);
    pyDependency["version_lt"] = Py::String(d.version_lt);
    pyDependency["version_lte"] = Py::String(d.version_lte);
    pyDependency["version_eq"] = Py::String(d.version_eq);
    pyDependency["version_gt"] = Py::String(d.version_gt);
    pyDependency["version_gte"] = Py::String(d.version_gte);
    pyDependency["condition"] = Py::String(d.condition);
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

Py::Object MetadataPy::getSubdirectory(void) const
{
    return Py::String(getMetadataPtr()->subdirectory().string());
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
        for (auto & element = elements.first; element != elements.second; ++element) {
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
    if (!PyArg_ParseTuple(args, "s", &name))
        return nullptr;

    auto gm = (*getMetadataPtr())[name];
    Py::List pyGenericMetadata;
    for (const auto& item : gm) {
        Py::Dict pyItem;
        pyItem["contents"] = Py::String(item.contents);
        Py::Dict pyAttributes;
        for (const auto& attribute : item.attributes) {
            pyAttributes[attribute.first] = Py::String(attribute.second);
        }
        pyItem["attributes"] = pyAttributes;
        pyGenericMetadata.append(pyItem);
    }
    return Py::new_reference_to(pyGenericMetadata);
}

Py::Object MetadataPy::getFreeCADMin() const
{
    return Py::String(getMetadataPtr()->freecadmin().str());
}

void MetadataPy::setFreeCADMin(Py::Object args)
{
    char* version = nullptr;
    PyObject* p = args.ptr();
    if (!PyArg_ParseTuple(p, "s", &version))
        return;

    getMetadataPtr()->setFreeCADMin(App::Meta::Version(version));
}

Py::Object MetadataPy::getFreeCADMax() const
{
    return Py::String(getMetadataPtr()->freecadmax().str());
}

void MetadataPy::setFreeCADMax(Py::Object args)
{
    char* version = nullptr;
    PyObject* p = args.ptr();
    if (!PyArg_ParseTuple(p, "s", &version))
        return;

    getMetadataPtr()->setFreeCADMax(App::Meta::Version(version));
}

PyObject* MetadataPy::getFirstSupportedFreeCADVersion(PyObject* p)
{
    if (!PyArg_ParseTuple(p, ""))
        return nullptr;

    // Short-circuit: if the toplevel sets a version, then the lower-levels are overridden
    if (getMetadataPtr()->freecadmin() != App::Meta::Version())
        return Py::new_reference_to(Py::String(getMetadataPtr()->freecadmin().str()));

    auto content = getMetadataPtr()->content();
    auto result = App::Meta::Version();
    for (const auto& item : content) {
        auto minVersion = item.second.freecadmin();
        if (minVersion != App::Meta::Version())
            if (result == App::Meta::Version() || minVersion < result)
                result = minVersion;
    }
    if (result != App::Meta::Version()) {
        return Py::new_reference_to(Py::String(result.str()));
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* MetadataPy::getLastSupportedFreeCADVersion(PyObject* p)
{
    if (!PyArg_ParseTuple(p, ""))
        return nullptr;

    // Short-circuit: if the toplevel sets a version, then the lower-levels are overridden
    if (getMetadataPtr()->freecadmax() != App::Meta::Version())
        return Py::new_reference_to(Py::String(getMetadataPtr()->freecadmax().str()));

    auto content = getMetadataPtr()->content();
    auto result = App::Meta::Version();
    for (const auto& item : content) {
        auto maxVersion = item.second.freecadmax();
        if (maxVersion != App::Meta::Version())
            if (result == App::Meta::Version() || maxVersion > result)
                result = maxVersion;
    }
    if (result != App::Meta::Version()) {
        return Py::new_reference_to(Py::String(result.str()));
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* MetadataPy::supportsCurrentFreeCAD(PyObject* p)
{
    if (!PyArg_ParseTuple(p, ""))
        return nullptr;

    bool result = getMetadataPtr()->supportsCurrentFreeCAD();
    return Py::new_reference_to(Py::Boolean(result));
}

PyObject* MetadataPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MetadataPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
