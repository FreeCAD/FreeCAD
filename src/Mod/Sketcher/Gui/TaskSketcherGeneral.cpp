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


#include "ui_TaskSketcherGeneral.h"
#include "TaskSketcherGeneral.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <QEvent>

#include "ViewProviderSketch.h"

using namespace SketcherGui;
using namespace Gui::TaskView;
namespace bp = boost::placeholders;

SketcherGeneralWidget::SketcherGeneralWidget(QWidget *parent)
  : QWidget(parent), ui(new Ui_TaskSketcherGeneral)
{
    ui->setupUi(this);
    ui->renderingOrder->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    // connecting the needed signals
    connect(ui->checkBoxShowGrid, SIGNAL(toggled(bool)),
            this, SIGNAL(emitToggleGridView(bool)));
    connect(ui->checkBoxGridSnap, SIGNAL(toggled(bool)),
            this, SIGNAL(emitToggleGridSnap(bool)));
    connect(ui->gridSize, SIGNAL(valueChanged(double)),
            this, SIGNAL(emitSetGridSize(double)));
    connect(ui->checkBoxAutoconstraints, SIGNAL(toggled(bool)),
            this, SIGNAL(emitToggleAutoconstraints(bool)));
    connect(ui->checkBoxRedundantAutoconstraints, SIGNAL(toggled(bool)),
        this, SIGNAL(emitToggleAvoidRedundant(bool)));
    ui->renderingOrder->installEventFilter(this);
}

SketcherGeneralWidget::~SketcherGeneralWidget()
{
}

bool SketcherGeneralWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->renderingOrder && event->type() == QEvent::ChildRemoved) {
        emitRenderOrderChanged();
    }
    return false;
}

void SketcherGeneralWidget::saveSettings()
{
    ui->checkBoxShowGrid->onSave();
    ui->gridSize->onSave();
    ui->checkBoxGridSnap->onSave();
    ui->checkBoxAutoconstraints->onSave();
    ui->checkBoxRedundantAutoconstraints->onSave();

    saveOrderingOrder();
}

void SketcherGeneralWidget::saveOrderingOrder()
{
    int topid = ui->renderingOrder->item(0)->data(Qt::UserRole).toInt();
    int midid = ui->renderingOrder->item(1)->data(Qt::UserRole).toInt();
    int lowid = ui->renderingOrder->item(2)->data(Qt::UserRole).toInt();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrp->SetInt("TopRenderGeometryId",topid);
    hGrp->SetInt("MidRenderGeometryId",midid);
    hGrp->SetInt("LowRenderGeometryId",lowid);
}

void SketcherGeneralWidget::loadSettings()
{
    ui->checkBoxShowGrid->onRestore();
    ui->gridSize->onRestore();
    if (ui->gridSize->rawValue() == 0) { ui->gridSize->setValue(10.0); }
    ui->checkBoxGridSnap->onRestore();
    ui->checkBoxAutoconstraints->onRestore();
    ui->checkBoxRedundantAutoconstraints->onRestore();

    loadOrderingOrder();
}

void SketcherGeneralWidget::loadOrderingOrder()
{
    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    // 1->Normal Geometry, 2->Construction, 3->External
    int topid = hGrpp->GetInt("TopRenderGeometryId",1);
    int midid = hGrpp->GetInt("MidRenderGeometryId",2);
    int lowid = hGrpp->GetInt("LowRenderGeometryId",3);

    {
        QSignalBlocker block(ui->renderingOrder);
        ui->renderingOrder->clear();

        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setData(Qt::UserRole, QVariant(topid));
        newItem->setText( topid==1?tr("Normal Geometry"):topid==2?tr("Construction Geometry"):tr("External Geometry"));
        ui->renderingOrder->insertItem(0,newItem);

        newItem = new QListWidgetItem;
        newItem->setData(Qt::UserRole, QVariant(midid));
        newItem->setText(midid==1?tr("Normal Geometry"):midid==2?tr("Construction Geometry"):tr("External Geometry"));
        ui->renderingOrder->insertItem(1,newItem);

        newItem = new QListWidgetItem;
        newItem->setData(Qt::UserRole, QVariant(lowid));
        newItem->setText(lowid==1?tr("Normal Geometry"):lowid==2?tr("Construction Geometry"):tr("External Geometry"));
        ui->renderingOrder->insertItem(2,newItem);
    }
}

void SketcherGeneralWidget::setGridSize(double val)
{
    ui->gridSize->setValue(Base::Quantity(val,Base::Unit::Length));
}

void SketcherGeneralWidget::checkGridView(bool on)
{
    ui->checkBoxShowGrid->setChecked(on);
}

void SketcherGeneralWidget::checkGridSnap(bool on)
{
    ui->checkBoxGridSnap->setChecked(on);
}

void SketcherGeneralWidget::checkAutoconstraints(bool on)
{
    ui->checkBoxAutoconstraints->setChecked(on);
}

void SketcherGeneralWidget::checkAvoidRedundant(bool on)
{
    ui->checkBoxRedundantAutoconstraints->setChecked(on);
}

void SketcherGeneralWidget::enableGridSettings(bool on)
{
    ui->label->setEnabled(on);
    ui->gridSize->setEnabled(on);
    ui->checkBoxGridSnap->setEnabled(on);
}

void SketcherGeneralWidget::enableAvoidRedundant(bool on)
{
    ui->checkBoxRedundantAutoconstraints->setEnabled(on);
}

void SketcherGeneralWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

// ----------------------------------------------------------------------------

TaskSketcherGeneral::TaskSketcherGeneral(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Edit controls"),true, nullptr)
    , sketchView(sketchView)
{
    // we need a separate container widget to add all controls to
    widget = new SketcherGeneralWidget(this);
    this->groupLayout()->addWidget(widget);
    
    {
        //Blocker probably not needed as signals aren't connected yet
        QSignalBlocker block(widget);
        //Load default settings to get ordering order & avoid redundant values
        widget->loadSettings();
        widget->checkGridView(sketchView->ShowGrid.getValue());
        if (sketchView->GridSize.getValue() > 0) {
            widget->setGridSize(sketchView->GridSize.getValue());
        }
        widget->checkGridSnap(sketchView->GridSnap.getValue());
        widget->enableGridSettings(sketchView->ShowGrid.getValue());
        widget->checkAutoconstraints(sketchView->Autoconstraints.getValue());
        widget->checkAvoidRedundant(sketchView->AvoidRedundant.getValue());
        widget->enableAvoidRedundant(sketchView->Autoconstraints.getValue());
    }

    // connecting the needed signals
    QObject::connect(
        widget, SIGNAL(emitToggleGridView(bool)),
        this  , SLOT  (onToggleGridView(bool))
    );

    QObject::connect(
        widget, SIGNAL(emitToggleGridSnap(bool)),
        this  , SLOT  (onToggleGridSnap(bool))
    );

    QObject::connect(
        widget, SIGNAL(emitSetGridSize(double)),
        this  , SLOT  (onSetGridSize(double))
    );

    QObject::connect(
        widget, SIGNAL(emitToggleAutoconstraints(bool)),
        this  , SLOT  (onToggleAutoconstraints(bool))
    );
    
    QObject::connect(
        widget, SIGNAL(emitToggleAvoidRedundant(bool)),
        this  , SLOT  (onToggleAvoidRedundant(bool))
    );

    QObject::connect(
        widget, SIGNAL(emitRenderOrderChanged()),
        this  , SLOT  (onRenderOrderChanged())
    );

    Gui::Selection().Attach(this);

    Gui::Application* app = Gui::Application::Instance;
    changedSketchView = app->signalChangedObject.connect(boost::bind
        (&TaskSketcherGeneral::onChangedSketchView, this, bp::_1, bp::_2));
}

TaskSketcherGeneral::~TaskSketcherGeneral()
{
    Gui::Selection().Detach(this);
}

void TaskSketcherGeneral::onChangedSketchView(const Gui::ViewProvider& vp,
                                              const App::Property& prop)
{
    if (sketchView == &vp) {
        if (&sketchView->ShowGrid == &prop) {
            QSignalBlocker block(widget);
            widget->checkGridView(sketchView->ShowGrid.getValue());
            widget->enableGridSettings(sketchView->ShowGrid.getValue());
        }
        else if (&sketchView->GridSize == &prop) {
            QSignalBlocker block(widget);
            widget->setGridSize(sketchView->GridSize.getValue());
        }
        else if (&sketchView->GridSnap == &prop) {
            QSignalBlocker block(widget);
            widget->checkGridSnap(sketchView->GridSnap.getValue());
        }
        else if (&sketchView->Autoconstraints == &prop) {
            QSignalBlocker block(widget);
            widget->checkAutoconstraints(sketchView->Autoconstraints.getValue());
            widget->enableAvoidRedundant(sketchView->Autoconstraints.getValue());
        }
        else if (&sketchView->AvoidRedundant == &prop) {
            QSignalBlocker block(widget);
            widget->checkAvoidRedundant(sketchView->AvoidRedundant.getValue());
        }
    }
}

void TaskSketcherGeneral::onToggleGridView(bool on)
{
    Base::ConnectionBlocker block(changedSketchView);
    sketchView->ShowGrid.setValue(on);
    widget->enableGridSettings(on);
}

void TaskSketcherGeneral::onSetGridSize(double val)
{
    Base::ConnectionBlocker block(changedSketchView);
    if (val > 0)
        sketchView->GridSize.setValue(val);
}

void TaskSketcherGeneral::onToggleGridSnap(bool on)
{
    Base::ConnectionBlocker block(changedSketchView);
    sketchView->GridSnap.setValue(on);
}

void TaskSketcherGeneral::onToggleAutoconstraints(bool on)
{
    Base::ConnectionBlocker block(changedSketchView);
    sketchView->Autoconstraints.setValue(on);
    widget->enableAvoidRedundant(on);
}

void TaskSketcherGeneral::onToggleAvoidRedundant(bool on)
{
    Base::ConnectionBlocker block(changedSketchView);
    sketchView->AvoidRedundant.setValue(on);
}

/// @cond DOXERR
void TaskSketcherGeneral::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                                   Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
    Q_UNUSED(Reason);
    //if (Reason.Type == SelectionChanges::AddSelection ||
    //    Reason.Type == SelectionChanges::RmvSelection ||
    //    Reason.Type == SelectionChanges::SetSelection ||
    //    Reason.Type == SelectionChanges::ClrSelection) {
    //}
}
/// @endcond DOXERR

void TaskSketcherGeneral::onRenderOrderChanged()
{
    widget->saveOrderingOrder();
    sketchView->updateColor();
}

#include "moc_TaskSketcherGeneral.cpp"
