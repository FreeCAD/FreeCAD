/***************************************************************************
 *   Copyright (c) 2012-2013 Luke Parry <l.parry@warwick.ac.uk>            *
 *   Copyright (c) 2024 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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

# include <QApplication>
# include <QGraphicsSceneHoverEvent>
# include <QGraphicsSceneMouseEvent>
# include <QPainter>
# include <QStyleOptionGraphicsItem>
# include <QTransform>


#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Tools.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawView.h>

#include "QGIView.h"
#include "MDIViewPage.h"
#include "PreferencesGui.h"
#include "QGCustomBorder.h"
#include "QGCustomClip.h"
#include "QGCustomImage.h"
#include "QGCustomLabel.h"
#include "QGICaption.h"
#include "QGIEdge.h"
#include "QGIVertex.h"
#include "QGIViewClip.h"
#include "QGIUserTypes.h"
#include "QGSPage.h"
#include "QGVPage.h"
#include "Rez.h"
#include "ViewProviderDrawingView.h"
#include "ViewProviderPage.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"


using namespace TechDrawGui;
using namespace TechDraw;
using DU = DrawUtil;

QGIView::QGIView()
    :QGraphicsItemGroup(),
    m_isHovered(false),
    viewObj(nullptr),
    m_innerView(false),
    m_multiselectActivated(false),
    snapping(false),
    m_label(new QGCustomLabel()),
    m_border(new QGCustomBorder()),
    m_caption(new QGICaption()),
    m_lock(new QGCustomImage())
{
    setCacheMode(QGraphicsItem::NoCache);
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);

    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_colNormal = prefNormalColor();
    m_colCurrent = m_colNormal;
    m_pen.setColor(m_colCurrent);

    m_decorPen.setStyle(Qt::DashLine);
    m_decorPen.setWidth(0); // 0 => 1px "cosmetic pen"

    addToGroup(m_label);
    addToGroup(m_border);
    addToGroup(m_caption);
    m_lock->setParentItem(m_border);
    m_lock->load(QStringLiteral(":/icons/TechDraw_Lock.svg"));
    QSize sizeLock = m_lock->imageSize();
    m_lockWidth = (double) sizeLock.width();
    m_lockHeight = (double) sizeLock.height();

    m_lock->hide();
    updateFrameVisibility();
}

void QGIView::isVisible(bool state)
{
    auto feat = getViewObject();
    if (!feat) return;
    auto vp = QGIView::getViewProvider(feat);
    if (!vp) return;
    Gui::ViewProviderDocumentObject* vpdo = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp);
    if (!vpdo) return;
    vpdo->Visibility.setValue(state);
}

bool QGIView::isVisible()
{
    auto feat = getViewObject();
    if (!feat) return false;
    auto vp = QGIView::getViewProvider(feat);
    if (!vp) return false;
    Gui::ViewProviderDocumentObject* vpdo = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp);
    if (!vpdo) return false;
    return vpdo->Visibility.getValue();
}

//Gets selection state for this view and/or eventually its children
bool QGIView::getGroupSelection()
{
    return isSelected();
}

//Set selection state for this and its children
//required for items like dimensions & balloons
void QGIView::setGroupSelection(bool isSelected)
{
    setSelected(isSelected);
}

// Set selection state of the feature (empty subName) or its sub items
void QGIView::setGroupSelection(bool isSelected, const std::vector<std::string> &subNames)
{
    Q_UNUSED(subNames);
    setGroupSelection(isSelected);
}

void QGIView::alignTo(QGraphicsItem*item, const QString &alignment)
{
    alignHash.clear();
    alignHash.insert(alignment, item);
}

QVariant QGIView::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();            //position within parent!
        TechDraw::DrawView* viewObj = getViewObject();
        auto* dpgi = dynamic_cast<TechDraw::DrawProjGroupItem*>(viewObj);
        if (dpgi && dpgi->getPGroup()) {
            // restrict movements of secondary views.
            if(alignHash.size() == 1) {   //if aligned.
                QGraphicsItem* item = alignHash.begin().value();
                QString alignMode   = alignHash.begin().key();
                if(alignMode == QStringLiteral("Vertical")) {
                    newPos.setX(item->pos().x());
                }
                else if(alignMode == QStringLiteral("Horizontal")) {
                    newPos.setY(item->pos().y());
                }
            }
        }
        else {
            // For general views we check if we need to snap to a position
            if (!(QApplication::keyboardModifiers() & Qt::AltModifier)) {
                snapPosition(newPos);
            }
        }

        return newPos;
    }

    // wf: why scene()? because if our selected state has changed because we have been removed from
    //     the scene, we don't do anything except wait to be deleted.
    if (change == ItemSelectedHasChanged && scene()) {
        if (isSelected() || hasSelectedChildren(this)) {
            m_colCurrent = getSelectColor();
            m_lock->setVisible(getViewObject()->isLocked() && getViewObject()->showLock());
        } else {
            dragFinished();

            if (!m_isHovered) {
                m_colCurrent = PreferencesGui::getAccessibleQColor(PreferencesGui::normalQColor());
                m_lock->hide();
            } else {
                m_colCurrent = getPreColor();
            }
        }
        updateFrameVisibility();
        drawBorder();
    }

    return QGraphicsItemGroup::itemChange(change, value);
}

//! The default behaviour here only applies to views whose (X, Y) describes a position on the page.
//! Others, like QGILeaderLine whose (X, Y) describes a position within a view's boundary and is not
//! draggable, should override this method.
void QGIView::dragFinished()
{
    if (!viewObj) {
        return;
    }

    double currX = viewObj->X.getValue();
    double currY = viewObj->Y.getValue();
    double candidateX = Rez::appX(pos().x());
    double candidateY = Rez::appX(-pos().y());
    bool setX = false;
    bool setY = false;

    const double tolerance = 0.001;  // mm
    if (!DrawUtil::fpCompare(currX, candidateX, tolerance)) {
        setX = true;
    }
    if (!DrawUtil::fpCompare(currY, candidateY, tolerance)) {
        setY = true;
    }

    if (!setX && !setY) {
        return;
    }

    bool ownTransaction = (viewObj->getDocument()->getTransactionID(true) == 0);

    if (ownTransaction) {
        Gui::Command::openCommand("Drag view");
    }
    // tell the feature that we have moved
    Gui::ViewProvider *vp = getViewProvider(viewObj);
    if (vp && !vp->isRestoring()) {
        snapping = true; // avoid triggering updateView by the VP updateData
        if (setX) {
            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.X = %f",
                                viewObj->getNameInDocument(), candidateX);
        }
        if (setY) {
            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.Y = %f",
                                viewObj->getNameInDocument(), candidateY);
        }

        snapping = false;
    }
    if (ownTransaction) {
        Gui::Command::commitCommand();
    }
}

//! align this view with others.  newPosition is in this view's parent's coord
//! system.  if this view is not in a ProjectionGroup, then this is the scene
//! position, otherwise it is the position within the ProjectionGroup.
void QGIView::snapPosition(QPointF& newPosition)
{
    if (!Preferences::SnapViews()) {
        return;
    }

    DrawView* feature = getViewObject();
    if (!feature) {
        return;
    }

    if (!feature->snapsToPosition()) {
        return;
    }

    auto* dvp = freecad_cast<DrawViewPart*>(feature);
    if (dvp  &&
        !dvp->hasGeometry()) {
        // too early. wait for updates to finish.
        return;
    }

    ViewProviderPage* vpPage = getViewProviderPage(feature);
    if (!vpPage) {
        // too early. not added to page yet?
        return;
    }

    QGSPage* scenePage = vpPage->getQGSPage();
    if (!scenePage) {
        return;
    }

    auto* sectionView = dynamic_cast<TechDraw::DrawViewSection*>(feature);
    if (sectionView) {
        snapSectionView(sectionView, newPosition);
        return;
    }

    // For general views we check if the view is close to aligned vertically or horizontally to another view.

    // if we are not a section view, then we could be in a projection group and
    // need to get the correct scene position.
    auto newScenePos = newPosition;
    if (parentItem()) {
        newScenePos = parentItem()->mapToScene(newPosition);
    }

    // First get a list of the views of the page.
    qreal snapPercent = Preferences::SnapLimitFactor();
    std::vector<QGIView*> views = scenePage->getViews();
    for (auto* view : views) {
        if (view == this) {
            continue;
        }
        auto viewFeature = view->getViewObject();
        auto viewDvp = freecad_cast<DrawViewPart*>(viewFeature);

        auto viewScenePos = view->scenePos();
        if (viewDvp &&
            DrawView::isProjGroupItem(viewDvp)) {
            viewScenePos = DU::toQPointF(projItemPagePos(viewDvp));
            viewScenePos = DU::invertY(Rez::guiX(viewScenePos));
        }

        auto xwindow = view->boundingRect().width() * snapPercent;
        auto ywindow = view->boundingRect().height() * snapPercent;

        auto xerror = fabs(newScenePos.x() - viewScenePos.x());
        auto yerror = fabs(newScenePos.y() - viewScenePos.y());

                // if the smaller of vertical and horizontal errors is within the acceptable
                // window, snap to position.
        if (xerror <= yerror  &&
            xerror <= xwindow) {
            newScenePos.setX(viewScenePos.x());
            if (parentItem()) {
                newScenePos = parentItem()->mapFromScene(newScenePos);
            }
            newPosition = newScenePos;
            return;
        }

        if (yerror < xerror &&
            yerror <= ywindow) {
            newScenePos.setY(viewScenePos.y());
            if (parentItem()) {
                newScenePos = parentItem()->mapFromScene(newScenePos);
            }
            newPosition = newScenePos;
            return;
        }
    }
}


//! snap this section view to its base view.  The section should be positioned on
//! line from the base view along the section normal direction, ie the same direction
//! as the arrows on the section line.
// Note: positions are in Qt inverted Y coordinates. They need to be converted before
// doing math on them, then converted back on return.
// Note: section views are never inside a ProjectionGroup, so their position is
// always in scene coordinates.
void QGIView::snapSectionView(const TechDraw::DrawViewSection* sectionView,
                              QPointF& newPosition)
{
    auto* baseView = sectionView->getBaseDVP();
    if (!baseView) {
        return;
    }
    auto* vpdv = freecad_cast<ViewProviderDrawingView*>(getViewProvider(baseView));
    if (!vpdv) {
        return;
    }
    auto* qgiv(dynamic_cast<QGIView*>(vpdv->getQView()));
    if (!qgiv) {
        return;
    }

    Base::Vector3d arrowDirection = sectionView->SectionNormal.getValue() * -1;
    auto arrowDirectionOnBase = baseView->projectPoint(arrowDirection, false);
    if (arrowDirectionOnBase.Length() < Precision::Confusion()) {
        return;
    }
    arrowDirectionOnBase.Normalize();
    double baseSize = Rez::guiX(baseView->getSizeAlongVector(arrowDirectionOnBase));
    double snapDist = baseSize * getScale() * Preferences::SnapLimitFactor();

    // find the scene position of the SO on the base view
    auto baseX = baseView->X.getValue();
    auto baseY = baseView->Y.getValue();
    Base::Vector3d baseScenePos{baseX, baseY, 0};       // paper space position
    if (DrawView::isProjGroupItem(baseView)) {
        baseScenePos = projItemPagePos(baseView);
    }
    auto sectionOrg3d      = sectionView->SectionOrigin.getValue();
    auto shapeCenter3d     = baseView->getCurrentCentroid();
    auto baseShapeCenter   = baseView->projectPoint(shapeCenter3d, false);
    auto baseSectionOrg    = baseView->projectPoint(sectionOrg3d, false);
    auto baseSOOffset      = (baseSectionOrg - baseShapeCenter) * baseView->getScale();
    auto baseSOScenePos    = baseScenePos + baseSOOffset;

    // find the SO offset from origin on the rotated & scaled sectionView
    auto sectionCutCenter     = sectionView->projectPoint(sectionView->getCutCentroid(), false);
    auto sectionSectionOrg    = sectionView->projectPoint(sectionOrg3d, false);
    auto sectionSOOffset      = (sectionSectionOrg - sectionCutCenter) * sectionView->getScale();
    auto sectionRotationDeg = sectionView->Rotation.getValue();
    sectionSOOffset.RotateZ(Base::toRadians(sectionRotationDeg));

            // from here on, we work with scene units (1/10 mm)
    sectionSOOffset = Rez::guiX(sectionSOOffset);
    baseSOScenePos  = Rez::guiX(baseSOScenePos);

            // check our alignment
    auto newSOPosition = DU::invertY(DU::toVector3d(newPosition)) + sectionSOOffset;

    Base::Vector3d actualAlignmentVector = newSOPosition - baseSOScenePos;
    actualAlignmentVector.Normalize();

    // if we are not on the correct side of the section line, we should not try to snap
    auto dot = arrowDirectionOnBase.Dot(actualAlignmentVector);
    if (dot <= 0) {
        return;
    }

    auto pointOnArrowLine = newSOPosition.Perpendicular(baseSOScenePos, arrowDirectionOnBase);
    auto errorVector = pointOnArrowLine - newSOPosition;
    if (errorVector.Length() < snapDist) {
        // get the position point corresponding to our SO alignment
        auto netPosition = pointOnArrowLine - sectionSOOffset;
        netPosition = DU::invertY(netPosition);
        newPosition = DU::toQPointF(netPosition);
    }

    return;
}

Base::Vector3d  QGIView::projItemPagePos(DrawViewPart* item)
{
    if (!DrawView::isProjGroupItem(item)) {
        return Base::Vector3d(0, 0, 0);
    }
    auto dpgi = static_cast<DrawProjGroupItem*>(item);
    auto group = dpgi->getPGroup();

    auto itemX = dpgi->X.getValue();
    auto itemY = dpgi->Y.getValue();
    Base::Vector3d itemOffsetPos{itemX, itemY, 0};       // relative to group
    auto groupX = group->X.getValue();
    auto groupY = group->Y.getValue();
    Base::Vector3d groupPagePos{groupX, groupY, 0};    // relative to page
    return groupPagePos + itemOffsetPos;
}


void QGIView::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    // this is never called for balloons (and dimensions?) because the label objects do not
    // inherit from QGIView, but directly from QGraphicsItem. - wf

    Qt::KeyboardModifiers originalModifiers = event->modifiers();
    if (event->button()&Qt::LeftButton) {
        m_multiselectActivated = false;
    }

    if (event->button() == Qt::LeftButton && PreferencesGui::multiSelection()) {
        std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
        if (!DrawGuiUtil::getSubsForSelectedObject(selection, getViewObject()).empty()) {
            // we have already selected geometry for this view
            m_multiselectActivated = true;
            event->setModifiers(originalModifiers | Qt::ControlModifier);
        }
    }

    QGraphicsItemGroup::mousePressEvent(event);

    event->setModifiers(originalModifiers);
}

void QGIView::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    QGraphicsItemGroup::mouseMoveEvent(event);
}

void QGIView::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    Qt::KeyboardModifiers originalModifiers = event->modifiers();
    if ((event->button()&Qt::LeftButton) && m_multiselectActivated) {
        if (PreferencesGui::multiSelection()) {
            event->setModifiers(originalModifiers | Qt::ControlModifier);
        }

        m_multiselectActivated = false;
    }

    dragFinished();
    QGraphicsItemGroup::mouseReleaseEvent(event);

    event->setModifiers(originalModifiers);
}

void QGIView::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsItemGroup::hoverEnterEvent(event);

    m_isHovered = true;

    if (isSelected()) {
        m_colCurrent = getSelectColor();
    } else {
        m_colCurrent = getPreColor();
    }

    updateFrameVisibility();

    m_lock->setVisible(getViewObject()->isLocked() && getViewObject()->showLock());

    drawBorder();
}


void QGIView::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsItemGroup::hoverLeaveEvent(event);

    m_isHovered = false;

    if (isSelected()) {
        m_colCurrent = getSelectColor();
        m_lock->setVisible(getViewObject()->isLocked() && getViewObject()->showLock());
    } else {
        m_colCurrent = PreferencesGui::getAccessibleQColor(PreferencesGui::normalQColor());
        m_lock->hide();
    }

    updateFrameVisibility();
    drawBorder();
}

//sets position in /Gui(graphics), not /App
void QGIView::setPosition(qreal xPos, qreal yPos)
{
    double newX = xPos;
    double newY = -yPos;
    double oldX = pos().x();
    double oldY = pos().y();

    if (TechDraw::DrawUtil::fpCompare(newX, oldX) &&
        TechDraw::DrawUtil::fpCompare(newY, oldY)) {
        return;
    } else {
        setPos(newX, newY);
    }
}

QGIViewClip* QGIView::getClipGroup()
{
    if (!getViewObject()->isInClip()) {
        return nullptr;
    }

    auto parentClip( dynamic_cast<QGCustomClip*>( parentItem() ) );
    if (!parentClip) return nullptr;

    auto parentView( dynamic_cast<QGIViewClip*>( parentClip->parentItem() ) );
    return parentView;
}

void QGIView::updateView(bool forceUpdate)
{
    setMovableFlag();

    if (getViewObject() && forceUpdate) {
        setPosition(Rez::guiX(getViewObject()->X.getValue()),
                    Rez::guiX(getViewObject()->Y.getValue()));
    }

    double appRotation = getViewObject()->Rotation.getValue();
    double guiRotation = rotation();
    if (!TechDraw::DrawUtil::fpCompare(appRotation, guiRotation)) {
        rotateView();
    }

    updateFrameVisibility();
    drawBorder();

    QGIView::draw();
}

//QGIVP derived classes do not need a rotate view method as rotation is handled on App side.
void QGIView::rotateView()
{
    //NOTE: QPainterPaths have to be rotated individually. This transform handles Rotation for everything else.
    //Scale is handled in GeometryObject for DVP & descendents
    //Objects not descended from DVP must setScale for themselves
    //note that setTransform(, ,rotation, ,) is not the same as setRotation!!!
    double rot = getViewObject()->Rotation.getValue();
    QPointF centre = boundingRect().center();
    setTransform(QTransform().translate(centre.x(), centre.y()).rotate(-rot).translate(-centre.x(), -centre.y()));
}

double QGIView::getScale()
{
    TechDraw::DrawView* feat = getViewObject();
    if (!feat) {
        return 1.0;
    }
    return feat->getScale();
}
const char * QGIView::getViewName() const
{
    return viewName.c_str();
}
const std::string QGIView::getViewNameAsString() const
{
    return viewName;
}


TechDraw::DrawView * QGIView::getViewObject() const
{
    return viewObj;
}

void QGIView::setViewFeature(TechDraw::DrawView *obj)
{
    if (!obj)
        return;

    viewObj = obj;
    viewName = obj->getNameInDocument();

            //mark the actual QGraphicsItem so we can check what's in the scene later
    setData(0, QStringLiteral("QGIV"));
    setData(1, QString::fromUtf8(obj->getNameInDocument()));
}

void QGIView::toggleCache(bool state)
{
    // temp for devl. chaching was hiding problems WF
    //setCacheMode((state)? NoCache : NoCache);
    Q_UNUSED(state);
    setCacheMode(NoCache);
}

void QGIView::draw()
{
    double xFeat, yFeat;
    if (getViewObject()) {
        xFeat = Rez::guiX(getViewObject()->X.getValue());
        yFeat = Rez::guiX(getViewObject()->Y.getValue());
        if (!getViewObject()->LockPosition.getValue()) {
            setPosition(xFeat, yFeat);
        }
    }
    if (isVisible()) {
        show();
    } else {
        hide();
    }
}

void QGIView::prepareCaption()
{
    m_caption->setDefaultTextColor(prefNormalColor());
    m_font.setFamily(Preferences::labelFontQString());
    int fontSize = exactFontSize(Preferences::labelFont(),
                                 Preferences::labelFontSizeMM());
    m_font.setPixelSize(fontSize);
    m_caption->setFont(m_font);
    QString captionStr = QString::fromUtf8(getViewObject()->Caption.getValue());
    m_caption->setPlainText(captionStr);
}

void QGIView::layoutDecorations(const QRectF& contentArea,
                              const QRectF& captionRect,
                              const QRectF& labelRect,
                              QRectF& outFrameRect,
                              QPointF& outCaptionPos,
                              QPointF& outLabelPos,
                              QPointF& outLockPos) const
{
    constexpr double padding{10};
    QRectF paddedContentArea = contentArea.adjusted(-padding, -padding, padding, padding);

    double frameWidth = paddedContentArea.width();
    // For standard views, expand frame to fit label. For RichAnno, keep frame tight to text.
    if (type() != UserType::QGIRichAnno) {
        frameWidth = qMax(frameWidth, labelRect.width());
    }
    double frameHeight = paddedContentArea.height();

    outFrameRect = QRectF(paddedContentArea.center().x() - (frameWidth / 2),
                          paddedContentArea.top(),
                          frameWidth,
                          frameHeight).adjusted(-padding, - padding, padding, padding);

    double firstTextVerticalPos = outFrameRect.bottom();
    if (m_caption->toPlainText().isEmpty()) {
        outLabelPos = QPointF(outFrameRect.center().x() - (labelRect.width() / 2),
                              firstTextVerticalPos);
    } else {
        outCaptionPos = QPointF(outFrameRect.center().x() - (captionRect.width() / 2),
                                firstTextVerticalPos);
        outLabelPos = QPointF(outFrameRect.center().x() - (labelRect.width() / 2),
                              firstTextVerticalPos + captionRect.height());
    }

    outLockPos = QPointF(outFrameRect.left(), outFrameRect.bottom() - m_lockHeight);
}


void QGIView::drawBorder()
{
    auto feat = getViewObject();
    if (!feat) {
        return;
    }

    prepareCaption();

    m_label->setDefaultTextColor(m_colCurrent);
    m_font.setFamily(Preferences::labelFontQString());
    int fontSize = exactFontSize(Preferences::labelFont(),
                                 Preferences::labelFontSizeMM());
    m_font.setPixelSize(fontSize);
    m_label->setFont(m_font);
    QString labelStr = QString::fromStdString(getViewObject()->Label.getValue());
    m_label->setPlainText(labelStr);

    QRectF contentArea = frameRect();
    QRectF captionRect = m_caption->boundingRect();
    QRectF labelRect = m_label->boundingRect();


    QRectF finalFrameRect;
    QPointF finalCaptionPos, finalLabelPos, finalLockPos;

    layoutDecorations(contentArea, captionRect, labelRect,
                      finalFrameRect, finalCaptionPos, finalLabelPos, finalLockPos);


    m_caption->setPos(finalCaptionPos);
    m_label->setPos(finalLabelPos);
    m_lock->setPos(finalLockPos);
    m_lock->setZValue(ZVALUE::LOCK);

    m_border->setBrush(Qt::NoBrush);
    m_decorPen.setColor(m_colCurrent);
    m_border->setPen(m_decorPen);
    m_border->setPos(0., 0.);
    m_border->setRect(finalFrameRect);

    prepareGeometryChange();
}

void QGIView::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //    painter->setPen(Qt::red);
    //    painter->drawRect(boundingRect());          //good for debugging

    QGraphicsItemGroup::paint(painter, &myOption, widget);
}


//! this is a specialized version of customChildrenBoundingRect used only for calculating the size
//! of the frame around selected views.
//! we could reduce code duplication here, but would incur an execution time cost to make a second
//! pass through the child items to add/subtract to the result of customChildrenBoundingRect.
QRectF QGIView::frameRect() const
{
    QList<QGraphicsItem*> children = childItems();
    // exceptions not to be included in determining the frame rectangle
    QRectF result;
    for (auto& child : children) {
        if (!child->isVisible()) {
            continue;
        }
        if (
            // we only want the area defined by the edges
            child->type() != UserType::QGIRichAnno &&
            child->type() != UserType::QGEPath &&
            child->type() != UserType::QGMText &&
            child->type() != UserType::QGCustomBorder &&
            child->type() != UserType::QGCustomLabel &&
            child->type() != UserType::QGICaption &&
            child->type() != UserType::QGIVertex &&
            child->type() != UserType::QGICMark  &&
            child->type() != UserType::QGIViewDimension &&
            child->type() != UserType::QGIViewBalloon) {
            QRectF childRect = mapFromItem(child, child->boundingRect()).boundingRect();
            result = result.united(childRect);
        }
    }
    return result;
}

//! this is the original customChildrenBoundingRect - used for calculating the bounding rect of
//! all the items that have to move with this view.
QRectF QGIView::customChildrenBoundingRect() const
{
    QList<QGraphicsItem*> children = childItems();
    // exceptions not to be included in determining the frame rectangle
    QRectF result;
    for (auto& child : children) {
        if (!child->isVisible()) {
            continue;
        }
        if (
            child->type() != UserType::QGIRichAnno &&
            child->type() != UserType::QGEPath &&
            child->type() != UserType::QGMText &&
            child->type() != UserType::QGCustomBorder &&
            child->type() != UserType::QGCustomLabel &&
            child->type() != UserType::QGICaption &&
            // we treat vertices as part of the boundingRect to allow loose vertices outside of the
            // area defined by the edges as in frameRect()
            // child->type() != UserType::QGIVertex &&
            child->type() != UserType::QGICMark) {
            QRectF childRect = mapFromItem(child, child->boundingRect()).boundingRect();
            result = result.united(childRect);
        }
    }
    return result;
}

QRectF QGIView::boundingRect() const
{
    QRectF totalRect = customChildrenBoundingRect();
    totalRect = totalRect.united(m_border->rect());
    totalRect = totalRect.united(m_label->boundingRect().translated(m_label->pos()));
    totalRect = totalRect.united(m_caption->boundingRect().translated(m_caption->pos()));

    if (m_lock && m_lock->isVisible()) {
        totalRect = totalRect.united(m_border->mapRectToParent(m_lock->boundingRect().translated(m_lock->pos())));
    }
    if (totalRect.isEmpty()) {
        return QRectF(0, 0, 1, 1);
    }
    return totalRect;
}

QGIView* QGIView::getQGIVByName(std::string name) const
{
    QList<QGraphicsItem*> qgItems = scene()->items();
    QList<QGraphicsItem*>::iterator it = qgItems.begin();
    for (; it != qgItems.end(); it++) {
        QGIView* qv = dynamic_cast<QGIView*>((*it));
        if (qv) {
            std::string qvName = qv->getViewNameAsString();
            if (name == qvName) {
                return (qv);
            }
        }
    }
    return nullptr;
}

/* static */
Gui::ViewProvider* QGIView::getViewProvider(App::DocumentObject* obj)
{
    if (obj) {
        Gui::Document* guiDoc = Gui::Application::Instance->getDocument(obj->getDocument());
        return guiDoc->getViewProvider(obj);
    }
    return nullptr;
}

