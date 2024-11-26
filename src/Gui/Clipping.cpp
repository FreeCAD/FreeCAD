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
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/sensors/SoTimerSensor.h>
#include <QDockWidget>
#include <QPointer>
#endif

#include "Clipping.h"
#include "ui_Clipping.h"
#include "DockWindowManager.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

using namespace Gui::Dialog;

class Clipping::Private
{
public:
    static Ui_Clipping ui;
    static Private* activeClip;
    QPointer<Gui::View3DInventor const> view;
    SoGroup* node;
    SoClipPlane* clipX;
    SoClipPlane* clipY;
    SoClipPlane* clipZ;
    SoClipPlane* clipView;
    bool flipX {false};
    bool flipY {false};
    bool flipZ {false};
    SoTimerSensor* sensor;
    bool isSensing {false};

    Private()
    {
        clipX = new SoClipPlane();
        clipX->on.setValue(false);
        clipX->plane.setValue(SbPlane(SbVec3f(1, 0, 0), 0));
        clipX->ref();

        clipY = new SoClipPlane();
        clipY->on.setValue(false);
        clipY->plane.setValue(SbPlane(SbVec3f(0, 1, 0), 0));
        clipY->ref();

        clipZ = new SoClipPlane();
        clipZ->on.setValue(false);
        clipZ->plane.setValue(SbPlane(SbVec3f(0, 0, 1), 0));
        clipZ->ref();

        clipView = new SoClipPlane();
        clipView->on.setValue(false);
        clipView->plane.setValue(SbPlane(SbVec3f(0, 0, 1), 0));
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

    static void moveCallback(void* data, SoSensor* sensor)
    {
        Q_UNUSED(sensor);
        auto self = static_cast<Private*>(data);
        Gui::View3DInventorViewer const* viewer = self->view.data() ? self->view->getViewer() : nullptr;
        if (viewer) {
            SoClipPlane* clip = self->clipView;
            SbPlane pln = clip->plane.getValue();
            clip->plane.setValue(SbPlane(viewer->getViewDirection(), pln.getDistanceFromOrigin()));
        }
        else {
            self->sensor->unschedule();
            self->isSensing = false;
        }
    }
};

Ui_Clipping Clipping::Private::ui;

Clipping::Private* Clipping::Private::activeClip = nullptr;

/* TRANSLATOR Gui::Dialog::Clipping */

Clipping::Clipping(Gui::View3DInventor const* view, QWidget* parent)
    : QDialog(parent)
{
    // create widgets
    Private::ui.setupUi(this);

    Private::ui.clipView->setRange(-INT_MAX, INT_MAX);
    Private::ui.clipView->setSingleStep(0.1f);
    Private::ui.clipX->setRange(-INT_MAX, INT_MAX);
    Private::ui.clipX->setSingleStep(0.1f);
    Private::ui.clipY->setRange(-INT_MAX, INT_MAX);
    Private::ui.clipY->setSingleStep(0.1f);
    Private::ui.clipZ->setRange(-INT_MAX, INT_MAX);
    Private::ui.clipZ->setSingleStep(0.1f);

    Private::ui.dirX->setRange(-INT_MAX, INT_MAX);
    Private::ui.dirX->setSingleStep(0.1f);
    Private::ui.dirY->setRange(-INT_MAX, INT_MAX);
    Private::ui.dirY->setSingleStep(0.1f);
    Private::ui.dirZ->setRange(-INT_MAX, INT_MAX);
    Private::ui.dirZ->setSingleStep(0.1f);
    Private::ui.dirZ->setValue(1.0f);

    auto p = new Private();
    p->view = view;
    Private::activeClip = p;
    setupConnections();
    setupPrivate(p);
}

void Clipping::setupPrivate(Private* p)
{
    d.insert(p->view, p);

    View3DInventorViewer* viewer = p->view->getViewer();
    auto sceneGraph = viewer->getSceneGraph();

    p->node = static_cast<SoGroup*>(sceneGraph);
    p->node->ref();
    p->node->insertChild(p->clipX, 0);
    p->node->insertChild(p->clipY, 0);
    p->node->insertChild(p->clipZ, 0);
    p->node->insertChild(p->clipView, 0);

    SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.apply(sceneGraph);
    SbBox3f box = action.getBoundingBox();

    if (!box.isEmpty()) {
        SbVec3f cnt = box.getCenter();
        Private::ui.clipView->setValue(cnt[2]);
        Private::ui.clipX->setValue(cnt[0]);
        Private::ui.clipY->setValue(cnt[1]);
        Private::ui.clipZ->setValue(cnt[2]);

        p->clipView->plane.setValue(SbPlane(SbVec3f(0, 0, 1), cnt[2]));
        p->clipX->plane.setValue(SbPlane(SbVec3f(1, 0, 0), cnt[0]));
        p->clipY->plane.setValue(SbPlane(SbVec3f(0, 1, 0), cnt[1]));
        p->clipZ->plane.setValue(SbPlane(SbVec3f(0, 0, 1), cnt[2]));

        int minDecimals = 2;
        float lenx, leny, lenz;
        box.getSize(lenx, leny, lenz);
        int steps = 100;
        float minlen = std::min<float>(lenx, std::min<float>(leny, lenz));

        // determine the single step values
        {
            minlen = minlen / steps;
            int dim = static_cast<int>(log10(minlen));
            double singleStep = pow(10.0, dim);
            Private::ui.clipView->setSingleStep(singleStep);
            minDecimals = std::max(minDecimals, -dim);
        }
        {
            lenx = lenx / steps;
            int dim = static_cast<int>(log10(lenx));
            double singleStep = pow(10.0, dim);
            Private::ui.clipX->setSingleStep(singleStep);
        }
        {
            leny = leny / steps;
            int dim = static_cast<int>(log10(leny));
            double singleStep = pow(10.0, dim);
            Private::ui.clipY->setSingleStep(singleStep);
        }
        {
            lenz = lenz / steps;
            int dim = static_cast<int>(log10(lenz));
            double singleStep = pow(10.0, dim);
            Private::ui.clipZ->setSingleStep(singleStep);
        }

        // set decimals
        Private::ui.clipView->setDecimals(minDecimals);
        Private::ui.clipX->setDecimals(minDecimals);
        Private::ui.clipY->setDecimals(minDecimals);
        Private::ui.clipZ->setDecimals(minDecimals);
    }
}

void Clipping::switchUi(Private* p)
{
    Private::ui.groupBoxX->setChecked(
        p->clipX->on.getValue()
    );
    Private::ui.clipX->setValue(
        p->clipX->plane.getValue().getDistanceFromOrigin()
    );
    Private::ui.flipClipX->setChecked(
        p->flipX
    );

    Private::ui.groupBoxY->setChecked(
        p->clipY->on.getValue()
    );
    Private::ui.clipY->setValue(
        p->clipY->plane.getValue().getDistanceFromOrigin()
    );
    Private::ui.flipClipY->setChecked(
        p->flipY
    );

    Private::ui.groupBoxZ->setChecked(
        p->clipZ->on.getValue()
    );
    Private::ui.clipZ->setValue(
        p->clipZ->plane.getValue().getDistanceFromOrigin()
    );
    Private::ui.flipClipZ->setChecked(
        p->flipZ
    );

    auto pln = p->clipView->plane.getValue();
    auto d = pln.getDistanceFromOrigin();
    auto n = pln.getNormal();
    Private::ui.groupBoxView->setChecked(
        p->clipView->on.getValue()
    );
    Private::ui.clipView->setValue(
        d
    );
    Private::ui.adjustViewdirection->setChecked(
        p->isSensing
    );
    Private::ui.dirX->setValue(
        n[0]
    );
    Private::ui.dirY->setValue(
        n[1]
    );
    Private::ui.dirZ->setValue(
        n[2]
    );
}

Clipping* Clipping::makeDockWidget(Gui::View3DInventor const* view)
{
    // embed this dialog into a QDockWidget
    auto clipping = new Clipping(view);
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    QDockWidget* dw = pDockMgr->addDockWindow("Clipping", clipping, Qt::LeftDockWidgetArea);
    dw->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dw->show();

    return clipping;
}

void Clipping::switchView(Gui::View3DInventor const* view)
{
    auto p = d.value(view);
    if (!p) {
        p = new Private();
        p->view = view;
        setupPrivate(p);
    }
    Private::activeClip = p;
    switchUi(p);
}

/** Destroys the object and frees any allocated resources */
Clipping::~Clipping()
{
    // clang-format off
    disconnect(Private::ui.groupBoxX, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxXToggled);
    disconnect(Private::ui.groupBoxY, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxYToggled);
    disconnect(Private::ui.groupBoxZ, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxZToggled);
    disconnect(Private::ui.clipX, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipXValueChanged);
    disconnect(Private::ui.clipY, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipYValueChanged);
    disconnect(Private::ui.clipZ, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipZValueChanged);
    disconnect(Private::ui.flipClipX, &QPushButton::clicked,
            this, &Clipping::onFlipClipXClicked);
    disconnect(Private::ui.flipClipY, &QPushButton::clicked,
            this, &Clipping::onFlipClipYClicked);
    disconnect(Private::ui.flipClipZ, &QPushButton::clicked,
            this, &Clipping::onFlipClipZClicked);
    disconnect(Private::ui.groupBoxView, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxViewToggled);
    disconnect(Private::ui.clipView, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipViewValueChanged);
    disconnect(Private::ui.fromView, &QPushButton::clicked,
            this, &Clipping::onFromViewClicked);
    disconnect(Private::ui.adjustViewdirection, &QCheckBox::toggled,
            this, &Clipping::onAdjustViewdirectionToggled);
    disconnect(Private::ui.dirX, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onDirXValueChanged);
    disconnect(Private::ui.dirY, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onDirYValueChanged);
    disconnect(Private::ui.dirZ, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onDirZValueChanged);
    // clang-format on

    Private::activeClip = nullptr;

    for (QMap<Gui::View3DInventor const*, Private *>::Iterator it = d.begin(); it != d.end(); ++it) {
        auto p = it.value();
        p->node->removeChild(p->clipX);
        p->node->removeChild(p->clipY);
        p->node->removeChild(p->clipZ);
        p->node->removeChild(p->clipView);
        p->node->unref();
        delete p;
    }
}

void Clipping::setupConnections()
{
    // clang-format off
    connect(Private::ui.groupBoxX, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxXToggled);
    connect(Private::ui.groupBoxY, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxYToggled);
    connect(Private::ui.groupBoxZ, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxZToggled);
    connect(Private::ui.clipX, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipXValueChanged);
    connect(Private::ui.clipY, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipYValueChanged);
    connect(Private::ui.clipZ, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipZValueChanged);
    connect(Private::ui.flipClipX, &QPushButton::clicked,
            this, &Clipping::onFlipClipXClicked);
    connect(Private::ui.flipClipY, &QPushButton::clicked,
            this, &Clipping::onFlipClipYClicked);
    connect(Private::ui.flipClipZ, &QPushButton::clicked,
            this, &Clipping::onFlipClipZClicked);
    connect(Private::ui.groupBoxView, &QGroupBox::toggled,
            this, &Clipping::onGroupBoxViewToggled);
    connect(Private::ui.clipView, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onClipViewValueChanged);
    connect(Private::ui.fromView, &QPushButton::clicked,
            this, &Clipping::onFromViewClicked);
    connect(Private::ui.adjustViewdirection, &QCheckBox::toggled,
            this, &Clipping::onAdjustViewdirectionToggled);
    connect(Private::ui.dirX, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onDirXValueChanged);
    connect(Private::ui.dirY, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onDirYValueChanged);
    connect(Private::ui.dirZ, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &Clipping::onDirZValueChanged);
    // clang-format on
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
        Private::ui.groupBoxView->setChecked(false);
    }

