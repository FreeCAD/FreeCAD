/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QMouseEvent>
#endif

#include "TreeView.h"
#include "Application.h"
#include "Document.h"
#include "DocumentModel.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "ViewProvider.h"

using namespace Gui;

TreeView::TreeView(QWidget* parent)
  : QTreeView(parent)
{
    setModel(new DocumentModel(this));
    QModelIndex root = this->model()->index(0,0,QModelIndex());
    this->setExpanded(root, true);
    this->setDragEnabled(true);
    this->setAcceptDrops(true);
    this->setDropIndicatorShown(false);
    this->setRootIsDecorated(false);
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->setMouseTracking(true); // needed for itemEntered() to work
}

TreeView::~TreeView() = default;

void TreeView::mouseDoubleClickEvent (QMouseEvent * event)
{
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid() || index.internalPointer() == Application::Instance)
        return;
    Base::BaseClass* item = nullptr;
    item = static_cast<Base::BaseClass*>(index.internalPointer());
    if (item->getTypeId() == Document::getClassTypeId()) {
        QTreeView::mouseDoubleClickEvent(event);
        const Gui::Document* doc = static_cast<Gui::Document*>(item);
        MDIView *view = doc->getActiveView();
        if (!view)
            return;
        getMainWindow()->setActiveWindow(view);
    }
    else if (item->getTypeId().isDerivedFrom(ViewProvider::getClassTypeId())) {
        if (!static_cast<ViewProvider*>(item)->doubleClicked())
            QTreeView::mouseDoubleClickEvent(event);
    }
}

void TreeView::rowsInserted (const QModelIndex & parent, int start, int end)
{
    QTreeView::rowsInserted(parent, start, end);
    if (parent.isValid()) {
        auto ptr = static_cast<Base::BaseClass*>(parent.internalPointer());
        // type is defined in DocumentModel.cpp
        if (ptr->getTypeId() == Base::Type::fromName("Gui::ApplicationIndex")) {
            for (int i=start; i<=end;i++) {
                QModelIndex document = this->model()->index(i, 0, parent);
                this->expand(document);
            }
        }
    }
}

#include "moc_TreeView.cpp"

