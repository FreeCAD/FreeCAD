/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "DlgUndoRedo.h"
#include "Application.h"
#include "MainWindow.h"
#include "Document.h"
#include "EditorView.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::UndoRedoDialog */

/**
 *  Constructs a UndoRedoDialog which is a child of 'parent', with the 
 *  name 'name'.' 
 */
UndoDialog::UndoDialog( QWidget* parent )
  : QMenu( parent )
{
    connect(this, SIGNAL(aboutToShow()), this, SLOT(onFetchInfo()));
}

/** 
 *  Destroys the object and frees any allocated resources.
 */
UndoDialog::~UndoDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/** 
 *  This method fetches the undo / redo information from the 
 *  active document and shows it in the undo / redo dialog.
 */
void UndoDialog::onFetchInfo() 
{
    clear(); // Remove first all items
    Gui::Document* pcDoc = Application::Instance->activeDocument();
    if (pcDoc)
    {
        std::vector<std::string> vecUndos = pcDoc->getUndoVector();
        for (std::vector<std::string>::iterator i=vecUndos.begin(); i!=vecUndos.end(); i++)
            addAction(QString::fromUtf8((*i).c_str()), this, SLOT(onSelected()));
    }
    else
    {
        EditorView* view = qobject_cast<EditorView*>(getMainWindow()->activeWindow());
        if (view) {
            QStringList vecUndos = view->undoActions();
            for (QStringList::Iterator i=vecUndos.begin(); i!=vecUndos.end(); i++)
                addAction(*i, this, SLOT(onSelected()));
        }
    }
}

/** Closes the dialog and sends the message 'Undo' to the currently active MDI view. */
void UndoDialog::onSelected()
{
    QAction* a = static_cast<QAction*>(sender());
    QList<QAction*> acts = this->actions();
    for (QList<QAction*>::ConstIterator it = acts.begin(); it != acts.end(); ++it) {
        Gui::Application::Instance->sendMsgToActiveView("Undo");
        if (*it == a)
            break;
    }
}

/* TRANSLATOR Gui::Dialog::RedoDialog */

/**
 *  Constructs a UndoRedoDialog which is a child of 'parent', with the 
 *  name 'name'.' 
 */
RedoDialog::RedoDialog( QWidget* parent )
  : QMenu( parent )
{
    connect(this, SIGNAL(aboutToShow()), this, SLOT(onFetchInfo()));
}

/** 
 *  Destroys the object and frees any allocated resources.
 */
RedoDialog::~RedoDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/** 
 *  This method fetches the undo / redo information from the 
 *  active document and shows it in the undo / redo dialog.
 */
void RedoDialog::onFetchInfo() 
{
    clear(); // Remove first all items
    Gui::Document* pcDoc = Application::Instance->activeDocument();
    if ( pcDoc )
    {
        std::vector<std::string> vecRedos = pcDoc->getRedoVector();
        for (std::vector<std::string>::iterator i=vecRedos.begin(); i!=vecRedos.end(); i++)
            addAction(QString::fromUtf8((*i).c_str()), this, SLOT(onSelected()));
    }
    else
    {
        EditorView* view = qobject_cast<EditorView*>(getMainWindow()->activeWindow());
        if (view) {
            QStringList vecRedos = view->redoActions();
            for (QStringList::Iterator i=vecRedos.begin(); i!=vecRedos.end(); i++)
                addAction(*i, this, SLOT(onSelected()));
        }
    }
}

/** Closes the dialog and sends the message 'Redo' to the currently active MDI view. */
void RedoDialog::onSelected()
{
    QAction* a = static_cast<QAction*>(sender());
    QList<QAction*> acts = this->actions();
    for (QList<QAction*>::ConstIterator it = acts.begin(); it != acts.end(); ++it) {
        Gui::Application::Instance->sendMsgToActiveView("Redo");
        if (*it == a)
            break;
    }
}

#include "moc_DlgUndoRedo.cpp"
