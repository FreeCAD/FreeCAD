/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QMessageBox>
# include <QProgressDialog>
# include <QWindow>
# ifdef FC_OS_WIN32
#   include <windows.h>
# endif
#endif

#include "WaitCursor.h"

using namespace Gui;

namespace Gui {
class WaitCursorP : public QObject
{
public:
    static WaitCursorP* getInstance();
    void setBusy(bool);
    WaitCursor::FilterEventsFlags ignoreEvents() const;
    void setIgnoreEvents(WaitCursor::FilterEventsFlags flags);

protected:
    bool eventFilter(QObject*, QEvent*) override;
    bool isModalDialog(QObject* o) const;

private:
    WaitCursorP(); // Disable constructor
    static WaitCursorP* _instance;
    bool isOn{false};
    WaitCursor::FilterEventsFlags flags{WaitCursor::AllEvents};
};
} // namespace Gui

WaitCursorP* WaitCursorP::_instance = nullptr;

WaitCursorP::WaitCursorP() : QObject(nullptr)
{
}

WaitCursorP* WaitCursorP::getInstance()
{
    if (!_instance)
        _instance = new WaitCursorP();
    return _instance;
}

void WaitCursorP::setBusy(bool on)
{
    if (on == this->isOn)
        return;

    if (on) {
        qApp->installEventFilter(this);
        QApplication::setOverrideCursor(Qt::WaitCursor);
    }
    else {
        qApp->removeEventFilter(this);
        QApplication::restoreOverrideCursor();
    }

    this->isOn = on;
}

WaitCursor::FilterEventsFlags WaitCursorP::ignoreEvents() const
{
    return this->flags;
}

void WaitCursorP::setIgnoreEvents(WaitCursor::FilterEventsFlags flags)
{
    this->flags = flags;
}

bool WaitCursorP::isModalDialog(QObject* o) const
{
    QWidget* parent = qobject_cast<QWidget*>(o);
    if (!parent) {
        QWindow* window = qobject_cast<QWindow*>(o);
        if (window)
            parent = QWidget::find(window->winId());
    }
    while (parent) {
        auto dlg = qobject_cast<QMessageBox*>(parent);
        if (dlg && dlg->isModal())
            return true;
        auto pd = qobject_cast<QProgressDialog*>(parent);
        if (pd)
            return true;
        parent = parent->parentWidget();
    }

    return false;
}

bool WaitCursorP::eventFilter(QObject* o, QEvent* e)
{
    // Note: This might cause problems when we want to open a modal dialog at the lifetime
    // of a WaitCursor instance because the incoming events are still filtered.
    if (e->type() == QEvent::KeyPress ||
        e->type() == QEvent::KeyRelease) {
        if (isModalDialog(o))
            return false;
        if (this->flags & WaitCursor::KeyEvents)
            return true;
    }
    if (e->type() == QEvent::MouseButtonPress ||
        e->type() == QEvent::MouseButtonRelease ||
        e->type() == QEvent::MouseButtonDblClick) {
        if (isModalDialog(o))
            return false;
        if (this->flags & WaitCursor::MouseEvents)
            return true;
    }
    return false;
}

int WaitCursor::instances = 0;

/**
 * Constructs this object and shows the wait cursor immediately. If you need to open a dialog as
 * long as an instance of WaitCursor exists you must call restoreCursor() before and setWaitCursor()
 * afterwards because all key events and mouse button events are filtered, otherwise you will run
 * into strange behaviour.
 */
WaitCursor::WaitCursor()
{
    if (instances++ == 0)
        setWaitCursor();
    filter = WaitCursorP::getInstance()->ignoreEvents();
}

/** Restores the last cursor again. */
WaitCursor::~WaitCursor()
{
    if (--instances == 0)
        restoreCursor();
    WaitCursorP::getInstance()->setIgnoreEvents(filter);
}

/**
 * Sets the wait cursor if needed.
 */
void WaitCursor::setWaitCursor()
{
    WaitCursorP::getInstance()->setBusy(true);
}

/**
 * Restores the last cursor if needed.
 */
void WaitCursor::restoreCursor()
{
    WaitCursorP::getInstance()->setBusy(false);
}

WaitCursor::FilterEventsFlags WaitCursor::ignoreEvents() const
{
    return WaitCursorP::getInstance()->ignoreEvents();
}

void WaitCursor::setIgnoreEvents(FilterEventsFlags flags)
{
    WaitCursorP::getInstance()->setIgnoreEvents(flags);
}
