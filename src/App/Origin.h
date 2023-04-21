/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef APP_Origin_H
#define APP_Origin_H

#include "GeoFeatureGroupExtension.h"
#include "OriginFeature.h"

#include "QCoreApplication"

namespace App
{

/** Base class of all geometric document objects.
 */
class AppExport Origin : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::Origin);
    Q_DECLARE_TR_FUNCTIONS(App::Origin)

public:
    /// Constructor
    Origin();
    ~Origin() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "Gui::ViewProviderOrigin";
    }

    /** @name Axis and plane access
     * This functions returns casted axis and planes objects and asserts they are set correctly
     * otherwise Base::Exception is thrown.
     */
    ///@{
    // returns X axis
    App::Line *getX () const {
        return getAxis (AxisRoles[0]);
    }
    // returns Y axis
    App::Line *getY () const {
        return getAxis (AxisRoles[1]);
    }
    // returns Z axis
    App::Line *getZ () const {
        return getAxis (AxisRoles[2]);
    }

    // returns XY plane
    App::Plane *getXY () const {
        return getPlane (PlaneRoles[0]);
    }
    // returns XZ plane
    App::Plane *getXZ () const {
        return getPlane (PlaneRoles[1]);
    }
    // returns YZ plane
    App::Plane *getYZ () const {
        return getPlane (PlaneRoles[2]);
    }

    /// Returns all axis objects to iterate on them
    std::vector<App::Line *> axes() const {
        return { getX(), getY(), getZ() };
    }

    /// Returns all base planes objects to iterate on them
    std::vector<App::Plane *> planes() const {
        return { getXY(), getXZ(), getYZ() };
    }

    /// Returns all controlled objects (both planes and axis) to iterate on them
    std::vector<App::OriginFeature *> baseObjects() const {
        return { getX(), getY(), getZ(), getXY(), getXZ(), getYZ() };
    }

    /// Returns an axis by it's name
    App::OriginFeature *getOriginFeature( const char* role ) const;

    /// Returns an axis by it's name
    App::Line *getAxis( const char* role ) const;

    /// Returns an axis by it's name
    App::Plane *getPlane( const char* role ) const;
    ///@}

    /// Returns true if the given object is part of the origin
    bool hasObject (const DocumentObject *obj) const;

    /// Returns the default bounding box of the origin (use this if you confused what should be s )
    // TODO Delete me if not really needed (2015-09-01, Fat-Zer)
    static Base::BoundBox3d defaultBoundBox();

    /// Returns true on changing OriginFeature set
    short mustExecute() const override;

    /// Axis types
    static constexpr char* AxisRoles[3] = {"X_Axis", "Y_Axis", "Z_Axis"};
    /// Baseplane types
    static constexpr char* PlaneRoles[3] = {"XY_Plane", "XZ_Plane", "YZ_Plane"};

    // Axis links
    PropertyLinkList OriginFeatures;

protected:
    /// Checks integrity of the Origin
    App::DocumentObjectExecReturn *execute() override;
    /// Creates all corresponding Axes and Planes objects for the origin if they aren't linked yet
    void setupObject () override;
    /// Removes all planes and axis if they are still linked to the document
    void unsetupObject () override;

private:
    struct SetupData;
    void setupOriginFeature (App::PropertyLink &featProp, const SetupData &data);

    class OriginExtension : public GeoFeatureGroupExtension {
        Origin* obj;
    public:
        explicit OriginExtension(Origin* obj);
        void initExtension(ExtensionContainer* obj) override;
        bool extensionGetSubObject(DocumentObject *&ret, const char *subname,
                PyObject **, Base::Matrix4D *, bool, int) const override;
    };
    OriginExtension extension;
};

} //namespace App

#endif // APP_Origin_H
