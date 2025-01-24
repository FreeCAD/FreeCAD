/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
# include <cmath>

# include <QGraphicsScene>
# include <QGraphicsSceneMouseEvent>
# include <QPainter>
# include <QPainterPath>
# include <QVector2D>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Mod/TechDraw/App/ArrowPropEnum.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "QGILeaderLine.h"
#include "PreferencesGui.h"
#include "QGEPath.h"
#include "QGIArrow.h"
#include "QGIPrimPath.h"
#include "Rez.h"
#include "ViewProviderLeader.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"


using namespace TechDrawGui;
using namespace TechDraw;
using DU = DrawUtil;
using DGU = DrawGuiUtil;


//**************************************************************
QGILeaderLine::QGILeaderLine()
    : m_lineColor(Qt::black),
      m_lineStyle(Qt::SolidLine),
      m_hasHover(false),
      m_saveX(0.0),
      m_saveY(0.0),
      m_blockDraw(false)

{
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setCacheMode(QGraphicsItem::NoCache);

    m_line = new QGIPrimPath();
    addToGroup(m_line);
    m_line->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_line->setAcceptHoverEvents(false);
    m_line->setPos(0.0, 0.0);

    m_editPath = new QGEPath();
    addToGroup(m_editPath);
    m_editPath->setPos(0.0, 0.0);
    m_editPath->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_editPath->setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    m_editPath->setZValue(ZVALUE::DIMENSION);
    m_editPath->hide();

    m_arrow1 = new QGIArrow();
    addToGroup(m_arrow1);
    m_arrow1->setPos(0.0, 0.0);
    m_arrow1->hide();
    m_arrow2 = new QGIArrow();
    addToGroup(m_arrow2);
    m_arrow2->setPos(0.0, 0.0);
    m_arrow2->hide();

    setZValue(ZVALUE::DIMENSION);

    QObject::connect(m_editPath,
                     &QGEPath::pointsUpdated,
                     this,
                     &QGILeaderLine::onLineEditFinished);
}

void QGILeaderLine::setLeaderFeature(TechDraw::DrawLeaderLine* feat)
{
    //    Base::Console().Message("QGILL::setLeaderFeature()\n");
    setViewFeature(static_cast<TechDraw::DrawView*>(feat));

    float x = Rez::guiX(feat->X.getValue());
    float y = Rez::guiX(-feat->Y.getValue());
    setPos(x, y);

    setNormalColorAll();
    setPrettyNormal();

    updateView();
}

QVariant QGILeaderLine::itemChange(GraphicsItemChange change, const QVariant& value)
{
    //    Base::Console().Message("QGILL::itemChange(%d)\n", change);
    if (change == ItemSelectedHasChanged && scene()) {
        if (isSelected()) {
            setPrettySel();
        }
        else {
            setPrettyNormal();
        }
        draw();
    }
    else if (change == ItemSceneChange && scene()) {
        // nothing special!
    }
    return QGIView::itemChange(change, value);
}

//QGILL isn't draggable so skip QGIV::mousePress have event
void QGILeaderLine::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mousePressEvent(event);
}

//QGILL isn't draggable so skip QGIV::Release
void QGILeaderLine::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
}

//! start editor on double click
void QGILeaderLine::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    auto ViewProvider = dynamic_cast<ViewProviderLeader*>(getViewProvider(getLeaderFeature()));
    if (!ViewProvider) {
        qWarning() << "QGILeaderLine::mouseDoubleClickEvent: No valid view provider";
        return;
    }

    ViewProvider->startDefaultEditMode();
    QGraphicsItem::mouseDoubleClickEvent(event);
}


void QGILeaderLine::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    //    Base::Console().Message("QGILL::hoverEnter() - selected; %d\n", isSelected());
    m_hasHover = true;
    if (!isSelected()) {
        setPrettyPre();
    }
    QGIView::hoverEnterEvent(event);
}

void QGILeaderLine::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    //    Base::Console().Message("QGILL::hoverLeave() - selected; %d\n", isSelected());
    m_hasHover = false;
    if (!isSelected()) {
        setPrettyNormal();
    }
    QGIView::hoverLeaveEvent(event);
}

