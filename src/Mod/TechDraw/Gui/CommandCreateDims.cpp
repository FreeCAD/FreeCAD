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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <string>
#include <vector>

#include <QApplication>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QPixmap>
#endif//#ifndef _PreComp_

#include <App/AutoTransaction.h>
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
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
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
#include "QGIViewDimension.h"
#include "QGVPage.h"
#include "MDIViewPage.h"
#include "TaskDimRepair.h"
#include "TaskLinkDim.h"
#include "TaskSelectLineAttributes.h"
#include "TechDrawHandler.h"
#include "ViewProviderDimension.h"
#include "ViewProviderDrawingView.h"


using namespace TechDrawGui;
using namespace TechDraw;
using namespace std;
using DimensionType = TechDraw::DrawViewDimension::DimensionType;

//===========================================================================
// utility routines
//===========================================================================

//internal functions
bool _checkSelection(Gui::Command* cmd, unsigned maxObjs = 2);
bool _checkDrawViewPart(Gui::Command* cmd);

bool isDimCmdActive(Gui::Command* cmd)
{
    bool havePage = DrawGuiUtil::needPage(cmd);
    bool haveView = DrawGuiUtil::needView(cmd);
    return (havePage && haveView);
}


void execDistance(Gui::Command* cmd);
void execDistanceX(Gui::Command* cmd);
void execDistanceY(Gui::Command* cmd);
void execAngle(Gui::Command* cmd);
void execAngle3Pt(Gui::Command* cmd);
void execRadius(Gui::Command* cmd);
void execDiameter(Gui::Command* cmd);
void execArea(Gui::Command* cmd);
void execDim(Gui::Command* cmd, std::string type, StringVector acceptableGeometry, std::vector<int> minimumCounts, std::vector<DimensionGeometryType> acceptableDimensionGeometrys);

void execExtent(Gui::Command* cmd, const std::string& dimType);

DrawViewDimension* dimensionMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                                  ReferenceVector references2d, ReferenceVector references3d);
DrawViewDimension* dimMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                                  ReferenceVector references2d, ReferenceVector references3d);

void positionDimText(DrawViewDimension* dim, int indexOffset = 0);

void activateHandler(TechDrawHandler* newHandler)
{
    auto* mdi = dynamic_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
    if (!mdi) {
        return;
    }

    ViewProviderPage* vp = mdi->getViewProviderPage();
    if (!vp) {
        return;
    }

    QGVPage* viewPage = vp->getQGVPage();
    if (!viewPage) {
        return;
    }
    viewPage->activateHandler(newHandler);
}

//===========================================================================
// TechDraw_Dimension
//===========================================================================

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

