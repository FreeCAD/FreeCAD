// SPDX-License-Identifier: LGPL-2.1-or-later

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



#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <Base/Base64.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyPythonObject.h"
#include "DocumentObject.h"


using namespace App;

namespace {

/**
 * @brief Check whether a path starts with a given directory prefix.
 *
 * @param[in] filePath The file path to check.
 * @param[in] directory The directory prefix to match against.
 * @return @c true if @p filePath starts with @p directory.
 */
bool isUnderDirectory(std::string filePath, std::string directory)
{
    std::ranges::replace(filePath, '\\', '/');
    std::ranges::replace(directory, '\\', '/');
    // Collapse repeated slashes (e.g. home path "build/debug//" + "Mod")
    auto collapseSlashes = [](std::string& s) {
        auto out = s.begin();
        for (auto it = s.begin(); it != s.end(); ++it) {
            if (*it == '/' && out != s.begin() && *(out - 1) == '/') {
                continue;
            }
            *out++ = *it;
        }
        s.erase(out, s.end());
    };
    collapseSlashes(filePath);
    collapseSlashes(directory);
    if (!directory.empty() && directory.back() != '/') {
        directory += '/';
    }
    return filePath.starts_with(directory);
}

/**
 * @brief Check whether a module import should be allowed during document restore.
 *
 * Modules already in @c sys.modules are permitted -- they were loaded by FreeCAD core or addons
 * during normal startup.  For modules not yet loaded we use @c importlib.util.find_spec() to
 * locate where the module would come from without executing it, then verify that path is under
 * a FreeCAD module directory.  This prevents a crafted FCStd from importing arbitrary modules
 * (whose <tt>__init__.py</tt> could run malicious code on import) while still allowing
 * legitimate lazy-loaded FreeCAD workbench modules to restore.
 *
 * @param[in] moduleName The fully qualified Python module name to check.
 * @return @c true if the module is allowed, @c false otherwise.
 */
bool isAllowedModule(const std::string& moduleName)
{
    Py::Dict sysModules(PyImport_GetModuleDict());
    if (sysModules.isNone()) {
        return false;
    }

    // 1) Already loaded? Must be safe.
    if (sysModules.hasKey(moduleName)) {
        return true;
    }

    // 2) Is it *in* an already loaded module? Safe.
    std::string::size_type dot = moduleName.find('.');
    if (dot != std::string::npos) {
        std::string topLevel = moduleName.substr(0, dot);
        if (sysModules.hasKey(topLevel)) {
            return true;
        }
    }

    // 3) The complicated path. Use importlib.util.find_spec() to find the origin of the module,
    // being careful to NOT load it (which is the code-execution vulnerability we're trying to
    // avoid in the first place). See if it's in one of our "safe" paths, and if it is, allow it.
    // Safe paths are a few subdirectories we recognize in the set "home", "resource", and
    // "userData" paths. Don't allow modules from outside of these directories. Not 100% mitigation,
    // but it's better than nothing.
    PyObject* importlibUtil = PyImport_ImportModule("importlib.util");
    if (!importlibUtil) {
        PyErr_Clear();
        return false;
    }
    Py::Module importlib(importlibUtil, true);
    Py::Callable findSpec(importlib.getAttr("find_spec"));

    // FreeCAD adds each workbench directory to sys.path individually (e.g. .../Mod/Assembly/),
    // so a module stored as "Assembly.JointObject" in the FCStd is actually importable as just
    // "JointObject". Try the full name first, then the part after the first dot.
    std::vector<std::string> namesToTry = {moduleName};
    if (dot != std::string::npos) {
        namesToTry.push_back(moduleName.substr(dot + 1));
    }
    Py::Object spec;
    for (const std::string& name : namesToTry) {
        Py::Tuple args(1);
        args.setItem(0, Py::String(name));
        try {
            spec = findSpec.apply(args);
        }
        catch (Py::Exception&) {
            PyErr_Clear();
            continue;
        }
        if (!spec.isNone()) {
            break;
        }
    }
    if (spec.isNone()) {
        return false;
    }

    // Use FreeCAD.__ModDirs__ as the authoritative list of allowed module directories.
    // This is populated during startup by FreeCADInit.py and includes built-in workbenches,
    // user addons, and any additional configured module paths.
    Py::Module freecad(PyImport_ImportModule("FreeCAD"), true);
    if (!freecad.hasAttr("__ModDirs__")) {
        throw Py::RuntimeError("FreeCAD.__ModDirs__ not set -- FreeCADInit.py has not run yet");
    }
    Py::List modDirs(freecad.getAttr("__ModDirs__"));

    auto isUnderFreeCAD = [&](const std::string& path) {
        for (int i = 0; i < static_cast<int>(modDirs.size()); ++i) {
            if (isUnderDirectory(path, Py::String(modDirs[i]).as_std_string())) {
                return true;
            }
        }
        return false;
    };

    // Get the origin (i.e. the file path) from the spec.
    Py::Object origin = spec.getAttr("origin");
    if (!origin.isNone() && origin.isString()) {
        return isUnderFreeCAD(Py::String(origin).as_std_string());
    }

    // No origin -- this could be a built-in module (why is an FCStd trying to load this? Very
    // suspicious, block it) or a synthetic module from a FreeCAD migration finder like
    // FemMigrateApp (which we will allow).  Check whether the spec's loader itself comes from a
    // FreeCAD module directory.
    if (!spec.hasAttr("loader") || spec.getAttr("loader").isNone()) {
        return false;
    }
    Py::Object loader = spec.getAttr("loader");
    auto loaderType = loader.type();
    if (!loaderType.hasAttr("__module__")) {
        return false;
    }
    std::string loaderModuleName = Py::String(loaderType.getAttr("__module__")).as_std_string();
    Py::Object loaderMod = sysModules.getItem(loaderModuleName);
    return isUnderFreeCAD(Py::String(loaderMod.getAttr("__file__")).as_std_string());
}

}  // anonymous namespace


