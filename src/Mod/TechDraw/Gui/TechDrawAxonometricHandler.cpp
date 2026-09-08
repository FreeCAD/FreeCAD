// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Morten Vajhøj
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#include "TechDrawAxonometricHandler.h"

#include <cmath>
#include <numbers>
#include <tuple>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QString>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/InputHint.h>

#include "QGVPage.h"
#include "QGIView.h"
#include "Rez.h"
#include "ViewProviderDrawingView.h"
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>


using namespace TechDrawGui;
using namespace TechDraw;


static double makePlumb(double dimensionLineAngle)
{
    double HalfPi = std::numbers::pi / 2.0;
    double AngularTolerance = 0.01; // This value is a guess.  It works for the file in
    // issue #16172, but seems small (~0.5deg).

    if (std::abs(dimensionLineAngle - HalfPi) < AngularTolerance) {
        return HalfPi;
    }
    if (std::abs(dimensionLineAngle + HalfPi) < AngularTolerance) {
        return -HalfPi;
    }

    return dimensionLineAngle;
}

static std::tuple<Base::Vector3d, Base::Vector3d, Base::Vector3d> getCoordinateVectors(TechDraw::DrawViewPart* view)
{
    Base::Vector3d diagonal = view->Direction.getValue();
    Base::Vector3d xDirection = view->getXDirection();
    Base::Vector3d origin;

    Base::Vector3d xAxisProj(1.0, 0.0, 0.0);
    xAxisProj.ProjectToPlane(origin, diagonal);
    Base::Vector3d yAxisProj(0.0, 1.0, 0.0);
    yAxisProj.ProjectToPlane(origin, diagonal);
    Base::Vector3d zAxisProj(0.0, 0.0, 1.0);
    zAxisProj.ProjectToPlane(origin, diagonal);

    // roll back in 3D
    Base::Rotation rotation3D(diagonal, Base::Vector3d(0.0, 0.0, 1.0));
    Base::Vector3d px = rotation3D.multVec(xAxisProj);
    Base::Vector3d py = rotation3D.multVec(yAxisProj);
    Base::Vector3d pz = rotation3D.multVec(zAxisProj);
    Base::Vector3d pDir = rotation3D.multVec(xDirection);
    pDir.z = 0.0;

    // rotate inside drawing plane
    Base::Rotation rotation2D(pDir, Base::Vector3d(1.0, 0.0, 0.0));
    px = rotation2D.multVec(px);
    py = rotation2D.multVec(py);
    pz = rotation2D.multVec(pz);

    return {px, py, pz};
}

static std::string formatValueToSpec(double value, std::string formatSpec)
{
    auto wPos = formatSpec.find('w');
    if (wPos == std::string::npos) {
        return QString::asprintf(formatSpec.c_str(), value).toStdString();
    }

    formatSpec[wPos] = 'f';
    auto numDig = formatSpec.find('.');
    if (numDig == std::string::npos) {
        return {};
    }
    numDig = numDig + 1;

    int digits = formatSpec[numDig] - '0';
    double factor = std::pow(10.0, digits);
    value = std::round(value * factor) / factor;

    QString strValue = QString::asprintf(formatSpec.c_str(), value);
    while (strValue.endsWith(u'0')) {
        strValue.chop(1);
    }
    if (strValue.endsWith(u'.')) {
        strValue.chop(1);
    }

    return strValue.toStdString();
}

static void setReferences(TechDraw::DrawViewDimension* dimension, TechDraw::DrawViewPart* view,
                           const std::vector<std::string>& edgeNameList,
                           const std::vector<std::string>& vertexNameList)
{
    TechDraw::ReferenceVector references;
    if (!vertexNameList.empty()) {
        for (const std::string& vert : vertexNameList) {
            references.emplace_back(view, vert);
        }
    } else {
        references.emplace_back(view, edgeNameList[0]);
    }

    dimension->setReferences2d(references);
}

