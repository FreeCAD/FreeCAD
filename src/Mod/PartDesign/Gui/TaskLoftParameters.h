/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef GUI_TASKVIEW_TaskLoftParameters_H
#define GUI_TASKVIEW_TaskLoftParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <QTextEdit>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderLoft.h"

class Ui_TaskLoftParameters;
class QListWidget;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {

class TaskLoftParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskLoftParameters(ViewProviderLoft *LoftView,bool newObj=false,QWidget *parent = 0);
    ~TaskLoftParameters();

private Q_SLOTS:
    void onProfileButton(bool);
    void onRefButtonAdd(bool);
    void onRefButtonRemvove(bool);
    void onClosed(bool);
    void onRuled(bool);
    void onDeleteSection();

protected:
    void changeEvent(QEvent *e);

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI(int index);
    bool referenceSelected(const Gui::SelectionChanges& msg) const;
    void removeFromListWidget(QListWidget*w, QString name);
    void clearButtons();
    void exitSelectionMode();

private:
    QWidget* proxy;
    Ui_TaskLoftParameters* ui;

    enum selectionModes { none, refAdd, refRemove, refProfile };
    selectionModes selectionMode = none;
};

////////////////////////////////////////////////////////////////

class TaskLoftWireOrders : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskLoftWireOrders(ViewProviderLoft *LoftView,bool newObj=false,QWidget *parent = 0);
    ~TaskLoftWireOrders();

private Q_SLOTS:
    void onUpButton();
    void onDownButton();
    void onSwapButton();
    void onPreviewCheckBoxToggled(bool);
    void onEnabledCheckBoxToggled(bool);
    void onSketchBoxRowChanged(int currentRow);
    void onWireBoxSelectionChanged();
    void unFlashSelected();
    void onSketchBoxItemDoubleClicked(QListWidgetItem* item);

protected:
    void changeEvent(QEvent *e);

private:
    void blockSignals(bool blocked);
    void setupUI();
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI(int sketchindex, int wireindex, bool bFlash=true);
    bool referenceSelected(const Gui::SelectionChanges& msg) const;
    void flashSelected();
    void slotChangedObject(const App::DocumentObject&, const App::Property&);
    boost::signals2::connection connectModObject;
    void openTransaction(const char *);
    void commitTransaction();
    App::Color getColor(int which_color);
    QColor getQColor(int which_color);

    PartGui::ViewProviderPart* svp;
    App::Color oldProfileFlashColor;
    QListWidget* sketchBox;
    QListWidget* legendBox;
    QListWidget* wireBox;
    QLabel* sketchLabel;
    QLabel* wiresLabel;
    QPushButton* upButton;
    QPushButton* downButton;
    QPushButton* swapButton;
    QCheckBox* enabledCheckBox;
    QCheckBox* previewCheckBox;

};

//////////////////////////////////////////////////////////////////

/// simulation dialog for the TaskView
class TaskDlgLoftParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgLoftParameters(ViewProviderLoft *LoftView,bool newObj=false);
    ~TaskDlgLoftParameters();

    ViewProviderLoft* getLoftView() const
    { return static_cast<ViewProviderLoft*>(vp); }

    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    virtual bool reject();

protected:
    TaskLoftParameters  *parameter;
    TaskLoftWireOrders *wireorder;
    std::vector<std::vector<int> > oldOrders; //to restore line colors on accept or reject


};
///////////////////////////







} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
