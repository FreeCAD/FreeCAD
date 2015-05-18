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

namespace PartDesign
{

class PartDesignExport FeaturePrimitive : public PartDesign::FeatureAddSub
{
    PROPERTY_HEADER(PartDesign::FeaturePrimitive);

public:
    enum Type {
        Box=0,
        Cylinder,
        Sphere
    };
    
    FeaturePrimitive();
    
    virtual const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderPrimitive";
    }
    Type         getPrimitiveType() {return primitiveType;};      
    TopoDS_Shape refineShapeIfActive(const TopoDS_Shape& oldShape) const;    
    virtual void onChanged(const App::Property* prop);
    
    /// The references datum defining the primtive location
    App::PropertyLink CoordinateSystem;
    
protected:
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

} //namespace PartDesign


#endif // PARTDESIGN_FeaturePrimitive_H
