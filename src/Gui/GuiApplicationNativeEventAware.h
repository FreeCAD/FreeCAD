/***************************************************************************
 *   Copyright (c) 2010 Thomas Anderson <ta@nextgenengineering>            *
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



#ifndef GUIAPPLICATIONNATIVEEVENTAWARE_H
#define GUIAPPLICATIONNATIVEEVENTAWARE_H

#include <QApplication>
#include <vector>

class QMainWindow;

namespace Gui
{
#if defined(_USE_3DCONNEXION_SDK) || defined(SPNAV_FOUND)
    class GuiNativeEvent;
#endif // Spacemice
    class GUIApplicationNativeEventAware : public QApplication
    {
        Q_OBJECT
    public:
        GUIApplicationNativeEventAware(int &argc, char *argv[]);
        ~GUIApplicationNativeEventAware();
        void initSpaceball(QMainWindow *window);
        bool isSpaceballPresent() const {return spaceballPresent;}
        void setSpaceballPresent(bool present) {spaceballPresent = present;} 
        bool processSpaceballEvent(QObject *object, QEvent *event);
        void postMotionEvent(std::vector<int> motionDataArray);
        void postButtonEvent(int buttonNumber, int buttonPress);
    private:
        bool spaceballPresent;
        void importSettings(std::vector<int>& motionDataArray);
        float convertPrefToSensitivity(int value);
      #if defined(_USE_3DCONNEXION_SDK) || defined(SPNAV_FOUND)
        GuiNativeEvent *nativeEvent;
      #endif
      #if defined(SPNAV_FOUND) && defined(SPNAV_USE_X11) && QT_VERSION < 0x050000
        bool x11EventFilter(XEvent *event) override final;
      #endif
    }; // end class GUIApplicationNativeEventAware
} // end namespace Gui

#endif // GUIAPPLICATIONNATIVEEVENTAWARE_H
