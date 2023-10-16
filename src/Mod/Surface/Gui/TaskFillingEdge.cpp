/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2017 Christophe Grellier <cg[at]grellier.fr>            *
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
#include <QMessageBox>
#include <QTimer>

#include <GeomAbs_Shape.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/SelectionObject.h>
#include <Gui/Widgets.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "TaskFilling.h"
#include "TaskFillingEdge.h"
#include "ui_TaskFillingEdge.h"


using namespace SurfaceGui;

namespace SurfaceGui
{

class FillingEdgePanel::ShapeSelection: public Gui::SelectionFilterGate
{
public:
    ShapeSelection(FillingEdgePanel::SelectionMode& mode, Surface::Filling* editedObject)
        : Gui::SelectionFilterGate(nullPointer())
        , mode(mode)
        , editedObject(editedObject)
    {}
    ~ShapeSelection() override
    {
        mode = FillingEdgePanel::None;
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
            case FillingEdgePanel::AppendEdge:
                return allowEdge(true, pObj, sSubName);
            case FillingEdgePanel::RemoveEdge:
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

        auto links = editedObject->UnboundEdges.getSubListValues();
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
    FillingEdgePanel::SelectionMode& mode;
    Surface::Filling* editedObject;
};

// ----------------------------------------------------------------------------

FillingEdgePanel::FillingEdgePanel(ViewProviderFilling* vp, Surface::Filling* obj)
{
    ui = new Ui_TaskFillingEdge();
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
    ui->listUnbound->addAction(action);
    connect(action, &QAction::triggered, this, &FillingEdgePanel::onDeleteUnboundEdge);
    ui->listUnbound->setContextMenuPolicy(Qt::ActionsContextMenu);
}

/*
 *  Destroys the object and frees any allocated resources
 */
FillingEdgePanel::~FillingEdgePanel()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
    Gui::Selection().rmvSelectionGate();
}

void FillingEdgePanel::setupConnections()
{
    connect(ui->buttonUnboundEdgeAdd,
            &QToolButton::toggled,
            this,
            &FillingEdgePanel::onButtonUnboundEdgeAddToggled);
    connect(ui->buttonUnboundEdgeRemove,
            &QToolButton::toggled,
            this,
            &FillingEdgePanel::onButtonUnboundEdgeRemoveToggled);
    connect(ui->listUnbound,
            &QListWidget::itemDoubleClicked,
            this,
            &FillingEdgePanel::onListUnboundItemDoubleClicked);
    connect(ui->buttonUnboundAccept,
            &QPushButton::clicked,
            this,
            &FillingEdgePanel::onButtonUnboundAcceptClicked);
    connect(ui->buttonUnboundIgnore,
            &QPushButton::clicked,
            this,
            &FillingEdgePanel::onButtonUnboundIgnoreClicked);
}

void FillingEdgePanel::appendButtons(Gui::ButtonGroup* buttonGroup)
{
    buttonGroup->addButton(ui->buttonUnboundEdgeAdd, int(SelectionMode::AppendEdge));
    buttonGroup->addButton(ui->buttonUnboundEdgeRemove, int(SelectionMode::RemoveEdge));
}

// stores object pointer, its old fill type and adjusts radio buttons according to it.
void FillingEdgePanel::setEditedObject(Surface::Filling* fea)
{
    editedObject = fea;

    // get the free edges, if set their adjacent faces and continuities
    auto objects = editedObject->UnboundEdges.getValues();
    auto edges = editedObject->UnboundEdges.getSubValues();
    auto count = objects.size();

    // fill up faces if wrong size
    auto faces = editedObject->UnboundFaces.getValues();
    if (faces.size() != edges.size()) {
        faces.resize(edges.size());
        std::fill(faces.begin(), faces.end(), std::string());
    }

    // fill up continuities if wrong size
    auto conts = editedObject->UnboundOrder.getValues();
    if (edges.size() != conts.size()) {
        conts.resize(edges.size());
        std::fill(conts.begin(), conts.end(), static_cast<long>(GeomAbs_C0));
    }

    App::Document* doc = editedObject->getDocument();
    for (std::size_t i = 0; i < count; i++) {
        App::DocumentObject* obj = objects[i];
        std::string edge = edges[i];
        std::string face = faces[i];

        QListWidgetItem* item = new QListWidgetItem(ui->listUnbound);
        ui->listUnbound->addItem(item);

        QString text = QString::fromLatin1("%1.%2").arg(QString::fromUtf8(obj->Label.getValue()),
                                                        QString::fromStdString(edge));
        item->setText(text);

        // The user data field of a list widget item
        // is a list of five elementa:
        // 1. document name
        // 2. object name
        // 3. sub-element name of the edge
        // 4. sub-element of an adjacent face or empty string
        // 5. the continuity as int
        QList<QVariant> data;
        data << QByteArray(doc->getName());
        data << QByteArray(obj->getNameInDocument());
        data << QByteArray(edge.c_str());
        data << QByteArray(face.c_str());
        data << static_cast<int>(conts[i]);
        item->setData(Qt::UserRole, data);
    }

    // attach this document observer
    attachDocument(Gui::Application::Instance->getDocument(doc));
}

void FillingEdgePanel::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void FillingEdgePanel::open()
{
    checkOpenCommand();

    // highlight the boundary edges
    this->vp->highlightReferences(ViewProviderFilling::Edge,
                                  editedObject->UnboundEdges.getSubListValues(),
                                  true);

    Gui::Selection().clearSelection();
}

void FillingEdgePanel::clearSelection()
{
    Gui::Selection().clearSelection();
}

void FillingEdgePanel::checkOpenCommand()
{
    if (checkCommand && !Gui::Command::hasPendingCommand()) {
        std::string Msg("Edit ");
        Msg += editedObject->Label.getValue();
        Gui::Command::openCommand(Msg.c_str());
        checkCommand = false;
    }
}

void FillingEdgePanel::slotUndoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void FillingEdgePanel::slotRedoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void FillingEdgePanel::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    // If this view provider is being deleted then reset the colors of
    // referenced part objects. The dialog will be deleted later.
    if (this->vp == &Obj) {
        this->vp->highlightReferences(ViewProviderFilling::Edge,
                                      editedObject->UnboundEdges.getSubListValues(),
                                      false);
    }
}

