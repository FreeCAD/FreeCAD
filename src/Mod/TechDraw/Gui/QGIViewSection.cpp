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

# include <cmath>
# include <limits>

# include <QGraphicsPathItem>
# include <QLineF>
# include <QTimer>

#include <Mod/TechDraw/App/DrawComplexSection.h>
#include <Mod/TechDraw/App/DrawViewSection.h>

#include <App/Document.h>
#include <Base/Tools.h>

#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIViewSection.h"
#include "PreferencesGui.h"
#include "QGIEdge.h"
#include "QGIFace.h"
#include "QGISectionLine.h"
#include "Rez.h"
#include "ViewProviderDrawingView.h"
#include "ViewProviderViewSection.h"
#include "ZVALUE.h"


using namespace TechDrawGui;
using FillMode = QGIFace::FillMode;

namespace
{
class SectionPlacementConnector final : public QGraphicsPathItem
{
public:
    explicit SectionPlacementConnector(QGraphicsItem* parent) :
        QGraphicsPathItem(parent)
    {}

    int type() const override
    {
        return UserType::QGISectionConnector;
    }
};
}

QGIViewSection::QGIViewSection()
{
    m_placementConnector = new SectionPlacementConnector(this);
    m_placementConnector->setAcceptedMouseButtons(Qt::NoButton);
    m_placementConnector->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_placementConnector->setZValue(ZVALUE::EDGE - 1);
    m_placementConnector->hide();
}

void QGIViewSection::draw()
{
    if (!isVisible()) {
        m_placementConnector->hide();
        return;
    }

    auto* section = dynamic_cast<TechDraw::DrawViewSection*>(getViewObject());
    if (section && section->SectionCutOnly.getValue()) {
        // Section faces are drawn separately below. Clear any primitives from
        // a previous full-section draw without constructing a projected
        // half-solid that would immediately be discarded.
        prepareGeometryChange();
        removePrimitives();
        removeDecorations();
        drawViewDecorations();
        prepareGeometryChange();
    }
    else {
        QGIViewPart::draw();
    }
    drawSectionFace();
    connectPlacementConnectorToBase();
    updatePlacementConnector();

    // Creating a section can schedule one more geometry/model update after
    // this draw. Refresh after it settles so a persistent connector starts
    // and ends at the final rendered centers.
    QTimer::singleShot(0, this, [this]() {
        updatePlacementConnector();
    });
}

QVariant QGIViewSection::itemChange(GraphicsItemChange change,
                                    const QVariant& value)
{
    QVariant result = QGIViewPart::itemChange(change, value);
    if ((change == ItemPositionHasChanged
         || change == ItemScenePositionHasChanged)
        && scene()) {
        updatePlacementConnector();
    }
    return result;
}

void QGIViewSection::connectPlacementConnectorToBase()
{
    QObject::disconnect(m_basePositionConnection);
    QObject::disconnect(m_basePositionFinishedConnection);
    m_basePositionConnection = {};
    m_basePositionFinishedConnection = {};
    auto* section =
        dynamic_cast<TechDraw::DrawViewSection*>(getViewObject());
    auto* base = section ? section->getBaseDVP() : nullptr;
    auto* baseProvider = base
        ? freecad_cast<ViewProviderDrawingView*>(getViewProvider(base))
        : nullptr;
    auto* baseItem = baseProvider
        ? dynamic_cast<QGIView*>(baseProvider->getQView())
        : nullptr;
    if (baseItem) {
        m_lastBaseScenePosition = baseItem->scenePos();
        m_hasBaseScenePosition = true;
        m_basePositionConnection = connect(
            baseItem, &QGIView::positionChanged,
            this, &QGIViewSection::basePositionChanged);
        m_basePositionFinishedConnection = connect(
            baseItem, &QGIView::positionChangeFinished,
            this, &QGIViewSection::basePositionChangeFinished);
    }
    else {
        m_hasBaseScenePosition = false;
    }
}

void QGIViewSection::basePositionChanged()
{
    auto* section =
        dynamic_cast<TechDraw::DrawViewSection*>(getViewObject());
    auto* base = section ? section->getBaseDVP() : nullptr;
    auto* baseProvider = base
        ? freecad_cast<ViewProviderDrawingView*>(getViewProvider(base))
        : nullptr;
    auto* baseItem = baseProvider
        ? dynamic_cast<QGIView*>(baseProvider->getQView())
        : nullptr;
    if (!baseItem) {
        m_hasBaseScenePosition = false;
        updatePlacementConnector();
        return;
    }
    const QPointF currentBasePosition = baseItem->scenePos();
    if (section->LockRelativePositionToSource.getValue()
        && m_hasBaseScenePosition) {
        const QPointF movement = currentBasePosition - m_lastBaseScenePosition;
        if (!movement.isNull()) {
            const QPointF movedSceneOrigin = mapToScene(QPointF()) + movement;
            const QPointF movedParentPosition = parentItem()
                ? parentItem()->mapFromScene(movedSceneOrigin)
                : movedSceneOrigin;
            setPositionWithoutSnapping(movedParentPosition);
            m_followingBasePosition = true;
        }
    }
    m_lastBaseScenePosition = currentBasePosition;
    m_hasBaseScenePosition = true;
    updatePlacementConnector();
}

