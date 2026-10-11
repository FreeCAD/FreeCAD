// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Caio Venâncio <caio.venancio784@gmail.com>          *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#pragma once

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "FeatureDressUp.h"
#include "ThreadUtils.h"

namespace PartDesign
{

class PartDesignExport Thread: public DressUp
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Thread);

public:
    Thread();

    enum ThreadType
    {
        Metric,
        MetricFine
    };

    App::PropertyLinkSub LateralFace;
    App::PropertyLinkSub StartPlane;
    App::PropertyLinkSub UpToGeometry;
    App::PropertyLength Depth;
    App::PropertyLength ThreadDepth;
    App::PropertyLength ThreadDiameter;
    App::PropertyLength Diameter;
    App::PropertyEnumeration ThreadType;
    App::PropertyEnumeration ThreadTypeName;
    App::PropertyEnumeration ThreadSize;
    App::PropertyEnumeration ThreadSizePitch;
    App::PropertyEnumeration ThreadClass;
    App::PropertyEnumeration ThreadFit;
    App::PropertyEnumeration DepthType;
    App::PropertyEnumeration ThreadDirection;
    App::PropertyEnumeration ThreadDepthType;
    App::PropertyString ThreadDesignation;
    App::PropertyBool UseCustomThreadClearance;
    App::PropertyLength CustomThreadClearance;
    App::PropertyBool ModelThread;
    App::PropertyBool CosmeticThread;
    App::PropertyBool IsInternal;
    App::PropertyBool Tapered;
    App::PropertyAngle TaperedAngle;


    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn* execute() override;

    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderThread";
    }
    //@}

    struct ThreadExtent
    {
        gp_Pnt origin;
        gp_Dir direction;
        double length;
    };

    /// Resolve the nominal thread interval, including StartPlane and DepthType.
    /// Throws when the selected geometry or length specification is invalid.
    ThreadExtent resolveThreadExtent();

    Base::Vector3d guessNormalDirection() const;
    std::vector<gp_Pnt> getThreadLocations() const;
    double getThreadPitch() const;
    std::optional<gp_Pnt> getThreadOrigin() const;
    std::optional<gp_Dir> getThreadNormal() const;
    const Part::TopoShape& getReducedBasePreviewShape() const;

protected:
    void onChanged(const App::Property* prop) override;
    void updatePreviewShape() override;
    
private:
    ThreadExtent resolveThreadExtent(const Part::TopoShape& base);
    ThreadUtils threadUtils;
    Part::TopoShape reducedBasePreviewShape;
    void addThreadType();
};

}  // namespace PartDesign
