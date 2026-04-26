/***************************************************************************
 *   Copyright (c) 2026 Théo Veilleux-Trinh <theo.veilleux.trinh@proton.me>*
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

#include "TaskCommandLink.h"

#include "ui_TaskCommandLink.h"

#include "Application.h"
#include "Document.h"
#include "MetaTypes.h"
#include "ViewProvider.h"

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <App/Document.h>

namespace Gui
{
TaskCommandLink::TaskCommandLink()
    : ui(new Ui_TaskCommandLinkDialog())
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    ui->objectsList->header()->hide();
    ui->objectsList->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    this->groupLayout()->addWidget(proxy);

    buildObjectsList();
}
TaskCommandLink::~TaskCommandLink()
{
    delete proxy;
    delete ui;
}
std::vector<App::DocumentObject*> TaskCommandLink::selectedObjects()
{
    auto selected = ui->objectsList->selectedItems();
    std::vector<App::DocumentObject*> dst;
    dst.reserve(selected.size());

    for (auto sel : selected) {
        dst.push_back(sel->data(0, Qt::UserRole).value<App::DocumentObject*>());
    }
    return dst;
}
void processObjectsHelper(std::vector<App::DocumentObject*> objs, QTreeWidgetItem* item)
{
    for (auto obj : objs) {
        auto objItem = new QTreeWidgetItem(item);

        objItem->setText(0, obj->Label.getValue());
        objItem->setData(0, Qt::UserRole, QVariant::fromValue(obj));

        Gui::ViewProvider* vp = nullptr;
        if (auto doc = Application::Instance->getDocument(obj->getDocument())) {
            vp = doc->getViewProvider(obj);
        }
        if (vp) {
            objItem->setIcon(0, vp->getIcon());
            processObjectsHelper(vp->claimChildren(), objItem);
        }
        else {
            objItem->setIcon(0, QIcon());
        }
    }
}
void TaskCommandLink::buildObjectsList()
{
    ui->objectsList->clear();

    auto allDocuments = App::GetApplication().getDocuments();
    bool collapse = true;
    std::map<QTreeWidgetItem*, App::Document*> docItemMap;

    for (auto doc : allDocuments) {
        auto docItem = new QTreeWidgetItem();
        std::string itemName = doc->Label.getValue();

        docItem->setText(0, QString::fromStdString(itemName));
        docItem->setIcon(0, QIcon(QStringLiteral(":/icons/Document.svg")));
        docItem->setFlags(docItem->flags() & ~Qt::ItemIsSelectable);  // Can't link a whole document

        docItemMap[docItem] = doc;

        ui->objectsList->addTopLevelItem(docItem);

        processObjectsHelper(Application::Instance->getDocument(doc)->getTreeRootObjects(), docItem);

        if (collapse) {
            ui->objectsList->collapseAll();
        }
        else {
            ui->objectsList->expandToDepth(0);
        }
    }
    ui->objectsList->selectedItems();
}

// dialog

TaskCommandLinkDialog::TaskCommandLinkDialog(
    std::function<void(std::vector<App::DocumentObject*>)> executor_
)
    : executor(executor_)
{
    commandLink = new TaskCommandLink();
    Content.push_back(commandLink);
}
void TaskCommandLinkDialog::open()
{}
bool TaskCommandLinkDialog::accept()
{
    executor(commandLink->selectedObjects());
    return true;
}
}  // namespace Gui
