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


#ifndef GUI_TASKVIEW_TaskSelectLinkProperty_H
#define GUI_TASKVIEW_TaskSelectLinkProperty_H

#include "TaskView.h"
#include <Gui/Selection.h>
#include <App/PropertyLinks.h>


class Ui_TaskSelectLinkProperty;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
namespace TaskView {

/** General Link/Selection editor for the Task view
 *  This can be used as part of a TaskDialog to alter
 *  the content of a LinkProperty by user input/selection.
 *  If set active it reflects the selection to the Property 
 *  given and acts due the selection filter given to the constructor. 
 *  It will allow only allowed elements to be selected (SelectionFilter)
 *  and shows by the background color if the selection criterion is met. 
 *  With the call of accept() or reject() the result gets permanent or
 *  discarded in the given Property.
 */


class GuiExport TaskSelectLinkProperty : public TaskBox, public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    TaskSelectLinkProperty(const char *,App::Property *,QWidget *parent = nullptr);
    ~TaskSelectLinkProperty();
    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                  Gui::SelectionSingleton::MessageType Reason);

    /// set the filter criterion (same as in constructor)
    bool setFilter(const char*);

    /// set the TaskSelectLinkProperty active, means setting the selection and control it
    void activate(void);

    /// call this to accept the changes the user has made and send back to the Property (Ok)
    bool accept(void);
    /// This discards the changes of the user and leaves the Property untouched (Cancel)
    bool reject(void);
    /// send the selection to the Property for e.g. forced recomputation of a feature
    void sendSelection2Property(void);
    /// checks if the filter is currently met
    inline bool isSelectionValid(void) const {return Filter->match();}

private Q_SLOTS:
    void on_Remove_clicked(bool);
    void on_Add_clicked(bool);
    void on_Invert_clicked(bool);
    void on_Help_clicked(bool);

Q_SIGNALS:
    void emitSelectionFit(void);
    void emitSelectionMisfit(void);

protected:
    void changeEvent(QEvent *e);

private:
    // checks for selection and set background color and signals
    void checkSelectionStatus(void);

    QWidget* proxy;
    Ui_TaskSelectLinkProperty* ui;

    // selection filter for the session 
    Gui::SelectionFilter *Filter;

    // possible used property types, only one is used
    App::PropertyLinkSub  *LinkSub;
    App::PropertyLinkList *LinkList;

    // string stores the Property at the beginning (for Cancel)
    std::vector<std::string> StartValueBuffer;
    App::DocumentObject      *StartObject;
};

} //namespace TaskView
} //namespace Gui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
