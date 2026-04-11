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

#include <functional>
#include <vector>

#include <QColor>
#include <QImage>
#include <QRect>

#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoInfo.h>

#include <Mod/Sketcher/App/GeoList.h>
#include <Mod/Sketcher/App/Constraint.h>

#include "EditModeCoinManagerParameters.h"


class SbVec3f;
class SoRayPickAction;
class SoPickedPoint;
class SbVec3s;

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
class PropertyConstraintList;
};  // namespace Sketcher

namespace SketcherGui
{

class ViewProviderSketch;

using GeoList = Sketcher::GeoList;
using GeoListFacade = Sketcher::GeoListFacade;

/** @brief      Class for managing the Edit mode coin nodes of ViewProviderSketch relating to
 * constraints.
 *  @details    To be documented.
 *
 */
class SketcherGuiExport EditModeConstraintCoinManager
{
private:
    /// Coin Node indices for constraints
    enum class ConstraintNodePosition
    {
        MaterialIndex = 0,
        DatumLabelIndex = 0,
        FirstTranslationIndex = 1,
        FirstIconIndex = 2,
        FirstConstraintIdIndex = 3,
        SecondTranslationIndex = 4,
        SecondIconIndex = 5,
        SecondConstraintIdIndex = 6
    };

public:
    explicit EditModeConstraintCoinManager(
        ViewProviderSketch& vp,
        DrawingParameters& drawingParams,
        GeometryLayerParameters& geometryLayerParams,
        ConstraintParameters& constraintParams,
        EditModeScenegraphNodes& editModeScenegraph,
        CoinMapping& coinMap
    );
    ~EditModeConstraintCoinManager();


    /** @name update coin nodes*/
    // geometry list to be used for constraints, which may be a temporal geometry
    void processConstraints(const GeoListFacade& geolistfacade);

    void updateVirtualSpace();

    /// Draw all constraint icons
    /*! Except maybe the radius and lock ones? */
    void drawConstraintIcons();

    // This specific overload is to use a specific geometry list, which may be a temporal one
    void drawConstraintIcons(const GeoListFacade& geolistfacade);
    //@}

    /** @name update coin colors*/
    //@{
    void updateConstraintColor(const std::vector<Sketcher::Constraint*>& constraints);
    //@}

    /** @name coin nodes creation*/
    void rebuildConstraintNodes();
    //@}

    /** @name change constraints selectability*/
    //@{
    void setConstraintSelectability(bool enabled = true);
    //@}

    std::set<int> detectPreselectionConstr(const SoPickedPoint* Point);

    SoSeparator* getConstraintIdSeparator(int i);

    void createEditModeInventorNodes();

private:
    void rebuildConstraintNodes(const GeoListFacade& geolistfacade);  // with specific geometry

    void rebuildConstraintNodes(
        const GeoListFacade& geolistfacade,
        const std::vector<Sketcher::Constraint*> constrlist,
        SbVec3f norm
    );

    /// finds a free position for placing a constraint icon
    Base::Vector3d seekConstraintPosition(const Base::Vector3d& norm, float step);

    /// Return display string for constraint including hiding units if
    // requested.
    QString getPresentationString(const Sketcher::Constraint* constraint, std::string prefix = "");

    /// Returns the size that Coin should display the indicated image at
    SbVec3s getDisplayedSize(const SoImage*) const;

    /** @name Protected helpers for drawing constraint icons*/
    //@{
    QString iconTypeFromConstraint(Sketcher::Constraint* constraint);

    /// Returns a QColor object appropriate for constraint with given id
    /*! In the case of combined icons, the icon color is chosen based on
     *  the constraint with the highest priority from constrColorPriority()
     */
    QColor constrColor(int constraintId);
    /// Used by drawMergedConstraintIcons to decide what color to make icons
    /*! See constrColor() */
    int constrColorPriority(int constraintId);

    // TODO: Review and refactor where these structs and types relating constraints
    // should actually go.

    // helper data structures for the constraint rendering
    std::vector<Sketcher::ConstraintType> vConstrType;

    // For each of the combined constraint icons drawn, also create a vector
    // of bounding boxes and associated constraint IDs, to go from the icon's
    // pixel coordinates to the relevant constraint IDs.
    //
    // The outside map goes from a string representation of a set of constraint
    // icons (like the one used by the constraint IDs we insert into the Coin
    // rendering tree) to a vector of those bounding boxes paired with relevant
    // constraint IDs.

    using ConstrIconBB = std::pair<QRect, std::set<int>>;
    using ConstrIconBBVec = std::vector<ConstrIconBB>;

    std::map<QString, ConstrIconBBVec> combinedConstrBoxes;


    /// Internal type used for drawing constraint icons
    struct constrIconQueueItem
    {
        /// Type of constraint the icon represents.  Eg: "small/Constraint_PointOnObject_sm"
        QString type;

        /// Internal constraint ID number
        /// These map to results of getSketchObject()->Constraints.getValues()
        int constraintId;

        /// Label to be rendered with this icon, if any
        QString label;

        /// Absolute coordinates of the constraint icon
        SbVec3f position;

        /// Pointer to the SoImage object where the icon should be written
        SoImage* destination;

        /// Pointer to SoInfo object where we store the constraint IDs that the icon refers to
        SoInfo* infoPtr;

        /// Angle to rotate an icon
        double iconRotation;

        bool visible;
    };

    using IconQueue = std::vector<constrIconQueueItem>;

    void combineConstraintIcons(IconQueue iconQueue);

    /// Renders an icon for a single constraint and sends it to Coin
    void drawTypicalConstraintIcon(const constrIconQueueItem& i);

    /// Combines multiple constraint icons and sends them to Coin
    void drawMergedConstraintIcons(IconQueue iconQueue);

    /// Helper for drawMergedConstraintIcons and drawTypicalConstraintIcon
    QImage renderConstrIcon(
        const QString& type,
        const QColor& iconColor,
        const QStringList& labels,
        const QList<QColor>& labelColors,
        double iconRotation,
        //! Gets populated with bounding boxes (in icon
        //! image coordinates) for the icon at left, then
        //! labels for different constraints.
        std::vector<QRect>* boundingBoxes = nullptr,
        //! If not NULL, gets set to the number of pixels
        //! that the text extends below the icon base.
        int* vPad = nullptr
    );

    /// Copies a QImage constraint icon into a SoImage*
    /*! Used by drawTypicalConstraintIcon() and drawMergedConstraintIcons() */
    void sendConstraintIconToCoin(const QImage& icon, SoImage* soImagePtr);

    /// Essentially a version of sendConstraintIconToCoin, with a blank icon
    void clearCoinImage(SoImage* soImagePtr);

    /// Find helper angle for radius/diameter constraint
    void findHelperAngles(
        double& helperStartAngle,
        double& helperRange,
        double angle,
        double startAngle,
        double endAngle
    );
    //@}

private:
    ViewProviderSketch& viewProvider;

    DrawingParameters& drawingParameters;
    GeometryLayerParameters& geometryLayerParameters;
    ConstraintParameters& constraintParameters;

    EditModeScenegraphNodes& editModeScenegraphNodes;

    CoinMapping& coinMapping;
};


}  // namespace SketcherGui
