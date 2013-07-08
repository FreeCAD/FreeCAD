/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/nodes/SoClipPlane.h>
# include <Inventor/nodes/SoGroup.h>
# include <QPointer>
#endif
# include <Inventor/sensors/SoTimerSensor.h>

#include "Clipping.h"
#include "ui_Clipping.h"
#include "Application.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

using namespace Gui::Dialog;

class Clipping::Private {
public:
    Ui_Clipping ui;
    QPointer<Gui::View3DInventor> view;
    SoGroup* node;
    SoClipPlane* clipX;
    SoClipPlane* clipY;
    SoClipPlane* clipZ;
    SoClipPlane* clipView;
    bool flipX;
    bool flipY;
    bool flipZ;
    SoTimerSensor* sensor;
    Private() : flipX(false), flipY(false), flipZ(false)
    {
        clipX = new SoClipPlane();
        clipX->on.setValue(false);
        clipX->plane.setValue(SbPlane(SbVec3f(1,0,0),0));
        clipX->ref();

        clipY = new SoClipPlane();
        clipY->on.setValue(false);
        clipY->plane.setValue(SbPlane(SbVec3f(0,1,0),0));
        clipY->ref();

        clipZ = new SoClipPlane();
        clipZ->on.setValue(false);
        clipZ->plane.setValue(SbPlane(SbVec3f(0,0,1),0));
        clipZ->ref();

        clipView = new SoClipPlane();
        clipView->on.setValue(false);
        clipView->plane.setValue(SbPlane(SbVec3f(0,0,1),0));
        clipView->ref();

        sensor = new SoTimerSensor(moveCallback, this);
    }
    ~Private()
    {
        clipX->unref();
        clipY->unref();
        clipZ->unref();
        clipView->unref();
        delete sensor;
    }
    static void moveCallback(void * data, SoSensor * sensor)
    {
        Private* self = reinterpret_cast<Private*>(data);
        if (self->view) {
            Gui::View3DInventorViewer* view = self->view->getViewer();
            SoClipPlane* clip = self->clipView;
            SbPlane pln = clip->plane.getValue();
            clip->plane.setValue(SbPlane(view->getViewDirection(),pln.getDistanceFromOrigin()));
        }
    }
};

/* TRANSLATOR Gui::Dialog::Clipping */

Clipping::Clipping(Gui::View3DInventor* view, QWidget* parent)
  : QWidget(parent), d(new Private)
{
    // create widgets
    d->ui.setupUi(this);
    d->ui.clipView->setRange(-INT_MAX,INT_MAX);
    d->ui.clipView->setSingleStep(0.1f);
    d->ui.clipX->setRange(-INT_MAX,INT_MAX);
    d->ui.clipX->setSingleStep(0.1f);
    d->ui.clipY->setRange(-INT_MAX,INT_MAX);
    d->ui.clipY->setSingleStep(0.1f);
    d->ui.clipZ->setRange(-INT_MAX,INT_MAX);
    d->ui.clipZ->setSingleStep(0.1f);

    d->ui.dirX->setRange(-INT_MAX,INT_MAX);
    d->ui.dirX->setSingleStep(0.1f);
    d->ui.dirY->setRange(-INT_MAX,INT_MAX);
    d->ui.dirY->setSingleStep(0.1f);
    d->ui.dirZ->setRange(-INT_MAX,INT_MAX);
    d->ui.dirZ->setSingleStep(0.1f);
    d->ui.dirZ->setValue(1.0f);

    d->view = view;
    View3DInventorViewer* viewer = view->getViewer();
    d->node = static_cast<SoGroup*>(viewer->getSceneGraph());
    d->node->ref();
    d->node->insertChild(d->clipX, 0);
    d->node->insertChild(d->clipY, 0);
    d->node->insertChild(d->clipZ, 0);
    d->node->insertChild(d->clipView, 0);

    SoGetBoundingBoxAction action(viewer->getViewportRegion());
    action.apply(viewer->getSceneGraph());
    SbBox3f box = action.getBoundingBox();

    if (!box.isEmpty()) {
        SbVec3f cnt = box.getCenter();
        d->ui.clipView->setValue(cnt[2]);
        d->ui.clipX->setValue(cnt[0]);
        d->ui.clipY->setValue(cnt[1]);
        d->ui.clipZ->setValue(cnt[2]);
    }
}

/** Destroys the object and frees any allocated resources */
Clipping::~Clipping()
{
    d->node->removeChild(d->clipX);
    d->node->removeChild(d->clipY);
    d->node->removeChild(d->clipZ);
    d->node->removeChild(d->clipView);
    d->node->unref();
    delete d;
}

void Clipping::on_groupBoxX_toggled(bool on)
{
    if (on) {
        d->ui.groupBoxView->setChecked(false);
    }

    d->clipX->on.setValue(on);
}

