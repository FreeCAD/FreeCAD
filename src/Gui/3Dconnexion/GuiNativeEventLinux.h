/***************************************************************************
 *   Copyright (c) 2018 Torsten Sadowski <tsadowski[at]gmx.net>            *
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

#ifndef GUINATIVEEVENT_H
#define GUINATIVEEVENT_H

#include "GuiAbstractNativeEvent.h"

class QMainWindow;

namespace Gui
{
    class GUIApplicationNativeEventAware;

    class GuiNativeEvent : public GuiAbstractNativeEvent
    {
    Q_OBJECT
    public:
        GuiNativeEvent(GUIApplicationNativeEventAware *app);
        ~GuiNativeEvent() override;
        void initSpaceball(QMainWindow *window) override final;
    private:
        GuiNativeEvent();
        GuiNativeEvent(const GuiNativeEvent&);
        GuiNativeEvent& operator=(const GuiNativeEvent&);
    private Q_SLOTS:
        void pollSpacenav();
    };
}

#endif //GUINATIVEEVENT_H

