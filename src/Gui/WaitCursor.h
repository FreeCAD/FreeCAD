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

#ifndef GUI_WAIT_CURSOR_H
#define GUI_WAIT_CURSOR_H


namespace Gui {

/**
 * This class sets a waitcursor automatically while a slow operation is running.
 * Therefore you just have to create an instance of WaitCursor before the time
 * consuming operation starts.
 *
 * \code:
 * WaitCursor ac;
 * ...
 * ...                   // slow operation
 * ...
 * \endcode
 *
 * Sometimes you have two slow operations with some user interactions in between them.
 * Avoiding to show the waiting cursor then you have to call the methods @ref restoreCursor()
 * and setWaitCursor manually, like:
 *
 * \code:
 * WaitCursor ac;
 * ...
 * ...                   // 1st slow operation
 * ac.restoreCursor();
 * ...
 * ...                  // some dialog stuff
 * ac.setWaitCursor();
 * ...
 * ...                  // 2nd slow operation
 * \endcode
 *
 * @author Werner Mayer
 */
class GuiExport WaitCursor
{
public:
    enum FilterEventsFlag {
        NoEvents = 0x00,
        KeyEvents = 0x01,
        MouseEvents = 0x02,
        AllEvents = KeyEvents | MouseEvents
    };
    Q_DECLARE_FLAGS(FilterEventsFlags, FilterEventsFlag)

    WaitCursor();
    ~WaitCursor();

    void setWaitCursor();
    void restoreCursor();
    FilterEventsFlags ignoreEvents() const;
    void setIgnoreEvents(FilterEventsFlags flags = AllEvents);

private:
    FilterEventsFlags filter;
    static int instances;
};

} // namespace Gui

#endif // GUI_WAIT_CURSOR_H

