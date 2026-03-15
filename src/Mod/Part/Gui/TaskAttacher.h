// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#pragma once

#include <Gui/Selection/Selection.h>
#include <Gui/DocumentObserver.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Mod/Part/App/Attacher.h>
#include <App/PropertyOverrides.h>

#include <functional>

#include <Mod/Part/PartGlobal.h>

class Ui_TaskAttacher;
class QLineEdit;

namespace App
{
class Property;
}

namespace Gui
{
class ViewProviderPlane;
class ViewProvider;
}  // namespace Gui

namespace PartGui
{

class Ui_TaskAttacher;


class PartGuiExport TaskAttacher: public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    using VisibilityFunction = std::function<
        void(bool, const std::string&, Gui::ViewProviderDocumentObject*, App::DocumentObject*, const std::string&)>;

    explicit TaskAttacher(
        Gui::ViewProviderDocumentObject* ViewProvider,
        QWidget* parent,
        QString picture,
        QString text,
        VisibilityFunction func = 0
    );
    ~TaskAttacher() override;

    bool getFlip() const;

    /**
     * @brief getActiveMapMode returns either the default mode for selected
     * references, or the mode that was selected by the user in the list. If
     * no modes fit current set of references, mmDeactivated is returned.
     */
    Attacher::eMapMode getActiveMapMode();

    bool isCompleted() const
    {
        return completed;
    }

Q_SIGNALS:
    void placementUpdated();

private Q_SLOTS:
    void onAttachmentOffsetChanged(double, int idx);
    void onAttachmentOffsetXChanged(double);
    void onAttachmentOffsetYChanged(double);
    void onAttachmentOffsetZChanged(double);
    void onAttachmentOffsetYawChanged(double);
    void onAttachmentOffsetPitchChanged(double);
    void onAttachmentOffsetRollChanged(double);
    void onCheckFlip(bool);
    void onRefName1(const QString& text);
    void onRefName2(const QString& text);
    void onRefName3(const QString& text);
    void onRefName4(const QString& text);
    void onButtonRef1(const bool checked = true);
    void onButtonRef2(const bool checked = true);
    void onButtonRef3(const bool checked = true);
    void onButtonRef4(const bool checked = true);
    void onModeSelect();
    void visibilityAutomation(bool opening_not_closing);

protected:
    void changeEvent(QEvent* e) override;

private:
    void objectDeleted(const Gui::ViewProviderDocumentObject&);
    void documentDeleted(const Gui::Document&);
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void updateReferencesUI();

    /**
     * @brief updatePreview: calculate attachment, update 3d view, update status message
     * @return true if attachment calculation was successful, false otherwise
     */
    bool updatePreview();

    void makeRefStrings(std::vector<QString>& refstrings, std::vector<std::string>& refnames);
    QLineEdit* getLine(unsigned idx);
    void onButtonRef(const bool checked, unsigned idx);
    void onRefName(const QString& text, unsigned idx);
    void updateRefButton(int idx);
    void updateAttachmentOffsetUI();

    void findCorrectObjAndSubInThisContext(App::DocumentObject*& obj, std::string& sub);
    void handleInitialSelection();
    struct SubAndObjName
    {
        std::string objName;
        std::string subName;
    };
    void addToReference(SubAndObjName pair);
    void addToReference(const std::vector<SubAndObjName>& pairs);

    /**
     * @brief updateListOfModes Fills the mode list with modes that apply to
     * current set of references. Maintains selection when possible.
     */
    void updateListOfModes();

    /**
     * @brief selectMapMode Select the given mode in the list widget
     */
    void selectMapMode(Attacher::eMapMode mmode);

    void showPlacementUtilities();

protected:
    Gui::ViewProviderDocumentObject* ViewProvider;
    std::string ObjectName;

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskAttacher> ui;
    VisibilityFunction visibilityFunc;

    // TODO fix documentation here (2015-11-10, Fat-Zer)
    int iActiveRef;  // what reference is being picked in 3d view now? -1 means no one, 0-3 means a
                     // reference is being picked.
    bool autoNext;   // if we should automatically switch to next reference (true after dialog
                     // launch, false afterwards)
    std::vector<Attacher::eMapMode> modesInList;  // this list is synchronous to what is populated
                                                  // into listOfModes widget.
    Attacher::SuggestResult lastSuggestResult;
    bool completed;

    using Connection = fastsignals::connection;
    Connection connectDelObject;
    Connection connectDelDocument;

    std::vector<Gui::ViewProviderWeakPtrT> modifiedPlaneViewProviders;

    App::PropertyOverrideContext overrides;
};

/// simulation dialog for the TaskView
class PartGuiExport TaskDlgAttacher: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgAttacher(
        Gui::ViewProviderDocumentObject* ViewProvider,
        bool createBox = true,
        std::function<void()> onAccept = {},
        std::function<void()> onReject = {}
    );
    ~TaskDlgAttacher() override;

    Gui::ViewProviderDocumentObject* getViewProvider() const
    {
        return ViewProvider;
    }


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

    /// returns for Close and Help button
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

protected:
    Gui::ViewProviderDocumentObject* ViewProvider;

    TaskAttacher* parameter;

    std::function<void()> onAccept;
    std::function<void()> onReject;
    bool accepted;
};

}  // namespace PartGui
