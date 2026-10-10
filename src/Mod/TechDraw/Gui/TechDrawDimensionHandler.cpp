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


#include "TechDrawDimensionHandler.h"
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include <QApplication>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QPixmap>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/TechDraw/App/DrawDimHelper.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/LandmarkDimension.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "CommandExtensionDims.h"
#include "DimensionValidators.h"
#include "DrawGuiUtil.h"
#include "QGIDatumLabel.h"
#include "QGIViewDimension.h"
#include "QGVPage.h"
#include "MDIViewPage.h"
#include "TaskDimRepair.h"
#include "TaskLinkDim.h"
#include "TaskSelectLineAttributes.h"
#include "TechDrawHandler.h"
#include "ViewProviderDimension.h"
#include "ViewProviderDrawingView.h"

using namespace std;

// Free functions defined (and shared with other Dimension commands) in CommandCreateDims.cpp
DrawViewDimension* dimMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                            ReferenceVector references2d, ReferenceVector references3d);
void positionDimText(DrawViewDimension* dim, int indexOffset = 0);

class GeomSelectionSizes
{
public:
    GeomSelectionSizes(size_t s_pts, size_t s_lns, size_t s_cir, size_t s_ell, size_t s_spl, size_t s_fcs) :
        s_pts(s_pts), s_lns(s_lns), s_cir(s_cir), s_ell(s_ell), s_spl(s_spl), s_fcs(s_fcs) {}
    ~GeomSelectionSizes() {}

    bool hasPoints()        const { return s_pts > 0; }
    bool hasLines()         const { return s_lns > 0; }
    bool hasCirclesOrArcs() const { return s_cir > 0; }
    bool hasEllipseAndCo()  const { return s_ell > 0; }
    bool hasSplineAndCo()   const { return s_spl > 0; }
    bool hasFaces()         const { return s_fcs > 0; }

    bool has1Face()              const { return s_pts == 0 && s_lns == 0 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 1; }
    bool hasFacesOnly()          const { return s_pts == 0 && s_lns == 0 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs >  0; }

