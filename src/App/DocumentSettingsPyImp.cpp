// SPDX-License-Identifier: LGPL-2.1-or-later

#include "DocumentSettings.h"

#include <sstream>

// inclusion of the generated files (generated out of DocumentSettings.pyi)
#include "DocumentSettingsPy.h"
#include "DocumentSettingsPy.cpp"

using namespace App;

namespace
{

bool parseUnicode(PyObject* object, const char* name, std::string& value)
{
    if (!PyUnicode_Check(object)) {
        PyErr_Format(PyExc_TypeError, "%s must be a string", name);
        return false;
    }

    const char* utf8 = PyUnicode_AsUTF8(object);
    if (!utf8) {
        return false;
    }

    value = utf8;
    return true;
}

bool parseBool(PyObject* object, const char* name, bool& value)
{
    if (!PyBool_Check(object)) {
        PyErr_Format(PyExc_TypeError, "%s must be a bool", name);
        return false;
    }

    value = object == Py_True;
    return true;
}

}  // namespace

std::string DocumentSettingsPy::representation() const
{
    std::stringstream str;
    str << "<DocumentSettings namespace='" << getDocumentSettingsPtr()->getNamespace() << "'>";
    return str.str();
}

PyObject* DocumentSettingsPy::getString(PyObject* args) const
{
    PyObject* keyObject = nullptr;
    PyObject* defaultObject = nullptr;
    if (!PyArg_ParseTuple(args, "O|O", &keyObject, &defaultObject)) {
        return nullptr;
    }

    std::string key;
    std::string defaultValue;
    if (!parseUnicode(keyObject, "key", key)) {
        return nullptr;
    }
    if (defaultObject && !parseUnicode(defaultObject, "default", defaultValue)) {
        return nullptr;
    }

    return Py::new_reference_to(
        Py::String(getDocumentSettingsPtr()->getString(key, defaultValue)));
}

PyObject* DocumentSettingsPy::setString(PyObject* args)
{
    PyObject* keyObject = nullptr;
    PyObject* valueObject = nullptr;
    if (!PyArg_ParseTuple(args, "OO", &keyObject, &valueObject)) {
        return nullptr;
    }

    std::string key;
    std::string value;
    if (!parseUnicode(keyObject, "key", key) || !parseUnicode(valueObject, "value", value)) {
        return nullptr;
    }

    getDocumentSettingsPtr()->setString(key, value);
    Py_Return;
}

PyObject* DocumentSettingsPy::getInt(PyObject* args) const
{
    PyObject* keyObject = nullptr;
    long defaultValue = 0;
    if (!PyArg_ParseTuple(args, "O|l", &keyObject, &defaultValue)) {
        return nullptr;
    }

    std::string key;
    if (!parseUnicode(keyObject, "key", key)) {
        return nullptr;
    }

    return Py::new_reference_to(Py::Long(getDocumentSettingsPtr()->getInt(key, defaultValue)));
}

PyObject* DocumentSettingsPy::setInt(PyObject* args)
{
    PyObject* keyObject = nullptr;
    long value = 0;
    if (!PyArg_ParseTuple(args, "Ol", &keyObject, &value)) {
        return nullptr;
    }

    std::string key;
    if (!parseUnicode(keyObject, "key", key)) {
        return nullptr;
    }

    getDocumentSettingsPtr()->setInt(key, value);
    Py_Return;
}

PyObject* DocumentSettingsPy::getFloat(PyObject* args) const
{
    PyObject* keyObject = nullptr;
    double defaultValue = 0.0;
    if (!PyArg_ParseTuple(args, "O|d", &keyObject, &defaultValue)) {
        return nullptr;
    }

    std::string key;
    if (!parseUnicode(keyObject, "key", key)) {
        return nullptr;
    }

    return Py::new_reference_to(Py::Float(getDocumentSettingsPtr()->getFloat(key, defaultValue)));
}

PyObject* DocumentSettingsPy::setFloat(PyObject* args)
{
    PyObject* keyObject = nullptr;
    double value = 0.0;
    if (!PyArg_ParseTuple(args, "Od", &keyObject, &value)) {
        return nullptr;
    }

    std::string key;
    if (!parseUnicode(keyObject, "key", key)) {
        return nullptr;
    }

    getDocumentSettingsPtr()->setFloat(key, value);
    Py_Return;
}

PyObject* DocumentSettingsPy::getBool(PyObject* args) const
{
    PyObject* keyObject = nullptr;
    PyObject* defaultObject = nullptr;
    if (!PyArg_ParseTuple(args, "O|O", &keyObject, &defaultObject)) {
        return nullptr;
    }

    std::string key;
    if (!parseUnicode(keyObject, "key", key)) {
        return nullptr;
    }

    bool defaultValue = false;
    if (defaultObject && !parseBool(defaultObject, "default", defaultValue)) {
        return nullptr;
    }

    return Py::new_reference_to(
        Py::Boolean(getDocumentSettingsPtr()->getBool(key, defaultValue)));
}

PyObject* DocumentSettingsPy::setBool(PyObject* args)
{
    PyObject* keyObject = nullptr;
    PyObject* valueObject = nullptr;
    if (!PyArg_ParseTuple(args, "OO", &keyObject, &valueObject)) {
        return nullptr;
    }

    std::string key;
    if (!parseUnicode(keyObject, "key", key)) {
        return nullptr;
    }

    bool value = false;
    if (!parseBool(valueObject, "value", value)) {
        return nullptr;
    }

    getDocumentSettingsPtr()->setBool(key, value);
    Py_Return;
}

PyObject* DocumentSettingsPy::remove(PyObject* args)
{
    PyObject* keyObject = nullptr;
    if (!PyArg_ParseTuple(args, "O", &keyObject)) {
        return nullptr;
    }

    std::string key;
    if (!parseUnicode(keyObject, "key", key)) {
        return nullptr;
    }

    getDocumentSettingsPtr()->remove(key);
    Py_Return;
}

PyObject* DocumentSettingsPy::keys(PyObject* args) const
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::List result;
    for (const auto& key : getDocumentSettingsPtr()->keys()) {
        result.append(Py::String(key));
    }
    return Py::new_reference_to(result);
}

PyObject* DocumentSettingsPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DocumentSettingsPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