    Private::activeClip->clipX->on.setValue(on);
}

void Clipping::onGroupBoxYToggled(bool on)
{
    if (on) {
        Private::ui.groupBoxView->setChecked(false);
    }

    Private::activeClip->clipY->on.setValue(on);
}

void Clipping::onGroupBoxZToggled(bool on)
{
    if (on) {
        Private::ui.groupBoxView->setChecked(false);
    }

    Private::activeClip->clipZ->on.setValue(on);
}

void Clipping::onClipXValueChanged(double val)
{
    SbPlane pln = Private::activeClip->clipX->plane.getValue();
    Private::activeClip->clipX->plane.setValue(SbPlane(pln.getNormal(), Private::activeClip->flipX ? -val : val));
}

void Clipping::onClipYValueChanged(double val)
{
    SbPlane pln = Private::activeClip->clipY->plane.getValue();
    Private::activeClip->clipY->plane.setValue(SbPlane(pln.getNormal(), Private::activeClip->flipY ? -val : val));
}

void Clipping::onClipZValueChanged(double val)
{
    SbPlane pln = Private::activeClip->clipZ->plane.getValue();
    Private::activeClip->clipZ->plane.setValue(SbPlane(pln.getNormal(), Private::activeClip->flipZ ? -val : val));
}

void Clipping::onFlipClipXClicked()
{
    Private::activeClip->flipX = !Private::activeClip->flipX;
    SbPlane pln = Private::activeClip->clipX->plane.getValue();
    Private::activeClip->clipX->plane.setValue(SbPlane(-pln.getNormal(), -pln.getDistanceFromOrigin()));
}

