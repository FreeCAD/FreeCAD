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
# include <QEvent>
# include <QKeyEvent>
#endif

#include <Base/Console.h>
#include <QCoreApplication>

#include "LineEdit.h"

using namespace SpreadsheetGui;

LineEdit::LineEdit(QWidget *parent)
    : Gui::ExpressionLineEdit(parent, false, '=')
    , lastKeyPressed(0)
{
    setFocusPolicy(Qt::FocusPolicy::ClickFocus);
}

bool LineEdit::event(QEvent *event)
{
    if (event && event->type() == QEvent::FocusOut) {
        if (lastKeyPressed)
            Q_EMIT finishedWithKey(lastKeyPressed, lastModifiers);
        lastKeyPressed = 0;
    }
    else if (event && event->type() == QEvent::KeyPress && !completerActive()) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(event);
        lastKeyPressed = kevent->key();
        lastModifiers = kevent->modifiers(); 
    }
    return Gui::ExpressionLineEdit::event(event);
}

/////////////////////////////////////////////////////////////////////////////////////

TextEdit::TextEdit(QWidget *parent)
    : Gui::ExpressionTextEdit(parent)
    , lastKeyPressed(0)
{
    setLeadChar('=');
}

void TextEdit::keyPressEvent(QKeyEvent *event)
{
    Gui::ExpressionTextEdit::keyPressEvent(event);
}

void TextEdit::finishEditing()
{
    if (filtering) {
        filtering = false;
        qApp->removeEventFilter(this);
    }
    if (int key = lastKeyPressed) {
        lastKeyPressed = 0;
        Q_EMIT finishedWithKey(key, lastModifiers);
    }
}

bool TextEdit::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab && keyEvent->modifiers() == Qt::ControlModifier) {
            lastKeyPressed = 0;
            textCursor().insertText(QString::fromLatin1("\t"));
            event->accept();
            return true;
        }
    }
    return false;
}

bool TextEdit::event(QEvent *event)
{
    if (!event)
        return false;

    switch (event->type()) {
    case QEvent::FocusIn:
        if (!filtering)
            qApp->installEventFilter(this);
        break;
    case QEvent::FocusOut:
        finishEditing();
        break;
    case QEvent::KeyPress: {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(event);
        lastKeyPressed = kevent->key();
        lastModifiers = kevent->modifiers(); 

        switch(kevent->key()) {
        case Qt::Key_Tab:
            // We want TextEdit to mimic QLineEdit by default, so do not insert
            // Tab into the text box if no modifier
            if (kevent->modifiers() == Qt::NoModifier) {
                finishEditing();
                event->accept();
                return true;
            }
            // For some reason Ctrl + Tab is never passed to us even having
            // handled the ShortcutOverride below. It is possible that the
            // eventFilter in QMdiArea captures the event, which is installed on
            // its child sub window. It is not clear how this event filter can
            // intercept event passing to TextEdit which is a decendent of the
            // sub window. Anyway, we can work around the problem by installing
            // a system event filter and handle the Ctrl + Tab there.
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            // Same for Enter/Return key, except that we can handle Ctrl + Enter
            // here.
            if (kevent->modifiers() == Qt::ControlModifier) {
                lastKeyPressed = 0;
                textCursor().insertText(QString::fromLatin1("\n"));
            } else {
                // Unlike LineEdit, we are derived from QPlainTextEdit, so we
                // need to manually finish editing on Enter/Return key
                finishEditing();
            }
            event->accept();
            return true;
        default:
            break;
        }
        break;
    }
    case QEvent::ShortcutOverride: {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(event);
        if (kevent->modifiers() != Qt::ControlModifier)
            break;
        if (kevent->key() == Qt::Key_Enter
            || kevent->key() == Qt::Key_Return
            || kevent->key() == Qt::Key_Tab)
        {
            // Override any potential shortcut of Ctrl + Enter/Return/Tab
            event->accept();
        }
        break;
    }
    default:
        break;
    }
    return Gui::ExpressionTextEdit::event(event);
}

#include "moc_LineEdit.cpp"
