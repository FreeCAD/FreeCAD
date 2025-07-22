/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
#include <Inventor/events/SoKeyboardEvent.h>
#include <QApplication>
#include <QEvent>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#endif

#include "ViewProviderSketch.h"

#include "DrawSketchKeyboardManager.h"

using namespace SketcherGui;


DrawSketchKeyboardManager::DrawSketchKeyboardManager()
    : QObject(nullptr)
    , keyMode(KeyboardEventHandlingMode::DSHControl)
{
    // get the active viewer, so that we can send it key events
    auto doc = Gui::Application::Instance->activeDocument();

    if (doc) {
        auto temp = dynamic_cast<Gui::View3DInventor*>(doc->getActiveView());
        if (temp) {
            vpViewer = temp->getViewer();
            keyMode = KeyboardEventHandlingMode::ViewProvider;
        }
    }

    timer.setSingleShot(true);

    QObject::connect(&timer, &QTimer::timeout, [this]() {
        onTimeOut();
    });
}

bool DrawSketchKeyboardManager::isMode(KeyboardEventHandlingMode mode)
{
    return mode == keyMode;
}

DrawSketchKeyboardManager::KeyboardEventHandlingMode DrawSketchKeyboardManager::getMode()
{
    return keyMode;
}

bool DrawSketchKeyboardManager::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object);

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        /*If a key shortcut is required to work on sketcher when a tool using ui controls is being
         * used, then you have to add this key to the below section such that the spinbox doesn't
         * keep the keypress event for itself. Note if you want the event to be handled by the
         * spinbox too, you can return false.*/

        auto keyEvent = static_cast<QKeyEvent*>(event);

        detectKeyboardEventHandlingMode(keyEvent);  // determine the handler

        if (vpViewer && isMode(KeyboardEventHandlingMode::ViewProvider)) {
            return QApplication::sendEvent(vpViewer, keyEvent);
        }

        return false;  // do not intercept the event and feed it to the widget
    }

    return false;
}

void DrawSketchKeyboardManager::detectKeyboardEventHandlingMode(QKeyEvent* keyEvent)
{
    QRegularExpression rx(QStringLiteral("^[0-9]$"));
    auto match = rx.match(keyEvent->text());
    if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return
        || keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab
        || keyEvent->key() == Qt::Key_Minus || keyEvent->key() == Qt::Key_Period
        || keyEvent->key() == Qt::Key_Comma
        || match.hasMatch()
        // double check for backspace as there may be windows/unix inconsistencies
        || keyEvent->key() == Qt::Key_Backspace || keyEvent->matches(QKeySequence::Backspace)
        || keyEvent->matches(QKeySequence::Delete)) {
        keyMode = KeyboardEventHandlingMode::DSHControl;
        timer.start(timeOutValue);
    }
}

void DrawSketchKeyboardManager::onTimeOut()
{
    keyMode = KeyboardEventHandlingMode::ViewProvider;
}

/// sets the timeout to the amount of milliseconds.
void DrawSketchKeyboardManager::setTimeOut(int milliseconds)
{
    timeOutValue = milliseconds;
}

// returns the current timeout amount
int DrawSketchKeyboardManager::timeOut()
{
    return timeOutValue;
}


#include "moc_DrawSketchKeyboardManager.cpp"
