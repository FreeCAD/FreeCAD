/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <cmath>
# include <QRegExp>
# include <QString>
#endif

#include "TaskSketcherConstrains.h"
#include "ui_TaskSketcherConstrains.h"
#include "EditDatumDialog.h"
#include "ViewProviderSketch.h"

#include <Mod/Sketcher/App/SketchObject.h>

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/BitmapFactory.h>
#include <boost/bind.hpp>
#include <Gui/Command.h>

using namespace SketcherGui;
using namespace Gui::TaskView;

/// Inserts a QAction into an existing menu
/// ICONSTR is the string of the icon in the resource file
/// NAMESTR is the text appearing in the contextual menuAction
/// CMDSTR is the string registered in the commandManager
/// FUNC is the name of the member function to be executed on selection of the menu item
/// ACTSONSELECTION is a true/false value to activate the command only if a selection is made
#define CONTEXT_ITEM(ICONSTR,NAMESTR,CMDSTR,FUNC,ACTSONSELECTION)                       \
QIcon icon_ ## FUNC( Gui::BitmapFactory().pixmap(ICONSTR) );                    \
    QAction* constr_ ## FUNC = menu.addAction(icon_ ## FUNC,tr(NAMESTR), this, SLOT(FUNC()),    \
        QKeySequence(QString::fromUtf8(Gui::Application::Instance->commandManager().getCommandByName(CMDSTR)->getAccel())));        \
    if(ACTSONSELECTION) constr_ ## FUNC->setEnabled(!items.isEmpty()); else constr_ ## FUNC->setEnabled(true);

/// Defines the member function corresponding to the CONTEXT_ITEM macro
#define CONTEXT_MEMBER_DEF(CMDSTR,FUNC)                             \
void ConstraintView::FUNC(){                               \
   Gui::Application::Instance->commandManager().runCommandByName(CMDSTR);}

// helper class to store additional information about the listWidget entry.
class ConstraintItem : public QListWidgetItem
{
public:
    ConstraintItem(const QIcon & icon, const QString & text,int ConstNbr,Sketcher::ConstraintType t)
        : QListWidgetItem(icon,text),ConstraintNbr(ConstNbr),Type(t)
    {
        this->setFlags(this->flags() | Qt::ItemIsEditable);
    }
    ConstraintItem(const QString & text,int ConstNbr,Sketcher::ConstraintType t)
        : QListWidgetItem(text),ConstraintNbr(ConstNbr),Type(t)
    {
        this->setFlags(this->flags() | Qt::ItemIsEditable);
    }
    ~ConstraintItem()
    {
    }
    void setData(int role, const QVariant & value)
    {
        if (role == Qt::UserRole) {
            quantity = value;
            return;
        }
        QListWidgetItem::setData(role, value);
    }
    QVariant data (int role) const
    {
        if (role == Qt::UserRole) {
            return quantity;
        }
        else if (role == Qt::DisplayRole && quantity.isValid()) {
            return quantity;
        }
        return QListWidgetItem::data(role);
    }

    int ConstraintNbr;
    Sketcher::ConstraintType Type;

private:
    QVariant quantity;
};

ConstraintView::ConstraintView(QWidget *parent)
    : QListWidget(parent)
{
}

ConstraintView::~ConstraintView()
{
}

void ConstraintView::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu;
    QListWidgetItem* item = currentItem();
    QList<QListWidgetItem *> items = selectedItems();
    
    CONTEXT_ITEM("Sketcher_SelectElementsAssociatedWithConstraints","Select Elements","Sketcher_SelectElementsAssociatedWithConstraints",doSelectConstraints,true)
    
    menu.addSeparator();

    QAction* change = menu.addAction(tr("Change value"), this, SLOT(modifyCurrentItem()));
    QVariant v = item ? item->data(Qt::UserRole) : QVariant();
    change->setEnabled(v.isValid());

    QAction* rename = menu.addAction(tr("Rename"), this, SLOT(renameCurrentItem())
#ifndef Q_WS_MAC // on Mac F2 doesn't seem to trigger an edit signal
        ,QKeySequence(Qt::Key_F2)
#endif
        );
    rename->setEnabled(item != 0);

    QAction* remove = menu.addAction(tr("Delete"), this, SLOT(deleteSelectedItems()),
        QKeySequence(QKeySequence::Delete));
    remove->setEnabled(!items.isEmpty());
    menu.exec(event->globalPos());
}