class TDHandlerDimension : public TechDrawHandler,
                           public Gui::SelectionObserver
{
public:
    explicit TDHandlerDimension(ReferenceVector refs, TechDraw::DrawViewPart* pFeat)
        : SelectionObserver(true)
        , specialDimension(SpecialDimension::None)
        , availableDimension(AvailableDimension::FIRST)
        , mousePos(QPoint(0, 0))
        , selPoints({})
        , selLine({})
        , selCircleArc({})
        , selEllipseArc({})
        , selSplineAndCo({})
        , selFaces({})
        , emptyVector({})
        , addedRef(ReferenceEntry())
        , removedRef(ReferenceEntry())
        , initialSelection(std::move(refs))
        , partFeat(pFeat)
        , dims({})
        , blockRemoveSel(false)
    {
    }
    ~TDHandlerDimension()
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


    void activated() override
    {
        auto* mdi = dynamic_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
        if (mdi) {
            mdi->setDimensionsSelectability(false);
        }
        Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::GreedySelection);
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Insert Dimension"));
        handleInitialSelection();
    }

    void deactivated() override
    {
        auto* mdi = dynamic_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
        if (mdi) {
            mdi->setDimensionsSelectability(true);
        }
        Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
        Gui::Command::abortCommand();
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_M && !selectionEmpty()) {
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
    void keyReleaseEvent(QKeyEvent* event) override
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

    void mouseMoveEvent(QMouseEvent* event) override
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
            updateChainDistanceType();
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

    QGIDatumLabel* getDimLabel(DrawViewDimension* d)
    {
        auto* vp = dynamic_cast<ViewProviderDimension*>(Gui::Application::Instance->getViewProvider(d));
        if (!vp) {
            return nullptr;
        }
        auto* qgivDimension(dynamic_cast<QGIViewDimension*>(vp->getQView()));
        if (!qgivDimension) {
            return nullptr;
        }
        return qgivDimension->getDatumLabel();
    }
    void moveDimension(QPoint pos, DrawViewDimension* dim, bool textToMiddle = false, Base::Vector3d dir = Base::Vector3d(),
        Base::Vector3d delta = Base::Vector3d(), DimensionType type = DimensionType::Distance, int i = 0)
    {
        if (!dim) { return; }
        auto label = getDimLabel(dim);
        if (!label) { return; }

        label->setPos(getDimPositionToBe(pos, label->pos(), textToMiddle, dir, delta, type, i));
    }
    QPointF getDimPositionToBe(QPoint pos, QPointF curPos = QPointF(), bool textToMiddle = false, Base::Vector3d dir = Base::Vector3d(),
        Base::Vector3d delta = Base::Vector3d(), DimensionType type = DimensionType::Distance, int i = 0)
    {
        auto* vpp = dynamic_cast<ViewProviderDrawingView*>(Gui::Application::Instance->getViewProvider(partFeat));
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

                double y = scenePos.y() + i * dimDistance;
                scenePos = QPointF(curPos.x(), y);
            }
            else if(type == DimensionType::DistanceY) {
                pointPair pp = dims[0]->getLinearPoints();
                if (Rez::guiX(pp.first().x) > scenePos.x())
                    dimDistance = -dimDistance;

                double x = scenePos.x() + i * dimDistance;
                scenePos = QPointF(x, curPos.y());
            }
        }

        return scenePos;
    }
    void finishDimensionMove()
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

    void setDimsSelectability(bool val)
    {
        for (auto dim : dims) {
            setDimSelectability(dim, val);
        }
    }
    void setDimSelectability(DrawViewDimension* d, bool val)
    {
        QGIDatumLabel* label = getDimLabel(d);
        if (label) {
            label->setSelectability(val);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        // Base::Console().Warning("mouseReleaseEvent TH\n");
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
                //Base::Console().Warning("RmvSelection \n");
                // Remove the reference from the vector
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
                //Base::Console().Warning("AddSelection\n");
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

                    availableDimension = AvailableDimension::FIRST;
                    bool selAllowed = makeAppropriateDimension();

                    if (!selAllowed) {
                        // remove from selection
                        blockRemoveSel = true;

                        Gui::Selection().rmvSelection(addedRef.getObject()->getDocument()->getName(),
                            addedRef.getObject()->getNameInDocument(), addedRef.getSubName().c_str());
                        blockRemoveSel = false;

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

    void onSelectionChanged(const Gui::SelectionChanges& msg) override
    {
        //Base::Console().Warning("onSelectionChanged %d - --%s--\n", (int)msg.Type, msg.pSubName);

        if (msg.Type == Gui::SelectionChanges::ClrSelection) {
            //clearAndRestartCommand();
            return;
        }

        if (msg.Object.getObjectName().empty()
            || msg.Object.getDocument() != getPage()->getDocument()) {
            if (msg.Type == Gui::SelectionChanges::AddSelection) {
                Gui::Selection().rmvSelection(msg.pDocName, msg.pObjectName, msg.pSubName);
            }
            return;
        }

        /*if (msg.Type == Gui::SelectionChanges::SetPreselect) {
            Base::Console().Warning("SetPreselect\n");
            std::string geomName = DrawUtil::getGeomTypeFromName(msg.pSubName);
            edgeOrPointPreselected = geomName == "Edge" || geomName == "Vertex";
            return;
        }
        else if (msg.Type == Gui::SelectionChanges::RmvPreselect) {
            Base::Console().Warning("RmvPreselect\n");
            edgeOrPointPreselected = false;
            return;
        }*/

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

private:
    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("TechDraw_Dimension_Pointer");
    }

protected:
    SpecialDimension specialDimension;
    AvailableDimension availableDimension;

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

    bool blockRemoveSel;

    void handleInitialSelection()
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

    void finalizeCommand()
    {
        //Base::Console().Warning("finalizeCommand \n");

        finishDimensionMove();

        // Ask for the value of datum dimensions
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/TechDraw");

        Gui::Command::commitCommand();

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

    ReferenceVector& getSelectionVector(ReferenceEntry& ref)
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

            if (geom->getGeomType() == TechDraw::GENERIC) {
                TechDraw::GenericPtr gen1 = std::static_pointer_cast<TechDraw::Generic>(geom);
                if (gen1->points.size() < 2) {
                    return emptyVector;
                }
                return selLine;
                //Base::Vector3d line = gen1->points.at(1) - gen1->points.at(0);
            }
            else if (geom->getGeomType() == TechDraw::CIRCLE || geom->getGeomType() == TechDraw::ARCOFCIRCLE) {
                return selCircleArc;
            }
            else if (geom->getGeomType() == TechDraw::ELLIPSE || geom->getGeomType() == TechDraw::ARCOFELLIPSE) {
                return selEllipseArc;
            }
            else if (geom->getGeomType() == TechDraw::BSPLINE) {
                //TechDraw::BSplinePtr spline = std::static_pointer_cast<TechDraw::BSpline>(geom);
                //if (spline->isCircle()) {
                //    return isBSplineCircle;
                //}
                //else {
                //}
                return selSplineAndCo;
            }
        }
        else if (geomName == "Vertex") {
            return selPoints;
        }

        return emptyVector;
    }

    /*
    bool notSelectedYet(const ReferenceEntry& elem)
    {
        auto contains = [&](const ReferenceVector& vec, const ReferenceEntry& elem) {
            for (const auto& x : vec)
            {
                if (x == elem){
                    return true;
                }
            }
            return false;
        };

        return !contains(selPoints, elem)
            && !contains(selLine, elem)
            && !contains(selCircleArc, elem)
            && !contains(selEllipseArc, elem)
            && !contains(selFaces, elem);
    }*/

    bool selectionEmpty()
    {
        return selPoints.empty() && selLine.empty() && selCircleArc.empty() && selEllipseArc.empty() && selSplineAndCo.empty() && selFaces.empty();
    }

    ReferenceVector allRefs()
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

    bool makeAppropriateDimension() {
        bool selAllowed = false;
        //Base::Console().Warning("makeAppropriateDimension %d %d %d %d %d %d\n", selPoints.size(), selLine.size(), selCircleArc.size(), selEllipseArc.size(), selSplineAndCo.size(), selFaces.size());

        GeomSelectionSizes selection(selPoints.size(), selLine.size(), selCircleArc.size(), selEllipseArc.size(), selSplineAndCo.size(), selFaces.size());
        if (selection.hasFaces()) {
            if (selection.has1Face()) { makeCts_Faces(selAllowed); }
            else { return false; }  // nothing else with face works
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

    void makeCts_Faces(bool& selAllowed)
    {
        //area
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Area dimension"));
            createAreaDimension(selFaces[0]);
            selAllowed = true;
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_2Point(bool& selAllowed)
    {
        //distance
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Distance dimension"));
            createDistanceDimension("Distance", { selPoints[0], selPoints[1] });
            specialDimension = SpecialDimension::LineOr2PointsDistance;
            selAllowed = true;
            if (!isVerticalDistance({ selPoints[0], selPoints[1] })) {
                availableDimension = AvailableDimension::RESET;
            }
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add DistanceX Chamfer dimension"));
            createDistanceDimension("DistanceX", { selPoints[0], selPoints[1] }, true);
            specialDimension = SpecialDimension::LineOr2PointsChamfer;
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_3Point(bool& selAllowed)
    {
        // chain distances, angle
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal chain dimensions"));
            createChainDimension("DistanceX");
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal coordinate dimensions"));
            createCoordDimension("DistanceX");
        }
        if (availableDimension == AvailableDimension::THIRD) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add 3-points angle dimension"));
            create3pAngleDimension({ selPoints[0], selPoints[1], selPoints[2] });
        }
        else if (availableDimension == AvailableDimension::FOURTH) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add 3-points angle dimension"));
            create3pAngleDimension({ selPoints[1], selPoints[2], selPoints[0] });
        }
        else if (availableDimension == AvailableDimension::FIFTH) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add 3-points angle dimension"));
            create3pAngleDimension({ selPoints[2], selPoints[0], selPoints[1] });
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_4MorePoints(bool& selAllowed)
    {
        // chain distances
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal chain dimension"));
            createChainDimension("DistanceX");
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal coordinate dimensions"));
            createCoordDimension("DistanceX");
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_1Point1Line(bool& selAllowed)
    {
        //distance
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add point to line Distance dimension"));
            createDistanceDimension("Distance", { selPoints[0], selLine[0] });
            selAllowed = true;
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_1Point1Circle(bool& selAllowed)
    {
        //Distance, extent distance
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add length dimension"));
            createDistanceDimension("Distance", { selPoints[0], selCircleArc[0] });
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Extent dimension"));
            createExtentDistanceDimension("DistanceX");
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_1Point1Ellipse(bool& selAllowed)
    {
        //Distance
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add length dimension"));
            createDistanceDimension("Distance", { selPoints[0], selEllipseArc[0] });
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Extent dimension"));
            createExtentDistanceDimension("DistanceX");
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_1Line(bool& selAllowed)
    {
        //distance
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add length dimension"));
            createDistanceDimension("Distance", { selLine[0] });
            specialDimension = SpecialDimension::LineOr2PointsDistance;
            selAllowed = true;
            if (!isVerticalDistance({ selLine[0] })) {
                availableDimension = AvailableDimension::RESET;
            }
            // Potential improvement for the future: we could show available modes in cursor trail.
            //std::vector<QPixmap> pixmaps = { icon("TechDraw_LengthDimension"), icon("TechDraw_ExtensionCreateHorizChamferDimension") };
            //addCursorTail(pixmaps);
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add DistanceX Chamfer dimension"));
            createDistanceDimension("DistanceX", { selLine[0] }, true);
            specialDimension = SpecialDimension::LineOr2PointsChamfer;
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_2Line(bool& selAllowed)
    {
        //angle (if parallel: Distance (see in createAngleDimension)).
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Angle dimension"));
            createAngleDimension(selLine[0], selLine[1]);
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Extent dimension"));
            createExtentDistanceDimension("DistanceX");
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_1Line1Circle(bool& selAllowed)
    {
        //distance, extent distance
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add circle to line Distance dimension"));
            createDistanceDimension("Distance", { selCircleArc[0], selLine[0] });
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Extent dimension"));
            createExtentDistanceDimension("DistanceX");
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_1Line1Ellipse(bool& selAllowed)
    {
        //distance, extent distance
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add ellipse to line Distance dimension"));
            createDistanceDimension("Distance", { selEllipseArc[0], selLine[0] });
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Extent dimension"));
            createExtentDistanceDimension("DistanceX");
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_1Circle(bool& selAllowed)
    {
        if (availableDimension == AvailableDimension::FIRST) {
            createRadiusDiameterDimension(selCircleArc[0], true);
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            createRadiusDiameterDimension(selCircleArc[0], false);
            if (selCircleArc[0].geomEdgeType() != TechDraw::ARCOFCIRCLE) {
                availableDimension = AvailableDimension::RESET;
            }
        }
        if (availableDimension == AvailableDimension::THIRD) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Arc Length dimension"));
            createArcLengthDimension(selCircleArc[0]);
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_2Circle(bool& selAllowed)
    {
        //Distance
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add circle to circle Distance dimension"));
            createDistanceDimension("Distance", { selCircleArc[0], selCircleArc[1] });
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Extent dimension"));
            createExtentDistanceDimension("DistanceX");
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_1Ellipse(bool& selAllowed)
    {
        if (availableDimension == AvailableDimension::FIRST) {
            createRadiusDiameterDimension(selEllipseArc[0], true);
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            createRadiusDiameterDimension(selEllipseArc[0], false);
            if (selEllipseArc[0].geomEdgeType() != TechDraw::ARCOFELLIPSE) {
                availableDimension = AvailableDimension::RESET;
            }
        }
        if (availableDimension == AvailableDimension::THIRD) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Arc Length dimension"));
            createArcLengthDimension(selEllipseArc[0]);
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_2Ellipses(bool& selAllowed)
    {
        //Distance
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add ellipse to ellipse Distance dimension"));
            createDistanceDimension("Distance", { selEllipseArc[0], selEllipseArc[1] });
            selAllowed = true;
        }
        if (availableDimension == AvailableDimension::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Extent dimension"));
            createExtentDistanceDimension("DistanceX");
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_1Spline(bool& selAllowed)
    {
        //Edge length
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add edge length dimension"));
            createArcLengthDimension(selSplineAndCo[0]);
            selAllowed = true;
            availableDimension = AvailableDimension::RESET;
        }
    }

    void makeCts_1SplineAndMore(bool& selAllowed)
    {
        //Extend
        if (availableDimension == AvailableDimension::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Extent dimension"));
            createExtentDistanceDimension("DistanceX");
            selAllowed = true;
            availableDimension = AvailableDimension::RESET;
        }
    }

    void createAreaDimension(ReferenceEntry ref)
    {
        DrawViewDimension* dim = dimMaker(partFeat, "Area", { ref }, {});

        dims.push_back(dim);
        moveDimension(mousePos, dim);
    }

    void createRadiusDiameterDimension(ReferenceEntry ref, bool firstCstr) {
        int GeoId(TechDraw::DrawUtil::getIndexFromName(ref.getSubName()));
        TechDraw::BaseGeomPtr geom = partFeat->getGeomByIndex(GeoId);
        bool isCircleGeom = (geom->getGeomType() == TechDraw::CIRCLE) || (geom->getGeomType() == TechDraw::ELLIPSE);

        // Use same preference as in sketcher?
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/TechDraw/dimensioning");
        bool dimensioningDiameter = hGrp->GetBool("DimensioningDiameter", true);
        bool dimensioningRadius = hGrp->GetBool("DimensioningRadius", true);

        DrawViewDimension* dim;
        if ((firstCstr && dimensioningRadius && !dimensioningDiameter) ||
            (!firstCstr && !dimensioningRadius && dimensioningDiameter) ||
            (firstCstr && dimensioningRadius && dimensioningDiameter && !isCircleGeom) ||
            (!firstCstr && dimensioningRadius && dimensioningDiameter && isCircleGeom)) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Radius dimension"));
            dim = dimMaker(partFeat, "Radius", { ref }, {});
        }
        else {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Diameter dimension"));
            dim = dimMaker(partFeat, "Diameter", { ref }, {});
        }

        dims.push_back(dim);
        moveDimension(mousePos, dim);
    }

    void createAngleDimension(ReferenceEntry ref1, ReferenceEntry ref2) {
        if (TechDraw::isValidMultiEdge({ ref1, ref2 }) != isAngle) {
            //isValidMultiEdge check if lines are parallel.
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Distance dimension"));
            createDistanceDimension("Distance", { ref1, ref2 });
            return;
        }

        DrawViewDimension* dim = dimMaker(partFeat, "Angle", {ref1, ref2}, {});

        dims.push_back(dim);
        moveDimension(mousePos, dim);
    }

    void create3pAngleDimension(ReferenceVector refs)
    {
        DrawViewDimension* dim = dimMaker(partFeat, "Angle3Pt", refs, {});

        dims.push_back(dim);
        moveDimension(mousePos, dim);
    }

    void createArcLengthDimension(ReferenceEntry ref)
    {
        DrawViewDimension* dim = makeArcLengthDimension(ref);

        dims.push_back(dim);
        moveDimension(mousePos, dim);
    }

    void createDistanceDimension(std::string type, ReferenceVector refs, bool chamfer = false)
    {
        DrawViewDimension* dim = dimMaker(partFeat, type, refs, {});

        if (chamfer) {
            // Add the angle to the label
            TechDraw::pointPair pp = dim->getLinearPoints();
            float dx = pp.first().x - pp.second().x;
            float dy = pp.first().y - pp.second().y;
            int alpha = round(Base::toDegrees(abs(atan(type == "DistanceY" ? (dx / dy) : (dy / dx)))));
            std::string sAlpha = std::to_string(alpha);
            std::string formatSpec = dim->FormatSpec.getStrValue();
            formatSpec = formatSpec + " x" + sAlpha + "°";
            dim->FormatSpec.setValue(formatSpec);
        }

        dims.push_back(dim);
        moveDimension(mousePos, dim);
    }

    void createExtentDistanceDimension(std::string type) {
        specialDimension = SpecialDimension::ExtendDistance;

        DrawViewDimension* dim = DrawDimHelper::makeExtentDim(partFeat, type, allRefs());

        dims.push_back(dim);
        moveDimension(mousePos, dim);
    }

    void updateDistanceType()
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
                restartCommand(QT_TRANSLATE_NOOP("Command", "Add DistanceX Chamfer dimension"));
            }
            else {
                restartCommand(QT_TRANSLATE_NOOP("Command", "Add DistanceX dimension"));
            }
            newType = "DistanceX";
        }
        else if (fpos.y() > minY && fpos.y() < maxY
            && (fpos.x() < minX || fpos.x() > maxX) && type != DimensionType::DistanceY) {
            if (chamfer) {
                restartCommand(QT_TRANSLATE_NOOP("Command", "Add DistanceY Chamfer dimension"));
            }
            else {
                restartCommand(QT_TRANSLATE_NOOP("Command", "Add DistanceY dimension"));
            }
            newType = "DistanceY";
        }
        else if ((((fpos.y() < minY || fpos.y() > maxY) && (fpos.x() < minX || fpos.x() > maxX))
            || (fpos.y() > minY && fpos.y() < maxY && fpos.x() > minX && fpos.x() < maxX)) && type != DimensionType::Distance
            && !chamfer) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Distance dimension"));
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

    void updateExtentDistanceType()
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
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add DistanceX extent dimension"));
            createExtentDistanceDimension("DistanceX");
        }
        else if (fpos.y() > minY && fpos.y() < maxY
            && (fpos.x() < minX || fpos.x() > maxX) && type != DimensionType::DistanceY) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add DistanceY extent dimension"));
            createExtentDistanceDimension("DistanceY");
        }
        else {
            return;
        }

        setDimsSelectability(false);
    }

    void updateChainDistanceType()
    {
        if (dims.empty()) {
            return;
        }

        double minX = DBL_MAX;
        double minY = DBL_MAX;
        double maxX = -DBL_MAX;
        double maxY = -DBL_MAX;
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

    void createChainDimension(std::string type)
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

    void createCoordDimension(std::string type)
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

    bool isVerticalDistance(ReferenceVector refs)
    {
        DimensionGeometryType geometryRefs2d = validateDimSelection(
            refs, { "Edge", "Vertex" }, { 1, 2 }, { isDiagonal });

        return geometryRefs2d == TechDraw::isDiagonal;
    }

    QPixmap icon(std::string name)
    {
        qreal pixelRatio = 1;
        Gui::View3DInventorViewer* viewer = getViewer();
        if (viewer) {
            pixelRatio = viewer->devicePixelRatio();
        }
        int width = 16 * pixelRatio;
        return Gui::BitmapFactory().pixmapFromSvg(name.c_str(), QSize(width, width));
    }

    void restartCommand(const char* cstrName) {
        specialDimension = SpecialDimension::None;
        Gui::Command::abortCommand();
        Gui::Command::openCommand(cstrName);

        dims.clear();
    }

    void clearAndRestartCommand() {
        Gui::Command::abortCommand();
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Dimension"));
        specialDimension = SpecialDimension::None;
        mousePos = QPoint(0,0);
        clearRefVectors();
        partFeat = nullptr;
        dims.clear();
    }

    void clearRefVectors()
    {
        selPoints.clear();
        selLine.clear();
        selCircleArc.clear();
        selEllipseArc.clear();
        selSplineAndCo.clear();
        selFaces.clear();
    }
};

DEF_STD_CMD_A(CmdTechDrawDimension)

CmdTechDrawDimension::CmdTechDrawDimension()
    : Command("TechDraw_Dimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Dimension");
    sToolTipText = QT_TR_NOOP("Dimension contextually based on your selection.\n"
        "Depending on your selection you might have several dimensions available. You can cycle through them using the M key.\n"
        "Left clicking on empty space will validate the current Dimension. Right clicking or pressing Esc will cancel.");
    sWhatsThis = "TechDraw_Dimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_Dimension";
    sAccel = "D";
}

void CmdTechDrawDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::AutoTransaction::setEnable(false);

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    activateHandler(new TDHandlerDimension(references2d, partFeat));
}

bool CmdTechDrawDimension::isActive()
{
    return isDimCmdActive(this);
}


// Comp for dimension tools =============================================

class CmdTechDrawCompDimensionTools : public Gui::GroupCommand
{
public:
    CmdTechDrawCompDimensionTools()
        : GroupCommand("TechDraw_CompDimensionTools")
    {
        sAppModule = "TechDraw";
        sGroup = "TechDraw";
        sMenuText = QT_TR_NOOP("Dimension");
        sToolTipText = QT_TR_NOOP("Dimension tools.");
        sWhatsThis = "TechDraw_CompDimensionTools";
        sStatusTip = sToolTipText;

        setCheckable(false);
        setRememberLast(false);

        addCommand("TechDraw_Dimension");
        addCommand(); //separator
        addCommand("TechDraw_LengthDimension");
        addCommand("TechDraw_HorizontalDimension");
        addCommand("TechDraw_VerticalDimension");
        addCommand("TechDraw_RadiusDimension");
        addCommand("TechDraw_DiameterDimension");
        addCommand("TechDraw_AngleDimension");
        addCommand("TechDraw_3PtAngleDimension");
        addCommand("TechDraw_AreaDimension");
        addCommand("TechDraw_ExtensionCreateLengthArc");
        addCommand(); //separator
        addCommand("TechDraw_HorizontalExtentDimension");
        addCommand("TechDraw_VerticalExtentDimension");
        addCommand(); //separator
        addCommand("TechDraw_ExtensionCreateHorizChainDimension");
        addCommand("TechDraw_ExtensionCreateVertChainDimension");
        addCommand("TechDraw_ExtensionCreateObliqueChainDimension");
        addCommand(); //separator
        addCommand("TechDraw_ExtensionCreateHorizCoordDimension");
        addCommand("TechDraw_ExtensionCreateVertCoordDimension");
        addCommand("TechDraw_ExtensionCreateObliqueCoordDimension");
        addCommand(); //separator
        addCommand("TechDraw_ExtensionCreateHorizChamferDimension");
        addCommand("TechDraw_ExtensionCreateVertChamferDimension");
    }

    const char* className() const override { return "CmdTechDrawCompDimensionTools"; }

    bool isActive() override
    {
        return isDimCmdActive(this);
    }
};

//===========================================================================
// TechDraw_RadiusDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawRadiusDimension)

CmdTechDrawRadiusDimension::CmdTechDrawRadiusDimension()
    : Command("TechDraw_RadiusDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Radius Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_RadiusDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_RadiusDimension";
}

void CmdTechDrawRadiusDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execRadius(this);
}

bool CmdTechDrawRadiusDimension::isActive()
{
    return isDimCmdActive(this);
}

void execRadius(Gui::Command* cmd)
{
    //Define the geometric configuration required for a radius dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys(
        {isCircle, isEllipse, isBSplineCircle, isBSpline});

    execDim(cmd, "Radius", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
}

//===========================================================================
// TechDraw_DiameterDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDiameterDimension)

CmdTechDrawDiameterDimension::CmdTechDrawDiameterDimension()
    : Command("TechDraw_DiameterDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Diameter Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_DiameterDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_DiameterDimension";
}

void CmdTechDrawDiameterDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execDiameter(this);
}

bool CmdTechDrawDiameterDimension::isActive()
{
    return isDimCmdActive(this);
}

void execDiameter(Gui::Command* cmd)
{
    //Define the geometric configuration required for a diameter dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys(
        {isCircle, isEllipse, isBSplineCircle, isBSpline});

    execDim(cmd, "Diameter", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
}

//===========================================================================
// TechDraw_LengthDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLengthDimension)

CmdTechDrawLengthDimension::CmdTechDrawLengthDimension()
    : Command("TechDraw_LengthDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Length Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_LengthDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_LengthDimension";
}

void CmdTechDrawLengthDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execDistance(this);
}

bool CmdTechDrawLengthDimension::isActive()
{
    return isDimCmdActive(this);
}

void execDistance(Gui::Command* cmd)
{
    StringVector acceptableGeometry({"Edge", "Vertex"});
    std::vector<int> minimumCounts({1, 2});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys(
        {isVertical, isHorizontal, isDiagonal, isHybrid});

    execDim(cmd, "Distance", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
}

//===========================================================================
// TechDraw_HorizontalDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawHorizontalDimension)

CmdTechDrawHorizontalDimension::CmdTechDrawHorizontalDimension()
    : Command("TechDraw_HorizontalDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Horizontal Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_HorizontalDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_HorizontalDimension";
    sAccel = "SHIFT+H";
}

void CmdTechDrawHorizontalDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execDistanceX(this);
}

bool CmdTechDrawHorizontalDimension::isActive()
{
    return isDimCmdActive(this);
}

void execDistanceX(Gui::Command* cmd)
{
    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Edge", "Vertex"});
    std::vector<int> minimumCounts({1, 2});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isHorizontal, isDiagonal, isHybrid});

    execDim(cmd, "DistanceX", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
}

//===========================================================================
// TechDraw_VerticalDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawVerticalDimension)

CmdTechDrawVerticalDimension::CmdTechDrawVerticalDimension()
    : Command("TechDraw_VerticalDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Vertical Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_VerticalDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_VerticalDimension";
    sAccel = "SHIFT+V";
}

void CmdTechDrawVerticalDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execDistanceY(this);
}

bool CmdTechDrawVerticalDimension::isActive()
{
    return isDimCmdActive(this);
}

void execDistanceY(Gui::Command* cmd)
{
    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Edge", "Vertex"});
    std::vector<int> minimumCounts({1, 2});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isVertical, isDiagonal, isHybrid});

    execDim(cmd, "DistanceY", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
}

//===========================================================================
// TechDraw_AngleDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAngleDimension)

CmdTechDrawAngleDimension::CmdTechDrawAngleDimension()
    : Command("TechDraw_AngleDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Angle Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_AngleDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_AngleDimension";
}

void CmdTechDrawAngleDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execAngle(this);
}