bool FillingEdgePanel::accept()
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

    this->vp->highlightReferences(ViewProviderFilling::Edge,
                                  editedObject->UnboundEdges.getSubListValues(),
                                  false);
    return true;
}

bool FillingEdgePanel::reject()
{
    this->vp->highlightReferences(ViewProviderFilling::Edge,
                                  editedObject->UnboundEdges.getSubListValues(),
                                  false);

    selectionMode = None;
    Gui::Selection().rmvSelectionGate();

    return true;
}

void FillingEdgePanel::onButtonUnboundEdgeAddToggled(bool checked)
{
    if (checked) {
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(new ShapeSelection(selectionMode, editedObject));
        selectionMode = AppendEdge;
    }
    else if (selectionMode == AppendEdge) {
        exitSelectionMode();
    }
}

void FillingEdgePanel::onButtonUnboundEdgeRemoveToggled(bool checked)
{
    if (checked) {
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(new ShapeSelection(selectionMode, editedObject));
        selectionMode = RemoveEdge;
    }
    else if (selectionMode == RemoveEdge) {
        exitSelectionMode();
    }
}

void FillingEdgePanel::onListUnboundItemDoubleClicked(QListWidgetItem* item)
{
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
    selectionMode = None;

    ui->comboBoxUnboundFaces->clear();
    ui->comboBoxUnboundCont->clear();

    if (item) {
        QList<QVariant> data;
        data = item->data(Qt::UserRole).toList();

        try {
            App::Document* doc = App::GetApplication().getDocument(data[0].toByteArray());
            App::DocumentObject* obj = doc ? doc->getObject(data[1].toByteArray()) : nullptr;
            if (obj && obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                const Part::TopoShape& shape = static_cast<Part::Feature*>(obj)->Shape.getShape();
                TopoDS_Shape edge = shape.getSubShape(data[2].toByteArray());

                // build up map edge->face
                TopTools_IndexedMapOfShape faces;
                TopExp::MapShapes(shape.getShape(), TopAbs_FACE, faces);
                TopTools_IndexedDataMapOfShapeListOfShape edge2Face;
                TopExp::MapShapesAndAncestors(shape.getShape(),
                                              TopAbs_EDGE,
                                              TopAbs_FACE,
                                              edge2Face);
                const TopTools_ListOfShape& adj_faces = edge2Face.FindFromKey(edge);
                if (adj_faces.Extent() > 0) {
                    int n = adj_faces.Extent();
                    ui->statusLabel->setText(tr("Edge has %n adjacent face(s)", nullptr, n));

                    // fill up the combo boxes
                    modifyBoundary(true);
                    ui->comboBoxUnboundFaces->addItem(tr("None"), QByteArray(""));
                    ui->comboBoxUnboundCont->addItem(QString::fromLatin1("C0"),
                                                     static_cast<int>(GeomAbs_C0));
                    ui->comboBoxUnboundCont->addItem(QString::fromLatin1("G1"),
                                                     static_cast<int>(GeomAbs_G1));
                    ui->comboBoxUnboundCont->addItem(QString::fromLatin1("G2"),
                                                     static_cast<int>(GeomAbs_G2));
                    TopTools_ListIteratorOfListOfShape it(adj_faces);
                    for (; it.More(); it.Next()) {
                        const TopoDS_Shape& F = it.Value();
                        int index = faces.FindIndex(F);
                        QString text = QString::fromLatin1("Face%1").arg(index);
                        ui->comboBoxUnboundFaces->addItem(text, text.toLatin1());
                    }

                    // activate face and continuity
                    if (data.size() == 5) {
                        int index = ui->comboBoxUnboundFaces->findData(data[3]);
                        ui->comboBoxUnboundFaces->setCurrentIndex(index);
                        index = ui->comboBoxUnboundCont->findData(data[4]);
                        ui->comboBoxUnboundCont->setCurrentIndex(index);
                    }
                }
                else {
                    ui->statusLabel->setText(tr("Edge has no adjacent faces"));
                }
            }

            Gui::Selection().addSelection(data[0].toByteArray(),
                                          data[1].toByteArray(),
                                          data[2].toByteArray());
        }
        catch (...) {
        }
    }
}