void Clipping::onFlipClipYClicked()
{
    Private::activeClip->flipY = !Private::activeClip->flipY;
    SbPlane pln = Private::activeClip->clipY->plane.getValue();
    Private::activeClip->clipY->plane.setValue(SbPlane(-pln.getNormal(), -pln.getDistanceFromOrigin()));
}

void Clipping::onFlipClipZClicked()
{
    Private::activeClip->flipZ = !Private::activeClip->flipZ;
    SbPlane pln = Private::activeClip->clipZ->plane.getValue();
    Private::activeClip->clipZ->plane.setValue(SbPlane(-pln.getNormal(), -pln.getDistanceFromOrigin()));
}

void Clipping::onGroupBoxViewToggled(bool on)
{
    if (on) {
        Private::ui.groupBoxX->setChecked(false);
        Private::ui.groupBoxY->setChecked(false);
        Private::ui.groupBoxZ->setChecked(false);
    }

    Private::activeClip->clipView->on.setValue(on);
}

void Clipping::onClipViewValueChanged(double val)
{
    SbPlane pln = Private::activeClip->clipView->plane.getValue();
    Private::activeClip->clipView->plane.setValue(SbPlane(pln.getNormal(), val));
}

void Clipping::onFromViewClicked()
{
    if (Private::activeClip) {
        Gui::View3DInventorViewer* viewer = Private::activeClip->view->getViewer();
        SbVec3f dir = viewer->getViewDirection();
        SbPlane pln = Private::activeClip->clipView->plane.getValue();
        Private::activeClip->clipView->plane.setValue(SbPlane(dir, pln.getDistanceFromOrigin()));
    }
}

