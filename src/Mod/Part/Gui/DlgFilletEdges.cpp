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
# include <climits>
# include <sstream>
# include <QHeaderView>
# include <QItemDelegate>
# include <QItemSelectionModel>
# include <QLocale>
# include <QMessageBox>
# include <QTimer>
# include <QVBoxLayout>

# include <BRep_Tool.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Shape.hxx>
# include <TopExp.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
# include <TopTools_IndexedMapOfShape.hxx>

# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/details/SoLineDetail.h>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Window.h>
#include <Mod/Part/App/FeatureChamfer.h>
#include <Mod/Part/App/FeatureFillet.h>

#include "DlgFilletEdges.h"
#include "ui_DlgFilletEdges.h"
#include "SoBrepEdgeSet.h"
#include "SoBrepFaceSet.h"
#include "SoBrepPointSet.h"


using namespace PartGui;
namespace sp = std::placeholders;

FilletRadiusDelegate::FilletRadiusDelegate(QObject *parent) : QItemDelegate(parent)
{
}

QWidget *FilletRadiusDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */,
                                            const QModelIndex & index) const
{
    if (index.column() < 1)
        return nullptr;

    Gui::QuantitySpinBox *editor = new Gui::QuantitySpinBox(parent);
    editor->setUnit(Base::Unit::Length);
    editor->setMinimum(0.0);
    editor->setMaximum(INT_MAX);
    editor->setSingleStep(0.1);

    return editor;
}

void FilletRadiusDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    Base::Quantity value = index.model()->data(index, Qt::EditRole).value<Base::Quantity>();

    Gui::QuantitySpinBox *spinBox = static_cast<Gui::QuantitySpinBox*>(editor);
    spinBox->setValue(value);
}

void FilletRadiusDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                        const QModelIndex &index) const
{
    Gui::QuantitySpinBox *spinBox = static_cast<Gui::QuantitySpinBox*>(editor);
    spinBox->interpretText();
    //double value = spinBox->value();
    //QString value = QString::fromLatin1("%1").arg(spinBox->value(),0,'f',2);
    //QString value = QLocale().toString(spinBox->value().getValue(),'f',Base::UnitsApi::getDecimals());
    Base::Quantity value = spinBox->value();

    model->setData(index, QVariant::fromValue<Base::Quantity>(value), Qt::EditRole);
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

void FilletRadiusModel::updateCheckStates()
{
    // See http://www.qtcentre.org/threads/18856-Checkboxes-in-Treeview-do-not-get-refreshed?s=b0fea2bfc66da1098413ae9f2a651a68&p=93201#post93201
    Q_EMIT layoutChanged();
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
        Q_EMIT toggleCheckState(index);
    }
    return ok;
}

QVariant FilletRadiusModel::data(const QModelIndex& index, int role) const
{
    QVariant value = QStandardItemModel::data(index, role);
    if (role == Qt::DisplayRole && index.column() >= 1) {
        Base::Quantity q = value.value<Base::Quantity>();
        QString str = q.getUserString();
        return str;
    }
    return value;
}

// --------------------------------------------------------------

namespace PartGui {
    class EdgeFaceSelection : public Gui::SelectionFilterGate
    {
        bool allowEdge{true};
        App::DocumentObject*& object;
    public:
        explicit EdgeFaceSelection(App::DocumentObject*& obj)
            : Gui::SelectionFilterGate(nullPointer())
            , object(obj)
        {
        }
        void selectEdges()
        {
            allowEdge = true;
        }
        void selectFaces()
        {
            allowEdge = false;
        }
        bool allow(App::Document* /*pDoc*/, App::DocumentObject*pObj, const char*sSubName) override
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            if (allowEdge)
                return element.substr(0,4) == "Edge";
            else
                return element.substr(0,4) == "Face";
        }
    };
    class DlgFilletEdges::Private
    {
    public:
        App::DocumentObject* object;
        EdgeFaceSelection* selection;
        Part::FilletBase* fillet;
        QTimer* highlighttimer;
        FilletType filletType;
        std::vector<int> edge_ids;
        TopTools_IndexedMapOfShape all_edges;
        TopTools_IndexedMapOfShape all_faces;
        using Connection = boost::signals2::connection;
        Connection connectApplicationDeletedObject;
        Connection connectApplicationDeletedDocument;

        class SelectionObjectCompare
        {
        public:
            App::DocumentObject* obj;
            explicit SelectionObjectCompare(App::DocumentObject* obj) : obj(obj)
            {
            }
            bool operator()(const Gui::SelectionObject& sel) const
            {
                return (sel.getObject() == obj);
            }
        };
    };
}

