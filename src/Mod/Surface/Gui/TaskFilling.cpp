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

#include <GeomAbs_Shape.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/SelectionObject.h>
#include <Gui/Widgets.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "TaskFilling.h"
#include "TaskFillingEdge.h"
#include "TaskFillingVertex.h"
#include "ui_TaskFilling.h"


using namespace SurfaceGui;

PROPERTY_SOURCE(SurfaceGui::ViewProviderFilling, PartGui::ViewProviderSpline)

namespace SurfaceGui
{

void ViewProviderFilling::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit filling"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderSpline::setupContextMenu(menu, receiver, member);
}

bool ViewProviderFilling::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this sketch the
        // object unsets and sets its edit mode without closing
        // the task panel

        Surface::Filling* obj = static_cast<Surface::Filling*>(this->getObject());

        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();

        // start the edit dialog
        if (dlg) {
            TaskFilling* tDlg = qobject_cast<TaskFilling*>(dlg);
            if (tDlg) {
                tDlg->setEditedObject(obj);
            }
            Gui::Control().showDialog(dlg);
        }
        else {
            Gui::Control().showDialog(new TaskFilling(this, obj));
        }
        return true;
    }
    else {
        return ViewProviderSpline::setEdit(ModNum);
    }
}

void ViewProviderFilling::unsetEdit(int ModNum)
{
    PartGui::ViewProviderSpline::unsetEdit(ModNum);
}

QIcon ViewProviderFilling::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Surface_Filling");
}

