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

#include "TechDrawLeaderLineHandler.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QPointF>
#include <QGraphicsItem>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

#include <App/Document.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/Selection/Selection.h>

#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/CosmeticVertex.h>

#include "QGIEdge.h"
#include "QGIVertex.h"
#include "QGIFace.h"
#include "QGIView.h"
#include "QGSPage.h"
#include "QGVPage.h"
#include "CommandHelpers.h"
#include "MDIViewPage.h"
#include "Rez.h"
#include "ViewProviderLeader.h"


using namespace TechDrawGui;
using namespace TechDraw;

void reorderClosestPoints(std::vector<QPointF>& points, const QPointF& referencePoint) {
    if (points.size() < 2) {
        return;
    }

    QPointF a = points[0];
    QPointF b = points[1];

    double distA = std::hypot(a.x() - referencePoint.x(), a.y() - referencePoint.y());
    double distB = std::hypot(b.x() - referencePoint.x(), b.y() - referencePoint.y());

    if (distB < distA) {
        std::swap(points[0], points[1]);
    }
}

void TechDrawLeaderLineHandler::addLeaderLine(TechDraw::DrawPage* basePage) {
    const std::string objectName{"LeaderLine"};
    std::string m_leaderName = basePage->getDocument()->getUniqueObjectName(objectName.c_str());
    std::string m_leaderType = "TechDraw::DrawLeaderLine";

    std::string PageName = basePage->getNameInDocument();

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create Leader"));
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('%s', '%s')",
                       m_leaderType.c_str(), m_leaderName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawLeaderLine', 'LeaderLine', '%s')",
              m_leaderName.c_str(), m_leaderName.c_str());

    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)",
                       PageName.c_str(), m_leaderName.c_str());

    App::DocumentObject* obj = basePage->getDocument()->getObject(m_leaderName.c_str());

    Gui::Command::updateActive();
    Gui::Command::commitCommand(tid);
    if (!obj->isDerivedFrom<TechDraw::DrawLeaderLine>()) {
        return;
    }

    leaderLineObj = static_cast<TechDraw::DrawLeaderLine*>(obj);
}

void TechDrawLeaderLineHandler::createLeaderLineObject() {
    
    // Most likely it is getting placed by some annotation
    if (!baseFeat) {
        // Random view for now
        auto items = viewPage->items();
        for (auto item : items) {
            if (auto view = dynamic_cast<QGIView*>(item)) {
                baseFeatView = view;
                baseFeat = view->getViewObject();
                reverse = true;
                break;
            }
        }
    }

    addLeaderLine(viewPage->getDrawPage());
    if (!leaderLineObj) {
        return;
    }

    oldLeaderType = currentLeaderType;
    updateLeader();

    switch (currentLeaderType) {
        case LeaderType::Normal:
            leaderLineObj->Type.setValue("Normal");
            break;
        case LeaderType::Straight:
            leaderLineObj->Type.setValue("Straight");
            break;
        case LeaderType::Complex:
            leaderLineObj->Type.setValue("Complex");
            break;
        case LeaderType::ComplexWithKink:
            leaderLineObj->Type.setValue("Complex with kink");
            break;
        default:
            leaderLineObj->Type.setValue("Normal");
    }

    leaderLineObj->LeaderParent.setValue(baseFeat);
}

void TechDrawLeaderLineHandler::deleteLeaderLine() {
    if (!leaderLineObj) {
        return;
    }

    TechDraw::DrawPage* page = leaderLineObj->findParentPage();
    if (!page) {
        return;
    }

    std::string PageName = page->getNameInDocument();
    std::string LeaderName = leaderLineObj->getNameInDocument();

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Delete Leader"));
    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.removeView(App.activeDocument().%s)",
                            PageName.c_str(), LeaderName.c_str());
    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().removeObject('%s')",
                            LeaderName.c_str());

    Gui::Command::updateActive();
    Gui::Command::commitCommand(tid);

    leaderLineObj = nullptr;
    sceneClickPoints.clear();
    waypoints.clear();
    baseFeat = nullptr;
    baseFeatView = nullptr;
    finished = false;
    reverse = false;
}

void TechDrawLeaderLineHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("actions/TechDraw_LeaderLine_Pointer.svg", hotspot);
        cursor = QCursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }

}

void TechDrawLeaderLineHandler::deactivated()
{
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }
}

void TechDrawLeaderLineHandler::updateLeaderPoints(std::vector<QPointF> points) {
    if (points.empty()) {
        points = sceneClickPoints;
    }

    QPointF firstPoint = points.front();

    std::vector<Base::Vector3d> pageDeltas;
    pageDeltas.reserve(points.size());
    for (auto& pt : points) {
        QPointF distFromFirst = pt - firstPoint;
        pageDeltas.push_back(Rez::appX(DrawUtil::toVector3d(distFromFirst)));
    }
    waypoints = leaderLineObj->makeCanonicalPointsInverted(pageDeltas);
    leaderLineObj->WayPoints.setValues(waypoints);
}

void TechDrawLeaderLineHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (auto vp = viewPage->viewport()) {
        vp->setCursor(cursor);
    }

    if (finished) {
        return;
    }

    if (sceneClickPoints.empty()) {
        return;
    }

    if (reverse && sceneClickPoints.size() >= 2
        && (currentLeaderType == LeaderType::Normal || currentLeaderType == LeaderType::ComplexWithKink)) {
        sceneClickPoints = updateKinkLength(sceneClickPoints);
    }

    QPointF scenePos = viewPage->mapToScene(event->pos());
    std::vector<QPointF> points;

    if (reverse) {
        if (currentLeaderType == LeaderType::Normal && sceneClickPoints.size() == 2) {
            points.push_back(scenePos);
            points.push_back(sceneClickPoints.front());
            points.push_back(sceneClickPoints.back());
        }
        if (currentLeaderType == LeaderType::Straight && sceneClickPoints.size() == 1) {
            points.push_back(scenePos);
            points.push_back(sceneClickPoints.front());
            leaderLineObj->AutoHorizontal.setValue(false);
        }
        if (currentLeaderType == LeaderType::Complex) {
            points.insert(points.end(), sceneClickPoints.begin(), sceneClickPoints.end());
            points.push_back(scenePos);
            std::reverse(points.begin(), points.end());
            leaderLineObj->AutoHorizontal.setValue(false);
        }
        if (currentLeaderType == LeaderType::ComplexWithKink) {
            points.insert(points.end(), sceneClickPoints.begin(), sceneClickPoints.end());
            points.push_back(scenePos);
            std::reverse(points.begin(), points.end());
        }

        if (baseFeatView && baseFeat) {
            QPointF localOffset = scenePos - baseFeatView->scenePos();
            auto attachPoint = DrawUtil::toVector3d(localOffset);
            double baseRotation = baseFeat->Rotation.getValue();
            if (baseRotation != 0) {
                attachPoint = DrawUtil::invertY(attachPoint);
                attachPoint.RotateZ(-Base::toRadians(baseRotation));
                attachPoint = DrawUtil::invertY(attachPoint);
            }
            Base::Vector3d attachDelta(Rez::appX(attachPoint.x), Rez::appX(-attachPoint.y), 0.0);
            leaderLineObj->setPosition(attachDelta.x, attachDelta.y, true);
        }
    }
    else {
        if (currentLeaderType == LeaderType::Normal) {
            points.push_back(sceneClickPoints.front());
            QPointF kinkStart = scenePos;
            kinkStart.setX(kinkStart.x() - Rez::guiX(kinkLength / 2.0));
            QPointF kinkEnd = scenePos;
            kinkEnd.setX(kinkEnd.x() + Rez::guiX(kinkLength / 2.0));

            std::vector<QPointF> kinkPoints = {kinkStart, kinkEnd};
            reorderClosestPoints(kinkPoints, sceneClickPoints.front());

            points.push_back(kinkPoints[0]);
            points.push_back(kinkPoints[1]);
        }
        else if (currentLeaderType == LeaderType::Straight) {
            points.push_back(sceneClickPoints.front());
            points.push_back(scenePos);
            leaderLineObj->AutoHorizontal.setValue(false);
        }
    }

    updateLeaderPoints(points);

    leaderLineObj->recomputeFeature();
}

