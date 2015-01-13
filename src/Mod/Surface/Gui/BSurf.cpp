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

using namespace SurfaceGui;
//#undef CS_FUTURE // multi-threading causes some problems

namespace SurfaceGui {

class ViewProviderBSurf : public Gui::ViewProvider
{
public:
    ViewProviderBSurf()
    {
    /*    coords = new SoCoordinate3();
        coords->ref();
        planes = new SoLineSet();
        planes->ref();
        SoBaseColor* color = new SoBaseColor();
        color->rgb.setValue(1.0f, 0.447059f, 0.337255f);
        SoDrawStyle* style = new SoDrawStyle();
        style->lineWidth.setValue(2.0f);
        this->pcRoot->addChild(color);
        this->pcRoot->addChild(style);
        this->pcRoot->addChild(coords);
        this->pcRoot->addChild(planes);*/
    }

    ~ViewProviderBSurf()
    {
    }

    void updateData(const App::Property*)
    {
    }

    const char* getDefaultDisplayMode() const
    {
        return "";
    }

    std::vector<std::string> getDisplayModes(void) const
    {
        return std::vector<std::string>();
    }

/*    void setCoords(const std::vector<Base::Vector3f>& v)
    {
    }*/

private:
};

BSurf::BSurf(const Base::BoundBox3d& bb, QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl), bbox(bb)
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

TaskBSurf::TaskBSurf(const Base::BoundBox3d& bb)
{
    widget = new BSurf(bb);
    taskbox = new Gui::TaskView::TaskBox(
        NULL,
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
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
