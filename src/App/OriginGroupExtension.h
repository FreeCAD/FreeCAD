// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#pragma once

#include "GeoFeatureGroupExtension.h"
#include "QCoreApplication"

namespace App
{
class Origin;

/**
 * Represents an abstract placeable group of objects with an associated Origin
 */
class AppExport OriginGroupExtension: public App::GeoFeatureGroupExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::OriginGroupExtension);
    Q_DECLARE_TR_FUNCTIONS(App::OriginGroupExtension)

public:
    OriginGroupExtension();
    ~OriginGroupExtension() override;

    /// Returns the origin link or throws an exception
    App::Origin* getOrigin() const;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName() const
    {
        return "Gui::ViewProviderOriginGroup";
    }

    /**
     * Returns the origin group which contains this object.
     * In case this object is not part of any geoFeatureGroup, 0 is returned.
     * @param obj       the object to search for
     */
    static DocumentObject* getGroupOfObject(const DocumentObject* obj);

    /// Returns true on changing OriginFeature set
    short extensionMustExecute() override;

    /// Origin linked to the group
    PropertyLink Origin;

    // changes all links of obj to a origin to point to this groups origin
    void relinkToOrigin(App::DocumentObject* obj);

    std::vector<DocumentObject*> addObjects(std::vector<DocumentObject*> obj) override;
    bool hasObject(const DocumentObject* obj, bool recursive = false) const override;

    bool extensionGetSubObject(DocumentObject*& ret,
                               const char* subname,
                               PyObject** pyObj,
                               Base::Matrix4D* mat,
                               bool transform,
                               int depth) const override;

    void extensionOnChanged(const Property* p) override;

protected:
    /// Checks integrity of the Origin
    App::DocumentObjectExecReturn* extensionExecute() override;
    /// Creates the corresponding Origin object
    void onExtendedSetupObject() override;
    /// Removes all planes and axis if they are still linked to the document
    void onExtendedUnsetupObject() override;

private:
    /// Creates a localized Origin object
    App::DocumentObject* getLocalizedOrigin(App::Document* doc);
};

using OriginGroupExtensionPython = ExtensionPythonT<GroupExtensionPythonT<OriginGroupExtension>>;

}  // namespace App