void ViewProviderFilling::highlightReferences(ShapeType type, const References& refs, bool on)
{
    for (const auto& it : refs) {
        Part::Feature* base = dynamic_cast<Part::Feature*>(it.first);
        if (base) {
            PartGui::ViewProviderPartExt* svp = dynamic_cast<PartGui::ViewProviderPartExt*>(
                Gui::Application::Instance->getViewProvider(base));
            if (svp) {
                switch (type) {
                    case ViewProviderFilling::Vertex:
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
                    case ViewProviderFilling::Edge:
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
                    case ViewProviderFilling::Face:
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

class FillingPanel::ShapeSelection: public Gui::SelectionFilterGate
{
public:
    ShapeSelection(FillingPanel::SelectionMode& mode, Surface::Filling* editedObject)
        : Gui::SelectionFilterGate(nullPointer())
        , mode(mode)
        , editedObject(editedObject)
    {}
    ~ShapeSelection() override
    {
        mode = FillingPanel::None;
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
            case FillingPanel::InitFace:
                return allowFace(pObj, sSubName);
            case FillingPanel::AppendEdge:
                return allowEdge(true, pObj, sSubName);
            case FillingPanel::RemoveEdge:
                return allowEdge(false, pObj, sSubName);
            default:
                return false;
        }
    }

private:
    bool allowFace(App::DocumentObject*, const char* sSubName)
    {
        std::string element(sSubName);
        if (element.substr(0, 4) != "Face") {
            return false;
        }
        return true;
    }
    bool allowEdge(bool appendEdges, App::DocumentObject* pObj, const char* sSubName)
    {
        std::string element(sSubName);
        if (element.substr(0, 4) != "Edge") {
            return false;
        }

        auto links = editedObject->BoundaryEdges.getSubListValues();
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
    FillingPanel::SelectionMode& mode;
    Surface::Filling* editedObject;
};

// ----------------------------------------------------------------------------

FillingPanel::FillingPanel(ViewProviderFilling* vp, Surface::Filling* obj)
    : editedObject(obj)
{
    ui = new Ui_TaskFilling();
    ui->setupUi(this);
    setupConnections();
    ui->statusLabel->clear();

    selectionMode = None;
    this->vp = vp;
    checkCommand = true;
    setEditedObject(obj);

    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    action->setShortcut(QString::fromLatin1("Del"));
    action->setShortcutContext(Qt::WidgetShortcut);
    ui->listBoundary->addAction(action);
    connect(action, &QAction::triggered, this, &FillingPanel::onDeleteEdge);
    ui->listBoundary->setContextMenuPolicy(Qt::ActionsContextMenu);

    // clang-format off
    connect(ui->listBoundary->model(), &QAbstractItemModel::rowsMoved,
            this, &FillingPanel::onIndexesMoved);
    // clang-format on
}

/*
 *  Destroys the object and frees any allocated resources
 */
FillingPanel::~FillingPanel()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void FillingPanel::setupConnections()
{
    // clang-format off
    connect(ui->buttonInitFace, &QPushButton::clicked,
            this, &FillingPanel::onButtonInitFaceClicked);
    connect(ui->buttonEdgeAdd, &QToolButton::toggled,
            this, &FillingPanel::onButtonEdgeAddToggled);
    connect(ui->buttonEdgeRemove, &QToolButton::toggled,
            this, &FillingPanel::onButtonEdgeRemoveToggled);
    connect(ui->lineInitFaceName, &QLineEdit::textChanged,
            this, &FillingPanel::onLineInitFaceNameTextChanged);
    connect(ui->listBoundary, &QListWidget::itemDoubleClicked,
            this, &FillingPanel::onListBoundaryItemDoubleClicked);
    connect(ui->buttonAccept, &QPushButton::clicked,
            this, &FillingPanel::onButtonAcceptClicked);
    connect(ui->buttonIgnore, &QPushButton::clicked,
            this, &FillingPanel::onButtonIgnoreClicked);
    // clang-format on
}

void FillingPanel::appendButtons(Gui::ButtonGroup* buttonGroup)
{
    buttonGroup->addButton(ui->buttonInitFace, int(SelectionMode::InitFace));
    buttonGroup->addButton(ui->buttonEdgeAdd, int(SelectionMode::AppendEdge));
    buttonGroup->addButton(ui->buttonEdgeRemove, int(SelectionMode::RemoveEdge));
}

// stores object pointer, its old fill type and adjusts radio buttons according to it.
void FillingPanel::setEditedObject(Surface::Filling* fea)
{
    editedObject = fea;

    // get the link to the initial surface if set
    App::DocumentObject* initFace = editedObject->InitialFace.getValue();
    const std::vector<std::string>& subList = editedObject->InitialFace.getSubValues();
    if (initFace && subList.size() == 1) {
        QString text =
            QString::fromLatin1("%1.%2").arg(QString::fromUtf8(initFace->Label.getValue()),
                                             QString::fromStdString(subList.front()));
        ui->lineInitFaceName->setText(text);
    }

    // get the boundary edges, if set their adjacent faces and continuities
    auto objects = editedObject->BoundaryEdges.getValues();
    auto edges = editedObject->BoundaryEdges.getSubValues();
    auto count = objects.size();

    // fill up faces if wrong size
    auto faces = editedObject->BoundaryFaces.getValues();
    if (faces.size() != edges.size()) {
        faces.resize(edges.size());
        std::fill(faces.begin(), faces.end(), std::string());
    }

    // fill up continuities if wrong size
    auto conts = editedObject->BoundaryOrder.getValues();
    if (edges.size() != conts.size()) {
        conts.resize(edges.size());
        std::fill(conts.begin(), conts.end(), static_cast<long>(GeomAbs_C0));
    }

    App::Document* doc = editedObject->getDocument();
    for (std::size_t i = 0; i < count; i++) {
        App::DocumentObject* obj = objects[i];
        std::string edge = edges[i];
        std::string face = faces[i];

        QListWidgetItem* item = new QListWidgetItem(ui->listBoundary);
        ui->listBoundary->addItem(item);

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

void FillingPanel::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void FillingPanel::open()
{
    checkOpenCommand();

    // highlight the boundary edges
    this->vp->highlightReferences(ViewProviderFilling::Edge,
                                  editedObject->BoundaryEdges.getSubListValues(),
                                  true);

    // highlight the referenced face
    std::vector<App::PropertyLinkSubList::SubSet> links;
    links.emplace_back(editedObject->InitialFace.getValue(),
                       editedObject->InitialFace.getSubValues());
    this->vp->highlightReferences(ViewProviderFilling::Face, links, true);

    Gui::Selection().clearSelection();

    // if the surface is not yet created then automatically start "AppendEdge" mode
    if (editedObject->Shape.getShape().isNull()) {
        ui->buttonEdgeAdd->setChecked(true);
    }
}

void FillingPanel::clearSelection()
{
    Gui::Selection().clearSelection();
}

void FillingPanel::checkOpenCommand()
{
    if (checkCommand && !Gui::Command::hasPendingCommand()) {
        std::string Msg("Edit ");
        Msg += editedObject->Label.getValue();
        Gui::Command::openCommand(Msg.c_str());
        checkCommand = false;
    }
}

void FillingPanel::slotUndoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void FillingPanel::slotRedoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void FillingPanel::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    // If this view provider is being deleted then reset the colors of
    // referenced part objects. The dialog will be deleted later.
    if (this->vp == &Obj) {
        this->vp->highlightReferences(ViewProviderFilling::Edge,
                                      editedObject->BoundaryEdges.getSubListValues(),
                                      false);

        // unhighlight the referenced face
        std::vector<App::PropertyLinkSubList::SubSet> links;
        links.emplace_back(editedObject->InitialFace.getValue(),
                           editedObject->InitialFace.getSubValues());
        this->vp->highlightReferences(ViewProviderFilling::Face, links, false);
    }
}

bool FillingPanel::accept()
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
                                  editedObject->BoundaryEdges.getSubListValues(),
                                  false);

    // unhighlight the referenced face
    std::vector<App::PropertyLinkSubList::SubSet> links;
    links.emplace_back(editedObject->InitialFace.getValue(),
                       editedObject->InitialFace.getSubValues());
    this->vp->highlightReferences(ViewProviderFilling::Face, links, false);

    return true;
}

bool FillingPanel::reject()
{
    if (!editedObject.expired()) {
        this->vp->highlightReferences(ViewProviderFilling::Edge,
                                      editedObject->BoundaryEdges.getSubListValues(),
                                      false);

        // unhighlight the referenced face
        std::vector<App::PropertyLinkSubList::SubSet> links;
        links.emplace_back(editedObject->InitialFace.getValue(),
                           editedObject->InitialFace.getSubValues());
        this->vp->highlightReferences(ViewProviderFilling::Face, links, false);
    }

    selectionMode = None;
    Gui::Selection().rmvSelectionGate();

    return true;
}

void FillingPanel::onLineInitFaceNameTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        checkOpenCommand();

        // unhighlight the referenced face
        std::vector<App::PropertyLinkSubList::SubSet> links;
        links.emplace_back(editedObject->InitialFace.getValue(),
                           editedObject->InitialFace.getSubValues());
        this->vp->highlightReferences(ViewProviderFilling::Face, links, false);

        editedObject->InitialFace.setValue(nullptr);
        editedObject->recomputeFeature();
    }
}

void FillingPanel::onButtonInitFaceClicked()
{
    // 'selectionMode' is passed by reference and changed when the filter is deleted
    Gui::Selection().addSelectionGate(new ShapeSelection(selectionMode, editedObject.get()));
    selectionMode = InitFace;
}

void FillingPanel::onButtonEdgeAddToggled(bool checked)
{
    if (checked) {
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(new ShapeSelection(selectionMode, editedObject.get()));
        selectionMode = AppendEdge;
    }
    else if (selectionMode == AppendEdge) {
        exitSelectionMode();
    }
}

void FillingPanel::onButtonEdgeRemoveToggled(bool checked)
{
    if (checked) {
        // 'selectionMode' is passed by reference and changed when the filter is deleted
        Gui::Selection().addSelectionGate(new ShapeSelection(selectionMode, editedObject.get()));
        selectionMode = RemoveEdge;
    }
    else if (selectionMode == RemoveEdge) {
        exitSelectionMode();
    }
}

void FillingPanel::onListBoundaryItemDoubleClicked(QListWidgetItem* item)
{
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
    selectionMode = None;

    ui->comboBoxFaces->clear();
    ui->comboBoxCont->clear();

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
                    ui->statusLabel->setText(tr("Edge has %n adjacent faces", nullptr, n));

                    // fill up the combo boxes
                    modifyBoundary(true);
                    ui->comboBoxFaces->addItem(tr("None"), QByteArray(""));
                    ui->comboBoxCont->addItem(QString::fromLatin1("C0"),
                                              static_cast<int>(GeomAbs_C0));
                    ui->comboBoxCont->addItem(QString::fromLatin1("G1"),
                                              static_cast<int>(GeomAbs_G1));
                    ui->comboBoxCont->addItem(QString::fromLatin1("G2"),
                                              static_cast<int>(GeomAbs_G2));
                    TopTools_ListIteratorOfListOfShape it(adj_faces);
                    for (; it.More(); it.Next()) {
                        const TopoDS_Shape& F = it.Value();
                        int index = faces.FindIndex(F);
                        QString text = QString::fromLatin1("Face%1").arg(index);
                        ui->comboBoxFaces->addItem(text, text.toLatin1());
                    }

                    // activate face and continuity
                    if (data.size() == 5) {
                        int index = ui->comboBoxFaces->findData(data[3]);
                        ui->comboBoxFaces->setCurrentIndex(index);
                        index = ui->comboBoxCont->findData(data[4]);
                        ui->comboBoxCont->setCurrentIndex(index);
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

void FillingPanel::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == None) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        checkOpenCommand();
        if (selectionMode == InitFace) {
            Gui::SelectionObject sel(msg);
            QString text = QString::fromLatin1("%1.%2").arg(
                QString::fromUtf8(sel.getObject()->Label.getValue()),
                QString::fromLatin1(msg.pSubName));
            ui->lineInitFaceName->setText(text);

            std::vector<std::string> subList;
            subList.emplace_back(msg.pSubName);
            editedObject->InitialFace.setValue(sel.getObject(), subList);

            // highlight the referenced face
            std::vector<App::PropertyLinkSubList::SubSet> links;
            links.emplace_back(sel.getObject(), subList);
            this->vp->highlightReferences(ViewProviderFilling::Face, links, true);

            Gui::Selection().rmvSelectionGate();
            selectionMode = None;
        }
        else if (selectionMode == AppendEdge) {
            QListWidgetItem* item = new QListWidgetItem(ui->listBoundary);
            ui->listBoundary->addItem(item);

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

            auto objects = editedObject->BoundaryEdges.getValues();
            std::size_t count = objects.size();
            objects.push_back(sel.getObject());
            auto element = editedObject->BoundaryEdges.getSubValues();
            element.emplace_back(msg.pSubName);
            editedObject->BoundaryEdges.setValues(objects, element);

            // extend faces and continuities lists if needed
            auto faces = editedObject->BoundaryFaces.getValues();
            if (count == faces.size()) {
                faces.emplace_back();
                editedObject->BoundaryFaces.setValues(faces);
            }
            auto conts = editedObject->BoundaryOrder.getValues();
            if (count == conts.size()) {
                conts.push_back(static_cast<long>(GeomAbs_C0));
                editedObject->BoundaryOrder.setValues(conts);
            }

            this->vp->highlightReferences(ViewProviderFilling::Edge,
                                          editedObject->BoundaryEdges.getSubListValues(),
                                          true);
        }
        else if (selectionMode == RemoveEdge) {
            Gui::SelectionObject sel(msg);
            QList<QVariant> data;
            data << QByteArray(msg.pDocName);
            data << QByteArray(msg.pObjectName);
            data << QByteArray(msg.pSubName);

            // only the three first elements must match
            for (int i = 0; i < ui->listBoundary->count(); i++) {
                QListWidgetItem* item = ui->listBoundary->item(i);
                QList<QVariant> userdata = item->data(Qt::UserRole).toList();
                if (userdata.mid(0, 3) == data) {
                    ui->listBoundary->takeItem(i);
                    delete item;
                    break;
                }
            }

            this->vp->highlightReferences(ViewProviderFilling::Edge,
                                          editedObject->BoundaryEdges.getSubListValues(),
                                          false);
            App::DocumentObject* obj = sel.getObject();
            std::string sub = msg.pSubName;
            auto objects = editedObject->BoundaryEdges.getValues();
            auto element = editedObject->BoundaryEdges.getSubValues();
            auto it = objects.begin();
            auto jt = element.begin();

            for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
                if (*it == obj && *jt == sub) {
                    std::size_t index = std::distance(objects.begin(), it);

                    objects.erase(it);
                    element.erase(jt);
                    editedObject->BoundaryEdges.setValues(objects, element);

                    // try to remove the item also from the faces
                    auto faces = editedObject->BoundaryFaces.getValues();
                    if (index < faces.size()) {
                        faces.erase(faces.begin() + index);
                        editedObject->BoundaryFaces.setValues(faces);
                    }

                    // try to remove the item also from the orders
                    auto order = editedObject->BoundaryOrder.getValues();
                    if (index < order.size()) {
                        order.erase(order.begin() + index);
                        editedObject->BoundaryOrder.setValues(order);
                    }
                    break;
                }
            }
            this->vp->highlightReferences(ViewProviderFilling::Edge,
                                          editedObject->BoundaryEdges.getSubListValues(),
                                          true);
        }

        editedObject->recomputeFeature();
        QTimer::singleShot(50, this, &FillingPanel::clearSelection);
    }
}