void FillingEdgePanel::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == None) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        checkOpenCommand();
        if (selectionMode == AppendEdge) {
            QListWidgetItem* item = new QListWidgetItem(ui->listUnbound);
            ui->listUnbound->addItem(item);

            Gui::SelectionObject sel(msg);
            QString text = QString::fromLatin1("%1.%2").arg(
                QString::fromUtf8(sel.getObject()->Label.getValue()),
                QString::fromLatin1(msg.pSubName));
            item->setText(text);

            QList<QVariant> data;
            data << QByteArray(msg.pDocName);
            data << QByteArray(msg.pObjectName);
            data << QByteArray(msg.pSubName);
            data << QByteArray("");
            data << static_cast<int>(GeomAbs_C0);
            item->setData(Qt::UserRole, data);

            auto objects = editedObject->UnboundEdges.getValues();
            std::size_t count = objects.size();
            objects.push_back(sel.getObject());
            auto element = editedObject->UnboundEdges.getSubValues();
            element.emplace_back(msg.pSubName);
            editedObject->UnboundEdges.setValues(objects, element);

            // extend faces and continuities lists if needed
            auto faces = editedObject->UnboundFaces.getValues();
            if (count == faces.size()) {
                faces.emplace_back();
                editedObject->UnboundFaces.setValues(faces);
            }
            auto conts = editedObject->UnboundOrder.getValues();
            if (count == conts.size()) {
                conts.push_back(static_cast<long>(GeomAbs_C0));
                editedObject->UnboundOrder.setValues(conts);
            }

            this->vp->highlightReferences(ViewProviderFilling::Edge,
                                          editedObject->UnboundEdges.getSubListValues(),
                                          true);
        }
        else if (selectionMode == RemoveEdge) {
            Gui::SelectionObject sel(msg);
            QList<QVariant> data;
            data << QByteArray(msg.pDocName);
            data << QByteArray(msg.pObjectName);
            data << QByteArray(msg.pSubName);

            // only the three first elements must match
            for (int i = 0; i < ui->listUnbound->count(); i++) {
                QListWidgetItem* item = ui->listUnbound->item(i);
                QList<QVariant> userdata = item->data(Qt::UserRole).toList();
                if (userdata.mid(0, 3) == data) {
                    ui->listUnbound->takeItem(i);
                    delete item;
                    break;
                }
            }

            this->vp->highlightReferences(ViewProviderFilling::Edge,
                                          editedObject->UnboundEdges.getSubListValues(),
                                          false);
            App::DocumentObject* obj = sel.getObject();
            std::string sub = msg.pSubName;
            auto objects = editedObject->UnboundEdges.getValues();
            auto element = editedObject->UnboundEdges.getSubValues();
            auto it = objects.begin();
            auto jt = element.begin();

            for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
                if (*it == obj && *jt == sub) {
                    std::size_t index = std::distance(objects.begin(), it);

                    objects.erase(it);
                    element.erase(jt);
                    editedObject->UnboundEdges.setValues(objects, element);

                    // try to remove the item also from the faces
                    auto faces = editedObject->UnboundFaces.getValues();
                    if (index < faces.size()) {
                        faces.erase(faces.begin() + index);
                        editedObject->UnboundFaces.setValues(faces);
                    }

                    // try to remove the item also from the orders
                    auto order = editedObject->UnboundOrder.getValues();
                    if (index < order.size()) {
                        order.erase(order.begin() + index);
                        editedObject->UnboundOrder.setValues(order);
                    }
                    break;
                }
            }
            this->vp->highlightReferences(ViewProviderFilling::Edge,
                                          editedObject->UnboundEdges.getSubListValues(),
                                          true);
        }

        editedObject->recomputeFeature();
        QTimer::singleShot(50, this, &FillingEdgePanel::clearSelection);
    }
}

