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

#pragma once

#include <QSplashScreen>

namespace Gui
{

class SplashObserver;

/** This widget provides a splash screen that can be shown during application startup.
 *
 * \author Werner Mayer
 */
class SplashScreen: public QSplashScreen
{
    Q_OBJECT

public:
    explicit SplashScreen(const QPixmap& pixmap = QPixmap(), Qt::WindowFlags f = Qt::WindowFlags());
    ~SplashScreen() override;

    void setShowMessages(bool on);

    static QPixmap splashImage();

protected:
    void drawContents(QPainter* painter) override;

private:
    SplashObserver* messages;
};

}  // namespace Gui