MDIViewPage* QGIView::getMDIViewPage() const
{
    if (!getViewObject()) {
        return nullptr;
    }
    ViewProviderPage* vpp = getViewProviderPage(getViewObject());
    if (vpp) {
        return vpp->getMDIViewPage();
    }
    return nullptr;
}

ViewProviderPage* QGIView::getViewProviderPage(TechDraw::DrawView* dView)
{
    if (!dView)  {
        return nullptr;
    }
    TechDraw::DrawPage* page = dView->findParentPage();
    if (!page) {
        return nullptr;
    }

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(page->getDocument());
    if (!activeGui) {
        return nullptr;
    }

    return freecad_cast<ViewProviderPage*>(activeGui->getViewProvider(page));
}

//remove a child of this from scene while keeping scene indexes valid
void QGIView::removeChild(QGIView* child)
{
    if (child && (child->parentItem() == this) ) {
        prepareGeometryChange();
        scene()->removeItem(child);
    }
}

void QGIView::hideFrame()
{
    m_border->hide();
    m_label->hide();
}

void QGIView::addArbitraryItem(QGraphicsItem* qgi)
{
    if (qgi) {
        //        m_randomItems.push_back(qgi);
        addToGroup(qgi);
        qgi->show();
    }
}

void QGIView::switchParentItem(QGIView *targetParent)
{
    auto currentParent = dynamic_cast<QGIView *>(this->parentItem());
    if (currentParent != targetParent) {
        if (targetParent) {
            targetParent->addToGroup(this);
            targetParent->updateView();
            if (currentParent) {
                currentParent->updateView();
            }
        }
        else {
            while (currentParent) {
                currentParent->removeFromGroup(this);
                currentParent->updateView();
                currentParent = dynamic_cast<QGIView *>(this->parentItem());
            }
        }
    }
}