/* TRANSLATOR PartGui::DlgFilletEdges */

DlgFilletEdges::DlgFilletEdges(FilletType type, Part::FilletBase* fillet, QWidget* parent, Qt::WindowFlags fl)
  : QWidget(parent, fl), ui(new Ui_DlgFilletEdges()), d(new Private())
{
    ui->setupUi(this);
    setupConnections();

    ui->filletStartRadius->setMaximum(INT_MAX);
    ui->filletStartRadius->setMinimum(0);
    ui->filletStartRadius->setUnit(Base::Unit::Length);

    ui->filletEndRadius->setMaximum(INT_MAX);
    ui->filletEndRadius->setMinimum(0);
    ui->filletEndRadius->setUnit(Base::Unit::Length);

    d->object = nullptr;
    d->selection = new EdgeFaceSelection(d->object);
    Gui::Selection().addSelectionGate(d->selection);

    d->fillet = fillet;
    //NOLINTBEGIN
    d->connectApplicationDeletedObject = App::GetApplication().signalDeletedObject
        .connect(std::bind(&DlgFilletEdges::onDeleteObject, this, sp::_1));
    d->connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument
        .connect(std::bind(&DlgFilletEdges::onDeleteDocument, this, sp::_1));
    //NOLINTEND
    // set tree view with three columns
    FilletRadiusModel* model = new FilletRadiusModel(this);
    connect(model, &FilletRadiusModel::toggleCheckState,
            this, &DlgFilletEdges::toggleCheckState);
    model->insertColumns(0,3);

    // timer for highlighting
    d->highlighttimer = new QTimer(this);
    d->highlighttimer->setSingleShot(true);
    connect(d->highlighttimer, &QTimer::timeout,
            this, &DlgFilletEdges::onHighlightEdges);

    d->filletType = type;
    if (d->filletType == DlgFilletEdges::CHAMFER) {
        ui->parameterName->setTitle(tr("Chamfer Parameter"));
        ui->labelfillet->setText(tr("Chamfer type"));
        ui->labelRadius->setText(tr("Length:"));
        ui->filletType->setItemText(0, tr("Equal distance"));
        ui->filletType->setItemText(1, tr("Two distances"));

        model->setHeaderData(0, Qt::Horizontal, tr("Edges to chamfer"), Qt::DisplayRole);
        model->setHeaderData(1, Qt::Horizontal, tr("Size"), Qt::DisplayRole);
        model->setHeaderData(2, Qt::Horizontal, tr("Size2"), Qt::DisplayRole);
    }
    else {
        ui->parameterName->setTitle(tr("Fillet Parameter"));
        ui->labelfillet->setText(tr("Fillet type"));
        model->setHeaderData(0, Qt::Horizontal, tr("Edges to fillet"), Qt::DisplayRole);
        model->setHeaderData(1, Qt::Horizontal, tr("Start radius"), Qt::DisplayRole);
        model->setHeaderData(2, Qt::Horizontal, tr("End radius"), Qt::DisplayRole);
    }
    ui->treeView->setRootIsDecorated(false);
    ui->treeView->setItemDelegate(new FilletRadiusDelegate(this));
    ui->treeView->setModel(model);

    QHeaderView* header = ui->treeView->header();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setDefaultAlignment(Qt::AlignLeft);
    header->setSectionsMovable(false);
    onFilletTypeActivated(0);
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

void DlgFilletEdges::setupConnections()
{
    connect(ui->shapeObject, qOverload<int>(&QComboBox::activated),
            this, &DlgFilletEdges::onShapeObjectActivated);
    connect(ui->selectEdges, &QRadioButton::toggled,
            this, &DlgFilletEdges::onSelectEdgesToggled);
    connect(ui->selectFaces, &QRadioButton::toggled,
            this, &DlgFilletEdges::onSelectFacesToggled);
    connect(ui->selectAllButton, &QPushButton::clicked,
            this, &DlgFilletEdges::onSelectAllButtonClicked);
    connect(ui->selectNoneButton, &QPushButton::clicked,
            this, &DlgFilletEdges::onSelectNoneButtonClicked);
    connect(ui->filletType, qOverload<int>(&QComboBox::activated),
            this, &DlgFilletEdges::onFilletTypeActivated);
    connect(ui->filletStartRadius,
            qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged),
            this, &DlgFilletEdges::onFilletStartRadiusValueChanged);
    connect(ui->filletEndRadius,
            qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged),
            this, &DlgFilletEdges::onFilletEndRadiusValueChanged);
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
            QString subelement = QString::fromLatin1(msg.pSubName);
            if (subelement.startsWith(QLatin1String("Edge"))) {
                onSelectEdge(subelement, msg.Type);
            }
            else if (subelement.startsWith(QLatin1String("Face"))) {
                d->selection->selectEdges();
                onSelectEdgesOfFace(subelement, msg.Type);
                d->selection->selectFaces();
            }
        }
    }

    if (msg.Type != Gui::SelectionChanges::SetPreselect &&
        msg.Type != Gui::SelectionChanges::RmvPreselect)
        d->highlighttimer->start(20);
}