void QGILeaderLine::setNormalColorAll()
{
    //    Base::Console().Message("QGILL::setNormalColorAll - normal color: %s\n", qPrintable(getNormalColor().name()));
    QColor qc = prefNormalColor();
    m_line->setNormalColor(qc);
    m_editPath->setNormalColor(qc);
    m_arrow1->setNormalColor(qc);
    m_arrow1->setFillColor(qc);
    m_arrow2->setNormalColor(qc);
    m_arrow2->setFillColor(qc);
}

void QGILeaderLine::setPrettyNormal()
{
    //    Base::Console().Message("QGILL::setPrettyNormal()\n");
    m_line->setPrettyNormal();
    m_arrow1->setPrettyNormal();
    m_arrow2->setPrettyNormal();
}

void QGILeaderLine::setPrettyPre()
{
    //    Base::Console().Message("QGILL::setPrettyPre()\n");
    m_line->setPrettyPre();
    m_arrow1->setPrettyPre();
    m_arrow2->setPrettyPre();
}

void QGILeaderLine::setPrettySel()
{
    //    Base::Console().Message("QGILL::setPrettySel()\n");
    m_line->setPrettySel();
    m_arrow1->setPrettySel();
    m_arrow2->setPrettySel();
}


void QGILeaderLine::closeEdit()
{
    //    Base::Console().Message("QGIL::closeEdit()\n");
    if (m_editPath) {
        m_editPath->onEndEdit();//tell QEPath that edit session ended
    }
}

//! scene coordinates of leaderline points are converted to real page coordinates as deltas from first point.
//! line start point (X,Y of leader feature) is recomputed as displacement from parent's (0,0).
//! signaled from QEPath
void QGILeaderLine::onLineEditFinished(QPointF tipDisplace, std::vector<QPointF> scenePoints)
{
    // Base::Console().Message("QGILL::onLineEditFinished(%s, %d)\n",
    //                        TechDraw::DrawUtil::formatVector(tipDisplace).c_str(),
    //                        scenePoints.size());

    m_blockDraw = true;
    auto featLeader = getLeaderFeature();
    if (!featLeader) {
        return;
    }
    double baseScale = featLeader->getBaseScale();

    if (!(TechDraw::DrawUtil::fpCompare(tipDisplace.x(), 0.0)
          && TechDraw::DrawUtil::fpCompare(tipDisplace.y(), 0.0))) {
        //tip was moved. need to change AttachPoint
        QPointF oldAttach = getAttachFromFeature();
        QPointF newAttach = oldAttach + (tipDisplace / baseScale);
        featLeader->setPosition(Rez::appX(newAttach.x()), Rez::appX(-newAttach.y()), true);
    }

    std::vector<Base::Vector3d> pageDeltas;
    for (auto& pt : scenePoints) {
        QPointF distFromP0 = pt - scenePoints.front();
        // convert deltas to mm
        Base::Vector3d deltaInPageCoords = Rez::appX(DU::toVector3d(distFromP0));
        pageDeltas.push_back(deltaInPageCoords);
    }
    pageDeltas.at(0) = Base::Vector3d(0.0, 0.0, 0.0);

    // to canonical form here
    auto temp = featLeader->makeCanonicalPointsInverted(pageDeltas);
    featLeader->WayPoints.setValues(temp);

    Q_EMIT editComplete();//tell task editing is complete

    m_blockDraw = false;
    m_editPath->hide();
    draw();
}

void QGILeaderLine::startPathEdit()
{
    saveState();
    auto featLeader(dynamic_cast<TechDraw::DrawLeaderLine*>(getViewObject()));
    if (!featLeader) {
        return;
    }

    double scale = featLeader->getScale();
    m_editPath->setScale(scale);
    m_editPath->inEdit(true);
    m_editPath->show();
    // m_editPath->startPathEdit(getWayPointsFromFeature());
    auto vPoints = featLeader->getTransformedWayPoints();
    std::vector<QPointF> qPoints;
    qPoints.reserve(vPoints.size());
    for (auto& point : vPoints) {
        qPoints.emplace_back(Rez::guiX(DU::toQPointF(point)));
    }
    m_editPath->startPathEdit(qPoints);
}

