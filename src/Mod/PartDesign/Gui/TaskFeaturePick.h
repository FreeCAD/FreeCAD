/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#ifndef PARTDESIGNGUI_FeaturePickDialog_H
#define PARTDESIGNGUI_FeaturePickDialog_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/ViewProviderOrigin.h>
#include <App/DocumentObject.h>

#include <boost/function.hpp>

namespace PartDesignGui {

class SoSwitch;
class Ui_TaskFeaturePick;
class TaskFeaturePick : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    enum featureStatus {
        validFeature = 0,
        invalidShape,
        noWire,
        isUsed,
        otherBody,
        basePlane,
        afterTip
    };

    TaskFeaturePick(std::vector<App::DocumentObject*> &objects, 
                    const std::vector<featureStatus> &status,
                    QWidget *parent = 0);
    
    ~TaskFeaturePick();

    std::vector<App::DocumentObject*> getFeatures();
    bool getReverse();

protected Q_SLOTS:
    void onCheckReverse(bool);
    void onCheckOtherFeature(bool);
    void onCheckOtherBody(bool);
    void onUpdate(bool);

    void onSelectionChanged(const Gui::SelectionChanges& msg);
    
private:
    Ui_TaskFeaturePick* ui;
    QWidget* proxy;
    SoSwitch* featureswitch;
    std::vector<Gui::ViewProviderOrigin*> origins;

    std::vector<QString> features;
    std::vector<featureStatus> statuses;

    void updateList();

    const QString getFeatureStatusString(const featureStatus st);
};


/// simulation dialog for the TaskView
class TaskDlgFeaturePick : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgFeaturePick(std::vector<App::DocumentObject*> &objects, 
                        const std::vector<TaskFeaturePick::featureStatus> &status,
                        boost::function<bool (std::vector<App::DocumentObject*>)> acceptfunc,
                        boost::function<void (std::vector<App::DocumentObject*>)> workfunc);
    ~TaskDlgFeaturePick();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button 
    virtual bool isAllowedAlterDocument(void) const
    { return false; }

    /// returns for Close and Help button 
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

protected:
    TaskFeaturePick  *pick;
    bool accepted;
    boost::function<bool (std::vector<App::DocumentObject*>)>  acceptFunction;
    boost::function<void (std::vector<App::DocumentObject*>)>  workFunction;
};

}

#endif // PARTDESIGNGUI_FeaturePickDialog_H