void QGIViewSection::basePositionChangeFinished()
{
    if (m_followingBasePosition) {
        // The source view still owns the drag transaction at this point, so
        // persist the dependent move as part of the same undoable action.
        QGIViewPart::dragFinished();
        m_followingBasePosition = false;
    }
    updatePlacementConnector();
}

void QGIViewSection::dragFinished()
{
    auto* section =
        dynamic_cast<TechDraw::DrawViewSection*>(getViewObject());
    const bool positionChanged = section
        && (!TechDraw::DrawUtil::fpCompare(
                section->X.getValue(), Rez::appX(pos().x()), 0.001)
            || !TechDraw::DrawUtil::fpCompare(
                section->Y.getValue(), Rez::appX(-pos().y()), 0.001));
    App::Document* document = section ? section->getDocument() : nullptr;
    const bool ownTransaction = positionChanged && document
        && document->getTransactionID(true) == 0;
    if (ownTransaction) {
        document->openTransaction("Drag section view");
    }
    if (positionChanged) {
        section->LockRelativePositionToSource.setValue(isPositionSnapped());
    }
    QGIViewPart::dragFinished();
    if (ownTransaction) {
        document->commitTransaction();
    }
    updatePlacementConnector();
    // QGIView clears the transient snap state immediately after dragFinished.
    // Refresh once more afterward so a disabled connection line does not stay
    // visible merely because this drag ended on a snap target.
    QTimer::singleShot(0, this, [this]() {
        updatePlacementConnector();
    });
}

void QGIViewSection::updatePlacementConnector()
{
    auto* section =
        dynamic_cast<TechDraw::DrawViewSection*>(getViewObject());
    if (!section
        || (!section->ConnectionLine.getValue() && !isPositionSnapped())
        || !isVisible() || !scene()) {
        m_placementConnector->hide();
        return;
    }
    auto* base = section->getBaseDVP();
    auto* baseProvider = base
        ? freecad_cast<ViewProviderDrawingView*>(getViewProvider(base))
        : nullptr;
    auto* baseItem = baseProvider
        ? dynamic_cast<QGIView*>(baseProvider->getQView())
        : nullptr;
    auto* sectionProvider =
        freecad_cast<ViewProviderViewSection*>(getViewProvider(section));
    if (!base || !baseItem || !sectionProvider) {
        m_placementConnector->hide();
        return;
    }

    const Base::Vector3d sectionOrigin = section->SectionOrigin.getValue();
    const Base::Vector3d baseShapeCenter = base->getCurrentCentroid();
    const Base::Vector3d baseSectionOrigin =
        base->projectPoint(sectionOrigin, false);
    const Base::Vector3d baseProjectedCenter =
        base->projectPoint(baseShapeCenter, false);
    const Base::Vector3d baseOffset =
        (baseSectionOrigin - baseProjectedCenter) * base->getScale();
    const QPointF calculatedBaseAnchor = baseItem->mapToScene(
        QPointF(Rez::guiX(baseOffset.x), -Rez::guiX(baseOffset.y)));

    QPointF baseAnchor = calculatedBaseAnchor;
    QPointF lineCenterAnchor = calculatedBaseAnchor;
    QPointF lineDirection;
    const QString sectionName = QString::fromUtf8(
        section->getNameInDocument());
    // Attach to the section line that the user sees, including its rotation
    // and any complex-section path, rather than to the base view centroid.
    for (QGraphicsItem* child : baseItem->childItems()) {
        auto* sectionLine = dynamic_cast<QGISectionLine*>(child);
        if (sectionLine && child->data(10).toString() == sectionName) {
            const QPointF lineCenter = sectionLine->mapToScene(
                sectionLine->lineCenter());
            lineCenterAnchor = lineCenter;
            const QPointF localDirection = sectionLine->lineDirection();
            if (!localDirection.isNull()) {
                const QPointF directionEnd = sectionLine->mapToScene(
                    sectionLine->lineCenter() + localDirection);
                const QLineF renderedDirection(lineCenter, directionEnd);
                if (renderedDirection.length() > 1.0e-6) {
                    lineDirection =
                        renderedDirection.unitVector().p2()
                        - renderedDirection.unitVector().p1();
                    baseAnchor = lineCenter + lineDirection
                        * QPointF::dotProduct(
                            calculatedBaseAnchor - lineCenter,
                            lineDirection);
                }
            }
            break;
        }
    }

    const Base::Vector3d cutCenter = section->projectPoint(
        section->getCutCentroid(), false);
    Base::Vector3d sectionAnchorOffset =
        (section->projectPoint(sectionOrigin, false) - cutCenter)
        * section->getScale();
    sectionAnchorOffset.RotateZ(
        Base::toRadians(section->Rotation.getValue()));
    const QPointF sectionAnchor = mapToScene(QPointF(
        Rez::guiX(sectionAnchorOffset.x),
        -Rez::guiX(sectionAnchorOffset.y)));
    QPointF connectorEnd = sectionAnchor;
    if (!lineDirection.isNull()) {
        // Keep the endpoint attached to the station in the section that
        // corresponds to the cutting-line midpoint. If the section is moved
        // away from the view-direction guide, the connector is then free to
        // angle instead of remaining artificially parallel to that guide.
        const QPointF correspondingStation = sectionAnchor + lineDirection
            * QPointF::dotProduct(
                lineCenterAnchor - baseAnchor, lineDirection);
        const QRectF visibleGeometry = frameRect();
        // Captions remain in the frame even when the cut has no geometry.
        // They must not move the connector away from the section datum.
        const QPointF sliceCenter =
            !section->hasGeometry() || visibleGeometry.isEmpty()
            ? sectionAnchor : mapToScene(visibleGeometry.center());
        connectorEnd = sliceCenter + lineDirection
            * QPointF::dotProduct(
                correspondingStation - sliceCenter, lineDirection);
    }

    QPainterPath path;
    path.moveTo(mapFromScene(lineCenterAnchor));
    path.lineTo(mapFromScene(connectorEnd));
    QPen pen = centerLinePen(sectionProvider->LineWidth.getValue());
    pen.setColor(PreferencesGui::getAccessibleQColor(
        PreferencesGui::normalQColor()));
    m_placementConnector->setPen(pen);
    m_placementConnector->setPath(path);
    m_placementConnector->setVisible(
        QLineF(lineCenterAnchor, connectorEnd).length() > 1.0e-6);
}

