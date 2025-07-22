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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QAction>
#include <QKeyEvent>
#include <QListWidget>
#include <QMessageBox>
#include <sstream>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Tools.h>
#include <Gui/ViewProvider.h>
#include <Mod/Fem/App/FemConstraint.h>

#include "TaskFemConstraint.h"
#include "ui_TaskFemConstraint.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraint */

TaskFemConstraint::TaskFemConstraint(ViewProviderFemConstraint* ConstraintView,
                                     QWidget* parent,
                                     const char* pixmapname)
    : TaskBox(Gui::BitmapFactory().pixmap(pixmapname),
              tr("Analysis feature parameters"),
              true,
              parent)
    , proxy(nullptr)
    , actionList(nullptr)
    , clearListAction(nullptr)
    , deleteAction(nullptr)
    , ConstraintView(ConstraintView)
    , selectionMode(selref)
{}

bool TaskFemConstraint::event(QEvent* event)
{
    if (event && event->type() == QEvent::ShortcutOverride) {
        auto ke = static_cast<QKeyEvent*>(event);  // NOLINT
        if (deleteAction) {
            if (ke->matches(QKeySequence::Delete) || ke->matches(QKeySequence::Backspace)) {
                ke->accept();
            }
        }
    }
    return TaskBox::event(event);
}

void TaskFemConstraint::keyPressEvent(QKeyEvent* ke)
{
    // if we have a Del key, trigger the deleteAction
    if (ke->matches(QKeySequence::Delete) || ke->matches(QKeySequence::Backspace)) {
        if (deleteAction && deleteAction->isEnabled()) {
            ke->accept();
            deleteAction->trigger();
        }
    }

    TaskBox::keyPressEvent(ke);
}

const std::string TaskFemConstraint::getReferences(const std::vector<std::string>& items) const
{
    std::string result;
    for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); i++) {
        int pos = i->find_last_of(":");
        std::string objStr = "App.ActiveDocument." + i->substr(0, pos);
        std::string refStr = "\"" + i->substr(pos + 1) + "\"";
        result = result + (i != items.begin() ? ", " : "") + "(" + objStr + "," + refStr + ")";
    }

    return result;
}

const std::string TaskFemConstraint::getScale() const
{
    Fem::Constraint* pcConstraint = ConstraintView->getObject<Fem::Constraint>();

    return std::to_string(pcConstraint->Scale.getValue());
}

void TaskFemConstraint::setSelection(QListWidgetItem* item)
{
    // highlights the list item in the model

    // get the document name
    std::string docName = ConstraintView->getObject()->getDocument()->getName();
    // name of the item
    std::string ItemName = item->text().toStdString();
    std::string delimiter = ":";
    size_t pos = 0;
    pos = ItemName.find(delimiter);
    // the objName is the name piece before the ':' of the item name
    std::string objName = ItemName.substr(0, pos);
    // the subName is the name piece behind the ':'
    ItemName.erase(0, pos + delimiter.length());
    // clear existing selection
    Gui::Selection().clearSelection();
    // highlight the selected item
    Gui::Selection().addSelection(docName.c_str(), objName.c_str(), ItemName.c_str(), 0, 0, 0);
}

void TaskFemConstraint::onReferenceClearList()
{
    QSignalBlocker block(actionList);
    actionList->clear();
}

void TaskFemConstraint::onReferenceDeleted(const int row)
{
    Fem::Constraint* pcConstraint = ConstraintView->getObject<Fem::Constraint>();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    Objects.erase(Objects.begin() + row);
    SubElements.erase(SubElements.begin() + row);
    pcConstraint->References.setValues(Objects, SubElements);
}

void TaskFemConstraint::onButtonReference(const bool pressed)
{
    if (pressed) {
        selectionMode = selref;
    }
    else {
        selectionMode = selnone;
    }
    Gui::Selection().clearSelection();
}

const QString TaskFemConstraint::makeRefText(const std::string& objName,
                                             const std::string& subName) const
{
    return QString::fromUtf8((objName + ":" + subName).c_str());
}

const QString TaskFemConstraint::makeRefText(const App::DocumentObject* obj,
                                             const std::string& subName) const
{
    return QString::fromUtf8((std::string(obj->getNameInDocument()) + ":" + subName).c_str());
}

void TaskFemConstraint::createActions(QListWidget* parentList)
{
    actionList = parentList;
    createDeleteAction(parentList);
    createClearListAction(parentList);
}

void TaskFemConstraint::createClearListAction(QListWidget* parentList)
{
    clearListAction = new QAction(tr("Clear list"), this);
    connect(clearListAction, &QAction::triggered, this, &TaskFemConstraint::onReferenceClearList);

    parentList->addAction(clearListAction);
    parentList->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void TaskFemConstraint::createDeleteAction(QListWidget* parentList)
{
    // creates a context menu, a shortcut for it and connects it to a slot function

    deleteAction = new QAction(tr("Delete"), this);
    deleteAction->setShortcut(Gui::QtTools::deleteKeySequence());

    // display shortcut behind the context menu entry
    deleteAction->setShortcutVisibleInContextMenu(true);

    parentList->addAction(deleteAction);
    parentList->setContextMenuPolicy(Qt::ActionsContextMenu);
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraint::open()
{
    if (!Gui::Command::hasPendingCommand()) {
        const char* typeName = ConstraintView->getObject()->getTypeId().getName();
        Gui::Command::openCommand(typeName);
        ConstraintView->setVisible(true);
    }
}

bool TaskDlgFemConstraint::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();

    try {
        std::string refs = parameter->getReferences();

        if (!refs.empty()) {
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.References = [%s]",
                                    name.c_str(),
                                    refs.c_str());
        }
        else {
            QMessageBox::warning(parameter,
                                 tr("Input error"),
                                 tr("You must specify at least one reference"));
            return false;
        }

        std::string scale = parameter->getScale();
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Scale = %s",
                                name.c_str(),
                                scale.c_str());
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        if (!ConstraintView->getObject()->isValid()) {
            throw Base::RuntimeError(ConstraintView->getObject()->getStatusString());
        }
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

bool TaskDlgFemConstraint::reject()
{
    // roll back the changes
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraint.cpp"