    bool has1Point()             const { return s_pts == 1 && s_lns == 0 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has2Points()            const { return s_pts == 2 && s_lns == 0 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has1Point1Line()        const { return s_pts == 1 && s_lns == 1 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has3Points()            const { return s_pts == 3 && s_lns == 0 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has4MorePoints()        const { return s_pts >= 4 && s_lns == 0 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has2Points1Line()       const { return s_pts == 2 && s_lns == 1 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has3MorePoints1Line()   const { return s_pts >= 3 && s_lns == 1 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has1Point1Circle()      const { return s_pts == 1 && s_lns == 0 && s_cir == 1 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has1Point1Ellipse()     const { return s_pts == 1 && s_lns == 0 && s_cir == 0 && s_ell == 1 && s_spl == 0 && s_fcs == 0; }

    bool has1Line()              const { return s_pts == 0 && s_lns == 1 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has2Lines()             const { return s_pts == 0 && s_lns == 2 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has3MoreLines()         const { return s_pts == 0 && s_lns >= 3 && s_cir == 0 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has1Line1Circle()       const { return s_pts == 0 && s_lns == 1 && s_cir == 1 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has1Line2Circles()      const { return s_pts == 0 && s_lns == 1 && s_cir == 2 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has1Line1Ellipse()      const { return s_pts == 0 && s_lns == 1 && s_cir == 0 && s_ell == 1 && s_spl == 0 && s_fcs == 0; }

    bool has1Circle()            const { return s_pts == 0 && s_lns == 0 && s_cir == 1 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has2Circles()           const { return s_pts == 0 && s_lns == 0 && s_cir == 2 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has3MoreCircles()       const { return s_pts == 0 && s_lns == 0 && s_cir >= 3 && s_ell == 0 && s_spl == 0 && s_fcs == 0; }
    bool has1Circle1Ellipse()    const { return s_pts == 0 && s_lns == 0 && s_cir == 1 && s_ell == 1 && s_spl == 0 && s_fcs == 0; }

    bool has1Ellipse()           const { return s_pts == 0 && s_lns == 0 && s_cir == 0 && s_ell == 1 && s_spl == 0 && s_fcs == 0; }
    bool has2Ellipses()          const { return s_pts == 0 && s_lns == 0 && s_cir == 0 && s_ell == 2 && s_spl == 0 && s_fcs == 0; }

    bool has1Spline()            const { return s_pts == 0 && s_lns == 0 && s_cir == 0 && s_ell == 0 && s_spl == 1 && s_fcs == 0; }
    bool has1SplineAndMore()     const { return s_spl >= 1 && s_fcs == 0; }

    size_t s_pts, s_lns, s_cir, s_ell, s_spl, s_fcs;
};

void TDHandlerDimension::activated()
{
    auto* mdi = qobject_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
    if (mdi) {
        mdi->setDimensionsSelectability(false);
    }
    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::GreedySelection);

    tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Insert dimension"));

    handleInitialSelection();
}

void TDHandlerDimension::deactivated()
{
    auto* mdi = qobject_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
    if (mdi) {
        mdi->setDimensionsSelectability(true);
    }
    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
    Gui::Command::abortCommand(tid);
}

void TDHandlerDimension::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_M && !selectionEmpty()) {
        if (dimType != "") {
            if (dimType == "3ptAngle_1") {
                dimType = "3ptAngle_2";
            }
            else if (dimType == "3ptAngle_2") {
                dimType = "3ptAngle_3";
            }
            else if (dimType == "3ptAngle_3") {
                dimType = "3ptAngle_1";
            }
            else {
                return;
            }
        }

        if (availableDimension == AvailableDimension::FIRST) {
            availableDimension = AvailableDimension::SECOND;
        }
        else if (availableDimension == AvailableDimension::SECOND) {
            availableDimension = AvailableDimension::THIRD;
        }
        else if (availableDimension == AvailableDimension::THIRD) {
            availableDimension = AvailableDimension::FOURTH;
        }
        else if (availableDimension == AvailableDimension::FOURTH) {
            availableDimension = AvailableDimension::FIFTH;
        }
        else if (availableDimension == AvailableDimension::FIFTH || availableDimension == AvailableDimension::RESET) {
            availableDimension = AvailableDimension::FIRST;
        }
        makeAppropriateDimension();
        event->accept();
    }
    else if (event->key() == Qt::Key_Z && (QApplication::keyboardModifiers() & Qt::ControlModifier)) {
        // User trying to cancel with Ctrl-Z
        quit();
        event->accept();
    }
}
void TDHandlerDimension::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Z && (QApplication::keyboardModifiers() & Qt::ControlModifier)) {
        // User trying to cancel with Ctrl-Z
        quit();
        event->accept();
    }
    else {
        TechDrawHandler::keyReleaseEvent(event);
    }
}

void TDHandlerDimension::mouseMoveEvent(QMouseEvent* event)
{
    mousePos = event->pos();

    if (dims.empty()){
        return;
    }

    bool textToMiddle = false;
    Base::Vector3d dirMaster, delta;
    //Change distance dimension based on position of mouse.
    if (specialDimension == SpecialDimension::LineOr2PointsDistance
        || specialDimension == SpecialDimension::LineOr2PointsChamfer){
        updateDistanceType();
    }
    else if (specialDimension == SpecialDimension::ExtendDistance){
        updateExtentDistanceType();
    }
    else if (specialDimension == SpecialDimension::ChainDistance || specialDimension == SpecialDimension::CoordDistance){
        if (dimType.empty()) {
            updateChainDistanceType();
        }
        textToMiddle = true;
        pointPair pp = dims[0]->getLinearPoints();
        dirMaster = pp.second() - pp.first();
        //dirMaster.y = -dirMaster.y; // not needed because y is reversed between property X/Y and scenePositions

        QPointF firstPos = getDimLabel(dims[0])->pos();
        Base::Vector3d pMaster(firstPos.x(), firstPos.y(), 0.0);
        Base::Vector3d ipDelta = DrawUtil::getTrianglePoint(pMaster, dirMaster, Base::Vector3d());
        delta = ipDelta.Normalize() * Rez::guiX(activeDimAttributes.getCascadeSpacing());
    }

    int i = 0;
    for (auto* dim : dims) {
        auto dimType = static_cast<DimensionType>(dim->Type.getValue());
        moveDimension(mousePos, dim, textToMiddle, dirMaster, delta, dimType, i);

        if (specialDimension == SpecialDimension::CoordDistance) {
            i++;
        }
    }
}

QGIDatumLabel* TDHandlerDimension::getDimLabel(DrawViewDimension* d)
{
    auto* vp = freecad_cast<ViewProviderDimension*>(Gui::Application::Instance->getViewProvider(d));
    if (!vp) {
        return nullptr;
    }
    auto* qgivDimension(dynamic_cast<QGIViewDimension*>(vp->getQView()));
    if (!qgivDimension) {
        return nullptr;
    }
    return qgivDimension->getDatumLabel();
}

void TDHandlerDimension::moveDimension(QPoint pos, DrawViewDimension* dim, bool textToMiddle, Base::Vector3d dir,
    Base::Vector3d delta, DimensionType type, int i)
{
    if (!dim) { return; }
    auto label = getDimLabel(dim);
    if (!label) { return; }

    label->setPos(getDimPositionToBe(pos, label->pos(), textToMiddle, dir, delta, type, i));
}
QPointF TDHandlerDimension::getDimPositionToBe(QPoint pos, QPointF curPos, bool textToMiddle, Base::Vector3d dir,
    Base::Vector3d delta, DimensionType type, int i)
{
    auto* vpp = freecad_cast<ViewProviderDrawingView*>(Gui::Application::Instance->getViewProvider(partFeat));
    if (!vpp) { return QPointF(); }


    QPointF scenePos = viewPage->mapToScene(pos) - vpp->getQView()->scenePos();

    if (textToMiddle) {
        // delta is for coord distances. i = 0 when it's a chain so delta is ignored.
        float dimDistance = Rez::guiX(activeDimAttributes.getCascadeSpacing());
        if (type == DimensionType::Distance) {
            Base::Vector3d pos3d(scenePos.x(), scenePos.y(), 0.0);
            float xDim = curPos.x();
            float yDim = curPos.y();
            Base::Vector3d pDim(xDim, yDim, 0.0);
            Base::Vector3d p3 = DrawUtil::getTrianglePoint(pos3d, dir, pDim);
            p3 = p3 + delta * i;
            scenePos = QPointF(p3.x, p3.y);
        }
        else if(type == DimensionType::DistanceX) {
            pointPair pp = dims[0]->getLinearPoints();
            if (Rez::guiX(pp.first().y) > scenePos.y())
                dimDistance = -dimDistance;

            double y = static_cast<double>(scenePos.y()) + i * static_cast<double>(dimDistance);
            scenePos = QPointF(curPos.x(), y);
        }
        else if(type == DimensionType::DistanceY) {
            pointPair pp = dims[0]->getLinearPoints();
            if (Rez::guiX(pp.first().x) > scenePos.x())
                dimDistance = -dimDistance;

            double x = static_cast<double>(scenePos.x()) + i * static_cast<double>(dimDistance);
            scenePos = QPointF(x, curPos.y());
        }
    }

    return scenePos;
}
void TDHandlerDimension::finishDimensionMove()
{
    for (auto* dim : dims) {
        auto label = getDimLabel(dim);
        double x = Rez::appX(label->X()), y = Rez::appX(label->Y());
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.X = %f",
            dim->getNameInDocument(), x);
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.Y = %f",
            dim->getNameInDocument(), -y);
    }
}

void TDHandlerDimension::setDimsSelectability(bool val)
{
    for (auto dim : dims) {
        setDimSelectability(dim, val);
    }
}
void TDHandlerDimension::setDimSelectability(DrawViewDimension* d, bool val)
{
    QGIDatumLabel* label = getDimLabel(d);
    if (label) {
        label->setSelectability(val);
    }
}

void TDHandlerDimension::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        if (!dims.empty()) {
            Gui::Selection().clearSelection();
            clearAndRestartCommand();
            event->accept();
        }
        else {
            TechDrawHandler::mouseReleaseEvent(event);
        }
        return;
    }
    else  if (event->button() == Qt::LeftButton) {
        mousePos = event->pos();

        bool finalize = true;

        if (removedRef.hasGeometry()) {
            finalize = false;

            ReferenceVector& selVector = getSelectionVector(removedRef);
            selVector.erase(std::remove(selVector.begin(), selVector.end(), removedRef), selVector.end());

            if (!selectionEmpty()) {
                availableDimension = AvailableDimension::FIRST;
                makeAppropriateDimension();
            }
            else {
                clearAndRestartCommand();
            }
            removedRef = ReferenceEntry();
        }

        if (addedRef.hasGeometry()) {
            finalize = false;
            //add the geometry to its type vector. Temporarily if not selAllowed
            if (addedRef.getSubName() == "") {
                // Behavior deactivated for now because I found it annoying.
                // To reactivate replace addedRef.hasGeometry() by addedRef.getObject() above.
                // This means user selected the view itself.
                if (selectionEmpty()) {
                    restartCommand(QT_TRANSLATE_NOOP("Command", "Add Extent dimension"));
                    createExtentDistanceDimension("DistanceX");
                }
            }
            else {
                ReferenceVector& selVector = getSelectionVector(addedRef);
                selVector.push_back(addedRef);
                
                if (DrawUtil::getGeomTypeFromName(addedRef.getSubName())
                    == "Face") {
                    
                    QPointF p = getDimPositionToBe(mousePos);
                    AreaLeaderPoint = Base::Vector3d(Rez::appX(p.x()), Rez::appX(p.y()), 0.0);
                    hasAreaLeaderPoint = true;
                }

                availableDimension = AvailableDimension::FIRST;
                bool selAllowed = makeAppropriateDimension();

                if (!selAllowed) {
                    // remove from selection
                    blockRemoveSel = true;

                    Gui::Selection().rmvSelection(addedRef.getObject()->getDocument()->getName(),
                        addedRef.getObject()->getNameInDocument(), addedRef.getSubName().c_str());
                    blockRemoveSel = false;

                    selVector.erase(std::remove(selVector.begin(), selVector.end(), addedRef), selVector.end());

                    if (selVector == selFaces) {
                        // if sel face and not allowed, then a dimension is being created
                        // and user clicked on a face to drop it.
                        // Better would be to disable face selectability when needed.
                        finalize = true;
                    }
                }
            }
            addedRef = ReferenceEntry();
        }


        // Finalize if click on empty space.
        if (finalize && !dims.empty()) {
            finalizeCommand();
        }
    }
}

void TDHandlerDimension::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        return;
    }

    App::Document* pageDoc = nullptr;
    if (auto page = getPage()) {
        pageDoc = page->getDocument();
    }
    if (msg.Object.getObjectName().empty()
        || (msg.Object.getDocument() != pageDoc)) {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            Gui::Selection().rmvSelection(msg.pDocName, msg.pObjectName, msg.pSubName);
        }
        return;
    }

    App::DocumentObject* obj = msg.Object.getObject();
    if (!obj) {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            Gui::Selection().rmvSelection(msg.pDocName, msg.pObjectName, msg.pSubName);
        }
        return;
    }

    auto* dvp = dynamic_cast<TechDraw::DrawViewPart*>(obj);
    if (!dvp) {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            Gui::Selection().rmvSelection(msg.pDocName, msg.pObjectName, msg.pSubName);
        }
        return;
    }

    if (partFeat && partFeat != dvp) {
        // Dimensions can only be within one view.

        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            Gui::Selection().rmvSelection(msg.pDocName, msg.pObjectName, msg.pSubName);
        }
        return;
    }
    else {
        partFeat = dvp;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        addedRef = ReferenceEntry(dvp, msg.pSubName);
    }
    else if (msg.Type == Gui::SelectionChanges::RmvSelection) {
        if (!blockRemoveSel) {
            removedRef = ReferenceEntry(dvp, msg.pSubName);
        }
    }
}

