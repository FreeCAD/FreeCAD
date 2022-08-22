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

#include "AttachExtension.h"
#include "PrismExtension.h"


namespace Part
{

class PartExport Primitive : public Part::Feature, public Part::AttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Part::Primitive);

public:
    Primitive();
    ~Primitive() override;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    PyObject* getPyObject() override;
    //@}

protected:
    void Restore(Base::XMLReader &reader) override;
    void onChanged (const App::Property* prop) override;
    void handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName) override;
    void handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop) override;
};

class PartExport Vertex : public Part::Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Vertex);

public:
    Vertex();
    ~Vertex() override;

    App::PropertyDistance X;
    App::PropertyDistance Y;
    App::PropertyDistance Z;

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    void onChanged(const App::Property*) override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderPointParametric";
    }
    //@}
};

class PartExport Line : public Part::Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Line);

public:
    Line();
    ~Line() override;

    App::PropertyDistance X1;
    App::PropertyDistance Y1;
    App::PropertyDistance Z1;
    App::PropertyDistance X2;
    App::PropertyDistance Y2;
    App::PropertyDistance Z2;

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    void onChanged(const App::Property*) override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderLineParametric";
    }
    //@}
};

class PartExport Plane : public Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Plane);

public:
    Plane();

    App::PropertyLength Length;
    App::PropertyLength Width;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderPlaneParametric";
    }
    //@}
};

class PartExport Sphere : public Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Sphere);

public:
    Sphere();

    App::PropertyLength Radius;
    App::PropertyAngle Angle1;
    App::PropertyAngle Angle2;
    App::PropertyAngle Angle3;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderSphereParametric";
    }
    //@}
};

class PartExport Ellipsoid : public Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Ellipsoid);

public:
    Ellipsoid();

    App::PropertyLength Radius1;
    App::PropertyLength Radius2;
    App::PropertyLength Radius3;
    App::PropertyAngle Angle1;
    App::PropertyAngle Angle2;
    App::PropertyAngle Angle3;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    //@}
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderEllipsoid";
    }
};

class PartExport Cylinder : public Primitive,
                            public PrismExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Cylinder);

public:
    Cylinder();

    App::PropertyLength Radius;
    App::PropertyLength Height;
    App::PropertyAngle Angle;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderCylinderParametric";
    }
    //@}
};

class PartExport Prism : public Primitive,
                         public PrismExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Prism);

public:
    Prism();

    App::PropertyIntegerConstraint Polygon;
    App::PropertyLength Circumradius;
    App::PropertyLength Height;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderPrism";
    }
    //@}
private:
    static App::PropertyIntegerConstraint::Constraints polygonRange;
};

class PartExport RegularPolygon : public Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::RegularPolygon);

public:
    RegularPolygon();

    App::PropertyIntegerConstraint Polygon;
    App::PropertyLength Circumradius;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderRegularPolygon";
    }
    //@}
private:
    static App::PropertyIntegerConstraint::Constraints polygon;
};

class PartExport Cone : public Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Cone);

public:
    Cone();

    App::PropertyLength Radius1;
    App::PropertyLength Radius2;
    App::PropertyLength Height;
    App::PropertyAngle Angle;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderConeParametric";
    }
    //@}
};

class PartExport Torus : public Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Torus);

public:
    Torus();

    App::PropertyLength Radius1;
    App::PropertyLength Radius2;
    App::PropertyAngle Angle1;
    App::PropertyAngle Angle2;
    App::PropertyAngle Angle3;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderTorusParametric";
    }
    //@}
};

class PartExport Helix : public Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Helix);

public:
    Helix();

    App::PropertyLength Pitch;
    App::PropertyLength Height;
    App::PropertyLength Radius;
    App::PropertyAngle Angle;
    App::PropertyQuantityConstraint SegmentLength;
    App::PropertyEnumeration     LocalCoord;
    App::PropertyEnumeration     Style;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderHelixParametric";
    }
    //@}

protected:
    void onChanged (const App::Property* prop) override;

private:
    static const char* LocalCSEnums[];
    static const char* StyleEnums[];
};

class PartExport Spiral : public Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Spiral);

public:
    Spiral();

    App::PropertyLength Growth;
    App::PropertyQuantityConstraint Rotations;
    App::PropertyLength Radius;
    App::PropertyQuantityConstraint SegmentLength;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderSpiralParametric";
    }
    //@}

protected:
    void onChanged (const App::Property* prop) override;
};

class PartExport Wedge : public Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Wedge);

public:
    Wedge();

    App::PropertyDistance Xmin;
    App::PropertyDistance Ymin;
    App::PropertyDistance Zmin;
    App::PropertyDistance Z2min;
    App::PropertyDistance X2min;
    App::PropertyDistance Xmax;
    App::PropertyDistance Ymax;
    App::PropertyDistance Zmax;
    App::PropertyDistance Z2max;
    App::PropertyDistance X2max;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderWedge";
    }
    //@}

protected:
    void onChanged(const App::Property* prop) override;
};

class PartExport Ellipse : public Part::Primitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Ellipse);

public:
    Ellipse();
    ~Ellipse() override;

    App::PropertyLength MajorRadius;
    App::PropertyLength MinorRadius;
    App::PropertyAngle Angle1;
    App::PropertyAngle Angle2;

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    void onChanged(const App::Property*) override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderEllipseParametric";
    }
    //@}

protected:
    void Restore(Base::XMLReader &reader) override;
    void handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName) override;

private:
    static App::PropertyQuantityConstraint::Constraints angleRange;
};

} //namespace Part


#endif // PART_PRIMITIVEFEATURE_H
