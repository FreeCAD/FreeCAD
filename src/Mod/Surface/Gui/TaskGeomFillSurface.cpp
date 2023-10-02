/***************************************************************************
 *   Copyright (c) 2015 Balázs Bámer                                       *
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/SelectionObject.h>
#include <Gui/Widgets.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "TaskGeomFillSurface.h"
#include "ui_TaskGeomFillSurface.h"


using namespace SurfaceGui;

PROPERTY_SOURCE(SurfaceGui::ViewProviderGeomFillSurface, PartGui::ViewProviderSpline)

namespace SurfaceGui
{

void ViewProviderGeomFillSurface::setupContextMenu(QMenu* menu,
                                                   QObject* receiver,
                                                   const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit filling"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderSpline::setupContextMenu(menu, receiver, member);
}

bool ViewProviderGeomFillSurface::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this sketch the
        // object unsets and sets its edit mode without closing
        // the task panel

        Surface::GeomFillSurface* obj = static_cast<Surface::GeomFillSurface*>(this->getObject());

        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();

        // start the edit dialog
        if (dlg) {
            TaskGeomFillSurface* tDlg = qobject_cast<TaskGeomFillSurface*>(dlg);
            if (tDlg) {
                tDlg->setEditedObject(obj);
            }
            Gui::Control().showDialog(dlg);
        }
        else {
            Gui::Control().showDialog(new TaskGeomFillSurface(this, obj));
        }
        return true;
    }
    else {
        return ViewProviderSpline::setEdit(ModNum);
    }
}

void ViewProviderGeomFillSurface::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        QTimer::singleShot(0, &Gui::Control(), &Gui::ControlSingleton::closeDialog);
    }
    else {
        PartGui::ViewProviderSpline::unsetEdit(ModNum);
    }
}

QIcon ViewProviderGeomFillSurface::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Surface_BSplineSurface");
}

void ViewProviderGeomFillSurface::highlightReferences(bool on)
{
    Surface::GeomFillSurface* surface = static_cast<Surface::GeomFillSurface*>(getObject());
    auto bounds = surface->BoundaryList.getSubListValues();
    for (const auto& it : bounds) {
        Part::Feature* base = dynamic_cast<Part::Feature*>(it.first);
        if (base) {
            PartGui::ViewProviderPartExt* svp = dynamic_cast<PartGui::ViewProviderPartExt*>(
                Gui::Application::Instance->getViewProvider(base));
            if (svp) {
                if (on) {
                    std::vector<App::Color> colors;
                    TopTools_IndexedMapOfShape eMap;
                    TopExp::MapShapes(base->Shape.getValue(), TopAbs_EDGE, eMap);
                    colors.resize(eMap.Extent(), svp->LineColor.getValue());

                    for (const auto& jt : it.second) {
                        std::size_t idx = static_cast<std::size_t>(std::stoi(jt.substr(4)) - 1);
                        assert(idx < colors.size());
                        colors[idx] = App::Color(1.0, 0.0, 1.0);  // magenta
                    }

                    svp->setHighlightedEdges(colors);
                }
                else {
                    svp->unsetHighlightedEdges();
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------

class GeomFillSurface::EdgeSelection: public Gui::SelectionFilterGate
{
public:
    EdgeSelection(bool appendEdges, Surface::GeomFillSurface* editedObject)
        : Gui::SelectionFilterGate(nullPointer())
        , appendEdges(appendEdges)
        , editedObject(editedObject)
    {}
    /**
     * Allow the user to pick only edges.
     */
    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName) override;

private:
    bool appendEdges;
    Surface::GeomFillSurface* editedObject;
};

bool GeomFillSurface::EdgeSelection::allow(App::Document*,
                                           App::DocumentObject* pObj,
                                           const char* sSubName)
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

    std::string element(sSubName);
    if (element.substr(0, 4) != "Edge") {
        return false;
    }

    auto links = editedObject->BoundaryList.getSubListValues();
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

// ----------------------------------------------------------------------------