void FillingPanel::onDeleteEdge()
{
    int row = ui->listBoundary->currentRow();
    QListWidgetItem* item = ui->listBoundary->item(row);
    if (item) {
        checkOpenCommand();
        QList<QVariant> data;
        data = item->data(Qt::UserRole).toList();
        ui->listBoundary->takeItem(row);
        delete item;

        App::Document* doc = App::GetApplication().getDocument(data[0].toByteArray());
        App::DocumentObject* obj = doc ? doc->getObject(data[1].toByteArray()) : nullptr;
        std::string sub = data[2].toByteArray().constData();
        auto objects = editedObject->BoundaryEdges.getValues();
        auto element = editedObject->BoundaryEdges.getSubValues();
        auto it = objects.begin();
        auto jt = element.begin();
        this->vp->highlightReferences(ViewProviderFilling::Edge,
                                      editedObject->BoundaryEdges.getSubListValues(),
                                      false);
        for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
            if (*it == obj && *jt == sub) {
                std::size_t index = std::distance(objects.begin(), it);

                objects.erase(it);
                element.erase(jt);
                editedObject->BoundaryEdges.setValues(objects, element);

                // try to remove the item also from the faces
                auto faces = editedObject->BoundaryFaces.getValues();
                if (index < faces.size()) {
                    faces.erase(faces.begin() + index);
                    editedObject->BoundaryFaces.setValues(faces);
                }

                // try to remove the item also from the orders
                auto order = editedObject->BoundaryOrder.getValues();
                if (index < order.size()) {
                    order.erase(order.begin() + index);
                    editedObject->BoundaryOrder.setValues(order);
                }
                break;
            }
        }
        this->vp->highlightReferences(ViewProviderFilling::Edge,
                                      editedObject->BoundaryEdges.getSubListValues(),
                                      true);

        editedObject->recomputeFeature();
    }
}

