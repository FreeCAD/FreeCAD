/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

# include <cassert>

#include <QFocusEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QTextCursor>

#include <Mod/TechDraw/App/DrawView.h>

#include "QGICaption.h"
#include "QGIView.h"
#include "Rez.h"
#include "ZVALUE.h"


using namespace TechDrawGui;

QGICaption::QGICaption()
{
//    setCacheMode(QGraphicsItem::NoCache);
//    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setTextInteractionFlags(Qt::NoTextInteraction);

    setZValue(ZVALUE::VIEWCAPTION);
}

QVariant QGICaption::itemChange(GraphicsItemChange change, const QVariant &value)
{
    // only updates for caption of views
    auto* parentView = dynamic_cast<QGIView*>(parentItem());
    if (!parentView) {
        return QGCustomText::itemChange(change, value);
    }

    TechDraw::DrawView* viewObj = parentView->getViewObject();
    if (!viewObj) {
        return QGCustomText::itemChange(change, value);
    }

    if (change == ItemPositionChange) {
        return snapToView(value.toPointF());
    }

    if (change == ItemPositionHasChanged) {
        QPointF newPos = value.toPointF();
        Base::Vector3d newLocation(Rez::appX(newPos.x()), Rez::appX(-newPos.y()), 0.0);
        viewObj->CaptionLocation.setValue(newLocation);
    }

    return QGCustomText::itemChange(change, value);
}

QPointF QGICaption::snapToView(QPointF pos) {
    auto* parentView = dynamic_cast<QGIView*>(parentItem());
    if (!parentView) {
        return pos;
    }

    TechDraw::DrawView* viewObj = parentView->getViewObject();
    if (!viewObj) {
        return pos;
    }

    // Do not snap if control key is pressed
    if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier) {
        viewObj->CaptionSnap.setValue("NoSnap");
        return pos;
    }

    QRectF frameRect = parentView->getFrameRect();
    QRectF captionRect = this->boundingRect();

    QPointF topSnap = QPointF(frameRect.center().x() - (captionRect.width() / 2),
                              frameRect.top() - captionRect.height());
    QPointF bottomSnap = QPointF(frameRect.center().x() - (captionRect.width() / 2),
                              frameRect.bottom());
    QPointF leftSnap = QPointF(frameRect.left() - captionRect.width(),
                              frameRect.center().y() - (captionRect.height() / 2));
    QPointF rightSnap = QPointF(frameRect.right(),
                              frameRect.center().y() - (captionRect.height() / 2));


    constexpr double snapDistanceScreenPixels{15.0};
    double zoomFactor = 1.0;
    zoomFactor = scene()->views().first()->transform().m11();

    double snapDistance = snapDistanceScreenPixels / zoomFactor;

    for (const QPointF& snapPos : {topSnap, bottomSnap, leftSnap, rightSnap}) {
        qreal distance = QLineF(pos, snapPos).length();
        if (distance < snapDistance) {

            viewObj->CaptionSnap.setValue(snapPos == topSnap ? "Top" :
                                        snapPos == bottomSnap ? "Bottom" :
                                        snapPos == leftSnap ? "Left" :
                                        snapPos == rightSnap ? "Right" : "NoSnap");
            return snapPos;
        }

    }

    viewObj->CaptionSnap.setValue("NoSnap");
    return pos;
}

void QGICaption::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    setEditMode(true);
    event->accept();
}

void QGICaption::focusOutEvent(QFocusEvent* event)
{
    setEditMode(false);

    auto* parentView = dynamic_cast<QGIView*>(parentItem());
    if (parentView) {
        parentView->prepareCaption();
    }

    QGCustomText::focusOutEvent(event);
}

void QGICaption::keyPressEvent(QKeyEvent* event)
{
    if (m_isEditing
        && (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return
            || event->key() == Qt::Key_Enter)) {
        clearFocus();
        event->accept();
        return;
    }

    QGCustomText::keyPressEvent(event);
}

void QGICaption::setEditMode(bool enable)
{
    auto* parentView = dynamic_cast<QGIView*>(parentItem());
    TechDraw::DrawView* viewObj = parentView ? parentView->getViewObject() : nullptr;

    m_isEditing = enable;

    QTextCursor cursor = textCursor();

    if (enable) {
        // the text edited should be what is saved in the view object
        // so it for an example has the <SCALE> tag used for detail views
        if (viewObj) {
            setPlainText(QString::fromUtf8(viewObj->Caption.getValue()));
        }

        setTextInteractionFlags(Qt::TextEditorInteraction);
        setFocus(Qt::MouseFocusReason);

        cursor.select(QTextCursor::Document);
        setTextCursor(cursor);
        return;
    }

    setTextInteractionFlags(Qt::NoTextInteraction);
    cursor.clearSelection();
    setTextCursor(cursor);
    clearFocus();

    if (viewObj) {
        viewObj->Caption.setValue(toPlainText().toUtf8().constData());
    }
}