CONTEXT_MEMBER_DEF("Sketcher_SelectElementsAssociatedWithConstraints",doSelectConstraints)

void ConstraintView::modifyCurrentItem()
{
    /*emit*/itemActivated(currentItem());
}

void ConstraintView::renameCurrentItem()
{
    QListWidgetItem* item = currentItem();
    if (item)
        editItem(item);
}

void ConstraintView::deleteSelectedItems()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) return;

    doc->openTransaction("Delete");
    std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx(doc->getName());
    for (std::vector<Gui::SelectionObject>::iterator ft = sel.begin(); ft != sel.end(); ++ft) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(ft->getObject());
        if (vp) {
            vp->onDelete(ft->getSubNames());
        }
    }
    doc->commitTransaction();
}

// ----------------------------------------------------------------------------

TaskSketcherConstrains::TaskSketcherConstrains(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Constraints"),true, 0)
    , sketchView(sketchView), inEditMode(false)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskSketcherConstrains();
    ui->setupUi(proxy);
    ui->listWidgetConstraints->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->listWidgetConstraints->setEditTriggers(QListWidget::EditKeyPressed);
    //QMetaObject::connectSlotsByName(this);

    // connecting the needed signals
    QObject::connect(
        ui->comboBoxFilter, SIGNAL(currentIndexChanged(int)),
        this              , SLOT  (on_comboBoxFilter_currentIndexChanged(int))
       );
    QObject::connect(
        ui->listWidgetConstraints, SIGNAL(itemSelectionChanged()),
        this                     , SLOT  (on_listWidgetConstraints_itemSelectionChanged())
       );
    QObject::connect(
        ui->listWidgetConstraints, SIGNAL(itemActivated(QListWidgetItem *)),
        this                     , SLOT  (on_listWidgetConstraints_itemActivated(QListWidgetItem *))
       );
    QObject::connect(
        ui->listWidgetConstraints, SIGNAL(itemChanged(QListWidgetItem *)),
        this                     , SLOT  (on_listWidgetConstraints_itemChanged(QListWidgetItem *))
       );

    connectionConstraintsChanged = sketchView->signalConstraintsChanged.connect(
        boost::bind(&SketcherGui::TaskSketcherConstrains::slotConstraintsChanged, this));

    this->groupLayout()->addWidget(proxy);

    slotConstraintsChanged();
}

TaskSketcherConstrains::~TaskSketcherConstrains()
{
    connectionConstraintsChanged.disconnect();
    delete ui;
}

