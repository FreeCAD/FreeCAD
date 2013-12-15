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
# include <QRegExp>
# include <QString>
#endif

#include "ui_TaskSketcherConstrains.h"
#include "TaskSketcherConstrains.h"
#include "EditDatumDialog.h"
#include "ViewProviderSketch.h"

#include <Mod/Sketcher/App/SketchObject.h>

#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/BitmapFactory.h>
#include <boost/bind.hpp>

using namespace SketcherGui;
using namespace Gui::TaskView;

// helper class to store additional information about the listWidget entry.
class ConstraintItem:public QListWidgetItem
{
public:
    ConstraintItem(const QIcon & icon, const QString & text,int ConstNbr,Sketcher::ConstraintType t):QListWidgetItem(icon,text),ConstraintNbr(ConstNbr),Type(t){}
    ConstraintItem(const QString & text,int ConstNbr,Sketcher::ConstraintType t):QListWidgetItem(text),ConstraintNbr(ConstNbr),Type(t){}
    int ConstraintNbr;
    Sketcher::ConstraintType Type;
};

TaskSketcherConstrains::TaskSketcherConstrains(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Constraints"),true, 0)
    , sketchView(sketchView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskSketcherConstrains();
    ui->setupUi(proxy);
    ui->listWidgetConstraints->setSelectionMode(QAbstractItemView::ExtendedSelection);
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
    //QObject::connect(
    //    ui->listWidgetConstraints, SIGNAL(entered(const QModelIndex &)),
    //    this                     , SLOT  (on_listWidgetConstraints_entered(const QModelIndex &))
    //   );

    connectionConstraintsChanged = sketchView->signalConstraintsChanged.connect(boost::bind(&SketcherGui::TaskSketcherConstrains::slotConstraintsChanged, this));

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
                    int index = rx.cap(1).toInt(&ok);
                    if (ok) {
                        int countItems = ui->listWidgetConstraints->count();
                        for (int i=0; i<countItems;i++) {
                            ConstraintItem* item = static_cast<ConstraintItem*>
                                (ui->listWidgetConstraints->item(i));
                            if (item->ConstraintNbr == index) {
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
        ss << "Constraint" << static_cast<ConstraintItem*>(*it)->ConstraintNbr;
        Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
    }
    this->blockConnection(block);
}

void TaskSketcherConstrains::on_listWidgetConstraints_itemActivated(QListWidgetItem *item)
{
    ConstraintItem *it = dynamic_cast<ConstraintItem*>(item);

    // if its the right constraint
    if (it->Type == Sketcher::Distance ||
        it->Type == Sketcher::DistanceX || it->Type == Sketcher::DistanceY ||
        it->Type == Sketcher::Radius || it->Type == Sketcher::Angle) {

        EditDatumDialog *editDatumDialog = new EditDatumDialog(this->sketchView, it->ConstraintNbr);
        editDatumDialog->exec(false);
        delete editDatumDialog;
    }
}

//void TaskSketcherConstrains::on_listWidgetConstraints_entered(const QModelIndex &index)
//{
//    index;
//}


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
    QIcon angl ( Gui::BitmapFactory().pixmap("Constraint_InternalAngle") );
    QIcon equal( Gui::BitmapFactory().pixmap("Constraint_EqualLength") );
    QIcon pntoo( Gui::BitmapFactory().pixmap("Constraint_PointOnObject") );
    QIcon symm ( Gui::BitmapFactory().pixmap("Constraint_Symmetric") );

    assert(sketchView);
    // Build up ListView with the constraints
    const std::vector< Sketcher::Constraint * > &vals = sketchView->getSketchObject()->Constraints.getValues();

    ui->listWidgetConstraints->clear();
    QString name;

    int Filter = ui->comboBoxFilter->currentIndex();

    int i=1;
    for(std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it!=vals.end();++it,++i){
        if ((*it)->Name == "")
            name = QString::fromLatin1("Constraint%1").arg(i);
        else
            name = QString::fromLatin1((*it)->Name.c_str());

        switch((*it)->Type){
            case Sketcher::Horizontal:
                if(Filter<2 || (*it)->Name != "")
                    ui->listWidgetConstraints->addItem(new ConstraintItem(horiz,name,i-1,(*it)->Type));
                break;
            case Sketcher::Vertical:
                if(Filter<2 || (*it)->Name != "")
                    ui->listWidgetConstraints->addItem(new ConstraintItem(vert,name,i-1,(*it)->Type));
                break;
            case Sketcher::Coincident:
                if(Filter<1 || (*it)->Name != "")
                    ui->listWidgetConstraints->addItem(new ConstraintItem(coinc,name,i-1,(*it)->Type));
                break;
            case Sketcher::PointOnObject:
                if(Filter<2 || (*it)->Name != "")
                ui->listWidgetConstraints->addItem(new ConstraintItem(pntoo,name,i-1,(*it)->Type));
                break;
            case Sketcher::Parallel:
                if(Filter<2 || (*it)->Name != "")
                    ui->listWidgetConstraints->addItem(new ConstraintItem(para,name,i-1,(*it)->Type));
                break;
            case Sketcher::Perpendicular:
                if(Filter<2 || (*it)->Name != "")
                    ui->listWidgetConstraints->addItem(new ConstraintItem(perp,name,i-1,(*it)->Type));
                break;
            case Sketcher::Tangent:
                if(Filter<2 || (*it)->Name != "")
                    ui->listWidgetConstraints->addItem(new ConstraintItem(tang,name,i-1,(*it)->Type));
                break;
            case Sketcher::Equal:
                if(Filter<2 || (*it)->Name != "")
                    ui->listWidgetConstraints->addItem(new ConstraintItem(equal,name,i-1,(*it)->Type));
                break;
            case Sketcher::Symmetric:
                if(Filter<2 || (*it)->Name != "")
                    ui->listWidgetConstraints->addItem(new ConstraintItem(symm,name,i-1,(*it)->Type));
                break;
            case Sketcher::Distance:
                if(Filter<3 || (*it)->Name != ""){
                    name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::Quantity((*it)->Value,Base::Unit::Length).getUserString());
                    //name = QString::fromLatin1("%1 (%2)").arg(name).arg((*it)->Value);
                    ui->listWidgetConstraints->addItem(new ConstraintItem(dist,name,i-1,(*it)->Type));
                }
                break;
            case Sketcher::DistanceX:
                if(Filter<3 || (*it)->Name != ""){
                    name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::Quantity((*it)->Value,Base::Unit::Length).getUserString());
                    //name = QString::fromLatin1("%1 (%2)").arg(name).arg(std::abs((*it)->Value));
                    ui->listWidgetConstraints->addItem(new ConstraintItem(hdist,name,i-1,(*it)->Type));
                }
                break;
            case Sketcher::DistanceY:
                if(Filter<3 || (*it)->Name != ""){
                    name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::Quantity((*it)->Value,Base::Unit::Length).getUserString());
                    //name = QString::fromLatin1("%1 (%2)").arg(name).arg(std::abs((*it)->Value));
                    ui->listWidgetConstraints->addItem(new ConstraintItem(vdist,name,i-1,(*it)->Type));
                }
                break;
            case Sketcher::Radius:
                if(Filter<3 || (*it)->Name != ""){
                    name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::Quantity((*it)->Value,Base::Unit::Length).getUserString());
                    //name = QString::fromLatin1("%1 (%2)").arg(name).arg((*it)->Value);
                    ui->listWidgetConstraints->addItem(new ConstraintItem(radi,name,i-1,(*it)->Type));
                }
                break;
            case Sketcher::Angle:
                if(Filter<3 || (*it)->Name != ""){
                    name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::Quantity(Base::toDegrees<double>(std::abs((*it)->Value)),Base::Unit::Angle).getUserString());
                    //name = QString::fromLatin1("%1 (%2)").arg(name).arg(Base::toDegrees<double>(std::abs((*it)->Value)));
                    ui->listWidgetConstraints->addItem(new ConstraintItem(angl,name,i-1,(*it)->Type));
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
