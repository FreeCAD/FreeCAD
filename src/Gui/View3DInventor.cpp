/***************************************************************************
 *   Copyright (c) 2004 J�rgen Riegel <juergen.riegel@web.de>              *
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
# include <QAction>
# include <QApplication>
# include <QFileInfo>
# include <QKeyEvent>
# include <QEvent>
# include <QDropEvent>
# include <QDragEnterEvent>
# include <QFileDialog>
# include <QPainter>
# include <QPrinter>
# include <QPrintDialog>
# include <QPrintPreviewDialog>
# include <QStackedWidget>
# include <QTimer>
# include <QUrl>
# include <Inventor/actions/SoWriteAction.h>
# include <Inventor/actions/SoGetPrimitiveCountAction.h>
# include <Inventor/nodes/SoDirectionalLight.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/events/SoEvent.h>
# include <Inventor/fields/SoSFString.h>
# include <Inventor/fields/SoSFColor.h>
#endif
# include <QStackedWidget>

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>

#include <App/DocumentObject.h>

#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "Document.h"
#include "FileDialog.h"
#include "Application.h"
#include "MainWindow.h"
#include "MenuManager.h"
#include "WaitCursor.h"
#include "SoFCVectorizeSVGAction.h"

// build in Inventor
#include "Inventor/Qt/viewers/SoQtExaminerViewer.h"
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>

#include "View3DInventorExamples.h"
#include "SoFCSelectionAction.h"
#include "View3DPy.h"
#include "SoFCDB.h"
#include "NavigationStyle.h"

#include <locale>

using namespace Gui;

void GLOverlayWidget::paintEvent(QPaintEvent* ev)
{
    QPainter paint(this);
    paint.drawImage(0,0,image);
    paint.end();
}

/* TRANSLATOR Gui::View3DInventor */

TYPESYSTEM_SOURCE_ABSTRACT(Gui::View3DInventor,Gui::BaseView);

View3DInventor::View3DInventor(Gui::Document* pcDocument, QWidget* parent, Qt::WFlags wflags)
    : MDIView(pcDocument, parent, wflags), _viewerPy(0)
{
    stack = new QStackedWidget(this);
    // important for highlighting 
    setMouseTracking(true);
    // accept drops on the window, get handled in dropEvent, dragEnterEvent   
    setAcceptDrops(true);
  
    // attach parameter Observer
    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Attach(this);

    // create the inventor widget and set the defaults
#if !defined (NO_USE_QT_MDI_AREA)
    _viewer = new View3DInventorViewer(0);
    stack->addWidget(_viewer->getWidget());
    setCentralWidget(stack);
#else
    _viewer = new View3DInventorViewer(this);
#endif
    // apply the user settings
    OnChange(*hGrp,"EyeDistance");
    OnChange(*hGrp,"CornerCoordSystem");
    OnChange(*hGrp,"UseAutoRotation");
    OnChange(*hGrp,"Gradient");
    OnChange(*hGrp,"BackgroundColor");
    OnChange(*hGrp,"BackgroundColor2");
    OnChange(*hGrp,"BackgroundColor3");
    OnChange(*hGrp,"BackgroundColor4");
    OnChange(*hGrp,"UseBackgroundColorMid");
    OnChange(*hGrp,"UseAntialiasing");
    OnChange(*hGrp,"ShowFPS");
    OnChange(*hGrp,"Orthographic");
    OnChange(*hGrp,"HeadlightColor");
    OnChange(*hGrp,"HeadlightDirection");
    OnChange(*hGrp,"HeadlightIntensity");
    OnChange(*hGrp,"EnableBacklight");
    OnChange(*hGrp,"BacklightColor");
    OnChange(*hGrp,"BacklightDirection");
    OnChange(*hGrp,"BacklightIntensity");
    OnChange(*hGrp,"NavigationStyle");
    OnChange(*hGrp,"OrbitStyle");

    stopSpinTimer = new QTimer(this);
    connect(stopSpinTimer, SIGNAL(timeout()), this, SLOT(stopAnimating()));
}

