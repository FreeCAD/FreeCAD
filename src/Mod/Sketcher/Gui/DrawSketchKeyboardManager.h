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


#pragma once


#include <QEvent>
#include <QKeyEvent>

#include <QTimer>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>


namespace Gui
{
class ViewProvider;
}  // namespace Gui

namespace SketcherGui
{

class ViewProviderSketch;

/** Class implementing an event filter for DrawSketchHandler tools, enabling seamless introduction
 * of values to parameters, including units, while still allowing operation of shortcuts.
 *
 * The basic mechanism to decide which control should respond is based on using timers, type of
 * entered event.
 */
class DrawSketchKeyboardManager: public QObject
{
    Q_OBJECT

public:
    DrawSketchKeyboardManager();


    /** Indicates whether the DSH control (e.g. on-view parameter or widget) should handle keyboard
     * input or should signal it via boost */
    enum class KeyboardEventHandlingMode
    {
        DSHControl,
        ViewProvider
    };

    /// returns whether the provided entity will currently receive the event.
    bool isMode(KeyboardEventHandlingMode mode);

    /// returns which entity will currently receive the event.
    KeyboardEventHandlingMode getMode();

    bool eventFilter(QObject* object, QEvent* event);

    /// sets the timeout to the amount of milliseconds.
    void setTimeOut(int milliseconds);

    // returns the current timeout amount
    int timeOut();

private:
    /// This function decides whether events should be send to the ViewProvider
    /// or to the UI control of DSH.
    void detectKeyboardEventHandlingMode(QKeyEvent* keyEvent);

    void onTimeOut();

private:
    /// Viewer responsible for the active document
    Gui::View3DInventorViewer* vpViewer = nullptr;
    KeyboardEventHandlingMode keyMode;

    QTimer timer;

    int timeOutValue = 2000;
};

}  // namespace SketcherGui
