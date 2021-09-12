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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMessageBox>
#endif

#include "Control.h"
#include "TaskDialog.h"

using namespace Gui::TaskView;


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDialog::TaskDialog()
    : QObject(nullptr), pos(North)
    , escapeButton(true)
    , autoCloseTransaction(false)
{

}

TaskDialog::~TaskDialog()
{
    for (std::vector<QWidget*>::iterator it=Content.begin();it!=Content.end();++it) {
        delete *it;
        *it = 0;
    }
}

//==== Slots ===============================================================

const std::vector<QWidget*> &TaskDialog::getDialogContent(void) const
{
    return Content;
}

bool TaskDialog::canClose() const
{
    QMessageBox msgBox;
    msgBox.setText(tr("A dialog is already open in the task panel"));
    msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
        return true;
    else
        return false;
}

bool TaskDialog::tryClose()
{
    if (!canClose())
        return false;
    // Note, this tryClose() function is meant to replace lots of duplicate
    // code doing mostly the same thing, with one exception. Many code closes
    // the dialog by calling TaskDialog::reject(). This will very likely
    // trigger active transaction abortion, which may cause some object bening
    // deleted. And if unlucky, this could be the very object we are trying to
    // edit.
    //
    // What's worse, abort is not like undo. It can't be redo. The object or
    // some already performed operation is truely lost. If the user really
    // meant for rejecting, he can always manually undo.
    Gui::Control().closeDialog();
    return true;
}

//==== calls from the TaskView ===============================================================

void TaskDialog::open()
{

}

void TaskDialog::closed()
{

}

void TaskDialog::autoClosedOnTransactionChange()
{

}

void TaskDialog::clicked(int)
{

}

bool TaskDialog::accept()
{
    return true;
}

bool TaskDialog::reject()
{
    return true;
}

void TaskDialog::helpRequested()
{

}




#include "moc_TaskDialog.cpp"
