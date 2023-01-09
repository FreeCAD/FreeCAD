/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com >    *
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

#ifndef GUI_NOTIFICATIONBOX_H
#define GUI_NOTIFICATIONBOX_H



namespace Gui {

    /* This class provides a non-intrusive tip alike notification
     * dialog, which unlike QToolTip, is kept shown during a time
     * unless it is clicked.
     *
     * The time is calculated based on the length if not provided.
     *
     * This class interface and its implementation are based on QT's
     * QToolTip.
     */
    class NotificationBox
    {
        NotificationBox() = delete;
    public:
        static void showText(const QPoint &pos, const QString &text, int msecShowTime = -1);
        static inline void hideText() { showText(QPoint(), QString()); }
        static bool isVisible();
        static QString text();
        static QPalette palette();
        static void setPalette(const QPalette &);
        static QFont font();
        static void setFont(const QFont &);
    };


} // namespace Gui

#endif // GUI_NOTIFICATIONBOX_H