void TaskSketcherConstrains::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    std::string temp;
    if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        ui->listWidgetConstraints->blockSignals(true);
        ui->listWidgetConstraints->clearSelection ();
        ui->listWidgetConstraints->blockSignals(false);
    }
    else if (msg.Type == Gui::SelectionChanges::AddSelection ||
             msg.Type == Gui::SelectionChanges::RmvSelection) {
        bool select = (msg.Type == Gui::SelectionChanges::AddSelection);
        // is it this object??
        if (strcmp(msg.pDocName,sketchView->getSketchObject()->getDocument()->getName())==0 &&
            strcmp(msg.pObjectName,sketchView->getSketchObject()->getNameInDocument())== 0) {
            if (msg.pSubName) {
                QRegExp rx(QString::fromAscii("^Constraint(\\d+)$"));
                QString expr = QString::fromAscii(msg.pSubName);
                int pos = expr.indexOf(rx);
                if (pos > -1) {
                    bool ok;
                    int ConstrId = rx.cap(1).toInt(&ok) - 1;
                    if (ok) {
                        int countItems = ui->listWidgetConstraints->count();
                        for (int i=0; i < countItems; i++) {
                            ConstraintItem* item = static_cast<ConstraintItem*>
                                (ui->listWidgetConstraints->item(i));
                            if (item->ConstraintNbr == ConstrId) {
                                ui->listWidgetConstraints->blockSignals(true);
                                item->setSelected(select);
                                ui->listWidgetConstraints->blockSignals(false);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    else if (msg.Type == Gui::SelectionChanges::SetSelection) {
        // do nothing here
    }
}

void TaskSketcherConstrains::on_comboBoxFilter_currentIndexChanged(int)
{
    slotConstraintsChanged();
}

void TaskSketcherConstrains::on_listWidgetConstraints_itemSelectionChanged(void)
{
    std::string doc_name = sketchView->getSketchObject()->getDocument()->getName();
    std::string obj_name = sketchView->getSketchObject()->getNameInDocument();

    bool block = this->blockConnection(true); // avoid to be notified by itself
    Gui::Selection().clearSelection();
    QList<QListWidgetItem *> items = ui->listWidgetConstraints->selectedItems();
    for (QList<QListWidgetItem *>::iterator it = items.begin(); it != items.end(); ++it) {
        std::stringstream ss;
        ss << "Constraint" << static_cast<ConstraintItem*>(*it)->ConstraintNbr + 1;
        Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
    }
    this->blockConnection(block);
}

void TaskSketcherConstrains::on_listWidgetConstraints_itemActivated(QListWidgetItem *item)
{
    ConstraintItem *it = dynamic_cast<ConstraintItem*>(item);
    if (!item) return;

    // if its the right constraint
    if (it->Type == Sketcher::Distance ||
        it->Type == Sketcher::DistanceX ||
        it->Type == Sketcher::DistanceY ||
        it->Type == Sketcher::Radius ||
        it->Type == Sketcher::Angle ||
        it->Type == Sketcher::SnellsLaw) {

        EditDatumDialog *editDatumDialog = new EditDatumDialog(this->sketchView, it->ConstraintNbr);
        editDatumDialog->exec(false);
        delete editDatumDialog;
    }
}

void TaskSketcherConstrains::on_listWidgetConstraints_itemChanged(QListWidgetItem *item)
{
    if (!item || inEditMode)
        return;
    ConstraintItem *it = dynamic_cast<ConstraintItem*>(item);
    const std::vector< Sketcher::Constraint * > &vals = sketchView->getSketchObject()->Constraints.getValues();
    Sketcher::Constraint* v = vals[it->ConstraintNbr];

    QString name = it->data(Qt::EditRole).toString();
    if (name.isEmpty())
        name = QString::fromLatin1("Constraint%1").arg(it->ConstraintNbr+1);

    QString unitStr;
    switch(v->Type) {
    case Sketcher::Distance:
    case Sketcher::DistanceX:
    case Sketcher::DistanceY:
    case Sketcher::Radius:
        unitStr = Base::Quantity(v->Value,Base::Unit::Length).getUserString();
        break;
    case Sketcher::Angle:
        unitStr = Base::Quantity(Base::toDegrees<double>(std::abs(v->Value)),Base::Unit::Angle).getUserString();
        break;
    case Sketcher::SnellsLaw:
        {
            double n1 = 1.0;
            double n2 = 1.0;
            if(abs(v->Value)>=1) {
                n2 = v->Value;
            } else {
                n1 = 1/v->Value;
            }
            unitStr = QString::fromLatin1("%1/%2").arg(n2).arg(n1);
        }
        break;
    default:
        break;
    }

    v->Name = (const char*)name.toUtf8();
    if (!unitStr.isEmpty()) {
        inEditMode = true;
        item->setData(Qt::UserRole, QString::fromLatin1("%1 (%2)")
            .arg(name)
            .arg(unitStr));
        inEditMode = false;
    }
}

void TaskSketcherConstrains::slotConstraintsChanged(void)
{
    QIcon hdist( Gui::BitmapFactory().pixmap("Constraint_HorizontalDistance") );
    QIcon vdist( Gui::BitmapFactory().pixmap("Constraint_VerticalDistance") );
    QIcon horiz( Gui::BitmapFactory().pixmap("Constraint_Horizontal") );
    QIcon vert ( Gui::BitmapFactory().pixmap("Constraint_Vertical") );
    QIcon lock ( Gui::BitmapFactory().pixmap("Sketcher_ConstrainLock") );
    QIcon coinc( Gui::BitmapFactory().pixmap("Constraint_PointOnPoint") );
    QIcon para ( Gui::BitmapFactory().pixmap("Constraint_Parallel") );
    QIcon perp ( Gui::BitmapFactory().pixmap("Constraint_Perpendicular") );
    QIcon tang ( Gui::BitmapFactory().pixmap("Constraint_Tangent") );
    QIcon dist ( Gui::BitmapFactory().pixmap("Constraint_Length") );
    QIcon radi ( Gui::BitmapFactory().pixmap("Constraint_Radius") );
    QIcon majradi ( Gui::BitmapFactory().pixmap("Constraint_Ellipse_Major_Radius") );
    QIcon minradi ( Gui::BitmapFactory().pixmap("Constraint_Ellipse_Minor_Radius") );
    QIcon angl ( Gui::BitmapFactory().pixmap("Constraint_InternalAngle") );
    QIcon ellipseXUAngl ( Gui::BitmapFactory().pixmap("Constraint_Ellipse_Axis_Angle") );
    QIcon equal( Gui::BitmapFactory().pixmap("Constraint_EqualLength") );
    QIcon pntoo( Gui::BitmapFactory().pixmap("Constraint_PointOnObject") );
    QIcon symm ( Gui::BitmapFactory().pixmap("Constraint_Symmetric") );
    QIcon snell ( Gui::BitmapFactory().pixmap("Constraint_SnellsLaw") );
    QIcon iaellipseminoraxis ( Gui::BitmapFactory().pixmap("Constraint_InternalAlignment_Ellipse_MinorAxis") );
    QIcon iaellipsemajoraxis ( Gui::BitmapFactory().pixmap("Constraint_InternalAlignment_Ellipse_MajorAxis") );
    QIcon iaellipsefocus1 ( Gui::BitmapFactory().pixmap("Constraint_InternalAlignment_Ellipse_Focus1") );
    QIcon iaellipsefocus2 ( Gui::BitmapFactory().pixmap("Constraint_InternalAlignment_Ellipse_Focus2") );
    QIcon iaellipseother ( Gui::BitmapFactory().pixmap("Constraint_InternalAlignment") );

    assert(sketchView);
    // Build up ListView with the constraints
    const std::vector< Sketcher::Constraint * > &vals = sketchView->getSketchObject()->Constraints.getValues();

    ui->listWidgetConstraints->clear();
    QString name;

    int Filter = ui->comboBoxFilter->currentIndex();

    int i=1;
    for(std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it!=vals.end();++it,++i){
        if ((*it)->Name.empty())
            name = QString::fromLatin1("Constraint%1").arg(i);
        else
            name = QString::fromUtf8((*it)->Name.c_str());

    /* Filter
      0 <=> All
      1 <=> Normal
      2 <=> Datums
      3 <=> Named
      */
        switch((*it)->Type){
            case Sketcher::Horizontal:
                if (Filter<2 || (Filter==3 && !(*it)->Name.empty()))
                    ui->listWidgetConstraints->addItem(new ConstraintItem(horiz,name,i-1,(*it)->Type));
                break;
            case Sketcher::Vertical:
                if (Filter<2 || (Filter==3 && !(*it)->Name.empty()))
                    ui->listWidgetConstraints->addItem(new ConstraintItem(vert,name,i-1,(*it)->Type));
                break;
            case Sketcher::Coincident:
                if (Filter<1 || (Filter==3 && !(*it)->Name.empty()))
                    ui->listWidgetConstraints->addItem(new ConstraintItem(coinc,name,i-1,(*it)->Type));
                break;
            case Sketcher::PointOnObject:
                if (Filter<2 || (Filter==3 && !(*it)->Name.empty()))
                    ui->listWidgetConstraints->addItem(new ConstraintItem(pntoo,name,i-1,(*it)->Type));
                break;
            case Sketcher::Parallel:
                if (Filter<2 || (Filter==3 && !(*it)->Name.empty()))
                    ui->listWidgetConstraints->addItem(new ConstraintItem(para,name,i-1,(*it)->Type));
                break;
            case Sketcher::Perpendicular:
                if (Filter<2 || (Filter==3 && !(*it)->Name.empty()))
                    ui->listWidgetConstraints->addItem(new ConstraintItem(perp,name,i-1,(*it)->Type));
                break;
            case Sketcher::Tangent:
                if (Filter<2 || (Filter==3 && !(*it)->Name.empty()))
                    ui->listWidgetConstraints->addItem(new ConstraintItem(tang,name,i-1,(*it)->Type));
                break;
            case Sketcher::Equal:
                if (Filter<2 || (Filter==3 && !(*it)->Name.empty()))
                    ui->listWidgetConstraints->addItem(new ConstraintItem(equal,name,i-1,(*it)->Type));
                break;
            case Sketcher::Symmetric:
                if (Filter<2 || (Filter==3 && !(*it)->Name.empty()))
                    ui->listWidgetConstraints->addItem(new ConstraintItem(symm,name,i-1,(*it)->Type));
                break;
            case Sketcher::Distance:
                if (Filter<3 || !(*it)->Name.empty()) {
                    ConstraintItem* item = new ConstraintItem(dist,name,i-1,(*it)->Type);
                    name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::Quantity((*it)->Value,Base::Unit::Length).getUserString());
                    item->setData(Qt::UserRole, name);
                    ui->listWidgetConstraints->addItem(item);
                }
                break;
            case Sketcher::DistanceX:
                if (Filter<3 || !(*it)->Name.empty()) {
                    ConstraintItem* item = new ConstraintItem(hdist,name,i-1,(*it)->Type);
                    name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::Quantity(std::abs((*it)->Value),Base::Unit::Length).getUserString());
                    item->setData(Qt::UserRole, name);
                    ui->listWidgetConstraints->addItem(item);
                }
                break;
            case Sketcher::DistanceY:
                if (Filter<3 || !(*it)->Name.empty()) {
                    ConstraintItem* item = new ConstraintItem(vdist,name,i-1,(*it)->Type);
                    name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::Quantity(std::abs((*it)->Value),Base::Unit::Length).getUserString());
                    item->setData(Qt::UserRole, name);
                    ui->listWidgetConstraints->addItem(item);
                }
                break;
            case Sketcher::Radius:
                if (Filter<3 || !(*it)->Name.empty()) {
                    ConstraintItem* item = new ConstraintItem(radi,name,i-1,(*it)->Type);
                    name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::Quantity((*it)->Value,Base::Unit::Length).getUserString());
                    item->setData(Qt::UserRole, name);
                    ui->listWidgetConstraints->addItem(item);
                }
                break;
            case Sketcher::Angle:
                if (Filter<3 || !(*it)->Name.empty()) {
                    ConstraintItem* item = new ConstraintItem(angl,name,i-1,(*it)->Type);
                    name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::Quantity(Base::toDegrees<double>(std::abs((*it)->Value)),Base::Unit::Angle).getUserString());
                    item->setData(Qt::UserRole, name);
                    ui->listWidgetConstraints->addItem(item);
                }
                break;
            case Sketcher::SnellsLaw:
                if (Filter<3 || !(*it)->Name.empty()) {
                    ConstraintItem* item = new ConstraintItem(snell,name,i-1,(*it)->Type);

                    double v = (*it)->Value;
                    double n1 = 1.0;
                    double n2 = 1.0;
                    if(abs(v)>=1) {
                        n2 = v;
                    } else {
                        n1 = 1/v;
                    }
                    name = QString::fromLatin1("%1 (%2/%3)").arg(name).arg(n2).arg(n1);
                    item->setData(Qt::UserRole, name);
                    ui->listWidgetConstraints->addItem(item);
                }
                break;
            case Sketcher::InternalAlignment:
                if (Filter<2 || (Filter==3 && !(*it)->Name.empty()))
                switch((*it)->AlignmentType){
                    case Sketcher::EllipseMajorDiameter:
                        ui->listWidgetConstraints->addItem(new ConstraintItem(iaellipsemajoraxis,name,i-1,(*it)->Type));
                        break;
                    case Sketcher::EllipseMinorDiameter:
                        ui->listWidgetConstraints->addItem(new ConstraintItem(iaellipseminoraxis,name,i-1,(*it)->Type));
                        break;
                    case Sketcher::EllipseFocus1: 
                        ui->listWidgetConstraints->addItem(new ConstraintItem(iaellipsefocus1,name,i-1,(*it)->Type));
                        break;
                    case Sketcher::EllipseFocus2: 
                        ui->listWidgetConstraints->addItem(new ConstraintItem(iaellipsefocus2,name,i-1,(*it)->Type));
                        break;
                    case Sketcher::Undef:
                    default: 
                        ui->listWidgetConstraints->addItem(new ConstraintItem(iaellipseother,name,i-1,(*it)->Type));
                        break;
                }
                break;
            default:
                ui->listWidgetConstraints->addItem(new ConstraintItem(name,i-1,(*it)->Type));
                break;
        }
    }
}

void TaskSketcherConstrains::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}



#include "moc_TaskSketcherConstrains.cpp"
