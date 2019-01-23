/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2006     *
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



#ifndef APP_FEATUREPYTHON_H
#define APP_FEATUREPYTHON_H


#include <Base/Exception.h>
#include <Base/Writer.h>
#include <App/GeoFeature.h>
#include <App/DynamicProperty.h>
#include <App/PropertyPythonObject.h>
#include <App/PropertyGeo.h>

namespace App
{

class Property;

// Helper class to hide implementation details
class AppExport FeaturePythonImp
{
public:
    FeaturePythonImp(App::DocumentObject*);
    ~FeaturePythonImp();

    bool execute();
    void onBeforeChange(const Property* prop);
    void onChanged(const Property* prop);
    void onDocumentRestored();
    PyObject *getPyObject(void);

private:
    App::DocumentObject* object;
};

/**
 * Generic Python feature class which allows to behave every DocumentObject
 * derived class as Python feature -- simply by subclassing.
 * @author Werner Mayer
 */
template <class FeatureT>
class FeaturePythonT : public FeatureT
{
    PROPERTY_HEADER(App::FeaturePythonT<FeatureT>);

public:
    FeaturePythonT() {
        ADD_PROPERTY(Proxy,(Py::Object()));
        // cannot move this to the initializer list to avoid warning
        imp = new FeaturePythonImp(this);
        props = new DynamicProperty(this);
    }
    virtual ~FeaturePythonT() {
        delete imp;
        delete props;
    }

    /** @name methods override DocumentObject */
    //@{
    short mustExecute() const {
        if (this->isTouched())
            return 1;
        return FeatureT::mustExecute();
    }
    /// recalculate the Feature
    virtual DocumentObjectExecReturn *execute(void) {
        try {
            bool handled = imp->execute();
            if (!handled)
                return FeatureT::execute();
        }
        catch (const Base::Exception& e) {
            return new App::DocumentObjectExecReturn(e.what());
        }
        return DocumentObject::StdReturn;
    }
    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return FeatureT::getViewProviderName();
        //return "Gui::ViewProviderPythonFeature";
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
        const char* objname = this->getNameInDocument();
        // if null then it's not part of the document
        if (objname) {
            writer.ObjectName = objname;
            props->Save(writer);
        }
    }
    void Restore(Base::XMLReader &reader) {
        props->Restore(reader);
    }
    //@}

    PyObject *getPyObject(void) {
        if (FeatureT::PythonObject.is(Py::_None())) {
            // ref counter is set to 1
            FeatureT::PythonObject = Py::Object(imp->getPyObject(),true);
        }
        return Py::new_reference_to(FeatureT::PythonObject);
    }
    void setPyObject(PyObject *obj) {
        if (obj)
            FeatureT::PythonObject = obj;
        else
            FeatureT::PythonObject = Py::None();
    }

protected:
    virtual void onBeforeChange(const Property* prop) {
        FeatureT::onBeforeChange(prop);
        imp->onBeforeChange(prop);
    }
    virtual void onChanged(const Property* prop) {
        imp->onChanged(prop);
        FeatureT::onChanged(prop);
    }
    virtual void onDocumentRestored() {
        imp->onDocumentRestored();
        FeatureT::onDocumentRestored();
    }

private:
    FeaturePythonImp* imp;
    DynamicProperty* props;
    PropertyPythonObject Proxy;
};

// Special Feature-Python classes
typedef FeaturePythonT<DocumentObject> FeaturePython;
typedef FeaturePythonT<GeoFeature    > GeometryPython;

} //namespace App

#endif // APP_FEATUREPYTHON_H