View3DInventor::~View3DInventor()
{
    hGrp->Detach(this);

    //If we destroy this viewer by calling 'delete' directly the focus proxy widget which is defined 
    //by a widget in SoQtViewer isn't resetted. This widget becomes to a dangling pointer and makes
    //the application crash. (Probably it's better to destroy this viewer by calling close().)
    //See also Gui::Document::~Document().
    QWidget* foc = qApp->focusWidget();
    if (foc) {
        QWidget* par = foc->parentWidget();
        while (par) {
            if (par == this) {
                foc->setFocusProxy(0);
                foc->clearFocus();
                break;
            }
            par = par->parentWidget();
        }
    }

    if (_viewerPy) {
        static_cast<View3DInventorPy*>(_viewerPy)->_view = 0;
        Py_DECREF(_viewerPy);
    }

    // here is from time to time trouble!!!
    delete _viewer;
}

PyObject *View3DInventor::getPyObject(void)
{
    if (!_viewerPy)
        _viewerPy = new View3DInventorPy(this);

    Py_INCREF(_viewerPy);
    return _viewerPy;
}

void View3DInventor::OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason)
{
    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
    if (strcmp(Reason,"HeadlightColor") == 0) {
        unsigned long headlight = rGrp.GetUnsigned("HeadlightColor",ULONG_MAX); // default color (white)
        float transparency;
        SbColor headlightColor;
        headlightColor.setPackedValue((uint32_t)headlight, transparency);
        _viewer->getHeadlight()->color.setValue(headlightColor);
    }
    else if (strcmp(Reason,"HeadlightDirection") == 0) {
        std::string pos = rGrp.GetASCII("HeadlightDirection");
        QString flt = QString::fromAscii("([-+]?[0-9]+\\.?[0-9]+)");
        QRegExp rx(QString::fromAscii("^\\(%1,%1,%1\\)$").arg(flt));
        if (rx.indexIn(QLatin1String(pos.c_str())) > -1) {
            float x = rx.cap(1).toFloat();
            float y = rx.cap(2).toFloat();
            float z = rx.cap(3).toFloat();
            _viewer->getHeadlight()->direction.setValue(x,y,z);
        }
    }
    else if (strcmp(Reason,"HeadlightIntensity") == 0) {
        long value = rGrp.GetInt("HeadlightIntensity", 100);
        _viewer->getHeadlight()->intensity.setValue((float)value/100.0f);
    }
    else if (strcmp(Reason,"EnableBacklight") == 0) {
        _viewer->setBacklight(rGrp.GetBool("EnableBacklight", false));
    }
    else if (strcmp(Reason,"BacklightColor") == 0) {
        unsigned long backlight = rGrp.GetUnsigned("BacklightColor",ULONG_MAX); // default color (white)
        float transparency;
        SbColor backlightColor;
        backlightColor.setPackedValue((uint32_t)backlight, transparency);
        _viewer->getBacklight()->color.setValue(backlightColor);
    }
    else if (strcmp(Reason,"BacklightDirection") == 0) {
        std::string pos = rGrp.GetASCII("BacklightDirection");
        QString flt = QString::fromAscii("([-+]?[0-9]+\\.?[0-9]+)");
        QRegExp rx(QString::fromAscii("^\\(%1,%1,%1\\)$").arg(flt));
        if (rx.indexIn(QLatin1String(pos.c_str())) > -1) {
            float x = rx.cap(1).toFloat();
            float y = rx.cap(2).toFloat();
            float z = rx.cap(3).toFloat();
            _viewer->getBacklight()->direction.setValue(x,y,z);
        }
    }
    else if (strcmp(Reason,"BacklightIntensity") == 0) {
        long value = rGrp.GetInt("BacklightIntensity", 100);
        _viewer->getBacklight()->intensity.setValue((float)value/100.0f);
    }
    else if (strcmp(Reason,"EnablePreselection") == 0) {
        const ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
        SoFCEnableHighlightAction cAct(rclGrp.GetBool("EnablePreselection", true));
        cAct.apply(_viewer->getSceneGraph());
    }
    else if (strcmp(Reason,"EnableSelection") == 0) {
        const ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
        SoFCEnableSelectionAction cAct(rclGrp.GetBool("EnableSelection", true));
        cAct.apply(_viewer->getSceneGraph());
    }
    else if (strcmp(Reason,"HighlightColor") == 0) {
        float transparency;
        SbColor highlightColor(0.8f, 0.1f, 0.1f);
        unsigned long highlight = (unsigned long)(highlightColor.getPackedValue());
        highlight = rGrp.GetUnsigned("HighlightColor", highlight);
        highlightColor.setPackedValue((uint32_t)highlight, transparency);
        SoSFColor col; col.setValue(highlightColor);
        SoFCHighlightColorAction cAct(col);
        cAct.apply(_viewer->getSceneGraph());
    }
    else if (strcmp(Reason,"SelectionColor") == 0) {
        float transparency;
        SbColor selectionColor(0.1f, 0.8f, 0.1f);
        unsigned long selection = (unsigned long)(selectionColor.getPackedValue());
        selection = rGrp.GetUnsigned("SelectionColor", selection);
        selectionColor.setPackedValue((uint32_t)selection, transparency);
        SoSFColor col; col.setValue(selectionColor);
        SoFCSelectionColorAction cAct(col);
        cAct.apply(_viewer->getSceneGraph());
    }
    else if (strcmp(Reason,"NavigationStyle") == 0) {
        // check whether the simple or the full mouse model is used
        std::string model = rGrp.GetASCII("NavigationStyle",CADNavigationStyle::getClassTypeId().getName());
        Base::Type type = Base::Type::fromName(model.c_str());
        _viewer->setNavigationType(type);
    }
    else if (strcmp(Reason,"OrbitStyle") == 0) {
        int style = rGrp.GetInt("OrbitStyle",1);
        _viewer->navigationStyle()->setOrbitStyle(NavigationStyle::OrbitStyle(style));
    }
    else if (strcmp(Reason,"InvertZoom") == 0) {
        bool on = rGrp.GetBool("InvertZoom", false);
        _viewer->navigationStyle()->setZoomInverted(on);
    }
    else if (strcmp(Reason,"ZoomAtCursor") == 0) {
        bool on = rGrp.GetBool("ZoomAtCursor", false);
        _viewer->navigationStyle()->setZoomAtCursor(on);
    }
    else if (strcmp(Reason,"ZoomSetp") == 0) {
        float val = rGrp.GetFloat("ZoomSetp", 0.0f);
        _viewer->navigationStyle()->setZoomStep(val);
    }
    else if (strcmp(Reason,"EyeDistance") == 0) {
        _viewer->setStereoOffset(rGrp.GetFloat("EyeDistance",65.0));
    }
    else if (strcmp(Reason,"CornerCoordSystem") == 0) {
        _viewer->setFeedbackVisibility(rGrp.GetBool("CornerCoordSystem",true));
    }
    else if (strcmp(Reason,"UseAutoRotation") == 0) {
        _viewer->setAnimationEnabled(rGrp.GetBool("UseAutoRotation",true));
    }
    else if (strcmp(Reason,"Gradient") == 0) {
        _viewer->setGradientBackgroud((rGrp.GetBool("Gradient",true)));
    }
    else if (strcmp(Reason,"UseAntialiasing") == 0) {
        _viewer->getGLRenderAction()->setSmoothing(rGrp.GetBool("UseAntialiasing",false));
    }
    else if (strcmp(Reason,"ShowFPS") == 0) {
        _viewer->setEnabledFPSCounter(rGrp.GetBool("ShowFPS",false));
    }
    else if (strcmp(Reason,"Orthographic") == 0) {
        // check whether a perspective or orthogrphic camera should be set
        if (rGrp.GetBool("Orthographic", true))
            _viewer->setCameraType(SoOrthographicCamera::getClassTypeId());
        else
            _viewer->setCameraType(SoPerspectiveCamera::getClassTypeId());
    }
    else {
        unsigned long col1 = rGrp.GetUnsigned("BackgroundColor",3940932863UL);
        unsigned long col2 = rGrp.GetUnsigned("BackgroundColor2",859006463UL); // default color (dark blue)
        unsigned long col3 = rGrp.GetUnsigned("BackgroundColor3",2880160255UL); // default color (blue/grey)
        unsigned long col4 = rGrp.GetUnsigned("BackgroundColor4",1869583359UL); // default color (blue/grey)
        float r1,g1,b1,r2,g2,b2,r3,g3,b3,r4,g4,b4;
        r1 = ((col1 >> 24) & 0xff) / 255.0; g1 = ((col1 >> 16) & 0xff) / 255.0; b1 = ((col1 >> 8) & 0xff) / 255.0;
        r2 = ((col2 >> 24) & 0xff) / 255.0; g2 = ((col2 >> 16) & 0xff) / 255.0; b2 = ((col2 >> 8) & 0xff) / 255.0;
        r3 = ((col3 >> 24) & 0xff) / 255.0; g3 = ((col3 >> 16) & 0xff) / 255.0; b3 = ((col3 >> 8) & 0xff) / 255.0;
        r4 = ((col4 >> 24) & 0xff) / 255.0; g4 = ((col4 >> 16) & 0xff) / 255.0; b4 = ((col4 >> 8) & 0xff) / 255.0;
        _viewer->setBackgroundColor(SbColor(r1, g1, b1));
        if (rGrp.GetBool("UseBackgroundColorMid",false) == false)
            _viewer->setGradientBackgroudColor(SbColor(r2, g2, b2), SbColor(r3, g3, b3));
        else
            _viewer->setGradientBackgroudColor(SbColor(r2, g2, b2), SbColor(r3, g3, b3), SbColor(r4, g4, b4));
    }
}