void TechDrawAxonometricHandler::removeDimension()
{
    if (!distanceDim || !dimParentView) {
        return;
    }

    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    activeDoc->openTransaction("Remove axonometric length dimension");

    std::string dimName = distanceDim->getNameInDocument();

    Gui::Command::doCommand(Gui::Command::Doc,
                                "App.activeDocument().removeObject(App.activeDocument().getObject('%s'))",
                                dimName);

    distanceDim = nullptr;
    dimParentView = nullptr;

    activeDoc->commitTransaction();
}

void TechDrawAxonometricHandler::execAxonometric() {


    std::vector<TechDraw::BaseGeomPtr> edges;
    std::vector<Base::Vector3d> vertexes;

    std::vector<std::string> edgeNames;
    std::vector<std::string> vertNames;

    TechDraw::DrawViewPart* view = nullptr;

    std::vector<Gui::SelectionObject> selObjs = Gui::Selection().getSelectionEx();
    
    for (auto& selObj : selObjs) {
        
        TechDraw::DrawViewPart* dvp = selObj.getObject<TechDraw::DrawViewPart>();
        if (!dvp) {
            continue;
        }
        view = dvp;

        const std::vector<std::string>& subNames = selObj.getSubNames();
        for (auto& subName : subNames) {
            std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
            int geoId = TechDraw::DrawUtil::getIndexFromName(subName);

            if (geomType == "Edge") 
            {
                edges.push_back(dvp->getGeomByIndex(geoId));
                edgeNames.push_back(subName);
            } 
            else if (geomType == "Vertex") {
                if (TechDraw::VertexPtr vert = dvp->getProjVertexByIndex(geoId)) {
                    vertexes.push_back(vert->point());
                    vertNames.push_back(subName);
                }
            }
        }
    }

    if (vertexes.size() < 2 && !edges.empty()) {
        vertexes.clear();
        vertexes.push_back(edges[0]->getStartPoint());
        vertexes.push_back(edges[0]->getEndPoint());
    }

    edgesSelected = edges.size();

    if (edges.size() < 2 || !view) {
        return;
    }

    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    activeDoc->openTransaction("Create axonometric length dimension");

    TechDraw::DrawPage* page = view->findParentPage();

    Base::Vector3d extLineVec =
        TechDraw::DrawUtil::invertY(edges[1]->getEndPoint() - edges[1]->getStartPoint());
    Base::Vector3d dimLineVec =
        TechDraw::DrawUtil::invertY(edges[0]->getEndPoint() - edges[0]->getStartPoint());

    Base::Vector3d xAxis(1.0, 0.0, 0.0);
    double extAngle = Base::toDegrees(extLineVec.GetAngle(xAxis));
    double lineAngle = Base::toDegrees(makePlumb(dimLineVec.GetAngle(xAxis)));

    if (extLineVec.y < 0.0) {
        extAngle = 180.0 - extAngle;
    }
    if (dimLineVec.y < 0.0) {
        lineAngle = 180.0 - lineAngle;
    }

    if (std::fabs(extAngle-lineAngle)>0.1) {
        // re issue: https://github.com/FreeCAD/FreeCAD/issues/13677
        // Instead of using makeDistanceDim (which is meant for extent dimensions), we use the
        // same steps as in CommandCreateDims.cpp to create a regular length dimension.  This avoids
        // the creation of CosmeticVertex objects to serve as dimension points.  These CosmeticVertex
        // objects are never deleted, but are no longer used once their dimension is deleted.
        // distanceDim = TechDraw::DrawDimHelper::makeDistanceDim(view, "Distance", vertexes[0]*scale, vertexes[1]*scale);
        std::string dimName = view->getDocument()->getUniqueObjectName("Dimension");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.activeDocument().addObject('TechDraw::DrawViewDimension', '%s')",
                                dimName.c_str());

        distanceDim = dynamic_cast<TechDraw::DrawViewDimension*>(view->getDocument()->getObject(dimName.c_str()));
        dimParentView = view;
        distanceDim->Type.setValue("Distance");
        distanceDim->MeasureType.setValue("Projected"); // should this not be True?
        setReferences(distanceDim, view, edgeNames, vertNames);
        page->addView(distanceDim);

        distanceDim->AngleOverride.setValue(true);
        distanceDim->LineAngle.setValue(lineAngle);
        distanceDim->ExtensionAngle.setValue(extAngle);

        distanceDim->recomputeFeature();     // ensure linearPoints has been set

        // as in CmdCreateDims::positionDimText:
        TechDraw::pointPair linearPoints = distanceDim->getLinearPoints();
        Base::Vector3d mid = (linearPoints.first() + linearPoints.second()) / 2.0;
        distanceDim->X.setValue(mid.x);
        distanceDim->Y.setValue(-mid.y);

        auto [px, py, pz] = getCoordinateVectors(view);
        TechDraw::pointPair arrowTips = distanceDim->getArrowPositions();
        double value2D = (arrowTips.second() - arrowTips.first()).Length();
        double value3D = 1.0;
        if (px.IsParallel(dimLineVec, 0.1)) {
            value3D = value2D/px.Length();
        } else if (py.IsParallel(dimLineVec, 0.1)) {
            value3D = value2D/py.Length();
        } else if (pz.IsParallel(dimLineVec, 0.1)) {
            value3D = value2D/pz.Length();
        }
        if (value3D != 1.0) {
            std::string fomatted3DValue = formatValueToSpec(value3D, distanceDim->FormatSpec.getStrValue());
            distanceDim->Arbitrary.setValue(true);
            std::string label = distanceDim->Label.getValue();
            std::string oldText = "Dimension";
            std::string newText = "Dimension3D";
            label.replace(0, oldText.length(), newText);

            distanceDim->Label.setValue(label);
            distanceDim->FormatSpec.setValue(fomatted3DValue);
        }

        distanceDim->recomputeFeature();
        view->requestPaint();
    }

    Gui::Selection().clearSelection();
    activeDoc->commitTransaction();
    view->touch(); // make view claim its new child
}

void TechDrawAxonometricHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("actions/TechDraw_AxoLengthDimension_Pointer.svg", hotspot);
        cursor = QCursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }

    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::GreedySelection);
}

void TechDrawAxonometricHandler::deactivated()
{
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }

    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
    Gui::Selection().clearSelection();
}

std::list<Gui::InputHint> TechDrawAxonometricHandler::getToolHints() const
{
    if (movingDim) {
        return {
            {QObject::tr("%1 place dimension"),
                {Gui::InputHint::UserInput::MouseLeft}},
        };
    }
    else if (edgesSelected == 0) {
        return {
            {QObject::tr("%1 pick edge to dimension"),
                {Gui::InputHint::UserInput::MouseLeft}},
        };
    }
    else if (edgesSelected == 1) {
        return {
            {QObject::tr("%1 pick dimension direction edge"),
                {Gui::InputHint::UserInput::MouseLeft}},
        };
    }

    return {};
}

void TechDrawAxonometricHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (auto vp = viewPage->viewport()) {
        vp->setCursor(cursor);
    }

    if (movingDim && distanceDim && dimParentView) {
        auto* vpp = freecad_cast<TechDrawGui::ViewProviderDrawingView*>(
            Gui::Application::Instance->getViewProvider(dimParentView));
        
        if (vpp && vpp->getQView()) {
            QPointF scenePos = viewPage->mapToScene(event->pos()) - vpp->getQView()->scenePos();
            distanceDim->X.setValue(Rez::appX(scenePos.x()));
            distanceDim->Y.setValue(-Rez::appX(scenePos.y()));
        }
    }

}

void TechDrawAxonometricHandler::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MouseButton::RightButton && movingDim) {
        removeDimension();
        return;
    }
    
    execAxonometric();

    if (movingDim) {
        movingDim = false;
        distanceDim = nullptr;
        dimParentView = nullptr;
    }

    if (distanceDim && !movingDim) {
        movingDim = true;
    }

    updateHint();

    Q_UNUSED(event);
}

void TechDrawAxonometricHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawAxonometricHandler::keyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawAxonometricHandler::addPreselected() {
    execAxonometric();
    dimParentView = nullptr;
    distanceDim = nullptr;
    movingDim = false;
    updateHint();
}