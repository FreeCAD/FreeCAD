/**************************************************************************
*                                                                         *
*   Copyright (c) 2022 FreeCAD Project Association                        *
*                                                                         *
*   This file is part of FreeCAD.                                         *
*                                                                         *
*   FreeCAD is free software: you can redistribute it and/or modify it    *
*   under the terms of the GNU Lesser General Public License as           *
*   published by the Free Software Foundation, either version 2.1 of the  *
*   License, or (at your option) any later version.                       *
*                                                                         *
*   FreeCAD is distributed in the hope that it will be useful, but        *
*   WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
*   Lesser General Public License for more details.                       *
*                                                                         *
*   You should have received a copy of the GNU Lesser General Public      *
*   License along with FreeCAD. If not, see                               *
*   <https://www.gnu.org/licenses/>.                                      *
*                                                                         *
**************************************************************************/

#include "PreCompiled.h"

#include "Metadata.h"
#include <Base/FileInfo.h>

// inclusion of the generated files (generated out of MetadataPy.xml)
#include "MetadataPy.h"
#include "MetadataPy.cpp"

using namespace Base;
XERCES_CPP_NAMESPACE_USE

// Returns a string which represents the object e.g. when printed in Python
std::string MetadataPy::representation() const
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

PyObject *MetadataPy::PyMake(struct _typeobject *, PyObject *, PyObject *)// Python wrapper
{
    return new MetadataPy(nullptr);
}