void View3DInventor::onRename(Gui::Document *pDoc)
{
    SoSFString name;
    name.setValue(pDoc->getDocument()->getName());
    SoFCDocumentAction cAct(name);
    cAct.apply(_viewer->getSceneGraph());
}

void View3DInventor::onUpdate(void)
{
#ifdef FC_LOGUPDATECHAIN
    Base::Console().Log("Acti: Gui::View3DInventor::onUpdate()");
#endif
    update();  
    _viewer->render();
}

void View3DInventor::viewAll()
{
    _viewer->viewAll();
}

const char *View3DInventor::getName(void) const
{
    return "View3DInventor";
}

void View3DInventor::print()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setFullPage(true);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        Gui::WaitCursor wc;
        print(&printer);
    }
}

void View3DInventor::printPdf()
{
    QString filename = FileDialog::getSaveFileName(this, tr("Export PDF"), QString(), tr("PDF file (*.pdf)"));
    if (!filename.isEmpty()) {
        Gui::WaitCursor wc;
        QPrinter printer(QPrinter::ScreenResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        print(&printer);
    }
}

void View3DInventor::printPreview()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setFullPage(true);
    //printer.setPageSize(QPrinter::A3);
    printer.setOrientation(QPrinter::Landscape);

    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, SIGNAL(paintRequested (QPrinter *)),
            this, SLOT(print(QPrinter *)));
    dlg.exec();
}