bool CmdTechDrawAngleDimension::isActive()
{
    return isDimCmdActive(this);
}

void execAngle(Gui::Command* cmd)
{
    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({2});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isAngle});

    execDim(cmd, "Angle", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
}

//===========================================================================
// TechDraw_3PtAngleDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDraw3PtAngleDimension)

CmdTechDraw3PtAngleDimension::CmdTechDraw3PtAngleDimension()
    : Command("TechDraw_3PtAngleDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert 3-Point Angle Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_3PtAngleDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_3PtAngleDimension";
}

void CmdTechDraw3PtAngleDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execAngle3Pt(this);
}

bool CmdTechDraw3PtAngleDimension::isActive()
{
    return isDimCmdActive(this);
}

void execAngle3Pt(Gui::Command* cmd)
{
    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Vertex"});
    std::vector<int> minimumCounts({3});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isAngle3Pt});

    execDim(cmd, "Angle3Pt", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
}

//===========================================================================
// TechDraw_AreaDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAreaDimension)

CmdTechDrawAreaDimension::CmdTechDrawAreaDimension()
    : Command("TechDraw_AreaDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Area Annotation");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_AreaDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_AreaDimension";
}

void CmdTechDrawAreaDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execArea(this);
}

bool CmdTechDrawAreaDimension::isActive()
{
    return isDimCmdActive(this);
}

