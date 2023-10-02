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
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/SelectionObject.h>
#include <Gui/Widgets.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "TaskSections.h"
#include "ui_TaskSections.h"


using namespace SurfaceGui;

PROPERTY_SOURCE(SurfaceGui::ViewProviderSections, PartGui::ViewProviderSpline)

namespace SurfaceGui
{

void ViewProviderSections::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit sections"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderSpline::setupContextMenu(menu, receiver, member);
}

bool ViewProviderSections::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this sketch the
        // object unsets and sets its edit mode without closing
        // the task panel

        Surface::Sections* obj = static_cast<Surface::Sections*>(this->getObject());

        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();

        // start the edit dialog
        if (dlg) {
            TaskSections* tDlg = qobject_cast<TaskSections*>(dlg);
            if (tDlg) {
                tDlg->setEditedObject(obj);
            }
            Gui::Control().showDialog(dlg);
        }
        else {
            Gui::Control().showDialog(new TaskSections(this, obj));
        }
        return true;
    }
    else {
        return ViewProviderSpline::setEdit(ModNum);
    }
}

void ViewProviderSections::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        QTimer::singleShot(0, &Gui::Control(), &Gui::ControlSingleton::closeDialog);
    }
    else {
        PartGui::ViewProviderSpline::unsetEdit(ModNum);
    }
}

QIcon ViewProviderSections::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Surface_Sections");
}