// constructor method
int MetadataPy::PyInit(PyObject *args, PyObject * /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        setTwinPointer(new Metadata());
        return 0;
    }

    // Data may be passed directly in as a bytes-like object buffer
    PyErr_Clear();
    Py_buffer dataBuffer;
    if (PyArg_ParseTuple(args, "y*", &dataBuffer)) {
        try {
            // NB: This is making a copy of the buffer for simplicity, but that shouldn't be
            // necessary. Use either a string_view or a span to avoid the copy in the future.
            auto md = new Metadata(
                std::string(static_cast<const char*>(dataBuffer.buf), dataBuffer.len)
            );
            setTwinPointer(md);
            return 0;
        }
        catch (const Base::XMLBaseException&) {
            // If the XML read failed, this might have been a path to a file, rather than a
            // bytes-like object. Fall through to the next case.
        }
    }

    // Main class constructor -- takes a file path, loads the metadata from it
    PyErr_Clear();
    char *filename;
    if (PyArg_ParseTuple(args, "et", "utf-8", &filename)) {
        try {
            std::string utf8Name = std::string(filename);
            PyMem_Free(filename);

            auto md = new Metadata(Base::FileInfo::stringToPath(utf8Name));
            setTwinPointer(md);
            return 0;
        }
        catch (const Base::XMLBaseException &e) {
            e.setPyException();
            return -1;
        }
        catch (const XMLException &toCatch) {
            char *message = XMLString::transcode(toCatch.getMessage());
            std::string what = message;
            XMLString::release(&message);
            PyErr_SetString(Base::PyExc_FC_XMLBaseException, what.c_str());
            return -1;
        }
        catch (const DOMException &toCatch) {
            char *message = XMLString::transcode(toCatch.getMessage());
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
    PyObject *o;
    if (PyArg_ParseTuple(args, "O!", &(App::MetadataPy::Type), &o)) {
        App::Metadata *a = static_cast<App::MetadataPy *>(o)->getMetadataPtr();
        setTwinPointer(new Metadata(*a));
        return 0;
    }

    PyErr_SetString(Base::PyExc_FC_GeneralError, "metadata object or path to metadata file expected");
    return -1;
}

Py::Object MetadataPy::getName() const
{
    return Py::String(getMetadataPtr()->name());
}

void MetadataPy::setName(Py::Object args)
{
    const char *name = nullptr;
    if (!PyArg_Parse(args.ptr(), "z", &name)) {
        throw Py::Exception();
    }
    if (name)
        getMetadataPtr()->setName(name);
    else
        getMetadataPtr()->setName("");
}

Py::Object MetadataPy::getVersion() const
{
    return Py::String(getMetadataPtr()->version().str());
}

void MetadataPy::setVersion(Py::Object args)
{
    const char *name = nullptr;
    if (!PyArg_Parse(args.ptr(), "z", &name))
        throw Py::Exception();
    if (name && name[0] != '\0')
        getMetadataPtr()->setVersion(App::Meta::Version(std::string(name)));
    else
        getMetadataPtr()->setVersion(App::Meta::Version());
}

Py::Object MetadataPy::getDate() const
{
    return Py::String(getMetadataPtr()->date());
}

void MetadataPy::setDate(Py::Object args)
{
    const char *date = nullptr;
    if (!PyArg_Parse(args.ptr(), "z", &date))
        throw Py::Exception();
    if (date) getMetadataPtr()->setDate(date);
    else
        getMetadataPtr()->setDate("");
}

Py::Object MetadataPy::getDescription() const
{
    return Py::String(getMetadataPtr()->description());
}

void MetadataPy::setDescription(Py::Object args)
{
    const char *description = nullptr;
    if (!PyArg_Parse(args.ptr(), "s", &description))
        throw Py::Exception();
    getMetadataPtr()->setDescription(description);
}

Py::Object MetadataPy::getType() const
{
    return Py::String(getMetadataPtr()->type());
}

void MetadataPy::setType(Py::Object args)
{
    const char *type = nullptr;
    if (!PyArg_Parse(args.ptr(), "s", &type))
        throw Py::Exception();
    getMetadataPtr()->setType(type);
}

Py::Object MetadataPy::getMaintainer() const
{
    auto maintainers = getMetadataPtr()->maintainer();
    Py::List pyMaintainers;
    for (const auto &m : maintainers) {
        Py::Dict pyMaintainer;
        pyMaintainer["name"] = Py::String(m.name);
        pyMaintainer["email"] = Py::String(m.email);
        pyMaintainers.append(pyMaintainer);
    }
    return pyMaintainers;
}

void MetadataPy::setMaintainer(Py::Object args)
{
    PyObject *list = nullptr;
    if (!PyArg_Parse(args.ptr(), "O!", &PyList_Type, &list))
        throw Py::Exception();

    getMetadataPtr()->clearMaintainer();
    Py::List maintainers(list);
    for (const auto &m : maintainers) {
        Py::Dict pyMaintainer(m);
        std::string name = pyMaintainer["name"].str();
        std::string email = pyMaintainer["email"].str();
        getMetadataPtr()->addMaintainer(App::Meta::Contact(name, email));
    }
}

PyObject *MetadataPy::addMaintainer(PyObject *args)
{
    const char *name = nullptr;
    const char *email = nullptr;
    if (!PyArg_ParseTuple(args, "ss", &name, &email))
        throw Py::Exception();
    getMetadataPtr()->addMaintainer(App::Meta::Contact(name, email));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *MetadataPy::removeMaintainer(PyObject *args)
{
    const char *name = nullptr;
    const char *email = nullptr;
    if (!PyArg_ParseTuple(args, "ss", &name, &email))
        throw Py::Exception();
    getMetadataPtr()->removeMaintainer(App::Meta::Contact(name, email));
    Py_INCREF(Py_None);
    return Py_None;
}


Py::Object MetadataPy::getAuthor() const
{
    auto authors = getMetadataPtr()->author();
    Py::List pyAuthors;
    for (const auto &a : authors) {
        Py::Dict pyAuthor;
        pyAuthor["name"] = Py::String(a.name);
        pyAuthor["email"] = Py::String(a.email);
        pyAuthors.append(pyAuthor);
    }
    return pyAuthors;
}

void MetadataPy::setAuthor(Py::Object args)
{
    PyObject *list = nullptr;
    if (!PyArg_Parse(args.ptr(), "O!", &PyList_Type, &list))
        throw Py::Exception();

    getMetadataPtr()->clearAuthor();
    Py::List authors(list);
    for (const auto &a : authors) {
        Py::Dict pyAuthor(a);
        std::string name = pyAuthor["name"].str();
        std::string email = pyAuthor["email"].str();
        getMetadataPtr()->addAuthor(App::Meta::Contact(name, email));
    }
}

PyObject *MetadataPy::addAuthor(PyObject *args)
{
    const char *name = nullptr;
    const char *email = nullptr;
    if (!PyArg_ParseTuple(args, "ss", &name, &email))
        throw Py::Exception();
    getMetadataPtr()->addAuthor(App::Meta::Contact(name, email));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *MetadataPy::removeAuthor(PyObject *args)
{
    const char *name = nullptr;
    const char *email = nullptr;
    if (!PyArg_ParseTuple(args, "ss", &name, &email))
        throw Py::Exception();
    getMetadataPtr()->removeAuthor(App::Meta::Contact(name, email));
    Py_INCREF(Py_None);
    return Py_None;
}

Py::Object MetadataPy::getLicense() const
{
    auto licenses = getMetadataPtr()->license();
    Py::List pyLicenses;
    for (const auto &lic : licenses) {
        Py::Dict pyLicense;
        pyLicense["name"] = Py::String(lic.name);
        pyLicense["file"] = Py::String(lic.file.string());
        pyLicenses.append(pyLicense);
    }
    return pyLicenses;
}

void MetadataPy::setLicense(Py::Object args)
{
    PyObject *list = nullptr;
    if (!PyArg_Parse(args.ptr(), "O!", &PyList_Type, &list))
        throw Py::Exception();

    getMetadataPtr()->clearLicense();
    Py::List licenses(list);
    for (const auto &l : licenses) {
        Py::Dict pyLicense(l);
        std::string name = pyLicense["name"].str();
        std::string path = pyLicense["file"].str();
        getMetadataPtr()->addLicense(App::Meta::License(name, path));
    }
}

PyObject *MetadataPy::addLicense(PyObject *args)
{
    const char *shortCode = nullptr;
    const char *path = nullptr;
    if (!PyArg_ParseTuple(args, "ss", &shortCode, &path))
        throw Py::Exception();
    getMetadataPtr()->addLicense(App::Meta::License(shortCode, path));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *MetadataPy::removeLicense(PyObject *args)
{
    const char *shortCode = nullptr;
    const char *path = nullptr;
    if (!PyArg_ParseTuple(args, "ss", &shortCode, &path))
        throw Py::Exception();
    getMetadataPtr()->removeLicense(App::Meta::License(shortCode, path));
    Py_INCREF(Py_None);
    return Py_None;
}

Py::Object MetadataPy::getUrls() const
{
    auto urls = getMetadataPtr()->url();
    Py::List pyUrls;
    for (const auto &url : urls) {
        Py::Dict pyUrl;
        pyUrl["location"] = Py::String(url.location);
        switch (url.type) {
            case Meta::UrlType::website: pyUrl["type"] = Py::String("website"); break;
            case Meta::UrlType::repository: pyUrl["type"] = Py::String("repository"); break;
            case Meta::UrlType::bugtracker: pyUrl["type"] = Py::String("bugtracker"); break;
            case Meta::UrlType::readme: pyUrl["type"] = Py::String("readme"); break;
            case Meta::UrlType::documentation: pyUrl["type"] = Py::String("documentation"); break;
            case Meta::UrlType::discussion: pyUrl["type"] = Py::String("discussion"); break;
            default: pyUrl["type"] = Py::String("unknown"); break;
        }
        if (url.type == Meta::UrlType::repository)
            pyUrl["branch"] = Py::String(url.branch);
        pyUrls.append(pyUrl);
    }
    return pyUrls;
}

void MetadataPy::setUrls(Py::Object args)
{
    PyObject *list = nullptr;
    if (!PyArg_Parse(args.ptr(), "O!", &PyList_Type, &list))
        throw Py::Exception();

    getMetadataPtr()->clearUrl();
    Py::List urls(list);
    for (const auto &url : urls) {
        Py::Dict pyUrl(url);
        std::string location = pyUrl["location"].str();
        std::string typeAsString = pyUrl["type"].str();
        std::string branch = pyUrl["branch"].str();
        auto newUrl = App::Meta::Url(location, Meta::UrlType::website);
        if (typeAsString == "website") {
            newUrl.type = Meta::UrlType::website;
        }
        else if (typeAsString == "repository") {
            newUrl.type = Meta::UrlType::repository;
            newUrl.branch = branch;
        }
        else if (typeAsString == "bugtracker") {
            newUrl.type = Meta::UrlType::bugtracker;
        }
        else if (typeAsString == "readme") {
            newUrl.type = Meta::UrlType::readme;
        }
        else if (typeAsString == "documentation") {
            newUrl.type = Meta::UrlType::documentation;
        }
        else if (typeAsString == "discussion") {
            newUrl.type = Meta::UrlType::discussion;
        }
        else {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Unrecognized URL type");
            return;
        }
        getMetadataPtr()->addUrl(newUrl);
    }
}

App::Meta::Url urlFromStrings(const char* urlTypeCharStar, const char* link, const char* branch)
{
    std::string urlTypeString(urlTypeCharStar);
    App::Meta::UrlType urlType{App::Meta::UrlType::documentation};
    if (urlTypeString == "repository")
        urlType = App::Meta::UrlType::repository;
    else if (urlTypeString == "bugtracker")
        urlType = App::Meta::UrlType::bugtracker;
    else if (urlTypeString == "documentation")
        urlType = App::Meta::UrlType::documentation;
    else if (urlTypeString == "readme")
        urlType = App::Meta::UrlType::readme;
    else if (urlTypeString == "website")
        urlType = App::Meta::UrlType::website;

    App::Meta::Url url(link, urlType);
    if (urlType == App::Meta::UrlType::repository)
        url.branch = std::string(branch);

    return url;
}

PyObject *MetadataPy::addUrl(PyObject *args)
{
    const char *urlTypeCharStar = nullptr;
    const char *link = nullptr;
    const char *branch = nullptr;
    if (!PyArg_ParseTuple(args, "ss|s", &urlTypeCharStar, &link, &branch))
        throw Py::Exception();

    getMetadataPtr()->addUrl(urlFromStrings(urlTypeCharStar, link, branch));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *MetadataPy::removeUrl(PyObject *args)
{
    const char *urlTypeCharStar = nullptr;
    const char *link = nullptr;
    const char *branch = nullptr;
    if (!PyArg_ParseTuple(args, "ss|s", &urlTypeCharStar, &link, &branch))
        throw Py::Exception();

    getMetadataPtr()->removeUrl(urlFromStrings(urlTypeCharStar, link, branch));
    Py_INCREF(Py_None);
    return Py_None;
}

Py::Object dependencyToPyObject(const Meta::Dependency &d)
{
    Py::Dict pyDependency;
    pyDependency["package"] = Py::String(d.package);
    pyDependency["version_lt"] = Py::String(d.version_lt);
    pyDependency["version_lte"] = Py::String(d.version_lte);
    pyDependency["version_eq"] = Py::String(d.version_eq);
    pyDependency["version_gt"] = Py::String(d.version_gt);
    pyDependency["version_gte"] = Py::String(d.version_gte);
    pyDependency["condition"] = Py::String(d.condition);
    pyDependency["optional"] = Py::Boolean(d.optional);
    switch (d.dependencyType) {
        case App::Meta::DependencyType::automatic:
            pyDependency["type"] = Py::String ("automatic");
            break;
        case App::Meta::DependencyType::addon:
            pyDependency["type"] = Py::String("addon");
            break;
        case App::Meta::DependencyType::internal:
            pyDependency["type"] = Py::String("internal");
            break;
        case App::Meta::DependencyType::python:
            pyDependency["type"] = Py::String("python");
            break;
    }
    return pyDependency;
}

Meta::Dependency pyObjectToDependency(const Py::Object &d)
{
    Py::Dict pyDependency(d);
    Meta::Dependency result;
    result.package = pyDependency["package"].str();
    if (pyDependency.hasKey("version_lt"))
        result.version_lt = pyDependency["version_lt"].str();
    if (pyDependency.hasKey("version_lte"))
        result.version_lte = pyDependency["version_lte"].str();
    if (pyDependency.hasKey("version_eq"))
        result.version_eq = pyDependency["version_eq"].str();
    if (pyDependency.hasKey("version_gt"))
        result.version_gt = pyDependency["version_gt"].str();
    if (pyDependency.hasKey("version_gte"))
        result.version_gte = pyDependency["version_gte"].str();
    if (pyDependency.hasKey("condition"))
        result.condition = pyDependency["condition"].str();
    if (pyDependency.hasKey("optional"))
        result.optional = Py::Boolean(pyDependency["optional"]).as_bool();
    if (pyDependency.hasKey("type")) {
        if (pyDependency["type"].str() == Py::String("automatic"))
            result.dependencyType = App::Meta::DependencyType::automatic;
        else if (pyDependency["type"].str() == Py::String("internal"))
            result.dependencyType = App::Meta::DependencyType::internal;
        else if (pyDependency["type"].str() == Py::String("addon"))
            result.dependencyType = App::Meta::DependencyType::addon;
        else if (pyDependency["type"].str() == Py::String("python"))
            result.dependencyType = App::Meta::DependencyType::python;
    }
    return result;
}

Py::Object MetadataPy::getDepend() const
{
    auto dependencies = getMetadataPtr()->depend();
    Py::List pyDependencies;
    for (const auto &d : dependencies) {
        pyDependencies.append(dependencyToPyObject(d));
    }
    return pyDependencies;
}

void MetadataPy::setDepend(Py::Object args)
{
    PyObject *list = nullptr;
    if (!PyArg_Parse(args.ptr(), "O!", &PyList_Type, &list))
        throw Py::Exception();

    getMetadataPtr()->clearDepend();
    Py::List deps(list);
    for (const auto &dep : deps) {
        Py::Dict pyDep(dep);
        getMetadataPtr()->addDepend(pyObjectToDependency(pyDep));
    }
}

PyObject *MetadataPy::addDepend(PyObject *args)
{
    PyObject *dictionary = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dictionary))
        throw Py::Exception();
    Py::Dict pyDep(dictionary);
    getMetadataPtr()->addDepend(pyObjectToDependency(pyDep));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *MetadataPy::removeDepend(PyObject *args)
{
    PyObject *dictionary = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dictionary))
        throw Py::Exception();
    Py::Dict pyDep(dictionary);
    getMetadataPtr()->removeDepend(pyObjectToDependency(pyDep));
    Py_INCREF(Py_None);
    return Py_None;
}

Py::Object MetadataPy::getConflict() const
{
    auto dependencies = getMetadataPtr()->conflict();
    Py::List pyDependencies;
    for (const auto &d : dependencies) {
        pyDependencies.append(dependencyToPyObject(d));
    }
    return pyDependencies;
}

void MetadataPy::setConflict(Py::Object args)
{
    PyObject *list = nullptr;
    if (!PyArg_Parse(args.ptr(), "O!", &PyList_Type, &list))
        throw Py::Exception();

    getMetadataPtr()->clearConflict();
    Py::List deps(list);
    for (const auto &dep : deps) {
        Py::Dict pyDep(dep);
        getMetadataPtr()->addConflict(pyObjectToDependency(pyDep));
    }
}

PyObject *MetadataPy::addConflict(PyObject *args)
{
    PyObject *dictionary = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dictionary))
        throw Py::Exception();
    Py::Dict pyDep(dictionary);
    getMetadataPtr()->addConflict(pyObjectToDependency(pyDep));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *MetadataPy::removeConflict(PyObject *args)
{
    PyObject *dictionary = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dictionary))
        throw Py::Exception();
    Py::Dict pyDep(dictionary);
    getMetadataPtr()->removeConflict(pyObjectToDependency(pyDep));
    Py_INCREF(Py_None);
    return Py_None;
}

