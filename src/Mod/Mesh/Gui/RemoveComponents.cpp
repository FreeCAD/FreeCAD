/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <algorithm>
# include <climits>
# include <boost/bind.hpp>
# include <QEventLoop>
# include <QImage>
# include <QPushButton>
# include <Inventor/SbBox2s.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoImage.h>
#endif

#include "RemoveComponents.h"
#include "ui_RemoveComponents.h"
#include "ViewProvider.h"
#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SoFCSelectionAction.h>
#include <Gui/MouseSelection.h>
#include <Gui/NavigationStyle.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/TopoAlgorithm.h>
#include <Mod/Mesh/App/Core/Tools.h>

using namespace MeshGui;


RemoveComponents::RemoveComponents(QWidget* parent, Qt::WFlags fl)
  : QWidget(parent, fl)
{
    ui = new Ui_RemoveComponents;
    ui->setupUi(this);
    ui->spSelectComp->setRange(1, INT_MAX);
    ui->spSelectComp->setValue(10);
    ui->spDeselectComp->setRange(1, INT_MAX);
    ui->spDeselectComp->setValue(10);

    meshSel.setCheckOnlyVisibleTriangles(ui->visibleTriangles->isChecked());
    meshSel.setCheckOnlyPointToUserTriangles(ui->screenTriangles->isChecked());
}

RemoveComponents::~RemoveComponents()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void RemoveComponents::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void RemoveComponents::on_selectRegion_clicked()
{
    meshSel.startSelection();
}

void RemoveComponents::on_deselectRegion_clicked()
{
    meshSel.startDeselection();
}

void RemoveComponents::on_selectAll_clicked()
{
    // select the complete meshes
    meshSel.fullSelection();
}

void RemoveComponents::on_deselectAll_clicked()
{
    // deselect all meshes
    meshSel.clearSelection();
}

void RemoveComponents::on_selectComponents_clicked()
{
    // select components upto a certain size
    int size = ui->spSelectComp->value();
    meshSel.selectComponent(size);
}

void RemoveComponents::on_deselectComponents_clicked()
{
    // deselect components from a certain size on
    int size = ui->spDeselectComp->value();
    meshSel.deselectComponent(size);
}

void RemoveComponents::on_visibleTriangles_toggled(bool on)
{
    meshSel.setCheckOnlyVisibleTriangles(on);
}

void RemoveComponents::on_screenTriangles_toggled(bool on)
{
    meshSel.setCheckOnlyPointToUserTriangles(on);
}

void RemoveComponents::on_cbSelectComp_toggled(bool on)
{
    meshSel.setAddComponentOnClick(on);
}

void RemoveComponents::on_cbDeselectComp_toggled(bool on)
{
    meshSel.setRemoveComponentOnClick(on);
}

void RemoveComponents::deleteSelection()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) return;
    // delete all selected faces
    doc->openCommand("Delete selection");
    bool ok = meshSel.deleteSelection();
    if (!ok)
        doc->abortCommand();
    else
        doc->commitCommand();
}

void RemoveComponents::invertSelection()
{
    meshSel.invertSelection();
}

void RemoveComponents::on_selectTriangle_clicked()
{
    meshSel.selectTriangle();
    meshSel.setAddComponentOnClick(ui->cbSelectComp->isChecked());
}

void RemoveComponents::on_deselectTriangle_clicked()
{
    meshSel.deselectTriangle();
    meshSel.setRemoveComponentOnClick(ui->cbDeselectComp->isChecked());
}

void RemoveComponents::reject()
{
    // deselect all meshes
    meshSel.clearSelection();
}