void QGIViewSection::drawSectionFace()
{
    auto section( dynamic_cast<TechDraw::DrawViewSection *>(getViewObject()) );
    if (!section) {
        return;
    }

    if (!section->hasGeometry()) {
        return;
    }

    ViewProviderViewSection* sectionVp = freecad_cast<ViewProviderViewSection*>(QGIView::getViewProvider(section));
    if (!sectionVp) {
        return;
    }

    auto sectionFaces( section->getTDFaceGeometry() );
    if (sectionFaces.empty()) {
        return;
    }

    float lineWidth    = sectionVp->LineWidth.getValue();
    const auto* complexSection =
        dynamic_cast<const TechDraw::DrawComplexSection*>(section);
    // The projected edge layer supplies the complete outline for a normal
    // partial section, including its dashed transition boundary. Drawing the
    // face outline as well would show a solid line through the dash gaps.
    const bool partialSectionWithProjectedEdges =
        complexSection && complexSection->isPartialSection()
        && !section->SectionCutOnly.getValue();
    const bool partialSectionCutOnly =
        complexSection && complexSection->isPartialSection()
        && section->SectionCutOnly.getValue();
    const QPainterPath partialClip = partialSectionClipPath();

    QPointF partialStart;
    QPointF partialEnd;
    QPointF partialStartNormal;
    QPointF partialEndNormal;
    if (partialSectionCutOnly
        && complexSection->ProjectionStrategy.getValue() == 0) {
        const auto boundaries =
            complexSection->partialSectionBoundaryPoints();
        const auto directions =
            complexSection->partialSectionBoundaryDirections();
        partialStart = QPointF(Rez::guiX(boundaries.first.x),
                               Rez::guiX(boundaries.first.y));
        partialEnd = QPointF(Rez::guiX(boundaries.second.x),
                             Rez::guiX(boundaries.second.y));
        partialStartNormal = QPointF(directions.first.x,
                                     directions.first.y);
        partialEndNormal = QPointF(directions.second.x,
                                   directions.second.y);
    }
    auto isPartialBoundary = [&](const QPainterPath& path) {
        if (!partialSectionCutOnly
            || complexSection->ProjectionStrategy.getValue() != 0
            || path.elementCount() < 2) {
            return false;
        }
        constexpr double boundaryTolerance = 0.05;
        bool onStart = true;
        bool onEnd = true;
        for (int elementIndex = 0;
             elementIndex < path.elementCount();
             ++elementIndex) {
            const auto element = path.elementAt(elementIndex);
            const QPointF point(element.x, element.y);
            onStart = onStart
                && std::abs(QPointF::dotProduct(
                       point - partialStart, partialStartNormal))
                    < boundaryTolerance;
            onEnd = onEnd
                && std::abs(QPointF::dotProduct(
                       point - partialEnd, partialEndNormal))
                    < boundaryTolerance;
        }
        return onStart || onEnd;
    };

    const auto alignedBoundaries = complexSection
        ? complexSection->alignedSectionBoundaryInfo()
        : TechDraw::AlignedSectionBoundaryInfo {};
    const QPointF alignedNormal(alignedBoundaries.normal.x,
                                alignedBoundaries.normal.y);
    const QPointF alignedTangent(-alignedNormal.y(), alignedNormal.x());
    auto guiPoint = [](const Base::Vector3d& point) {
        return QPointF(Rez::guiX(point.x), Rez::guiX(point.y));
    };
    const QPointF alignedStart = guiPoint(alignedBoundaries.startPoint);
    const QPointF alignedCenter = guiPoint(alignedBoundaries.centerPoint);
    const QPointF alignedEnd = guiPoint(alignedBoundaries.endPoint);
    const bool alignedSectionWithProjectedEdges =
        alignedBoundaries.valid && !section->SectionCutOnly.getValue();
    const bool customSectionFaceEdges = partialSectionCutOnly
        || (alignedBoundaries.valid
            && section->SectionCutOnly.getValue());
    auto isOnAlignedBoundary = [&](const QPainterPath& path,
                                   const QPointF& point) {
        if (!alignedBoundaries.valid || path.elementCount() < 2) {
            return false;
        }
        constexpr double tolerance = 0.05;
        for (int i = 0; i < path.elementCount(); ++i) {
            const auto element = path.elementAt(i);
            if (std::abs(QPointF::dotProduct(
                    QPointF(element.x, element.y) - point,
                    alignedNormal)) >= tolerance) {
                return false;
            }
        }
        return true;
    };
    double centerMinimum = std::numeric_limits<double>::max();
    double centerMaximum = std::numeric_limits<double>::lowest();

    std::vector<TechDraw::FacePtr>::iterator fit = sectionFaces.begin();
    int i = 0;
    for(; fit != sectionFaces.end(); fit++, i++) {
        QGIFace* newFace = drawFace(*fit, -1);
        newFace->setPaintClip(partialClip);
        newFace->setZValue(ZVALUE::SECTIONFACE);
        // A normal cut-only view has no projected half-solid to provide the
        // boundary, so its section faces must draw their own outline. A
        // partial cut-only view needs separate edge items because its
        // transition boundary uses a different line style.
        if (!customSectionFaceEdges
            && (section->SectionCutOnly.getValue()
                || (section->showSectionEdges()
                    && !partialSectionWithProjectedEdges
                    && !alignedSectionWithProjectedEdges))) {
            newFace->setDrawEdges(true);
            newFace->setStyle(Qt::SolidLine);
            newFace->setWidth(Rez::guiX(lineWidth));
        } else {
            newFace->setDrawEdges(false);
        }

        if (section->CutSurfaceDisplay.isValue("Hide")) {
            newFace->setFillMode(FillMode::NoFill);
            newFace->setFillColor(Qt::transparent);
        }
        else if (section->CutSurfaceDisplay.isValue("Color")) {
            newFace->isHatched(true);
            QColor faceColor = (sectionVp->CutSurfaceColor.getValue()).asValue<QColor>();
            faceColor.setAlpha((100 - sectionVp->CutSurfaceTransparency.getValue())*255/100);
            newFace->setFillColor(faceColor);
            newFace->setFillMode(faceColor.alpha() ? FillMode::PlainFill : FillMode::NoFill);
        } else if (section->CutSurfaceDisplay.isValue("SvgHatch")) {
            newFace->isHatched(true);
            newFace->setFillMode(FillMode::SvgFill);
            newFace->setHatchColor(sectionVp->HatchColor.getValue());
            newFace->setHatchScale(section->HatchScale.getValue());
            newFace->setHatchRotation(section->HatchRotation.getValue());
            newFace->setHatchOffset(section->HatchOffset.getValue());
            std::string hatchSpec = section->SvgIncluded.getValue();
            newFace->setHatchFile(hatchSpec);
        } else if (section->CutSurfaceDisplay.isValue("PatHatch")) {
            newFace->isHatched(true);
            newFace->setFillMode(FillMode::GeomHatchFill);
            newFace->setHatchColor(sectionVp->GeomHatchColor.getValue());
            newFace->setHatchScale(section->HatchScale.getValue());
            newFace->setHatchRotation(section->HatchRotation.getValue());
            newFace->setHatchOffset(section->HatchOffset.getValue());
            newFace->setLineWeight(sectionVp->WeightPattern.getValue());
            std::vector<TechDraw::LineSet> lineSets = section->getDrawableLines(i);
            if (!lineSets.empty()) {
                for (auto& ls: lineSets) {
                    newFace->addLineSet(ls);
                }
            }
        } else {
            Base::Console().warning("QGIVS::draw - unknown CutSurfaceDisplay: %d\n",
                                    section->CutSurfaceDisplay.getValue());
        }

        newFace->draw();
        newFace->setPrettyNormal();
        newFace->setAcceptHoverEvents(false);
        newFace->setFlag(QGraphicsItem::ItemIsSelectable, false);

        if (customSectionFaceEdges) {
            for (const TechDraw::Wire* wire : (*fit)->wires) {
                for (const TechDraw::BaseGeomPtr& geom : wire->geoms) {
                    const QPainterPath edgePath = drawPainterPath(geom);
                    if (isOnAlignedBoundary(edgePath, alignedCenter)) {
                        for (int elementIndex = 0;
                             elementIndex < edgePath.elementCount();
                             ++elementIndex) {
                            const auto element =
                                edgePath.elementAt(elementIndex);
                            const double position = QPointF::dotProduct(
                                QPointF(element.x, element.y)
                                    - alignedCenter,
                                alignedTangent);
                            centerMinimum = std::min(centerMinimum, position);
                            centerMaximum = std::max(centerMaximum, position);
                        }
                        continue;
                    }
                    const bool alignedPartialBoundary =
                        (alignedBoundaries.startPartial
                         && isOnAlignedBoundary(edgePath, alignedStart))
                        || (alignedBoundaries.endPartial
                            && isOnAlignedBoundary(edgePath, alignedEnd));
                    const bool partialBoundary =
                        isPartialBoundary(edgePath)
                        || alignedPartialBoundary;
                    auto* edge = new QGIEdge(-1);
                    addToGroupWithoutUpdate(edge);
                    edge->setPath(edgePath);
                    edge->setNormalColor(
                        PreferencesGui::getAccessibleQColor(
                            PreferencesGui::normalQColor()));
                    if (partialBoundary) {
                        edge->setLinePen(centerLinePen(lineWidth * 2.0));
                    }
                    else {
                        edge->setStyle(Qt::SolidLine);
                        edge->setWidth(Rez::guiX(lineWidth));
                    }
                    edge->setPos(0.0, 0.0);
                    edge->setZValue(ZVALUE::EDGE);
                    edge->setPrettyNormal();
                    edge->setAcceptHoverEvents(false);
                    edge->setAcceptedMouseButtons(Qt::NoButton);
                    edge->setFlag(QGraphicsItem::ItemIsSelectable, false);
                }
            }
        }
    }

    if (centerMinimum <= centerMaximum) {
        constexpr double extension = 4.0;
        const double guiExtension = Rez::guiX(extension);
        QPainterPath centerPath;
        centerPath.moveTo(
            alignedCenter
            + alignedTangent * (centerMinimum - guiExtension));
        centerPath.lineTo(
            alignedCenter
            + alignedTangent * (centerMaximum + guiExtension));
        auto* centerEdge = new QGIEdge(-1);
        addToGroupWithoutUpdate(centerEdge);
        centerEdge->setPath(centerPath);
        centerEdge->setNormalColor(
            PreferencesGui::getAccessibleQColor(
                PreferencesGui::normalQColor()));
        centerEdge->setLinePen(centerLinePen(lineWidth));
        centerEdge->setPos(0.0, 0.0);
        centerEdge->setZValue(ZVALUE::EDGE + 1);
        centerEdge->setPrettyNormal();
        centerEdge->setAcceptHoverEvents(false);
        centerEdge->setAcceptedMouseButtons(Qt::NoButton);
        centerEdge->setFlag(QGraphicsItem::ItemIsSelectable, false);
    }
}

void QGIViewSection::updateView(bool update)
{
    Q_UNUSED(update);
    auto viewPart( dynamic_cast<TechDraw::DrawViewSection *>(getViewObject()) );
    if (!viewPart)
        return;
    draw();
    QGIView::updateView(update);
}
