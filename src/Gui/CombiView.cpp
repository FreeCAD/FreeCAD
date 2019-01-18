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
# include <QSplitter>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......

#include "CombiView.h"
#include "BitmapFactory.h"
#include "PropertyView.h"
#include "ProjectView.h"
#include "Application.h"
#include "Document.h"
#include "Tree.h"
#include "TaskView/TaskView.h"
#include "propertyeditor/PropertyEditor.h"

using namespace Gui;
using namespace Gui::DockWnd;


/* TRANSLATOR Gui::DockWnd::CombiView */

CombiView::CombiView(Gui::Document* pcDocument, QWidget *parent)
  : DockWindow(pcDocument,parent), oldTabIndex(0)
{
    setWindowTitle(tr("CombiView"));

    QGridLayout* pLayout = new QGridLayout(this); 
    pLayout->setSpacing( 0 );
    pLayout->setMargin ( 0 );

    // tabs to switch between Tree/Properties and TaskPanel
    tabs = new QTabWidget ();
    tabs->setObjectName(QString::fromUtf8("combiTab"));
    tabs->setTabPosition(QTabWidget::North);
    pLayout->addWidget( tabs, 0, 0 );

    // splitter between tree and property view
    QSplitter *splitter = new QSplitter();
    splitter->setOrientation(Qt::Vertical);

    // tree widget
    tree =  new TreePanel(this);
    splitter->addWidget(tree);

    // property view
    prop = new PropertyView(this);
    splitter->addWidget(prop);
    tabs->addTab(splitter,trUtf8("Model"));

    // task panel
    taskPanel = new Gui::TaskView::TaskView(this);
    tabs->addTab(taskPanel, trUtf8("Tasks"));

    // task panel
    //projectView = new Gui::ProjectWidget(this);
    //tabs->addTab(projectView, trUtf8("Project"));
}

CombiView::~CombiView()
{
}

void CombiView::showDialog(Gui::TaskView::TaskDialog *dlg)
{
    static QIcon icon = Gui::BitmapFactory().pixmap("edit-edit.svg");

    // switch to the TaskView tab
    oldTabIndex = tabs->currentIndex();
    tabs->setCurrentIndex(1);
    tabs->setTabIcon(1, icon);
    // set the dialog
    taskPanel->showDialog(dlg);
}

void CombiView::closeDialog()
{
    // close the dialog
    taskPanel->removeDialog();
}

void CombiView::closedDialog()
{
    static QIcon icon = QIcon();

    // dialog has been closed
    tabs->setCurrentIndex(oldTabIndex);
    tabs->setTabIcon(1, icon);
}

void CombiView::showTreeView()
{
    // switch to the tree view
    tabs->setCurrentIndex(0);
}

void CombiView::showTaskView()
{
    // switch to the task view
    tabs->setCurrentIndex(1);
}

void CombiView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        tabs->setTabText(0, trUtf8("Model"));
        tabs->setTabText(1, trUtf8("Tasks"));
        //tabs->setTabText(2, trUtf8("Project"));
    }

    DockWindow::changeEvent(e);
}


#include "moc_CombiView.cpp"