void QGIView::setStack(int z)
{
    m_zOrder = z;
    setZValue(z);
    draw();
}

void QGIView::setStackFromVP()
{
    TechDraw::DrawView* feature = getViewObject();
    ViewProviderDrawingView* vpdv = static_cast<ViewProviderDrawingView*>
        (getViewProvider(feature));
    int z = vpdv->getZ();
    setStack(z);
}

QColor QGIView::prefNormalColor()
{
    return PreferencesGui::getAccessibleQColor(PreferencesGui::normalQColor());
}

QColor QGIView::getPreColor()
{
    return PreferencesGui::getAccessibleQColor(PreferencesGui::preselectQColor());
}

QColor QGIView::getSelectColor()
{
    return PreferencesGui::getAccessibleQColor(PreferencesGui::selectQColor());
}

Base::Reference<ParameterGrp> QGIView::getParmGroupCol()
{
    return App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
}

//convert input font size in mm to scene units
//note that when used to set font size this will result in
//text that is smaller than sizeInMillimetres.  If exactly
//correct sized text is required, use exactFontSize.
int QGIView::calculateFontPixelSize(double sizeInMillimetres)
{
    // Calculate font size in pixels by using resolution conversion
    // and round to nearest integer
    return (int) (Rez::guiX(sizeInMillimetres) + 0.5);
}

