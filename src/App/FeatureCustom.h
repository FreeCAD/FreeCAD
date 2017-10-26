/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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



#ifndef APP_FEATURECUSTOM_H
#define APP_FEATURECUSTOM_H


#include <Base/Writer.h>
#include <App/DocumentObject.h>
#include <App/DynamicProperty.h>

namespace App
{

class Property;

/**
 * FeatureCustomT is a template class to be used with DocumentObject or
 * any of its subclasses as template parameter.
 * FeatureCustomT offers a way to add or remove a property at runtime.
 * This class is similar to \ref FeaturePythonT with the difference that
 * it has no support for in Python written feature classes.
 * @author Werner Mayer
 */
template <class FeatureT>
class FeatureCustomT : public FeatureT
{
    PROPERTY_HEADER(App::FeatureCustomT<FeatureT>);

public:
    FeatureCustomT() {
        props = new DynamicProperty(this);
    }
    virtual ~FeatureCustomT() {
        delete props;
    }

    /** @name methods override DocumentObject */
    //@{
    short mustExecute() const {
        return FeatureT::mustExecute();
    }
    /// recalculate the Feature
    virtual DocumentObjectExecReturn *execute(void) {
        return FeatureT::execute();
    }
    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return FeatureT::getViewProviderName();
    }

    /** @name Access properties */
    //@{
    Property* addDynamicProperty(
        const char* type, const char* name=0,
        const char* group=0, const char* doc=0,
        short attr=0, bool ro=false, bool hidden=false) {
        return props->addDynamicProperty(type, name, group, doc, attr, ro, hidden);
    }
    virtual bool removeDynamicProperty(const char* name) {
        FeatureT::onAboutToRemoveProperty(name);
        return props->removeDynamicProperty(name);
    }
    std::vector<std::string> getDynamicPropertyNames() const {
        return props->getDynamicPropertyNames();
    }
    Property *getDynamicPropertyByName(const char* name) const {
        return props->getDynamicPropertyByName(name);
    }
    virtual void addDynamicProperties(const PropertyContainer* cont) {
        return props->addDynamicProperties(cont);
    }
    /// get all properties of the class (including properties of the parent)
    virtual void getPropertyList(std::vector<Property*> &List) const {
        props->getPropertyList(List);
    }
    /// get all properties of the class (including parent)
    void getPropertyMap(std::map<std::string,Property*> &Map) const {
        props->getPropertyMap(Map);
    }
    /// find a property by its name
    virtual Property *getPropertyByName(const char* name) const {
        return props->getPropertyByName(name);
    }
    /// get the name of a property
    virtual const char* getPropertyName(const Property* prop) const {
        return props->getPropertyName(prop);
    }
    //@}

    /** @name Property attributes */
    //@{
    /// get the Type of a Property
    short getPropertyType(const Property* prop) const {
        return props->getPropertyType(prop);
    }
    /// get the Type of a named Property
    short getPropertyType(const char *name) const {
        return props->getPropertyType(name);
    }
    /// get the Group of a Property
    const char* getPropertyGroup(const Property* prop) const {
        return props->getPropertyGroup(prop);
    }
    /// get the Group of a named Property
    const char* getPropertyGroup(const char *name) const {
        return props->getPropertyGroup(name);
    }
    /// get the Documentation of a Property
    const char* getPropertyDocumentation(const Property* prop) const {
        return props->getPropertyDocumentation(prop);
    }
    /// get the Group of a named Property
    const char* getPropertyDocumentation(const char *name) const {
        return props->getPropertyDocumentation(name);
    }
    //@}

    /** @name Property serialization */
    //@{
    void Save (Base::Writer &writer) const {
        writer.ObjectName = this->getNameInDocument();
        props->Save(writer);
    }
    void Restore(Base::XMLReader &reader) {
        props->Restore(reader);
    }
    //@}

    PyObject *getPyObject(void) {
        return FeatureT::getPyObject();
    }
    void setPyObject(PyObject *obj) {
        FeatureT::setPyObject(obj);
    }

protected:
    virtual void onBeforeChange(const Property* prop) {
        FeatureT::onBeforeChange(prop);
    }
    virtual void onChanged(const Property* prop) {
        FeatureT::onChanged(prop);
    }
    virtual void onDocumentRestored() {
        FeatureT::onDocumentRestored();
    }
    virtual void onSettingDocument() {
        FeatureT::onSettingDocument();
    }

private:
    DynamicProperty* props;
};

} //namespace App

#endif // APP_FEATURECUSTOM_H
