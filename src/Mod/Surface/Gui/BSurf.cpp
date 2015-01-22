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
#ifndef _PreComp_
# include <Standard_math.hxx>
# include <BRep_Builder.hxx>
# include <TopoDS.hxx>
# include <TopExp_Explorer.hxx>
# include <gp_Pln.hxx>
# include <cfloat>
# include <QFuture>
# include <QFutureWatcher>
# include <QtConcurrentMap>
# include <boost/bind.hpp>
#endif

#include "BSurf.h"
#include "../App/FeatureBSurf.h"
#include "../FillType.h"
#include <Gui/ViewProvider.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Base/Sequencer.h>
#include <Gui/Control.h>

#include "ui_BSurf.h"


using namespace SurfaceGui;
//#undef CS_FUTURE // multi-threading causes some problems

PROPERTY_SOURCE(SurfaceGui::ViewProviderBSurf, PartGui::ViewProviderPart)

namespace SurfaceGui {

bool ViewProviderBSurf::setEdit(int ModNum)
{
// When double-clicking on the item for this sketch the
// object unsets and sets its edit mode without closing
// the task panel

    Surface::BSurf* obj =  static_cast<Surface::BSurf*>(this->getObject());

    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    TaskBSurf* tDlg = qobject_cast<TaskBSurf*>(dlg);
// start the edit dialog
    if(dlg)
    {
        tDlg->setEditedObject(obj);
        Gui::Control().showDialog(tDlg);
    }
    else
    {
        Gui::Control().showDialog(new TaskBSurf(this, obj));
    }
    return true;
}

void ViewProviderBSurf::unsetEdit(int ModNum)
{
    // nothing to do
}

BSurf::BSurf(ViewProviderBSurf* vp, Surface::BSurf* obj)
{
    ui = new Ui_DlgBSurf();
    ui->setupUi(this);
    this->vp = vp;
    setEditedObject(obj);
}

/*
 *  Destroys the object and frees any allocated resources
 */
BSurf::~BSurf()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

// stores object pointer, its old fill type and adjusts radio buttons according to it.
void BSurf::setEditedObject(Surface::BSurf* obj)
{
    editedObject = obj;
    filltype_t ft = (filltype_t)(editedObject->filltype.getValue());
    switch(ft)
    {
    case StretchStyle:
        oldFillType = ft;
        ui->fillType_stretch->setChecked(true);
        break;
    case CoonsStyle:
        oldFillType = ft;
        ui->fillType_coons->setChecked(true);
        break;
    case CurvedStyle:
        oldFillType = ft;
        ui->fillType_curved->setChecked(true);
        break;
    /*default:
        printf("BSurf::setEditedObject: illegal fill type: %d\n", editedObject->filltype.getValue());
        Standard_Failure::Raise("BSurf::setEditedObject: illegal fill type.");*/
    }
    fillType = oldFillType;
}

filltype_t BSurf::getFillType() const
{
    filltype_t ret;
    if (ui->fillType_stretch->isChecked())
        ret = StretchStyle;
    else if (ui->fillType_coons->isChecked())
        ret = CoonsStyle;
    else
        ret = CurvedStyle;
    return ret;
}

void BSurf::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void BSurf::accept()
{
    // applies the changes
    apply();
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
}

void BSurf::reject()
{
    // if the object fill type was changed, reset the old one
    if(editedObject->filltype.getValue() != oldFillType)
    {
        editedObject->filltype.setValue(oldFillType);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
}

void BSurf::apply()
{
    // apply the change only if it is a real change
    if(editedObject->filltype.getValue() != fillType)
    {
        editedObject->filltype.setValue(fillType);
        oldFillType = fillType;
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    }
}

void BSurf::on_fillType_stretch_clicked()
{
    fillType = StretchStyle;
}

void BSurf::on_fillType_coons_clicked()
{
    fillType = CoonsStyle;
}

void BSurf::on_fillType_curved_clicked()
{
    fillType = CurvedStyle;
}

// ---------------------------------------

TaskBSurf::TaskBSurf(ViewProviderBSurf* vp, Surface::BSurf* obj)
{
    widget = new BSurf(vp, obj);
    Content.push_back(widget);
}

TaskBSurf::~TaskBSurf()
{
    // automatically deleted in the sub-class
}

void TaskBSurf::setEditedObject(Surface::BSurf* obj)
{
    widget->setEditedObject(obj);
}

bool TaskBSurf::accept()
{
    widget->accept();
    return true;
}

bool TaskBSurf::reject()
{
    widget->reject();
    return true;
}

// Apply clicked
void TaskBSurf::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        widget->apply();
    }
}

}
#include "moc_BSurf.cpp"
