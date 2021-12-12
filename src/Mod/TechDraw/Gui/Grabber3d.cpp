/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#endif

//#include <QGuiApplication>

#include <QImage>
#include <QPixmap>
#include <QBitmap>
#include <QGraphicsView>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QGraphicsProxyWidget>
#include <QScreen>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/NavigationStyle.h>
#include <Gui/ViewProvider.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/SoFCVectorizeSVGAction.h>

#include <Inventor/SbBasic.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2d.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoRenderManager.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/annex/HardCopy/SoVectorizeAction.h>

#include "Rez.h"
#include "QGVPage.h"
#include "MDIViewPage.h"
#include "DrawGuiUtil.h"

#include "Grabber3d.h"

using namespace TechDrawGui;
using namespace TechDraw;
using namespace Gui;

    //notes for selection view
    //SoSeparator* newSG;
    //for obj in objList: 
    //    vProv = obj.ViewObject
    //    sg = vProv.getSG(obj);
    //    for child in sg:
    //       newSG->addChild();
    //

//creates Svg file and returns estimate of scale
double Grabber3d::copyActiveViewToSvgFile(App::Document* appDoc, 
                                        std::string fileSpec,
                                        double outWidth, double outHeight,
                                        bool paintBackground, const QColor& bgColor,
                                        double lineWidth, double border,
                                        int renderMode)
{
//    Base::Console().Message("G3d::copyActiveViewToSvgFile()\n");
    double result = 1.0;         //best estimate of scale of result

    //get the active view
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(appDoc);
    Gui::MDIView* mdiView = guiDoc->getActiveView();
    if (mdiView == nullptr) {
        Base::Console().Warning("G3d::copyActiveViewToSvgFile - no ActiveView - returning\n");
        return result;
    }

    View3DInventor* view3d = qobject_cast<View3DInventor*>(mdiView);
    if (view3d == nullptr) {
        Base::Console().Warning("G3d::copyActiveViewToSvgFile - no viewer for ActiveView - returning\n");
        return result;
    }

    View3DInventorViewer* viewer = view3d->getViewer();
    if (viewer == nullptr) {
        Base::Console().Warning("G3d::copyActiveViewToSvgFile - could not create viewer - returning\n");
        return result;
    }

    //save parameters of source view
    double mdiWidth = view3d->width();
    double mdiHigh  = view3d->height();

    SbViewportRegion sourceVPRegion = viewer->getSoRenderManager()->getViewportRegion();
    SoCamera* sourceCam     = viewer->getSoRenderManager()->getCamera();
    SbRotation sourceOrient = viewer->getCameraOrientation();
    SbVec3f sourcePos       = viewer->getSoRenderManager()->getCamera()->position.getValue();
    double sourceNear       = viewer->getSoRenderManager()->getCamera()->nearDistance.getValue();
    double sourceFar        = viewer->getSoRenderManager()->getCamera()->farDistance.getValue();
    double sourceFocal      = viewer->getSoRenderManager()->getCamera()->focalDistance.getValue();
    double sourceAspect     = viewer->getSoRenderManager()->getCamera()->aspectRatio.getValue();
    SoOrthographicCamera* oCam = nullptr;
    SoPerspectiveCamera*  pCam = nullptr;
    double sourceHeight = 0.0;
    double sourceAngle  = 45.0;
    if (sourceCam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        oCam = static_cast<SoOrthographicCamera*>(sourceCam);
        sourceHeight = oCam->height.getValue();
    } else if (sourceCam->getTypeId() == SoPerspectiveCamera::getClassTypeId()) { 
        pCam = static_cast<SoPerspectiveCamera*>(sourceCam);
        sourceAngle = pCam->heightAngle.getValue();
    }
    oCam = nullptr;
    pCam = nullptr;

    //make a view for rendering Svg
    View3DInventor* view3DI = new View3DInventor(0, 0);         //essentially an undisplayed mdi
//    view3DI->setWindowTitle(QString::fromUtf8("SvgRenderViewer"));  //fluff
//    Gui::getMainWindow()->addWindow(view3DI);                   //just for debugging.  comment for release.

    View3DInventorViewer* viewerSvg = view3DI->getViewer();
    viewerSvg->setBackgroundColor(QColor(Qt::white));

    SoRenderManager* renderMgr = viewerSvg->getSoRenderManager();
    renderMgr->setRenderMode(SoRenderManager::RenderMode(renderMode));

    SbViewportRegion targetVPRegion;
    targetVPRegion.setWindowSize(mdiWidth, mdiHigh);
    targetVPRegion.setPixelsPerInch(sourceVPRegion.getPixelsPerInch());
    renderMgr->setViewportRegion(targetVPRegion);

    auto sgOld         = viewer->getSceneGraph();
    SoSeparator* sgNew = copySceneGraph(sgOld);
    viewerSvg->setSceneGraph(sgNew);

    if (sourceCam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        viewerSvg->setCameraType(SoOrthographicCamera::getClassTypeId());
    } else if (sourceCam->getTypeId() == SoPerspectiveCamera::getClassTypeId()) { 
        viewerSvg->setCameraType(SoPerspectiveCamera::getClassTypeId());
    }

//    SoWriteAction writeAction;                //dump the IV  (debugging)
//    writeAction.apply(sgNew);

    //set Svg view params to match source
    SoCamera* svgCam = viewerSvg->getSoRenderManager()->getCamera();
    double zoomFactor = 1.0;                              //not used
    svgCam->orientation.setValue(sourceOrient);
    svgCam->position.setValue(sourcePos);
    svgCam->nearDistance.setValue(sourceNear);
    svgCam->farDistance.setValue(sourceFar);
    svgCam->focalDistance.setValue(sourceFocal);
    svgCam->aspectRatio.setValue(sourceAspect);
    if (svgCam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        SoOrthographicCamera* oSvgCam = static_cast<SoOrthographicCamera*>(svgCam);
        double newHeight = sourceHeight * zoomFactor;
        oSvgCam->height.setValue(newHeight);
    } else if (svgCam->getTypeId() == SoPerspectiveCamera::getClassTypeId()) { 
        SoPerspectiveCamera* vSvgCam = static_cast<SoPerspectiveCamera*>(svgCam);
        vSvgCam->heightAngle.setValue(sourceAngle);
    }

    viewerSvg->redraw();

    std::unique_ptr<SoVectorizeAction> va;
    va = std::unique_ptr<SoVectorizeAction>(new SoFCVectorizeSVGAction());

    SoVectorOutput* out = va->getOutput();
    if (!out || !out->openFile(fileSpec.c_str())) {
        Base::Console().Error("G3D::copyActiveViewToSvgFile - can not open file - %s/n", fileSpec.c_str());
        return result;
    }
    QColor dbColor(Qt::blue);
    execVectorizeAction(viewerSvg,
                        va.get(),
                        outWidth, outHeight,
                        paintBackground, bgColor,
                        lineWidth, border);

    out->closeFile();

    result = getViewerScale(viewerSvg);       //screen : world

    double pScale = getPaperScale(viewerSvg, outWidth, outHeight);

    //TODO: figure out net scaling world -> screen -> paper
    Base::Console().Log(
            "G3d::copyActiveViewToSvgFile - approx screen:world scale: 1:%.5f w/ort pixel size issues\n",
             result);
    Base::Console().Log(
            "G3d::copyActiveViewToSvgFile - approx paper/screen  scale: 1:%.5f w/ort pixel size issues\n",
            pScale);

//    postProcessSvg(fileSpec);

    view3DI->close();                             //comment out for debugging

    viewerSvg->setSceneGraph(nullptr);
    sgNew->unref();
    sgNew = nullptr;

    return result;
}