void execArea(Gui::Command* cmd)
{
    //Define the geometric configuration required for a area dimension
    StringVector acceptableGeometry({"Face"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isFace});

    execDim(cmd, "Area", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
}


// TechDraw_LinkDimension is DEPRECATED.  Use TechDraw_DimensionRepair instead.
//! link 3D geometry to Dimension(s) on a Page
//TODO: should we present all potential Dimensions from all Pages?
//===========================================================================
// TechDraw_LinkDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLinkDimension)

CmdTechDrawLinkDimension::CmdTechDrawLinkDimension()
    : Command("TechDraw_LinkDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Link Dimension to 3D Geometry");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_LinkDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_LinkDimension";
}

void CmdTechDrawLinkDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    bool result = _checkSelection(this, 2);
    if (!result) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx(
        nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::NoResolve);

    App::DocumentObject* obj3D = nullptr;
    std::vector<App::DocumentObject*> parts;
    std::vector<std::string> subs;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++) {
        obj3D = ((*itSel).getObject());
        std::vector<std::string> subList = (*itSel).getSubNames();
        for (auto& s : subList) {
            parts.push_back(obj3D);
            subs.push_back(s);
        }
    }

    if (!obj3D) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("There is no 3D object in your selection"));
        return;
    }

    if (subs.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("There are no 3D Edges or Vertices in your selection"));
        return;
    }


    // dialog to select the Dimension to link
    Gui::Control().showDialog(new TaskDlgLinkDim(parts, subs, page));

    page->getDocument()->recompute();//still need to recompute in Gui. why?
}

