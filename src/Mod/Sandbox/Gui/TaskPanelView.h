/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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



#ifndef SANDBOXGUI_TASKPANELVIEW_H
#define SANDBOXGUI_TASKPANELVIEW_H

#include <QWidget>

namespace SandboxGui {

/** A test class. A more elaborate class description.
 */
class TaskPanelView : public QWidget
{
    //Q_OBJECT

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    TaskPanelView(QWidget *parent=0);

    /**
     * A destructor.
     * A more elaborate description of the destructor.
    */
    virtual ~TaskPanelView();

    void executeAction();
    void on_rbDefaultScheme_toggled(bool b);
    void on_rbXPBlueScheme_toggled(bool b);
    void on_rbXPBlue2Scheme_toggled(bool b);
    void on_rbVistaScheme_toggled(bool b);
    void on_rbMacScheme_toggled(bool b);
    void on_rbAndroidScheme_toggled(bool b);

private:
    QWidget* actionGroup;
};

} // namespace SandboxGui

#endif // SANDBOXGUI_TASKPANELVIEW_H