void TechDrawLeaderLineHandler::mouseReleaseEvent(QMouseEvent* event)
{
    if (finished) {
        return;
    }

    if (event->button() == Qt::RightButton) {
        deleteLeaderLine();
        return;
    }

    QPointF scenePos = viewPage->mapToScene(event->pos());
    QPainterPath circle;
    circle.addEllipse(scenePos, 5.0, 5.0);
    auto items = viewPage->scene()->items(circle);


    if (!baseFeat || reverse) {
        for (auto item : items) {
            QGIView* qgiView = nullptr;
            if (dynamic_cast<QGIEdge*>(item) || dynamic_cast<QGIVertex*>(item) || dynamic_cast<QGIFace*>(item)) {
                qgiView = dynamic_cast<QGIView*>(item->parentItem());
            }
            if (qgiView) {
                if (dynamic_cast<TechDraw::DrawLeaderLine*>(qgiView->getViewObject())) {
                    continue;
                }
                if (dynamic_cast<TechDraw::DrawProjGroup*>(qgiView->getViewObject())) {
                    continue;
                }

                baseFeat = qgiView->getViewObject();
                baseFeatView = qgiView;

                if (reverse) {
                    if (leaderLineObj) {
                        // Dot startsymbol
                        if (dynamic_cast<QGIFace*>(item)) {
                            leaderLineObj->StartSymbol.setValue(3L);
                        }

                        leaderLineObj->LeaderParent.setValue(baseFeat);

                        QPointF localOffset = scenePos - baseFeatView->scenePos();
                        Base::Vector3d attachPoint = DrawUtil::toVector3d(localOffset);
                        double baseRotation = baseFeat->Rotation.getValue();
                        if (baseRotation != 0) {
                            attachPoint = DrawUtil::invertY(attachPoint);
                            attachPoint.RotateZ(-Base::toRadians(baseRotation));
                            attachPoint = DrawUtil::invertY(attachPoint);
                        }
                        Base::Vector3d attachDelta(Rez::appX(attachPoint.x), Rez::appX(-attachPoint.y), 0.0);
                        leaderLineObj->setPosition(attachDelta.x, attachDelta.y, true);

                        leaderLineObj->recomputeFeature();
                        finished = true;
                        if (attached) {
                            Gui::Selection().clearSelection();
                            if (onFinished) {
                                onFinished();
                            }
                            viewPage->deactivateHandler();
                            return;
                        }
                    }
                }

                break;
            }
        }
        if (!baseFeat) {
            auto items = viewPage->items();
            for (auto item : items) {
                if (auto view = dynamic_cast<QGIView*>(item)) {
                    baseFeatView = view;
                    baseFeat = view->getViewObject();
                    reverse = true;
                    break;
                }
            }
        }
        if (!baseFeat) {
            return;
        }
    }
    
    Gui::Selection().clearSelection();

    if (reverse) {
        if (currentLeaderType == LeaderType::Normal && sceneClickPoints.empty()) {
            QPointF kinkStart = scenePos;
            kinkStart.setX(kinkStart.x() - Rez::guiX(kinkLength / 2.0));
            QPointF kinkEnd = scenePos;
            kinkEnd.setX(kinkEnd.x() + Rez::guiX(kinkLength / 2.0));

            sceneClickPoints.push_back(kinkStart);
            sceneClickPoints.push_back(kinkEnd);
        }
        if (currentLeaderType == LeaderType::Straight && sceneClickPoints.empty()) {
            sceneClickPoints.push_back(scenePos);
        }
        if (currentLeaderType == LeaderType::Complex) {
            sceneClickPoints.push_back(scenePos);
        }
        if (currentLeaderType == LeaderType::ComplexWithKink) {
            if (sceneClickPoints.empty()) {
                QPointF kinkStart = scenePos;
                kinkStart.setX(kinkStart.x() + Rez::guiX(kinkLength / 2.0));
                QPointF kinkEnd = scenePos;
                kinkEnd.setX(kinkEnd.x() - Rez::guiX(kinkLength / 2.0));

                sceneClickPoints.push_back(kinkStart);
                sceneClickPoints.push_back(kinkEnd);
            }
            else {
                sceneClickPoints.push_back(scenePos);
            }
        }
    }
    else {
        sceneClickPoints.push_back(scenePos);
    }

    if (!leaderLineObj) {
        createLeaderLineObject();

        for (auto& item : items) {
            if (dynamic_cast<QGIEdge*>(item) || dynamic_cast<QGIVertex*>(item)) {
                break;
            }
            if (dynamic_cast<QGIFace*>(item)) {
                leaderLineObj->StartSymbol.setValue(3L);
                break;
            }
        }

        QPointF localOffset = scenePos - baseFeatView->scenePos();
        Base::Vector3d attachPoint = DrawUtil::toVector3d(localOffset);
        double baseRotation = baseFeat->Rotation.getValue();
        if (baseRotation != 0) {
            attachPoint = DrawUtil::invertY(attachPoint);
            attachPoint.RotateZ(-Base::toRadians(baseRotation));
            attachPoint = DrawUtil::invertY(attachPoint);
        }
        Base::Vector3d attachDelta(Rez::appX(attachPoint.x), Rez::appX(-attachPoint.y), 0.0);
        leaderLineObj->setPosition(attachDelta.x, attachDelta.y, true);
    }


    if ((currentLeaderType == LeaderType::Complex || currentLeaderType == LeaderType::ComplexWithKink) && !reverse) {
        if (currentLeaderType == LeaderType::ComplexWithKink) {
            if (sceneClickPoints.size() > 2) {
                sceneClickPoints.erase(sceneClickPoints.end() - 2);
            }
            if (sceneClickPoints.size() > 1) {
                QPointF kinkStart = sceneClickPoints.back();
                const QPointF& prevPoint = sceneClickPoints[sceneClickPoints.size() - 2];

                QPointF kinkEnd = kinkStart;
                if (kinkStart.x() < prevPoint.x()) {
                    kinkEnd.setX(kinkEnd.x() - Rez::guiX(kinkLength));
                }
                else {
                    kinkEnd.setX(kinkEnd.x() + Rez::guiX(kinkLength));
                }

                sceneClickPoints.push_back(kinkEnd);
            }
        }

        updateLeaderPoints();
        leaderLineObj->AutoHorizontal.setValue(false);
        leaderLineObj->recomputeFeature();
    }
    
    if (currentLeaderType == LeaderType::Normal && sceneClickPoints.size() == 2 && !reverse) {
        finished = true;
        if (attached) {
            if (onFinished) {
                onFinished();
            }
            viewPage->deactivateHandler();
            return;
        }
    }
    if (currentLeaderType == LeaderType::Straight && sceneClickPoints.size() == 2 && !reverse) {
        finished = true;
        if (attached) {
            if (onFinished) {
                onFinished();
            }
            viewPage->deactivateHandler();
            return;
        }
    }
}

void TechDrawLeaderLineHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawLeaderLineHandler::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        event->accept();
    }
}

void TechDrawLeaderLineHandler::updateLeader()
{
    if (!leaderLineObj) {
        return;
    }

    if (oldLeaderType != currentLeaderType) {
        deleteLeaderLine();
        oldLeaderType = currentLeaderType;
        baseFeat = nullptr;
        baseFeatView = nullptr;
        return;
    }

    auto* viewProvider =
        Gui::Application::Instance->getViewProvider<TechDrawGui::ViewProviderLeader>(leaderLineObj);

    if (viewProvider) {
        viewProvider->Color.setValue(color);
        viewProvider->LineStyle.setValue(lineStyle);
        viewProvider->LineWidth.setValue(lineWidth);
    }

    leaderLineObj->StartSymbol.setValue(startSymbol);
    leaderLineObj->EndSymbol.setValue(endSymbol);

    std::vector<Base::Vector3d> updatedWaypoints = updateKinkLength(waypoints);
    if (updatedWaypoints.size() >= 3) {
        leaderLineObj->WayPoints.setValues(updatedWaypoints);
    }

    leaderLineObj->recomputeFeature();
}

std::vector<Base::Vector3d> TechDrawLeaderLineHandler::updateKinkLength(std::vector<Base::Vector3d> waypoints) {
    if (attached) {
        return waypoints;
    }
    if (currentLeaderType == LeaderType::Normal || currentLeaderType == LeaderType::ComplexWithKink) {
        if (waypoints.size() >= 3) {
            Base::Vector3d kinkStart, kinkEnd;
            if (currentLeaderType == LeaderType::Normal) {
                kinkStart = waypoints[1];
                kinkEnd = waypoints[2];
            }
    
            if (currentLeaderType == LeaderType::ComplexWithKink) {
                kinkStart = waypoints[waypoints.size() - 2];
                kinkEnd = waypoints[waypoints.size() - 1];
            }
    
            double currentKinkLength = std::hypot(kinkEnd.x - kinkStart.x, kinkEnd.y - kinkStart.y);
    
            if (kinkLength != currentKinkLength) {
                Base::Vector3d mid = (kinkStart + kinkEnd) / 2.0;
                double halfLength = (kinkEnd.x >= kinkStart.x) ? kinkLength / 2.0 : -kinkLength / 2.0;
    
                kinkStart.x = mid.x - halfLength;
                kinkEnd.x = mid.x + halfLength;
            }
    
            if (currentLeaderType == LeaderType::Normal) {
                waypoints[1] = kinkStart;
                waypoints[2] = kinkEnd;
            }
            else if (currentLeaderType == LeaderType::ComplexWithKink) {
                waypoints[waypoints.size() - 2] = kinkStart;
                waypoints[waypoints.size() - 1] = kinkEnd;
            }
        }
    }

    return waypoints;
}

std::vector<QPointF> TechDrawLeaderLineHandler::updateKinkLength(std::vector<QPointF> scenePoints) {
    if (attached) {
        return scenePoints;
    }
    if ((currentLeaderType == LeaderType::Normal || currentLeaderType == LeaderType::ComplexWithKink)
        && scenePoints.size() >= 2) {
        QPointF& kinkStart = scenePoints[0];
        QPointF& kinkEnd = scenePoints[1];

        double currentKinkLength = std::hypot(kinkEnd.x() - kinkStart.x(), kinkEnd.y() - kinkStart.y());
        double actualKinkLength = Rez::guiX(kinkLength);

        if (actualKinkLength != currentKinkLength) {
            QPointF mid = (kinkStart + kinkEnd) / 2.0;
            double halfLength = (kinkEnd.x() >= kinkStart.x()) ? actualKinkLength / 2.0 : -actualKinkLength / 2.0;

            kinkStart.setX(mid.x() - halfLength);
            kinkEnd.setX(mid.x() + halfLength);
        }
    }

    return scenePoints;
}