bool CmdTechDrawLinkDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage && haveView && !taskInProgress);
}

//===========================================================================
// TechDraw_ExtentGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtentGroup)

CmdTechDrawExtentGroup::CmdTechDrawExtentGroup()
    : Command("TechDraw_ExtentGroup")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Extent Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_ExtentGroup";
    sStatusTip = sToolTipText;
    //    eType           = ForEdit;
}

void CmdTechDrawExtentGroup::activated(int iMsg)
{
    //    Base::Console().Message("CMD::ExtentGrp - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
        case 0:
            execExtent(this, "DistanceX");
            break;
        case 1:
            execExtent(this, "DistanceY");
            break;
        default:
            Base::Console().Message("CMD::ExtGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtentGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_HorizontalExtentDimension"));
    p1->setObjectName(QString::fromLatin1("TechDraw_HorizontalExtentDimension"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_HorizontalExtentDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_VerticalExtentDimension"));
    p2->setObjectName(QString::fromLatin1("TechDraw_VerticalExtentDimension"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_VerticalExtentDimension"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtentGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction) {
        return;
    }
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtentGroup", "Horizontal Extent"));
    arc1->setToolTip(
        QApplication::translate("TechDraw_HorizontalExtent", "Insert Horizontal Extent Dimension"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtentGroup", "Vertical Extent"));
    arc2->setToolTip(QApplication::translate("TechDraw_VerticalExtentDimension",
                                             "Insert Vertical Extent Dimension"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtentGroup::isActive()
{
    return isDimCmdActive(this);
}

//===========================================================================
// TechDraw_HorizontalExtentDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawHorizontalExtentDimension)

CmdTechDrawHorizontalExtentDimension::CmdTechDrawHorizontalExtentDimension()
    : Command("TechDraw_HorizontalExtentDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Horizontal Extent Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_HorizontalExtentDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_HorizontalExtentDimension";
}

void CmdTechDrawHorizontalExtentDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execExtent(this, "DistanceX");
}

bool CmdTechDrawHorizontalExtentDimension::isActive()
{
    return isDimCmdActive(this);
}

void execExtent(Gui::Command* cmd, const std::string& dimType)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No View of a Part in selection."));
        return;
    }
    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    // if sticky selection is in use we may get confusing selections that appear to
    // include both 2d and 3d geometry for the extent dim.
    if (!references3d.empty())  {
        for (auto& ref : references2d) {
            if (!ref.getSubName().empty()) {
                QMessageBox::warning(Gui::getMainWindow(),
                    QObject::tr("Incorrect selection"),
                    QObject::tr("Selection contains both 2D and 3D geometry"));
                return;
            }
        }
    }

    //Define the geometric configuration required for a extent dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isMultiEdge,
                                                                     isHorizontal,
                                                                     isVertical,
                                                                     isDiagonal,
                                                                     isCircle,
                                                                     isEllipse,
                                                                     isBSplineCircle,
                                                                     isBSpline,
                                                                     isZLimited});

    //what 2d geometry configuration did we receive?
    DimensionGeometryType geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == TechDraw::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Can not make 2D extent dimension from selection"));
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometryType geometryRefs3d;
    if (geometryRefs2d == TechDraw::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
                                                references3d,
                                                acceptableGeometry,
                                                minimumCounts,
                                                acceptableDimensionGeometrys);
        if (geometryRefs3d == isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Incorrect Selection"),
                                 QObject::tr("Can not make 3D extent dimension from selection"));
            return;
        }
    }

    if (references3d.empty()) {
        DrawDimHelper::makeExtentDim(partFeat, dimType, references2d);
    }
    else {
        DrawDimHelper::makeExtentDim3d(partFeat, dimType, references3d);
    }
}

