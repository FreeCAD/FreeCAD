/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <algorithm>
# include <sstream>
# include <BRep_Tool.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Shape.hxx>
# include <TopExp.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <QItemDelegate>
# include <QLocale>
# include <QHeaderView>
# include <QMessageBox>
# include <QVBoxLayout>
# include <QItemSelection>
# include <QItemSelectionModel>
# include <boost/signal.hpp>
# include <boost/bind.hpp>
#endif

#include "DlgFilletEdges.h"
#include "ui_DlgFilletEdges.h"

#include "../App/PartFeature.h"
#include "../App/FeatureFillet.h"
#include "../App/FeatureChamfer.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/ViewProvider.h>

using namespace PartGui;

FilletRadiusDelegate::FilletRadiusDelegate(QObject *parent) : QItemDelegate(parent)
{
}

QWidget *FilletRadiusDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */,
                                            const QModelIndex & index) const
{
    if (index.column() < 1)
        return 0;

    QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
    editor->setMinimum(0.0);
    editor->setMaximum(100.0);
    editor->setSingleStep(0.1);

    return editor;
}

void FilletRadiusDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    double value = index.model()->data(index, Qt::EditRole).toDouble();

    QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
    spinBox->setValue(value);
}

void FilletRadiusDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                        const QModelIndex &index) const
{
    QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
    spinBox->interpretText();
    //double value = spinBox->value();
    //QString value = QString::fromAscii("%1").arg(spinBox->value(),0,'f',2);
    QString value = QLocale::system().toString(spinBox->value(),'f',2);

    model->setData(index, value, Qt::EditRole);
}

void FilletRadiusDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                                const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

// --------------------------------------------------------------

FilletRadiusModel::FilletRadiusModel(QObject * parent) : QStandardItemModel(parent)
{
}

Qt::ItemFlags FilletRadiusModel::flags (const QModelIndex & index) const
{
    Qt::ItemFlags fl = QStandardItemModel::flags(index);
    if (index.column() == 0)
        fl = fl | Qt::ItemIsUserCheckable;
    return fl;
}

bool FilletRadiusModel::setData (const QModelIndex & index, const QVariant & value, int role)
{
    bool ok = QStandardItemModel::setData(index, value, role);
    if (role == Qt::CheckStateRole) {
        toggleCheckState(index);
    }
    return ok;
}

// --------------------------------------------------------------

namespace PartGui {
    class EdgeSelection : public Gui::SelectionFilterGate
    {
        App::DocumentObject*& object;
    public:
        EdgeSelection(App::DocumentObject*& obj)
            : Gui::SelectionFilterGate((Gui::SelectionFilter*)0), object(obj)
        {
        }
        bool allow(App::Document*pDoc, App::DocumentObject*pObj, const char*sSubName)
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            return element.substr(0,4) == "Edge";
        }
    };
    class DlgFilletEdgesP
    {
    public:
        App::DocumentObject* object;
        EdgeSelection* selection;
        Part::FilletBase* fillet;
        typedef boost::signals::connection Connection;
        Connection connectApplicationDeletedObject;
        Connection connectApplicationDeletedDocument;
    };
};

/* TRANSLATOR PartGui::DlgFilletEdges */