void FillingPanel::onIndexesMoved()
{
    QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(sender());
    if (!model) {
        return;
    }

    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> element;
    std::vector<std::string> faces;
    std::vector<long> order;

    int rows = model->rowCount();
    for (int i = 0; i < rows; i++) {
        QModelIndex index = model->index(i, 0);
        QList<QVariant> data;
        data = index.data(Qt::UserRole).toList();

        App::Document* doc = App::GetApplication().getDocument(data[0].toByteArray());
        App::DocumentObject* obj = doc ? doc->getObject(data[1].toByteArray()) : nullptr;
        std::string sub = data[2].toByteArray().constData();
        std::string face = data[3].toByteArray().constData();
        long cont = data[4].toInt();

        objects.push_back(obj);
        element.push_back(sub);

        faces.push_back(face);
        order.push_back(cont);
    }

    editedObject->BoundaryEdges.setValues(objects, element);
    editedObject->BoundaryFaces.setValues(faces);
    editedObject->BoundaryOrder.setValues(order);
    editedObject->recomputeFeature();
}

void FillingPanel::onButtonAcceptClicked()
{
    QListWidgetItem* item = ui->listBoundary->currentItem();
    if (item) {
        QList<QVariant> data;
        data = item->data(Qt::UserRole).toList();

        QVariant face = ui->comboBoxFaces->itemData(ui->comboBoxFaces->currentIndex());
        QVariant cont = ui->comboBoxCont->itemData(ui->comboBoxCont->currentIndex());
        if (data.size() == 5) {
            data[3] = face;
            data[4] = cont;
        }
        else {
            data << face;
            data << cont;
        }

        item->setData(Qt::UserRole, data);

        std::size_t index = ui->listBoundary->row(item);

        // try to set the item of the faces
        auto faces = editedObject->BoundaryFaces.getValues();
        if (index < faces.size()) {
            faces[index] = face.toByteArray().data();
            editedObject->BoundaryFaces.setValues(faces);
        }

        // try to set the item of the orders
        auto order = editedObject->BoundaryOrder.getValues();
        if (index < order.size()) {
            order[index] = cont.toInt();
            editedObject->BoundaryOrder.setValues(order);
        }
    }

    modifyBoundary(false);
    ui->comboBoxFaces->clear();
    ui->comboBoxCont->clear();
    ui->statusLabel->clear();

    editedObject->recomputeFeature();
}

