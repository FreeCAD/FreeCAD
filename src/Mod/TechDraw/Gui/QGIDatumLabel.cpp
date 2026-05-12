/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <QApplication>
#include <QGraphicsSceneMouseEvent>

#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Mod/TechDraw/App/DimensionFormatter.h>
#include <Mod/TechDraw/App/DimensionGeometry.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "PreferencesGui.h"
#include "QGCustomText.h"
#include "QGIDatumLabel.h"
#include "QGIViewDimension.h"
#include "TaskSelectLineAttributes.h"
#include "ViewProviderDimension.h"

#include <Base/Console.h>


#define NORMAL 0
#define PRE 1
#define SEL 2

using namespace TechDraw;
using Format = DimensionFormatter::Format;

namespace TechDrawGui {

QGIDatumLabel::QGIDatumLabel() : m_dragState(DragState::NoDrag)
{
    verticalSep = false;

    parent = nullptr;

    setCacheMode(QGraphicsItem::NoCache);
    setFlag(ItemSendsGeometryChanges, true);
    setFlag(ItemIsMovable, true);
    setSelectability(true);
    setFiltersChildEvents(true);

    m_textItems = new QGraphicsItemGroup();
    m_textItems->setParentItem(this);
    m_dimText = new QGCustomText();
    m_dimText->setTightBounding(true);
    m_dimText->setParentItem(m_textItems);
    m_tolTextOver = new QGCustomText();
    m_tolTextOver->setTightBounding(true);
    m_tolTextOver->setParentItem(m_textItems);
    m_tolTextUnder = new QGCustomText();
    m_tolTextUnder->setTightBounding(true);
    m_tolTextUnder->setParentItem(m_textItems);
    m_unitText = new QGCustomText();
    m_unitText->setTightBounding(true);
    m_unitText->setParentItem(m_textItems);

    m_frame = new QGraphicsRectItem();
    QPen framePen;
    framePen.setWidthF(Rez::guiX(0.5));
    framePen.setColor(m_dimText->defaultTextColor());
    framePen.setJoinStyle(Qt::MiterJoin);
    m_frame->setPen(framePen);

    m_ctrl = false;
}

void QGIDatumLabel::setFramed(bool framed)
{
    if(framed) {
        m_frame->setVisible(true);
        m_frame->setParentItem(this);
    }
    else {
        m_frame->setVisible(false);
        m_frame->setParentItem(nullptr);
    }
}

QVariant QGIDatumLabel::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if (isSelected()) {
            setPrettySel();
        }
        else {
            setPrettyNormal();
            if (m_dragState == DragState::Dragging) {
                //stop the drag if we are no longer selected.
                m_dragState = DragState::NoDrag;
                Q_EMIT dragFinished();
            }
        }
    }
    else if (change == ItemPositionHasChanged && scene()) {
        if (!(QApplication::keyboardModifiers() & Qt::AltModifier)) {
            QPointF newPos = value.toPointF();    //position within parent!
            if (!m_inhibitSnapOnPosChange) {
                // we don't want to snap if snap caused this position change
                snapPosition(newPos);
            }
            m_inhibitSnapOnPosChange = false;
        }

        m_dragState = DragState::Dragging;
        Q_EMIT dragging(m_ctrl);
    }

    return QGraphicsItem::itemChange(change, value);
}

