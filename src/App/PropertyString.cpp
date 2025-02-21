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

#include <boost/algorithm/string/predicate.hpp>

#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Base/Quantity.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "ObjectIdentifier.h"
#include "PropertyString.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyString, App::Property)

PropertyString::PropertyString() = default;

PropertyString::~PropertyString() = default;

void PropertyString::setValue(const char* newLabel)
{
    if (!newLabel) {
        return;
    }

    if (_cValue == newLabel) {
        return;
    }

    std::string _newLabel;

    std::vector<std::pair<Property*, std::unique_ptr<Property>>> propChanges;
    std::string label;
    auto obj = dynamic_cast<DocumentObject*>(getContainer());
    bool commit = false;

    if (obj && obj->isAttachedToDocument() && this == &obj->Label
        && (!obj->getDocument()->testStatus(App::Document::Restoring)
            || obj->getDocument()->testStatus(App::Document::Importing))
        && !obj->getDocument()->isPerformingTransaction()) {
        // allow object to control label change

        static ParameterGrp::handle _hPGrp;
        if (!_hPGrp) {
            _hPGrp = GetApplication().GetUserParameter().GetGroup("BaseApp");
            _hPGrp = _hPGrp->GetGroup("Preferences")->GetGroup("Document");
        }
        App::Document* doc = obj->getDocument();
        if (doc && !_hPGrp->GetBool("DuplicateLabels") && !obj->allowDuplicateLabel()) {
            std::vector<std::string> objectLabels;
            std::vector<App::DocumentObject*>::const_iterator it;
            std::vector<App::DocumentObject*> objs = doc->getObjects();
            bool match = false;
            for (it = objs.begin(); it != objs.end(); ++it) {
                if (*it == obj) {
                    continue;  // don't compare object with itself
                }
                std::string objLabel = (*it)->Label.getValue();
                if (!match && objLabel == newLabel) {
                    match = true;
                }
                objectLabels.push_back(objLabel);
            }

            // make sure that there is a name conflict otherwise we don't have to do anything
            if (match && *newLabel) {
                label = newLabel;
                // remove number from end to avoid lengthy names
                size_t lastpos = label.length() - 1;
                while (label[lastpos] >= 48 && label[lastpos] <= 57) {
                    // if 'lastpos' becomes 0 then all characters are digits. In this case we use
                    // the complete label again
                    if (lastpos == 0) {
                        lastpos = label.length() - 1;
                        break;
                    }
                    lastpos--;
                }

                bool changed = false;
                label = label.substr(0, lastpos + 1);
                if (label != obj->getNameInDocument()
                    && boost::starts_with(obj->getNameInDocument(), label)) {
                    // In case the label has the same base name as object's
                    // internal name, use it as the label instead.
                    const char* objName = obj->getNameInDocument();
                    const char* c = &objName[lastpos + 1];
                    for (; *c; ++c) {
                        if (*c < 48 || *c > 57) {
                            break;
                        }
                    }
                    if (*c == 0
                        && std::find(objectLabels.begin(),
                                     objectLabels.end(),
                                     obj->getNameInDocument())
                            == objectLabels.end()) {
                        label = obj->getNameInDocument();
                        changed = true;
                    }
                }
                if (!changed) {
                    label = Base::Tools::getUniqueName(label, objectLabels, 3);
                }
            }
        }

        if (label.empty()) {
            label = newLabel;
        }
        obj->onBeforeChangeLabel(label);
        newLabel = label.c_str();

        if (!obj->getDocument()->testStatus(App::Document::Restoring)) {
            // Only update label reference if we are not restoring. When
            // importing (which also counts as restoring), it is possible the
            // new object changes its label. However, we cannot update label
            // references here, because object restoring is not based on
            // dependency order. It can only be done in afterRestore().
            //
            // See PropertyLinkBase::restoreLabelReference() for more details.
            propChanges = PropertyLinkBase::updateLabelReferences(obj, newLabel);
        }

        if (!propChanges.empty() && !GetApplication().getActiveTransaction()) {
            commit = true;
            std::ostringstream str;
            str << "Change " << obj->getNameInDocument() << ".Label";
            GetApplication().setActiveTransaction(str.str().c_str());
        }
    }

    aboutToSetValue();
    _cValue = newLabel;
    hasSetValue();

    for (auto& change : propChanges) {
        change.first->Paste(*change.second.get());
    }

    if (commit) {
        GetApplication().closeActiveTransaction();
    }
}