void QGILeaderLine::saveState()
{
    //    Base::Console().Message("QGILL::saveState()\n");
    auto featLeader = getLeaderFeature();
    if (featLeader) {
        m_savePoints = featLeader->WayPoints.getValues();
        m_saveX = featLeader->X.getValue();
        m_saveY = featLeader->Y.getValue();
    }
}

void QGILeaderLine::restoreState()
{
    //    Base::Console().Message("QGILL::restoreState()\n");
    auto featLeader = getLeaderFeature();
    if (featLeader) {
        featLeader->WayPoints.setValues(m_savePoints);
        featLeader->X.setValue(m_saveX);
        featLeader->Y.setValue(m_saveY);
        featLeader->recomputeFeature();
    }
}

//******************************************************************************

void QGILeaderLine::updateView(bool update)
{
    // Base::Console().Message("QGILL::updateView()\n");
    Q_UNUSED(update);
    auto featLeader(dynamic_cast<TechDraw::DrawLeaderLine*>(getViewObject()));
    if (!featLeader) {
        Base::Console().Warning("QGILL::updateView - no feature!\n");
        return;
    }

    auto vp = static_cast<ViewProviderLeader*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    draw();
}

void QGILeaderLine::draw()
{
    if (m_blockDraw) {
        return;
    }
    if (!isVisible()) {
        return;
    }
    TechDraw::DrawLeaderLine* featLeader = getLeaderFeature();
    if (!featLeader) {
        return;
    }
    auto vp = static_cast<ViewProviderLeader*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    TechDraw::DrawView* parent = featLeader->getBaseView();

    if (!parent) {
        return;
    }

    if (m_editPath->inEdit()) {
        return;
    }

    //********
    if (featLeader->isLocked()) {
        setFlag(QGraphicsItem::ItemIsMovable, false);
    }
    else {
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }

    // set the leader's Qt position from feature's X,Y and scale.
    // the feature's x,y is unscaled, unrotated and conventional Y
     QPointF qPoint = DU::toQPointF(getAttachPoint());
    // ???? why does the attach point not need Rez applied?
    setPos(qPoint);

    // line style is standing in for line number here?
    m_lineStyle = static_cast<Qt::PenStyle>(vp->LineStyle.getValue());

    m_line->setFillStyle(Qt::NoBrush);
    m_line->setStyle(m_lineStyle);
    m_line->setWidth(getLineWidth());

    m_line->setPos(0, 0);   //make m_line coords == leader coords

    std::vector<QPointF> qPoints = getWayPointsFromFeature();
    if (qPoints.empty() ) {
        Base::Console().Message("QGILL::draw - no points\n");
        return;
    }

    setNormalColorAll();
    m_line->setPath(makeLeaderPath(qPoints));
    setArrows(qPoints);

    if (isSelected()) {
        setPrettySel();
    }
    else if (m_hasHover) {
        setPrettyPre();
    }
    else {
        setPrettyNormal();
    }
    update(boundingRect());
}