void ViewProviderSections::highlightReferences(ShapeType type, const References& refs, bool on)
{
    for (const auto& it : refs) {
        Part::Feature* base = dynamic_cast<Part::Feature*>(it.first);
        if (base) {
            PartGui::ViewProviderPartExt* svp = dynamic_cast<PartGui::ViewProviderPartExt*>(
                Gui::Application::Instance->getViewProvider(base));
            if (svp) {
                switch (type) {
                    case ViewProviderSections::Vertex:
                        if (on) {
                            std::vector<App::Color> colors;
                            TopTools_IndexedMapOfShape vMap;
                            TopExp::MapShapes(base->Shape.getValue(), TopAbs_VERTEX, vMap);
                            colors.resize(vMap.Extent(), svp->PointColor.getValue());

                            for (const auto& jt : it.second) {
                                // check again that the index is in range because it's possible that
                                // the sub-names are invalid
                                std::size_t idx =
                                    static_cast<std::size_t>(std::stoi(jt.substr(6)) - 1);
                                if (idx < colors.size()) {
                                    colors[idx] = App::Color(1.0, 0.0, 1.0);  // magenta
                                }
                            }

                            svp->setHighlightedPoints(colors);
                        }
                        else {
                            svp->unsetHighlightedPoints();
                        }
                        break;
                    case ViewProviderSections::Edge:
                        if (on) {
                            std::vector<App::Color> colors;
                            TopTools_IndexedMapOfShape eMap;
                            TopExp::MapShapes(base->Shape.getValue(), TopAbs_EDGE, eMap);
                            colors.resize(eMap.Extent(), svp->LineColor.getValue());

                            for (const auto& jt : it.second) {
                                std::size_t idx =
                                    static_cast<std::size_t>(std::stoi(jt.substr(4)) - 1);
                                // check again that the index is in range because it's possible that
                                // the sub-names are invalid
                                if (idx < colors.size()) {
                                    colors[idx] = App::Color(1.0, 0.0, 1.0);  // magenta
                                }
                            }

                            svp->setHighlightedEdges(colors);
                        }
                        else {
                            svp->unsetHighlightedEdges();
                        }
                        break;
                    case ViewProviderSections::Face:
                        if (on) {
                            std::vector<App::Color> colors;
                            TopTools_IndexedMapOfShape fMap;
                            TopExp::MapShapes(base->Shape.getValue(), TopAbs_FACE, fMap);
                            colors.resize(fMap.Extent(), svp->ShapeColor.getValue());

                            for (const auto& jt : it.second) {
                                std::size_t idx =
                                    static_cast<std::size_t>(std::stoi(jt.substr(4)) - 1);
                                // check again that the index is in range because it's possible that
                                // the sub-names are invalid
                                if (idx < colors.size()) {
                                    colors[idx] = App::Color(1.0, 0.0, 1.0);  // magenta
                                }
                            }

                            svp->setHighlightedFaces(colors);
                        }
                        else {
                            svp->unsetHighlightedFaces();
                        }
                        break;
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------

class SectionsPanel::ShapeSelection: public Gui::SelectionFilterGate
{
public:
    ShapeSelection(SectionsPanel::SelectionMode& mode, Surface::Sections* editedObject)
        : Gui::SelectionFilterGate(nullPointer())
        , mode(mode)
        , editedObject(editedObject)
    {}
    ~ShapeSelection() override
    {
        mode = SectionsPanel::None;
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
            case SectionsPanel::AppendEdge:
                return allowEdge(true, pObj, sSubName);
            case SectionsPanel::RemoveEdge:
                return allowEdge(false, pObj, sSubName);
            default:
                return false;
        }
    }

private:
    bool allowEdge(bool appendEdges, App::DocumentObject* pObj, const char* sSubName)
    {
        std::string element(sSubName);
        if (element.substr(0, 4) != "Edge") {
            return false;
        }

        auto links = editedObject->NSections.getSubListValues();
        for (const auto& it : links) {
            if (it.first == pObj) {
                for (const auto& jt : it.second) {
                    if (jt == sSubName) {
                        return !appendEdges;
                    }
                }
            }
        }

        return appendEdges;
    }

private:
    SectionsPanel::SelectionMode& mode;
    Surface::Sections* editedObject;
};

// ----------------------------------------------------------------------------

SectionsPanel::SectionsPanel(ViewProviderSections* vp, Surface::Sections* obj)
    : ui(new Ui_Sections())
{
    ui->setupUi(this);
    setupConnections();
    ui->statusLabel->clear();

    selectionMode = None;
    this->vp = vp;
    checkCommand = true;
    setEditedObject(obj);

    // Set up button group
    buttonGroup = new Gui::ButtonGroup(this);
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(ui->buttonEdgeAdd, (int)SelectionMode::AppendEdge);
    buttonGroup->addButton(ui->buttonEdgeRemove, (int)SelectionMode::RemoveEdge);

    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    action->setShortcut(QKeySequence::Delete);
    ui->listSections->addAction(action);
    connect(action, &QAction::triggered, this, &SectionsPanel::onDeleteEdge);
    ui->listSections->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->listSections->model(),
            &QAbstractItemModel::rowsMoved,
            this,
            &SectionsPanel::onIndexesMoved);
}

/*
 *  Destroys the object and frees any allocated resources
 */
SectionsPanel::~SectionsPanel() = default;

void SectionsPanel::setupConnections()
{
    connect(ui->buttonEdgeAdd, &QToolButton::toggled, this, &SectionsPanel::onButtonEdgeAddToggled);
    connect(ui->buttonEdgeRemove,
            &QToolButton::toggled,
            this,
            &SectionsPanel::onButtonEdgeRemoveToggled);
}

// stores object pointer, its old fill type and adjusts radio buttons according to it.
void SectionsPanel::setEditedObject(Surface::Sections* fea)
{
    editedObject = fea;

    // get the section edges
    auto objects = editedObject->NSections.getValues();
    auto edges = editedObject->NSections.getSubValues();
    auto count = objects.size();

    App::Document* doc = editedObject->getDocument();
    for (std::size_t i = 0; i < count; i++) {
        App::DocumentObject* obj = objects[i];
        std::string edge = edges[i];

        QListWidgetItem* item = new QListWidgetItem(ui->listSections);
        ui->listSections->addItem(item);

        QString text = QString::fromLatin1("%1.%2").arg(QString::fromUtf8(obj->Label.getValue()),
                                                        QString::fromStdString(edge));
        item->setText(text);

        // The user data field of a list widget item
        // is a list of five elementa:
        // 1. document name
        // 2. object name
        // 3. sub-element name of the edge
        QList<QVariant> data;
        data << QByteArray(doc->getName());
        data << QByteArray(obj->getNameInDocument());
        data << QByteArray(edge.c_str());
        item->setData(Qt::UserRole, data);
    }

    // attach this document observer
    attachDocument(Gui::Application::Instance->getDocument(doc));
}

void SectionsPanel::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void SectionsPanel::open()
{
    checkOpenCommand();

    // highlight the boundary edges
    this->vp->highlightReferences(ViewProviderSections::Edge,
                                  editedObject->NSections.getSubListValues(),
                                  true);

    Gui::Selection().clearSelection();
}

void SectionsPanel::clearSelection()
{
    Gui::Selection().clearSelection();
}

void SectionsPanel::checkOpenCommand()
{
    if (checkCommand && !Gui::Command::hasPendingCommand()) {
        std::string Msg("Edit ");
        Msg += editedObject->Label.getValue();
        Gui::Command::openCommand(Msg.c_str());
        checkCommand = false;
    }
}

void SectionsPanel::slotUndoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void SectionsPanel::slotRedoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void SectionsPanel::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    // If this view provider is being deleted then reset the colors of
    // referenced part objects. The dialog will be deleted later.
    if (this->vp == &Obj) {
        this->vp->highlightReferences(ViewProviderSections::Edge,
                                      editedObject->NSections.getSubListValues(),
                                      false);
    }
}

bool SectionsPanel::accept()
{
    selectionMode = None;
    Gui::Selection().rmvSelectionGate();

    if (editedObject->mustExecute()) {
        editedObject->recomputeFeature();
    }
    if (!editedObject->isValid()) {
        QMessageBox::warning(this,
                             tr("Invalid object"),
                             QString::fromLatin1(editedObject->getStatusString()));
        return false;
    }

    this->vp->highlightReferences(ViewProviderSections::Edge,
                                  editedObject->NSections.getSubListValues(),
                                  false);

    return true;
}

bool SectionsPanel::reject()
{
    this->vp->highlightReferences(ViewProviderSections::Edge,
                                  editedObject->NSections.getSubListValues(),
                                  false);

    selectionMode = None;
    Gui::Selection().rmvSelectionGate();

    return true;
}

void SectionsPanel::onButtonEdgeAddToggled(bool checked)
{
    if (checked) {
        selectionMode = AppendEdge;
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(new ShapeSelection(selectionMode, editedObject));
    }
    else if (selectionMode == AppendEdge) {
        exitSelectionMode();
    }
}

void SectionsPanel::onButtonEdgeRemoveToggled(bool checked)
{
    if (checked) {
        selectionMode = RemoveEdge;
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(new ShapeSelection(selectionMode, editedObject));
    }
    else if (selectionMode == RemoveEdge) {
        exitSelectionMode();
    }
}

void SectionsPanel::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == None) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        checkOpenCommand();
        if (selectionMode == AppendEdge) {
            QListWidgetItem* item = new QListWidgetItem(ui->listSections);
            ui->listSections->addItem(item);

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

            appendCurve(sel.getObject(), msg.pSubName);
        }
        else if (selectionMode == RemoveEdge) {
            Gui::SelectionObject sel(msg);
            QList<QVariant> data;
            data << QByteArray(msg.pDocName);
            data << QByteArray(msg.pObjectName);
            data << QByteArray(msg.pSubName);

            // only the three first elements must match
            for (int i = 0; i < ui->listSections->count(); i++) {
                QListWidgetItem* item = ui->listSections->item(i);
                QList<QVariant> userdata = item->data(Qt::UserRole).toList();
                if (userdata.mid(0, 3) == data) {
                    ui->listSections->takeItem(i);
                    delete item;
                    break;
                }
            }

            removeCurve(sel.getObject(), msg.pSubName);
        }

        editedObject->recomputeFeature();
        QTimer::singleShot(50, this, &SectionsPanel::clearSelection);
    }
}

