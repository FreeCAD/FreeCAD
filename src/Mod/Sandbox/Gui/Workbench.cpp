/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
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
# include <QGridLayout>
# include <QPainter>
# include <Inventor/actions/SoAction.h>
# include <Inventor/elements/SoModelMatrixElement.h>
# include <Inventor/elements/SoViewVolumeElement.h>
# include <Inventor/elements/SoViewportRegionElement.h>
# include <Inventor/SoPrimitiveVertex.h>
# include <Inventor/SbLinear.h>
#endif

#include <QtOpenGL.h>

#include "Workbench.h"
#include <App/Application.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Gui/MainWindow.h>
#include <Gui/DockWindow.h>
#include <Gui/DockWindowManager.h>
#include <Gui/TreeView.h>

using namespace SandboxGui;

/// @namespace SandboxGui @class Workbench
TYPESYSTEM_SOURCE(SandboxGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
    // Tree view
    Gui::DockWindow* tree = new Gui::DockWindow(0, Gui::getMainWindow());
    tree->setWindowTitle(QString::fromLatin1("Tree view"));
    Gui::TreeView* treeWidget = new Gui::TreeView(tree);
    treeWidget->setRootIsDecorated(false);
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    treeWidget->setIndentation(hGrp->GetInt("Indentation", treeWidget->indentation()));

    QGridLayout* pLayout = new QGridLayout(tree); 
    pLayout->setSpacing(0);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(treeWidget, 0, 0);

    tree->setObjectName
        (QString::fromLatin1(QT_TRANSLATE_NOOP("QDockWidget","Tree view (MVC)")));
    tree->setMinimumWidth(210);
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    pDockMgr->registerDockWindow("Std_TreeViewMVC", tree);
}

Workbench::~Workbench()
{
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");
    Gui::MenuItem* test = new Gui::MenuItem;
    root->insertItem(item, test);
    Gui::MenuItem* threads = new Gui::MenuItem;
    threads->setCommand("Python Threads");
    *threads << "Sandbox_PythonLockThread" << "Sandbox_NolockPython"
             << "Sandbox_PySideThread" << "Sandbox_PythonThread" << "Sandbox_PythonMainThread";
    test->setCommand("Threads");
    *test << "Sandbox_Thread" << "Sandbox_TestThread" << "Sandbox_SaveThread"
          << "Sandbox_WorkerThread" << "Sandbox_SeqThread"
          << "Sandbox_BlockThread" << "Sandbox_NoThread" << threads << "Separator"
          << "Sandbox_Dialog" << "Sandbox_FileDialog";
    Gui::MenuItem* misc = new Gui::MenuItem;
    root->insertItem(item, misc);
    misc->setCommand("Misc");
    *misc << "Sandbox_EventLoop" << "Sandbox_MeshLoad"
          << "Sandbox_MeshLoaderBoost"
          << "Sandbox_MeshLoaderFuture"
          << "Sandbox_MeshTestJob"
          << "Sandbox_MeshTestRef"
          << "Sandbox_CryptographicHash"
          << "Sandbox_MengerSponge";

    Gui::MenuItem* widg = new Gui::MenuItem;
    root->insertItem(item, widg);
    widg->setCommand("Widgets");
    *widg << "Std_GrabWidget"
          << "Std_ImageNode"
          << "Sandbox_WidgetShape"
          << "Sandbox_GDIWidget"
          << "Sandbox_RedirectPaint"
          << "Std_TestGraphicsView"
          << "Std_TestTaskBox";

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* test = new Gui::ToolBarItem(root);
    test->setCommand( "Sandbox Tools" );
    *test << "Sandbox_Thread" << "Sandbox_WorkerThread" << "Sandbox_SeqThread"
          << "Sandbox_BlockThread" << "Sandbox_NoThread"
          << "Sandbox_Dialog" << "Sandbox_FileDialog"; 
    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    return 0;
}

Gui::DockWindowItems* Workbench::setupDockWindows() const
{
    Gui::DockWindowItems* root = Gui::StdWorkbench::setupDockWindows();
    root->setVisibility(false); // hide all dock windows by default
    root->addDockWidget("Std_TreeViewMVC", Qt::RightDockWidgetArea, true, true);
    return root;
}

// ----------------------------------------------------


SO_NODE_SOURCE(SoWidgetShape)

void SoWidgetShape::initClass()
{
    SO_NODE_INIT_CLASS(SoWidgetShape, SoShape, "Shape");
}

SoWidgetShape::SoWidgetShape()
{
    SO_NODE_CONSTRUCTOR(SoWidgetShape);
}

