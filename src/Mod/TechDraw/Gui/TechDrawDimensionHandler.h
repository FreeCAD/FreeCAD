/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#include <string>
#include <vector>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QPixmap>

#include <Gui/Selection/Selection.h>
#include <Mod/TechDraw/App/DrawDimHelper.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "DimensionValidators.h"
#include "QGIDatumLabel.h"
#include "TechDrawHandler.h"

using namespace TechDrawGui;
using namespace TechDraw;

using DimensionType = TechDraw::DrawViewDimension::DimensionType;
using DimensionGeometry = TechDraw::DimensionGeometry;

void activateHandler(TechDrawHandler* newHandler);

class TDHandlerDimension : public TechDrawHandler,
                           public Gui::SelectionObserver
{
    
public:
    explicit TDHandlerDimension(ReferenceVector refs, TechDraw::DrawViewPart* pFeat, std::string dimType = "")
        : SelectionObserver(true)
        , initialSelection(std::move(refs))
        , partFeat(pFeat)
        , dimType(dimType)
    {
    }

    enum class AvailableDimension {
        FIRST,
        SECOND,
        THIRD,
        FOURTH,
        FIFTH,
        RESET
    };

    enum class SpecialDimension {
        LineOr2PointsDistance,
        LineOr2PointsChamfer,
        ExtendDistance,
        ChainDistance,
        CoordDistance,
        None
    };


    void activated() override;

    void deactivated() override;

    void keyPressEvent(QKeyEvent* event) override;

    void keyReleaseEvent(QKeyEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    QGIDatumLabel* getDimLabel(DrawViewDimension* d);

    void moveDimension(QPoint pos, DrawViewDimension* dim, bool textToMiddle = false, Base::Vector3d dir = Base::Vector3d(),
        Base::Vector3d delta = Base::Vector3d(), DimensionType type = DimensionType::Distance, int i = 0);


    QPointF getDimPositionToBe(QPoint pos, QPointF curPos = QPointF(), bool textToMiddle = false, Base::Vector3d dir = Base::Vector3d(),
        Base::Vector3d delta = Base::Vector3d(), DimensionType type = DimensionType::Distance, int i = 0);

    void finishDimensionMove();

    void setDimsSelectability(bool val);
    void setDimSelectability(DrawViewDimension* d, bool val);


    void mouseReleaseEvent(QMouseEvent* event) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    QString getCrosshairCursorSVGName() const override;

protected:
    SpecialDimension specialDimension = SpecialDimension::None;
    AvailableDimension availableDimension = AvailableDimension::FIRST;

    QPoint mousePos;

    ReferenceVector selPoints;
    ReferenceVector selLine;
    ReferenceVector selCircleArc;
    ReferenceVector selEllipseArc;
    ReferenceVector selSplineAndCo;
    ReferenceVector selFaces;
    ReferenceVector emptyVector;

    ReferenceEntry addedRef;
    ReferenceEntry removedRef;

    ReferenceVector initialSelection;

    TechDraw::DrawViewPart* partFeat;

    std::vector<DrawViewDimension*> dims;

    std::string dimType;

    bool blockRemoveSel = false;

    Base::Vector3d AreaLeaderPoint;
    bool hasAreaLeaderPoint = false;
    int tid = 0;

    void handleInitialSelection();

    void finalizeCommand();

    ReferenceVector& getSelectionVector(ReferenceEntry& ref);

    bool selectionEmpty();

    ReferenceVector allRefs();

    bool makeAppropriateDimension();


    bool _checkDimType(AvailableDimension dimension, std::string type);

    void makeCts_Faces(bool& selAllowed);
    void makeCts_2Point(bool& selAllowed);
    void makeCts_3Point(bool& selAllowed);
    void makeCts_4MorePoints(bool& selAllowed);
    void makeCts_1Point1Line(bool& selAllowed);
    void makeCts_1Point1Circle(bool& selAllowed);
    void makeCts_1Point1Ellipse(bool& selAllowed);
    void makeCts_1Line(bool& selAllowed);
    void makeCts_2Line(bool& selAllowed);
    void makeCts_1Line1Circle(bool& selAllowed);
    void makeCts_1Line1Ellipse(bool& selAllowed);
    void makeCts_1Circle(bool& selAllowed);
    void makeCts_2Circle(bool& selAllowed);
    void makeCts_1Ellipse(bool& selAllowed);
    void makeCts_2Ellipses(bool& selAllowed);
    void makeCts_1Spline(bool& selAllowed);
    void makeCts_1SplineAndMore(bool& selAllowed);

    void createAreaDimension(ReferenceEntry ref);
    void createRadiusDiameterDimension(ReferenceEntry ref, bool firstCstr);
    void createRadiusDimension(ReferenceEntry ref);
    void createDiameterDimension(ReferenceEntry ref);
    void createAngleDimension(ReferenceEntry ref1, ReferenceEntry ref2);
    void create3pAngleDimension(ReferenceVector refs);
    void createArcLengthDimension(ReferenceEntry ref);
    void createDistanceDimension(std::string type, ReferenceVector refs, bool chamfer = false);
    void createExtentDistanceDimension(std::string type);
    void createChainDimension(std::string type);
    void createCoordDimension(std::string type);
    
    void updateDistanceType();
    void updateExtentDistanceType();
    void updateChainDistanceType();

    bool isVerticalDistance(ReferenceVector refs);
    QPixmap icon(std::string name);
    void restartCommand(const char* cstrName);

    void clearAndRestartCommand();
    void clearRefVectors();
};