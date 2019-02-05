/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef PARTDESIGN_FeaturePrimitive_H
#define PARTDESIGN_FeaturePrimitive_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#include "FeatureAddSub.h"
#include <Mod/Part/App/AttachExtension.h>

namespace PartDesign
{

class PartDesignExport FeaturePrimitive : public PartDesign::FeatureAddSub, public Part::AttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesign::FeaturePrimitive);

public:
    enum Type {
        Box=0,
        Cylinder,
        Sphere,
        Cone,
        Ellipsoid,
        Torus,
        Prism,
        Wedge
    };
    
    FeaturePrimitive();
    
    virtual const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderPrimitive";
    }
    Type         getPrimitiveType() {return primitiveType;}
    virtual void onChanged(const App::Property* prop);
    virtual PyObject* getPyObject();
    
    /// Do nothing, just to suppress warning, must be redefined in derived classes
    virtual App::DocumentObjectExecReturn* execute() {
        return PartDesign::FeatureAddSub::execute();
    }
protected:
    void handleChangedPropertyName(Base::XMLReader &reader, const char* TypeName, const char* PropName);
    //make the boolean ops with the primitives provided by the derived features
    App::DocumentObjectExecReturn* execute(const TopoDS_Shape& primitiveShape);
    Type primitiveType = Box;   
};

class PartDesignExport Box : public PartDesign::FeaturePrimitive {

    PROPERTY_HEADER(PartDesign::Box);

public:
    
    Box();
    
    App::PropertyLength Length,Height,Width;
    
    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    
protected:
    
};

class PartDesignExport AdditiveBox : public Box {
    PROPERTY_HEADER(PartDesign::AdditiveBox);
    
    AdditiveBox() {
        addSubType = FeatureAddSub::Additive;
    }
};

class PartDesignExport SubtractiveBox : public Box {
    PROPERTY_HEADER(PartDesign::SubtractiveBox);
    
    SubtractiveBox() {
        addSubType = FeatureAddSub::Subtractive;
    }
};


class PartDesignExport Cylinder : public PartDesign::FeaturePrimitive {

    PROPERTY_HEADER(PartDesign::Cylinder);

public:
    
    Cylinder();
    
    App::PropertyLength Radius;
    App::PropertyLength Height;
    App::PropertyAngle Angle;
    
    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    
protected:
    
};

class PartDesignExport AdditiveCylinder : public Cylinder {
    PROPERTY_HEADER(PartDesign::AdditiveCylinder);
    
    AdditiveCylinder() {
        addSubType = FeatureAddSub::Additive;
    }
};

class PartDesignExport SubtractiveCylinder : public Cylinder {
    PROPERTY_HEADER(PartDesign::SubtractiveCylinder);
    
    SubtractiveCylinder() {
        addSubType = FeatureAddSub::Subtractive;
    }
};


class PartDesignExport Sphere : public PartDesign::FeaturePrimitive {

    PROPERTY_HEADER(PartDesign::Sphere);

public:
    
    Sphere();
    
    App::PropertyLength Radius;
    App::PropertyAngle Angle1;
    App::PropertyAngle Angle2;
    App::PropertyAngle Angle3;
    
    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    
protected:
    
};

class PartDesignExport AdditiveSphere : public Sphere {
    PROPERTY_HEADER(PartDesign::AdditiveSphere);
    
    AdditiveSphere() {
        addSubType = FeatureAddSub::Additive;
    }
};

class PartDesignExport SubtractiveSphere : public Sphere {
    PROPERTY_HEADER(PartDesign::SubtractiveSphere);
    
    SubtractiveSphere() {
        addSubType = FeatureAddSub::Subtractive;
    }
};

class PartDesignExport Cone : public PartDesign::FeaturePrimitive {

    PROPERTY_HEADER(PartDesign::Cone);

public:
    
    Cone();
    
    App::PropertyLength Radius1;
    App::PropertyLength Radius2;
    App::PropertyLength Height;
    App::PropertyAngle  Angle;
    
    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    
protected:
    
};

class PartDesignExport AdditiveCone : public Cone {
    PROPERTY_HEADER(PartDesign::AdditiveCone);
    
    AdditiveCone() {
        addSubType = FeatureAddSub::Additive;
    }
};

class PartDesignExport SubtractiveCone : public Cone {
    PROPERTY_HEADER(PartDesign::SubtractiveCone);
    
    SubtractiveCone() {
        addSubType = FeatureAddSub::Subtractive;
    }
};


class PartDesignExport Ellipsoid : public PartDesign::FeaturePrimitive {

    PROPERTY_HEADER(PartDesign::Ellipsoid);

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
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    
protected:
    
};

class PartDesignExport AdditiveEllipsoid : public Ellipsoid {
    PROPERTY_HEADER(PartDesign::AdditiveEllipsoid);
    
    AdditiveEllipsoid() {
        addSubType = FeatureAddSub::Additive;
    }
};

class PartDesignExport SubtractiveEllipsoid : public Ellipsoid {
    PROPERTY_HEADER(PartDesign::SubtractiveEllipsoid);
    
    SubtractiveEllipsoid() {
        addSubType = FeatureAddSub::Subtractive;
    }
};


class PartDesignExport Torus : public PartDesign::FeaturePrimitive {

    PROPERTY_HEADER(PartDesign::Torus);

public:
    
    Torus();
    
    App::PropertyLength Radius1;
    App::PropertyLength Radius2;
    App::PropertyAngle Angle1;
    App::PropertyAngle Angle2;
    App::PropertyAngle Angle3;
    
    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    
protected:
    
};

class PartDesignExport AdditiveTorus : public Torus {
    PROPERTY_HEADER(PartDesign::AdditiveTorus);
    
    AdditiveTorus() {
        addSubType = FeatureAddSub::Additive;
    }
};

class PartDesignExport SubtractiveTorus : public Torus {
    PROPERTY_HEADER(PartDesign::SubtractiveTorus);
    
    SubtractiveTorus() {
        addSubType = FeatureAddSub::Subtractive;
    }
};


class PartDesignExport Prism : public PartDesign::FeaturePrimitive {

    PROPERTY_HEADER(PartDesign::Prism);

public:
    
    Prism();
    
    App::PropertyIntegerConstraint Polygon;
    App::PropertyLength Circumradius;
    App::PropertyLength Height;
    
    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    
protected:
    
};

class PartDesignExport AdditivePrism : public Prism {
    PROPERTY_HEADER(PartDesign::AdditivePrism);
    
    AdditivePrism() {
        addSubType = FeatureAddSub::Additive;
    }
};

class PartDesignExport SubtractivePrism : public Prism {
    PROPERTY_HEADER(PartDesign::SubtractivePrism);
    
    SubtractivePrism() {
        addSubType = FeatureAddSub::Subtractive;
    }
};


class PartDesignExport Wedge : public PartDesign::FeaturePrimitive {

    PROPERTY_HEADER(PartDesign::Wedge);

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
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    
protected:
    
};

class PartDesignExport AdditiveWedge : public Wedge {
    PROPERTY_HEADER(PartDesign::AdditiveWedge);
    
    AdditiveWedge() {
        addSubType = FeatureAddSub::Additive;
    }
};

class PartDesignExport SubtractiveWedge : public Wedge {
    PROPERTY_HEADER(PartDesign::SubtractiveWedge);
    
    SubtractiveWedge() {
        addSubType = FeatureAddSub::Subtractive;
    }
};

} //namespace PartDesign


#endif // PARTDESIGN_FeaturePrimitive_H
