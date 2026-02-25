// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
 *   Copyright (c) 2024 Ondsel (PL Boyer) <development@ondsel.com>         *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "GeoFeature.h"
#include "GeoFeatureGroupExtension.h"
#include "QCoreApplication"

namespace Base
{
class Rotation;
}

namespace App
{
class LocalCoordinateSystem;

class AppExport DatumElement: public App::GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::DatumElement);

public:
    /// additional information about the feature usage (e.g. "BasePlane-XY" or "Axis-X")
    PropertyString Role;

    /// Constructor
    DatumElement(bool hideRole = true);
    ~DatumElement() override;

    /// Finds the origin object this plane belongs to
    App::LocalCoordinateSystem* getLCS() const;
    Base::Vector3d getBasePoint() const;
    Base::Vector3d getDirection() const;
    Base::Vector3d getBaseDirection() const;

    bool getCameraAlignmentDirection(Base::Vector3d& directionZ, Base::Vector3d& directionX, const char* subname) const override;

    /// Returns true if this DatumElement is part of a App::Origin.
    bool isOriginFeature() const;

protected:
    void setBaseDirection(const Base::Vector3d& dir);

private:
    Base::Vector3d baseDir;
};

class AppExport Plane: public App::DatumElement
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::DatumElement);

public:
    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderPlane";
    }
};

class AppExport Line: public App::DatumElement
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::DatumElement);

public:
    Line();
    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderLine";
    }
};

class AppExport Point: public App::DatumElement
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::DatumElement);

public:
    Point();
    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderPoint";
    }
};

class AppExport LocalCoordinateSystem: public App::GeoFeature, public App::GeoFeatureGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(App::LocalCoordinateSystem);
    Q_DECLARE_TR_FUNCTIONS(App::LocalCoordinateSystem)

public:
    /// Constructor
    LocalCoordinateSystem();
    ~LocalCoordinateSystem() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderCoordinateSystem";
    }

    bool getCameraAlignmentDirection(Base::Vector3d& directionZ, Base::Vector3d& directionX, const char* subname) const override;

    /** @name Axis and plane access
     * This functions returns casted axis and planes objects and asserts they are set correctly
     * otherwise Base::Exception is thrown.
     */
    ///@{
    // returns origin point
    App::Point* getOrigin() const
    {
        return getPoint(PointRoles[0]);
    }

    // returns X axis
    App::Line* getX() const
    {
        return getAxis(AxisRoles[0]);
    }
    // returns Y axis
    App::Line* getY() const
    {
        return getAxis(AxisRoles[1]);
    }
    // returns Z axis
    App::Line* getZ() const
    {
        return getAxis(AxisRoles[2]);
    }

    // returns XY plane
    App::Plane* getXY() const
    {
        return getPlane(PlaneRoles[0]);
    }
    // returns XZ plane
    App::Plane* getXZ() const
    {
        return getPlane(PlaneRoles[1]);
    }
    // returns YZ plane
    App::Plane* getYZ() const
    {
        return getPlane(PlaneRoles[2]);
    }

    /// Returns all axis objects to iterate on them
    std::vector<App::Line*> axes() const
    {
        return {getX(), getY(), getZ()};
    }

    /// Returns all base planes objects to iterate on them
    std::vector<App::Plane*> planes() const
    {
        return {getXY(), getXZ(), getYZ()};
    }

    /// Returns all controlled objects (both planes and axis) to iterate on them
    std::vector<App::DatumElement*> baseObjects() const
    {
        return {getX(), getY(), getZ(), getXY(), getXZ(), getYZ(), getOrigin()};
    }

    /// Returns an axis by it's name
    App::DatumElement* getDatumElement(const char* role) const;

    /// Returns an axis by it's name
    App::Line* getAxis(const char* role) const;

    /// Returns an axis by it's name
    App::Plane* getPlane(const char* role) const;

    /// Returns a point by it's name
    App::Point* getPoint(const char* role) const;
    ///@}

    /// Returns true on changing DatumElement set
    short mustExecute() const override;

    /// Axis types
    static constexpr const char* AxisRoles[3] = {"X_Axis", "Y_Axis", "Z_Axis"};
    /// Baseplane types
    static constexpr const char* PlaneRoles[3] = {"XY_Plane", "XZ_Plane", "YZ_Plane"};
    /// Points types
    static constexpr const char* PointRoles[1] = {"Origin"};

    virtual bool isOrigin() const
    {
        return false;
    }

    // Axis links
    PropertyLinkList OriginFeatures;

    // GeoFeatureGroupExtension overrides:
    bool extensionGetSubObject(DocumentObject*& ret,
        const char* subname,
        PyObject**,
        Base::Matrix4D*,
        bool,
        int) const override;

    // Reimplement the hasObject because LCS doesn't use Group but stores objects in OriginFeatures for whatever reason.
    bool hasObject(const DocumentObject* obj, bool recursive = false) const override;

protected:
    /// Checks integrity of the LCS
    App::DocumentObjectExecReturn* execute() override;
    /// Creates all corresponding Axes and Planes objects for the LCS if they aren't linked yet
    void setupObject() override;
    /// Removes all planes and axis if they are still linked to the document
    void unsetupObject() override;
    void onDocumentRestored() override;

private:
    struct SetupData;
    void setupDatumElement(App::PropertyLink& featProp, const SetupData& data);

    struct SetupData
    {
        Base::Type type;
        const char* role = nullptr;
        QString label;
        Base::Rotation rot;
    };
    static const std::vector<SetupData>& getSetupData();

    DatumElement* createDatum(const SetupData& data);
    SetupData getData(const char* role);

    void migrateOriginPoint();
};

}  // namespace App