DlgFilletEdges::DlgFilletEdges(Part::FilletBase* fillet, QWidget* parent, Qt::WFlags fl)
  : QWidget(parent, fl), ui(new Ui_DlgFilletEdges()), d(new DlgFilletEdgesP())
{
    ui->setupUi(this);

    d->object = 0;
    d->selection = new EdgeSelection(d->object);
    Gui::Selection().addSelectionGate(d->selection);

    d->fillet = fillet;
    d->connectApplicationDeletedObject = App::GetApplication().signalDeletedObject
        .connect(boost::bind(&DlgFilletEdges::onDeleteObject, this, _1));
    d->connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument
        .connect(boost::bind(&DlgFilletEdges::onDeleteDocument, this, _1));
    // set tree view with three columns
    QStandardItemModel* model = new FilletRadiusModel(this);
    connect(model, SIGNAL(toggleCheckState(const QModelIndex&)),
            this, SLOT(toggleCheckState(const QModelIndex&)));
    model->insertColumns(0,3);
    model->setHeaderData(0, Qt::Horizontal, tr("Edges to fillet"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, tr("Start radius"), Qt::DisplayRole);
    model->setHeaderData(2, Qt::Horizontal, tr("End radius"), Qt::DisplayRole);
    ui->treeView->setRootIsDecorated(false);
    ui->treeView->setItemDelegate(new FilletRadiusDelegate(this));
    ui->treeView->setModel(model);

    QHeaderView* header = ui->treeView->header();
    header->setResizeMode(0, QHeaderView::Stretch);
    header->setDefaultAlignment(Qt::AlignLeft);
    header->setMovable(false);
    on_filletType_activated(0);
    findShapes();
}

/*  
 *  Destroys the object and frees any allocated resources
 */
DlgFilletEdges::~DlgFilletEdges()
{
    // no need to delete child widgets, Qt does it all for us
    d->connectApplicationDeletedDocument.disconnect();
    d->connectApplicationDeletedObject.disconnect();
    Gui::Selection().rmvSelectionGate();
}

void DlgFilletEdges::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // no object selected in the combobox or no sub-element was selected
    if (!d->object || !msg.pSubName)
        return;
    if (msg.Type == Gui::SelectionChanges::AddSelection ||
        msg.Type == Gui::SelectionChanges::RmvSelection) {
        // when adding a sub-element to the selection check
        // whether this is the currently handled object
        App::Document* doc = d->object->getDocument();
        std::string docname = doc->getName();
        std::string objname = d->object->getNameInDocument();
        if (docname==msg.pDocName && objname==msg.pObjectName) {
            QString subelement = QString::fromAscii(msg.pSubName);
            QAbstractItemModel* model = ui->treeView->model();
            for (int i=0; i<model->rowCount(); ++i) {
                int id = model->data(model->index(i,0), Qt::UserRole).toInt();
                QString name = QString::fromAscii("Edge%1").arg(id);
                if (name == subelement) {
                    // ok, check the selected sub-element
                    Qt::CheckState checkState =
                        (msg.Type == Gui::SelectionChanges::AddSelection
                        ? Qt::Checked : Qt::Unchecked);
                    QVariant value(static_cast<int>(checkState));
                    QModelIndex index = model->index(i,0);
                    model->setData(index, value, Qt::CheckStateRole);
                    // select the item
                    ui->treeView->selectionModel()->setCurrentIndex(index,QItemSelectionModel::NoUpdate);
                    QItemSelection selection(index, model->index(i,1));
                    ui->treeView->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
                    ui->treeView->update();
                    break;
                }
            }
        }
    }
}

void DlgFilletEdges::onDeleteObject(const App::DocumentObject& obj)
{
    if (d->fillet == &obj) {
        d->fillet = 0;
    }
    else if (d->fillet && d->fillet->Base.getValue() == &obj) {
        d->fillet = 0;
        d->object = 0;
        ui->shapeObject->setCurrentIndex(0);
        on_shapeObject_activated(0);
    }
    else if (d->object == &obj) {
        d->object = 0;
        ui->shapeObject->removeItem(ui->shapeObject->currentIndex());
        ui->shapeObject->setCurrentIndex(0);
        on_shapeObject_activated(0);
    }
    else {
        QString shape = QString::fromAscii(obj.getNameInDocument());
        // start from the second item
        for (int i=1; i<ui->shapeObject->count(); i++) {
            if (ui->shapeObject->itemData(i).toString() == shape) {
                ui->shapeObject->removeItem(i);
                break;
            }
        }
    }
}

void DlgFilletEdges::onDeleteDocument(const App::Document& doc)
{
    if (d->object) {
        if (d->object->getDocument() == &doc) {
            ui->shapeObject->setCurrentIndex(0);
            on_shapeObject_activated(0);
            setEnabled(false);
        }
    }
    else if (App::GetApplication().getActiveDocument() == &doc) {
        ui->shapeObject->setCurrentIndex(0);
        on_shapeObject_activated(0);
        setEnabled(false);
    }
}

void DlgFilletEdges::toggleCheckState(const QModelIndex& index)
{
    if (!d->object)
        return;
    QVariant check = index.data(Qt::CheckStateRole);
    int id = index.data(Qt::UserRole).toInt();
    QString name = QString::fromAscii("Edge%1").arg(id);
    Qt::CheckState checkState = static_cast<Qt::CheckState>(check.toInt());

    bool block = this->blockConnection(false);

    // is item checked
    if (checkState & Qt::Checked) {
        App::Document* doc = d->object->getDocument();
        Gui::Selection().addSelection(doc->getName(),
            d->object->getNameInDocument(),
            (const char*)name.toAscii());
    }
    else {
        App::Document* doc = d->object->getDocument();
        Gui::Selection().rmvSelection(doc->getName(),
            d->object->getNameInDocument(),
            (const char*)name.toAscii());
    }

    this->blockConnection(block);
}

