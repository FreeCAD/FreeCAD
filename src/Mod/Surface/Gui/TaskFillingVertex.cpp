/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QTimer>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/SelectionObject.h>
#include <Gui/Widgets.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "TaskFilling.h"
#include "TaskFillingVertex.h"
#include "ui_TaskFillingVertex.h"


using namespace SurfaceGui;

namespace SurfaceGui
{

class FillingVertexPanel::VertexSelection: public Gui::SelectionFilterGate
{
public:
    VertexSelection(FillingVertexPanel::SelectionMode& mode, Surface::Filling* editedObject)
        : Gui::SelectionFilterGate(nullPointer())
        , mode(mode)
        , editedObject(editedObject)
    {}
    ~VertexSelection() override
    {
        mode = FillingVertexPanel::None;
    }
    /**
     * Allow the user to pick only edges.
     */
    bool allow(App::Document*, App::DocumentObject* pObj, const char* sSubName) override
    {
        // don't allow references to itself
        if (pObj == editedObject) {
            return false;
        }
        if (!pObj->isDerivedFrom(Part::Feature::getClassTypeId())) {
            return false;
        }

        if (!sSubName || sSubName[0] == '\0') {
            return false;
        }

        switch (mode) {
            case FillingVertexPanel::AppendVertex:
                return allowVertex(true, pObj, sSubName);
            case FillingVertexPanel::RemoveVertex:
                return allowVertex(false, pObj, sSubName);
            default:
                return false;
        }
    }

private:
    bool allowVertex(bool appendVertex, App::DocumentObject* pObj, const char* sSubName)
    {
        std::string element(sSubName);
        if (element.substr(0, 6) != "Vertex") {
            return false;
        }

        auto links = editedObject->Points.getSubListValues();
        for (const auto& it : links) {
            if (it.first == pObj) {
                for (const auto& jt : it.second) {
                    if (jt == sSubName) {
                        return !appendVertex;
                    }
                }
            }
        }

        return appendVertex;
    }

private:
    FillingVertexPanel::SelectionMode& mode;
    Surface::Filling* editedObject;
};

// ----------------------------------------------------------------------------

FillingVertexPanel::FillingVertexPanel(ViewProviderFilling* vp, Surface::Filling* obj)
{
    ui = new Ui_TaskFillingVertex();
    ui->setupUi(this);
    setupConnections();

    selectionMode = None;
    this->vp = vp;
    checkCommand = true;
    setEditedObject(obj);

    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    action->setShortcut(QString::fromLatin1("Del"));
    action->setShortcutContext(Qt::WidgetShortcut);
    ui->listFreeVertex->addAction(action);
    connect(action, &QAction::triggered, this, &FillingVertexPanel::onDeleteVertex);
    ui->listFreeVertex->setContextMenuPolicy(Qt::ActionsContextMenu);
}

/*
 *  Destroys the object and frees any allocated resources
 */
FillingVertexPanel::~FillingVertexPanel()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
    Gui::Selection().rmvSelectionGate();
}

void FillingVertexPanel::setupConnections()
{
    connect(ui->buttonVertexAdd,
            &QToolButton::toggled,
            this,
            &FillingVertexPanel::onButtonVertexAddToggled);
    connect(ui->buttonVertexRemove,
            &QToolButton::toggled,
            this,
            &FillingVertexPanel::onButtonVertexRemoveToggled);
}

void FillingVertexPanel::appendButtons(Gui::ButtonGroup* buttonGroup)
{
    buttonGroup->addButton(ui->buttonVertexAdd, int(SelectionMode::AppendVertex));
    buttonGroup->addButton(ui->buttonVertexRemove, int(SelectionMode::RemoveVertex));
}

// stores object pointer, its old fill type and adjusts radio buttons according to it.
void FillingVertexPanel::setEditedObject(Surface::Filling* obj)
{
    editedObject = obj;

    auto objects = editedObject->Points.getValues();
    auto element = editedObject->Points.getSubValues();
    auto it = objects.begin();
    auto jt = element.begin();

    App::Document* doc = editedObject->getDocument();
    for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
        QListWidgetItem* item = new QListWidgetItem(ui->listFreeVertex);
        ui->listFreeVertex->addItem(item);

        QString text = QString::fromLatin1("%1.%2").arg(QString::fromUtf8((*it)->Label.getValue()),
                                                        QString::fromStdString(*jt));
        item->setText(text);

        QList<QVariant> data;
        data << QByteArray(doc->getName());
        data << QByteArray((*it)->getNameInDocument());
        data << QByteArray(jt->c_str());
        item->setData(Qt::UserRole, data);
    }
    attachDocument(Gui::Application::Instance->getDocument(doc));
}

void FillingVertexPanel::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void FillingVertexPanel::open()
{
    checkOpenCommand();
    this->vp->highlightReferences(ViewProviderFilling::Vertex,
                                  editedObject->Points.getSubListValues(),
                                  true);
    Gui::Selection().clearSelection();
}

void FillingVertexPanel::reject()
{
    this->vp->highlightReferences(ViewProviderFilling::Vertex,
                                  editedObject->Points.getSubListValues(),
                                  false);
}

void FillingVertexPanel::clearSelection()
{
    Gui::Selection().clearSelection();
}