Py::Object MetadataPy::getReplace() const
{
    auto dependencies = getMetadataPtr()->replace();
    Py::List pyDependencies;
    for (const auto &d : dependencies) {
        pyDependencies.append(dependencyToPyObject(d));
    }
    return pyDependencies;
}

void MetadataPy::setReplace(Py::Object args)
{
    PyObject *list = nullptr;
    if (!PyArg_Parse(args.ptr(), "O!", &PyList_Type, &list))
        throw Py::Exception();

    getMetadataPtr()->clearReplace();
    Py::List deps(list);
    for (const auto &dep : deps) {
        Py::Dict pyDep(dep);
        getMetadataPtr()->addReplace(pyObjectToDependency(pyDep));
    }
}

PyObject *MetadataPy::addReplace(PyObject *args)
{
    PyObject *dictionary = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dictionary))
        throw Py::Exception();
    Py::Dict pyDep(dictionary);
    getMetadataPtr()->addReplace(pyObjectToDependency(pyDep));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *MetadataPy::removeReplace(PyObject *args)
{
    PyObject *dictionary = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dictionary))
        throw Py::Exception();
    Py::Dict pyDep(dictionary);
    getMetadataPtr()->removeReplace(pyObjectToDependency(pyDep));
    Py_INCREF(Py_None);
    return Py_None;
}

