/***************************************************************************
 *   Copyright (c) 2015 Balázs Bámer                                       *
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

#include <Mod/Surface/App/FillType.h>
#include <Gui/ViewProvider.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Base/Sequencer.h>
#include <Gui/Control.h>
#include <Gui/BitmapFactory.h>

#include "SurfaceFilling.h"
#include "ui_SurfaceFilling.h"


using namespace SurfaceGui;

PROPERTY_SOURCE(SurfaceGui::ViewProviderSurfaceFeature, PartGui::ViewProviderPart)

namespace SurfaceGui {

void ViewProviderSurfaceFeature::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit filling"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);
}

bool ViewProviderSurfaceFeature::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this sketch the
        // object unsets and sets its edit mode without closing
        // the task panel

        Surface::SurfaceFeature* obj =  static_cast<Surface::SurfaceFeature*>(this->getObject());

        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();

        // start the edit dialog
        if (dlg) {
            TaskSurfaceFilling* tDlg = qobject_cast<TaskSurfaceFilling*>(dlg);
            if (tDlg)
                tDlg->setEditedObject(obj);
            Gui::Control().showDialog(dlg);
        }
        else {
            Gui::Control().showDialog(new TaskSurfaceFilling(this, obj));
        }
        return true;
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}

void ViewProviderSurfaceFeature::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        QTimer::singleShot(0, &Gui::Control(), SLOT(closeDialog()));
    }
    else {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
    }
}

QIcon ViewProviderSurfaceFeature::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap("BSplineSurf");
}

SurfaceFilling::SurfaceFilling(ViewProviderSurfaceFeature* vp, Surface::SurfaceFeature* obj)
{
    ui = new Ui_SurfaceFilling();
    ui->setupUi(this);
    this->vp = vp;
    setEditedObject(obj);
}

/*
 *  Destroys the object and frees any allocated resources
 */
SurfaceFilling::~SurfaceFilling()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

// stores object pointer, its old fill type and adjusts radio buttons according to it.
void SurfaceFilling::setEditedObject(Surface::SurfaceFeature* obj)
{
    editedObject = obj;
    oldFillType = (FillType_t)(editedObject->FillType.getValue());
    switch(oldFillType)
    {
    case StretchStyle:
        ui->fillType_stretch->setChecked(true);
        break;
    case CoonsStyle:
        ui->fillType_coons->setChecked(true);
        break;
    case CurvedStyle:
        ui->fillType_curved->setChecked(true);
        break;
    }
    fillType = oldFillType;
}

FillType_t SurfaceFilling::getFillType() const
{
    FillType_t ret;
    if (ui->fillType_stretch->isChecked())
        ret = StretchStyle;
    else if (ui->fillType_coons->isChecked())
        ret = CoonsStyle;
    else
        ret = CurvedStyle;
    return ret;
}

void SurfaceFilling::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void SurfaceFilling::accept()
{
    // applies the changes
    apply();
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
}

void SurfaceFilling::reject()
{
    if (oldFillType == InvalidStyle) {
        Gui::Command::abortCommand();
    }
    else {
        // if the object fill type was changed, reset the old one
        if (editedObject->FillType.getValue() != oldFillType) {
            editedObject->FillType.setValue(oldFillType);
        }

        Gui::Command::commitCommand();
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    }

    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
}

void SurfaceFilling::apply()
{
    // apply the change only if it is a real change
    if (editedObject->FillType.getValue() != fillType) {
        editedObject->FillType.setValue(fillType);
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    }
}

void SurfaceFilling::on_fillType_stretch_clicked()
{
    fillType = StretchStyle;
}

void SurfaceFilling::on_fillType_coons_clicked()
{
    fillType = CoonsStyle;
}

void SurfaceFilling::on_fillType_curved_clicked()
{
    fillType = CurvedStyle;
}

// ---------------------------------------

TaskSurfaceFilling::TaskSurfaceFilling(ViewProviderSurfaceFeature* vp, Surface::SurfaceFeature* obj)
{
    widget = new SurfaceFilling(vp, obj);
    widget->setWindowTitle(QObject::tr("Surface"));
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("BezSurf"),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskSurfaceFilling::~TaskSurfaceFilling()
{
    // automatically deleted in the sub-class
}

void TaskSurfaceFilling::setEditedObject(Surface::SurfaceFeature* obj)
{
    widget->setEditedObject(obj);
}

bool TaskSurfaceFilling::accept()
{
    widget->accept();
    return true;
}

bool TaskSurfaceFilling::reject()
{
    widget->reject();
    return true;
}

// Apply clicked
void TaskSurfaceFilling::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        widget->apply();
    }
}

}

#include "moc_SurfaceFilling.cpp"
