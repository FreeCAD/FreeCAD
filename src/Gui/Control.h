/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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


#ifndef GUI_CONTROL_H
#define GUI_CONTROL_H

// Std. configurations

#include <QObject>
#include <bitset>
#include <stack>

#include <Gui/TaskView/TaskDialog.h>

namespace App
{
  class DocumentObject;
  class Document;
}

namespace Gui
{
namespace TaskView
{
    class TaskDialog;
    class TaskView;
}


/** The control class
 */
class GuiExport ControlSingleton : public QObject
{
     Q_OBJECT

public:
    static ControlSingleton& instance(void);
    static void destruct (void);

    /** @name dialog handling 
     *  this methods are used to control the TaskDialog stuff.
     */
    //@{
    /// This method start an Task dialog in the TaskView
    void showDialog(Gui::TaskView::TaskDialog *dlg);
    Gui::TaskView::TaskDialog* activeDialog() const;
    //void closeDialog();
    //@}

    /** @name task view handling 
     */
    //@{
    Gui::TaskView::TaskView* taskPanel() const;
    /// raising the model view
    void showModelView();
    //@}

    bool isAllowedAlterDocument(void) const;
    bool isAllowedAlterView(void) const;
    bool isAllowedAlterSelection(void) const;

public Q_SLOTS:
    void closeDialog();
    /// raises the task view panel 
    void showTaskView();

private Q_SLOTS:
    /// This get called by the TaskView when the Dialog is finished
    void closedDialog();

private:
    struct status {
        std::bitset<32> StatusBits;
    } CurrentStatus;

    std::stack<status> StatusStack;

    Gui::TaskView::TaskDialog *ActiveDialog;
 
private:
    /// Construction
    ControlSingleton();
    /// Destruction
    virtual ~ControlSingleton();

    static ControlSingleton* _pcSingleton;
};

/// Get the global instance
inline ControlSingleton& Control(void)
{
    return ControlSingleton::instance();
}

} //namespace Gui

#endif // GUI_CONTROL_H