//===========================================================================
// TechDraw_VerticalExtentDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawVerticalExtentDimension)

CmdTechDrawVerticalExtentDimension::CmdTechDrawVerticalExtentDimension()
    : Command("TechDraw_VerticalExtentDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Vertical Extent Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_VerticalExtentDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_VerticalExtentDimension";
}

void CmdTechDrawVerticalExtentDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execExtent(this, "DistanceY");
}

bool CmdTechDrawVerticalExtentDimension::isActive()
{
    return isDimCmdActive(this);
}

//===========================================================================
// TechDraw_DimensionRepair
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDimensionRepair)

CmdTechDrawDimensionRepair::CmdTechDrawDimensionRepair()
    : Command("TechDraw_DimensionRepair")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Repair Dimension References");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_DimensionRepair";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_DimensionRepair";
}

void CmdTechDrawDimensionRepair::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> dimObjs =
        getSelection().getObjectsOfType(TechDraw::DrawViewDimension::getClassTypeId());
    TechDraw::DrawViewDimension* dim = nullptr;
    if (dimObjs.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("There is no Dimension in your selection"));
        return;
    } else {
        dim = static_cast<TechDraw::DrawViewDimension*>(dimObjs.at(0));
    }

    Gui::Control().showDialog(new TaskDlgDimReference(dim));
}