void RemoveComponents::paintSelection()
{
#if 0
    SoAnnotation* hudRoot = new SoAnnotation;
    hudRoot->ref();

    SoOrthographicCamera* hudCam = new SoOrthographicCamera();
    hudCam->viewportMapping = SoCamera::LEAVE_ALONE;
    // Set the position in the window.
    // [0, 0] is in the center of the screen.
    //
    SoTranslation* hudTrans = new SoTranslation;
    hudTrans->translation.setValue(-1.0f, -1.0f, 0.0f);

    QImage image(100,100,QImage::Format_ARGB32_Premultiplied);
    image.fill(0x00000000);
    SoSFImage sfimage;
    Gui::BitmapFactory().convert(image, sfimage);
    SoImage* hudImage = new SoImage();
    hudImage->image = sfimage;

    // Assemble the parts...
    //
    hudRoot->addChild(hudCam);
    hudRoot->addChild(hudTrans);
    hudRoot->addChild(hudImage);

    Gui::View3DInventorViewer* viewer = this->getViewer();
    static_cast<SoGroup*>(viewer->getSceneGraph())->addChild(hudRoot);

    QWidget* gl = viewer->getGLWidget();
    DrawingPlane pln(hudImage->image, viewer, gl);
    gl->installEventFilter(&pln);
    QEventLoop loop;
    QObject::connect(&pln, SIGNAL(emitSelection()), &loop, SLOT(quit()));
    loop.exec();
    static_cast<SoGroup*>(viewer->getSceneGraph())->removeChild(hudRoot);
#endif
}

// ---------------------------------------

DrawingPlane::DrawingPlane(SoSFImage& data, SoQtViewer* s, QWidget* view)
  : QObject(), data(data), glView(view), soqt(s), image(view->size(), QImage::Format_ARGB32)
{
    image.fill(qRgba(255, 255, 255, 0));

    myPenWidth = 50;

    QRgb p = qRgba(255,255,0,0);
    int q = ((p << 16) & 0xff0000) | ((p >> 16) & 0xff) | (p & 0xff00ff00);
    int r = qRed(q);
    int g = qGreen(q);
    int b = qBlue(q);
    myPenColor = qRgb(r,g,b);//Qt::yellow;
    myRadius = 5.0f;
}

DrawingPlane::~DrawingPlane()
{
}

void DrawingPlane::changeRadius(double radius)
{
    this->myRadius = (double)radius;
}

void DrawingPlane::mousePressEvent(QMouseEvent *event)
{
    // Calculate the given radius from mm into px
    const SbViewportRegion& vp = soqt->getViewportRegion();
    float fRatio = vp.getViewportAspectRatio();
    const SbVec2s& sp = vp.getViewportSizePixels();
    float dX, dY; vp.getViewportSize().getValue(dX, dY);
    SbViewVolume vv = soqt->getCamera()->getViewVolume(fRatio);

    SbVec3f p1(0,0,0);
    SbVec3f p2(0,this->myRadius,0);
    vv.projectToScreen(p1, p1);
    vv.projectToScreen(p2, p2);

    if (fRatio > 1.0f) {
        p1[0] = (p1[0] - 0.5f*dX) / fRatio + 0.5f*dX;
        p2[0] = (p2[0] - 0.5f*dX) / fRatio + 0.5f*dX;
    }
    else if (fRatio < 1.0f) {
        p1[1] = (p1[1] - 0.5f*dY) * fRatio + 0.5f*dY;
        p2[1] = (p2[1] - 0.5f*dY) * fRatio + 0.5f*dY;
    }

    int x1 = p1[0] * sp[0];
    int y1 = p1[1] * sp[1];
    int x2 = p2[0] * sp[0];
    int y2 = p2[1] * sp[1];

    //myPenWidth = 2*abs(y1-y2);

    if (event->button() == Qt::LeftButton) {
        lastPoint = event->pos();
        scribbling = true;
    }
}

void DrawingPlane::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && scribbling) {
        const QPoint& pos = event->pos();
        drawLineTo(pos);

        // filter out some points
        if (selection.isEmpty()) {
            selection << pos;
        }
        else {
            const QPoint& top = selection.last();
            if (abs(top.x()-pos.x()) > 20 ||
                abs(top.y()-pos.y()) > 20)
                selection << pos;
        }
    }
}

void DrawingPlane::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && scribbling) {
        drawLineTo(event->pos());
        scribbling = false;
        /*emit*/ emitSelection();
    }
}