void QGIDatumLabel::snapPosition(QPointF& pos)
{
    if (!Preferences::SnapViews()) {
        return;
    }

    using std::numbers::pi;


    auto* qgivd = dynamic_cast<QGIViewDimension*>(parentItem());
    if (!qgivd) {
        return;
    }
    auto* dim(dynamic_cast<TechDraw::DrawViewDimension*>(qgivd->getViewObject()));
    if (!dim) {
        return;
    }

    // We have snap for:
    // distances constraints
    // Angles
    // Radius & Diameter
    std::string type = dim->Type.getValueAsString();
    bool shouldSnap = false;
    if (type == "Distance" || type == "DistanceX" || type == "DistanceY") {
        snapDistanceType(pos, dim, qgivd, shouldSnap);
    }
    else if (type == "Radius" || type == "Diameter") {
        snapRadialType(pos, dim, qgivd, shouldSnap);
    } 
    else if (type == "Angle" || type == "Angle3Pt") {
        snapAngleType(pos, dim, qgivd, shouldSnap);
    }
    else {
        return;
    }

    if (!shouldSnap) {
        return;
    }

    // block itemChange from calling snapPosition again
    m_inhibitSnapOnPosChange = true;
    setPos(pos);
}

void QGIDatumLabel::snapDistanceType(QPointF& pos, TechDraw::DrawViewDimension* dim,
                        QGIViewDimension* qgivd, bool& shouldSnap)
{
    qreal snapPercent = 0.4;
    double dimSpacing = Rez::guiX(activeDimAttributes.getCascadeSpacing());
    std::string type = dim->Type.getValueAsString();

    // We try to snap the label to its center position.
    pointPair pp = dim->getLinearPoints();
    if (pp.first().IsEqual(pp.second(), EWTOLERANCE)) {
        // broken dim guard: overlapping points
        return;
    }
    Base::Vector3d p1_3d = Rez::guiX(pp.first());
    Base::Vector3d p2_3d = Rez::guiX(pp.second());
    Base::Vector2d p1 = Base::Vector2d(p1_3d.x, p1_3d.y);
    Base::Vector2d p2 = Base::Vector2d(p2_3d.x, p2_3d.y);
    if (type == "DistanceX") {
        p2 = Base::Vector2d(p2.x, p1.y);
    }
    else if (type == "DistanceY") {
        p2 = Base::Vector2d(p1.x, p2.y);
    }
    Base::Vector2d mid = (p1 + p2) * 0.5;
    Base::Vector2d dir = p2 - p1;
    Base::Vector2d normal = Base::Vector2d(-dir.y, dir.x);

    Base::Vector2d toCenter = getPosToCenterVec();

    Base::Vector2d posV = Base::Vector2d(pos.x(), pos.y()) + toCenter;

    Base::Vector2d projPnt;
    projPnt.ProjectToLine(posV - mid, normal);
    projPnt = projPnt + mid;

    if ((projPnt - posV).Length() < dimSpacing * snapTextPercent) {
        posV = projPnt;
        pos.setX(posV.x - toCenter.x);
        pos.setY(posV.y - toCenter.y);
        shouldSnap = true;
    }

    // We check for coord/chain dimensions to offer proper snapping
    double snapChainPercent = Preferences::SnapDimensionsChainFactor();
    auto* qgiv = dynamic_cast<QGIView*>(qgivd->parentItem());
    if (qgiv) {
        auto* dvp = dynamic_cast<TechDraw::DrawViewPart*>(qgiv->getViewObject());
        if (dvp) {
            std::vector<TechDraw::DrawViewDimension*> dims = dvp->getDimensions();
            for (auto& d : dims) {
                if (d == dim) { continue; }

                std::string typei = d->Type.getValueAsString();
                if (typei != "Distance" && typei != "DistanceX" && typei != "DistanceY") {
                    continue;
                }

                pp = d->getLinearPoints();
                if (pp.first().IsEqual(pp.second(), EWTOLERANCE)) {
                    // broken dim guard: overlapping points
                    continue;
                }
                Base::Vector3d ip1_3d = Rez::guiX(pp.first());
                Base::Vector3d ip2_3d = Rez::guiX(pp.second());

                Base::Vector2d ip1 = Base::Vector2d(ip1_3d.x, ip1_3d.y);
                Base::Vector2d ip2 = Base::Vector2d(ip2_3d.x, ip2_3d.y);
                if (typei == "DistanceX") {
                    ip2 = Base::Vector2d(ip2.x, ip1.y);
                }
                else if (typei == "DistanceY") {
                    ip2 = Base::Vector2d(ip1.x, ip2.y);
                }

                Base::Vector2d idir = ip2 - ip1;

                if (fabs(dir.x * idir.y - dir.y * idir.x) > Precision::Confusion()) {
                    // dimensions not parallel
                    continue;
                }

                auto* vp = freecad_cast<ViewProviderDimension*>(
                    Gui::Application::Instance->getViewProvider(d)
                );
                if (!vp) {
                    continue;
                }
                auto* qgivDi(dynamic_cast<QGIViewDimension*>(vp->getQView()));
                if (!qgivDi) {
                    continue;
                }
                auto labeli = qgivDi->getDatumLabel();
                if (!labeli) {
                    continue;
                }
                QPointF posi = labeli->pos();
                Base::Vector2d toCenteri = labeli->getPosToCenterVec();
                Base::Vector2d posVi = Base::Vector2d(posi.x(), posi.y()) + toCenteri;

                Base::Vector2d projPnt2;
                projPnt2.ProjectToLine(posV - posVi, idir);
                projPnt2 = projPnt2 + posVi;

                if ((projPnt2 - posV).Length() < dimSpacing * snapPercent) {
                    posV = projPnt2;
                    pos.setX(posV.x - toCenter.x);
                    pos.setY(posV.y - toCenter.y);
                    shouldSnap = true;
                    break;
                }
                else if (fabs((projPnt2 - posV).Length() - fabs(dimSpacing))
                         < dimSpacing * snapPercent) {
                    posV = projPnt2 + (posV - projPnt2).Normalize() * dimSpacing;
                    pos.setX(posV.x - toCenter.x);
                    pos.setY(posV.y - toCenter.y);
                    shouldSnap = true;
                    break;
                }
            }
        }
    }

}

