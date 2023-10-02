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
# include <Inventor/sensors/SoTimerSensor.h>
# include <QDockWidget>
# include <QPointer>
#endif

#include "Clipping.h"
#include "ui_Clipping.h"
#include "DockWindowManager.h"
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
    bool flipX{false};
    bool flipY{false};
    bool flipZ{false};
    SoTimerSensor* sensor;
    Private()
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

        node = nullptr;
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
        Q_UNUSED(sensor);
        auto self = static_cast<Private*>(data);
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
  : QDialog(parent)
  , d(new Private)
{
    // create widgets
    d->ui.setupUi(this);
    setupConnections();

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

    SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.apply(viewer->getSceneGraph());
    SbBox3f box = action.getBoundingBox();

    if (!box.isEmpty()) {
        SbVec3f cnt = box.getCenter();
        d->ui.clipView->setValue(cnt[2]);
        d->ui.clipX->setValue(cnt[0]);
        d->ui.clipY->setValue(cnt[1]);
        d->ui.clipZ->setValue(cnt[2]);

        int minDecimals = 2;
        float lenx, leny,lenz;
        box.getSize(lenx, leny, lenz);
        int steps = 100;
        float minlen = std::min<float>(lenx, std::min<float>(leny, lenz));

        // determine the single step values
        {
            minlen = minlen / steps;
            int dim = static_cast<int>(log10(minlen));
            double singleStep = pow(10.0, dim);
            d->ui.clipView->setSingleStep(singleStep);
            minDecimals = std::max(minDecimals, -dim);
        }
        {
            lenx = lenx / steps;
            int dim = static_cast<int>(log10(lenx));
            double singleStep = pow(10.0, dim);
            d->ui.clipX->setSingleStep(singleStep);
        }
        {
            leny = leny / steps;
            int dim = static_cast<int>(log10(leny));
            double singleStep = pow(10.0, dim);
            d->ui.clipY->setSingleStep(singleStep);
        }
        {
            lenz = lenz / steps;
            int dim = static_cast<int>(log10(lenz));
            double singleStep = pow(10.0, dim);
            d->ui.clipZ->setSingleStep(singleStep);
        }

        // set decimals
        d->ui.clipView->setDecimals(minDecimals);
        d->ui.clipX->setDecimals(minDecimals);
        d->ui.clipY->setDecimals(minDecimals);
        d->ui.clipZ->setDecimals(minDecimals);
    }
}