TYPESYSTEM_SOURCE(App::PropertyPythonObject, App::Property)

PropertyPythonObject::PropertyPythonObject() = default;

PropertyPythonObject::~PropertyPythonObject()
{
    // this is needed because the release of the pickled object may need the
    // GIL. Thus, we grab the GIL and replace the pickled with an empty object
    Base::PyGILStateLocker lock;
    try {
        this->object = Py::Object();
    } catch (Py::TypeError &) {
        Base::Console().warning("Py::TypeError Exception caught while destroying PropertyPythonObject\n");
    }
}

void PropertyPythonObject::setValue(const Py::Object& py)
{
    Base::PyGILStateLocker lock;
    aboutToSetValue();
    this->object = py;
    hasSetValue();
}

Py::Object PropertyPythonObject::getValue() const
{
    return object;
}

PyObject* PropertyPythonObject::getPyObject()
{
    return Py::new_reference_to(this->object);
}

void PropertyPythonObject::setPyObject(PyObject* obj)
{
    Base::PyGILStateLocker lock;
    aboutToSetValue();
    this->object = obj;
    hasSetValue();
}

std::string PropertyPythonObject::toString() const
{
    std::string repr;
    Base::PyGILStateLocker lock;
    try {
        Py::Module pickle(PyImport_ImportModule("json"), true);
        if (pickle.isNull()) {
            throw Py::Exception();
        }
        Py::Callable method(pickle.getAttr(std::string("dumps")));
        Py::Object dump;
        if (this->object.hasAttr("dumps")) {
            Py::Tuple args;
            Py::Callable state(this->object.getAttr("dumps"));
            dump = state.apply(args);
        }
        // support add-ons that use the old method names
        else if (this->object.hasAttr("__getstate__")
#if PY_VERSION_HEX >= 0x030b0000
                 && this->object.getAttr("__getstate__").hasAttr("__func__")
#endif
        ) {
            Py::Tuple args;
            Py::Callable state(this->object.getAttr("__getstate__"));
            dump = state.apply(args);
        }
        else if (this->object.hasAttr("__dict__")) {
            dump = this->object.getAttr("__dict__");
        }
        else {
            dump = this->object;
        }

        Py::Tuple args(1);
        args.setItem(0, dump);
        Py::Object res = method.apply(args);
        Py::String str(res);
        repr = str.as_std_string("ascii");
    }
    catch (Py::Exception&) {
        Py::String typestr(this->object.type().str());
        Base::Console().error("PropertyPythonObject::toString(): failed for %s\n",
                              typestr.as_string().c_str());
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }

    return repr;
}