void QGIDatumLabel::snapRadialType(QPointF& pos, TechDraw::DrawViewDimension* dim,
                        QGIViewDimension* qgivd, bool& shouldSnap)
{
    qreal snapPercent = 0.4;
    double dimSpacing = Rez::guiX(activeDimAttributes.getCascadeSpacing());
    std::string type = dim->Type.getValueAsString();

    arcPoints arc = dim->getArcPoints();
    if (arc.radius < Precision::Confusion()) {
        // broken dim guard: zero radius
        return;
    }
    Base::Vector2d rotationCenter = to2D(Rez::guiX(arc.center));
    double radius = Rez::guiX(arc.radius);

    Base::Vector2d toCenter = getPosToCenterVec();
    Base::Vector2d labelCenter = Base::Vector2d(pos.x(), pos.y()) + toCenter;

    Base::Vector2d radialDir
        = Base::Vector2d::FromPolar(radius, Base::toRadians(this->rotation())).Normalize();
    auto cachedAngle = qgivd->getCachedDiameterLineAngle();
    if (!cachedAngle.has_value()) {
        return;
    }
    else {
        radialDir = Base::Vector2d::FromPolar(1.0, -cachedAngle.value());
    }
    Base::Vector2d normal = radialDir.Perpendicular(true);

    Base::Vector2d mid = rotationCenter;  // for type == "Diameter"
    if (type == "Radius") {
        mid += (radius / 2.0) * radialDir;
    }

    Base::Vector2d toMid = labelCenter - mid;
    Base::Vector2d projPnt;
    projPnt.ProjectToLine(toMid, normal);
    double trigDist = (toMid - projPnt).Length();

    if (trigDist < dimSpacing * snapPercent && toMid.Length() < dimSpacing) {
        double perpDist = toMid.x * normal.x + toMid.y * normal.y;
        double lineLabelDistance = tightBoundingRect().height() * 0.5
            + Rez::guiX(qgivd->getIsoDimensionLineSpacing());

        if (std::abs(perpDist) > lineLabelDistance * 0.5) {  // ISO Style
            if (!m_snappedNormalOffset.has_value()) {
                m_snappedNormalOffset = perpDist >= 0 ? lineLabelDistance : -lineLabelDistance;
            }
            labelCenter = mid + m_snappedNormalOffset.value() * normal;
        }
        else {  // ASME Style
            labelCenter = mid;
        }

        pos.setX(labelCenter.x - toCenter.x);
        pos.setY(labelCenter.y - toCenter.y);
        shouldSnap = true;
    }
    else {
        m_snappedNormalOffset.reset();
    }

}