void PropertyString::setValue(const std::string& sString)
{
    setValue(sString.c_str());
}

const char* PropertyString::getValue() const
{
    return _cValue.c_str();
}

PyObject* PropertyString::getPyObject()
{
    PyObject* p = PyUnicode_DecodeUTF8(_cValue.c_str(), _cValue.size(), nullptr);
    if (!p) {
        throw Base::UnicodeError("UTF8 conversion failure at PropertyString::getPyObject()");
    }
    return p;
}

void PropertyString::setPyObject(PyObject* value)
{
    std::string string;
    if (PyUnicode_Check(value)) {
        string = PyUnicode_AsUTF8(value);
    }
    else {
        std::string error = std::string("type must be str or unicode, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }

    // assign the string
    setValue(string);
}

void PropertyString::Save(Base::Writer& writer) const
{
    std::string val;
    auto obj = dynamic_cast<DocumentObject*>(getContainer());
    writer.Stream() << writer.ind() << "<String ";
    bool exported = false;
    if (obj && obj->isAttachedToDocument() && obj->isExporting() && &obj->Label == this) {
        if (obj->allowDuplicateLabel()) {
            writer.Stream() << "restore=\"1\" ";
        }
        else if (_cValue == obj->getNameInDocument()) {
            writer.Stream() << "restore=\"0\" ";
            val = encodeAttribute(obj->getExportName());
            exported = true;
        }
    }
    if (!exported) {
        val = encodeAttribute(_cValue);
    }
    writer.Stream() << "value=\"" << val << "\"/>" << std::endl;
}

void PropertyString::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("String");
    // get the value of my Attribute
    auto obj = dynamic_cast<DocumentObject*>(getContainer());
    if (obj && &obj->Label == this) {
        if (reader.hasAttribute("restore")) {
            int restore = reader.getAttributeAsInteger("restore");
            if (restore == 1) {
                aboutToSetValue();
                _cValue = reader.getAttribute("value");
                hasSetValue();
            }
            else {
                setValue(reader.getName(reader.getAttribute("value")));
            }
        }
        else {
            setValue(reader.getAttribute("value"));
        }
    }
    else {
        setValue(reader.getAttribute("value"));
    }
}

Property* PropertyString::Copy() const
{
    PropertyString* p = new PropertyString();
    p->_cValue = _cValue;
    return p;
}

void PropertyString::Paste(const Property& from)
{
    setValue(dynamic_cast<const PropertyString&>(from)._cValue);
}

unsigned int PropertyString::getMemSize() const
{
    return static_cast<unsigned int>(_cValue.size());
}

void PropertyString::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    verifyPath(path);
    if (value.type() == typeid(bool)) {
        setValue(boost::any_cast<bool>(value) ? "True" : "False");
    }
    else if (value.type() == typeid(int)) {
        setValue(std::to_string(boost::any_cast<int>(value)));
    }
    else if (value.type() == typeid(long)) {
        setValue(std::to_string(boost::any_cast<long>(value)));
    }
    else if (value.type() == typeid(double)) {
        setValue(std::to_string(App::any_cast<double>(value)));
    }
    else if (value.type() == typeid(float)) {
        setValue(std::to_string(App::any_cast<float>(value)));
    }
    else if (value.type() == typeid(Base::Quantity)) {
        setValue(boost::any_cast<Base::Quantity>(value).getUserString().c_str());
    }
    else if (value.type() == typeid(std::string)) {
        setValue(boost::any_cast<const std::string &>(value));
    }
    else {
        Base::PyGILStateLocker lock;
        setValue(pyObjectFromAny(value).as_string());
    }
}

const boost::any PropertyString::getPathValue(const ObjectIdentifier& path) const
{
    verifyPath(path);
    return _cValue;
}

}  // namespace App
