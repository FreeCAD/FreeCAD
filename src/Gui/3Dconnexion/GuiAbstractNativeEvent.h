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

#ifndef GUIABSTRACTNATIVEEVENT_H
#define GUIABSTRACTNATIVEEVENT_H

#include <QObject>
#include <vector>

class QMainWindow;

namespace Gui
{
	class GUIApplicationNativeEventAware;

	class GuiAbstractNativeEvent : public QObject
	{
	Q_OBJECT
	public:
		GuiAbstractNativeEvent(GUIApplicationNativeEventAware *app);
		virtual ~GuiAbstractNativeEvent()=0;
		virtual void initSpaceball(QMainWindow *window)=0;
	private:
		GuiAbstractNativeEvent();
		GuiAbstractNativeEvent(const GuiAbstractNativeEvent&);
		GuiAbstractNativeEvent& operator=(const GuiAbstractNativeEvent&);
	protected:
		static GUIApplicationNativeEventAware *mainApp;
        static std::vector<int>motionDataArray;
	};
}


#endif //GUIABSTRACTNATIVEEVENT_H