void QGIDatumLabel::snapAngleType(QPointF& pos, TechDraw::DrawViewDimension* dim,
                        QGIViewDimension* qgivd, bool& shouldSnap)
{
    using std::numbers::pi;
    qreal snapPercent = 0.4;  // <- copied from original implementation
    double dimSpacing = Rez::guiX(activeDimAttributes.getCascadeSpacing());
    std::string type = dim->Type.getValueAsString();

    anglePoints ap = dim->getAnglePoints();
    bool isInverted = dim->Inverted.getValue();

    if (ap.first().IsEqual(ap.vertex(), EWTOLERANCE) || ap.second().IsEqual(ap.vertex(), EWTOLERANCE)
        || ap.first().IsEqual(ap.second(), EWTOLERANCE)) {
        // broken dim guard: any two (of the three) points overlap
        return;
    }

    // central vertex for angle
    Base::Vector2d angleVertex = to2D(Rez::guiX(ap.vertex()));

    // get normal direction for angle arc
    Base::Vector2d firstDimPoint = to2D(Rez::guiX(ap.first()));
    Base::Vector2d secondDimPoint = to2D(Rez::guiX(ap.second()));

    Base::Vector2d dir1 = (secondDimPoint - angleVertex).Normalize();
    Base::Vector2d dir2 = (firstDimPoint - angleVertex).Normalize();

    double endAngle = dir1.Angle();
    double startAngle = dir2.Angle();
    if (isInverted) {
        std::swap(endAngle, startAngle);
    }

    Base::Vector2d normalDir = (dir1 - dir2).Perpendicular(isInverted).Normalize();

    // get label position & direction
    Base::Vector2d toCenter = getPosToCenterVec();
    Base::Vector2d labelCenter = Base::Vector2d(pos.x(), pos.y()) + toCenter;
    Base::Vector2d labelRadialDir(labelCenter - angleVertex);
    double selfLen = labelRadialDir.Length();

    // find mid-point & check distance from label
    Base ::Vector2d mid = angleVertex + Base::Vector2d::FromPolar(selfLen, normalDir.Angle());

    Base::Vector2d projPnt;
    projPnt.ProjectToLine(labelCenter - mid, normalDir);
    projPnt = projPnt + mid;

    double centerDist = (projPnt - labelCenter).Length();
    bool shouldSnap2Center = centerDist < dimSpacing * snapPercent;
    Base::Vector2d centerSnapPnt = projPnt;

    // Neighbouring Angles Snap

    bool shouldSnap2Nbr = false;
    Base::Vector2d nbrSnapPnt = labelCenter;
    double nbrSnapDist = std::numeric_limits<double>::max();

    // rearrange the arc angles for comparison
    if (startAngle > endAngle) {
        std::swap(startAngle, endAngle);
    }
    if ((endAngle - startAngle) > pi) {
        std::swap(startAngle, endAngle);
    }
    if (isInverted) {
        std::swap(startAngle, endAngle);
    }

    auto* qgiv = dynamic_cast<QGIView*>(qgivd->parentItem());
    if (qgiv) {
        auto* dvp = dynamic_cast<TechDraw::DrawViewPart*>(qgiv->getViewObject());
        if (dvp) {
            snapPercent = 0.2;  // <- copied from original implementation

            for (auto& d : dvp->getDimensions()) {
                const std::string nbrType = d->Type.getValueAsString();
                if (d == dim || (nbrType != "Angle" && nbrType != "Angle3Pt")) {
                    continue;
                }

                // get neighbour angle points
                anglePoints nbrAP = d->getAnglePoints();
                bool inv = d->Inverted.getValue();

                if (nbrAP.first().IsEqual(nbrAP.vertex(), EWTOLERANCE)
                    || nbrAP.second().IsEqual(nbrAP.vertex(), EWTOLERANCE)
                    || nbrAP.first().IsEqual(nbrAP.second(), EWTOLERANCE)) {
                    // broken dim guard: any two (of the three) points overlap
                    continue;
                }

                // check for a common origin
                Base::Vector2d nbrAngleVertex = to2D(Rez::guiX(nbrAP.vertex()));

                // neighbour end & start angles
                Base::Vector2d nbrFirDimPoint = to2D(Rez::guiX(nbrAP.first()));
                Base::Vector2d nbrSecDimPoint = to2D(Rez::guiX(nbrAP.second()));

                double nbrEndAngle = (nbrSecDimPoint - nbrAngleVertex).Angle();
                double nbrStartAngle = (nbrFirDimPoint - nbrAngleVertex).Angle();

                auto* vp = freecad_cast<ViewProviderDimension*>(
                    Gui::Application::Instance->getViewProvider(d)
                );
                if (!vp) {
                    continue;
                }

                auto* qgivDi(dynamic_cast<QGIViewDimension*>(vp->getQView()));
                if (!qgivDi) {
                    continue;
                }

                auto labeli = qgivDi->getDatumLabel();
                if (!labeli) {
                    continue;
                }

                // Cached values from QGIViewDimension
                const double rNbr = Rez::guiX(qgivDi->getCachedAngleArcRadius().value_or(0.0));
                const double offsetSelf = Rez::guiX(
                    qgivd->getCachedAngleLabelArcOffset().value_or(0.0)
                );
                double rSelf = selfLen - offsetSelf;

                if (rNbr <= 0.0 || rSelf <= 0.0) {
                    continue;
                }

                // rearrange the arc angles for comparison
                if (nbrStartAngle > nbrEndAngle) {
                    std::swap(nbrStartAngle, nbrEndAngle);
                }
                if ((nbrEndAngle - nbrStartAngle) > pi) {
                    std::swap(nbrStartAngle, nbrEndAngle);
                }
                if (inv) {
                    std::swap(nbrStartAngle, nbrEndAngle);
                }

                bool nbrEdge = std::fabs(nbrEndAngle - startAngle) < Precision::Angular()
                    || std::fabs(nbrStartAngle - endAngle) < Precision::Angular();
                bool cascadeEdge = std::fabs(nbrStartAngle - startAngle) < Precision::Angular()
                    || std::fabs(nbrEndAngle - endAngle) < Precision::Angular();
                bool shouldAlign = nbrEdge && std::fabs(rNbr - rSelf) < dimSpacing * snapPercent;
                bool shouldCascade = cascadeEdge
                    && std::fabs(std::fabs(rNbr - rSelf) - 2 * dimSpacing) < dimSpacing * snapPercent;
                if (!shouldAlign && !shouldCascade) {
                    continue;
                }

                double targetLen = shouldAlign
                    ? rNbr + offsetSelf
                    : rNbr + std::copysign(2 * dimSpacing, selfLen - rNbr) + offsetSelf;
                if (targetLen <= Precision::Confusion()) {
                    continue;
                }
                Base::Vector2d candidate = angleVertex + (targetLen)*labelRadialDir.Normalize();
                double distToCandidate = (candidate - labelCenter).Length();
                if (distToCandidate < nbrSnapDist) {
                    nbrSnapPnt = candidate;
                    nbrSnapDist = distToCandidate;
                    shouldSnap2Nbr = true;
                }
            }
        }
    }

    Base::Vector2d finalCenter = labelCenter;

    if (shouldSnap2Center) {
        shouldSnap = true;
        finalCenter = centerSnapPnt;
    }

    if (shouldSnap2Nbr) {
        shouldSnap = true;
        Base::Vector2d dir = finalCenter - angleVertex;
        double len = dir.Length();
        if (len > Precision::Confusion()) {
            double targetLen = (nbrSnapPnt - angleVertex).Length();
            finalCenter = angleVertex + targetLen * (dir / len);
        }
    }

    if (shouldSnap) {
        pos.setX(finalCenter.x - toCenter.x);
        pos.setY(finalCenter.y - toCenter.y);
    }
}

