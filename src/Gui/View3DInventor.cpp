/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <string>
# include <QAction>
# include <QApplication>
# include <QKeyEvent>
# include <QEvent>
# include <QDropEvent>
# include <QDragEnterEvent>
# include <QLayout>
# include <QMdiSubWindow>
# include <QMessageBox>
# include <QMimeData>
# include <QPainter>
# include <QPrinter>
# include <QPrintDialog>
# include <QPrintPreviewDialog>
# include <QStackedWidget>
# include <QTimer>
# include <QUrl>
# include <QWindow>
# include <Inventor/actions/SoGetPrimitiveCountAction.h>
# include <Inventor/fields/SoSFString.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <App/Document.h>
#include <Base/Builder3D.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "View3DInventor.h"
#include "View3DSettings.h"
#include "Application.h"
#include "Camera.h"
#include "Document.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "NaviCube.h"
#include "NavigationStyle.h"
#include "SoFCDB.h"
#include "SoFCSelectionAction.h"
#include "SoFCVectorizeSVGAction.h"
#include "View3DInventorExamples.h"
#include "View3DInventorViewer.h"
#include "View3DPy.h"
#include "ViewProvider.h"
#include "WaitCursor.h"


using namespace Gui;

void GLOverlayWidget::paintEvent(QPaintEvent*)
{
    QPainter paint(this);
    paint.drawImage(0,0,image);
    paint.end();
}

/* TRANSLATOR Gui::View3DInventor */

TYPESYSTEM_SOURCE_ABSTRACT(Gui::View3DInventor,Gui::MDIView)

View3DInventor::View3DInventor(Gui::Document* pcDocument, QWidget* parent,
                               const QtGLWidget* sharewidget, Qt::WindowFlags wflags)
    : MDIView(pcDocument, parent, wflags), _viewerPy(nullptr)
{
    stack = new QStackedWidget(this);
    // important for highlighting
    setMouseTracking(true);
    // accept drops on the window, get handled in dropEvent, dragEnterEvent
    setAcceptDrops(true);

    //anti-aliasing settings
    bool smoothing = false;
    bool glformat = false;
    int samples = View3DInventorViewer::getNumSamples();
    QtGLFormat f;

    if (samples > 1) {
        glformat = true;
        f.setSamples(samples);
    }
    else if (samples > 0) {
        smoothing = true;
    }

    if (glformat)
        _viewer = new View3DInventorViewer(f, this, sharewidget);
    else
        _viewer = new View3DInventorViewer(this, sharewidget);

    if (smoothing)
        _viewer->getSoRenderManager()->getGLRenderAction()->setSmoothing(true);

    // create the inventor widget and set the defaults
    _viewer->setDocument(this->_pcDocument);
    stack->addWidget(_viewer->getWidget());
    // http://forum.freecad.org/viewtopic.php?f=3&t=6055&sid=150ed90cbefba50f1e2ad4b4e6684eba
    // describes a minor error but trying to fix it leads to a major issue
    // http://forum.freecad.org/viewtopic.php?f=3&t=6085&sid=3f4bcab8007b96aaf31928b564190fd7
    // so the change is commented out
    // By default, the wheel events are processed by the 3d view AND the mdi area.
    //_viewer->getGLWidget()->setAttribute(Qt::WA_NoMousePropagation);
    setCentralWidget(stack);

    // apply the user settings
    applySettings();

    stopSpinTimer = new QTimer(this);
    connect(stopSpinTimer, &QTimer::timeout, this, &View3DInventor::stopAnimating);
}

View3DInventor::~View3DInventor()
{
    if(_pcDocument) {
        SoCamera * Cam = _viewer->getSoRenderManager()->getCamera();
        if (Cam)
            _pcDocument->saveCameraSettings(SoFCDB::writeNodesToString(Cam).c_str());
    }

    viewSettings.reset();

    //If we destroy this viewer by calling 'delete' directly the focus proxy widget which is defined
    //by a widget in SoQtViewer isn't reset. This widget becomes a dangling pointer and makes
    //the application crash. (Probably it's better to destroy this viewer by calling close().)
    //See also Gui::Document::~Document().
    QWidget* foc = qApp->focusWidget();
    if (foc) {
        QWidget* par = foc->parentWidget();
        while (par) {
            if (par == this) {
                foc->setFocusProxy(nullptr);
                foc->clearFocus();
                break;
            }
            par = par->parentWidget();
        }
    }

    if (_viewerPy) {
        Base::PyGILStateLocker lock;
        Py_DECREF(_viewerPy);
    }

    // here is from time to time trouble!!!
    delete _viewer;
}