GeomFillSurface::GeomFillSurface(ViewProviderGeomFillSurface* vp, Surface::GeomFillSurface* obj)
{
    ui = new Ui_GeomFillSurface();
    ui->setupUi(this);
    setupConnections();

    selectionMode = None;
    this->vp = vp;
    checkCommand = true;
    setEditedObject(obj);

    // Set up button group
    buttonGroup = new Gui::ButtonGroup(this);
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(ui->buttonEdgeAdd, (int)SelectionMode::Append);
    buttonGroup->addButton(ui->buttonEdgeRemove, (int)SelectionMode::Remove);

    // Create context menu
    QAction* remove = new QAction(tr("Remove"), this);
    remove->setShortcut(QString::fromLatin1("Del"));
    ui->listWidget->addAction(remove);
    connect(remove, &QAction::triggered, this, &GeomFillSurface::onDeleteEdge);

    QAction* orientation = new QAction(tr("Flip orientation"), this);
    ui->listWidget->addAction(orientation);
    connect(orientation, &QAction::triggered, this, &GeomFillSurface::onFlipOrientation);

    ui->listWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
}

/*
 *  Destroys the object and frees any allocated resources
 */
GeomFillSurface::~GeomFillSurface()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void GeomFillSurface::setupConnections()
{
    connect(ui->fillType_stretch,
            &QRadioButton::clicked,
            this,
            &GeomFillSurface::onFillTypeStretchClicked);
    connect(ui->fillType_coons,
            &QRadioButton::clicked,
            this,
            &GeomFillSurface::onFillTypeCoonsClicked);
    connect(ui->fillType_curved,
            &QRadioButton::clicked,
            this,
            &GeomFillSurface::onFillTypeCurvedClicked);
    connect(ui->buttonEdgeAdd,
            &QToolButton::toggled,
            this,
            &GeomFillSurface::onButtonEdgeAddToggled);
    connect(ui->buttonEdgeRemove,
            &QToolButton::toggled,
            this,
            &GeomFillSurface::onButtonEdgeRemoveToggled);
    connect(ui->listWidget,
            &QListWidget::itemDoubleClicked,
            this,
            &GeomFillSurface::onListWidgetItemDoubleClicked);
}

