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


namespace App
{

/** Base class of all geometric document objects.
 */
class AppExport Origin : public App::DocumentObject
{
    PROPERTY_HEADER(App::Origin);

public:
    /// Constructor
    Origin(void);
    virtual ~Origin();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "Gui::ViewProviderOrigin";
    }

    /** @name Axis and plane access
     * This functions returns casted axis and planes objects and asserts they are set correctly
     * otherwise Base::Exception is thrown.
     */
    ///@{
    // returns X axis
    App::Line *getX () const {
        return getAxis ("X_Axis");
    }
    // returns Y axis
    App::Line *getY () const {
        return getAxis ("Y_Axis");
    }
    // returns Z axis
    App::Line *getZ () const {
        return getAxis ("Z_Axis");
    }

    // returns XY plane
    App::Plane *getXY () const {
        return getPlane ("XY_Plane");
    }
    // returns XZ plane
    App::Plane *getXZ () const {
        return getPlane ("XZ_Plane");
    }
    // returns YZ plane
    App::Plane *getYZ () const {
        return getPlane ("YZ_Plane");
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
    virtual short mustExecute(void) const;

    /// Axis types
    static const char* AxisRoles[3];
    /// Baseplane types
    static const char* PlaneRoles[3];

    // Axis links
    PropertyLinkList OriginFeatures;

protected:
    /// Checks integrity of the Origin
    virtual App::DocumentObjectExecReturn *execute(void);
    /// Creates all corresponding Axes and Planes objects for the origin if they aren't linked yet
    virtual void setupObject ();
    /// Removes all planes and axis if they are still linked to the document
    virtual void unsetupObject ();

private:
    struct SetupData;
    void setupOriginFeature (App::PropertyLink &featProp, const SetupData &data);

    class OriginExtension : public GeoFeatureGroupExtension {
        Origin* obj;
    public:
        OriginExtension(Origin* obj);
        void initExtension(ExtensionContainer* obj);
        bool extensionGetSubObject(DocumentObject *&ret, const char *subname,
                PyObject **, Base::Matrix4D *, bool, int) const;
    };
    OriginExtension extension;
};

} //namespace App

#endif // APP_Origin_H
