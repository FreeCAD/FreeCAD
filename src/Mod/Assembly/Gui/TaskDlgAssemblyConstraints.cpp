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
#endif

#include "TaskDlgAssemblyConstraints.h"
#include <Gui/Command.h>
#include <App/Application.h>

using namespace AssemblyGui;


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgAssemblyConstraints::TaskDlgAssemblyConstraints(ViewProviderConstraint* vp)
    : TaskDialog(), view(vp)
{
    Constraints  = new TaskAssemblyConstraints(vp);
    
    Content.push_back(Constraints);
}

TaskDlgAssemblyConstraints::~TaskDlgAssemblyConstraints()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgAssemblyConstraints::open()
{

}

void TaskDlgAssemblyConstraints::clicked(int)
{
    
}

bool TaskDlgAssemblyConstraints::accept()
{
    std::string document = getDocumentName(); // needed because resetEdit() deletes this instance
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.getDocument('%s').resetEdit()", document.c_str());

    return true;
}

bool TaskDlgAssemblyConstraints::reject()
{
    std::string document = getDocumentName(); // needed because resetEdit() deletes this instance
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.getDocument('%s').resetEdit()", document.c_str());

    return true;
}

void TaskDlgAssemblyConstraints::helpRequested()
{

}


#include "moc_TaskDlgAssemblyConstraints.cpp"