void View3DInventor::print(QPrinter* printer)
{
    // The SVG output needs to be improved
#if 0
    SoFCVectorizeSVGAction action;
    SoSVGVectorOutput* out = action.getSVGOutput();
    std::string tmp = Base::FileInfo::getTempFileName();
    if (!out || !out->openFile(tmp.c_str()))
        return;
    SoVectorizeAction::PageSize ps;
    switch (printer->pageSize()) {
    case QPrinter::A0:
        ps = SoVectorizeAction::A0;
        break;
    case QPrinter::A1:
        ps = SoVectorizeAction::A1;
        break;
    case QPrinter::A2:
        ps = SoVectorizeAction::A2;
        break;
    case QPrinter::A3:
        ps = SoVectorizeAction::A3;
        break;
    case QPrinter::A4:
        ps = SoVectorizeAction::A4;
        break;
    case QPrinter::A5:
        ps = SoVectorizeAction::A5;
        break;
    case QPrinter::A6:
        ps = SoVectorizeAction::A6;
        break;
    case QPrinter::A7:
        ps = SoVectorizeAction::A7;
        break;
    case QPrinter::A8:
        ps = SoVectorizeAction::A8;
        break;
    case QPrinter::A9:
        ps = SoVectorizeAction::A9;
        break;
    default:
        ps = SoVectorizeAction::A4;
        break;
    }
    _viewer->saveGraphic(ps,View3DInventorViewer::White,&action);
    out->closeFile();
    QSvgRenderer svg;
    if (svg.load(QString::fromUtf8(tmp.c_str()))) {
        QPainter p(printer);
        svg.render(&p);
        p.end();
    }
#else
    QImage img;
    QPainter p(printer);
    QRect rect = printer->pageRect();
    _viewer->savePicture(rect.width(), rect.height(), View3DInventorViewer::White, img);
    p.drawImage(0,0,img);
    p.end();
#endif
}

// **********************************************************************************

