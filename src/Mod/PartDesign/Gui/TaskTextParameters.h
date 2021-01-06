/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef GUI_TASKVIEW_TaskTextParameters_H
#define GUI_TASKVIEW_TaskTextParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderText.h"

class Ui_TaskTextParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {



class TaskTextParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskTextParameters(ViewProviderText *TextView,QWidget *parent = 0);
    ~TaskTextParameters();

    virtual void apply() override;

    /**
     * @brief fillAxisCombo fills the combo and selects the item according to
     * current value of revolution object's axis reference.
     * @param forceRefill if true, the combo box will be completely refilled. If
     * false, the current value of revolution object's axis will be added to the
     * list (if necessary), and selected. If the list is empty, it will be refilled anyway.
     */
    void fillAxisCombo(bool forceRefill = false);
    void addAxisToCombo(App::DocumentObject *linkObj, std::string linkSubname, QString itemText);

private Q_SLOTS:
    void onAxisChanged(int);
    void onTextChanged(QString);
    void onFontChanged(QFont);
    void onSizeChanged(double);
    void onHeightChanged(double);

private:
    void updateUI();

    QWidget* proxy;

    Ui_TaskTextParameters* ui;

    /**
     * @brief axesInList is the list of links corresponding to axis combo; must
     * be kept in sync with the combo. A special value of zero-pointer link is
     * for "Select axis" item.
     *
     * It is a list of pointers, because properties prohibit assignment. .
     */
    std::vector<std::unique_ptr<App::PropertyLinkSub>> axesInList;

    // this is a map between font family name and filepath to the font file.
    std::map<QString, QString> fontPath;

    // this method populates fontPath.
    void getFontPaths(void);

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void changeEvent(QEvent *e) override;
    bool updateView() const;
    void getReferenceAxis(App::DocumentObject *&obj, std::vector<std::string> &sub) const;


    //mirrors of helixes's properties
    App::PropertyString*      propText;
    App::PropertyLength*      propHeight;
    App::PropertyFont*        propFont;
    App::PropertyLength*      propSize;
    App::PropertyLinkSub*     propReferenceAxis;

};

/// simulation dialog for the TaskView
class TaskDlgTextParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgTextParameters(ViewProviderText *TextView);

    ViewProviderText* getTextView() const
    { return static_cast<ViewProviderText*>(vp); }
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskTextParameters_H