// Tag, icon, classname, file

Py::Object MetadataPy::getTag() const
{
    auto tags = getMetadataPtr()->tag();
    Py::List pyTags;
    for (const auto &t : tags) {
        pyTags.append(Py::String(t));
    }
    return pyTags;
}

void MetadataPy::setTag(Py::Object args)
{
    PyObject *list = nullptr;
    if (!PyArg_Parse(args.ptr(), "O!", &PyList_Type, &list))
        throw Py::Exception();

    getMetadataPtr()->clearTag();
    Py::List tags(list);
    for (const auto &tag : tags) {
        Py::String pyTag(tag);
        getMetadataPtr()->addTag(pyTag.as_std_string());
    }
}

PyObject *MetadataPy::addTag(PyObject *args)
{
    const char *tag = nullptr;
    if (!PyArg_ParseTuple(args, "s", &tag))
        throw Py::Exception();
    getMetadataPtr()->addTag(tag);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *MetadataPy::removeTag(PyObject *args)
{
    const char *tag = nullptr;
    if (!PyArg_ParseTuple(args, "s", &tag))
        throw Py::Exception();
    getMetadataPtr()->removeTag(tag);
    Py_INCREF(Py_None);
    return Py_None;
}

Py::Object MetadataPy::getIcon() const
{
    return Py::String(getMetadataPtr()->icon().string());
}

void MetadataPy::setIcon(Py::Object args)
{
    const char *name;
    if (!PyArg_Parse(args.ptr(), "s", &name))
        throw Py::Exception();
    getMetadataPtr()->setIcon(name);
}

Py::Object MetadataPy::getClassname() const
{
    return Py::String(getMetadataPtr()->classname());
}

void MetadataPy::setClassname(Py::Object args)
{
    const char *name;
    if (!PyArg_Parse(args.ptr(), "s", &name))
        throw Py::Exception();
    getMetadataPtr()->setClassname(name);
}

Py::Object MetadataPy::getSubdirectory() const
{
    return Py::String(getMetadataPtr()->subdirectory().string());
}

void MetadataPy::setSubdirectory(Py::Object args)
{
    const char *name;
    if (!PyArg_Parse(args.ptr(), "s", &name))
        throw Py::Exception();
    getMetadataPtr()->setSubdirectory(name);
}

Py::Object MetadataPy::getFile() const
{
    auto files = getMetadataPtr()->file();
    Py::List pyFiles;
    for (const auto &f : files) {
        pyFiles.append(Py::String(f.string()));
    }
    return pyFiles;
}

void MetadataPy::setFile(Py::Object args)
{
    PyObject *list = nullptr;
    if (!PyArg_Parse(args.ptr(), "O!", &PyList_Type, &list))
        throw Py::Exception();

    getMetadataPtr()->clearTag();
    Py::List files(list);
    for (const auto &file : files) {
        Py::String pyFile(file);
        getMetadataPtr()->addFile(pyFile.as_std_string());
    }
}

PyObject *MetadataPy::addFile(PyObject *args)
{
    const char *file = nullptr;
    if (!PyArg_ParseTuple(args, "s", &file))
        throw Py::Exception();
    getMetadataPtr()->addFile(file);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *MetadataPy::removeFile(PyObject *args)
{
    const char *file = nullptr;
    if (!PyArg_ParseTuple(args, "s", &file))
        throw Py::Exception();
    getMetadataPtr()->removeFile(file);
    Py_INCREF(Py_None);
    return Py_None;
}


Py::Object MetadataPy::getContent() const
{
    auto content = getMetadataPtr()->content();
    std::set<std::string> keys;
    for (const auto &item : content) {
        keys.insert(item.first);
    }

    // For the Python, we'll use a dictionary of lists to store the content components:
    Py::Dict pyContent;
    for (const auto &key : keys) {
        Py::List pyContentForKey;
        auto elements = content.equal_range(key);
        for (auto &element = elements.first; element != elements.second; ++element) {
            auto contentMetadataItem = new MetadataPy(new Metadata(element->second));
            pyContentForKey.append(Py::asObject(contentMetadataItem));
        }
        pyContent[key] = pyContentForKey;
    }
    return pyContent;
}

void MetadataPy::setContent(Py::Object arg)
{
    PyObject *obj = nullptr;
    if (!PyArg_Parse(arg.ptr(), "O!", &PyList_Type, &obj))
        throw Py::Exception();

    getMetadataPtr()->clearContent();
    Py::Dict outerDict(obj);
    for (const auto &pyContentType : outerDict) {
        auto contentType = Py::String(pyContentType.first).as_std_string();
        auto contentList = Py::List(pyContentType.second);
        for (const auto& contentItem : contentList) {
            auto item = static_cast<MetadataPy *>(contentItem.ptr());
            getMetadataPtr()->addContentItem(contentType, *(item->getMetadataPtr()));
        }
    }

}

PyObject *MetadataPy::getGenericMetadata(PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return nullptr;

    auto gm = (*getMetadataPtr())[name];
    Py::List pyGenericMetadata;
    for (const auto &item : gm) {
        Py::Dict pyItem;
        pyItem["contents"] = Py::String(item.contents);
        Py::Dict pyAttributes;
        for (const auto &attribute : item.attributes) {
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
    char *version = nullptr;
    PyObject *p = args.ptr();
    if (!PyArg_Parse(p, "z", &version))
        throw Py::Exception();
    if (version)
        getMetadataPtr()->setFreeCADMin(App::Meta::Version(version));
    else
        getMetadataPtr()->setFreeCADMin(App::Meta::Version());
}

Py::Object MetadataPy::getFreeCADMax() const
{
    return Py::String(getMetadataPtr()->freecadmax().str());
}

void MetadataPy::setFreeCADMax(Py::Object args)
{
    char *version = nullptr;
    PyObject *p = args.ptr();
    if (!PyArg_Parse(p, "z", &version))
        throw Py::Exception();

    if (version)
        getMetadataPtr()->setFreeCADMax(App::Meta::Version(version));
    else
        getMetadataPtr()->setFreeCADMax(App::Meta::Version());
}

Py::Object MetadataPy::getPythonMin() const
{
    return Py::String(getMetadataPtr()->pythonmin().str());
}

void MetadataPy::setPythonMin(Py::Object args)
{
    char *version = nullptr;
    PyObject *p = args.ptr();
    if (!PyArg_Parse(p, "z", &version)) throw Py::Exception();
    if (version) getMetadataPtr()->setPythonMin(App::Meta::Version(version));
    else
        getMetadataPtr()->setPythonMin(App::Meta::Version());
}

PyObject *MetadataPy::getFirstSupportedFreeCADVersion(PyObject *p)
{
    if (!PyArg_ParseTuple(p, ""))
        return nullptr;

    // Short-circuit: if the toplevel sets a version, then the lower-levels are overridden
    if (getMetadataPtr()->freecadmin() != App::Meta::Version())
        return Py::new_reference_to(Py::String(getMetadataPtr()->freecadmin().str()));

    auto content = getMetadataPtr()->content();
    auto result = App::Meta::Version();
    for (const auto &item : content) {
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

PyObject *MetadataPy::getLastSupportedFreeCADVersion(PyObject *p)
{
    if (!PyArg_ParseTuple(p, ""))
        return nullptr;

    // Short-circuit: if the toplevel sets a version, then the lower-levels are overridden
    if (getMetadataPtr()->freecadmax() != App::Meta::Version())
        return Py::new_reference_to(Py::String(getMetadataPtr()->freecadmax().str()));

    auto content = getMetadataPtr()->content();
    auto result = App::Meta::Version();
    for (const auto &item : content) {
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

PyObject *MetadataPy::supportsCurrentFreeCAD(PyObject *p)
{
    if (!PyArg_ParseTuple(p, ""))
        return nullptr;

    bool result = getMetadataPtr()->supportsCurrentFreeCAD();
    return Py::new_reference_to(Py::Boolean(result));
}

PyObject* MetadataPy::addContentItem(PyObject* arg)
{
    char *contentType = nullptr;
    PyObject *contentItem = nullptr;
    if (!PyArg_ParseTuple(arg, "sO!", &contentType, &(App::MetadataPy::Type), &contentItem))
        return nullptr;

    if (!contentItem || !contentType)
        return nullptr;
    auto item = static_cast<MetadataPy *>(contentItem);
    getMetadataPtr()->addContentItem(contentType, *(item->getMetadataPtr()));

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *MetadataPy::removeContentItem(PyObject *arg)
{
    char *contentType = nullptr;
    char *contentName = nullptr;
    if (!PyArg_ParseTuple(arg, "ss", &contentType, &contentName))
        return nullptr;
    if (contentType && contentName)
        getMetadataPtr()->removeContentItem(contentType, contentName);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* MetadataPy::write(PyObject* args)
{
    char *filename = nullptr;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return nullptr;
    getMetadataPtr()->write(filename);

    Py_INCREF(Py_None);
    return Py_None;
}


PyObject *MetadataPy::getCustomAttributes(const char * /*attr*/) const
{
    return nullptr;
}

int MetadataPy::setCustomAttributes(const char * /*attr*/, PyObject * /*obj*/)
{
    return 0;
}