void QGIDatumLabel::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        m_ctrl = true;
    }

    QGraphicsItem::mousePressEvent(event);
}

void QGIDatumLabel::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    //    Base::Console().message("QGIDL::mouseReleaseEvent()\n");
    m_ctrl = false;
    if (m_dragState == DragState::Dragging) {
        m_dragState = DragState::NoDrag;
        Q_EMIT dragFinished();
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIDatumLabel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    QGIViewDimension* qgivDimension = dynamic_cast<QGIViewDimension*>(parentItem());
    if (!qgivDimension) {
        qWarning() << "QGIDatumLabel::mouseDoubleClickEvent: No parent item";
        return;
    }

    auto ViewProvider = freecad_cast<ViewProviderDimension*>(
        qgivDimension->getViewProvider(qgivDimension->getViewObject()));
    if (!ViewProvider) {
        qWarning() << "QGIDatumLabel::mouseDoubleClickEvent: No valid view provider";
        return;
    }

    ViewProvider->startDefaultEditMode();
    QGraphicsItem::mouseDoubleClickEvent(event);
}

void QGIDatumLabel::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_EMIT hover(true);
    if (!isSelected()) {
        setPrettyPre();
    }
    else {
        setPrettySel();
    }
    QGraphicsItem::hoverEnterEvent(event);
}

void QGIDatumLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_EMIT hover(false);
    if (!isSelected()) {
        setPrettyNormal();
    }
    else {
        setPrettySel();
    }

    QGraphicsItem::hoverLeaveEvent(event);
}

QRectF QGIDatumLabel::boundingRect() const
{
    return childrenBoundingRect();
}

QRectF QGIDatumLabel::tightBoundingRect() const
{
    QRectF totalRect;
    for (QGraphicsItem* item : m_textItems->childItems()) {
        auto* customText = dynamic_cast<QGCustomText*>(item);
        if (customText && !customText->toPlainText().isEmpty()) {
            QRectF itemRect = customText->alignmentRect();
            QPointF pos = customText->pos();
            itemRect.translate(pos.x(), pos.y());
            totalRect = totalRect.isNull() ? itemRect : totalRect.united(itemRect);
        }
    }
    int fontSize = m_dimText->font().pixelSize();
    int paddingLeft = fontSize * 0.2;
    int paddingTop = fontSize * 0.1;
    int paddingRight = fontSize * 0.2;
    int paddingBottom = fontSize * 0.1;
    return totalRect.adjusted(-paddingLeft, -paddingTop, paddingRight, paddingBottom);
}

void QGIDatumLabel::updateFrameRect() {
    prepareGeometryChange();
    m_frame->setRect(tightBoundingRect());
}

void QGIDatumLabel::boundingRectChanged()
{
    setTransformOriginPoint(tightBoundingRect().center());
}

void QGIDatumLabel::setLineWidth(double lineWidth)
{
    QPen pen = m_frame->pen();
    pen.setWidthF(lineWidth);
    m_frame->setPen(pen);
}

void QGIDatumLabel::setFrameColor(QColor color)
{
    QPen pen = m_frame->pen();
    pen.setColor(color);
    m_frame->setPen(pen);
}

void QGIDatumLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                          QWidget* widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(painter);
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //    painter->setPen(Qt::blue);
    //    painter->drawRect(boundingRect());          //good for debugging
}

void QGIDatumLabel::setPosFromCenter(const double& xCenter, const double& yCenter)
{
    //set label's Qt position(top, left) given boundingRect center point
    Base::Vector2d centerOffset = getPosToCenterVec();
    double xTopLeft = xCenter - centerOffset.x;
    double yTopLeft = yCenter - centerOffset.y;
    setPos(xTopLeft, yTopLeft);
    updateChildren();
}

void QGIDatumLabel::updateChildren()
{
    prepareGeometryChange();
    QString uText = m_unitText->toPlainText();
    if ((uText.size() > 0) && (uText.at(0) != QChar::fromLatin1(' '))) {
        QString vText = m_dimText->toPlainText();
        vText = vText + uText;
        m_dimText->setPlainText(vText);
        m_unitText->setPlainText(QString());
    }

    QRectF labelBox = m_dimText->alignmentRect();
    double right = labelBox.right();
    double middle = labelBox.center().y();

    //set unit position
    QRectF unitBox = m_unitText->alignmentRect();
    double unitWidth = unitBox.width();
    double unitRight = right + unitWidth;
    // Set the m_unitText font *baseline* at same height as the m_dimText font baseline
    m_unitText->setPos(right, 0.0);

    //set tolerance position
    QRectF overBox = m_tolTextOver->alignmentRect();
    double tolLeft  = unitRight;

    // Adjust for difference in tight and original bounding box sizes, note the y-coord down system
    QPointF tol_adj = m_tolTextOver->tightBoundingAdjust();
    m_tolTextOver->justifyLeftAt(tolLeft + tol_adj.x(), middle + tol_adj.y()/2.0, false);
    tol_adj = m_tolTextUnder->tightBoundingAdjust();
    m_tolTextUnder->justifyLeftAt(tolLeft + tol_adj.x(), middle + overBox.height() + tol_adj.y()/2.0, false);
}

Base::Vector2d QGIDatumLabel::getPosToCenterVec() const
{
    QPointF center = tightBoundingRect().center();
    return Base::Vector2d(center.x(), center.y());
}

void QGIDatumLabel::setFont(QFont font)
{
    prepareGeometryChange();
    m_dimText->setFont(font);
    m_unitText->setFont(font);
    QFont tFont(font);
    double fontSize = font.pixelSize();
    double tolAdj = getTolAdjust();
    tFont.setPixelSize(std::max(1, static_cast<int>(fontSize * tolAdj)));
    m_tolTextOver->setFont(tFont);
    m_tolTextUnder->setFont(tFont);
    updateFrameRect();
    boundingRectChanged();
}

void QGIDatumLabel::setDimString(QString text, qreal maxWidth)
{
    prepareGeometryChange();
    m_dimText->setPlainText(text);
    m_dimText->setTextWidth(maxWidth);
    updateFrameRect();
    boundingRectChanged();
}