void FillingPanel::onButtonIgnoreClicked()
{
    modifyBoundary(false);
    ui->comboBoxFaces->clear();
    ui->comboBoxCont->clear();
    ui->statusLabel->clear();
}

void FillingPanel::modifyBoundary(bool on)
{
    ui->buttonInitFace->setDisabled(on);
    ui->lineInitFaceName->setDisabled(on);
    ui->buttonEdgeAdd->setDisabled(on);
    ui->buttonEdgeRemove->setDisabled(on);
    ui->listBoundary->setDisabled(on);

    ui->comboBoxFaces->setEnabled(on);
    ui->comboBoxCont->setEnabled(on);
    ui->buttonAccept->setEnabled(on);
    ui->buttonIgnore->setEnabled(on);
}

void FillingPanel::exitSelectionMode()
{
    // 'selectionMode' is passed by reference to the filter and changed when the filter is deleted
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
}

// ----------------------------------------------------------------------------

TaskFilling::TaskFilling(ViewProviderFilling* vp, Surface::Filling* obj)
{
    // Set up button group
    buttonGroup = new Gui::ButtonGroup(this);
    buttonGroup->setExclusive(true);

    // first task box
    widget1 = new FillingPanel(vp, obj);
    widget1->appendButtons(buttonGroup);
    Gui::TaskView::TaskBox* taskbox1 =
        new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Surface_Filling"),
                                   widget1->windowTitle(),
                                   true,
                                   nullptr);
    taskbox1->groupLayout()->addWidget(widget1);
    Content.push_back(taskbox1);

    // second task box
    widget2 = new FillingEdgePanel(vp, obj);
    widget2->appendButtons(buttonGroup);
    Gui::TaskView::TaskBox* taskbox2 =
        new Gui::TaskView::TaskBox(QPixmap(), widget2->windowTitle(), true, nullptr);
    taskbox2->groupLayout()->addWidget(widget2);
    Content.push_back(taskbox2);
    taskbox2->hideGroupBox();

    // third task box
    widget3 = new FillingVertexPanel(vp, obj);
    widget3->appendButtons(buttonGroup);
    Gui::TaskView::TaskBox* taskbox3 =
        new Gui::TaskView::TaskBox(QPixmap(), widget3->windowTitle(), true, nullptr);
    taskbox3->groupLayout()->addWidget(widget3);
    Content.push_back(taskbox3);
    taskbox3->hideGroupBox();
}

void TaskFilling::setEditedObject(Surface::Filling* obj)
{
    widget1->setEditedObject(obj);
}

void TaskFilling::open()
{
    widget1->open();
    widget2->open();
    widget3->open();
}

void TaskFilling::closed()
{
    widget1->reject();
}

bool TaskFilling::accept()
{
    bool ok = widget1->accept();
    if (ok) {
        widget2->reject();
        widget3->reject();
        Gui::Command::commitCommand();
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        Gui::Command::updateActive();
    }

    return ok;
}

bool TaskFilling::reject()
{
    bool ok = widget1->reject();
    if (ok) {
        widget2->reject();
        widget3->reject();
        Gui::Command::abortCommand();
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        Gui::Command::updateActive();
    }

    return ok;
}

}  // namespace SurfaceGui

#include "moc_TaskFilling.cpp"
