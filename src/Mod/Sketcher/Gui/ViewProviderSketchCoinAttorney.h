// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include <QFont>
#include <vector>

#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/actions/SoRayPickAction.h>

#include "ViewProviderSketch.h"


namespace Base
{
template<typename T>
class Vector3;

class Vector2d;

class Placement;
}  // namespace Base

namespace Part
{
class Geometry;
}

namespace Sketcher
{
class Constraint;
class PropertyConstraintList;
};  // namespace Sketcher

namespace SketcherGui
{

class ViewProviderSketch;

using GeoList = Sketcher::GeoList;
using GeoListFacade = Sketcher::GeoListFacade;

/** @brief      Attorney class for limiting access to viewprovider
 *  @details
 *  ViewProviderSketch delegates a substantial part of coin related visualisation to
 *  ViewProviderSketchCoinAttorney during edit mode.
 *
 *  Sometimes friend classes of ViewProviderSketchCoinAttorney need to access selected
 * functionalities only available to ViewProviderSketch.
 *
 *  This attorney class regulates which specific functionalities ViewProviderSketchCoinAttorney is
 * able to access in ViewProviderSketch.
 *
 *  The objective is:
 *  - to preserve as much as possible ViewProviderSketch encapsulation
 *  - to promote as much as reasonably possible loose coupling between tightly related classes.
 *  - to keep control over the interactions between these classes and easily identify the
 * cooperation interface.
 */
class ViewProviderSketchCoinAttorney
{
private:
    static inline bool constraintHasExpression(const ViewProviderSketch& vp, int constrid);
    static inline const std::vector<Sketcher::Constraint*> getConstraints(const ViewProviderSketch& vp);
    static inline const GeoList getGeoList(const ViewProviderSketch& vp);
    static inline const GeoListFacade getGeoListFacade(const ViewProviderSketch& vp);
    static inline Base::Placement getEditingPlacement(const ViewProviderSketch& vp);
    static inline bool isShownVirtualSpace(const ViewProviderSketch& vp);
    static inline std::unique_ptr<SoRayPickAction> getRayPickAction(const ViewProviderSketch& vp);

    static inline float getScaleFactor(const ViewProviderSketch& vp);
    static inline SbVec2f getScreenCoordinates(const ViewProviderSketch& vp, SbVec2f sketchcoordinates);
    static inline QFont getApplicationFont(const ViewProviderSketch& vp);
    static inline double getRotation(const ViewProviderSketch& vp, SbVec3f pos0, SbVec3f pos1);
    static inline int defaultApplicationFontSizePixels(const ViewProviderSketch& vp);
    static inline double getDevicePixelRatio(const ViewProviderSketch& vp);
    static inline int getApplicationLogicalDPIX(const ViewProviderSketch& vp);
    static inline int getViewOrientationFactor(const ViewProviderSketch& vp);

    static inline bool isSketchInvalid(const ViewProviderSketch& vp);
    static inline bool isSketchFullyConstrained(const ViewProviderSketch& vp);
    static inline bool haveConstraintsInvalidGeometry(const ViewProviderSketch& vp);

    static inline void addNodeToRoot(ViewProviderSketch& vp, SoSeparator* node);

    static inline void removeNodeFromRoot(ViewProviderSketch& vp, SoSeparator* node);

    static inline int getPreselectPoint(const ViewProviderSketch& vp);
    static inline int getPreselectCurve(const ViewProviderSketch& vp);
    static inline int getPreselectCross(const ViewProviderSketch& vp);

    static inline bool isConstraintPreselected(const ViewProviderSketch& vp, int constraintId);
    static inline bool isPointSelected(const ViewProviderSketch& vp, int pointId);
    static inline bool isCurveSelected(const ViewProviderSketch& vp, int curveId);
    static inline bool isConstraintSelected(const ViewProviderSketch& vp, int constraintId);

    static inline void executeOnSelectionPointSet(
        const ViewProviderSketch& vp,
        std::function<void(const int)>&& operation
    );