//==============================================================================

SoSeparator* Grabber3d::copySceneGraph(SoNode* sgIn)
{
//    Base::Console().Message("G3d::copySceneGraph()\n");
    SoSeparator* result = new SoSeparator();

    SoDirectionalLight* newLight = new SoDirectionalLight;
    result->addChild(newLight);

    SoNodeList* sgChildren = sgIn->getChildren();
    int childSize = sgChildren->getLength();

    //gather up the nodes to display
    for (int i=0; i < childSize; i++) {
        SoNode* c = (*sgChildren)[i];
        if (c->isOfType(SoGroup::getClassTypeId())) {
            auto cCopy = c->copy();
            result->addChild(cCopy);
        }
    }

    result->ref();
    return result;
}

//==============================================================================

//loosely based on View3DInventorViewer::saveGraphic
void Grabber3d::execVectorizeAction(Gui::View3DInventorViewer* viewer,
                               SoVectorizeAction* va,
                               double width, double height, 
                               bool paintBackground, const QColor& bgColor,
                               double lineWidth, double border)
{
//    Base::Console().Message("G3d::execVectorizeAction() - va: %X\n", va);
    if (va->getTypeId() == SoFCVectorizeSVGAction::getClassTypeId()) {
        SoFCVectorizeSVGAction* vaFC = static_cast<SoFCVectorizeSVGAction*>(va);
        vaFC->setBackgroundState(paintBackground);
        vaFC->setLineWidth(lineWidth);
        vaFC->setUseMM(true);
    }
    
    if (paintBackground && bgColor.isValid()) {
        va->setBackgroundColor(true, SbColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF()));
    } else {
        va->setBackgroundColor(false);
    }
    va->setOrientation(SoVectorizeAction::PORTRAIT);    //don't play with my w x h

    va->beginPage(SbVec2f(border, border), SbVec2f(width, height));
    va->beginViewport();
    va->calibrate(viewer->getSoRenderManager()->getViewportRegion());
    va->apply(viewer->getSoRenderManager()->getSceneGraph());
    va->endViewport();
    va->endPage();
}