bool CmdTechDrawDimensionRepair::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage && haveView && !taskInProgress);
}

//NOTE: to be deprecated.  revisions to the basic dimension allows it to handle
//everything that the Landmark Dimension was created to handle.
//===========================================================================
// TechDraw_LandmarkDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLandmarkDimension)

CmdTechDrawLandmarkDimension::CmdTechDrawLandmarkDimension()
    : Command("TechDraw_LandmarkDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Landmark Dimension - EXPERIMENTAL");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_LandmarkDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_LandmarkDimension";
}

void CmdTechDrawLandmarkDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this, 3);
    if (!result) {
        return;
    }

    const std::vector<App::DocumentObject*> objects =
        getSelection().getObjectsOfType(Part::Feature::getClassTypeId());//??
    if (objects.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select 2 point objects and 1 View. (1)"));
        return;
    }

    const std::vector<App::DocumentObject*> views =
        getSelection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (views.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select 2 point objects and 1 View. (2)"));
        return;
    }

    TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(views.front());

    std::vector<App::DocumentObject*> refs2d;

    std::vector<std::string> subs;
    subs.emplace_back("Vertex1");
    subs.emplace_back("Vertex1");
    TechDraw::DrawPage* page = dvp->findParentPage();
    std::string parentName = dvp->getNameInDocument();
    std::string PageName = page->getNameInDocument();

    TechDraw::LandmarkDimension* dim = nullptr;
    std::string FeatName = getUniqueObjectName("LandmarkDim");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
    doCommand(Doc,
              "App.activeDocument().addObject('TechDraw::LandmarkDimension', '%s')",
              FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.translateLabel('LandmarkDimension', 'LandmarkDim', '%s')",
              FeatName.c_str(), FeatName.c_str());

    if (objects.size() == 2) {
        //what about distanceX and distanceY??
        doCommand(Doc, "App.activeDocument().%s.Type = '%s'", FeatName.c_str(), "Distance");
        refs2d.push_back(dvp);
        refs2d.push_back(dvp);
    }
    //    } else if (objects.size() == 3) {             //not implemented yet
    //        doCommand(Doc, "App.activeDocument().%s.Type = '%s'", FeatName.c_str(), "Angle3Pt");
    //        refs2d.push_back(dvp);
    //        refs2d.push_back(dvp);
    //        refs2d.push_back(dvp);
    //        //subs.push_back("Vertex1");
    //        //subs.push_back("Vertex1");
    //        //subs.push_back("Vertex1");
    //    }

    dim = dynamic_cast<TechDraw::LandmarkDimension*>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawLandmarkDimension - dim not found\n");
    }
    dim->References2D.setValues(refs2d, subs);
    dim->References3D.setValues(objects, subs);
    doCommand(Doc,
              "App.activeDocument().%s.addView(App.activeDocument().%s)",
              PageName.c_str(),
              FeatName.c_str());

    commitCommand();

    // Touch the parent feature so the dimension in tree view appears as a child
    dvp->touch(true);

    dim->recomputeFeature();
}

bool CmdTechDrawLandmarkDimension::isActive()
{
    return isDimCmdActive(this);
}

//------------------------------------------------------------------------------

