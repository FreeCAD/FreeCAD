// SPDX-License-Identifier: LGPL-2.1-or-later
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


#pragma once

#include <string>
#include <vector>

#include <QDialogButtonBox>
#include <QPointer>
#include <FCGlobal.h>


namespace App
{
class DocumentObject;
}

namespace Gui
{
class MDIView;
namespace TaskView
{

class TaskContent;
class TaskDialogAttorney;
class TaskDialogPy;
class TaskView;

/// Father class of content with header and Icon
class GuiExport TaskDialog: public QObject
{
    Q_OBJECT

public:
    enum ButtonPosition
    {
        North,
        South
    };

    TaskDialog();
    ~TaskDialog() override;

    QWidget* addTaskBox(QWidget* widget, bool expandable = true, QWidget* parent = nullptr);
    QWidget* addTaskBox(
        const QPixmap& icon,
        QWidget* widget,
        bool expandable = true,
        QWidget* parent = nullptr
    );
    QWidget* addTaskBoxWithoutHeader(QWidget* widget);

    void setButtonPosition(ButtonPosition p)
    {
        pos = p;
    }
    ButtonPosition buttonPosition() const
    {
        return pos;
    }
    const std::vector<QWidget*>& getDialogContent() const;
    bool canClose() const;

    /// tells the framework which buttons are wished for the dialog
    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }
    virtual void modifyStandardButtons(QDialogButtonBox*)
    {}

    /// Defines whether a task dialog can be rejected by pressing Esc
    void setEscapeButtonEnabled(bool on)
    {
        escapeButton = on;
    }
    bool isEscapeButtonEnabled() const
    {
        return escapeButton;
    }

    /// Defines whether a task dialog must be closed if the document changed the
    /// active transaction.
    void setAutoCloseOnTransactionChange(bool on)
    {
        autoCloseTransaction = on;
    }
    bool isAutoCloseOnTransactionChange() const
    {
        return autoCloseTransaction;
    }

    /// Defines whether a task dialog must be closed if the document is
    /// deleted.
    void setAutoCloseOnDeletedDocument(bool on)
    {
        autoCloseDeletedDocument = on;
    }
    bool isAutoCloseOnDeletedDocument() const
    {
        return autoCloseDeletedDocument;
    }

    const std::string& getDocumentName() const
    {
        return documentName;
    }
    void setDocumentName(const std::string& doc)
    {
        documentName = doc;
    }

    /// Defines whether a task dialog must be closed if the associated view
    /// is deleted.
    void setAutoCloseOnClosedView(bool on)
    {
        autoCloseClosedView = on;
    }
    bool isAutoCloseOnClosedView() const
    {
        return autoCloseClosedView;
    }
    void associateToObject3dView(App::DocumentObject* obj);

    const Gui::MDIView* getAssociatedView() const
    {
        return associatedView;
    }
    void setAssociatedView(const Gui::MDIView* view)
    {
        associatedView = view;
    }

    /*!
      Indicates whether this task dialog allows other commands to modify
      the document while it is open.
    */
    virtual bool isAllowedAlterDocument() const
    {
        return false;
    }
    /*!
      Indicates whether this task dialog allows other commands to modify
      the 3d view while it is open.
    */
    virtual bool isAllowedAlterView() const
    {
        return true;
    }
    /*!
      Indicates whether this task dialog allows other commands to modify
      the selection while it is open.
    */
    virtual bool isAllowedAlterSelection() const
    {
        return true;
    }
    virtual bool needsFullSpace() const
    {
        return false;
    }

public:
    /// is called by the framework when the dialog is opened
    virtual void open();
    /// is called by the framework when the dialog is closed
    virtual void closed();
    /// is called by the framework when the dialog is automatically closed due to
    /// changing the active transaction
    virtual void autoClosedOnTransactionChange();
    /// is called by the framework when the dialog is automatically closed due to
    /// deleting the document
    virtual void autoClosedOnDeletedDocument();
    /// is called by the framework when the dialog is automatically closed due to
    /// closing of associated view
    virtual void autoClosedOnClosedView();
    /// is called by the framework if a button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user press the help button
    virtual void helpRequested();
    /// is called by the framework if the user press the undo button
    virtual void onUndo();
    /// is called by the framework if the user press the redo button
    virtual void onRedo();

    void emitDestructionSignal()
    {
        Q_EMIT aboutToBeDestroyed();
    }

Q_SIGNALS:
    void aboutToBeDestroyed();

protected:
    QPointer<QDialogButtonBox> buttonBox;
    /// List of TaskBoxes of that dialog
    std::vector<QWidget*> Content;
    ButtonPosition pos;

private:
    std::string documentName;
    const Gui::MDIView* associatedView;
    bool escapeButton;
    bool autoCloseTransaction;
    bool autoCloseDeletedDocument;
    bool autoCloseClosedView;

    friend class TaskDialogAttorney;
};

class TaskDialogAttorney
{
private:
    static void setButtonBox(TaskDialog* dlg, QDialogButtonBox* box)
    {
        dlg->buttonBox = box;
    }
    static QDialogButtonBox* getButtonBox(TaskDialog* dlg)
    {
        return dlg->buttonBox;
    }

    friend class TaskDialogPy;
    friend class TaskView;
};

}  // namespace TaskView
}  // namespace Gui