void FillingVertexPanel::checkOpenCommand()
{
    if (checkCommand && !Gui::Command::hasPendingCommand()) {
        std::string Msg("Edit ");
        Msg += editedObject->Label.getValue();
        Gui::Command::openCommand(Msg.c_str());
        checkCommand = false;
    }
}

void FillingVertexPanel::slotUndoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void FillingVertexPanel::slotRedoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void FillingVertexPanel::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    // If this view provider is being deleted then reset the colors of
    // referenced part objects. The dialog will be deleted later.
    if (this->vp == &Obj) {
        this->vp->highlightReferences(ViewProviderFilling::Vertex,
                                      editedObject->Points.getSubListValues(),
                                      false);
    }
}

void FillingVertexPanel::onButtonVertexAddToggled(bool checked)
{
    if (checked) {
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(new VertexSelection(selectionMode, editedObject));
        selectionMode = AppendVertex;
    }
    else if (selectionMode == AppendVertex) {
        exitSelectionMode();
    }
}

void FillingVertexPanel::onButtonVertexRemoveToggled(bool checked)
{
    if (checked) {
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(new VertexSelection(selectionMode, editedObject));
        selectionMode = RemoveVertex;
    }
    else if (selectionMode == RemoveVertex) {
        exitSelectionMode();
    }
}

void FillingVertexPanel::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == None) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        checkOpenCommand();
        if (selectionMode == AppendVertex) {
            QListWidgetItem* item = new QListWidgetItem(ui->listFreeVertex);
            ui->listFreeVertex->addItem(item);

            Gui::SelectionObject sel(msg);
            QString text = QString::fromLatin1("%1.%2").arg(
                QString::fromUtf8(sel.getObject()->Label.getValue()),
                QString::fromLatin1(msg.pSubName));
            item->setText(text);

            QList<QVariant> data;
            data << QByteArray(msg.pDocName);
            data << QByteArray(msg.pObjectName);
            data << QByteArray(msg.pSubName);
            item->setData(Qt::UserRole, data);

            auto objects = editedObject->Points.getValues();
            objects.push_back(sel.getObject());
            auto element = editedObject->Points.getSubValues();
            element.emplace_back(msg.pSubName);
            editedObject->Points.setValues(objects, element);
            this->vp->highlightReferences(ViewProviderFilling::Vertex,
                                          editedObject->Points.getSubListValues(),
                                          true);
        }
        else if (selectionMode == RemoveVertex) {
            Gui::SelectionObject sel(msg);
            QList<QVariant> data;
            data << QByteArray(msg.pDocName);
            data << QByteArray(msg.pObjectName);
            data << QByteArray(msg.pSubName);
            for (int i = 0; i < ui->listFreeVertex->count(); i++) {
                QListWidgetItem* item = ui->listFreeVertex->item(i);
                if (item && item->data(Qt::UserRole) == data) {
                    ui->listFreeVertex->takeItem(i);
                    delete item;
                }
            }

            this->vp->highlightReferences(ViewProviderFilling::Vertex,
                                          editedObject->Points.getSubListValues(),
                                          false);
            App::DocumentObject* obj = sel.getObject();
            std::string sub = msg.pSubName;
            auto objects = editedObject->Points.getValues();
            auto element = editedObject->Points.getSubValues();
            auto it = objects.begin();
            auto jt = element.begin();
            for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
                if (*it == obj && *jt == sub) {
                    objects.erase(it);
                    element.erase(jt);
                    editedObject->Points.setValues(objects, element);
                    break;
                }
            }
            this->vp->highlightReferences(ViewProviderFilling::Vertex,
                                          editedObject->Points.getSubListValues(),
                                          true);
        }

        editedObject->recomputeFeature();
        QTimer::singleShot(50, this, &FillingVertexPanel::clearSelection);
    }
}

void FillingVertexPanel::onDeleteVertex()
{
    int row = ui->listFreeVertex->currentRow();
    QListWidgetItem* item = ui->listFreeVertex->item(row);
    if (item) {
        checkOpenCommand();
        QList<QVariant> data;
        data = item->data(Qt::UserRole).toList();
        ui->listFreeVertex->takeItem(row);
        delete item;

        App::Document* doc = App::GetApplication().getDocument(data[0].toByteArray());
        App::DocumentObject* obj = doc ? doc->getObject(data[1].toByteArray()) : nullptr;
        std::string sub = data[2].toByteArray().constData();
        auto objects = editedObject->Points.getValues();
        auto element = editedObject->Points.getSubValues();
        auto it = objects.begin();
        auto jt = element.begin();
        this->vp->highlightReferences(ViewProviderFilling::Vertex,
                                      editedObject->Points.getSubListValues(),
                                      false);

        for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
            if (*it == obj && *jt == sub) {
                objects.erase(it);
                element.erase(jt);
                editedObject->Points.setValues(objects, element);
                editedObject->recomputeFeature();
                break;
            }
        }

        this->vp->highlightReferences(ViewProviderFilling::Vertex,
                                      editedObject->Points.getSubListValues(),
                                      true);
    }
}

void FillingVertexPanel::exitSelectionMode()
{
    // 'selectionMode' is passed by reference to the filter and changed when the filter is deleted
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
}

}  // namespace SurfaceGui

#include "moc_TaskFillingVertex.cpp"