void PropertyPythonObject::fromString(const std::string& repr)
{
    Base::PyGILStateLocker lock;
    try {
        if (repr.empty()) {
            return;
        }
        Py::Module pickle(PyImport_ImportModule("json"), true);
        if (pickle.isNull()) {
            throw Py::Exception();
        }
        Py::Callable method(pickle.getAttr(std::string("loads")));
        Py::Tuple args(1);
        args.setItem(0, Py::String(repr));
        Py::Object res = method.apply(args);

        if (this->object.hasAttr("loads")) {
            Py::Tuple args(1);
            args.setItem(0, res);
            Py::Callable state(this->object.getAttr("loads"));
            state.apply(args);
        }
        // support add-ons that use the old method names
        else if (this->object.hasAttr("__setstate__")
#if PY_VERSION_HEX >= 0x030b0000
                 && this->object.getAttr("__setstate__").hasAttr("__func__")
#endif
        ) {
            Py::Tuple args(1);
            args.setItem(0, res);
            Py::Callable state(this->object.getAttr("__setstate__"));
            state.apply(args);
        }
        else if (this->object.hasAttr("__dict__")) {
            if (!res.isNone()) {
                this->object.setAttr("__dict__", res);
            }
        }
        else {
            this->object = res;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }
}


std::string PropertyPythonObject::encodeValue(const std::string& str) const
{
    std::string tmp;
    for (char it : str) {
        if (it == '<') {
            tmp += "&lt;";
        }
        else if (it == '"') {
            tmp += "&quot;";
        }
        else if (it == '&') {
            tmp += "&amp;";
        }
        else if (it == '>') {
            tmp += "&gt";
        }
        else if (it == '\n') {
            tmp += "\\n";
        }
        else {
            tmp += it;
        }
    }

    return tmp;
}

std::string PropertyPythonObject::decodeValue(const std::string& str) const
{
    std::string tmp;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (*it == '\\') {
            ++it;
            if (it != str.end() && *it == 'n') {
                tmp += '\n';
            }
        }
        else {
            tmp += *it;
        }
    }

    return tmp;
}

void PropertyPythonObject::saveObject(Base::Writer& writer) const
{
    Base::PyGILStateLocker lock;
    try {
        PropertyContainer* parent = this->getContainer();
        if (parent->isDerivedFrom(Base::Type::fromName("App::DocumentObject"))) {
            if (this->object.hasAttr("__object__")) {
                writer.Stream() << " object=\"yes\"";
            }
        }
        if (parent->isDerivedFrom(Base::Type::fromName("Gui::ViewProvider"))) {
            if (this->object.hasAttr("__vobject__")) {
                writer.Stream() << " vobject=\"yes\"";
            }
        }
    }
    catch (Py::Exception& e) {
        e.clear();
    }
}

void PropertyPythonObject::restoreObject(Base::XMLReader& reader)
{
    Base::PyGILStateLocker lock;
    try {
        PropertyContainer* parent = this->getContainer();
        if (reader.hasAttribute("object")) {
            if (strcmp(reader.getAttribute<const char*>("object"), "yes") == 0) {
                Py::Object obj = Py::asObject(parent->getPyObject());
                this->object.setAttr("__object__", obj);
            }
        }
        if (reader.hasAttribute("vobject")) {
            if (strcmp(reader.getAttribute<const char*>("vobject"), "yes") == 0) {
                Py::Object obj = Py::asObject(parent->getPyObject());
                this->object.setAttr("__vobject__", obj);
            }
        }
    }
    catch (Py::Exception& e) {
        e.clear();
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
    }
    catch (...) {
        Base::Console().error("Critical error in PropertyPythonObject::restoreObject\n");
    }
}