int QGIView::calculateFontPixelWidth(const QFont &font)
{
    // Return the width of digit 0, most likely the most wide digit
    return Gui::QtTools::horizontalAdvance(QFontMetrics(font), QChar::fromLatin1('0'));
}

const double QGIView::DefaultFontSizeInMM = 5.0;

void QGIView::dumpRect(const char* text, QRectF rect) {
    Base::Console().message("DUMP - %s - rect: (%.3f, %.3f) x (%.3f, %.3f)\n", text,
                            rect.left(), rect.top(), rect.right(), rect.bottom());
}

//determine the required font size to generate text with upper case
//letter height = nominalSize
int QGIView::exactFontSize(std::string fontFamily, double nominalSize)
{
    double sceneSize = Rez::guiX(nominalSize);      //desired height in scene units
    QFont font;
    font.setFamily(QString::fromUtf8(fontFamily.c_str()));
    font.setPixelSize(sceneSize);

    QFontMetricsF fm(font);
    double capHeight = fm.capHeight();
    double ratio = sceneSize / capHeight;
    return (int) sceneSize * ratio;
}

void QGIView::makeMark(double xPos, double yPos, QColor color)
{
    QGIVertex* vItem = new QGIVertex(-1);
    vItem->setParentItem(this);
    vItem->setPos(xPos, yPos);
    vItem->setWidth(2.0);
    vItem->setRadius(20.0);
    vItem->setNormalColor(color);
    vItem->setFillColor(color);
    vItem->setPrettyNormal();
    vItem->setZValue(ZVALUE::VERTEX);
}

