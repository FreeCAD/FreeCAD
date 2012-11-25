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

#include "TaskDialog.h"

using namespace Gui::TaskView;


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDialog::TaskDialog()
    : QObject(0), pos(North)
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

//==== calls from the TaskView ===============================================================

void TaskDialog::open()
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
