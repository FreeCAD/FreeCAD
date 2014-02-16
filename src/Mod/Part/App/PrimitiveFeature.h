/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PART_PRIMITIVEFEATURE_H
#define PART_PRIMITIVEFEATURE_H

#include <App/PropertyUnits.h>
#include "PartFeature.h"

namespace Part
{

class PartExport Primitive : public Part::Feature
{
    PROPERTY_HEADER(Part::Primitive);

public:
    Primitive();
    virtual ~Primitive();

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void) = 0;
    short mustExecute() const;
    //@}

protected:
    void onChanged (const App::Property* prop);
};

class PartExport Vertex : public Part::Primitive
{
    PROPERTY_HEADER(Part::Vertex);

public:
    Vertex();
    virtual ~Vertex();

    App::PropertyFloat X;
    App::PropertyFloat Y;
    App::PropertyFloat Z;

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    void onChanged(const App::Property*);
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderPointParametric";
    }
    //@}
};

class PartExport Line : public Part::Primitive
{
    PROPERTY_HEADER(Part::Line);

public:
    Line();
    virtual ~Line();

    App::PropertyFloat X1;
    App::PropertyFloat Y1;
    App::PropertyFloat Z1;
    App::PropertyFloat X2;
    App::PropertyFloat Y2;
    App::PropertyFloat Z2;

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    void onChanged(const App::Property*);
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderLineParametric";
    }
    //@}
};

class PartExport Plane : public Primitive
{
    PROPERTY_HEADER(Part::Plane);

public:
    Plane();

    App::PropertyLength Length;
    App::PropertyLength Width;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderPlaneParametric";
    }
    //@}
};

class PartExport Sphere : public Primitive
{
    PROPERTY_HEADER(Part::Sphere);

public:
    Sphere();

    App::PropertyFloatConstraint Radius;
    App::PropertyFloatConstraint Angle1;
    App::PropertyFloatConstraint Angle2;
    App::PropertyFloatConstraint Angle3;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderSphereParametric";
    }
    //@}
};

class PartExport Ellipsoid : public Primitive
{
    PROPERTY_HEADER(Part::Ellipsoid);

public:
    Ellipsoid();

    App::PropertyFloatConstraint Radius1;
    App::PropertyFloatConstraint Radius2;
    App::PropertyFloatConstraint Angle1;
    App::PropertyFloatConstraint Angle2;
    App::PropertyFloatConstraint Angle3;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    //@}
    virtual const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderEllipsoid";
    }
};

class PartExport Cylinder : public Primitive
{
    PROPERTY_HEADER(Part::Cylinder);

public:
    Cylinder();

    App::PropertyLength Radius;
    App::PropertyLength Height;
    App::PropertyFloatConstraint Angle;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderCylinderParametric";
    }
    //@}
};

class PartExport Prism : public Primitive
{
    PROPERTY_HEADER(Part::Prism);

public:
    Prism();

    App::PropertyIntegerConstraint Polygon;
    App::PropertyLength Circumradius;
    App::PropertyLength Height;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderPrism";
    }
    //@}
private:
    static App::PropertyIntegerConstraint::Constraints polygonRange;
};

class PartExport RegularPolygon : public Primitive
{
    PROPERTY_HEADER(Part::RegularPolygon);

public:
    RegularPolygon();

    App::PropertyIntegerConstraint Polygon;
    App::PropertyLength Circumradius;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderRegularPolygon";
    }
    //@}
private:
    static App::PropertyIntegerConstraint::Constraints polygon;
};

class PartExport Cone : public Primitive
{
    PROPERTY_HEADER(Part::Cone);

public:
    Cone();

    App::PropertyLength Radius1;
    App::PropertyLength Radius2;
    App::PropertyLength Height;
    App::PropertyFloatConstraint Angle;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderConeParametric";
    }
    //@}
};

class PartExport Torus : public Primitive
{
    PROPERTY_HEADER(Part::Torus);

public:
    Torus();

    App::PropertyFloatConstraint Radius1;
    App::PropertyFloatConstraint Radius2;
    App::PropertyFloatConstraint Angle1;
    App::PropertyFloatConstraint Angle2;
    App::PropertyFloatConstraint Angle3;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderTorusParametric";
    }
    //@}
};

class PartExport Helix : public Primitive
{
    PROPERTY_HEADER(Part::Helix);

public:
    Helix();

    App::PropertyFloatConstraint Pitch;
    App::PropertyFloatConstraint Height;
    App::PropertyFloatConstraint Radius;
    App::PropertyFloatConstraint Angle;
    App::PropertyEnumeration     LocalCoord;
    App::PropertyEnumeration     Style;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderHelixParametric";
    }
    //@}

protected:
    void onChanged (const App::Property* prop);

private:
    static const char* LocalCSEnums[];
    static const char* StyleEnums[];
};

class PartExport Spiral : public Primitive
{
    PROPERTY_HEADER(Part::Spiral);

public:
    Spiral();

    App::PropertyFloatConstraint Growth;
    App::PropertyFloatConstraint Rotations;
    App::PropertyFloatConstraint Radius;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderSpiralParametric";
    }
    //@}

protected:
    void onChanged (const App::Property* prop);
};

class PartExport Wedge : public Primitive
{
    PROPERTY_HEADER(Part::Wedge);

public:
    Wedge();

    App::PropertyFloat Xmin;
    App::PropertyFloat Ymin;
    App::PropertyFloat Zmin;
    App::PropertyFloat Z2min;
    App::PropertyFloat X2min;
    App::PropertyFloat Xmax;
    App::PropertyFloat Ymax;
    App::PropertyFloat Zmax;
    App::PropertyFloat Z2max;
    App::PropertyFloat X2max;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderWedge";
    }
    //@}

protected:
    void onChanged(const App::Property* prop);
};

class Ellipse : public Part::Primitive
{
    PROPERTY_HEADER(Part::Ellipse);

public:
    Ellipse();
    virtual ~Ellipse();

    App::PropertyFloat MajorRadius;
    App::PropertyFloat MinorRadius;
    App::PropertyAngle Angle0;
    App::PropertyAngle Angle1;

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    void onChanged(const App::Property*);
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderEllipseParametric";
    }
    //@}

private:
    static App::PropertyQuantityConstraint::Constraints angleRange;
};

} //namespace Part


#endif // PART_PRIMITIVEFEATURE_H