void SoWidgetShape::GLRender(SoGLRenderAction * /*action*/)
{
#if defined(HAVE_QT5_OPENGL)
    this->image = QPixmap::grabWidget(w, w->rect()).toImage();
#else
    this->image = w->grab(w->rect()).toImage();
#endif
    glRasterPos2d(10,10);
    glDrawPixels(this->image.width(),this->image.height(),GL_RGBA,GL_UNSIGNED_BYTE,this->image.bits());
}

void SoWidgetShape::computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center)
{
    // ignore if node is empty
    if (this->image.isNull())
        return;

    SbVec3f v0, v1, v2, v3;
    // this will cause a cache dependency on the view volume,
    // model matrix and viewport.
    this->getQuad(action->getState(), v0, v1, v2, v3);

    box.makeEmpty();
    box.extendBy(v0);
    box.extendBy(v1);
    box.extendBy(v2);
    box.extendBy(v3);
    center = box.getCenter();
}

// Calculates the quad in 3D.
void
SoWidgetShape::getQuad(SoState * state, SbVec3f & v0, SbVec3f & v1,
                       SbVec3f & v2, SbVec3f & v3)
{
    SbVec3f nilpoint(0.0f, 0.0f, 0.0f);
    const SbMatrix & mat = SoModelMatrixElement::get(state);
    mat.multVecMatrix(nilpoint, nilpoint);

    const SbViewVolume &vv = SoViewVolumeElement::get(state);

    SbVec3f screenpoint;
    vv.projectToScreen(nilpoint, screenpoint);

    const SbViewportRegion & vp = SoViewportRegionElement::get(state);
    SbVec2s vpsize = vp.getViewportSizePixels();

    // find normalized width and height of image
    float nw = (float)this->image.width();
    nw /= (float)vpsize[0];
    float nh = (float)this->image.height();
    nh /= (float)vpsize[1];

    // need only half the width
    nw *= 0.5f;
    nh *= 0.5f;

    SbVec2f n0, n1, n2, n3;

    n0 = SbVec2f(screenpoint[0]-nw, screenpoint[1]-nh);
    n1 = SbVec2f(screenpoint[0]+nw, screenpoint[1]-nh);
    n2 = SbVec2f(screenpoint[0]+nw, screenpoint[1]+nh);
    n3 = SbVec2f(screenpoint[0]-nw, screenpoint[1]+nh);

    // get distance from nilpoint to camera plane
    float dist = -vv.getPlane(0.0f).getDistance(nilpoint);

    // find the four image points in the plane
    v0 = vv.getPlanePoint(dist, n0);
    v1 = vv.getPlanePoint(dist, n1);
    v2 = vv.getPlanePoint(dist, n2);
    v3 = vv.getPlanePoint(dist, n3);

    // transform back to object space
    SbMatrix inv = mat.inverse();
    inv.multVecMatrix(v0, v0);
    inv.multVecMatrix(v1, v1);
    inv.multVecMatrix(v2, v2);
    inv.multVecMatrix(v3, v3);
}

void SoWidgetShape::generatePrimitives(SoAction *action)
{
    if (this->image.isNull())
        return;

    SoState *state = action->getState();
    state->push();

    SbVec2s size;
    SbVec3f v0, v1, v2, v3;
    this->getQuad(action->getState(), v0, v1, v2, v3);

    SbVec3f n = (v1-v0).cross(v2-v0);
    n.normalize();

    this->beginShape(action, SoShape::QUADS);
    SoPrimitiveVertex vertex;
    vertex.setNormal(n);

    vertex.setTextureCoords(SbVec2f(0,0));
    vertex.setPoint(v0);
    this->shapeVertex(&vertex);

    vertex.setTextureCoords(SbVec2f(1,0));
    vertex.setPoint(v1);
    this->shapeVertex(&vertex);

    vertex.setTextureCoords(SbVec2f(1,1));
    vertex.setPoint(v2);
    this->shapeVertex(&vertex);

    vertex.setTextureCoords(SbVec2f(0,1));
    vertex.setPoint(v3);
    this->shapeVertex(&vertex);

    this->endShape();

    state->pop();
}

void SoWidgetShape::setWidget(QWidget* w)
{
    this->w = w;
    this->w->show();
    QPixmap img(this->w->size());
    this->w->render(&img);
    this->image = img.toImage();

#if !defined(HAVE_QT5_OPENGL)
    this->image = w->grab(w->rect()).toImage();
#endif
}
