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
#endif

#include "LineEdit.h"


using namespace SpreadsheetGui;

LineEdit::LineEdit(QWidget* parent)
    : Gui::ExpressionLineEdit(parent, false, '=', true)
    , lastKeyPressed(0)
{
    setFocusPolicy(Qt::FocusPolicy::ClickFocus);
}

bool LineEdit::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object);
    if (event && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab) {
            // Special tab handling -- must be done via a QApplication event filter, otherwise the
            // widget system will always grab the tab events
            if (completerActive()) {
                hideCompleter();
                event->accept();
                return true;  // To make sure this tab press doesn't do anything else
            }
            else {
                lastKeyPressed = keyEvent->key();
                lastModifiers = keyEvent->modifiers();
            }
        }
    }
    return false;  // We don't usually actually "handle" the tab event, we just keep track of it
}

bool LineEdit::event(QEvent* event)
{
    if (event && event->type() == QEvent::FocusIn) {
        qApp->installEventFilter(this);
    }
    else if (event && event->type() == QEvent::FocusOut) {
        qApp->removeEventFilter(this);
        if (lastKeyPressed) {
            Q_EMIT finishedWithKey(lastKeyPressed, lastModifiers);
        }
        lastKeyPressed = 0;
    }
    else if (event && event->type() == QEvent::KeyPress && !completerActive()) {
        QKeyEvent* kevent = static_cast<QKeyEvent*>(event);
        lastKeyPressed = kevent->key();
        lastModifiers = kevent->modifiers();
    }
    return Gui::ExpressionLineEdit::event(event);
}

#include "moc_LineEdit.cpp"