void Clipping::on_groupBoxY_toggled(bool on)
{
    if (on) {
        d->ui.groupBoxView->setChecked(false);
    }

    d->clipY->on.setValue(on);
}

void Clipping::on_groupBoxZ_toggled(bool on)
{
    if (on) {
        d->ui.groupBoxView->setChecked(false);
    }

    d->clipZ->on.setValue(on);
}

void Clipping::on_clipX_valueChanged(double val)
{
    SbPlane pln = d->clipX->plane.getValue();
    d->clipX->plane.setValue(SbPlane(pln.getNormal(),d->flipX ? -val : val));
}

void Clipping::on_clipY_valueChanged(double val)
{
    SbPlane pln = d->clipY->plane.getValue();
    d->clipY->plane.setValue(SbPlane(pln.getNormal(),d->flipY ? -val : val));
}

void Clipping::on_clipZ_valueChanged(double val)
{
    SbPlane pln = d->clipZ->plane.getValue();
    d->clipZ->plane.setValue(SbPlane(pln.getNormal(),d->flipZ ? -val : val));
}

void Clipping::on_flipClipX_clicked()
{
    d->flipX = !d->flipX;
    SbPlane pln = d->clipX->plane.getValue();
    d->clipX->plane.setValue(SbPlane(-pln.getNormal(),-pln.getDistanceFromOrigin()));
}

void Clipping::on_flipClipY_clicked()
{
    d->flipY = !d->flipY;
    SbPlane pln = d->clipY->plane.getValue();
    d->clipY->plane.setValue(SbPlane(-pln.getNormal(),-pln.getDistanceFromOrigin()));
}

void Clipping::on_flipClipZ_clicked()
{
    d->flipZ = !d->flipZ;
    SbPlane pln = d->clipZ->plane.getValue();
    d->clipZ->plane.setValue(SbPlane(-pln.getNormal(),-pln.getDistanceFromOrigin()));
}

void Clipping::on_groupBoxView_toggled(bool on)
{
    if (on) {
        d->ui.groupBoxX->setChecked(false);
        d->ui.groupBoxY->setChecked(false);
        d->ui.groupBoxZ->setChecked(false);
    }

    d->clipView->on.setValue(on);
}

void Clipping::on_clipView_valueChanged(double val)
{
    SbPlane pln = d->clipView->plane.getValue();
    d->clipView->plane.setValue(SbPlane(pln.getNormal(),val));
}

void Clipping::on_fromView_clicked()
{
    if (d->view) {
        Gui::View3DInventorViewer* view = d->view->getViewer();
        SbVec3f dir = view->getViewDirection();
        SbPlane pln = d->clipView->plane.getValue();
        d->clipView->plane.setValue(SbPlane(dir,pln.getDistanceFromOrigin()));
    }
}

void Clipping::on_adjustViewdirection_toggled(bool on)
{
    d->ui.dirX->setDisabled(on);
    d->ui.dirY->setDisabled(on);
    d->ui.dirZ->setDisabled(on);
    d->ui.fromView->setDisabled(on);

    if (on)
        d->sensor->schedule();
    else
        d->sensor->unschedule();
}

void Clipping::on_dirX_valueChanged(double)
{
    double x = d->ui.dirX->value();
    double y = d->ui.dirY->value();
    double z = d->ui.dirZ->value();

    SbPlane pln = d->clipView->plane.getValue();
    SbVec3f normal(x,y,z);
    if (normal.sqrLength() > 0.0f)
        d->clipView->plane.setValue(SbPlane(normal,pln.getDistanceFromOrigin()));
}

void Clipping::on_dirY_valueChanged(double)
{
    double x = d->ui.dirX->value();
    double y = d->ui.dirY->value();
    double z = d->ui.dirZ->value();

    SbPlane pln = d->clipView->plane.getValue();
    SbVec3f normal(x,y,z);
    if (normal.sqrLength() > 0.0f)
        d->clipView->plane.setValue(SbPlane(normal,pln.getDistanceFromOrigin()));
}

void Clipping::on_dirZ_valueChanged(double)
{
    double x = d->ui.dirX->value();
    double y = d->ui.dirY->value();
    double z = d->ui.dirZ->value();

    SbPlane pln = d->clipView->plane.getValue();
    SbVec3f normal(x,y,z);
    if (normal.sqrLength() > 0.0f)
        d->clipView->plane.setValue(SbPlane(normal,pln.getDistanceFromOrigin()));
}

// ---------------------------------------

/* TRANSLATOR Gui::Dialog::TaskClipping */

TaskClipping::TaskClipping(Gui::View3DInventor* view)
{
    QWidget* widget = new Clipping(view);
    Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), false, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskClipping::~TaskClipping()
{
    // automatically deleted in the sub-class
}

#include "moc_Clipping.cpp"