//==============================================================================
//find scale factor screen:world
double Grabber3d::getViewerScale(Gui::View3DInventorViewer* viewer)
{
//    Base::Console().Message("G3d::getViewerScale()\n");

    double result = 1;
//    double printerpxmm  = 3.94;    //? 100 dpi?
    double coinpxmm     = 2.83;    //72 dpi

    //accurate dpmm for screen is not easily acquired!
//    double qtpxmm = 96;
//    QScreen *screen = QGuiApplication::primaryScreen();
//    double qtppi = screen->physicalDotsPerInch();  //~111 dpi
//    qtpxmm       = qtppi / 25.4; 

    SbViewportRegion vpRegion = viewer->getSoRenderManager()->getViewportRegion();
    SbVec2s winSizePx = vpRegion.getWindowSize();                //pixel coords

    Base::Vector3d p1v(0,0,0);
    Base::Vector3d p2v(winSizePx[0] - 1, winSizePx[1] - 1);
    double screenLengthpx = (p2v - p1v).Length();           //length in pixels
    double screenLengthmm = (screenLengthpx / coinpxmm);

    SbVec2s p1s(0,0);
    SbVec2s p2s(winSizePx[0] - 1, winSizePx[1] - 1);
    SbVec3f p1w = viewer->getPointOnFocalPlane(p1s);
    SbVec3f p2w = viewer->getPointOnFocalPlane(p2s);
    double  worldLengthmm = (p2w - p1w).length();         //mm

    result =  worldLengthmm / screenLengthmm;
    return result;
}
//==============================================================================

//find scale factor screen:"paper"
double Grabber3d::getPaperScale(Gui::View3DInventorViewer* viewer,
                                double pWidth, double pHeight)
{
//    Base::Console().Message("G3d::getPaperScale()\n");

    double result = 1;
//    double printerpxmm  = 3.94;    //? 100 dpi?
    double coinpxmm     = 2.83;    //72 dpi

    //accurate dpmm for screen is not easily acquired!
//    double qtpxmm = 96;
//    QScreen *screen = QGuiApplication::primaryScreen();
//    double qtppi = screen->physicalDotsPerInch();  //~111 dpi
//    qtpxmm       = qtppi / 25.4; 

    SbViewportRegion vpRegion = viewer->getSoRenderManager()->getViewportRegion();
    SbVec2s winSizePx = vpRegion.getWindowSize();                //pixel coords

    Base::Vector3d p1v(0,0,0);
    Base::Vector3d p2v(winSizePx[0] - 1, winSizePx[1] - 1);
    double screenLengthpx = (p2v - p1v).Length();           //length in pixels

//    double screenLengthmm = (screenLengthpx /  qtpxmm);
    double screenLengthmm = (screenLengthpx / coinpxmm);

    double paperLengthmm = sqrt( pow(pWidth, 2) + pow(pHeight, 2));

    result =  paperLengthmm / screenLengthmm;

    double paperLengthpx = sqrt( pow(pWidth*coinpxmm, 2) + pow(pHeight*coinpxmm, 2));
    double resultpx = paperLengthpx / screenLengthpx;
    Base::Console().Log("G3d::getPaperScale - screenLenpx: %.3f paperLenpx: %.3f resultpx: %.3f\n",
                            screenLengthpx, paperLengthpx, resultpx);

    Base::Console().Log("G3d::getPaperScale - screenLen: %.3f paperLen: %.3f result: %.3f\n",
                            screenLengthmm, paperLengthmm, result);
    return result;
}

//==============================================================================

void Grabber3d::postProcessSvg(std::string fileSpec)
{
    (void) fileSpec;
}