void View3DInventor::deleteSelf()
{
    _viewer->setSceneGraph(nullptr);
    _viewer->setDocument(nullptr);
    MDIView::deleteSelf();
}

PyObject *View3DInventor::getPyObject()
{
    if (!_viewerPy)
        _viewerPy = new View3DInventorPy(this);

    Py_INCREF(_viewerPy);
    return _viewerPy;
}

void View3DInventor::applySettings()
{
    viewSettings = std::make_unique<View3DSettings>(App::GetApplication().GetParameterGroupByPath
                                   ("User parameter:BaseApp/Preferences/View"), _viewer);
    naviSettings = std::make_unique<NaviCubeSettings>(App::GetApplication().GetParameterGroupByPath
                                   ("User parameter:BaseApp/Preferences/NaviCube"), _viewer);
    viewSettings->applySettings();
    naviSettings->applySettings();
}

void View3DInventor::onRename(Gui::Document *pDoc)
{
    SoSFString name;
    name.setValue(pDoc->getDocument()->getName());
    SoFCDocumentAction cAct(name);
    cAct.apply(_viewer->getSceneGraph());
}

void View3DInventor::onUpdate()
{
#ifdef FC_LOGUPDATECHAIN
    Base::Console().Log("Acti: Gui::View3DInventor::onUpdate()");
#endif
    update();
    _viewer->redraw();
}

void View3DInventor::viewAll()
{
    _viewer->viewAll();
}

const char *View3DInventor::getName() const
{
    return "View3DInventor";
}

void View3DInventor::print()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setFullPage(true);
    restorePrinterSettings(&printer);

    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        Gui::WaitCursor wc;
        print(&printer);
        savePrinterSettings(&printer);
    }
}

void View3DInventor::printPdf()
{
    QString filename = FileDialog::getSaveFileName(this, tr("Export PDF"), QString(),
        QString::fromLatin1("%1 (*.pdf)").arg(tr("PDF file")));
    if (!filename.isEmpty()) {
        Gui::WaitCursor wc;
        QPrinter printer(QPrinter::ScreenResolution);
        // setPdfVersion sets the printied PDF Version to comply with PDF/A-1b, more details under: https://www.kdab.com/creating-pdfa-documents-qt/
        printer.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setPageOrientation(QPageLayout::Landscape);
        printer.setOutputFileName(filename);
        print(&printer);
    }
}

void View3DInventor::printPreview()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setFullPage(true);
    restorePrinterSettings(&printer);

    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, &QPrintPreviewDialog::paintRequested,
            this, qOverload<QPrinter*>(&View3DInventor::print));
    dlg.exec();
    savePrinterSettings(&printer);
}

void View3DInventor::print(QPrinter* printer)
{
    QPainter p(printer);
    p.setRenderHints(QPainter::Antialiasing);
    if (!p.isActive() && !printer->outputFileName().isEmpty()) {
        qApp->setOverrideCursor(Qt::ArrowCursor);
        QMessageBox::critical(this, tr("Opening file failed"),
            tr("Can't open file '%1' for writing.").arg(printer->outputFileName()));
        qApp->restoreOverrideCursor();
        return;
    }

    QRect rect = printer->pageLayout().paintRectPixels(printer->resolution());
    QImage img;
    _viewer->imageFromFramebuffer(rect.width(), rect.height(), 8, QColor(255,255,255), img);
    p.drawImage(0,0,img);
    p.end();
}

bool View3DInventor::containsViewProvider(const ViewProvider* vp) const
{
    return _viewer->containsViewProvider(vp);
}

// **********************************************************************************