// stores object pointer, its old fill type and adjusts radio buttons according to it.
void GeomFillSurface::setEditedObject(Surface::GeomFillSurface* obj)
{
    editedObject = obj;
    GeomFill_FillingStyle curtype =
        static_cast<GeomFill_FillingStyle>(editedObject->FillType.getValue());
    switch (curtype) {
        case GeomFill_StretchStyle:
            ui->fillType_stretch->setChecked(true);
            break;
        case GeomFill_CoonsStyle:
            ui->fillType_coons->setChecked(true);
            break;
        case GeomFill_CurvedStyle:
            ui->fillType_curved->setChecked(true);
            break;
        default:
            break;
    }

    auto objects = editedObject->BoundaryList.getValues();
    auto element = editedObject->BoundaryList.getSubValues();
    auto boolean = editedObject->ReversedList.getValues();
    auto it = objects.begin();
    auto jt = element.begin();
    std::size_t index = 0;

    QPixmap rotateLeft = Gui::BitmapFactory().pixmap("view-rotate-left");
    QPixmap rotateRight = Gui::BitmapFactory().pixmap("view-rotate-right");

    App::Document* doc = editedObject->getDocument();
    for (; it != objects.end() && jt != element.end(); ++it, ++jt, ++index) {
        QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
        if (index < boolean.size()) {
            if (boolean[index]) {
                item->setIcon(rotateLeft);
            }
            else {
                item->setIcon(rotateRight);
            }
        }
        ui->listWidget->addItem(item);

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

void GeomFillSurface::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void GeomFillSurface::open()
{
    checkOpenCommand();
    this->vp->highlightReferences(true);
    Gui::Selection().clearSelection();
}

void GeomFillSurface::clearSelection()
{
    Gui::Selection().clearSelection();
}

void GeomFillSurface::checkOpenCommand()
{
    if (checkCommand && !Gui::Command::hasPendingCommand()) {
        std::string Msg("Edit ");
        Msg += editedObject->Label.getValue();
        Gui::Command::openCommand(Msg.c_str());
        checkCommand = false;
    }
}

void GeomFillSurface::slotUndoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void GeomFillSurface::slotRedoDocument(const Gui::Document&)
{
    checkCommand = true;
}

void GeomFillSurface::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    // If this view provider is being deleted then reset the colors of
    // referenced part objects. The dialog will be deleted later.
    if (this->vp == &Obj) {
        this->vp->highlightReferences(false);
    }
}

bool GeomFillSurface::accept()
{
    selectionMode = None;
    Gui::Selection().rmvSelectionGate();

    int count = ui->listWidget->count();
    if (count > 4) {
        QMessageBox::warning(this,
                             tr("Too many edges"),
                             tr("The tool requires two, three or four edges"));
        return false;
    }
    else if (count < 2) {
        QMessageBox::warning(this,
                             tr("Too less edges"),
                             tr("The tool requires two, three or four edges"));
        return false;
    }

    if (editedObject->mustExecute()) {
        editedObject->recomputeFeature();
    }
    if (!editedObject->isValid()) {
        QMessageBox::warning(this,
                             tr("Invalid object"),
                             QString::fromLatin1(editedObject->getStatusString()));
        return false;
    }

    this->vp->highlightReferences(false);

    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    Gui::Command::updateActive();
    return true;
}

bool GeomFillSurface::reject()
{
    this->vp->highlightReferences(false);
    selectionMode = None;
    Gui::Selection().rmvSelectionGate();

    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    Gui::Command::updateActive();
    return true;
}

void GeomFillSurface::onFillTypeStretchClicked()
{
    changeFillType(GeomFill_StretchStyle);
}

void GeomFillSurface::onFillTypeCoonsClicked()
{
    changeFillType(GeomFill_CoonsStyle);
}

void GeomFillSurface::onFillTypeCurvedClicked()
{
    changeFillType(GeomFill_CurvedStyle);
}

void GeomFillSurface::changeFillType(GeomFill_FillingStyle fillType)
{
    GeomFill_FillingStyle curtype =
        static_cast<GeomFill_FillingStyle>(editedObject->FillType.getValue());
    if (curtype != fillType) {
        checkOpenCommand();
        editedObject->FillType.setValue(static_cast<long>(fillType));
        editedObject->recomputeFeature();
        if (!editedObject->isValid()) {
            Base::Console().Error("Surface filling: %s", editedObject->getStatusString());
        }
    }
}

void GeomFillSurface::onButtonEdgeAddToggled(bool checked)
{
    if (checked) {
        selectionMode = Append;
        Gui::Selection().addSelectionGate(new EdgeSelection(true, editedObject));
    }
    else if (selectionMode == Append) {
        exitSelectionMode();
    }
}

void GeomFillSurface::onButtonEdgeRemoveToggled(bool checked)
{
    if (checked) {
        selectionMode = Remove;
        Gui::Selection().addSelectionGate(new EdgeSelection(false, editedObject));
    }
    else if (selectionMode == Remove) {
        exitSelectionMode();
    }
}

void GeomFillSurface::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == None) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        checkOpenCommand();
        if (selectionMode == Append) {
            QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
            item->setIcon(Gui::BitmapFactory().pixmap("view-rotate-right"));
            ui->listWidget->addItem(item);

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

            auto objects = editedObject->BoundaryList.getValues();
            objects.push_back(sel.getObject());
            auto element = editedObject->BoundaryList.getSubValues();
            element.emplace_back(msg.pSubName);
            editedObject->BoundaryList.setValues(objects, element);
            auto booleans = editedObject->ReversedList.getValues();
            booleans.push_back(false);
            editedObject->ReversedList.setValues(booleans);
            this->vp->highlightReferences(true);
        }
        else {
            Gui::SelectionObject sel(msg);
            QList<QVariant> data;
            int row = 0;
            data << QByteArray(msg.pDocName);
            data << QByteArray(msg.pObjectName);
            data << QByteArray(msg.pSubName);
            for (int i = 0; i < ui->listWidget->count(); i++) {
                QListWidgetItem* item = ui->listWidget->item(i);
                if (item && item->data(Qt::UserRole) == data) {
                    row = i;
                    ui->listWidget->takeItem(i);
                    delete item;
                }
            }

            this->vp->highlightReferences(false);
            App::DocumentObject* obj = sel.getObject();
            std::string sub = msg.pSubName;
            auto objects = editedObject->BoundaryList.getValues();
            auto element = editedObject->BoundaryList.getSubValues();
            auto it = objects.begin();
            auto jt = element.begin();

            // remove the element of the bitset at position 'row'
            const boost::dynamic_bitset<>& old_booleans = editedObject->ReversedList.getValues();
            boost::dynamic_bitset<> new_booleans = old_booleans >> 1;
            new_booleans.resize(objects.size() - 1);

            // double-check in case 'old_booleans' is out of sync
            if (new_booleans.size() < old_booleans.size()) {
                for (int i = 0; i < row; i++) {
                    new_booleans[i] = old_booleans[i];
                }
            }

            for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
                if (*it == obj && *jt == sub) {
                    objects.erase(it);
                    element.erase(jt);
                    editedObject->BoundaryList.setValues(objects, element);
                    editedObject->ReversedList.setValues(new_booleans);
                    break;
                }
            }
            this->vp->highlightReferences(true);
        }

        editedObject->recomputeFeature();
        QTimer::singleShot(50, this, &GeomFillSurface::clearSelection);
    }
}