//! true if parent has any children which are selected
bool QGIView::hasSelectedChildren(QGIView* parent)
{
    QList<QGraphicsItem*> children = parent->childItems();

    auto itMatch = std::find_if(children.begin(), children.end(),
             [&](QGraphicsItem* child) {
                return child->isSelected();
             });

    return itMatch != children.end();
}


void QGIView::makeMark(Base::Vector3d pos, QColor color)
{
    makeMark(pos.x, pos.y, color);
}

void QGIView::makeMark(QPointF pos, QColor color)
{
    makeMark(pos.x(), pos.y(), color);
}

void QGIView::updateFrameVisibility()
{
    if (shouldShowFrame()) {
        m_border->show();
        m_label->show();
        if (m_lock && getViewObject()) {
            m_lock->setVisible(getViewObject()->isLocked() && getViewObject()->showLock());
        }
    } else {
        m_border->hide();
        m_label->hide();
        if (m_lock) {
             m_lock->hide();
        }
    }
}

bool QGIView::shouldShowFrame() const
{
    if (isExporting()) {
        return false;
    }

    if (isSelected()) {
        return true;
    }

    ViewFrameMode frameMode = PreferencesGui::getViewFrameMode();
    switch(frameMode) {
        case ViewFrameMode::Manual:
            return shouldShowFromViewProvider();
        case ViewFrameMode::AlwaysOn:
            return true;
        case ViewFrameMode::AlwaysOff:
            return false;
            break;
        default:
            return m_isHovered;
    };

}

