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

#include "ComboView.h"
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


/* TRANSLATOR Gui::DockWnd::ComboView */

ComboView::ComboView(bool showModel, Gui::Document* pcDocument, QWidget *parent)
  : DockWindow(pcDocument,parent)
  , oldTabIndex(0)
  , modelIndex(-1)
  , taskIndex(-1)
{
    setWindowTitle(tr("Combo View"));

    QGridLayout* pLayout = new QGridLayout(this);
    pLayout->setSpacing( 0 );
    pLayout->setMargin ( 0 );

    // tabs to switch between Tree/Properties and TaskPanel
    tabs = new QTabWidget ();
    tabs->setObjectName(QString::fromUtf8("combiTab"));
    tabs->setTabPosition(QTabWidget::North);
    pLayout->addWidget( tabs, 0, 0 );

    connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(onCurrentTabChanged(int)));
    if (showModel) {
        // splitter between tree and property view
        QSplitter *splitter = new QSplitter();
        splitter->setOrientation(Qt::Vertical);

        tree =  new TreePanel("ComboView", this);
        splitter->addWidget(tree);

        // property view
        prop = new PropertyView(this);
        splitter->addWidget(prop);
        modelIndex = tabs->addTab(splitter,tr("Model"));
    }
    else {
        tree = nullptr;
        prop = nullptr;
    }

    // task panel
    taskPanel = new Gui::TaskView::TaskView(this);
    taskIndex = tabs->addTab(taskPanel, tr("Tasks"));

    // task panel
    //projectView = new Gui::ProjectWidget(this);
    //tabs->addTab(projectView, tr("Project"));
}

ComboView::~ComboView()
{
}

void ComboView::showDialog(Gui::TaskView::TaskDialog *dlg)
{
    static QIcon icon = Gui::BitmapFactory().pixmap("edit-edit.svg");

    // switch to the TaskView tab
    oldTabIndex = tabs->currentIndex();
    tabs->setCurrentIndex(taskIndex);
    tabs->setTabIcon(taskIndex, icon);
    // set the dialog
    taskPanel->showDialog(dlg);

    // force to show the combo view
    if (modelIndex < 0) {
        if (parentWidget())
            parentWidget()->raise();
    }
}

void ComboView::closeDialog()
{
    // close the dialog
    taskPanel->removeDialog();
}

void ComboView::closedDialog()
{
    static QIcon icon = QIcon();

    // dialog has been closed
    tabs->setCurrentIndex(oldTabIndex);
    tabs->setTabIcon(taskIndex, icon);
}

void ComboView::showTreeView()
{
    // switch to the tree view
    tabs->setCurrentIndex(modelIndex);
}

void ComboView::showTaskView()
{
    // switch to the task view
    tabs->setCurrentIndex(taskIndex);
}

void ComboView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        tabs->setTabText(modelIndex, tr("Model"));
        tabs->setTabText(taskIndex, tr("Tasks"));
        //tabs->setTabText(2, tr("Project"));
    }

    DockWindow::changeEvent(e);
}

void ComboView::onCurrentTabChanged(int index)
{
    if (index != taskIndex)
        oldTabIndex = index;
}

#include "moc_ComboView.cpp"
