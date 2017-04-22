/***************************************************************************
 *   Copyright (c) 2015 Balázs Bámer                                       *
 *                      Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <Gui/ViewProvider.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/SelectionObject.h>
#include <Base/Console.h>
#include <Gui/Control.h>
#include <Gui/BitmapFactory.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "TaskFilling.h"
#include "ui_TaskFilling.h"


using namespace SurfaceGui;

PROPERTY_SOURCE(SurfaceGui::ViewProviderFilling, PartGui::ViewProviderSpline)

namespace SurfaceGui {

void ViewProviderFilling::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit filling"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderSpline::setupContextMenu(menu, receiver, member);
}

bool ViewProviderFilling::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this sketch the
        // object unsets and sets its edit mode without closing
        // the task panel

        Surface::Filling* obj =  static_cast<Surface::Filling*>(this->getObject());

        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();

        // start the edit dialog
        if (dlg) {
            TaskFilling* tDlg = qobject_cast<TaskFilling*>(dlg);
            if (tDlg)
                tDlg->setEditedObject(obj);
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
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        QTimer::singleShot(0, &Gui::Control(), SLOT(closeDialog()));
    }
    else {
        PartGui::ViewProviderSpline::unsetEdit(ModNum);
    }
}

QIcon ViewProviderFilling::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap("BSplineSurf");
}

void ViewProviderFilling::highlightReferences(bool on)
{
    Surface::Filling* surface = static_cast<Surface::Filling*>(getObject());
    auto bounds = surface->Border.getSubListValues();
    for (auto it : bounds) {
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

                    for (auto jt : it.second) {
                        std::size_t idx = static_cast<std::size_t>(std::stoi(jt.substr(4)) - 1);
                        assert (idx < colors.size());
                        colors[idx] = App::Color(1.0,0.0,1.0); // magenta
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

class FillingPanel::ShapeSelection : public Gui::SelectionFilterGate
{
public:
    ShapeSelection(FillingPanel::SelectionMode mode, Surface::Filling* editedObject)
        : Gui::SelectionFilterGate(static_cast<Gui::SelectionFilter*>(nullptr))
        , mode(mode)
        , editedObject(editedObject)
    {
    }
    /**
      * Allow the user to pick only edges.
      */
    bool allow(App::Document*, App::DocumentObject* pObj, const char* sSubName)
    {
        // don't allow references to itself
        if (pObj == editedObject)
            return false;
        if (!pObj->isDerivedFrom(Part::Feature::getClassTypeId()))
            return false;

        if (!sSubName || sSubName[0] == '\0')
            return false;

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
    bool allowFace(App::DocumentObject* pObj, const char* sSubName)
    {
        std::string element(sSubName);
        if (element.substr(0,4) != "Face")
            return false;
        return true;
    }
    bool allowEdge(bool appendEdges, App::DocumentObject* pObj, const char* sSubName)
    {
        std::string element(sSubName);
        if (element.substr(0,4) != "Edge")
            return false;

        auto links = editedObject->Border.getSubListValues();
        for (auto it : links) {
            if (it.first == pObj) {
                for (auto jt : it.second) {
                    if (jt == sSubName)
                        return !appendEdges;
                }
            }
        }

        return appendEdges;
    }

private:
    FillingPanel::SelectionMode mode;
    Surface::Filling* editedObject;
};

// ----------------------------------------------------------------------------

FillingPanel::FillingPanel(ViewProviderFilling* vp, Surface::Filling* obj)
{
    ui = new Ui_TaskFilling();
    ui->setupUi(this);
    selectionMode = None;
    this->vp = vp;
    checkCommand = true;
    setEditedObject(obj);

    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    action->setShortcut(QString::fromLatin1("Del"));
    ui->listWidget->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onDeleteEdge()));
    ui->listWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
}

/*
 *  Destroys the object and frees any allocated resources
 */
FillingPanel::~FillingPanel()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

// stores object pointer, its old fill type and adjusts radio buttons according to it.
void FillingPanel::setEditedObject(Surface::Filling* obj)
{
    editedObject = obj;

    auto objects = editedObject->Border.getValues();
    auto element = editedObject->Border.getSubValues();
    auto it = objects.begin();
    auto jt = element.begin();

    App::Document* doc = editedObject->getDocument();
    for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
        QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
        ui->listWidget->addItem(item);

        QString text = QString::fromLatin1("%1.%2")
                .arg(QString::fromUtf8((*it)->Label.getValue()))
                .arg(QString::fromStdString(*jt));
        item->setText(text);

        QList<QVariant> data;
        data << QByteArray(doc->getName());
        data << QByteArray((*it)->getNameInDocument());
        data << QByteArray(jt->c_str());
        item->setData(Qt::UserRole, data);
    }
    attachDocument(Gui::Application::Instance->getDocument(doc));
}

void FillingPanel::changeEvent(QEvent *e)
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
    this->vp->highlightReferences(true);
    Gui::Selection().clearSelection();
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

bool FillingPanel::accept()
{
    this->vp->highlightReferences(false);
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

    if (editedObject->mustExecute())
        editedObject->recomputeFeature();
    if (!editedObject->isValid()) {
        QMessageBox::warning(this, tr("Invalid object"),
            QString::fromLatin1(editedObject->getStatusString()));
        return false;
    }

    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    Gui::Command::updateActive();
    return true;
}