void DlgFilletEdges::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) return;

    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType
        (Part::Feature::getClassTypeId());
    int index = 1;
    int current_index = 0;
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it!=objs.end(); ++it, ++index) {
        ui->shapeObject->addItem(QString::fromUtf8((*it)->Label.getValue()));
        ui->shapeObject->setItemData(index, QString::fromAscii((*it)->getNameInDocument()));
        if (current_index == 0) {
            if (Gui::Selection().isSelected(*it)) {
                current_index = index;
            }
        }
    }

    // if only one object is in the document then simply use that
    if (objs.size() == 1)
        current_index = 1;

    if (current_index > 0) {
        ui->shapeObject->setCurrentIndex(current_index);
        on_shapeObject_activated(current_index);
    }

    // if an existing fillet object is given start the edit mode
    if (d->fillet) {
        setupFillet(objs);
    }
}

void DlgFilletEdges::setupFillet(const std::vector<App::DocumentObject*>& objs)
{
    App::DocumentObject* base = d->fillet->Base.getValue();
    const std::vector<Part::FilletElement>& e = d->fillet->Edges.getValues();
    std::vector<App::DocumentObject*>::const_iterator it = std::find(objs.begin(), objs.end(), base);
    if (it != objs.end()) {
        // toggle visibility
        Gui::ViewProvider* vp;
        vp = Gui::Application::Instance->getViewProvider(d->fillet);
        if (vp) vp->hide();
        vp = Gui::Application::Instance->getViewProvider(base);
        if (vp) vp->show();

        int current_index = (it - objs.begin()) + 1;
        ui->shapeObject->setCurrentIndex(current_index);
        on_shapeObject_activated(current_index);
        ui->shapeObject->setEnabled(false);
        QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
        for (std::vector<Part::FilletElement>::const_iterator et = e.begin(); et != e.end(); ++et) {
            int index = et->edgeid-1;
            model->setData(model->index(index, 0), Qt::Checked, Qt::CheckStateRole);
            model->setData(model->index(index, 1), QVariant(QLocale::system().toString(et->radius1,'f',2)));
            model->setData(model->index(index, 2), QVariant(QLocale::system().toString(et->radius2,'f',2)));
        }
    }
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgFilletEdges::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        int index = ui->shapeObject->currentIndex();
        // only get the items from index 1 on since the first one will be added automatically
        int count = ui->shapeObject->count() - 1;
        QStringList text;
        QList<QVariant> data;
        for (int i=0; i<count; i++) {
            text << ui->shapeObject->itemText(i+1);
            data << ui->shapeObject->itemData(i+1);
        }

        ui->retranslateUi(this);
        for (int i=0; i<count; i++) {
            ui->shapeObject->addItem(text.at(i));
            ui->shapeObject->setItemData(i+1, data.at(i));
        }

        ui->shapeObject->setCurrentIndex(index);
        QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
        count = model->rowCount();
        for (int i=0; i<count; i++) {
            int id = model->data(model->index(i, 0), Qt::UserRole).toInt();
            model->setData(model->index(i, 0), QVariant(tr("Edge%1").arg(id)));
        }
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgFilletEdges::on_shapeObject_activated(int index)
{
    d->object = 0;
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
    model->removeRows(0, model->rowCount());

    QByteArray name = ui->shapeObject->itemData(index).toByteArray();
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc)
        return;
    App::DocumentObject* part = doc->getObject((const char*)name);
    if (part && part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        d->object = part;
        TopoDS_Shape myShape = static_cast<Part::Feature*>(part)->Shape.getValue();
        // build up map edge->face
        TopTools_IndexedDataMapOfShapeListOfShape edge2Face;
        TopExp::MapShapesAndAncestors(myShape, TopAbs_EDGE, TopAbs_FACE, edge2Face);
        TopTools_IndexedMapOfShape mapOfShape;
        TopExp::MapShapes(myShape, TopAbs_EDGE, mapOfShape);

        // populate the model
        std::vector<int> edge_ids;
        for (int i=1; i<= edge2Face.Extent(); ++i) {
            // set the index value as user data to use it in accept()
            const TopTools_ListOfShape& los = edge2Face.FindFromIndex(i);
            if (los.Extent() == 2) {
                // set the index value as user data to use it in accept()
                const TopoDS_Shape& edge = edge2Face.FindKey(i);
                const TopTools_ListOfShape& los = edge2Face.FindFromIndex(i);
                if (los.Extent() == 2) {
                    // Now check also the continuity to only allow C0-continious
                    // faces
                    const TopoDS_Shape& face1 = los.First();
                    const TopoDS_Shape& face2 = los.Last();
                    GeomAbs_Shape cont = BRep_Tool::Continuity(TopoDS::Edge(edge),
                                                               TopoDS::Face(face1),
                                                               TopoDS::Face(face2));
                    if (cont == GeomAbs_C0) {
                        int id = mapOfShape.FindIndex(edge);
                        edge_ids.push_back(id);
                    }
                }
            }
        }

        model->insertRows(0, edge_ids.size());
        int index = 0;
        for (std::vector<int>::iterator it = edge_ids.begin(); it != edge_ids.end(); ++it) {
            model->setData(model->index(index, 0), QVariant(tr("Edge%1").arg(*it)));
            model->setData(model->index(index, 0), QVariant(*it), Qt::UserRole);
            model->setData(model->index(index, 1), QVariant(QLocale::system().toString(1.0,'f',2)));
            model->setData(model->index(index, 2), QVariant(QLocale::system().toString(1.0,'f',2)));
            std::stringstream element;
            element << "Edge" << *it;
            if (Gui::Selection().isSelected(part, element.str().c_str()))
                model->setData(model->index(index, 0), Qt::Checked, Qt::CheckStateRole);
            else
                model->setData(model->index(index, 0), Qt::Unchecked, Qt::CheckStateRole);
            index++;
        }
    }
}