bool DrawingPlane::eventFilter(QObject* o, QEvent* e)
{
    if (o == glView) {
        if (e->type() == QEvent::Resize)
            resizeEvent(static_cast<QResizeEvent*>(e));
        else if (e->type() == QEvent::MouseButtonPress)
            mousePressEvent(static_cast<QMouseEvent*>(e));
        else if (e->type() == QEvent::MouseButtonRelease)
            mouseReleaseEvent(static_cast<QMouseEvent*>(e));
        else if (e->type() == QEvent::MouseMove)
            mouseMoveEvent(static_cast<QMouseEvent*>(e));
    }

    return false;
}

void DrawingPlane::resizeEvent(QResizeEvent *event)
{
    QImage img(event->size(), QImage::Format_ARGB32);
    img.fill(qRgba(255, 255, 255, 0));
    image = img;
}

void DrawingPlane::drawLineTo(const QPoint &endPoint)
{
    QPainter painter(&image);
    painter.setPen(QPen(myPenColor, myPenWidth, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    painter.setOpacity(0.5);
    painter.drawLine(lastPoint.x(), image.height()-lastPoint.y(), endPoint.x(), image.height()-endPoint.y());

    QImage img = image;//QGLWidget::convertToGLFormat(image);
    int nc = img.numBytes() / ( img.width() * img.height() );
    data.setValue(SbVec2s(img.width(), img.height()), nc, img.bits());
    soqt->scheduleRedraw();
    lastPoint = endPoint;
}

// -------------------------------------------------

RemoveComponentsDialog::RemoveComponentsDialog(QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl)
{
    widget = new RemoveComponents(this);
    this->setWindowTitle(widget->windowTitle());

    QVBoxLayout* hboxLayout = new QVBoxLayout(this);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close|QDialogButtonBox::Ok);
    QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setText(MeshGui::TaskRemoveComponents::tr("Delete"));
    buttonBox->addButton(MeshGui::TaskRemoveComponents::tr("Invert"),
        QDialogButtonBox::ActionRole);
    
    connect(buttonBox, SIGNAL(clicked(QAbstractButton*)),
            this, SLOT(clicked(QAbstractButton*)));

    hboxLayout->addWidget(widget);
    hboxLayout->addWidget(buttonBox);
}

RemoveComponentsDialog::~RemoveComponentsDialog()
{
}

void RemoveComponentsDialog::reject()
{
    widget->reject();
    QDialog::reject();
}

void RemoveComponentsDialog::clicked(QAbstractButton* btn)
{
    QDialogButtonBox* buttonBox = qobject_cast<QDialogButtonBox*>(sender());
    QDialogButtonBox::StandardButton id = buttonBox->standardButton(btn);
    if (id == QDialogButtonBox::Ok) {
        widget->deleteSelection();
    }
    else if (id == QDialogButtonBox::Close) {
        this->reject();
    }
    else if (id == QDialogButtonBox::NoButton) {
        widget->invertSelection();
    }
}

// ---------------------------------------

/* TRANSLATOR MeshGui::TaskRemoveComponents */

TaskRemoveComponents::TaskRemoveComponents()
{
    widget = new RemoveComponents();
    taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), false, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskRemoveComponents::~TaskRemoveComponents()
{
    // automatically deleted in the sub-class
}

void TaskRemoveComponents::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btn = box->button(QDialogButtonBox::Ok);
    btn->setText(tr("Delete"));
    box->addButton(tr("Invert"), QDialogButtonBox::ActionRole);
}

bool TaskRemoveComponents::accept()
{
    return false;
}

void TaskRemoveComponents::clicked(int id)
{
    if (id == QDialogButtonBox::Ok) {
        widget->deleteSelection();
    }
    else if (id == QDialogButtonBox::Close) {
        widget->reject();
    }
    else if (id == QDialogButtonBox::NoButton) {
        widget->invertSelection();
    }
}

#include "moc_RemoveComponents.cpp"