void DlgFilletEdges::onHighlightEdges()
{
    Gui::ViewProvider* view = Gui::Application::Instance->getViewProvider(d->object);
    if (view) {
        // deselect all faces
        {
            SoSearchAction searchAction;
            searchAction.setType(PartGui::SoBrepFaceSet::getClassTypeId());
            searchAction.setInterest(SoSearchAction::FIRST);
            searchAction.apply(view->getRoot());
            SoPath* selectionPath = searchAction.getPath();
            if (selectionPath) {
                Gui::SoSelectionElementAction action(Gui::SoSelectionElementAction::None);
                action.apply(selectionPath);
            }
        }
        // deselect all points
        {
            SoSearchAction searchAction;
            searchAction.setType(PartGui::SoBrepPointSet::getClassTypeId());
            searchAction.setInterest(SoSearchAction::FIRST);
            searchAction.apply(view->getRoot());
            SoPath* selectionPath = searchAction.getPath();
            if (selectionPath) {
                Gui::SoSelectionElementAction action(Gui::SoSelectionElementAction::None);
                action.apply(selectionPath);
            }
        }
        // select the edges
        {
            SoSearchAction searchAction;
            searchAction.setType(PartGui::SoBrepEdgeSet::getClassTypeId());
            searchAction.setInterest(SoSearchAction::FIRST);
            searchAction.apply(view->getRoot());
            SoPath* selectionPath = searchAction.getPath();
            if (selectionPath) {
                ParameterGrp::handle hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("View");
                SbColor selectionColor(0.1f, 0.8f, 0.1f);
                unsigned long selection = (unsigned long)(selectionColor.getPackedValue());
                selection = hGrp->GetUnsigned("SelectionColor", selection);
                float transparency;
                selectionColor.setPackedValue((uint32_t)selection, transparency);

                // clear the selection first
                Gui::SoSelectionElementAction clear(Gui::SoSelectionElementAction::None);
                clear.apply(selectionPath);

                Gui::SoSelectionElementAction action(Gui::SoSelectionElementAction::Append);
                action.setColor(selectionColor);
                action.apply(selectionPath);

                QAbstractItemModel* model = ui->treeView->model();
                SoLineDetail detail;
                action.setElement(&detail);
                for (int i=0; i<model->rowCount(); ++i) {
                    QVariant value = model->index(i,0).data(Qt::CheckStateRole);
                    Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

                    // is item checked
                    if (checkState & Qt::Checked) {
                        // the index value of the edge
                        int id = model->index(i,0).data(Qt::UserRole).toInt();
                        detail.setLineIndex(id-1);
                        action.apply(selectionPath);
                    }
                }
            }
        }
    }
}