QString TDHandlerDimension::getCrosshairCursorSVGName() const
{
    if (dimType == "") return QStringLiteral("TechDraw_Dimension_Pointer");
    else if (dimType == "LengthFree") return QStringLiteral("TechDraw_LengthDimension_Pointer");
    else if (dimType == "VerticalLength") return QStringLiteral("TechDraw_VerticalDimension_Pointer");
    else if (dimType == "HorizontalLength") return QStringLiteral("TechDraw_HorizontalDimension_Pointer");
    else if (dimType == "Radius") return QStringLiteral("TechDraw_RadiusDimension_Pointer");
    else if (dimType == "Diameter") return QStringLiteral("TechDraw_DiameterDimension_Pointer");
    else if (dimType == "Angle") return QStringLiteral("TechDraw_AngleDimension_Pointer");
    else if (dimType == "3ptAngle_1" || dimType == "3ptAngle_2" || dimType == "3ptAngle_3")
        return QStringLiteral("TechDraw_3PtAngleDimension_Pointer");
    else if (dimType == "Area") return QStringLiteral("TechDraw_AreaDimension_Pointer");
    else if (dimType == "ArcLength") return QStringLiteral("TechDraw_LengthArcDimension_Pointer");
    else if (dimType == "HorizontalExtent") return QStringLiteral("TechDraw_HorizontalExtentDimension_Pointer");
    else if (dimType == "VerticalExtent") return QStringLiteral("TechDraw_VerticalExtentDimension_Pointer");
    else if (dimType == "HorizontalChainDimension") return QStringLiteral("TechDraw_HorizontalChainDimension_Pointer");
    else if (dimType == "VerticalChainDimension") return QStringLiteral("TechDraw_VerticalChainDimension_Pointer");
    else if (dimType == "ObliqueChainDimension") return QStringLiteral("TechDraw_ObliqueChainDimension_Pointer");
    else if (dimType == "HorizontalCoordDimension") return QStringLiteral("TechDraw_HorizontalCoordDimension_Pointer");
    else if (dimType == "VerticalCoordDimension") return QStringLiteral("TechDraw_VerticalCoordDimension_Pointer");
    else if (dimType == "ObliqueCoordDimension") return QStringLiteral("TechDraw_ObliqueCoordDimension_Pointer");
    else if (dimType == "HorizontalChamfer") return QStringLiteral("TechDraw_HorizontalChamferDimension_Pointer");
    else if (dimType == "VerticalChamfer") return QStringLiteral("TechDraw_VerticalChamferDimension_Pointer");

    return QStringLiteral("TechDraw_Dimension_Pointer");
}

void TDHandlerDimension::handleInitialSelection()
{
    if (initialSelection.size() == 0) {
        return;
    }

    availableDimension = AvailableDimension::FIRST;

    partFeat = dynamic_cast<TechDraw::DrawViewPart*>(initialSelection[0].getObject());
    if (!partFeat) { return; }

    // Add the selected elements to their corresponding selection vectors
    for (auto& ref : initialSelection) {
        ReferenceVector& selVector = getSelectionVector(ref);
        selVector.push_back(ref);
    }

    // See if the selection is valid
    bool selAllowed = makeAppropriateDimension();

    if (!selAllowed) {
        clearRefVectors();
    }
}

