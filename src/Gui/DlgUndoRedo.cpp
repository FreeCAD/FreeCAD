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

#ifndef _PreComp_
#include <QAction>
#include <QList>
#endif

#include "DlgUndoRedo.h"
#include "Application.h"
#include "MainWindow.h"
#include "MDIView.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::UndoRedoDialog */

/**
 *  Constructs a UndoRedoDialog which is a child of 'parent', with the
 *  name 'name'.'
 */
UndoDialog::UndoDialog( QWidget* parent )
  : QMenu( parent )
{
    connect(this, &QMenu::aboutToShow, this, &UndoDialog::onFetchInfo);
}

/**
 *  Destroys the object and frees any allocated resources.
 */
UndoDialog::~UndoDialog() = default;

/**
 *  This method fetches the undo / redo information from the
 *  active document and shows it in the undo / redo dialog.
 */
void UndoDialog::onFetchInfo()
{
    clear(); // Remove first all items

    MDIView* mdi =  getMainWindow()->activeWindow();
    if (mdi) {
        QStringList vecUndos = mdi->undoActions();
        for (QStringList::Iterator i = vecUndos.begin(); i != vecUndos.end(); ++i) {
            addAction(*i, this, &UndoDialog::onSelected);
        }
    }
}

/** Closes the dialog and sends the message 'Undo' to the currently active MDI view. */
void UndoDialog::onSelected()
{
    auto a = static_cast<QAction*>(sender());
    QList<QAction*> acts = this->actions();
    for (QList<QAction*>::Iterator it = acts.begin(); it != acts.end(); ++it) {
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
    connect(this, &QMenu::aboutToShow, this, &RedoDialog::onFetchInfo);
}

/**
 *  Destroys the object and frees any allocated resources.
 */
RedoDialog::~RedoDialog() = default;

/**
 *  This method fetches the undo / redo information from the
 *  active document and shows it in the undo / redo dialog.
 */
void RedoDialog::onFetchInfo()
{
    clear(); // Remove first all items

    MDIView* mdi = getMainWindow()->activeWindow();
    if (mdi) {
        QStringList vecRedos = mdi->redoActions();
        for (QStringList::Iterator i = vecRedos.begin(); i != vecRedos.end(); ++i) {
            addAction(*i, this, &RedoDialog::onSelected);
        }
    }
}

/** Closes the dialog and sends the message 'Redo' to the currently active MDI view. */
void RedoDialog::onSelected()
{
    auto a = static_cast<QAction*>(sender());
    QList<QAction*> acts = this->actions();
    for (QList<QAction*>::Iterator it = acts.begin(); it != acts.end(); ++it) {
        Gui::Application::Instance->sendMsgToActiveView("Redo");
        if (*it == a)
            break;
    }
}

#include "moc_DlgUndoRedo.cpp"
