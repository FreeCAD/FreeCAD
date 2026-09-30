// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include <QAbstractSpinBox>
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>

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
}

bool DrawSketchKeyboardManager::isMode(KeyboardEventHandlingMode mode)
{
    return mode == keyMode;
}

DrawSketchKeyboardManager::KeyboardEventHandlingMode DrawSketchKeyboardManager::getMode()
{
    return keyMode;
}

void DrawSketchKeyboardManager::resetMode()
{
    keyMode = KeyboardEventHandlingMode::ViewProvider;
}

bool DrawSketchKeyboardManager::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object);
    if (!vpViewer) {
        return false;
    }
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        // Here we decide if we redirect the input the viewer
        auto keyEvent = static_cast<QKeyEvent*>(event);

        // Pressing Alt/Option sends the keypress to the viewer
        // without permanently switching the destination (keyMode)
        bool altPressed = (keyEvent->modifiers() & Qt::AltModifier);
        if (isMode(KeyboardEventHandlingMode::DSHControl) && altPressed) {
            // Convert from Alt+Key to just Key For view navigation
            keyEvent->setModifiers(keyEvent->modifiers() & ~Qt::AltModifier);
            return QApplication::sendEvent(vpViewer, keyEvent);
        }
        const int key = keyEvent->key();
        if (key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Tab) {
            // This keys switch to camera control but are sent to the widget
            keyMode = KeyboardEventHandlingMode::ViewProvider;
            return false;
        }

        keyMode = detectKeyboardEventHandlingMode(keyEvent);  // determine the handler

        if (isMode(KeyboardEventHandlingMode::ViewProvider)) {
            return QApplication::sendEvent(vpViewer, keyEvent);
        }
        return false;  // do not intercept the event and feed it to the widget
    }

    return false;
}

DrawSketchKeyboardManager::KeyboardEventHandlingMode DrawSketchKeyboardManager::detectKeyboardEventHandlingMode(
    QKeyEvent* keyEvent
)
{
    QWidget* focusWidget = QApplication::focusWidget();
    bool isTextInputFocused = focusWidget
        && (qobject_cast<QAbstractSpinBox*>(focusWidget) || qobject_cast<QLineEdit*>(focusWidget)
            || qobject_cast<QAbstractSpinBox*>(focusWidget->parentWidget()));

    if (!isTextInputFocused) {
        return keyMode;
    }

    bool isPrintable = !keyEvent->text().isEmpty() && keyEvent->text().at(0).isPrint();
    bool isEditingKey = keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return
        || keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab
        || keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right
        || keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down
        || keyEvent->key() == Qt::Key_Backspace || keyEvent->matches(QKeySequence::Backspace)
        || keyEvent->matches(QKeySequence::Delete);

    if (isPrintable || isEditingKey) {
        return KeyboardEventHandlingMode::DSHControl;
    }
    return keyMode;
}

#include "moc_DrawSketchKeyboardManager.cpp"