    friend class EditModeCoinManager;
    friend class EditModeConstraintCoinManager;
    friend class EditModeGeometryCoinManager;
    friend class EditModeInformationOverlayCoinConverter;
    friend class EditModeGeometryCoinConverter;
};

inline bool ViewProviderSketchCoinAttorney::constraintHasExpression(
    const ViewProviderSketch& vp,
    int constrid
)
{
    return vp.constraintHasExpression(constrid);
};

inline const std::vector<Sketcher::Constraint*> ViewProviderSketchCoinAttorney::getConstraints(
    const ViewProviderSketch& vp
)
{
    return vp.getConstraints();
}

inline const GeoList ViewProviderSketchCoinAttorney::getGeoList(const ViewProviderSketch& vp)
{
    return vp.getGeoList();
}

const GeoListFacade ViewProviderSketchCoinAttorney::getGeoListFacade(const ViewProviderSketch& vp)
{
    return vp.getGeoListFacade();
}

inline Base::Placement ViewProviderSketchCoinAttorney::getEditingPlacement(const ViewProviderSketch& vp)
{
    return vp.getEditingPlacement();
}

inline bool ViewProviderSketchCoinAttorney::isShownVirtualSpace(const ViewProviderSketch& vp)
{
    return vp.viewProviderParameters.isShownVirtualSpace;
}

inline std::unique_ptr<SoRayPickAction> ViewProviderSketchCoinAttorney::getRayPickAction(
    const ViewProviderSketch& vp
)
{
    return vp.getRayPickAction();
}

inline float ViewProviderSketchCoinAttorney::getScaleFactor(const ViewProviderSketch& vp)
{
    return vp.getScaleFactor();
}

inline SbVec2f ViewProviderSketchCoinAttorney::getScreenCoordinates(
    const ViewProviderSketch& vp,
    SbVec2f sketchcoordinates
)
{
    return vp.getScreenCoordinates(sketchcoordinates);
}

inline QFont ViewProviderSketchCoinAttorney::getApplicationFont(const ViewProviderSketch& vp)
{
    return vp.getApplicationFont();
}

inline double ViewProviderSketchCoinAttorney::getRotation(
    const ViewProviderSketch& vp,
    SbVec3f pos0,
    SbVec3f pos1
)
{
    return vp.getRotation(pos0, pos1);
}

inline int ViewProviderSketchCoinAttorney::defaultApplicationFontSizePixels(const ViewProviderSketch& vp)
{
    return vp.defaultFontSizePixels();
}

inline double ViewProviderSketchCoinAttorney::getDevicePixelRatio(const ViewProviderSketch& vp)
{
    return vp.getDevicePixelRatio();
}

inline int ViewProviderSketchCoinAttorney::getApplicationLogicalDPIX(const ViewProviderSketch& vp)
{
    return vp.getApplicationLogicalDPIX();
}

inline int ViewProviderSketchCoinAttorney::getViewOrientationFactor(const ViewProviderSketch& vp)
{
    return vp.getViewOrientationFactor();
}

inline bool ViewProviderSketchCoinAttorney::isSketchInvalid(const ViewProviderSketch& vp)
{
    return vp.isSketchInvalid();
}

inline bool ViewProviderSketchCoinAttorney::isSketchFullyConstrained(const ViewProviderSketch& vp)
{
    return vp.isSketchFullyConstrained();
}

inline bool ViewProviderSketchCoinAttorney::haveConstraintsInvalidGeometry(const ViewProviderSketch& vp)
{
    return vp.haveConstraintsInvalidGeometry();
}

inline void ViewProviderSketchCoinAttorney::addNodeToRoot(ViewProviderSketch& vp, SoSeparator* node)
{
    vp.addNodeToRoot(node);
}

inline void ViewProviderSketchCoinAttorney::removeNodeFromRoot(ViewProviderSketch& vp, SoSeparator* node)
{
    vp.removeNodeFromRoot(node);
}

inline int ViewProviderSketchCoinAttorney::getPreselectPoint(const ViewProviderSketch& vp)
{
    return vp.getPreselectPoint();
}

inline int ViewProviderSketchCoinAttorney::getPreselectCurve(const ViewProviderSketch& vp)
{
    return vp.getPreselectCurve();
}

inline int ViewProviderSketchCoinAttorney::getPreselectCross(const ViewProviderSketch& vp)
{
    return vp.getPreselectCross();
}

inline bool ViewProviderSketchCoinAttorney::isConstraintPreselected(
    const ViewProviderSketch& vp,
    int constraintId
)
{
    return vp.isConstraintPreselected(constraintId);
}

inline bool ViewProviderSketchCoinAttorney::isPointSelected(const ViewProviderSketch& vp, int pointId)
{
    return vp.isPointSelected(pointId);
}

inline bool ViewProviderSketchCoinAttorney::isCurveSelected(const ViewProviderSketch& vp, int curveId)
{
    return vp.isCurveSelected(curveId);
}

inline bool ViewProviderSketchCoinAttorney::isConstraintSelected(
    const ViewProviderSketch& vp,
    int constraintId
)
{
    return vp.isConstraintSelected(constraintId);
}

inline void ViewProviderSketchCoinAttorney::executeOnSelectionPointSet(
    const ViewProviderSketch& vp,
    std::function<void(const int)>&& operation
)
{
    vp.executeOnSelectionPointSet(std::move(operation));
}

}  // namespace SketcherGui