bool View3DInventor::onMsg(const char* pMsg, const char** ppReturn)
{
    if (strcmp("ViewFit",pMsg) == 0) {
        _viewer->viewAll();
        return true;
    }
    else if (strcmp("ViewVR",pMsg) == 0) {
        // call the VR portion of the viewer
        _viewer->viewVR();
        return true;
    }
    else if(strcmp("ViewSelection",pMsg) == 0) {
        _viewer->viewSelection();
        return true;
    }
    else if(strcmp("SetStereoRedGreen",pMsg) == 0 ) {
        _viewer->setStereoMode(Quarter::SoQTQuarterAdaptor::ANAGLYPH);
        return true;
    }
    else if(strcmp("SetStereoQuadBuff",pMsg) == 0 ) {
        _viewer->setStereoMode(Quarter::SoQTQuarterAdaptor::QUAD_BUFFER );
        return true;
    }
    else if(strcmp("SetStereoInterleavedRows",pMsg) == 0 ) {
        _viewer->setStereoMode(Quarter::SoQTQuarterAdaptor::INTERLEAVED_ROWS );
        return true;
    }
    else if(strcmp("SetStereoInterleavedColumns",pMsg) == 0 ) {
        _viewer->setStereoMode(Quarter::SoQTQuarterAdaptor::INTERLEAVED_COLUMNS  );
        return true;
    }
    else if(strcmp("SetStereoOff",pMsg) == 0 ) {
        _viewer->setStereoMode(Quarter::SoQTQuarterAdaptor::MONO );
        return true;
    }
    else if(strcmp("Example1",pMsg) == 0 ) {
        auto root = new SoSeparator;
        Texture3D(root);
        _viewer->setSceneGraph(root);
        return true;
    }
    else if(strcmp("Example2",pMsg) == 0 ) {
        auto root = new SoSeparator;
        LightManip(root);
        _viewer->setSceneGraph(root);
        return true;
    }
    else if(strcmp("Example3",pMsg) == 0 ) {
        auto root = new SoSeparator;
        AnimationTexture(root);
        _viewer->setSceneGraph(root);
        return true;
    }
    else if(strcmp("GetCamera",pMsg) == 0 ) {
        SoCamera * Cam = _viewer->getSoRenderManager()->getCamera();
        if (!Cam)
            return false;
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
        _viewer->setCameraOrientation(Camera::rotation(Camera::Bottom));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewFront",pMsg) == 0 ) {
        _viewer->setCameraOrientation(Camera::rotation(Camera::Front));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewLeft",pMsg) == 0 ) {
        _viewer->setCameraOrientation(Camera::rotation(Camera::Left));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewRear",pMsg) == 0 ) {
        _viewer->setCameraOrientation(Camera::rotation(Camera::Rear));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewRight",pMsg) == 0 ) {
        _viewer->setCameraOrientation(Camera::rotation(Camera::Right));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewTop",pMsg) == 0 ) {
        _viewer->setCameraOrientation(Camera::rotation(Camera::Top));
        _viewer->viewAll();
        return true;
    }
    else if(strcmp("ViewAxo",pMsg) == 0 ) {
        _viewer->setCameraOrientation(Camera::rotation(Camera::Isometric));
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
    else if (strcmp("SaveCopy",pMsg) == 0) {
        getGuiDocument()->saveCopy();
        return true;
    }
    else if (strcmp("ZoomIn", pMsg) == 0) {
        View3DInventorViewer* viewer = getViewer();
        viewer->navigationStyle()->zoomIn();
        return true;
    }
    else if (strcmp("ZoomOut", pMsg) == 0) {
        View3DInventorViewer* viewer = getViewer();
        viewer->navigationStyle()->zoomOut();
        return true;
    }

    return false;
}

bool View3DInventor::onHasMsg(const char* pMsg) const
{
    if  (strcmp("Save",pMsg) == 0) {
        return true;
    }
    else if (strcmp("SaveAs",pMsg) == 0) {
        return true;
    }
    else if (strcmp("SaveCopy",pMsg) == 0) {
        return true;
    }
    else if (strcmp("Undo",pMsg) == 0) {
        App::Document* doc = getAppDocument();
        return doc && doc->getAvailableUndos() > 0;
    }
    else if (strcmp("Redo",pMsg) == 0) {
        App::Document* doc = getAppDocument();
        return doc && doc->getAvailableRedos() > 0;
    }
    else if (strcmp("Print",pMsg) == 0) {
        return true;
    }
    else if (strcmp("PrintPreview",pMsg) == 0) {
        return true;
    }
    else if (strcmp("PrintPdf",pMsg) == 0) {
        return true;
    }
    else if(strcmp("SetStereoRedGreen",pMsg) == 0) {
        return true;
    }
    else if(strcmp("SetStereoQuadBuff",pMsg) == 0) {
        return true;
    }
    else if(strcmp("SetStereoInterleavedRows",pMsg) == 0) {
        return true;
    }
    else if(strcmp("SetStereoInterleavedColumns",pMsg) == 0) {
        return true;
    }
    else if(strcmp("SetStereoOff",pMsg) == 0) {
        return true;
    }
    else if(strcmp("Example1",pMsg) == 0) {
        return true;
    }
    else if(strcmp("Example2",pMsg) == 0) {
        return true;
    }
    else if(strcmp("Example3",pMsg) == 0) {
        return true;
    }
    else if(strcmp("ViewFit",pMsg) == 0) {
        return true;
    }
    else if(strcmp("ViewVR",pMsg) == 0) {
#ifdef BUILD_VR
        return true;
#else
        return false;
#endif
    }
    else if(strcmp("ViewSelection",pMsg) == 0) {
        return true;
    }
    else if(strcmp("ViewBottom",pMsg) == 0) {
        return true;
    }
    else if(strcmp("ViewFront",pMsg) == 0) {
        return true;
    }
    else if(strcmp("ViewLeft",pMsg) == 0) {
        return true;
    }
    else if(strcmp("ViewRear",pMsg) == 0) {
        return true;
    }
    else if(strcmp("ViewRight",pMsg) == 0) {
        return true;
    }
    else if(strcmp("ViewTop",pMsg) == 0) {
        return true;
    }
    else if(strcmp("ViewAxo",pMsg) == 0) {
        return true;
    }
    else if(strcmp("GetCamera",pMsg) == 0) {
        return true;
    }
    else if(strncmp("SetCamera",pMsg,9) == 0) {
        return true;
    }
    else if(strncmp("Dump",pMsg,4) == 0) {
        return true;
    }
    if (strcmp("ZoomIn", pMsg) == 0) {
        return true;
    }
    if (strcmp("ZoomOut", pMsg) == 0) {
        return true;
    }

    return false;
}

bool View3DInventor::setCamera(const char* pCamera)
{
    SoCamera * CamViewer = _viewer->getSoRenderManager()->getCamera();
    if (!CamViewer) {
        throw Base::RuntimeError("No camera set so far...");
    }

    SoInput in;
    in.setBuffer((void*)pCamera,std::strlen(pCamera));

    SoNode * Cam;
    SoDB::read(&in,Cam);

    if (!Cam || !Cam->isOfType(SoCamera::getClassTypeId())) {
        throw Base::RuntimeError("Camera settings failed to read");
    }

    // this is to make sure to reliably delete the node
    CoinPtr<SoNode> camPtr(Cam, true);

    // toggle between perspective and orthographic camera
    if (Cam->getTypeId() != CamViewer->getTypeId()) {
        _viewer->setCameraType(Cam->getTypeId());
        CamViewer = _viewer->getSoRenderManager()->getCamera();
    }

    SoPerspectiveCamera  * CamViewerP = nullptr;
    SoOrthographicCamera * CamViewerO = nullptr;

    if (CamViewer->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        CamViewerP = static_cast<SoPerspectiveCamera *>(CamViewer);  // safe downward cast, knows the type
    }
    else if (CamViewer->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        CamViewerO = static_cast<SoOrthographicCamera *>(CamViewer);  // safe downward cast, knows the type
    }

    if (Cam->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        if (CamViewerP){
            CamViewerP->position      = static_cast<SoPerspectiveCamera *>(Cam)->position;
            CamViewerP->orientation   = static_cast<SoPerspectiveCamera *>(Cam)->orientation;
            CamViewerP->nearDistance  = static_cast<SoPerspectiveCamera *>(Cam)->nearDistance;
            CamViewerP->farDistance   = static_cast<SoPerspectiveCamera *>(Cam)->farDistance;
            CamViewerP->focalDistance = static_cast<SoPerspectiveCamera *>(Cam)->focalDistance;
        }
        else {
            throw Base::TypeError("Camera type mismatch");
        }
    }
    else if (Cam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        if (CamViewerO){
            CamViewerO->viewportMapping  = static_cast<SoOrthographicCamera *>(Cam)->viewportMapping;
            CamViewerO->position         = static_cast<SoOrthographicCamera *>(Cam)->position;
            CamViewerO->orientation      = static_cast<SoOrthographicCamera *>(Cam)->orientation;
            CamViewerO->nearDistance     = static_cast<SoOrthographicCamera *>(Cam)->nearDistance;
            CamViewerO->farDistance      = static_cast<SoOrthographicCamera *>(Cam)->farDistance;
            CamViewerO->focalDistance    = static_cast<SoOrthographicCamera *>(Cam)->focalDistance;
            CamViewerO->aspectRatio      = static_cast<SoOrthographicCamera *>(Cam)->aspectRatio ;
            CamViewerO->height           = static_cast<SoOrthographicCamera *>(Cam)->height;
        }
        else {
            throw Base::TypeError("Camera type mismatch");
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

void View3DInventor::setOverlayWidget(QWidget* widget)
{
    removeOverlayWidget();
    stack->addWidget(widget);
    stack->setCurrentIndex(1);
}

void View3DInventor::removeOverlayWidget()
{
    stack->setCurrentIndex(0);
    QWidget* overlay = stack->widget(1);
    if (overlay) stack->removeWidget(overlay);
}

void View3DInventor::setOverrideCursor(const QCursor& aCursor)
{
    _viewer->getWidget()->setCursor(aCursor);
}

void View3DInventor::restoreOverrideCursor()
{
    _viewer->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
}

// defined in SoFCDB.cpp
extern SoNode* replaceSwitchesInSceneGraph(SoNode*);

void View3DInventor::dump(const char* filename, bool onlyVisible)
{
    SoGetPrimitiveCountAction action;
    action.setCanApproximate(true);
    action.apply(_viewer->getSceneGraph());

    SoNode* node = _viewer->getSceneGraph();
    if (onlyVisible) {
        node = replaceSwitchesInSceneGraph(node);
        node->ref();
    }

    if ( action.getTriangleCount() > 100000 || action.getPointCount() > 30000 || action.getLineCount() > 10000 )
        _viewer->dumpToFile(node, filename, true);
    else
        _viewer->dumpToFile(node, filename, false);

    if (onlyVisible) {
        node->unref();
    }
}

void View3DInventor::windowStateChanged(MDIView* view)
{
    bool canStartTimer = false;
    if (this != view) {
        // If both views are child widgets of the workspace and view is maximized this view
        // must be hidden, hence we can start the timer.
        // Note: If view is top-level or fullscreen it doesn't necessarily hide the other view
        // e.g. if it is on a second monitor.
        canStartTimer = (!this->isWindow() && !view->isWindow() && view->isMaximized());
    } else if (isMinimized()) {
        // I am the active view but minimized
        canStartTimer = true;
    }

    if (canStartTimer) {
        int msecs = viewSettings->stopAnimatingIfDeactivated();
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
    // Here we must allow uri drags and check them in dropEvent
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

    if (newmode == Child) {
        // Fix in two steps:
        // The mdi view got a QWindow when it became a top-level widget and when resetting it to a child widget
        // the QWindow must be deleted because it has an impact on resize events and may break the layout of
        // mdi view inside the QMdiSubWindow.
        // In the second step below the layout must be invalidated after it's again a child widget to make sure
        // the mdi view fits into the QMdiSubWindow.
        QWindow* winHandle = this->windowHandle();
        if (winHandle)
            winHandle->destroy();
    }

    MDIView::setCurrentViewMode(newmode);

    // Internally the QOpenGLWidget switches of the multi-sampling and there is no
    // way to switch it on again. So as a workaround we just re-create a new viewport
    // The method is private but defined as slot to avoid to call it by accident.
    //int index = _viewer->metaObject()->indexOfMethod("replaceViewport()");
    //if (index >= 0) {
    //    _viewer->qt_metacall(QMetaObject::InvokeMetaMethod, index, 0);
    //}

    // This widget becomes the focus proxy of the embedded GL widget if we leave
    // the 'Child' mode. If we reenter 'Child' mode the focus proxy is reset to 0.
    // If we change from 'TopLevel' mode to 'Fullscreen' mode or vice versa nothing
    // happens.
    // Grabbing keyboard when leaving 'Child' mode (as done in a recent version) should
    // be avoided because when two or more windows are either in 'TopLevel' or 'Fullscreen'
    // mode only the last window gets all key event even if it is not the active one.
    //
    // It is important to set the focus proxy to get all key events otherwise we would lose
    // control after redirecting the first key event to the GL widget.
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
        _viewer->getGLWidget()->setFocusProxy(nullptr);
        qApp->removeEventFilter(this);
        QList<QAction*> acts = this->actions();
        for (QAction* it : acts)
            this->removeAction(it);

        // Step two
        auto mdi = qobject_cast<QMdiSubWindow*>(parentWidget());
        if (mdi && mdi->layout())
            mdi->layout()->invalidate();
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
        auto a = static_cast<QActionEvent*>(e);
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
    // See StdViewDockUndockFullscreen::activated()
    // With Qt5 one cannot directly use 'setCurrentViewMode'
    // of an MDI view because it causes rendering problems.
    // The only reliable solution is to clone the MDI view,
    // set its view mode and close the original MDI view.

    QMainWindow::keyPressEvent(e);
}

void View3DInventor::keyReleaseEvent (QKeyEvent* e)
{
    QMainWindow::keyReleaseEvent(e);
}

void View3DInventor::focusInEvent (QFocusEvent *)
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
        auto se = static_cast<NavigationStyleEvent*>(e);
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/View");
        if (hGrp->GetBool("SameStyleForAllViews", true))
            hGrp->SetASCII("NavigationStyle", se->style().getName());
        else
            _viewer->setNavigationType(se->style());
    }
}


#include "moc_View3DInventor.cpp"