void DlgFilletEdges::onSelectEdge(const QString& subelement, int type)
{
    Gui::SelectionChanges::MsgType msgType = Gui::SelectionChanges::MsgType(type);
    QAbstractItemModel* model = ui->treeView->model();
    for (int i=0; i<model->rowCount(); ++i) {
        int id = model->data(model->index(i,0), Qt::UserRole).toInt();
        QString name = QString::fromLatin1("Edge%1").arg(id);
        if (name == subelement) {
            // ok, check the selected sub-element
            Qt::CheckState checkState =
                (msgType == Gui::SelectionChanges::AddSelection
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

void DlgFilletEdges::onSelectEdgesOfFace(const QString& subelement, int type)
{
    bool ok;
    int index = subelement.mid(4).toInt(&ok);
    if (ok) {
        try {
            const TopoDS_Shape& face = d->all_faces.FindKey(index);
            TopTools_IndexedMapOfShape mapOfEdges;
            TopExp::MapShapes(face, TopAbs_EDGE, mapOfEdges);

            for(int j = 1; j <= mapOfEdges.Extent(); ++j) {
                TopoDS_Edge edge = TopoDS::Edge(mapOfEdges.FindKey(j));
                int id = d->all_edges.FindIndex(edge);
                QString name = QString::fromLatin1("Edge%1").arg(id);
                onSelectEdge(name, type);
                Gui::SelectionChanges::MsgType msgType = Gui::SelectionChanges::MsgType(type);
                if (msgType == Gui::SelectionChanges::AddSelection) {
                    Gui::Selection().addSelection(d->object->getDocument()->getName(),
                        d->object->getNameInDocument(), (const char*)name.toLatin1());
                }
            }
        }
        catch (Standard_Failure&) {
        }
    }
}

void DlgFilletEdges::onDeleteObject(const App::DocumentObject& obj)
{
    if (d->fillet == &obj) {
        d->fillet = nullptr;
    }
    else if (d->fillet && d->fillet->Base.getValue() == &obj) {
        d->fillet = nullptr;
        d->object = nullptr;
        ui->shapeObject->setCurrentIndex(0);
        onShapeObjectActivated(0);
    }
    else if (d->object == &obj) {
        d->object = nullptr;
        ui->shapeObject->removeItem(ui->shapeObject->currentIndex());
        ui->shapeObject->setCurrentIndex(0);
        onShapeObjectActivated(0);
    }
    else {
        QString shape = QString::fromLatin1(obj.getNameInDocument());
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
            onShapeObjectActivated(0);
            setEnabled(false);
        }
    }
    else if (App::GetApplication().getActiveDocument() == &doc) {
        ui->shapeObject->setCurrentIndex(0);
        onShapeObjectActivated(0);
        setEnabled(false);
    }
}

void DlgFilletEdges::toggleCheckState(const QModelIndex& index)
{
    if (!d->object)
        return;
    QVariant check = index.data(Qt::CheckStateRole);
    int id = index.data(Qt::UserRole).toInt();
    QString name = QString::fromLatin1("Edge%1").arg(id);
    Qt::CheckState checkState = static_cast<Qt::CheckState>(check.toInt());

    bool block = this->blockSelection(true);

    // is item checked
    if (checkState & Qt::Checked) {
        App::Document* doc = d->object->getDocument();
        Gui::Selection().addSelection(doc->getName(),
            d->object->getNameInDocument(),
            (const char*)name.toLatin1());
    }
    else {
        App::Document* doc = d->object->getDocument();
        Gui::Selection().rmvSelection(doc->getName(),
            d->object->getNameInDocument(),
            (const char*)name.toLatin1());
    }

    this->blockSelection(block);
}

void DlgFilletEdges::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc)
        return;

    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType
        (Part::Feature::getClassTypeId());
    int index = 1;
    int current_index = 0;
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it!=objs.end(); ++it, ++index) {
        ui->shapeObject->addItem(QString::fromUtf8((*it)->Label.getValue()));
        ui->shapeObject->setItemData(index, QString::fromLatin1((*it)->getNameInDocument()));
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
        onShapeObjectActivated(current_index);
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
        onShapeObjectActivated(current_index);
        ui->shapeObject->setEnabled(false);

        double startRadius = 1;
        double endRadius = 1;
        bool twoRadii = false;

        std::vector<std::string> subElements;
        QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
        bool block = model->blockSignals(true); // do not call toggleCheckState
        for (const auto & et : e) {
            std::vector<int>::iterator it = std::find(d->edge_ids.begin(), d->edge_ids.end(), et.edgeid);
            if (it != d->edge_ids.end()) {
                int index = it - d->edge_ids.begin();
                model->setData(model->index(index, 0), Qt::Checked, Qt::CheckStateRole);
                //model->setData(model->index(index, 1), QVariant(QLocale().toString(et->radius1,'f',Base::UnitsApi::getDecimals())));
                //model->setData(model->index(index, 2), QVariant(QLocale().toString(et->radius2,'f',Base::UnitsApi::getDecimals())));
                model->setData(model->index(index, 1), QVariant::fromValue<Base::Quantity>(Base::Quantity(et.radius1, Base::Unit::Length)));
                model->setData(model->index(index, 2), QVariant::fromValue<Base::Quantity>(Base::Quantity(et.radius2, Base::Unit::Length)));

                startRadius = et.radius1;
                endRadius = et.radius2;
                if (startRadius != endRadius)
                    twoRadii = true;

                int id = model->index(index, 0).data(Qt::UserRole).toInt();
                std::stringstream str;
                str << "Edge" << id;
                subElements.push_back(str.str());
            }
        }
        model->blockSignals(block);

        // #0002273
        if (twoRadii) {
            ui->filletType->setCurrentIndex(1);
            onFilletTypeActivated(1);
        }

        // #0001746
        ui->filletStartRadius->blockSignals(true);
        ui->filletStartRadius->setValue(startRadius);
        ui->filletStartRadius->blockSignals(false);
        ui->filletEndRadius->blockSignals(true);
        ui->filletEndRadius->setValue(endRadius);
        ui->filletEndRadius->blockSignals(false);

        App::Document* doc = d->object->getDocument();
        // get current selection and their sub-elements
        //std::vector<Gui::SelectionObject> selObj = Gui::Selection().getSelectionEx(doc->getName());
        //std::vector<Gui::SelectionObject>::iterator selIt = std::find_if(selObj.begin(), selObj.end(),
        //    Private::SelectionObjectCompare(d->object));


         /*
          * Edit: the following check is no longer necessary, as Gui::Selection
          * will do the check
          *
        // If sub-objects are already selected then only add the un-selected parts.
        // This is important to avoid recursive calls of rmvSelection() which
        // invalidates the internal iterator (#0002200).
        if (selIt != selObj.end()) {
            std::vector<std::string> selElements = selIt->getSubNames();
            std::sort(selElements.begin(), selElements.end());
            std::sort(subElements.begin(), subElements.end());

            std::vector<std::string> complementary;
            std::back_insert_iterator<std::vector<std::string> > biit(complementary);
            std::set_difference(subElements.begin(), subElements.end(), selElements.begin(), selElements.end(), biit);
            subElements = complementary;
        }
        */

        Gui::Selection().clearSelection(doc->getName());

        if (!subElements.empty()) {
            Gui::Selection().addSelections(doc->getName(),
                d->object->getNameInDocument(),
                subElements);
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

void DlgFilletEdges::onShapeObjectActivated(int itemPos)
{
    d->object = nullptr;
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
    model->removeRows(0, model->rowCount());

    QByteArray name = ui->shapeObject->itemData(itemPos).toByteArray();
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc)
        return;
    App::DocumentObject* part = doc->getObject((const char*)name);
    if (part && part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        d->object = part;
        TopoDS_Shape myShape = static_cast<Part::Feature*>(part)->Shape.getValue();

        d->all_edges.Clear();
        TopExp::MapShapes(myShape, TopAbs_EDGE, d->all_edges);

        d->all_faces.Clear();
        TopExp::MapShapes(myShape, TopAbs_FACE, d->all_faces);

        // build up map edge->face
        TopTools_IndexedDataMapOfShapeListOfShape edge2Face;
        TopExp::MapShapesAndAncestors(myShape, TopAbs_EDGE, TopAbs_FACE, edge2Face);
        TopTools_IndexedMapOfShape mapOfShape;
        TopExp::MapShapes(myShape, TopAbs_EDGE, mapOfShape);

        // populate the model
        d->edge_ids.clear();
        for (int i=1; i<= edge2Face.Extent(); ++i) {
            // set the index value as user data to use it in accept()
            const TopTools_ListOfShape& los = edge2Face.FindFromIndex(i);
            if (los.Extent() == 2) {
                // set the index value as user data to use it in accept()
                const TopoDS_Shape& edge = edge2Face.FindKey(i);
                // Now check also the continuity to only allow C0-continious
                // faces
                const TopoDS_Shape& face1 = los.First();
                const TopoDS_Shape& face2 = los.Last();
                GeomAbs_Shape cont = BRep_Tool::Continuity(TopoDS::Edge(edge),
                                                           TopoDS::Face(face1),
                                                           TopoDS::Face(face2));
                if (cont == GeomAbs_C0) {
                    int id = mapOfShape.FindIndex(edge);
                    d->edge_ids.push_back(id);
                }
            }
        }

        model->insertRows(0, d->edge_ids.size());
        int index = 0;
        for (int id : d->edge_ids) {
            model->setData(model->index(index, 0), QVariant(tr("Edge%1").arg(id)));
            model->setData(model->index(index, 0), QVariant(id), Qt::UserRole);
          //model->setData(model->index(index, 1), QVariant(QLocale().toString(1.0,'f',Base::UnitsApi::getDecimals())));
          //model->setData(model->index(index, 2), QVariant(QLocale().toString(1.0,'f',Base::UnitsApi::getDecimals())));
            model->setData(model->index(index, 1), QVariant::fromValue<Base::Quantity>(Base::Quantity(1.0,Base::Unit::Length)));
            model->setData(model->index(index, 2), QVariant::fromValue<Base::Quantity>(Base::Quantity(1.0,Base::Unit::Length)));
            std::stringstream element;
            element << "Edge" << id;
            if (Gui::Selection().isSelected(part, element.str().c_str()))
                model->setData(model->index(index, 0), Qt::Checked, Qt::CheckStateRole);
            else
                model->setData(model->index(index, 0), Qt::Unchecked, Qt::CheckStateRole);
            index++;
        }
    }
}

void DlgFilletEdges::onSelectEdgesToggled(bool on)
{
    if (on) d->selection->selectEdges();
}

void DlgFilletEdges::onSelectFacesToggled(bool on)
{
    if (on) d->selection->selectFaces();
}

void DlgFilletEdges::onSelectAllButtonClicked()
{
    std::vector<std::string> subElements;
    FilletRadiusModel* model = static_cast<FilletRadiusModel*>(ui->treeView->model());
    bool block = model->blockSignals(true); // do not call toggleCheckState
    for (int i=0; i<model->rowCount(); ++i) {
        QModelIndex index = model->index(i,0);

        // is not yet checked?
        QVariant check = index.data(Qt::CheckStateRole);
        Qt::CheckState state = static_cast<Qt::CheckState>(check.toInt());
        if (state == Qt::Unchecked) {
            int id = index.data(Qt::UserRole).toInt();
            std::stringstream str;
            str << "Edge" << id;
            subElements.push_back(str.str());
        }

        Qt::CheckState checkState = Qt::Checked;
        QVariant value(static_cast<int>(checkState));
        model->setData(index, value, Qt::CheckStateRole);
    }
    model->blockSignals(block);
    model->updateCheckStates();

    if (d->object) {
        App::Document* doc = d->object->getDocument();
        Gui::Selection().addSelections(doc->getName(),
            d->object->getNameInDocument(),
            subElements);
    }
}

void DlgFilletEdges::onSelectNoneButtonClicked()
{
    FilletRadiusModel* model = static_cast<FilletRadiusModel*>(ui->treeView->model());
    bool block = model->blockSignals(true); // do not call toggleCheckState
    for (int i=0; i<model->rowCount(); ++i) {
        Qt::CheckState checkState = Qt::Unchecked;
        QVariant value(static_cast<int>(checkState));
        model->setData(model->index(i,0), value, Qt::CheckStateRole);
    }
    model->blockSignals(block);
    model->updateCheckStates();

    if (d->object) {
        App::Document* doc = d->object->getDocument();
        Gui::Selection().clearSelection(doc->getName());
    }
}

void DlgFilletEdges::onFilletTypeActivated(int index)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
    if (index == 0) {
        if (d->filletType == DlgFilletEdges::CHAMFER)
            model->setHeaderData(1, Qt::Horizontal, tr("Length"), Qt::DisplayRole);
        else
            model->setHeaderData(1, Qt::Horizontal, tr("Radius"), Qt::DisplayRole);
        ui->treeView->hideColumn(2);
        ui->filletEndRadius->hide();
    }
    else {
        if (d->filletType == DlgFilletEdges::CHAMFER)
            model->setHeaderData(1, Qt::Horizontal, tr("Start length"), Qt::DisplayRole);
        else
            model->setHeaderData(1, Qt::Horizontal, tr("Start radius"), Qt::DisplayRole);
        ui->treeView->showColumn(2);
        ui->filletEndRadius->show();
    }

    ui->treeView->resizeColumnToContents(0);
    ui->treeView->resizeColumnToContents(1);
    ui->treeView->resizeColumnToContents(2);
}

void DlgFilletEdges::onFilletStartRadiusValueChanged(const Base::Quantity& radius)
{
    QAbstractItemModel* model = ui->treeView->model();
    for (int i=0; i<model->rowCount(); ++i) {
        QVariant value = model->index(i,0).data(Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

        // is item checked
        if (checkState & Qt::Checked) {
            model->setData(model->index(i, 1), QVariant::fromValue<Base::Quantity>(radius));
        }
    }
}

void DlgFilletEdges::onFilletEndRadiusValueChanged(const Base::Quantity& radius)
{
    QAbstractItemModel* model = ui->treeView->model();
    for (int i=0; i<model->rowCount(); ++i) {
        QVariant value = model->index(i,0).data(Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

        // is item checked
        if (checkState & Qt::Checked) {
            model->setData(model->index(i, 2), QVariant::fromValue<Base::Quantity>(radius));
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
    type = QString::fromLatin1("Part::%1").arg(QString::fromLatin1(fillet.c_str()));

    if (d->fillet)
        name = QString::fromLatin1(d->fillet->getNameInDocument());
    else
        name = QString::fromLatin1(activeDoc->getUniqueObjectName(fillet.c_str()).c_str());

    activeDoc->openTransaction(fillet.c_str());
    QString code;
    if (!d->fillet) {
        code = QString::fromLatin1(
        "FreeCAD.ActiveDocument.addObject(\"%1\",\"%2\")\n"
        "FreeCAD.ActiveDocument.%2.Base = FreeCAD.ActiveDocument.%3\n")
        .arg(type, name, shape);
    }
    code += QString::fromLatin1("__fillets__ = []\n");
    for (int i=0; i<model->rowCount(); ++i) {
        QVariant value = model->index(i,0).data(Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

        // is item checked
        if (checkState & Qt::Checked) {
            // the index value of the edge
            int id = model->index(i,0).data(Qt::UserRole).toInt();
            Base::Quantity r1 = model->index(i,1).data(Qt::EditRole).value<Base::Quantity>();
            Base::Quantity r2 = r1;
            if (end_radius)
                r2 = model->index(i,2).data(Qt::EditRole).value<Base::Quantity>();
            code += QString::fromLatin1(
                "__fillets__.append((%1,%2,%3))\n")
                .arg(id)
                .arg(r1.getValue(),0,'f',Base::UnitsApi::getDecimals())
                .arg(r2.getValue(),0,'f',Base::UnitsApi::getDecimals());
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
    code += QString::fromLatin1(
        "FreeCAD.ActiveDocument.%1.Edges = __fillets__\n"
        "del __fillets__\n"
        "FreeCADGui.ActiveDocument.%2.Visibility = False\n")
        .arg(name, shape);
    Gui::Command::runCommand(Gui::Command::App, code.toLatin1());
    activeDoc->commitTransaction();
    activeDoc->recompute();
    if (d->fillet) {
        Gui::ViewProvider* vp;
        vp = Gui::Application::Instance->getViewProvider(d->fillet);
        if (vp) vp->show();
    }

    QByteArray to = name.toLatin1();
    QByteArray from = shape.toLatin1();
    Gui::Command::copyVisual(to, "LineColor", from);
    Gui::Command::copyVisual(to, "PointColor", from);
    return true;
}

// ---------------------------------------

FilletEdgesDialog::FilletEdgesDialog(DlgFilletEdges::FilletType type, Part::FilletBase* fillet, QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl)
{
    widget = new DlgFilletEdges(type, fillet, this);
    this->setWindowTitle(widget->windowTitle());

    QVBoxLayout* hboxLayout = new QVBoxLayout(this);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &FilletEdgesDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &FilletEdgesDialog::reject);

    hboxLayout->addWidget(widget);
    hboxLayout->addWidget(buttonBox);
}

FilletEdgesDialog::~FilletEdgesDialog() = default;

void FilletEdgesDialog::accept()
{
    if (widget->accept())
        QDialog::accept();
}

// ---------------------------------------

TaskFilletEdges::TaskFilletEdges(Part::Fillet* fillet)
{
    widget = new DlgFilletEdges(DlgFilletEdges::FILLET, fillet);
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Fillet"),
        widget->windowTitle(), true, nullptr);
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

DlgChamferEdges::DlgChamferEdges(Part::FilletBase* chamfer, QWidget* parent, Qt::WindowFlags fl)
  : DlgFilletEdges(DlgFilletEdges::CHAMFER, chamfer, parent, fl)
{
    this->setWindowTitle(tr("Chamfer Edges"));
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgChamferEdges::~DlgChamferEdges() = default;

const char* DlgChamferEdges::getFilletType() const
{
    return "Chamfer";
}

TaskChamferEdges::TaskChamferEdges(Part::Chamfer* chamfer)
{
    widget = new DlgChamferEdges(chamfer);
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Chamfer"),
        widget->windowTitle(), true, nullptr);
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