bool View3DInventor::onMsg(const char* pMsg, const char** ppReturn)
{
    if (strcmp("ViewFit",pMsg) == 0) {
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewSelection",pMsg) == 0) {
        _viewer->viewSelection();
        return true;
    // comment out on older Inventor
#if SOQT_MAJOR_VERSION > 1 || (SOQT_MAJOR_VERSION == 1 && SOQT_MINOR_VERSION >= 2)
    }
    else if(strcmp("SetStereoRedGreen",pMsg) == 0 ) {
        _viewer->setStereoType(SoQtViewer::STEREO_ANAGLYPH);
        return true;
    }
    else if(strcmp("SetStereoQuadBuff",pMsg) == 0 ) {
        _viewer->setStereoType(SoQtViewer::STEREO_QUADBUFFER );
        return true;
    }
    else if(strcmp("SetStereoInterleavedRows",pMsg) == 0 ) {
        _viewer->setStereoType(SoQtViewer::STEREO_INTERLEAVED_ROWS );
        return true;
    }
    else if(strcmp("SetStereoInterleavedColumns",pMsg) == 0 ) {
        _viewer->setStereoType(SoQtViewer::STEREO_INTERLEAVED_COLUMNS  );
        return true;
    }
    else if(strcmp("SetStereoOff",pMsg) == 0 ) {
        _viewer->setStereoType(SoQtViewer::STEREO_NONE );
        return true;
#else
    }
    else if(strcmp("SetStereoRedGreen",pMsg) == 0 ) {
        Base::Console().Warning("Use SoQt 1.2.x or later!\n");
        return true;
    }
    else if(strcmp("SetStereoQuadBuff",pMsg) == 0 ) {
        Base::Console().Warning("Use SoQt 1.2.x or later!\n");
        return true;
    }
    else if(strcmp("SetStereoInterleavedRows",pMsg) == 0 ) {
        Base::Console().Warning("Use SoQt 1.2.x or later!\n");
        return true;
    }
    else if(strcmp("SetStereoInterleavedColumns",pMsg) == 0 ) {
        Base::Console().Warning("Use SoQt 1.2.x or later!\n");
        return true;
    }
    else if(strcmp("SetStereoOff",pMsg) == 0 ) {
        Base::Console().Warning("Use SoQt 1.2.x or later!\n");
        return true;
#endif
    }
    else if(strcmp("Example1",pMsg) == 0 ) {
        SoSeparator * root = new SoSeparator;
        Texture3D(root);
        _viewer->setSceneGraph(root);
        return true;
    }
    else if(strcmp("Example2",pMsg) == 0 ) {
        SoSeparator * root = new SoSeparator;
        LightManip(root);
        _viewer->setSceneGraph(root);
        return true;
    }
    else if(strcmp("Example3",pMsg) == 0 ) {
        SoSeparator * root = new SoSeparator;
        AnimationTexture(root);
        _viewer->setSceneGraph(root);
        return true;
    }
    else if(strcmp("GetCamera",pMsg) == 0 ) {
        SoCamera * Cam = _viewer->getCamera();
        if (!Cam) return false;
        *ppReturn = SoFCDB::writeNodesToString(Cam).c_str();
        return true;
    }
    else if(strncmp("SetCamera",pMsg,9) == 0 ) {
        return setCamera(pMsg+10);
    }
    else if(strncmp("Dump",pMsg,4) == 0 ) {
        dump(pMsg+5);
        return true;
    }
    else if(strcmp("ViewBottom",pMsg) == 0 ) {
        _viewer->setCameraOrientation(SbRotation(-1, 0, 0, 0));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewFront",pMsg) == 0 ) {
        float root = (float)(sqrt(2.0)/2.0);
        _viewer->setCameraOrientation(SbRotation(-root, 0, 0, -root));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewLeft",pMsg) == 0 ) {
        _viewer->setCameraOrientation(SbRotation(-0.5, 0.5, 0.5, -0.5));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewRear",pMsg) == 0 ) {
        float root = (float)(sqrt(2.0)/2.0);
        _viewer->setCameraOrientation(SbRotation(0, root, root, 0));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewRight",pMsg) == 0 ) {
        _viewer->setCameraOrientation(SbRotation(0.5, 0.5, 0.5, 0.5));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewTop",pMsg) == 0 ) {
        _viewer->setCameraOrientation(SbRotation(0, 0, 0, 1));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewAxo",pMsg) == 0 ) {
        _viewer->setCameraOrientation(SbRotation
            (-0.353553f, -0.146447f, -0.353553f, -0.853553f));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("OrthographicCamera",pMsg) == 0 ) {
        _viewer->setCameraType(SoOrthographicCamera::getClassTypeId());
        return true;
    }
    else if(strcmp("PerspectiveCamera",pMsg) == 0 ) {
        _viewer->setCameraType(SoPerspectiveCamera::getClassTypeId());
        return true;
    }
    else  if(strcmp("Undo",pMsg) == 0 ) {
        getGuiDocument()->undo(1);
        return true;
    }
    else  if(strcmp("Redo",pMsg) == 0 ) {
        getGuiDocument()->redo(1);
        return true;
    }
    else if (strcmp("Save",pMsg) == 0) {
        getGuiDocument()->save();
        return true;
    }
    else if (strcmp("SaveAs",pMsg) == 0) {
        getGuiDocument()->saveAs();
        return true;
    }
    else
        return false;
}