void PropertyPythonObject::Save(Base::Writer& writer) const
{
    std::string repr = this->toString();
    repr = Base::base64_encode((const unsigned char*)repr.c_str(), repr.size());
    std::string val = /*encodeValue*/ (repr);
    writer.Stream() << writer.ind() << "<Python value=\"" << val << R"(" encoded="yes")";

    Base::PyGILStateLocker lock;
    try {
        if (this->object.hasAttr("__module__") && this->object.hasAttr("__class__")) {
            Py::String mod(this->object.getAttr("__module__"));
            Py::Object cls(this->object.getAttr("__class__"));
            if (cls.hasAttr("__name__")) {
                Py::String name(cls.getAttr("__name__"));
                writer.Stream() << " module=\"" << (std::string)mod << "\""
                                << " class=\"" << (std::string)name << "\"";
            }
        }
        else {
            writer.Stream() << " json=\"yes\"";
        }
    }
    catch (Py::Exception&) {
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }

    saveObject(writer);
    writer.Stream() << "/>" << std::endl;
}

void PropertyPythonObject::Restore(Base::XMLReader& reader)
{
    reader.readElement("Python");
    if (reader.hasAttribute("file")) {
        std::string file(reader.getAttribute<const char*>("file"));
        reader.addFile(file.c_str(), this);
    }
    else {
        bool load_json = false;
        bool load_failed = false;
        std::string buffer = reader.getAttribute<const char*>("value");
        if (reader.hasAttribute("encoded") && strcmp(reader.getAttribute<const char*>("encoded"), "yes") == 0) {
            buffer = Base::base64_decode(buffer);
        }
        else {
            buffer = decodeValue(buffer);
        }

        Base::PyGILStateLocker lock;
        try {
            if (reader.hasAttribute("module") && reader.hasAttribute("class")) {
                std::string moduleName = reader.getAttribute<const char*>("module");
                if (!isAllowedModule(moduleName)) {
                    Base::Console().warning(
                        "PropertyPythonObject::Restore: blocked import of module '%s' during"
                        " document restore. Only modules from FreeCAD or installed addons"
                        " are permitted.\n",
                        moduleName.c_str());
                    throw Py::ImportError("module not permitted: " + moduleName);
                }
                Py::Module mod(PyImport_ImportModule(moduleName.c_str()), true);
                if (mod.isNull()) {
                    throw Py::Exception();
                }
                std::string className = reader.getAttribute<const char*>("class");
                PyObject* cls = mod.getAttr(className).ptr();
                if (!cls) {
                    std::stringstream s;
                    s << "Module " << moduleName << " has no class " << className;
                    throw Py::AttributeError(s.str());
                }
                if (PyType_Check(cls)) {
                    this->object = PyType_GenericAlloc((PyTypeObject*)cls, 0);
                }
                else {
                    throw Py::TypeError("neither class nor type object");
                }
                load_json = true;
            }
            else if (reader.hasAttribute("json")) {
                load_json = true;
            }
        }
        catch (Py::Exception&) {
            Base::PyException e;  // extract the Python error text
            e.reportException();
            this->object = Py::None();
            load_failed = true;
        }

        aboutToSetValue();
        if (load_json) {
            this->fromString(buffer);
        }
        else if (!load_failed) {
            Base::Console().warning(
                "PropertyPythonObject::Restore: unsupported serialisation: %s\n",
                buffer.c_str());
        }
        restoreObject(reader);
        hasSetValue();
    }
}

void PropertyPythonObject::SaveDocFile(Base::Writer& writer) const
{
    std::string buffer = this->toString();
    for (char it : buffer) {
        writer.Stream().put(it);
    }
}

void PropertyPythonObject::RestoreDocFile(Base::Reader& reader)
{
    aboutToSetValue();
    std::string buffer;
    char ch {};
    while (reader.get(ch)) {
        buffer.push_back(ch);
    }
    this->fromString(buffer);
    hasSetValue();
}

unsigned int PropertyPythonObject::getMemSize() const
{
    return sizeof(Py::Object);
}

Property* PropertyPythonObject::Copy() const
{
    PropertyPythonObject* p = new PropertyPythonObject();
    Base::PyGILStateLocker lock;
    p->object = this->object;
    return p;
}

void PropertyPythonObject::Paste(const Property& from)
{
    if (from.is<PropertyPythonObject>()) {
        Base::PyGILStateLocker lock;
        aboutToSetValue();
        this->object = static_cast<const PropertyPythonObject&>(from).object;
        hasSetValue();
    }
}
