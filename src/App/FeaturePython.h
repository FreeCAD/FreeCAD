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
    bool mustExecute() const;
    void onBeforeChange(const Property* prop);
    bool onBeforeChangeLabel(std::string &newLabel);
    void onChanged(const Property* prop);
    void onDocumentRestored();
    std::string getViewProviderName();
    PyObject *getPyObject(void);

    bool getSubObject(App::DocumentObject *&ret, const char *subname, PyObject **pyObj, 
            Base::Matrix4D *mat, bool transform, int depth) const;

    bool getSubObjects(std::vector<std::string> &ret, int reason) const;

    bool getLinkedObject(App::DocumentObject *&ret, bool recurse, 
            Base::Matrix4D *mat, bool transform, int depth) const;

    int canLinkProperties() const;

    int allowDuplicateLabel() const;

    bool redirectSubName(std::ostringstream &ss,
            App::DocumentObject *topParent, App::DocumentObject *child) const;

    int canLoadPartial() const;

    /// return true to activate tree view group object handling
    int hasChildElement() const;
    /// Get sub-element visibility
    int isElementVisible(const char *) const;
    /// Set sub-element visibility
    int setElementVisible(const char *, bool);

private:
    App::DocumentObject* object;
    bool has__object__;

#define FC_PY_FEATURE_PYTHON \
    FC_PY_ELEMENT(execute)\
    FC_PY_ELEMENT(mustExecute)\
    FC_PY_ELEMENT(onBeforeChange)\
    FC_PY_ELEMENT(onBeforeChangeLabel)\
    FC_PY_ELEMENT(onChanged)\
    FC_PY_ELEMENT(onDocumentRestored)\
    FC_PY_ELEMENT(getViewProviderName)\
    FC_PY_ELEMENT(getSubObject)\
    FC_PY_ELEMENT(getSubObjects)\
    FC_PY_ELEMENT(getLinkedObject)\
    FC_PY_ELEMENT(canLinkProperties)\
    FC_PY_ELEMENT(allowDuplicateLabel)\
    FC_PY_ELEMENT(redirectSubName)\
    FC_PY_ELEMENT(canLoadPartial)\
    FC_PY_ELEMENT(hasChildElement)\
    FC_PY_ELEMENT(isElementVisible)\
    FC_PY_ELEMENT(setElementVisible)

#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) Py::Object py_##_name;

    FC_PY_FEATURE_PYTHON

public:
    void init(PyObject *pyobj);
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
        auto ret = FeatureT::mustExecute();
        if(ret) return ret;
        return imp->mustExecute()?1:0;
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
    virtual const char* getViewProviderNameOverride(void) const override{
        viewProviderName = imp->getViewProviderName();
        if(viewProviderName.size())
            return viewProviderName.c_str();
        return FeatureT::getViewProviderNameOverride();
    }
    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return FeatureT::getViewProviderName();
        //return "Gui::ViewProviderPythonFeature";
    }

    virtual App::DocumentObject *getSubObject(const char *subname, PyObject **pyObj, 
            Base::Matrix4D *mat, bool transform, int depth) const override 
    {
        App::DocumentObject *ret = 0;
        if(imp->getSubObject(ret,subname,pyObj,mat,transform,depth))
            return ret;
        return FeatureT::getSubObject(subname,pyObj,mat,transform,depth);
    }

    virtual std::vector<std::string> getSubObjects(int reason=0) const override {
        std::vector<std::string> ret;
        if(imp->getSubObjects(ret,reason))
            return ret;
        return FeatureT::getSubObjects(reason);
    }

    virtual App::DocumentObject *getLinkedObject(bool recurse, 
            Base::Matrix4D *mat, bool transform, int depth) const override
    {
        App::DocumentObject *ret = 0;
        if(imp->getLinkedObject(ret,recurse,mat,transform,depth))
            return ret;
        return FeatureT::getLinkedObject(recurse,mat,transform,depth);
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

    /// return true to activate tree view group object handling
    virtual bool hasChildElement() const override {
        int ret = imp->hasChildElement();
        if(ret<0) 
            return FeatureT::hasChildElement();
        return ret?true:false;
    }
    /// Get sub-element visibility
    virtual int isElementVisible(const char *element) const override {
        int ret = imp->isElementVisible(element);
        if(ret == -2)
            return FeatureT::isElementVisible(element);
        return ret;
    }
    /// Set sub-element visibility
    virtual int setElementVisible(const char *element, bool visible) override {
        int ret = imp->setElementVisible(element,visible);
        if(ret == -2)
            return FeatureT::setElementVisible(element,visible);
        return ret;
    }

    virtual bool canLinkProperties() const override {
        int ret = imp->canLinkProperties();
        if(ret < 0)
            return FeatureT::canLinkProperties();
        return ret?true:false;
    }

    virtual bool allowDuplicateLabel() const override {
        int ret = imp->allowDuplicateLabel();
        if(ret < 0)
            return FeatureT::allowDuplicateLabel();
        return ret?true:false;
    }

    virtual bool redirectSubName(std::ostringstream &ss,
            App::DocumentObject *topParent, App::DocumentObject *child) const override 
    {
        return imp->redirectSubName(ss,topParent,child) ||
            FeatureT::redirectSubName(ss,topParent,child);
    }

    virtual int canLoadPartial() const override {
        int ret = imp->canLoadPartial();
        if(ret>=0)
            return ret;
        return FeatureT::canLoadPartial();
    }

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
    virtual void onBeforeChangeLabel(std::string &newLabel) override{
        if(!imp->onBeforeChangeLabel(newLabel))
            FeatureT::onBeforeChangeLabel(newLabel);
    }
    virtual void onChanged(const Property* prop) {
        if(prop == &Proxy)
            imp->init(Proxy.getValue().ptr());
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
    mutable std::string viewProviderName;
};

// Special Feature-Python classes
typedef FeaturePythonT<DocumentObject> FeaturePython;
typedef FeaturePythonT<GeoFeature    > GeometryPython;

} //namespace App

#endif // APP_FEATUREPYTHON_H