void DlgFilletEdges::on_selectAllButton_clicked()
{
    QAbstractItemModel* model = ui->treeView->model();
    for (int i=0; i<model->rowCount(); ++i) {
        Qt::CheckState checkState = Qt::Checked;
        QVariant value(static_cast<int>(checkState));
        model->setData(model->index(i,0), value, Qt::CheckStateRole);
    }
}

void DlgFilletEdges::on_selectNoneButton_clicked()
{
    QAbstractItemModel* model = ui->treeView->model();
    for (int i=0; i<model->rowCount(); ++i) {
        Qt::CheckState checkState = Qt::Unchecked;
        QVariant value(static_cast<int>(checkState));
        model->setData(model->index(i,0), value, Qt::CheckStateRole);
    }
}

void DlgFilletEdges::on_filletType_activated(int index)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
    if (index == 0) {
        model->setHeaderData(1, Qt::Horizontal, tr("Radius"), Qt::DisplayRole);
        ui->treeView->hideColumn(2);
        ui->filletEndRadius->hide();
    }
    else {
        model->setHeaderData(1, Qt::Horizontal, tr("Start radius"), Qt::DisplayRole);
        ui->treeView->showColumn(2);
        ui->filletEndRadius->show();
    }

    ui->treeView->resizeColumnToContents(0);
    ui->treeView->resizeColumnToContents(1);
    ui->treeView->resizeColumnToContents(2);
}

