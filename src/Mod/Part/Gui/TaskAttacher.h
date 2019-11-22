/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>        *
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


#ifndef GUI_TASKVIEW_TaskAttacher_H
#define GUI_TASKVIEW_TaskAttacher_H

#include <Gui/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Mod/Part/App/Attacher.h>


class Ui_TaskAttacher;
class QLineEdit;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartGui { 

class Ui_TaskAttacher;

class PartGuiExport TaskAttacher : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskAttacher(Gui::ViewProviderDocumentObject *ViewProvider,QWidget *parent = 0, 
        QString picture = QString::fromLatin1(""), QString text = QString::fromLatin1("Attachment"));
    ~TaskAttacher();

    bool   getFlip(void) const;

    /**
     * @brief getActiveMapMode returns either the default mode for selected
     * references, or the mode that was selected by the user in the list. If
     * no modes fit current set of references, mmDeactivated is returned.
     */
    Attacher::eMapMode getActiveMapMode();

    bool isCompleted() const { return completed; }

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
    void onModeSelect(void);
    void visibilityAutomation(bool opening_not_closing);

protected:
    void changeEvent(QEvent *e) override;
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

    /**
     * @brief updateListOfModes Fills the mode list with modes that apply to
     * current set of references. Maintains selection when possible.
     */
    void updateListOfModes();
    
    /**
     * @brief selectMapMode Select the given mode in the list widget
     */
    void selectMapMode(Attacher::eMapMode mmode);

protected:
    Gui::ViewProviderDocumentObject *ViewProvider;

private:
    QWidget* proxy;
    Ui_TaskAttacher* ui;

    // TODO fix documentation here (2015-11-10, Fat-Zer)
    int iActiveRef; //what reference is being picked in 3d view now? -1 means no one, 0-3 means a reference is being picked.
    bool autoNext;//if we should automatically switch to next reference (true after dialog launch, false afterwards)
    std::vector<Attacher::eMapMode> modesInList; //this list is synchronous to what is populated into listOfModes widget.
    Attacher::SuggestResult lastSuggestResult;
    bool completed;

    typedef boost::signals2::connection Connection;
    Connection connectDelObject;
    Connection connectDelDocument;
};

/// simulation dialog for the TaskView
class PartGuiExport TaskDlgAttacher : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgAttacher(Gui::ViewProviderDocumentObject *ViewProvider, bool createBox = true);
    ~TaskDlgAttacher();

    Gui::ViewProviderDocumentObject* getViewProvider() const
    { return ViewProvider; }


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
    Gui::ViewProviderDocumentObject   *ViewProvider;

    TaskAttacher  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