void SectionsPanel::onDeleteEdge()
{
    int row = ui->listSections->currentRow();
    QListWidgetItem* item = ui->listSections->takeItem(row);
    if (item) {
        checkOpenCommand();
        QList<QVariant> data;
        data = item->data(Qt::UserRole).toList();
        delete item;

        App::Document* doc = App::GetApplication().getDocument(data[0].toByteArray());
        App::DocumentObject* obj = doc ? doc->getObject(data[1].toByteArray()) : nullptr;
        std::string sub = data[2].toByteArray().constData();

        removeCurve(obj, sub);
        editedObject->recomputeFeature();
    }
}

void SectionsPanel::onIndexesMoved()
{
    QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(sender());
    if (!model) {
        return;
    }

    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> element;

    int rows = model->rowCount();
    for (int i = 0; i < rows; i++) {
        QModelIndex index = model->index(i, 0);
        QList<QVariant> data;
        data = index.data(Qt::UserRole).toList();

        App::Document* doc = App::GetApplication().getDocument(data[0].toByteArray());
        App::DocumentObject* obj = doc ? doc->getObject(data[1].toByteArray()) : nullptr;
        std::string sub = data[2].toByteArray().constData();

        objects.push_back(obj);
        element.push_back(sub);
    }

    editedObject->NSections.setValues(objects, element);
    editedObject->recomputeFeature();
}

