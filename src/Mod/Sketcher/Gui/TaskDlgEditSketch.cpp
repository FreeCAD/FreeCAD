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
# include <boost/bind.hpp>
#endif

#include "TaskDlgEditSketch.h"
#include "ViewProviderSketch.h"
#include <Gui/Command.h>

using namespace SketcherGui;


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgEditSketch::TaskDlgEditSketch(ViewProviderSketch *sketchView)
    : TaskDialog(),sketchView(sketchView)
{
    assert(sketchView);
    Constraints  = new TaskSketcherConstrains(sketchView);
    Elements = new TaskSketcherElements(sketchView);
    General  = new TaskSketcherGeneral(sketchView);
    Messages  = new TaskSketcherMessages(sketchView);

    Content.push_back(Messages);
    Content.push_back(General);
    Content.push_back(Constraints);
    Content.push_back(Elements);

    App::Document* document = sketchView->getObject()->getDocument();
    connectUndoDocument =
        document->signalUndo.connect(boost::bind(&TaskDlgEditSketch::slotUndoDocument, this, _1));
    connectRedoDocument =
        document->signalRedo.connect(boost::bind(&TaskDlgEditSketch::slotRedoDocument, this, _1));
}

TaskDlgEditSketch::~TaskDlgEditSketch()
{
    connectUndoDocument.disconnect();
    connectRedoDocument.disconnect();
}

void TaskDlgEditSketch::slotUndoDocument(const App::Document& doc)
{
    const_cast<App::Document&>(doc).recomputeFeature(sketchView->getObject());
}

void TaskDlgEditSketch::slotRedoDocument(const App::Document& doc)
{
    const_cast<App::Document&>(doc).recomputeFeature(sketchView->getObject());
}

//==== calls from the TaskView ===============================================================


void TaskDlgEditSketch::open()
{

}

void TaskDlgEditSketch::clicked(int)
{
    
}

bool TaskDlgEditSketch::accept()
{
    return true;
}

bool TaskDlgEditSketch::reject()
{
    std::string document = getDocumentName(); // needed because resetEdit() deletes this instance
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.getDocument('%s').resetEdit()", document.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument('%s').recompute()", document.c_str());

    return true;
}

void TaskDlgEditSketch::helpRequested()
{

}


#include "moc_TaskDlgEditSketch.cpp"
