/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsProxyWidget>
#endif

#include "LineEdit.h"
#include <Gui/MainWindow.h>
#include "ZoomableView.h"


using namespace SpreadsheetGui;

LineEdit::LineEdit(QWidget* parent)
    : Gui::ExpressionLineEdit(parent, false, '=', true)
    , lastKeyPressed(0)
{
    setFocusPolicy(Qt::FocusPolicy::ClickFocus);
}

void LineEdit::setDocumentObject(const App::DocumentObject* currentDocObj, bool checkInList)
{
    ExpressionLineEdit::setDocumentObject(currentDocObj, checkInList);

    /* The code below is supposed to fix the input of an expression and to make the popup
     * functional. The input seems to be broken because of installed event filters. My solution is
     * to re-add the widget into the scene. Only a parentless widget can be added to the scene.
     * Making a widget parentless makes it lose its windowFlags, even if it is added to the scene.
     * So, the algorithm is to obtain globalPos, then to make the widget parentless,
     * to add it to the scene, setting the globalPos after. */

    QPointer<Gui::MDIView> active_view = Gui::MainWindow::getInstance()->activeWindow();
    if (!active_view) {
        Base::Console().developerWarning("LineEdit::setDocumentObject",
                                         "The active view is not a spreadsheet");
        return;
    }
    QPointer<ZoomableView> zv = active_view->findChild<ZoomableView*>();
    if (!zv) {
        Base::Console().developerWarning("LineEdit::setDocumentObject", "ZoomableView not found");
        return;
    }

    auto getPos = [this]() {
        return this->mapToGlobal(QPoint {0, 0});
    };
    const QPoint old_pos = getPos();

    auto xpopup = new XListView(this);
    getCompleter()->setPopup(xpopup);
    setParent(nullptr);
    QGraphicsProxyWidget* proxy_lineedit = zv->scene()->addWidget(this);

    const QPoint new_pos = getPos();
    const QPoint shift = old_pos - new_pos;
    const qreal scale_factor = static_cast<qreal>(zv->zoomLevel()) / 100.0;
    const qreal shift_x = static_cast<qreal>(shift.x()) / scale_factor,
                shift_y = static_cast<qreal>(shift.y()) / scale_factor;

    QTransform trans = proxy_lineedit->transform();
    proxy_lineedit->setTransform(trans.translate(shift_x, shift_y));


    auto getPopupPos = [proxy_lineedit, zv]() {
        const QPointF scene_pos =
            proxy_lineedit->mapToScene(proxy_lineedit->boundingRect().bottomLeft());
        const QPoint view_pos = zv->mapFromScene(scene_pos);
        const QPoint global_pos = zv->viewport()->mapToGlobal(view_pos);

        return global_pos;
    };

    auto getPopupWidth = [this, zv]() {
        const int zoom_level = zv->zoomLevel(), editors_width = this->width();

        return qMax(editors_width * zoom_level / 100, editors_width);
    };

    auto updatePopupGeom = [getPopupPos, getPopupWidth, xpopup]() {
        const QPoint new_pos = getPopupPos();
        xpopup->setGeometry(new_pos.x(), new_pos.y(), getPopupWidth(), xpopup->height());
    };

    QObject::connect(xpopup, &XListView::geometryChanged, this, updatePopupGeom);
}

void LineEdit::focusOutEvent(QFocusEvent* event)
{
    if (lastKeyPressed) {
        Q_EMIT finishedWithKey(lastKeyPressed, lastModifiers);
    }

    Gui::ExpressionLineEdit::focusOutEvent(event);
}

void LineEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key::Key_Tab && completerActive()) {
        hideCompleter();
        Gui::ExpressionLineEdit::keyPressEvent(event);
        return;
    }

    lastKeyPressed = event->key();
    lastModifiers = event->modifiers();

    if ((lastKeyPressed == Qt::Key::Key_Down || lastKeyPressed == Qt::Key::Key_Up)
        && completerActive()) {
        auto kevent = new QKeyEvent(QEvent::KeyPress, lastKeyPressed, Qt::NoModifier);
        QCoreApplication::postEvent(getCompleter()->popup(), kevent);
    }

    Gui::ExpressionLineEdit::keyPressEvent(event);
}

XListView::XListView(LineEdit* parent)
    : QListView(parent)
{
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setAttribute(Qt::WidgetAttribute::WA_ShowWithoutActivating);
}

void XListView::resizeEvent(QResizeEvent* event)
{
    Q_EMIT geometryChanged();
    QListView::resizeEvent(event);
}

void XListView::updateGeometries()
{
    QListView::updateGeometries();
    Q_EMIT geometryChanged();
}

#include "moc_LineEdit.cpp"