void SectionsPanel::appendCurve(App::DocumentObject* obj, const std::string& subname)
{
    auto objects = editedObject->NSections.getValues();
    objects.push_back(obj);
    auto element = editedObject->NSections.getSubValues();
    element.push_back(subname);
    editedObject->NSections.setValues(objects, element);

    this->vp->highlightReferences(ViewProviderSections::Edge,
                                  editedObject->NSections.getSubListValues(),
                                  true);
}

void SectionsPanel::removeCurve(App::DocumentObject* obj, const std::string& subname)
{
    this->vp->highlightReferences(ViewProviderSections::Edge,
                                  editedObject->NSections.getSubListValues(),
                                  false);

    auto objects = editedObject->NSections.getValues();
    auto element = editedObject->NSections.getSubValues();

    auto it = objects.begin();
    auto jt = element.begin();
    for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
        if (*it == obj && *jt == subname) {
            objects.erase(it);
            element.erase(jt);
            editedObject->NSections.setValues(objects, element);
            break;
        }
    }
    this->vp->highlightReferences(ViewProviderSections::Edge,
                                  editedObject->NSections.getSubListValues(),
                                  true);
}

void SectionsPanel::exitSelectionMode()
{
    // 'selectionMode' is passed by reference to the filter and changed when the filter is deleted
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
}

// ----------------------------------------------------------------------------

TaskSections::TaskSections(ViewProviderSections* vp, Surface::Sections* obj)
{
    // first task box
    widget1 = new SectionsPanel(vp, obj);
    Gui::TaskView::TaskBox* taskbox1 =
        new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Surface_Sections"),
                                   widget1->windowTitle(),
                                   true,
                                   nullptr);
    taskbox1->groupLayout()->addWidget(widget1);
    Content.push_back(taskbox1);
}

void TaskSections::setEditedObject(Surface::Sections* obj)
{
    widget1->setEditedObject(obj);
}

void TaskSections::open()
{
    widget1->open();
}

bool TaskSections::accept()
{
    bool ok = widget1->accept();
    if (ok) {
        Gui::Command::commitCommand();
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        Gui::Command::updateActive();
    }

    return ok;
}

bool TaskSections::reject()
{
    bool ok = widget1->reject();
    if (ok) {
        Gui::Command::abortCommand();
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        Gui::Command::updateActive();
    }

    return ok;
}

}  // namespace SurfaceGui

#include "moc_TaskSections.cpp"
