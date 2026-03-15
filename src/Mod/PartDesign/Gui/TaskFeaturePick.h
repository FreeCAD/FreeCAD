// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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

#pragma once

#include <functional>
#include <QListWidgetItem>

#include <App/DocumentObject.h>
#include <Gui/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/ViewProviderCoordinateSystem.h>


namespace PartDesignGui
{

class SoSwitch;
class Ui_TaskFeaturePick;
class TaskFeaturePick: public Gui::TaskView::TaskBox,
                       public Gui::SelectionObserver,
                       public Gui::DocumentObserver
{
    Q_OBJECT

public:
    enum featureStatus
    {
        validFeature = 0,
        invalidShape,
        noWire,
        isUsed,
        otherBody,
        otherPart,
        notInBody,
        basePlane,
        afterTip
    };

    TaskFeaturePick(
        std::vector<App::DocumentObject*>& objects,
        const std::vector<featureStatus>& status,
        bool singleFeatureSelect,
        QWidget* parent = nullptr
    );

    ~TaskFeaturePick() override;

    std::vector<App::DocumentObject*> getFeatures();
    std::vector<App::DocumentObject*> buildFeatures();
    void showExternal(bool val);
    bool isSingleSelectionEnabled() const;

    static App::DocumentObject* makeCopy(App::DocumentObject* obj, std::string sub, bool independent);

protected Q_SLOTS:
    void onUpdate(bool);
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void onItemSelectionChanged();
    void onDoubleClick(QListWidgetItem* item);

protected:
    /** Notifies when the object is about to be removed. */
    void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj) override;
    /** Notifies on undo */
    void slotUndoDocument(const Gui::Document& Doc) override;
    /** Notifies on document deletion */
    void slotDeleteDocument(const Gui::Document& Doc) override;

private:
    std::unique_ptr<Ui_TaskFeaturePick> ui;
    QWidget* proxy;
    std::vector<Gui::ViewProviderCoordinateSystem*> origins;
    bool doSelection;
    std::string documentName;

    std::vector<QString> features;
    std::vector<featureStatus> statuses;

    void updateList();
    const QString getFeatureStatusString(const featureStatus st);
};


/// simulation dialog for the TaskView
class TaskDlgFeaturePick: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgFeaturePick(
        std::vector<App::DocumentObject*>& objects,
        const std::vector<TaskFeaturePick::featureStatus>& status,
        std::function<bool(std::vector<App::DocumentObject*>)> acceptfunc,
        std::function<void(std::vector<App::DocumentObject*>)> workfunc,
        bool singleFeatureSelect,
        std::function<void(void)> abortfunc = 0
    );
    ~TaskDlgFeaturePick() override;

public:
    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user presses the help button
    bool isAllowedAlterDocument() const override
    {
        return false;
    }

    void showExternal(bool val);

    /// returns for Close and Help button
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }


protected:
    TaskFeaturePick* pick;
    bool accepted;
    std::function<bool(std::vector<App::DocumentObject*>)> acceptFunction;
    std::function<void(std::vector<App::DocumentObject*>)> workFunction;
    std::function<void(void)> abortFunction;
};

}  // namespace PartDesignGui
