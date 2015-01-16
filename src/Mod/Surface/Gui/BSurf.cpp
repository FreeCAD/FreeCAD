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
#include <Gui/ViewProvider.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Base/Sequencer.h>
#include <Gui/Control.h>


using namespace SurfaceGui;
//#undef CS_FUTURE // multi-threading causes some problems

namespace SurfaceGui {

PROPERTY_SOURCE(SurfaceGui::ViewProviderBSurf, PartGui::ViewProviderPart)

bool ViewProviderBSurf::setEdit(int ModNum)
{
// When double-clicking on the item for this sketch the
// object unsets and sets its edit mode without closing
// the task panel
   Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
   TaskBSurf* tDlg = qobject_cast<TaskBSurf*>(dlg);
// start the edit dialog
    if(dlg)
       Gui::Control().showDialog(tDlg);
    else
       Gui::Control().showDialog(new TaskBSurf(this));
//       draw();
    return true;
}

void ViewProviderBSurf::unsetEdit(int ModNum)
{
/*if(!Gui::Selection().isSelected(pcObject) || !Visibility.getValue()) {
    internal_vp.switch_node(false);
    pcModeSwitch->whichChild = -1;
    m_selected = false;
    } */
}

BSurf::BSurf(ViewProviderBSurf* vp)
  //: QDialog(parent, fl), bbox(bb)
{
    ui = new Ui_DlgBSurf();
    ui->setupUi(this);
    vp = new ViewProviderBSurf();
}

/*
 *  Destroys the object and frees any allocated resources
 */
BSurf::~BSurf()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
    delete vp;
}

filltype_t BSurf::getFillType() const
{
    if (ui->fillType_stretch->isChecked())
        return StretchStyle;
    else if (ui->fillType_coons->isChecked())
        return CoonsStyle;
    else
        return CurvedStyle;
}

void BSurf::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QDialog::changeEvent(e);
    }
}

void BSurf::accept()
{
    apply();
    QDialog::accept();
}

void BSurf::apply()
{
    printf("apply\n");
 //   std::vector<App::DocumentObject*> obj = Gui::Selection().
   //     getObjectsOfType(Part::Feature::getClassTypeId());
////////////////
}

void BSurf::on_fillType_stretch_clicked()
{
}

void BSurf::on_fillType_coons_clicked()
{
}

void BSurf::on_fillType_curved_clicked()
{
}

// ---------------------------------------

TaskBSurf::TaskBSurf(ViewProviderBSurf* vp)
{
    widget = new BSurf(vp);
/*    taskbox = new Gui::TaskView::TaskBox(
        NULL,
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);*/
    Content.push_back(widget);
}

TaskBSurf::~TaskBSurf()
{
    // automatically deleted in the sub-class
}

bool TaskBSurf::accept()
{
    widget->accept();
    return (widget->result() == QDialog::Accepted);
}

void TaskBSurf::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        widget->apply();
    }
}

}
#include "moc_BSurf.cpp"