void TDHandlerDimension::finalizeCommand()
{
    finishDimensionMove();

    // Ask for the value of datum dimensions
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/TechDraw");

    Gui::Command::commitCommand(tid);

    // Touch the parent feature so the dimension in tree view appears as a child
    partFeat->touch(true);

    // This code enables the continuous creation mode.
    bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);
    if (continuousMode) {
        Gui::Selection().clearSelection();
        clearAndRestartCommand();
    }
    else {
        viewPage->deactivateHandler(); // no code after this line, Handler get deleted in QGVPage
    }
}

ReferenceVector& TDHandlerDimension::getSelectionVector(ReferenceEntry& ref)
{
    std::string subName = ref.getSubName();
    if (subName == "") {
        return emptyVector;
    }

    auto* dvp = static_cast<TechDraw::DrawViewPart*>(ref.getObject());

    std::string geomName = DrawUtil::getGeomTypeFromName(subName);
    if (geomName == "Face") {
        return selFaces;
    }
    else if (geomName == "Edge") {

        int GeoId(TechDraw::DrawUtil::getIndexFromName(subName));
        TechDraw::BaseGeomPtr geom = dvp->getGeomByIndex(GeoId);
        if (!geom) {
            return emptyVector;
        }

        if (geom->getGeomType() == GeomType::GENERIC) {
            TechDraw::GenericPtr gen1 = std::static_pointer_cast<TechDraw::Generic>(geom);
            if (gen1->points.size() < 2) {
                return emptyVector;
            }
            return selLine;
        }
        else if (geom->getGeomType() == GeomType::CIRCLE || geom->getGeomType() == GeomType::ARCOFCIRCLE) {
            return selCircleArc;
        }
        else if (geom->getGeomType() == GeomType::ELLIPSE || geom->getGeomType() == GeomType::ARCOFELLIPSE) {
            return selEllipseArc;
        }
        else if (geom->getGeomType() == GeomType::BSPLINE) {
            return selSplineAndCo;
        }
    }
    else if (geomName == "Vertex") {
        return selPoints;
    }

    return emptyVector;
}

bool TDHandlerDimension::selectionEmpty()
{
    return selPoints.empty() && selLine.empty() && selCircleArc.empty() && selEllipseArc.empty() && selSplineAndCo.empty() && selFaces.empty();
}

ReferenceVector TDHandlerDimension::allRefs()
{
    ReferenceVector result;

    result.reserve(selPoints.size() + selLine.size() + selCircleArc.size() + selEllipseArc.size() + selSplineAndCo.size() + selFaces.size());

    // Append each vector to result
    result.insert(result.end(), selPoints.begin(), selPoints.end());
    result.insert(result.end(), selLine.begin(), selLine.end());
    result.insert(result.end(), selCircleArc.begin(), selCircleArc.end());
    result.insert(result.end(), selEllipseArc.begin(), selEllipseArc.end());
    result.insert(result.end(), selSplineAndCo.begin(), selSplineAndCo.end());
    result.insert(result.end(), selFaces.begin(), selFaces.end());

    return result;
}

bool TDHandlerDimension::makeAppropriateDimension() {
    bool selAllowed = false;

    GeomSelectionSizes selection(selPoints.size(), selLine.size(), selCircleArc.size(), selEllipseArc.size(), selSplineAndCo.size(), selFaces.size());

    // if we drop a dimension text on a face, interpreting selection with hasFaces() will cause
    // us to try to create an area dim instead of positioning the text.  Since we do not have a
    // dimension type that involves faces + some other geometry, we must use hasFacesOnly() to
    // determine if we are creating an area.
    if (selection.hasFacesOnly()) {
        makeCts_Faces(selAllowed);
        if (!selection.has1Face()) {
            Base::Console().warning("Multiple faces are selected. Using first.\n");
        }
    }
    else if (selection.hasPoints()) {
        if (selection.has1Point()) { selAllowed = true; }
        else if (selection.has2Points()) { makeCts_2Point(selAllowed); }
        else if (selection.has3Points()) { makeCts_3Point(selAllowed); }
        else if (selection.has4MorePoints()) { makeCts_4MorePoints(selAllowed); }
        else if (selection.has1Point1Line()) { makeCts_1Point1Line(selAllowed); }
        else if (selection.has1Point1Circle()) { makeCts_1Point1Circle(selAllowed); }
        else if (selection.has1Point1Ellipse()) { makeCts_1Point1Ellipse(selAllowed); }
    }
    else if (selection.hasLines()) {
        if (selection.has1Line()) { makeCts_1Line(selAllowed); }
        else if (selection.has2Lines()) { makeCts_2Line(selAllowed); }
        else if (selection.has1Line1Circle()) { makeCts_1Line1Circle(selAllowed); }
        else if (selection.has1Line1Ellipse()) { makeCts_1Line1Ellipse(selAllowed); }
    }
    else if (selection.hasCirclesOrArcs()) {
        if (selection.has1Circle()) { makeCts_1Circle(selAllowed); }
        else if (selection.has2Circles()) { makeCts_2Circle(selAllowed); }
    }
    else if (selection.hasEllipseAndCo()) {
        if (selection.has1Ellipse()) { makeCts_1Ellipse(selAllowed); }
        if (selection.has2Ellipses()) { makeCts_2Ellipses(selAllowed); }
    }
    else if (selection.hasSplineAndCo()) {
        if (selection.has1Spline()) { makeCts_1Spline(selAllowed); }
        if (selection.has1SplineAndMore()) { makeCts_1SplineAndMore(selAllowed); }
    }

    // Make created constraints unselectable.
    if (selAllowed) {
        setDimsSelectability(false);
    }

    return selAllowed;
}

bool TDHandlerDimension::_checkDimType(AvailableDimension dimension, std::string type) {
    if (dimType == "") {
        return availableDimension == dimension;
    }
    return dimType == type;
}