void GeomFillSurface::onDeleteEdge()
{
    int row = ui->listWidget->currentRow();
    QListWidgetItem* item = ui->listWidget->item(row);
    if (item) {
        checkOpenCommand();
        QList<QVariant> data;
        data = item->data(Qt::UserRole).toList();
        ui->listWidget->takeItem(row);
        delete item;

        App::Document* doc = App::GetApplication().getDocument(data[0].toByteArray());
        App::DocumentObject* obj = doc ? doc->getObject(data[1].toByteArray()) : nullptr;
        std::string sub = data[2].toByteArray().constData();
        auto objects = editedObject->BoundaryList.getValues();
        auto element = editedObject->BoundaryList.getSubValues();
        auto it = objects.begin();
        auto jt = element.begin();
        this->vp->highlightReferences(false);

        // remove the element of the bitset at position 'row'
        const boost::dynamic_bitset<>& old_booleans = editedObject->ReversedList.getValues();
        boost::dynamic_bitset<> new_booleans = old_booleans >> 1;
        new_booleans.resize(objects.size() - 1);

        // double-check in case 'old_booleans' is out of sync
        if (new_booleans.size() < old_booleans.size()) {
            for (int i = 0; i < row; i++) {
                new_booleans[i] = old_booleans[i];
            }
        }

        for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
            if (*it == obj && *jt == sub) {
                objects.erase(it);
                element.erase(jt);
                editedObject->BoundaryList.setValues(objects, element);
                editedObject->ReversedList.setValues(new_booleans);
                break;
            }
        }
        this->vp->highlightReferences(true);
    }
}

void GeomFillSurface::flipOrientation(QListWidgetItem* item)
{
    // toggle the orientation of the input curve
    //
    QPixmap rotateLeft = Gui::BitmapFactory().pixmap("view-rotate-left");
    QPixmap rotateRight = Gui::BitmapFactory().pixmap("view-rotate-right");

    int row = ui->listWidget->row(item);
    if (row < editedObject->ReversedList.getSize()) {
        auto booleans = editedObject->ReversedList.getValues();
        bool reversed = !booleans[row];
        booleans[row] = reversed;
        if (reversed) {
            item->setIcon(rotateLeft);
        }
        else {
            item->setIcon(rotateRight);
        }

        editedObject->ReversedList.setValues(booleans);
        editedObject->recomputeFeature();
    }
}

void GeomFillSurface::onListWidgetItemDoubleClicked(QListWidgetItem* item)
{
    if (item) {
        flipOrientation(item);
    }
}

void GeomFillSurface::onFlipOrientation()
{
    QListWidgetItem* item = ui->listWidget->currentItem();
    if (item) {
        flipOrientation(item);
    }
}

void GeomFillSurface::exitSelectionMode()
{
    selectionMode = None;
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
}

// ----------------------------------------------------------------------------

TaskGeomFillSurface::TaskGeomFillSurface(ViewProviderGeomFillSurface* vp,
                                         Surface::GeomFillSurface* obj)
{
    widget = new GeomFillSurface(vp, obj);
    widget->setWindowTitle(QObject::tr("Surface"));
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Surface_BSplineSurface"),
                                         widget->windowTitle(),
                                         true,
                                         nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

void TaskGeomFillSurface::setEditedObject(Surface::GeomFillSurface* obj)
{
    widget->setEditedObject(obj);
}

void TaskGeomFillSurface::open()
{
    widget->open();
}

bool TaskGeomFillSurface::accept()
{
    return widget->accept();
}

bool TaskGeomFillSurface::reject()
{
    return widget->reject();
}

}  // namespace SurfaceGui

#include "moc_TaskGeomFillSurface.cpp"