void DlgFilletEdges::on_filletStartRadius_valueChanged(double radius)
{
    QAbstractItemModel* model = ui->treeView->model();
    QString text = QLocale::system().toString(radius,'f',2);
    for (int i=0; i<model->rowCount(); ++i) {
        QVariant value = model->index(i,0).data(Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

        // is item checked
        if (checkState & Qt::Checked) {
            model->setData(model->index(i, 1), QVariant(text));
        }
    }
}

void DlgFilletEdges::on_filletEndRadius_valueChanged(double radius)
{
    QAbstractItemModel* model = ui->treeView->model();
    QString text = QLocale::system().toString(radius,'f',2);
    for (int i=0; i<model->rowCount(); ++i) {
        QVariant value = model->index(i,0).data(Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

        // is item checked
        if (checkState & Qt::Checked) {
            model->setData(model->index(i, 2), QVariant(text));
        }
    }
}

const char* DlgFilletEdges::getFilletType() const
{
    return "Fillet";
}

bool DlgFilletEdges::accept()
{
    if (!d->object) {
        QMessageBox::warning(this, tr("No shape selected"),
            tr("No valid shape is selected.\n"
               "Please select a valid shape in the drop-down box first."));
        return false;
    }
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    QAbstractItemModel* model = ui->treeView->model();
    bool end_radius = !ui->treeView->isColumnHidden(2);
    bool todo = false;

    QString shape, type, name;
    std::string fillet = getFilletType();
    int index = ui->shapeObject->currentIndex();
    shape = ui->shapeObject->itemData(index).toString();
    type = QString::fromAscii("Part::%1").arg(QString::fromAscii(fillet.c_str()));

    if (d->fillet)
        name = QString::fromAscii(d->fillet->getNameInDocument());
    else
        name = QString::fromAscii(activeDoc->getUniqueObjectName(fillet.c_str()).c_str());

    activeDoc->openTransaction(fillet.c_str());
    QString code;
    if (!d->fillet) {
        code = QString::fromAscii(
        "FreeCAD.ActiveDocument.addObject(\"%1\",\"%2\")\n"
        "FreeCAD.ActiveDocument.%2.Base = FreeCAD.ActiveDocument.%3\n")
        .arg(type).arg(name).arg(shape);
    }
    code += QString::fromAscii("__fillets__ = []\n");
    for (int i=0; i<model->rowCount(); ++i) {
        QVariant value = model->index(i,0).data(Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

        // is item checked
        if (checkState & Qt::Checked) {
            // the index value of the edge
            int id = model->index(i,0).data(Qt::UserRole).toInt();
            double r1 = model->index(i,1).data().toDouble();
            double r2 = r1;
            if (end_radius)
                r2 = model->index(i,2).data().toDouble();
            code += QString::fromAscii(
                "__fillets__.append((%1,%2,%3))\n")
                .arg(id).arg(r1,0,'f',2).arg(r2,0,'f',2);
            todo = true;
        }
    }

    if (!todo) {
        QMessageBox::warning(this, tr("No edge selected"),
            tr("No edge entity is checked to fillet.\n"
               "Please check one or more edge entities first."));
        return false;
    }

    Gui::WaitCursor wc;
    code += QString::fromAscii(
        "FreeCAD.ActiveDocument.%1.Edges = __fillets__\n"
        "del __fillets__\n"
        "FreeCADGui.ActiveDocument.%2.Visibility = False\n")
        .arg(name).arg(shape);
    Gui::Application::Instance->runPythonCode((const char*)code.toAscii());
    activeDoc->commitTransaction();
    activeDoc->recompute();
    if (d->fillet) {
        Gui::ViewProvider* vp;
        vp = Gui::Application::Instance->getViewProvider(d->fillet);
        if (vp) vp->show();
    }

    QByteArray to = name.toAscii();
    QByteArray from = shape.toAscii();
    Gui::Command::copyVisual(to, "ShapeColor", from);
    Gui::Command::copyVisual(to, "LineColor", from);
    Gui::Command::copyVisual(to, "PointColor", from);
    return true;
}

// ---------------------------------------

FilletEdgesDialog::FilletEdgesDialog(Part::FilletBase* fillet, QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl)
{
    widget = new DlgFilletEdges(fillet, this);
    this->setWindowTitle(widget->windowTitle());

    QVBoxLayout* hboxLayout = new QVBoxLayout(this);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    hboxLayout->addWidget(widget);
    hboxLayout->addWidget(buttonBox);
}

FilletEdgesDialog::~FilletEdgesDialog()
{
}

void FilletEdgesDialog::accept()
{
    if (widget->accept())
        QDialog::accept();
}

// ---------------------------------------

TaskFilletEdges::TaskFilletEdges(Part::Fillet* fillet)
{
    widget = new DlgFilletEdges(fillet);
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Fillet"),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskFilletEdges::~TaskFilletEdges()
{
    // automatically deleted in the sub-class
}

void TaskFilletEdges::open()
{
}

void TaskFilletEdges::clicked(int)
{
}

bool TaskFilletEdges::accept()
{
    bool ok = widget->accept();
    if (ok)
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    return ok;
}

bool TaskFilletEdges::reject()
{
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    return true;
}

// --------------------------------------------------------------

/* TRANSLATOR PartGui::DlgChamferEdges */

DlgChamferEdges::DlgChamferEdges(Part::FilletBase* chamfer, QWidget* parent, Qt::WFlags fl)
  : DlgFilletEdges(chamfer, parent, fl)
{
    this->setWindowTitle(tr("Chamfer Edges"));
}

/*  
 *  Destroys the object and frees any allocated resources
 */
DlgChamferEdges::~DlgChamferEdges()
{
}

const char* DlgChamferEdges::getFilletType() const
{
    return "Chamfer";
}

TaskChamferEdges::TaskChamferEdges(Part::Chamfer* chamfer)
{
    widget = new DlgChamferEdges(chamfer);
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Chamfer"),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskChamferEdges::~TaskChamferEdges()
{
    // automatically deleted in the sub-class
}

void TaskChamferEdges::open()
{
}

void TaskChamferEdges::clicked(int)
{
}

bool TaskChamferEdges::accept()
{
    bool ok = widget->accept();
    if (ok)
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    return ok;
}

bool TaskChamferEdges::reject()
{
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    return true;
}

#include "moc_DlgFilletEdges.cpp"
