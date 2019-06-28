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
    }
    virtual ~FeaturePythonT() {
        delete imp;
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