void Clipping::onAdjustViewdirectionToggled(bool on)
{
    Private::ui.dirX->setDisabled(on);
    Private::ui.dirY->setDisabled(on);
    Private::ui.dirZ->setDisabled(on);
    Private::ui.fromView->setDisabled(on);

    Private::activeClip->isSensing = on;
    if (on) {
        Private::activeClip->sensor->schedule();
    }
    else {
        Private::activeClip->sensor->unschedule();
    }
}

void Clipping::onDirXValueChanged(double)
{
    double x = Private::ui.dirX->value();
    double y = Private::ui.dirY->value();
    double z = Private::ui.dirZ->value();

    SbPlane pln = Private::activeClip->clipView->plane.getValue();
    SbVec3f normal(x, y, z);
    if (normal.sqrLength() > 0.0f) {
        Private::activeClip->clipView->plane.setValue(SbPlane(normal, pln.getDistanceFromOrigin()));
    }
}

void Clipping::onDirYValueChanged(double)
{
    double x = Private::ui.dirX->value();
    double y = Private::ui.dirY->value();
    double z = Private::ui.dirZ->value();

    SbPlane pln = Private::activeClip->clipView->plane.getValue();
    SbVec3f normal(x, y, z);
    if (normal.sqrLength() > 0.0f) {
        Private::activeClip->clipView->plane.setValue(SbPlane(normal, pln.getDistanceFromOrigin()));
    }
}

void Clipping::onDirZValueChanged(double)
{
    double x = Private::ui.dirX->value();
    double y = Private::ui.dirY->value();
    double z = Private::ui.dirZ->value();

    SbPlane pln = Private::activeClip->clipView->plane.getValue();
    SbVec3f normal(x, y, z);
    if (normal.sqrLength() > 0.0f) {
        Private::activeClip->clipView->plane.setValue(SbPlane(normal, pln.getDistanceFromOrigin()));
    }
}

#include "moc_Clipping.cpp"
