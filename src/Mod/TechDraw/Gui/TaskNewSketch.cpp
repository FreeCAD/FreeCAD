// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <algorithm>

#include <QIcon>
#include <QLabel>
#include <QMetaObject>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>

#include "MDIViewPage.h"
#include "TaskNewSketch.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;

namespace
{
QString objectLabel(const App::DocumentObject* object)
{
    if (!object) {
        return {};
    }
    const char* label = object->Label.getValue();
    return QString::fromUtf8(label && *label ? label : object->getNameInDocument());
}
}

TaskNewSketch::TaskNewSketch(TechDraw::DrawPage* page, QWidget* parent)
    : QWidget(parent)
    , m_page(page)
    , m_ownerTree(new QTreeWidget(this))
{
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Owner"), this));

    m_ownerTree->setHeaderHidden(true);
    m_ownerTree->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_ownerTree);

    if (!m_page) {
        return;
    }

    auto* pageItem = new QTreeWidgetItem(m_ownerTree, {objectLabel(m_page)});
    pageItem->setData(0, Qt::UserRole, QString());
    pageItem->setIcon(0, QIcon(Gui::BitmapFactory().pixmap("actions/TechDraw_PageDefault")));
    pageItem->setExpanded(true);
    pageItem->setSelected(true);

    for (auto* object : m_page->getAllViews()) {
        auto* view = freecad_cast<TechDraw::DrawView*>(object);
        if (view && std::find(m_views.begin(), m_views.end(), view) == m_views.end()) {
            m_views.push_back(view);
        }
    }
    addViewChildren(nullptr, pageItem);
    m_ownerTree->setCurrentItem(pageItem);
    m_ownerTree->expandAll();
}

void TaskNewSketch::addViewChildren(TechDraw::DrawView* owner, QTreeWidgetItem* parent)
{
    if (!parent) {
        return;
    }

    for (auto* view : m_views) {
        if (view->claimParent() != owner) {
            continue;
        }

        auto* item = new QTreeWidgetItem(parent, {objectLabel(view)});
        item->setData(0, Qt::UserRole, QString::fromUtf8(view->getNameInDocument()));
        if (auto* viewProvider = Gui::Application::Instance->getViewProvider(view)) {
            item->setIcon(0, viewProvider->getIcon());
        }

        addViewChildren(view, item);
    }
}

TechDraw::DrawView* TaskNewSketch::selectedOwner() const
{
    QTreeWidgetItem* item = m_ownerTree->currentItem();
    if (!item) {
        return nullptr;
    }
    const QByteArray objectName = item->data(0, Qt::UserRole).toString().toUtf8();
    if (objectName.isEmpty() || !m_page || !m_page->getDocument()) {
        return nullptr;
    }
    return freecad_cast<TechDraw::DrawView*>(
        m_page->getDocument()->getObject(objectName.constData())
    );
}

bool TaskNewSketch::createSketch()
{
    if (!m_page || !m_page->isAttachedToDocument()) {
        return false;
    }

    auto* document = m_page->getDocument();
    auto* owner = selectedOwner();
    if (owner && (!owner->isAttachedToDocument() || owner->getDocument() != document)) {
        return false;
    }

    const int transactionId = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Create a TechDraw sketch")
    );
    App::DocumentObject* sketch = nullptr;
    try {
        sketch = document->addObject("Sketcher::SketchObject", "Sketch");
        if (!sketch) {
            throw Base::RuntimeError("Could not create Sketcher::SketchObject");
        }
        sketch->Label.setValue("Sketch");
        m_page->addView(sketch, false);
        if (owner) {
            owner->addSketch(sketch);
        }
        document->recompute();
        Gui::Command::commitCommand(transactionId);
    }
    catch (const Base::Exception& error) {
        Gui::Command::abortCommand(transactionId);
        Base::Console().error("TechDraw new sketch: %s\n", error.what());
        return false;
    }

    auto* pageProvider = freecad_cast<ViewProviderPage*>(
        Gui::Application::Instance->getViewProvider(m_page)
    );
    if (!pageProvider) {
        return true;
    }
    pageProvider->show();
    auto* pageView = pageProvider->getMDIViewPage();
    if (!pageView) {
        return true;
    }

    // The owner chooser must close before Sketcher installs its own task box.
    QMetaObject::invokeMethod(pageView, [pageView, sketch, owner]() {
        pageView->editSketch(sketch, owner);
    }, Qt::QueuedConnection);
    return true;
}

TaskDlgNewSketch::TaskDlgNewSketch(TechDraw::DrawPage* page)
    : m_widget(new TaskNewSketch(page))
{
    addTaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_NewSketch"), m_widget);
}

bool TaskDlgNewSketch::accept()
{
    return m_widget->createSketch();
}

bool TaskDlgNewSketch::reject()
{
    return true;
}

#include "moc_TaskNewSketch.cpp"
