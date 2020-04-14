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
    }

    virtual ~FeatureCustomT() {
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
};

} //namespace App

#endif // APP_FEATURECUSTOM_H