void CreateTechDrawCommandsDims()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawDimension());
    rcCmdMgr.addCommand(new CmdTechDrawRadiusDimension());
    rcCmdMgr.addCommand(new CmdTechDrawDiameterDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLengthDimension());
    rcCmdMgr.addCommand(new CmdTechDrawHorizontalDimension());
    rcCmdMgr.addCommand(new CmdTechDrawVerticalDimension());
    rcCmdMgr.addCommand(new CmdTechDrawAngleDimension());
    rcCmdMgr.addCommand(new CmdTechDraw3PtAngleDimension());
    rcCmdMgr.addCommand(new CmdTechDrawAreaDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtentGroup());
    rcCmdMgr.addCommand(new CmdTechDrawVerticalExtentDimension());
    rcCmdMgr.addCommand(new CmdTechDrawHorizontalExtentDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLinkDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLandmarkDimension());
    rcCmdMgr.addCommand(new CmdTechDrawDimensionRepair());
    rcCmdMgr.addCommand(new CmdTechDrawCompDimensionTools());
}

//------------------------------------------------------------------------------

//Common code to build a dimension feature

void execDim(Gui::Command* cmd, std::string type, StringVector acceptableGeometry, std::vector<int> minimumCounts, std::vector<DimensionGeometryType> acceptableDimensionGeometrys)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("Incorrect selection"),
            QObject::tr("No View of a Part in selection."));
        return;
    }

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    //what 2d geometry configuration did we receive?
    DimensionGeometryType geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == TechDraw::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("Incorrect Selection"),
            QObject::tr("Can not make 2D dimension from selection"));
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometryType geometryRefs3d{TechDraw::isInvalid};
    if (geometryRefs2d == TechDraw::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
            references3d,
            acceptableGeometry,
            minimumCounts,
            acceptableDimensionGeometrys);

        if (geometryRefs3d == TechDraw::isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                QObject::tr("Incorrect Selection"),
                QObject::tr("Can not make 3D dimension from selection"));
            return;
        }
    }
    else {
        references3d.clear();
    }

    //errors and warnings
    if (type == "Radius" || type == "Diameter") {
        if (geometryRefs2d == isEllipse || geometryRefs3d == isEllipse) {
            QMessageBox::StandardButton result = QMessageBox::warning(
                Gui::getMainWindow(),
                QObject::tr("Ellipse Curve Warning"),
                QObject::tr("Selected edge is an Ellipse. Value will be approximate. Continue?"),
                QMessageBox::Ok | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (result != QMessageBox::Ok) {
                return;
            }
        }
        if (geometryRefs2d == isBSplineCircle || geometryRefs3d == isBSplineCircle) {
            QMessageBox::StandardButton result = QMessageBox::warning(
                Gui::getMainWindow(),
                QObject::tr("BSpline Curve Warning"),
                QObject::tr("Selected edge is a B-spline. Value will be approximate. Continue?"),
                QMessageBox::Ok | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (result != QMessageBox::Ok) {
                return;
            }
        }
        if (geometryRefs2d == isBSpline || geometryRefs3d == isBSpline) {
            QMessageBox::critical(
                Gui::getMainWindow(),
                QObject::tr("BSpline Curve Error"),
                QObject::tr("Selected edge is a B-spline and a radius/diameter can not be calculated."));
            return;
        }
    }

    //build the dimension
    DrawViewDimension* dim = dimensionMaker(partFeat, type, references2d, references3d);

    if (type == "Distance" || type == "DistanceX" || type == "DistanceY" || type == "Angle" || type == "Angle3Pt") {
        //position the Dimension text on the view
        positionDimText(dim);
    }
}

DrawViewDimension* dimensionMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                                  ReferenceVector references2d, ReferenceVector references3d)
{
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));

    TechDraw::DrawViewDimension* dim = dimMaker(dvp, dimType, references2d, references3d);

    Gui::Command::commitCommand();

    // Touch the parent feature so the dimension in tree view appears as a child
    dvp->touch(true);

    // Select only the newly created dimension
    Gui::Selection().clearSelection();
    Gui::Selection().addSelection(dvp->getDocument()->getName(), dim->getNameInDocument());

    return dim;
}

DrawViewDimension* dimMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                            ReferenceVector references2d, ReferenceVector references3d)
{
    TechDraw::DrawPage* page = dvp->findParentPage();
    std::string PageName = page->getNameInDocument();

    std::string dimName = dvp->getDocument()->getUniqueObjectName("Dimension");

    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().addObject('TechDraw::DrawViewDimension', '%s')",
                            dimName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewDimension', 'Dimension', '%s')",
              dimName.c_str(), dimName.c_str());

    Gui::Command::doCommand(
        Gui::Command::Doc, "App.activeDocument().%s.Type = '%s'", dimName.c_str(), dimType.c_str());

    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().%s.MeasureType = '%s'",
                            dimName.c_str(),
                            "Projected");

    auto* dim = dynamic_cast<TechDraw::DrawViewDimension*>(dvp->getDocument()->getObject(dimName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawNewDiameterDimension - dim not found\n");
    }

    //always have References2D, even if only for the parent DVP
    dim->setReferences2d(references2d);
    dim->setReferences3d(references3d);

    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().%s.addView(App.activeDocument().%s)",
                            PageName.c_str(),
                            dimName.c_str());


    dim->recomputeFeature();

    return dim;
}

//position the Dimension text on the view
void positionDimText(DrawViewDimension* dim, int offsetIndex)
{
    TechDraw::pointPair pp = dim->getLinearPoints();
    Base::Vector3d mid = (pp.first() + pp.second()) / 2.0;
    dim->X.setValue(mid.x);
    double fontSize = Preferences::dimFontSizeMM();
    dim->Y.setValue(-mid.y + (offsetIndex * 1.5 + 0.5) * fontSize);
}
//===========================================================================
// Selection Validation Helpers
//===========================================================================

//! common checks of Selection for Dimension commands
//non-empty selection, no more than maxObjs selected and at least 1 DrawingPage exists
bool _checkSelection(Gui::Command* cmd, unsigned maxObjs)
{
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (selection.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("Select an object first"));
        return false;
    }

    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() > maxObjs) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("Too many objects selected"));
        return false;
    }

    std::vector<App::DocumentObject*> pages =
        cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("Create a page first."));
        return false;
    }
    return true;
}

bool _checkDrawViewPart(Gui::Command* cmd)
{
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    for (auto& sel : selection) {
        auto dvp = dynamic_cast<TechDraw::DrawViewPart*>(sel.getObject());
        if (dvp) {
            return true;
        }
    }
    return false;
}