bool View3DInventor::onHasMsg(const char* pMsg) const
{
    if  (strcmp("Save",pMsg) == 0)
        return true;
    else if (strcmp("SaveAs",pMsg) == 0)
        return true;
    else if (strcmp("Undo",pMsg) == 0)
        return getAppDocument()->getAvailableUndos() > 0;
    else if (strcmp("Redo",pMsg) == 0)
        return getAppDocument()->getAvailableRedos() > 0; 
    else if (strcmp("Print",pMsg) == 0)
        return true; 
    else if (strcmp("PrintPreview",pMsg) == 0)
        return true; 
    else if (strcmp("PrintPdf",pMsg) == 0)
        return true; 
    else if(strcmp("SetStereoRedGreen",pMsg) == 0)
        return true;
    else if(strcmp("SetStereoQuadBuff",pMsg) == 0)
        return true;
    else if(strcmp("SetStereoInterleavedRows",pMsg) == 0)
        return true;
    else if(strcmp("SetStereoInterleavedColumns",pMsg) == 0)
        return true;
    else if(strcmp("SetStereoOff",pMsg) == 0)
        return true;
    else if(strcmp("Example1",pMsg) == 0)
        return true;
    else if(strcmp("Example2",pMsg) == 0)
        return true;
    else if(strcmp("Example3",pMsg) == 0)
        return true;
    else if(strcmp("ViewFit",pMsg) == 0)
        return true;
    else if(strcmp("ViewSelection",pMsg) == 0)
        return true;
    else if(strcmp("ViewBottom",pMsg) == 0)
        return true;
    else if(strcmp("ViewFront",pMsg) == 0)
        return true;
    else if(strcmp("ViewLeft",pMsg) == 0)
        return true;
    else if(strcmp("ViewRear",pMsg) == 0)
        return true;
    else if(strcmp("ViewRight",pMsg) == 0)
        return true;
    else if(strcmp("ViewTop",pMsg) == 0)
        return true;
    else if(strcmp("ViewAxo",pMsg) == 0)
        return true;
    else if(strcmp("GetCamera",pMsg) == 0)
        return true;
    else if(strncmp("SetCamera",pMsg,9) == 0)
        return true;
    else if(strncmp("Dump",pMsg,4) == 0)
        return true;
    return false;
}