bool FillingPanel::reject()
{
    this->vp->highlightReferences(false);
    selectionMode = None;
    Gui::Selection().rmvSelectionGate();

    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    Gui::Command::updateActive();
    return true;
}

void FillingPanel::on_lineInitFaceName_textChanged(const QString& text)
{
    if (text.isEmpty()) {
        checkOpenCommand();
        editedObject->InitialFace.setValue(nullptr);
        editedObject->recomputeFeature();
    }
}

void FillingPanel::on_buttonInitFace_clicked()
{
    selectionMode = InitFace;
    Gui::Selection().addSelectionGate(new ShapeSelection(selectionMode, editedObject));
}

void FillingPanel::on_buttonEdgeAdd_clicked()
{
    selectionMode = AppendEdge;
    Gui::Selection().addSelectionGate(new ShapeSelection(selectionMode, editedObject));
}

void FillingPanel::on_buttonEdgeRemove_clicked()
{
    selectionMode = RemoveEdge;
    Gui::Selection().addSelectionGate(new ShapeSelection(selectionMode, editedObject));
}

void FillingPanel::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == None)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        checkOpenCommand();
        if (selectionMode == InitFace) {
            Gui::SelectionObject sel(msg);
            QString text = QString::fromLatin1("%1.%2")
                    .arg(QString::fromUtf8(sel.getObject()->Label.getValue()))
                    .arg(QString::fromLatin1(msg.pSubName));
            ui->lineInitFaceName->setText(text);

            std::vector<std::string> subList;
            subList.push_back(msg.pSubName);
            editedObject->InitialFace.setValue(sel.getObject(), subList);
            //this->vp->highlightReferences(true);

            Gui::Selection().rmvSelectionGate();
            selectionMode = None;
        }
        else if (selectionMode == AppendEdge) {
            QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
            ui->listWidget->addItem(item);

            Gui::SelectionObject sel(msg);
            QString text = QString::fromLatin1("%1.%2")
                    .arg(QString::fromUtf8(sel.getObject()->Label.getValue()))
                    .arg(QString::fromLatin1(msg.pSubName));
            item->setText(text);

            QList<QVariant> data;
            data << QByteArray(msg.pDocName);
            data << QByteArray(msg.pObjectName);
            data << QByteArray(msg.pSubName);
            item->setData(Qt::UserRole, data);

            auto objects = editedObject->Border.getValues();
            objects.push_back(sel.getObject());
            auto element = editedObject->Border.getSubValues();
            element.push_back(msg.pSubName);
            editedObject->Border.setValues(objects, element);
            this->vp->highlightReferences(true);
        }
        else if (selectionMode == RemoveEdge) {
            Gui::SelectionObject sel(msg);
            QList<QVariant> data;
            data << QByteArray(msg.pDocName);
            data << QByteArray(msg.pObjectName);
            data << QByteArray(msg.pSubName);
            for (int i=0; i<ui->listWidget->count(); i++) {
                QListWidgetItem* item = ui->listWidget->item(i);
                if (item && item->data(Qt::UserRole) == data) {
                    ui->listWidget->takeItem(i);
                    delete item;
                }
            }

            this->vp->highlightReferences(false);
            App::DocumentObject* obj = sel.getObject();
            std::string sub = msg.pSubName;
            auto objects = editedObject->Border.getValues();
            auto element = editedObject->Border.getSubValues();
            auto it = objects.begin();
            auto jt = element.begin();
            for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
                if (*it == obj && *jt == sub) {
                    objects.erase(it);
                    element.erase(jt);
                    editedObject->Border.setValues(objects, element);
                    break;
                }
            }
            this->vp->highlightReferences(true);
        }

        editedObject->recomputeFeature();
        QTimer::singleShot(50, this, SLOT(clearSelection()));
    }
}

void FillingPanel::onDeleteEdge()
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
        auto objects = editedObject->Border.getValues();
        auto element = editedObject->Border.getSubValues();
        auto it = objects.begin();
        auto jt = element.begin();
        this->vp->highlightReferences(false);
        for (; it != objects.end() && jt != element.end(); ++it, ++jt) {
            if (*it == obj && *jt == sub) {
                objects.erase(it);
                element.erase(jt);
                editedObject->Border.setValues(objects, element);
                break;
            }
        }
        this->vp->highlightReferences(true);
    }
}

// ----------------------------------------------------------------------------

TaskFilling::TaskFilling(ViewProviderFilling* vp, Surface::Filling* obj)
{
    widget = new FillingPanel(vp, obj);
    widget->setWindowTitle(QObject::tr("Surface"));
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("BezSurf"),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskFilling::~TaskFilling()
{
    // automatically deleted in the sub-class
}

void TaskFilling::setEditedObject(Surface::Filling* obj)
{
    widget->setEditedObject(obj);
}

void TaskFilling::open()
{
    widget->open();
}

bool TaskFilling::accept()
{
    return widget->accept();
}

bool TaskFilling::reject()
{
    return widget->reject();
}

}

#include "moc_TaskFilling.cpp"