void QGIDatumLabel::setToleranceString()
{
    prepareGeometryChange();
    QGIViewDimension* qgivd = dynamic_cast<QGIViewDimension*>(parentItem());
    if (!qgivd) {
        return;
    }
    const auto dim(dynamic_cast<TechDraw::DrawViewDimension*>(qgivd->getViewObject()));
    if (!dim) {
        return;
        // don't show if both are zero or if EqualTolerance is true
    }
    else if (!dim->hasOverUnderTolerance() || dim->EqualTolerance.getValue()
             || dim->TheoreticalExact.getValue()) {
        m_tolTextOver->hide();
        m_tolTextUnder->hide();
        // we must explicitly empty the text otherwise the frame drawn for
        // TheoreticalExact would be as wide as necessary for the text
        m_tolTextOver->setPlainText(QString());
        m_tolTextUnder->setPlainText(QString());
        updateFrameRect();
        boundingRectChanged();
        return;
    }

    std::pair<std::string, std::string> labelTexts, unitTexts;

    if (dim->ArbitraryTolerances.getValue()) {
        labelTexts = dim->getFormattedToleranceValues(Format::FORMATTED);//copy tolerance spec
        unitTexts.first = "";
        unitTexts.second = "";
    }
    else {
        if (dim->isMultiValueSchema()) {
            labelTexts = dim->getFormattedToleranceValues(Format::UNALTERED);//don't format multis
            unitTexts.first = "";
            unitTexts.second = "";
        }
        else {
            labelTexts = dim->getFormattedToleranceValues(Format::FORMATTED);// prefix value [unit] postfix
            unitTexts = dim->getFormattedToleranceValues(Format::UNIT); //just the unit
        }
    }

    if (labelTexts.first.empty()) {
        m_tolTextUnder->hide();
    }
    else {
        m_tolTextUnder->setPlainText(QString::fromUtf8(labelTexts.first.c_str()));
        m_tolTextUnder->show();
    }
    if (labelTexts.second.empty()) {
        m_tolTextOver->hide();
    }
    else {
        m_tolTextOver->setPlainText(QString::fromUtf8(labelTexts.second.c_str()));
        m_tolTextOver->show();
    }

    updateFrameRect();
    boundingRectChanged();
}


int QGIDatumLabel::getPrecision()
{
    if (Preferences::useGlobalDecimals()) {
        return Base::UnitsApi::getDecimals();
    }
    return Preferences::getPreferenceGroup("Dimensions")->GetInt("AltDecimals", 2);
}

double QGIDatumLabel::getTolAdjust()
{
    return Preferences::getPreferenceGroup("Dimensions")->GetFloat("TolSizeAdjust", 0.50);
}


void QGIDatumLabel::setPrettySel()
{
    //    Base::Console().message("QGIDL::setPrettySel()\n");
    m_dimText->setPrettySel();
    m_tolTextOver->setPrettySel();
    m_tolTextUnder->setPrettySel();
    m_unitText->setPrettySel();
    setFrameColor(PreferencesGui::selectQColor());
    Q_EMIT setPretty(SEL);
}

void QGIDatumLabel::setPrettyPre()
{
    //    Base::Console().message("QGIDL::setPrettyPre()\n");
    m_dimText->setPrettyPre();
    m_tolTextOver->setPrettyPre();
    m_tolTextUnder->setPrettyPre();
    m_unitText->setPrettyPre();
    setFrameColor(PreferencesGui::preselectQColor());
    Q_EMIT setPretty(PRE);
}

void QGIDatumLabel::setPrettyNormal()
{
    //    Base::Console().message("QGIDL::setPrettyNormal()\n");
    m_dimText->setPrettyNormal();
    m_tolTextOver->setPrettyNormal();
    m_tolTextUnder->setPrettyNormal();
    m_unitText->setPrettyNormal();
    setFrameColor(m_colNormal);
    Q_EMIT setPretty(NORMAL);
}

void QGIDatumLabel::setColor(QColor color)
{
    //    Base::Console().message("QGIDL::setColor(%s)\n", qPrintable(c.name()));
    m_colNormal = color;
    m_dimText->setColor(m_colNormal);
    m_tolTextOver->setColor(m_colNormal);
    m_tolTextUnder->setColor(m_colNormal);
    m_unitText->setColor(m_colNormal);
    setFrameColor(m_colNormal);
}

void QGIDatumLabel::setSelectability(bool val)
{
    setFlag(ItemIsSelectable, val);
    setAcceptHoverEvents(val);
    setAcceptedMouseButtons(val ? Qt::AllButtons : Qt::NoButton);
}

}  // namespace TechDrawGui
