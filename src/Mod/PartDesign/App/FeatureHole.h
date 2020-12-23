/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_Hole_H
#define PARTDESIGN_Hole_H

#include <App/PropertyUnits.h>
#include "FeatureSketchBased.h"

class Property;

namespace Base {
class XMLReader;
}

namespace PartDesign
{

class PartDesignExport Hole : public ProfileBased
{
    PROPERTY_HEADER(PartDesign::Hole);

public:
    Hole();

    App::PropertyBool           Threaded;
    App::PropertyBool           ModelActualThread;
    App::PropertyLength         ThreadPitch;
    App::PropertyAngle          ThreadAngle;
    App::PropertyLength         ThreadCutOffInner;
    App::PropertyLength         ThreadCutOffOuter;
    App::PropertyEnumeration    ThreadType;
    App::PropertyEnumeration    ThreadSize;
    App::PropertyEnumeration    ThreadClass;
    App::PropertyEnumeration    ThreadFit;
    App::PropertyLength         Diameter;
    App::PropertyEnumeration    ThreadDirection;
    App::PropertyEnumeration    HoleCutType;
    App::PropertyLength         HoleCutDiameter;
    App::PropertyLength         HoleCutDepth;
    App::PropertyAngle          HoleCutCountersinkAngle;
    App::PropertyEnumeration    DepthType;
    App::PropertyLength         Depth;
    App::PropertyEnumeration    DrillPoint;
    App::PropertyAngle          DrillPointAngle;
    App::PropertyBool           Tapered;
    App::PropertyAngle          TaperedAngle;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderHole";
    }
    //@}
    short mustExecute() const;

    typedef struct {
        const char * designation;
        double diameter;
        double pitch;
    } ThreadDescription;

    static const ThreadDescription threadDescription[][171];

    virtual void Restore(Base::XMLReader & reader);

    virtual void updateProps();

protected:
    void onChanged(const App::Property* prop);
private:
    static const char* DepthTypeEnums[];
    static const char* ThreadTypeEnums[];
    static const char* ThreadFitEnums[];
    static const char* DrillPointEnums[];
    static const char* ThreadDirectionEnums[];

    /* "None" thread profile */
    static const char* HoleCutType_None_Enums[];
    static const char* ThreadSize_None_Enums[];
    static const char* ThreadClass_None_Enums[];

    /* ISO metric coarse profile */
    static const char* HoleCutType_ISOmetric_Enums[];
    static const char* ThreadSize_ISOmetric_Enums[];
    static const char* ThreadClass_ISOmetric_Enums[];

    /* ISO metric fine profile */
    static const char* HoleCutType_ISOmetricfine_Enums[];
    static const char* ThreadSize_ISOmetricfine_Enums[];
    static const char* ThreadClass_ISOmetricfine_Enums[];

    /* UNC profile */
    static const char* HoleCutType_UNC_Enums[];
    static const char* ThreadSize_UNC_Enums[];
    static const char* ThreadClass_UNC_Enums[];

    /* UNF profile */
    static const char* HoleCutType_UNF_Enums[];
    static const char* ThreadSize_UNF_Enums[];
    static const char* ThreadClass_UNF_Enums[];

    /* UNEF profile */
    static const char* HoleCutType_UNEF_Enums[];
    static const char* ThreadSize_UNEF_Enums[];
    static const char* ThreadClass_UNEF_Enums[];

    void updateHoleCutParams();
    void updateDiameterParam();
};

} //namespace PartDesign


#endif // PART_Hole_H