bool View3DInventor::setCamera(const char* pCamera)
{
    SoCamera * CamViewer = _viewer->getCamera();
    if (!CamViewer) {
        throw Base::Exception("No camera set so far...");
    }

    SoInput in;
    in.setBuffer((void*)pCamera,std::strlen(pCamera));

    SoNode * Cam;
    SoDB::read(&in,Cam);

    if (!Cam){
        throw Base::Exception("Camera settings failed to read");
    }

    // toggle between persepective and orthographic camera
    if (Cam->getTypeId() != CamViewer->getTypeId())
    {
        _viewer->setCameraType(Cam->getTypeId());
        CamViewer = _viewer->getCamera();
    }

    SoPerspectiveCamera  * CamViewerP = 0;
    SoOrthographicCamera * CamViewerO = 0;

    if (CamViewer->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        CamViewerP = (SoPerspectiveCamera *)CamViewer;  // safe downward cast, knows the type
    } else if (CamViewer->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        CamViewerO = (SoOrthographicCamera *)CamViewer;  // safe downward cast, knows the type
    }

    if (Cam->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        if (CamViewerP){
            CamViewerP->position      = ((SoPerspectiveCamera *)Cam)->position;
            CamViewerP->orientation   = ((SoPerspectiveCamera *)Cam)->orientation;
            CamViewerP->nearDistance  = ((SoPerspectiveCamera *)Cam)->nearDistance;
            CamViewerP->farDistance   = ((SoPerspectiveCamera *)Cam)->farDistance;
            CamViewerP->focalDistance = ((SoPerspectiveCamera *)Cam)->focalDistance;
        } else {
            throw Base::Exception("Camera type mismatch");
        }
    } else if (Cam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        if (CamViewerO){
            CamViewerO->viewportMapping  = ((SoOrthographicCamera *)Cam)->viewportMapping;
            CamViewerO->position         = ((SoOrthographicCamera *)Cam)->position;
            CamViewerO->orientation      = ((SoOrthographicCamera *)Cam)->orientation;
            CamViewerO->nearDistance     = ((SoOrthographicCamera *)Cam)->nearDistance;
            CamViewerO->farDistance      = ((SoOrthographicCamera *)Cam)->farDistance;
            CamViewerO->focalDistance    = ((SoOrthographicCamera *)Cam)->focalDistance;
            CamViewerO->aspectRatio      = ((SoOrthographicCamera *)Cam)->aspectRatio ;
            CamViewerO->height           = ((SoOrthographicCamera *)Cam)->height;
        } else {
            throw Base::Exception("Camera type mismatch");
        }
    }

    return true;
}

void View3DInventor::toggleClippingPlane()
{
    _viewer->toggleClippingPlane();
}

bool View3DInventor::hasClippingPlane() const
{
    return _viewer->hasClippingPlane();
}

void View3DInventor::setOverlayWidget(GLOverlayWidget* widget)
{
    removeOverlayWidget();
    QGLWidget* w = static_cast<QGLWidget*>(_viewer->getGLWidget());
    QImage img = w->grabFrameBuffer();
    widget->setImage(img);
    stack->addWidget(widget);
    stack->setCurrentIndex(1);
}

void View3DInventor::removeOverlayWidget()
{
    stack->setCurrentIndex(0);
    QWidget* overlay = stack->widget(1);
    if (overlay) stack->removeWidget(overlay);
}

void View3DInventor::setCursor(const QCursor& aCursor)
{
    _viewer->getWidget()->setCursor(aCursor);
}

void View3DInventor::setCursor(Qt::CursorShape aCursor)
{
    _viewer->getWidget()->setCursor(aCursor);
}

void View3DInventor::dump(const char* filename)
{
    SoGetPrimitiveCountAction action;
    action.setCanApproximate(true);
    action.apply(_viewer->getSceneGraph());

    if ( action.getTriangleCount() > 100000 || action.getPointCount() > 30000 || action.getLineCount() > 10000 )
        _viewer->dumpToFile(filename,true);
    else
        _viewer->dumpToFile(filename,false);
}

void View3DInventor::windowStateChanged(MDIView* view)
{
    bool canStartTimer = false;
    if (this != view) {
        // If both views are child widgets of the workspace and view is maximized this view 
        // must be hidden, hence we can start the timer.
        // Note: If view is top-level or fullscreen it doesn't necessarily hide the other view
        // e.g. if it is on a second monitor.
        canStartTimer = (!this->isTopLevel() && !view->isTopLevel() && view->isMaximized());
    } else if (isMinimized()) {
        // I am the active view but minimized
        canStartTimer = true;
    }

    if (canStartTimer) {
        // do a single shot event (maybe insert a checkbox in viewer settings)
        int msecs = hGrp->GetInt("stopAnimatingIfDeactivated", 3000);
        if (!stopSpinTimer->isActive() && msecs >= 0) { // if < 0 do not stop rotation
            stopSpinTimer->setSingleShot(true);
            stopSpinTimer->start(msecs);
        }
    } else if (stopSpinTimer->isActive()) {
        // If this view may be visible again we can stop the timer
        stopSpinTimer->stop();
    }
}

void View3DInventor::stopAnimating()
{
    _viewer->stopAnimating();
}

/**
 * Drops the event \a e and writes the right Python command.
 */
void View3DInventor::dropEvent (QDropEvent * e)
{
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        getMainWindow()->loadUrls(getAppDocument(), data->urls());
    }
    else {
        MDIView::dropEvent(e);
    }
}

