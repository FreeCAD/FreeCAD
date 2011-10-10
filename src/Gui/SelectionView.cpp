/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QVBoxLayout>
# include <QListWidget>
# include <QListWidgetItem>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <App/Document.h>

#include "SelectionView.h"
#include "Application.h"
#include "Document.h"
#include "ViewProvider.h"



using namespace Gui;
using namespace Gui::DockWnd;


/* TRANSLATOR Gui::DockWnd::SelectionView */

SelectionView::SelectionView(Gui::Document* pcDocument, QWidget *parent)
  : DockWindow(pcDocument,parent)
{
    setWindowTitle( tr( "Property View" ) );

    QVBoxLayout* pLayout = new QVBoxLayout( this ); 
    pLayout->setSpacing( 0 );
    pLayout->setMargin ( 0 );

  
    selectionView = new QListWidget(this);
    pLayout->addWidget( selectionView);
    resize( 200, 200 );

    Gui::Selection().Attach(this);
}

SelectionView::~SelectionView()
{
    Gui::Selection().Detach(this);
}

/// @cond DOXERR
void SelectionView::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                             Gui::SelectionSingleton::MessageType Reason)
{
    std::string temp;

    if (Reason.Type == SelectionChanges::AddSelection) {
        // insert the selection as item
        temp = Reason.pDocName;
        temp += ".";
        temp += Reason.pObjectName;
        if (Reason.pSubName[0] != 0 ) {
            temp += ".";
            temp += Reason.pSubName;
        }

        new QListWidgetItem(QString::fromAscii(temp.c_str()), selectionView);
    }
    else if (Reason.Type == SelectionChanges::ClrSelection) {
        // remove all items
        selectionView->clear();
    }
    else if (Reason.Type == SelectionChanges::RmvSelection) {
        // build name
        temp = Reason.pDocName;
        temp += ".";
        temp += Reason.pObjectName;
        if (Reason.pSubName[0] != 0) {
            temp += ".";
            temp += Reason.pSubName;
        }

        // remove all items
        QList<QListWidgetItem *> l = selectionView->findItems(QLatin1String(temp.c_str()),Qt::MatchExactly);
        if (l.size() == 1)
            delete l[0];

    }
    else if (Reason.Type == SelectionChanges::SetSelection) {
        // remove all items
        selectionView->clear();
        std::vector<SelectionSingleton::SelObj> objs = Gui::Selection().getSelection(Reason.pDocName);
        for (std::vector<SelectionSingleton::SelObj>::iterator it = objs.begin(); it != objs.end(); ++it) {
            // build name
            temp = it->DocName;
            temp += ".";
            temp += it->FeatName;
            if (it->SubName && it->SubName[0] != '\0') {
                temp += ".";
                temp += it->SubName;
            }
            new QListWidgetItem(QString::fromAscii(temp.c_str()), selectionView);
        }
    }
}

void SelectionView::onUpdate(void)
{
}

bool SelectionView::onMsg(const char* pMsg)
{
    // no messages yet
    return false;
}
/// @endcond

#include "moc_SelectionView.cpp"
