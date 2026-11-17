// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2015 Werner Mayer
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#pragma once

#include <App/PropertyContainer.h>

namespace App
{

class DocumentObjectExecReturn;
class Property;

/**
 * FeatureCustomT is a template class to be used with DocumentObject or
 * any of its subclasses as template parameter.
 * FeatureCustomT offers a way to add or remove a property at runtime.
 * This class is similar to \ref FeaturePythonT with the difference that
 * it has no support for in Python written feature classes.
 * @author Werner Mayer
 */
template<class FeatureT>
class FeatureCustomT: public FeatureT  // NOLINT
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::FeatureCustomT<FeatureT>);

public:
    FeatureCustomT() = default;

    ~FeatureCustomT() override = default;

    /** @name methods override DocumentObject */
    //@{
    short mustExecute() const override
    {
        return FeatureT::mustExecute();
    }
    /// recalculate the Feature
    DocumentObjectExecReturn* execute() override
    {
        return FeatureT::execute();
    }
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return FeatureT::getViewProviderName();
    }

    PyObject* getPyObject() override
    {
        return FeatureT::getPyObject();
    }
    void setPyObject(PyObject* obj) override
    {
        FeatureT::setPyObject(obj);
    }

protected:
    void onBeforeChange(const Property* prop) override
    {
        FeatureT::onBeforeChange(prop);
    }
    void onChanged(const Property* prop) override
    {
        FeatureT::onChanged(prop);
    }
    void onDocumentRestored() override
    {
        FeatureT::onDocumentRestored();
    }
    void onSettingDocument() override
    {
        FeatureT::onSettingDocument();
    }

public:
    FeatureCustomT(const FeatureCustomT&) = delete;
    FeatureCustomT(FeatureCustomT&&) = delete;
    FeatureCustomT& operator=(const FeatureCustomT&) = delete;
    FeatureCustomT& operator=(FeatureCustomT&&) = delete;
};

}  // namespace App