QPainterPath QGILeaderLine::makeLeaderPath(std::vector<QPointF> qPoints)
{
    QPainterPath result;
    DrawLeaderLine* featLeader = getLeaderFeature();
    if (!featLeader) {
        // Base::Console().Message("QGILL::makeLeaderPath - featLeader is nullptr\n");
        return result;
    }

    QPointF startAdjVec(0.0, 0.0);
    double startAdjLength(0.0);
    QPointF endAdjVec(0.0, 0.0);
    double endAdjLength(0.0);
    if (qPoints.size() > 1) {
        //make path adjustment to hide leaderline ends behind arrowheads
        if (featLeader->StartSymbol.getValue() != ArrowType::NONE) {
            startAdjLength = QGIArrow::getOverlapAdjust(featLeader->StartSymbol.getValue(),
                                                        QGIArrow::getPrefArrowSize());
        }
        if (featLeader->EndSymbol.getValue() != ArrowType::NONE) {
            endAdjLength = QGIArrow::getOverlapAdjust(featLeader->EndSymbol.getValue(),
                                                      QGIArrow::getPrefArrowSize());
        }

        //get adjustment directions
        startAdjVec = qPoints.at(1) - qPoints.front();
        endAdjVec = (*(qPoints.end() - 2)) - qPoints.back();

        //get adjustment vectors
        QVector2D startTemp(startAdjVec);
        QVector2D endTemp(endAdjVec);
        startTemp.normalize();
        endTemp.normalize();
        startAdjVec = startTemp.toPointF() * startAdjLength;
        endAdjVec = endTemp.toPointF() * endAdjLength;

        qPoints.front() += startAdjVec;
        qPoints.back() += endAdjVec;
        result.moveTo(qPoints.front());
        for (int i = 1; i < (int)qPoints.size(); i++) {
            result.lineTo(qPoints.at(i));
        }
    }
    return result;
}

//! returns the point (on the parent) to which the leader is attached.
//! result is relative to the center of the unscaled, unrotated parent.
//! result is is not inverted (Y grows upwards).
QPointF QGILeaderLine::getAttachFromFeature()
{
    TechDraw::DrawLeaderLine* featLeader = getLeaderFeature();
    if (!featLeader) {
        // Base::Console().Message("QGIL::getAttachFromLeader - no feature\n");
        return {};
    }
    double x = Rez::guiX(featLeader->X.getValue());
    double y = -Rez::guiX(featLeader->Y.getValue());
    // this is unrotated point.  is it scaled?
    return {x, y};
}

std::vector<QPointF> QGILeaderLine::getWayPointsFromFeature()
{
    DrawLeaderLine* featLeader = getLeaderFeature();
    if (!featLeader) {
        // Base::Console().Message("QGILL::getWayPointsFromFeature - featLeader is nullptr\n");
        return {};
    }

    // vPoints are in mm with conventional Y axis
    auto doScale = featLeader->Scalable.getValue();
    auto doRotate = featLeader->RotatesWithParent.getValue();
    auto vPoints =  featLeader->getScaledAndRotatedPoints(doScale, doRotate);
    if (featLeader->AutoHorizontal.getValue()) {
        vPoints = DrawLeaderLine::horizLastSegment(vPoints, featLeader->getBaseView()->Rotation.getValue());
    }

    std::vector<QPointF> qPoints;
    qPoints.reserve(vPoints.size());
    // now convert to sceneUnits and Qt Y axis
    for (auto& entry : vPoints) {
        if (useOldCoords()) {
            // use points as saved in <= v0.21 (in Qt form, Rez'd and
            qPoints.push_back(DU::toQPointF(entry));
        } else {
            // use points as saved in >= v0.22
            qPoints.push_back(DU::toQPointF(Rez::guiX(entry)));
        }
    }

    if (qPoints.empty()) {
        Base::Console().Warning("QGILeaderLine::getWayPointsFromFeature - no points\n");
    }
    return qPoints;
}

void QGILeaderLine::setArrows(std::vector<QPointF> pathPoints)
{
    Base::Vector3d stdX(1.0, 0.0, 0.0);
    TechDraw::DrawLeaderLine* featLeader = getLeaderFeature();

    QPointF lastOffset = (pathPoints.back() - pathPoints.front());

    if (featLeader->StartSymbol.getValue() != ArrowType::NONE) {
        m_arrow1->setStyle(featLeader->StartSymbol.getValue());
        m_arrow1->setWidth(getLineWidth());
        m_arrow1->setSize(QGIArrow::getPrefArrowSize());
        m_arrow1->setDirMode(true);
        m_arrow1->setDirection(stdX);
        if (pathPoints.size() > 1) {
            auto it = pathPoints.begin();
            QPointF s = (*it);
            QPointF e = (*(it + 1));
            QPointF qsVec = s - e;
            Base::Vector3d sVec(qsVec.x(), qsVec.y(), 0.0);
            m_arrow1->setDirection(sVec);
            m_arrow1->setPos(0.0, 0.0);
        }
        m_arrow1->draw();
        m_arrow1->show();
    }
    else {
        m_arrow1->hide();
    }

    if (featLeader->EndSymbol.getValue() != ArrowType::NONE) {
        m_arrow2->setStyle(featLeader->EndSymbol.getValue());
        m_arrow2->setWidth(getLineWidth());
        m_arrow2->setSize(QGIArrow::getPrefArrowSize());
        m_arrow2->setDirMode(true);
        m_arrow2->setDirection(-stdX);
        if (pathPoints.size() > 1) {
            auto itr = pathPoints.rbegin();
            QPointF s = (*itr);
            QPointF e = (*(itr + 1));
            QPointF qeVec = s - e;
            Base::Vector3d eVec(qeVec.x(), qeVec.y(), 0.0);
            m_arrow2->setDirection(eVec);
            m_arrow2->setPos(lastOffset);
        }
        m_arrow2->draw();
        m_arrow2->show();
    }
    else {
        m_arrow2->hide();
    }
}

