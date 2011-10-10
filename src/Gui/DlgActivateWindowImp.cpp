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
# include <QHeaderView>
#endif

#include "DlgActivateWindowImp.h"
#include "MainWindow.h"
#include "MDIView.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgActivateWindowImp */

/**
 *  Constructs a DlgActivateWindowImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgActivateWindowImp::DlgActivateWindowImp( QWidget* parent, Qt::WFlags fl )
  : QDialog( parent, fl )
{
    // create widgets
    setupUi(this);
    QTreeWidgetItem* active=0;
    QStringList labels; labels << tr("Windows");
    treeWidget->setHeaderLabels(labels);
    treeWidget->header()->hide();

    QList<QWidget*> windows = getMainWindow()->windows();
    if (windows.isEmpty())
    {
        this->buttonOk->setDisabled(true);
        return;
    }

    QWidget* activeWnd = getMainWindow()->activeWindow();

    for (QList<QWidget*>::ConstIterator it = windows.begin(); it != windows.end(); ++it) {
        QTreeWidgetItem* item = new QTreeWidgetItem(treeWidget);
        item->setText(0, (*it)->windowTitle());
        if ( *it == activeWnd )
            active = item;
    }

    if (active)
        treeWidget->setCurrentItem( active );
    treeWidget->setFocus();
}

/** Destroys the object and frees any allocated resources */
DlgActivateWindowImp::~DlgActivateWindowImp()
{
}

/**
 * Activates the MDI window you wish and closes the dialog.
 */
void DlgActivateWindowImp::accept()
{
    QTreeWidgetItem* item = treeWidget->currentItem();
    QList<QWidget*> windows = getMainWindow()->windows();

    if (item) {
        int index = treeWidget->indexOfTopLevelItem(item);
        getMainWindow()->setActiveWindow((MDIView*)windows.at(index));
    }

    QDialog::accept();
}

#include "moc_DlgActivateWindowImp.cpp"