void TDHandlerDimension::makeCts_Faces(bool& selAllowed)
{
    //area
    if (_checkDimType(AvailableDimension::FIRST, "Area")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add area dimension"));
        createAreaDimension(selFaces[0]);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_2Point(bool& selAllowed)
{
    if (dimType == "3ptAngle_1" || dimType == "3ptAngle_2" || dimType == "3ptAngle_3"
        || dimType == "HorizontalChainDimension" || dimType == "VerticalChainDimension" || dimType == "ObliqueChainDimension"
        || dimType == "HorizontalCoordDimension" || dimType == "VerticalCoordDimension" || dimType == "ObliqueCoordDimension") {
        selAllowed = true;
        return;
    }

    //distance
    if (_checkDimType(AvailableDimension::FIRST, "LengthFree")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add distance dimension"));
        createDistanceDimension("Distance", { selPoints[0], selPoints[1] });
        specialDimension = SpecialDimension::LineOr2PointsDistance;
        selAllowed = true;
        if (!isVerticalDistance({ selPoints[0], selPoints[1] })) {
            availableDimension = AvailableDimension::RESET;
        }
    }
    else if (dimType == "HorizontalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal distance dimension"));
        createDistanceDimension("DistanceX", { selPoints[0], selPoints[1] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical distance dimension"));
        createDistanceDimension("DistanceY", { selPoints[0], selPoints[1] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "HorizontalChamfer") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal chamfer dimension"));
        createDistanceDimension("DistanceX", { selPoints[0], selPoints[1] }, true);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalChamfer") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical chamfer dimension"));
        createDistanceDimension("DistanceY", { selPoints[0], selPoints[1] }, true);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }

    if (availableDimension == AvailableDimension::SECOND) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add distanceX chamfer dimension"));
        createDistanceDimension("DistanceX", { selPoints[0], selPoints[1] }, true);
        specialDimension = SpecialDimension::LineOr2PointsChamfer;
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_3Point(bool& selAllowed)
{
    // chain distances, angle
    if (_checkDimType(AvailableDimension::FIRST, "HorizontalChainDimension")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal chain dimensions"));
        createChainDimension("DistanceX");
        selAllowed = true;
    }
    else if (dimType == "VerticalChainDimension") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical chain dimensions"));
        createChainDimension("DistanceY");
        selAllowed = true;
    }
    else if (dimType == "ObliqueChainDimension") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add oblique chain dimensions"));
        createChainDimension("Distance");
        selAllowed = true;
    }

    if (_checkDimType(AvailableDimension::SECOND, "HorizontalCoordDimension")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal coordinate dimensions"));
        createCoordDimension("DistanceX");
        selAllowed = true;
    }
    else if (dimType == "VerticalCoordDimension") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical coordinate dimensions"));
        createCoordDimension("DistanceY");
        selAllowed = true;
    }
    else if (dimType == "ObliqueCoordDimension") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add oblique coordinate dimensions"));
        createCoordDimension("Distance");
        selAllowed = true;
    }

    if (_checkDimType(AvailableDimension::THIRD, "3ptAngle_1")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add 3-points angle dimension"));
        create3pAngleDimension({ selPoints[0], selPoints[1], selPoints[2] });
        selAllowed = true;
    }
    else if (_checkDimType(AvailableDimension::FOURTH, "3ptAngle_2")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add 3-points angle dimension"));
        create3pAngleDimension({ selPoints[1], selPoints[2], selPoints[0] });
        selAllowed = true;
    }
    else if (_checkDimType(AvailableDimension::FIFTH, "3ptAngle_3")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add 3-points angle dimension"));
        create3pAngleDimension({ selPoints[2], selPoints[0], selPoints[1] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_4MorePoints(bool& selAllowed)
{
    // chain distances
    if (_checkDimType(AvailableDimension::FIRST, "HorizontalChainDimension")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal chain dimension"));
        createChainDimension("DistanceX");
        selAllowed = true;
    }
    else if (dimType == "VerticalChainDimension") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical chain dimension"));
        createChainDimension("DistanceY");
        selAllowed = true;
    }
    else if (dimType == "ObliqueChainDimension") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add oblique chain dimension"));
        createChainDimension("Distance");
        selAllowed = true;
    }

    if (_checkDimType(AvailableDimension::SECOND, "HorizontalCoordDimension")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal coordinate dimensions"));
        createCoordDimension("DistanceX");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalCoordDimension") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical coordinate dimensions"));
        createCoordDimension("DistanceY");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "ObliqueCoordDimension") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add oblique coordinate dimensions"));
        createCoordDimension("Distance");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_1Point1Line(bool& selAllowed)
{
    //distance
    if (_checkDimType(AvailableDimension::FIRST, "LengthFree")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add point to line distance dimension"));
        createDistanceDimension("Distance", { selPoints[0], selLine[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "HorizontalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal distance dimension"));
        createDistanceDimension("DistanceX", { selPoints[0], selLine[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical distance dimension"));
        createDistanceDimension("DistanceY", { selPoints[0], selLine[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_1Point1Circle(bool& selAllowed)
{
    //Distance, extent distance
    if (_checkDimType(AvailableDimension::FIRST, "LengthFree")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add length dimension"));
        createDistanceDimension("Distance", { selPoints[0], selCircleArc[0] });
        selAllowed = true;
    }
    else if (dimType == "HorizontalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal distance dimension"));
        createDistanceDimension("DistanceX", { selPoints[0], selCircleArc[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical distance dimension"));
        createDistanceDimension("DistanceY", { selPoints[0], selCircleArc[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }

    if (_checkDimType(AvailableDimension::SECOND, "HorizontalExtent")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceX");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalExtent") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceY");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_1Point1Ellipse(bool& selAllowed)
{
    //Distance
    if (_checkDimType(AvailableDimension::FIRST, "LengthFree")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add length dimension"));
        createDistanceDimension("Distance", { selPoints[0], selEllipseArc[0] });
        selAllowed = true;
    }
    else if (dimType == "HorizontalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal distance dimension"));
        createDistanceDimension("DistanceX", { selPoints[0], selEllipseArc[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical distance dimension"));
        createDistanceDimension("DistanceY", { selPoints[0], selEllipseArc[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }

    if (_checkDimType(AvailableDimension::SECOND, "HorizontalExtent")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceX");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalExtent") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceY");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_1Line(bool& selAllowed)
{
    if (dimType == "Angle" || dimType == "HorizontalExtent" || dimType == "VerticalExtent") {
        selAllowed = true;
        return;
    }

    //distance
    if (_checkDimType(AvailableDimension::FIRST, "LengthFree")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add length dimension"));
        createDistanceDimension("Distance", { selLine[0] });
        specialDimension = SpecialDimension::LineOr2PointsDistance;
        selAllowed = true;
        if (!isVerticalDistance({ selLine[0] })) {
            availableDimension = AvailableDimension::RESET;
        }
    }
    else if (dimType == "HorizontalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal distance dimension"));
        createDistanceDimension("DistanceX", { selLine[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical distance dimension"));
        createDistanceDimension("DistanceY", { selLine[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "HorizontalChamfer") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal chamfer dimension"));
        createDistanceDimension("DistanceX", { selLine[0] }, true);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalChamfer") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical chamfer dimension"));
        createDistanceDimension("DistanceY", { selLine[0] }, true);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }

    if (availableDimension == AvailableDimension::SECOND) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add distanceX chamfer dimension"));
        createDistanceDimension("DistanceX", { selLine[0] }, true);
        specialDimension = SpecialDimension::LineOr2PointsChamfer;
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_2Line(bool& selAllowed)
{
    //angle (if parallel: Distance (see in createAngleDimension)).
    if (_checkDimType(AvailableDimension::FIRST, "Angle")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add angle dimension"));
        createAngleDimension(selLine[0], selLine[1]);
        selAllowed = true;
    }
    if (_checkDimType(AvailableDimension::SECOND, "HorizontalExtent")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceX");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalExtent") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceY");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_1Line1Circle(bool& selAllowed)
{
    //distance, extent distance
    if (_checkDimType(AvailableDimension::FIRST, "LengthFree")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add circle to line distance dimension"));
        createDistanceDimension("Distance", { selCircleArc[0], selLine[0] });
        selAllowed = true;
    }
    else if (dimType == "HorizontalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal distance dimension"));
        createDistanceDimension("DistanceX", { selLine[0], selCircleArc[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical distance dimension"));
        createDistanceDimension("DistanceY", { selLine[0], selCircleArc[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }

    if (_checkDimType(AvailableDimension::SECOND, "HorizontalExtent")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceX");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalExtent") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceY");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_1Line1Ellipse(bool& selAllowed)
{
    //distance, extent distance
    if (_checkDimType(AvailableDimension::FIRST, "LengthFree")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add ellipse to line distance dimension"));
        createDistanceDimension("Distance", { selEllipseArc[0], selLine[0] });
        selAllowed = true;
    }
    else if (dimType == "HorizontalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal distance dimension"));
        createDistanceDimension("DistanceX", { selLine[0], selEllipseArc[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical distance dimension"));
        createDistanceDimension("DistanceY", { selLine[0], selEllipseArc[0] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }

    if (_checkDimType(AvailableDimension::SECOND, "HorizontalExtent")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceX");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalExtent") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceY");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_1Circle(bool& selAllowed)
{
    if (dimType == "LengthFree" || dimType == "HorizontalLength" || dimType == "VerticalLength"
        || dimType == "HorizontalExtent" || dimType == "VerticalExtent") {
        selAllowed = true;
        return;
    }

    if (dimType == "") {
        if (availableDimension == AvailableDimension::FIRST) {
            createRadiusDiameterDimension(selCircleArc[0], true);
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            createRadiusDiameterDimension(selCircleArc[0], false);
            if (selCircleArc[0].geomEdgeType() != GeomType::ARCOFCIRCLE) {
                availableDimension = AvailableDimension::RESET;
            }
        }
    }
    if (_checkDimType(AvailableDimension::THIRD, "ArcLength")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add arc length dimension"));
        createArcLengthDimension(selCircleArc[0]);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "Radius") {
        createRadiusDimension(selCircleArc[0]);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "Diameter") {
        createDiameterDimension(selCircleArc[0]);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_2Circle(bool& selAllowed)
{
    //Distance
    if (_checkDimType(AvailableDimension::FIRST, "LengthFree")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add circle to circle distance dimension"));
        createDistanceDimension("Distance", { selCircleArc[0], selCircleArc[1] });
        selAllowed = true;
    }
    else if (dimType == "HorizontalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal distance dimension"));
        createDistanceDimension("DistanceX", { selCircleArc[0], selCircleArc[1] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical distance dimension"));
        createDistanceDimension("DistanceY", { selCircleArc[0], selCircleArc[1] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }

    if (_checkDimType(AvailableDimension::SECOND, "HorizontalExtent")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceX");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalExtent") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceY");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_1Ellipse(bool& selAllowed)
{
    if (dimType == "LengthFree" || dimType == "HorizontalLength" || dimType == "VerticalLength"
        || dimType == "HorizontalExtent" || dimType == "VerticalExtent") {
        selAllowed = true;
        return;
    }

    if (dimType == "") {
        if (availableDimension == AvailableDimension::FIRST) {
            createRadiusDiameterDimension(selEllipseArc[0], true);
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            createRadiusDiameterDimension(selEllipseArc[0], false);
            if (selEllipseArc[0].geomEdgeType() != GeomType::ARCOFELLIPSE) {
                availableDimension = AvailableDimension::RESET;
            }
        }
    }
    if (_checkDimType(AvailableDimension::THIRD, "ArcLength")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add arc length dimension"));
        createArcLengthDimension(selEllipseArc[0]);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "Radius") {
        createRadiusDimension(selEllipseArc[0]);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "Diameter") {
        createDiameterDimension(selEllipseArc[0]);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_2Ellipses(bool& selAllowed)
{
    //Distance
    if (_checkDimType(AvailableDimension::FIRST, "LengthFree")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add ellipse to ellipse distance dimension"));
        createDistanceDimension("Distance", { selEllipseArc[0], selEllipseArc[1] });
        selAllowed = true;
    }
    else if (dimType == "HorizontalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal distance dimension"));
        createDistanceDimension("DistanceX", { selEllipseArc[0], selEllipseArc[1] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalLength") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical distance dimension"));
        createDistanceDimension("DistanceY", { selEllipseArc[0], selEllipseArc[1] });
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }

    if (_checkDimType(AvailableDimension::SECOND, "HorizontalExtent")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceX");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalExtent") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceY");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_1Spline(bool& selAllowed)
{
    //Edge length
    if (_checkDimType(AvailableDimension::FIRST, "ArcLength")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add edge length dimension"));
        createArcLengthDimension(selSplineAndCo[0]);
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::makeCts_1SplineAndMore(bool& selAllowed)
{
    //Extend
    if (_checkDimType(AvailableDimension::FIRST, "HorizontalExtent")) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceX");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
    else if (dimType == "VerticalExtent") {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add extent dimension"));
        createExtentDistanceDimension("DistanceY");
        selAllowed = true;
        availableDimension = AvailableDimension::RESET;
    }
}

void TDHandlerDimension::createAreaDimension(ReferenceEntry ref)
{
    DrawViewDimension* dim = dimMaker(partFeat, "Area", { ref }, {});

    dims.push_back(dim);

    if (hasAreaLeaderPoint) {
        dim->UseAreaLeaderPoint.setValue(true);
        dim->AreaLeaderPoint.setValue(AreaLeaderPoint);
        dim->recomputeFeature();
        hasAreaLeaderPoint = false;
    }

    moveDimension(mousePos, dim);
}

void TDHandlerDimension::createRadiusDiameterDimension(ReferenceEntry ref, bool firstCstr) {
    int GeoId(TechDraw::DrawUtil::getIndexFromName(ref.getSubName()));
    TechDraw::BaseGeomPtr geom = partFeat->getGeomByIndex(GeoId);
    bool isCircleGeom = (geom->getGeomType() == GeomType::CIRCLE) || (geom->getGeomType() == GeomType::ELLIPSE);

    // Use same preference as in sketcher?
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/TechDraw/dimensioning");
    bool dimensioningDiameter = hGrp->GetBool("DimensioningDiameter", true);
    bool dimensioningRadius = hGrp->GetBool("DimensioningRadius", true);

    DrawViewDimension* dim;
    if ((firstCstr && dimensioningRadius && !dimensioningDiameter) ||
        (!firstCstr && !dimensioningRadius && dimensioningDiameter) ||
        (firstCstr && dimensioningRadius && dimensioningDiameter && !isCircleGeom) ||
        (!firstCstr && dimensioningRadius && dimensioningDiameter && isCircleGeom)) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add radius dimension"));
        dim = dimMaker(partFeat, "Radius", { ref }, {});
    }
    else {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add diameter dimension"));
        dim = dimMaker(partFeat, "Diameter", { ref }, {});
    }

    dims.push_back(dim);
    moveDimension(mousePos, dim);
}

void TDHandlerDimension::createRadiusDimension(ReferenceEntry ref)
{
    restartCommand(QT_TRANSLATE_NOOP("Command", "Add radius dimension"));
    DrawViewDimension* dim = dimMaker(partFeat, "Radius", { ref }, {});

    dims.push_back(dim);
    moveDimension(mousePos, dim);
}

void TDHandlerDimension::createDiameterDimension(ReferenceEntry ref)
{
    restartCommand(QT_TRANSLATE_NOOP("Command", "Add diameter dimension"));
    DrawViewDimension* dim = dimMaker(partFeat, "Diameter", { ref }, {});

    dims.push_back(dim);
    moveDimension(mousePos, dim);
}

void TDHandlerDimension::createAngleDimension(ReferenceEntry ref1, ReferenceEntry ref2) {
    if (TechDraw::isValidMultiEdge({ ref1, ref2 }) != DimensionGeometry::isAngle) {
        //isValidMultiEdge check if lines are parallel.
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add distance dimension"));
        createDistanceDimension("Distance", { ref1, ref2 });
        return;
    }

    DrawViewDimension* dim = dimMaker(partFeat, "Angle", {ref1, ref2}, {});

    dims.push_back(dim);
    moveDimension(mousePos, dim);
}

void TDHandlerDimension::create3pAngleDimension(ReferenceVector refs)
{
    DrawViewDimension* dim = dimMaker(partFeat, "Angle3Pt", refs, {});

    dims.push_back(dim);
    moveDimension(mousePos, dim);
}

void TDHandlerDimension::createArcLengthDimension(ReferenceEntry ref)
{
    DrawViewDimension* dim = makeArcLengthDimension(ref);

    dims.push_back(dim);
    moveDimension(mousePos, dim);
}

void TDHandlerDimension::createDistanceDimension(std::string type, ReferenceVector refs, bool chamfer)
{
    DrawViewDimension* dim = dimMaker(partFeat, type, refs, {});

    if (chamfer) {
        // Add the angle to the label
        TechDraw::pointPair pp = dim->getLinearPoints();
        float dx = pp.first().x - pp.second().x;
        float dy = pp.first().y - pp.second().y;
        int alpha = std::round(Base::toDegrees(std::abs(std::atan(type == "DistanceY" ? (dx / dy) : (dy / dx)))));
        std::string sAlpha = std::to_string(alpha);
        std::string formatSpec = dim->FormatSpec.getStrValue();
        formatSpec = formatSpec + " x" + sAlpha + "°";
        dim->FormatSpec.setValue(formatSpec);
    }

    dims.push_back(dim);
    moveDimension(mousePos, dim);
}

void TDHandlerDimension::createExtentDistanceDimension(std::string type) {
    specialDimension = (dimType == "") ? SpecialDimension::ExtendDistance : SpecialDimension::None;

    DrawViewDimension* dim = DrawDimHelper::makeExtentDim(partFeat, type, allRefs());

    dims.push_back(dim);
    moveDimension(mousePos, dim);
}

void TDHandlerDimension::updateDistanceType()
{
    if (dims.empty()) {
        return;
    }

    auto type = static_cast<DimensionType>(dims[0]->Type.getValue());
    SpecialDimension backup = specialDimension;
    bool chamfer = specialDimension == SpecialDimension::LineOr2PointsChamfer;

    TechDraw::pointPair pp = dims[0]->getLinearPoints();
    Base::Vector3d pnt1 = Rez::guiX(pp.first());
    Base::Vector3d pnt2 = Rez::guiX(pp.second());

    QPointF fpos = getDimPositionToBe(mousePos);
    double minX, minY, maxX, maxY;
    minX = min(pnt1.x, pnt2.x);
    maxX = max(pnt1.x, pnt2.x);
    minY = min(pnt1.y, pnt2.y);
    maxY = max(pnt1.y, pnt2.y);

    std::string newType = "Distance";
    if (fpos.x() > minX && fpos.x() < maxX
        && (fpos.y() < minY || fpos.y() > maxY) && type != DimensionType::DistanceX) {
        if (chamfer) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add distanceX chamfer dimension"));
        }
        else {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add distanceX dimension"));
        }
        newType = "DistanceX";
    }
    else if (fpos.y() > minY && fpos.y() < maxY
        && (fpos.x() < minX || fpos.x() > maxX) && type != DimensionType::DistanceY) {
        if (chamfer) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add distanceY chamfer dimension"));
        }
        else {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add distanceY dimension"));
        }
        newType = "DistanceY";
    }
    else if ((((fpos.y() < minY || fpos.y() > maxY) && (fpos.x() < minX || fpos.x() > maxX))
        || (fpos.y() > minY && fpos.y() < maxY && fpos.x() > minX && fpos.x() < maxX)) && type != DimensionType::Distance
        && !chamfer) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add distance dimension"));
    }
    else {
        return;
    }
    specialDimension = backup;

    if (selLine.size() == 1) {
        createDistanceDimension(newType, { selLine[0] }, chamfer);
    }
    else {
        createDistanceDimension(newType, { selPoints[0], selPoints[1] }, chamfer);
    }
    setDimsSelectability(false);
}

void TDHandlerDimension::updateExtentDistanceType()
{
    if (dims.empty()) {
        return;
    }

    auto type = static_cast<DimensionType>(dims[0]->Type.getValue());

    TechDraw::pointPair pp = dims[0]->getLinearPoints();
    Base::Vector3d pnt1 = Rez::guiX(pp.first());
    Base::Vector3d pnt2 = Rez::guiX(pp.second());

    QPointF fpos = getDimPositionToBe(mousePos);

    double minX, minY, maxX, maxY;
    minX = min(pnt1.x, pnt2.x);
    maxX = max(pnt1.x, pnt2.x);
    minY = min(pnt1.y, pnt2.y);
    maxY = max(pnt1.y, pnt2.y);

    if (fpos.x() > minX && fpos.x() < maxX
        && (fpos.y() < minY || fpos.y() > maxY) && type != DimensionType::DistanceX) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add distanceX extent dimension"));
        createExtentDistanceDimension("DistanceX");
    }
    else if (fpos.y() > minY && fpos.y() < maxY
        && (fpos.x() < minX || fpos.x() > maxX) && type != DimensionType::DistanceY) {
        restartCommand(QT_TRANSLATE_NOOP("Command", "Add distanceY extent dimension"));
        createExtentDistanceDimension("DistanceY");
    }
    else {
        return;
    }

    setDimsSelectability(false);
}

void TDHandlerDimension::updateChainDistanceType()
{
    if (dims.empty()) {
        return;
    }

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = -std::numeric_limits<double>::max();
    double maxY = -std::numeric_limits<double>::max();
    for (auto dim : dims) {
        TechDraw::pointPair pp = dim->getLinearPoints();
        Base::Vector3d pnt1 = Rez::guiX(pp.first());
        Base::Vector3d pnt2 = Rez::guiX(pp.second());

        minX = min(minX, min(pnt1.x, pnt2.x));
        maxX = max(maxX, max(pnt1.x, pnt2.x));
        minY = min(minY, min(pnt1.y, pnt2.y));
        maxY = max(maxY, max(pnt1.y, pnt2.y));
    }

    QPointF fpos = getDimPositionToBe(mousePos);

    auto type = static_cast<DimensionType>(dims[0]->Type.getValue());

    if (fpos.x() > minX && fpos.x() < maxX
        && (fpos.y() < minY || fpos.y() > maxY) && type != DimensionType::DistanceX) {
        if (specialDimension == SpecialDimension::ChainDistance) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal chain dimensions"));
            createChainDimension("DistanceX");
        }
        else {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal coord dimensions"));
            createCoordDimension("DistanceX");
        }
    }
    else if (fpos.y() > minY && fpos.y() < maxY
        && (fpos.x() < minX || fpos.x() > maxX) && type != DimensionType::DistanceY) {
        if (specialDimension == SpecialDimension::ChainDistance) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical chain dimensions"));
            createChainDimension("DistanceY");
        }
        else {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add vertical coord dimensions"));
            createCoordDimension("DistanceY");
        }
    }
    else if (((fpos.y() < minY || fpos.y() > maxY) && (fpos.x() < minX || fpos.x() > maxX)) && type != DimensionType::Distance) {
        if (specialDimension == SpecialDimension::ChainDistance) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add oblique chain dimensions"));
            createChainDimension("Distance");
        }
        else {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add oblique coord dimensions"));
            createCoordDimension("Distance");
        }
    }
    else {
        return;
    }

    setDimsSelectability(false);
}

void TDHandlerDimension::createChainDimension(std::string type)
{
    specialDimension = SpecialDimension::ChainDistance;
    if (type == "Distance") {
        dims = makeObliqueChainDimension(selPoints);
    }
    else {
        for (size_t i = 0; i < selPoints.size() - 1; ++i) {
            DrawViewDimension* dim = dimMaker(partFeat, type, { selPoints[i], selPoints[i + 1] }, {});

            dims.push_back(dim);
            positionDimText(dim);
        }
    }
}

void TDHandlerDimension::createCoordDimension(std::string type)
{
    specialDimension = SpecialDimension::CoordDistance;
    if (type == "Distance") {
        dims = makeObliqueCoordDimension(selPoints);
    }
    else {
        for (size_t i = 0; i < selPoints.size() - 1; ++i) {
            DrawViewDimension* dim = dimMaker(partFeat, type, { selPoints[0], selPoints[i + 1] }, {});

            dims.push_back(dim);
            positionDimText(dim, i);
        }
    }
}

bool TDHandlerDimension::isVerticalDistance(ReferenceVector refs)
{
    DimensionGeometry geometryRefs2d = validateDimSelection(
        refs, { "Edge", "Vertex" }, { 1, 2 }, { DimensionGeometry::isDiagonal });

    return geometryRefs2d == DimensionGeometry::isDiagonal;
}

QPixmap TDHandlerDimension::icon(std::string name)
{
    constexpr int width = 16;
    return Gui::BitmapFactory().pixmapFromSvg(name.c_str(), QSize(width, width));
}

void TDHandlerDimension::restartCommand(const char* cstrName) {
    specialDimension = SpecialDimension::None;
    Gui::Command::abortCommand(tid);
    tid  = Gui::Command::openActiveDocumentCommand(cstrName);

    dims.clear();
}

void TDHandlerDimension::clearAndRestartCommand() {
    Gui::Command::abortCommand(tid);
    tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Dimension"));
    specialDimension = SpecialDimension::None;
    mousePos = QPoint(0,0);
    clearRefVectors();
    partFeat = nullptr;
    dims.clear();
}

void TDHandlerDimension::clearRefVectors()
{
    selPoints.clear();
    selLine.clear();
    selCircleArc.clear();
    selEllipseArc.clear();
    selSplineAndCo.clear();
    selFaces.clear();
}