bool QGIView::shouldShowFromViewProvider() const
{
    DrawView* feature = getViewObject();
    if (!feature) {
        return false;
    }
    ViewProviderPage* vpPage = getViewProviderPage(feature);
    if (!vpPage) {
        return false;
    }

    return vpPage->getFrameState();
}


bool QGIView::isExporting() const
{
    auto* view{freecad_cast<TechDraw::DrawView*>(getViewObject())};
    auto vpPage = getViewProviderPage(view);
    if (!view || !vpPage) {
        return false;
    }

    QGSPage* scenePage = vpPage->getQGSPage();
    if (!scenePage) {
        return false;
    }

    return scenePage->getExportingAny();
}

void QGIView::setMovableFlag()
{
    if (getViewObject()->isLocked()) {
        setFlag(QGraphicsItem::ItemIsMovable, false);
    } else {
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }
}

//! Retrieves objects of type T with given indexes
template <typename T>
std::vector<T> QGIView::getObjects(std::vector<int> indexes)
{
    QList<QGraphicsItem*> children = childItems();
    std::vector<T> result;
    for (QGraphicsItem*& child : children) {
        //                   Convert QGIVertex* (as T) to QGIVertex
        if (child->type() != std::remove_pointer<T>::type::Type) {
            continue;
        }

        // Get index of child item
        T object = static_cast<T>(child);
        int target = object->getProjIndex();
        // If child item's index in indexes, then add to results
        if (std::ranges::find(indexes, target) != indexes.end()) {
            result.push_back(object);
        }
    }
    return result;
}

template std::vector<QGIVertex*> QGIView::getObjects<QGIVertex*>(std::vector<int>);
template std::vector<QGIEdge*> QGIView::getObjects<QGIEdge*>(std::vector<int>);


#include <Mod/TechDraw/Gui/moc_QGIView.cpp>