void FillingEdgePanel::onDeleteUnboundEdge()
{
    int row = ui->listUnbound->currentRow();
    QListWidgetItem* item = ui->listUnbound->item(row);
    if (item) {
        checkOpenCommand();
        QList<QVariant> data;
        data = item->data(Qt::UserRole).toList();
        ui->listUnbound->takeItem(row);
        delete item;

        App::Document* doc = App::GetApplication().getDocument(data[0].toByteArray());
        App::DocumentObject* obj = doc ? doc->getObject(data[1].toByteArray()) : nullptr;
        std::string sub = data[2].toByteArray().constData();
        auto objects = editedObject->UnboundEdges.getValues();
        auto element = editedObject->UnboundEdges.getSubValues();
        auto it = objects.begin();
        auto jt = element.begin();
        this->vp->highlightReferences(ViewProviderFilling::Edge,
                                      editedObject->UnboundEdges.getSubListValues(),
                                      false);
        for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
            if (*it == obj && *jt == sub) {
                std::size_t index = std::distance(objects.begin(), it);

                objects.erase(it);
                element.erase(jt);
                editedObject->UnboundEdges.setValues(objects, element);

                // try to remove the item also from the faces
                auto faces = editedObject->UnboundFaces.getValues();
                if (index < faces.size()) {
                    faces.erase(faces.begin() + index);
                    editedObject->UnboundFaces.setValues(faces);
                }

                // try to remove the item also from the orders
                auto order = editedObject->UnboundOrder.getValues();
                if (index < order.size()) {
                    order.erase(order.begin() + index);
                    editedObject->UnboundOrder.setValues(order);
                }
                break;
            }
        }
        this->vp->highlightReferences(ViewProviderFilling::Edge,
                                      editedObject->UnboundEdges.getSubListValues(),
                                      true);

        editedObject->recomputeFeature();
    }
}

void FillingEdgePanel::onButtonUnboundAcceptClicked()
{
    QListWidgetItem* item = ui->listUnbound->currentItem();
    if (item) {
        QList<QVariant> data;
        data = item->data(Qt::UserRole).toList();

        QVariant face =
            ui->comboBoxUnboundFaces->itemData(ui->comboBoxUnboundFaces->currentIndex());
        QVariant cont = ui->comboBoxUnboundCont->itemData(ui->comboBoxUnboundCont->currentIndex());
        if (data.size() == 5) {
            data[3] = face;
            data[4] = cont;
        }
        else {
            data << face;
            data << cont;
        }

        item->setData(Qt::UserRole, data);

        std::size_t index = ui->listUnbound->row(item);

        // try to set the item of the faces
        auto faces = editedObject->UnboundFaces.getValues();
        if (index < faces.size()) {
            faces[index] = face.toByteArray().data();
            editedObject->UnboundFaces.setValues(faces);
        }

        // try to set the item of the orders
        auto order = editedObject->UnboundOrder.getValues();
        if (index < order.size()) {
            order[index] = cont.toInt();
            editedObject->UnboundOrder.setValues(order);
        }
    }

    modifyBoundary(false);
    ui->comboBoxUnboundFaces->clear();
    ui->comboBoxUnboundCont->clear();
    ui->statusLabel->clear();

    editedObject->recomputeFeature();
}

void FillingEdgePanel::onButtonUnboundIgnoreClicked()
{
    modifyBoundary(false);
    ui->comboBoxUnboundFaces->clear();
    ui->comboBoxUnboundCont->clear();
    ui->statusLabel->clear();
}

void FillingEdgePanel::modifyBoundary(bool on)
{
    ui->buttonUnboundEdgeAdd->setDisabled(on);
    ui->buttonUnboundEdgeRemove->setDisabled(on);
    ui->listUnbound->setDisabled(on);

    ui->comboBoxUnboundFaces->setEnabled(on);
    ui->comboBoxUnboundCont->setEnabled(on);
    ui->buttonUnboundAccept->setEnabled(on);
    ui->buttonUnboundIgnore->setEnabled(on);
}
}  // namespace SurfaceGui

void FillingEdgePanel::exitSelectionMode()
{
    // 'selectionMode' is passed by reference to the filter and changed when the filter is deleted
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
}

#include "moc_TaskFillingEdge.cpp"