void View3DInventor::dragEnterEvent (QDragEnterEvent * e)
{
    // Here we must allow uri drafs and check them in dropEvent
    const QMimeData* data = e->mimeData();
    if (data->hasUrls())
        e->accept();
    else
        e->ignore();
}

void View3DInventor::setCurrentViewMode(ViewMode newmode)
{
    ViewMode oldmode = MDIView::currentViewMode();
    if (oldmode == newmode)
        return;
    MDIView::setCurrentViewMode(newmode);

    // This widget becomes the focus proxy of the embedded GL widget if we leave 
    // the 'Child' mode. If we reenter 'Child' mode the focus proxy is reset to 0.
    // If we change from 'TopLevel' mode to 'Fullscreen' mode or vice versa nothing
    // happens.
    // Grabbing keyboard when leaving 'Child' mode (as done in a recent version) should
    // be avoided because when two or more windows are either in 'TopLevel' or 'Fullscreen'
    // mode only the last window gets all key event even if it is not the active one.
    //
    // It is important to set the focus proxy to get all key events otherwise we would loose
    // control after redirecting the first key event to the GL widget.
    // We redirect these events in keyPressEvent() and keyReleaseEvent().
    if (oldmode == Child) {
        // To make a global shortcut working from this window we need to add
        // all existing actions from the mainwindow and its sub-widgets 
        QList<QAction*> acts = getMainWindow()->findChildren<QAction*>();
        this->addActions(acts);
        _viewer->getGLWidget()->setFocusProxy(this);
        // To be notfified for new actions
        qApp->installEventFilter(this);
    }
    else if (newmode == Child) {
        _viewer->getGLWidget()->setFocusProxy(0);
        qApp->removeEventFilter(this);
        QList<QAction*> acts = this->actions();
        for (QList<QAction*>::Iterator it = acts.begin(); it != acts.end(); ++it)
            this->removeAction(*it);
    }
}

bool View3DInventor::eventFilter(QObject* watched, QEvent* e)
{
    // As long as this widget is a top-level window (either in 'TopLevel' or 'FullScreen' mode) we
    // need to be notified when an action is added to a widget. This action must also be added to 
    // this window to allow to make use of its shortcut (if defined).
    // Note: We don't need to care about removing an action if its parent widget gets destroyed.
    // This does the action itself for us.
    if (watched != this && e->type() == QEvent::ActionAdded) {
        QActionEvent* a = static_cast<QActionEvent*>(e);
        QAction* action = a->action();

        if (!action->isSeparator()) {
            QList<QAction*> actions = this->actions();
            if (!actions.contains(action))
                this->addAction(action);
        }
    }

    return false;
}

void View3DInventor::keyPressEvent (QKeyEvent* e)
{
    ViewMode mode = MDIView::currentViewMode();
    if (mode != Child) {
        // If the widget is in fullscreen mode then we can return to normal mode either
        // by pressing the matching accelerator or ESC. 
        if (e->key() == Qt::Key_Escape) {
            setCurrentViewMode(Child);
        }
        else {
            // Note: The key events should be redirected directly to the GL widget and not to the main window
            // otherwise the first redirected key event always disappears in hyperspace.
            //
            // send the event to the GL widget that converts to and handles an SoEvent
            QWidget* w = _viewer->getGLWidget();
            QApplication::sendEvent(w,e);
        }
    }
    else {
        QMainWindow::keyPressEvent(e);
    }
}

void View3DInventor::keyReleaseEvent (QKeyEvent* e)
{
    ViewMode mode = MDIView::currentViewMode();
    if (mode != Child) {
        // send the event to the GL widget that converts to and handles an SoEvent
        QWidget* w = _viewer->getGLWidget();
        QApplication::sendEvent(w,e);
    } else {
        QMainWindow::keyReleaseEvent(e);
    }
}

void View3DInventor::focusInEvent (QFocusEvent * e)
{
    _viewer->getGLWidget()->setFocus();
}

void View3DInventor::contextMenuEvent (QContextMenuEvent*e)
{
    MDIView::contextMenuEvent(e);
}

void View3DInventor::customEvent(QEvent * e)
{
    if (e->type() == QEvent::User) {
        NavigationStyleEvent* se = static_cast<NavigationStyleEvent*>(e);
        _viewer->setNavigationType(se->style());
    }
}


#include "moc_View3DInventor.cpp"