Clipping* Clipping::makeDockWidget(Gui::View3DInventor* view)
{
    // embed this dialog into a QDockWidget
    auto clipping = new Clipping(view);
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    QDockWidget* dw = pDockMgr->addDockWindow("Clipping", clipping, Qt::LeftDockWidgetArea);
    dw->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    dw->show();

    return clipping;
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

void Clipping::setupConnections()
{
    connect(d->ui.groupBoxX, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxXToggled);
    connect(d->ui.groupBoxY, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxYToggled);
    connect(d->ui.groupBoxZ, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxZToggled);
    connect(d->ui.clipX, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipXValueChanged);
    connect(d->ui.clipY, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipYValueChanged);
    connect(d->ui.clipZ, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipZValueChanged);
    connect(d->ui.flipClipX, &QPushButton::clicked,
            this, &Clipping::onFlipClipXClicked);
    connect(d->ui.flipClipY, &QPushButton::clicked,
            this, &Clipping::onFlipClipYClicked);
    connect(d->ui.flipClipZ, &QPushButton::clicked,
            this, &Clipping::onFlipClipZClicked);
    connect(d->ui.groupBoxView, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxViewToggled);
    connect(d->ui.clipView, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipViewValueChanged);
    connect(d->ui.fromView, &QPushButton::clicked,
            this, &Clipping::onFromViewClicked);
    connect(d->ui.adjustViewdirection, &QCheckBox::toggled,
            this, &Clipping::onAdjustViewdirectionToggled);
    connect(d->ui.dirX, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onDirXValueChanged);
    connect(d->ui.dirY, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onDirYValueChanged);
    connect(d->ui.dirZ, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onDirZValueChanged);
}

void Clipping::reject()
{
    QDialog::reject();
    auto dw = qobject_cast<QDockWidget*>(parent());
    if (dw) {
        dw->deleteLater();
    }
}

void Clipping::onGroupBoxXToggled(bool on)
{
    if (on) {
        d->ui.groupBoxView->setChecked(false);
    }

    d->clipX->on.setValue(on);
}

void Clipping::onGroupBoxYToggled(bool on)
{
    if (on) {
        d->ui.groupBoxView->setChecked(false);
    }

    d->clipY->on.setValue(on);
}

void Clipping::onGroupBoxZToggled(bool on)
{
    if (on) {
        d->ui.groupBoxView->setChecked(false);
    }

    d->clipZ->on.setValue(on);
}

void Clipping::onClipXValueChanged(double val)
{
    SbPlane pln = d->clipX->plane.getValue();
    d->clipX->plane.setValue(SbPlane(pln.getNormal(),d->flipX ? -val : val));
}

void Clipping::onClipYValueChanged(double val)
{
    SbPlane pln = d->clipY->plane.getValue();
    d->clipY->plane.setValue(SbPlane(pln.getNormal(),d->flipY ? -val : val));
}

void Clipping::onClipZValueChanged(double val)
{
    SbPlane pln = d->clipZ->plane.getValue();
    d->clipZ->plane.setValue(SbPlane(pln.getNormal(),d->flipZ ? -val : val));
}

void Clipping::onFlipClipXClicked()
{
    d->flipX = !d->flipX;
    SbPlane pln = d->clipX->plane.getValue();
    d->clipX->plane.setValue(SbPlane(-pln.getNormal(),-pln.getDistanceFromOrigin()));
}

void Clipping::onFlipClipYClicked()
{
    d->flipY = !d->flipY;
    SbPlane pln = d->clipY->plane.getValue();
    d->clipY->plane.setValue(SbPlane(-pln.getNormal(),-pln.getDistanceFromOrigin()));
}

void Clipping::onFlipClipZClicked()
{
    d->flipZ = !d->flipZ;
    SbPlane pln = d->clipZ->plane.getValue();
    d->clipZ->plane.setValue(SbPlane(-pln.getNormal(),-pln.getDistanceFromOrigin()));
}

void Clipping::onGroupBoxViewToggled(bool on)
{
    if (on) {
        d->ui.groupBoxX->setChecked(false);
        d->ui.groupBoxY->setChecked(false);
        d->ui.groupBoxZ->setChecked(false);
    }

    d->clipView->on.setValue(on);
}

void Clipping::onClipViewValueChanged(double val)
{
    SbPlane pln = d->clipView->plane.getValue();
    d->clipView->plane.setValue(SbPlane(pln.getNormal(),val));
}

void Clipping::onFromViewClicked()
{
    if (d->view) {
        Gui::View3DInventorViewer* view = d->view->getViewer();
        SbVec3f dir = view->getViewDirection();
        SbPlane pln = d->clipView->plane.getValue();
        d->clipView->plane.setValue(SbPlane(dir,pln.getDistanceFromOrigin()));
    }
}

void Clipping::onAdjustViewdirectionToggled(bool on)
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

void Clipping::onDirXValueChanged(double)
{
    double x = d->ui.dirX->value();
    double y = d->ui.dirY->value();
    double z = d->ui.dirZ->value();

    SbPlane pln = d->clipView->plane.getValue();
    SbVec3f normal(x,y,z);
    if (normal.sqrLength() > 0.0f)
        d->clipView->plane.setValue(SbPlane(normal,pln.getDistanceFromOrigin()));
}

void Clipping::onDirYValueChanged(double)
{
    double x = d->ui.dirX->value();
    double y = d->ui.dirY->value();
    double z = d->ui.dirZ->value();

    SbPlane pln = d->clipView->plane.getValue();
    SbVec3f normal(x,y,z);
    if (normal.sqrLength() > 0.0f)
        d->clipView->plane.setValue(SbPlane(normal,pln.getDistanceFromOrigin()));
}

void Clipping::onDirZValueChanged(double)
{
    double x = d->ui.dirX->value();
    double y = d->ui.dirY->value();
    double z = d->ui.dirZ->value();

    SbPlane pln = d->clipView->plane.getValue();
    SbVec3f normal(x,y,z);
    if (normal.sqrLength() > 0.0f)
        d->clipView->plane.setValue(SbPlane(normal,pln.getDistanceFromOrigin()));
}

#include "moc_Clipping.cpp"