void QGILeaderLine::drawBorder()
{
    ////Leaders have no border!
    //    QGIView::drawBorder();   //good for debugging
}


//! return the position of the tip of the leader's arrow
Base::Vector3d  QGILeaderLine::getAttachPoint()
{
    TechDraw::DrawLeaderLine* featLeader = getLeaderFeature();
    if (!featLeader) {
        return Base::Vector3d(0, 0, 0);
    }
        TechDraw::DrawView* parent = featLeader->getBaseView();

    if (!parent) {
        return Base::Vector3d(0, 0, 0);
    }

    double baseScale = featLeader->getBaseScale();
    double xPos = Rez::guiX(featLeader->X.getValue());
    double yPos = Rez::guiX(featLeader->Y.getValue());
    Base::Vector3d vAttachPoint{xPos, yPos};
    vAttachPoint = vAttachPoint * baseScale;
    double rotationRad = parent->Rotation.getValue() * M_PI / DegreesHalfCircle;
    if (rotationRad != 0.0) {
        vAttachPoint.RotateZ(rotationRad);
    }
    vAttachPoint = DU::invertY(vAttachPoint);
    return vAttachPoint;
}

//******************************************************************************


void QGILeaderLine::abandonEdit()
{
    //    Base::Console().Message("QGIL::abandonEdit()\n");
    m_editPath->clearMarkers();
    m_editPath->hide();
    restoreState();
}

double QGILeaderLine::getLineWidth()
{
    auto vp = static_cast<ViewProviderLeader*>(getViewProvider(getViewObject()));
    if (!vp) {
        return Rez::guiX(LineGroup::getDefaultWidth("Graphic"));
    }
    return Rez::guiX(vp->LineWidth.getValue());
}

TechDraw::DrawLeaderLine* QGILeaderLine::getLeaderFeature()
{
    return static_cast<TechDraw::DrawLeaderLine*>(getViewObject());
}

double QGILeaderLine::getEdgeFuzz() const
{
    return PreferencesGui::edgeFuzz();
}

QColor QGILeaderLine::prefNormalColor()
{
    //    Base::Console().Message("QGILL::getNormalColor()\n");
    setNormalColor(PreferencesGui::leaderQColor());

    auto vp = dynamic_cast<ViewProviderLeader*>(getViewProvider(getViewObject()));
    if (vp) {
        QColor normal = vp->Color.getValue().asValue<QColor>();
        setNormalColor(PreferencesGui::getAccessibleQColor(normal));
    }
    return getNormalColor();
}

QRectF QGILeaderLine::boundingRect() const
{
    return childrenBoundingRect();
}

void QGILeaderLine::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                          QWidget* widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    // painter->setPen(Qt::blue);
    // painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint(painter, &myOption, widget);
}

bool QGILeaderLine::useOldCoords() const
{
    auto vp = dynamic_cast<ViewProviderLeader*>(getViewProvider(getViewObject()));
    if (vp) {
        return vp->UseOldCoords.getValue();
    }
    return false;
}


#include <Mod/TechDraw/Gui/moc_QGILeaderLine.cpp>
