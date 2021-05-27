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
# include <QScrollArea>
# include <QAction>
# include <cmath>
#endif

#include <QStackedWidget>

#include "Clipping.h"
#include "ui_Clipping.h"
#include "Application.h"
#include "DockWindowManager.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewParams.h"

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

        node = 0;
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
  : QDialog(parent)
  , d(new Private)
{
    connect(view, SIGNAL(destroyed(QObject*)), this, SLOT(onViewDestroyed(QObject*)));

    // create widgets
    d->ui.setupUi(this);

    d->ui.checkBoxFill->setChecked(ViewParams::getSectionFill());
    d->ui.checkBoxInvert->setChecked(ViewParams::getSectionFillInvert());
    d->ui.checkBoxConcave->setChecked(ViewParams::getSectionConcave());
    d->ui.checkBoxOnTop->setChecked(ViewParams::getNoSectionOnTop());
    d->ui.checkBoxOnTop->setDisabled(ViewParams::getSectionConcave());

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

static QPointer<QDockWidget> _DockWidget;
static QPointer<QStackedWidget> _StackedWidget;
static std::map<QObject*, QPointer<QScrollArea> > _Clippings;
static bool _Inited = false;

void Clipping::onViewDestroyed(QObject *o)
{
    _Clippings.erase(o);
    auto parent = parentWidget();
    if (parent)
        parent->deleteLater();
}

static QWidget *bindView(Gui::View3DInventor *view)
{
    if (!_StackedWidget)
        return nullptr;
    auto &scrollArea = _Clippings[view];
    if (!scrollArea) {
        scrollArea = new QScrollArea(nullptr);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto clipping = new Clipping(view, scrollArea);
        scrollArea->setWidget(clipping);
        _StackedWidget->addWidget(scrollArea);
    }
    _StackedWidget->setCurrentWidget(scrollArea);
    return scrollArea;
}

void Clipping::toggle()
{
    auto view = qobject_cast<Gui::View3DInventor*>(Application::Instance->activeView());
    if (!view)
        return;

    if (!_Inited) {
        _Inited = true;
        Application::Instance->signalActivateView.connect(
            [](const Gui::MDIView *view) {
                auto view3d = qobject_cast<const Gui::View3DInventor*>(view);
                if (!view3d || !_DockWidget)
                    return;
                bindView(const_cast<Gui::View3DInventor*>(view3d));
            });
    }

    bool doToggle = true;
    if (!_DockWidget || !_StackedWidget) {
        if (_DockWidget)
            _DockWidget->deleteLater();
        doToggle = false;
        _StackedWidget = new QStackedWidget(nullptr);
        Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
        _DockWidget = pDockMgr->addDockWindow("Clipping", _StackedWidget, Qt::LeftDockWidgetArea);
        _DockWidget->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        _DockWidget->show();
    }

    auto widget = bindView(view);
    if (widget && doToggle)
        _DockWidget->toggleViewAction()->activate(QAction::Trigger);
}

/** Destroys the object and frees any allocated resources */
Clipping::~Clipping()
{
    if (d->view) {
        d->node->removeChild(d->clipX);
        d->node->removeChild(d->clipY);
        d->node->removeChild(d->clipZ);
        d->node->removeChild(d->clipView);
        d->node->unref();
    }
    delete d;
}

void Clipping::done(int r)
{
    if (_DockWidget)
        _DockWidget->deleteLater();
    _Clippings.clear();
    QDialog::done(r);
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

void Clipping::on_checkBoxFill_toggled(bool on)
{
    ViewParams::setSectionFill(on);
    if (d->view)
        d->view->getViewer()->redraw();
}

void Clipping::on_checkBoxInvert_toggled(bool on)
{
    ViewParams::setSectionFillInvert(on);
    if (d->view)
        d->view->getViewer()->redraw();
}

void Clipping::on_checkBoxConcave_toggled(bool on)
{
    ViewParams::setSectionConcave(on);
    d->ui.checkBoxOnTop->setDisabled(on);
    if (d->view)
        d->view->getViewer()->redraw();
}

void Clipping::on_checkBoxOnTop_toggled(bool on)
{
    ViewParams::setNoSectionOnTop(on);
    if (d->view)
        d->view->getViewer()->redraw();
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

#include "moc_Clipping.cpp"
