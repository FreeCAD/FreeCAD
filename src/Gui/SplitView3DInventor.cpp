/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QSplitter>
# include <Inventor/nodes/SoDirectionalLight.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
#endif

#include <Base/Builder3D.h>
#include <Base/Interpreter.h>

#include "SplitView3DInventor.h"
#include "Application.h"
#include "Camera.h"
#include "Document.h"
#include "NavigationStyle.h"
#include "SoFCSelectionAction.h"
#include "View3DInventorViewer.h"
#include "View3DPy.h"
#include "View3DSettings.h"


using namespace Gui;

TYPESYSTEM_SOURCE_ABSTRACT(Gui::AbstractSplitView,Gui::MDIView)

AbstractSplitView::AbstractSplitView(Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags)
  : MDIView(pcDocument,parent, wflags)
{
    _viewerPy = nullptr;
    // important for highlighting
    setMouseTracking(true);
}

AbstractSplitView::~AbstractSplitView()
{
    for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
        delete *it;
    }
    if (_viewerPy) {
        Base::PyGILStateLocker lock;
        Py_DECREF(_viewerPy);
    }
}

void AbstractSplitView::deleteSelf()
{
    for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
        (*it)->setSceneGraph(nullptr);
    }
    MDIView::deleteSelf();
}

void AbstractSplitView::setDocumentOfViewers(Gui::Document* document)
{
    for (auto view : _viewer) {
        view->setDocument(document);
    }
}

void AbstractSplitView::viewAll()
{
    for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it)
        (*it)->viewAll();
}

bool AbstractSplitView::containsViewProvider(const ViewProvider* vp) const
{
    for (auto it = _viewer.begin(); it != _viewer.end(); ++it) {
        if ((*it)->containsViewProvider(vp))
            return true;
    }

    return false;
}

void AbstractSplitView::setupSettings()
{
    viewSettings = std::make_unique<View3DSettings>(App::GetApplication().GetParameterGroupByPath
                                   ("User parameter:BaseApp/Preferences/View"), _viewer);
    // tmp. disabled will be activated after redesign of 3d viewer
    // check whether the simple or the Full Mouse model is used
    viewSettings->ignoreNavigationStyle = true;
    // Disable VBO for split screen as this leads to random crashes
    viewSettings->ignoreVBO = true;
    viewSettings->ignoreTransparent = true;
    viewSettings->ignoreRenderCache = true;
    viewSettings->ignoreDimensions = true;
    viewSettings->applySettings();

    for (auto view : _viewer) {
        NaviCubeSettings naviSettings(App::GetApplication().GetParameterGroupByPath
                                     ("User parameter:BaseApp/Preferences/NaviCube"), view);
        naviSettings.applySettings();
    }
}

View3DInventorViewer* AbstractSplitView::getViewer(unsigned int n) const
{
    return (_viewer.size() > n ? _viewer[n] : nullptr);
}

void AbstractSplitView::onUpdate()
{
    update();
}

const char *AbstractSplitView::getName() const
{
    return "SplitView3DInventor";
}

bool AbstractSplitView::onMsg(const char* pMsg, const char**)
{
    if (strcmp("ViewFit",pMsg) == 0 ) {
        viewAll();
        return true;
    }
    else if (strcmp("ViewBottom",pMsg) == 0) {
        SbRotation rot(Camera::rotation(Camera::Bottom));
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getSoRenderManager()->getCamera();
            cam->orientation.setValue(rot);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewFront",pMsg) == 0) {
        SbRotation rot(Camera::rotation(Camera::Front));
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getSoRenderManager()->getCamera();
            cam->orientation.setValue(rot);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewLeft",pMsg) == 0) {
        SbRotation rot(Camera::rotation(Camera::Left));
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getSoRenderManager()->getCamera();
            cam->orientation.setValue(rot);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewRear",pMsg) == 0) {
        SbRotation rot(Camera::rotation(Camera::Rear));
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getSoRenderManager()->getCamera();
            cam->orientation.setValue(rot);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewRight",pMsg) == 0) {
        SbRotation rot(Camera::rotation(Camera::Right));
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getSoRenderManager()->getCamera();
            cam->orientation.setValue(rot);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewTop",pMsg) == 0) {
        SbRotation rot(Camera::rotation(Camera::Top));
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getSoRenderManager()->getCamera();
            cam->orientation.setValue(rot);
            (*it)->viewAll();
        }
        return true;
    }
    else if (strcmp("ViewAxo",pMsg) == 0) {
        SbRotation rot(Camera::rotation(Camera::Isometric));
        for (std::vector<View3DInventorViewer*>::iterator it = _viewer.begin(); it != _viewer.end(); ++it) {
            SoCamera* cam = (*it)->getSoRenderManager()->getCamera();
            cam->orientation.setValue(rot);
            (*it)->viewAll();
        }
        return true;
    }

    return false;
}

bool AbstractSplitView::onHasMsg(const char* pMsg) const
{
    if (strcmp("ViewFit",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewBottom",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewFront",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewLeft",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewRear",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewRight",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewTop",pMsg) == 0) {
        return true;
    }
    else if (strcmp("ViewAxo",pMsg) == 0) {
        return true;
    }
    return false;
}

void AbstractSplitView::setOverrideCursor(const QCursor& aCursor)
{
    Q_UNUSED(aCursor);
    //_viewer->getWidget()->setCursor(aCursor);
}

PyObject *AbstractSplitView::getPyObject()
{
    if (!_viewerPy)
        _viewerPy = new AbstractSplitViewPy(this);
    Py_INCREF(_viewerPy);
    return _viewerPy;
}

void AbstractSplitView::setPyObject(PyObject *)
{
    throw Base::AttributeError("Attribute is read-only");
}

int AbstractSplitView::getSize()
{
    return static_cast<int>(_viewer.size());
}

// ------------------------------------------------------

void AbstractSplitViewPy::init_type()
{
    behaviors().name("AbstractSplitViewPy");
    behaviors().doc("Python binding class for the Inventor viewer class");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    behaviors().supportSequenceType();

    add_varargs_method("fitAll",&AbstractSplitViewPy::fitAll,"fitAll()");
    add_varargs_method("viewBottom",&AbstractSplitViewPy::viewBottom,"viewBottom()");
    add_varargs_method("viewFront",&AbstractSplitViewPy::viewFront,"viewFront()");
    add_varargs_method("viewLeft",&AbstractSplitViewPy::viewLeft,"viewLeft()");
    add_varargs_method("viewRear",&AbstractSplitViewPy::viewRear,"viewRear()");
    add_varargs_method("viewRight",&AbstractSplitViewPy::viewRight,"viewRight()");
    add_varargs_method("viewTop",&AbstractSplitViewPy::viewTop,"viewTop()");
    add_varargs_method("viewAxometric",&AbstractSplitViewPy::viewIsometric,"viewAxometric()");
    add_varargs_method("viewIsometric",&AbstractSplitViewPy::viewIsometric,"viewIsometric()");
    add_varargs_method("getViewer",&AbstractSplitViewPy::getViewer,"getViewer(index)");
    add_varargs_method("close",&AbstractSplitViewPy::close,"close()");
    add_varargs_method("cast_to_base", &AbstractSplitViewPy::cast_to_base, "cast_to_base() cast to MDIView class");
    behaviors().readyType();
}

AbstractSplitViewPy::AbstractSplitViewPy(AbstractSplitView *vi)
  : base(vi)
{
}

AbstractSplitViewPy::~AbstractSplitViewPy() = default;

Py::Object AbstractSplitViewPy::cast_to_base(const Py::Tuple&)
{
    return Gui::MDIViewPy::create(base.getMDIViewPtr());
}

Py::Object AbstractSplitViewPy::repr()
{
    std::ostringstream s_out;
    if (!getSplitViewPtr())
        throw Py::RuntimeError("Cannot print representation of deleted object");
    s_out << "AbstractSplitView";
    return Py::String(s_out.str());
}

// Since with PyCXX it's not possible to make a sub-class of MDIViewPy
// a trick is to use MDIViewPy as class member and override getattr() to
// join the attributes of both classes. This way all methods of MDIViewPy
// appear for SheetViewPy, too.
Py::Object AbstractSplitViewPy::getattr(const char * attr)
{
    getSplitViewPtr();
    std::string name( attr );
    if (name == "__dict__" || name == "__class__") {
        Py::Dict dict_self(BaseType::getattr("__dict__"));
        Py::Dict dict_base(base.getattr("__dict__"));
        for (const auto& it : dict_base) {
            dict_self.setItem(it.first, it.second);
        }
        return dict_self;
    }

    try {
        return BaseType::getattr(attr);
    }
    catch (Py::AttributeError& e) {
        e.clear();
        return base.getattr(attr);
    }
}

AbstractSplitView* AbstractSplitViewPy::getSplitViewPtr()
{
    auto view = qobject_cast<AbstractSplitView*>(base.getMDIViewPtr());
    if (!(view && view->getViewer(0)))
        throw Py::RuntimeError("Object already deleted");
    return view;
}

Py::Object AbstractSplitViewPy::fitAll(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        getSplitViewPtr()->onMsg("ViewFit", nullptr);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
    return Py::None();
}

Py::Object AbstractSplitViewPy::viewBottom(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        getSplitViewPtr()->onMsg("ViewBottom", nullptr);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object AbstractSplitViewPy::viewFront(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        getSplitViewPtr()->onMsg("ViewFront", nullptr);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object AbstractSplitViewPy::viewLeft(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        getSplitViewPtr()->onMsg("ViewLeft", nullptr);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object AbstractSplitViewPy::viewRear(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        getSplitViewPtr()->onMsg("ViewRear", nullptr);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object AbstractSplitViewPy::viewRight(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        getSplitViewPtr()->onMsg("ViewRight", nullptr);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object AbstractSplitViewPy::viewTop(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        getSplitViewPtr()->onMsg("ViewTop", nullptr);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object AbstractSplitViewPy::viewIsometric(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        getSplitViewPtr()->onMsg("ViewAxo", nullptr);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }

    return Py::None();
}

Py::Object AbstractSplitViewPy::getViewer(const Py::Tuple& args)
{
    int viewIndex;
    if (!PyArg_ParseTuple(args.ptr(), "i", &viewIndex))
        throw Py::Exception();

    try {
        Gui::View3DInventorViewer* view = getSplitViewPtr()->getViewer(viewIndex);
        if (!view)
            throw Py::IndexError("Index out of range");
        return Py::asObject(view->getPyObject());
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const Py::Exception&) {
        // re-throw
        throw;
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object AbstractSplitViewPy::sequence_item(Py_ssize_t viewIndex)
{
    AbstractSplitView* view = getSplitViewPtr();
    if (viewIndex >= view->getSize() || viewIndex < 0)
        throw Py::IndexError("Index out of range");
    PyObject* viewer = view->getViewer(viewIndex)->getPyObject();
    return Py::asObject(viewer);
}

PyCxx_ssize_t AbstractSplitViewPy::sequence_length()
{
    AbstractSplitView* view = getSplitViewPtr();
    return view->getSize();
}

Py::Object AbstractSplitViewPy::close(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    AbstractSplitView* view = getSplitViewPtr();
    view->close();
    if (view->parentWidget())
        view->parentWidget()->deleteLater();

    return Py::None();
}

// ------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Gui::SplitView3DInventor, Gui::AbstractSplitView)

SplitView3DInventor::SplitView3DInventor(int views, Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags)
  : AbstractSplitView(pcDocument,parent, wflags)
{
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

    // minimal 2 views
    while (views < 2)
        views ++;

    QSplitter* mainSplitter = nullptr;

    // if views < 3 show them as a row
    if (views <= 3) {
        mainSplitter = new QSplitter(Qt::Horizontal, this);
        for (int i=0; i < views; i++) {
            if (glformat)
                _viewer.push_back(new View3DInventorViewer(f, mainSplitter));
            else
                _viewer.push_back(new View3DInventorViewer(mainSplitter));
        }
    }
    else {
        mainSplitter = new QSplitter(Qt::Vertical, this);
        auto topSplitter = new QSplitter(Qt::Horizontal, mainSplitter);
        auto botSplitter = new QSplitter(Qt::Horizontal, mainSplitter);

        if (glformat) {
            _viewer.push_back(new View3DInventorViewer(f, topSplitter));
            _viewer.push_back(new View3DInventorViewer(f, topSplitter));
        }
        else {
            _viewer.push_back(new View3DInventorViewer(topSplitter));
            _viewer.push_back(new View3DInventorViewer(topSplitter));
        }

        for (int i=2;i<views;i++) {
            if (glformat)
                _viewer.push_back(new View3DInventorViewer(f, botSplitter));
            else
                _viewer.push_back(new View3DInventorViewer(botSplitter));
        }

        topSplitter->setOpaqueResize(true);
        botSplitter->setOpaqueResize(true);
    }

    if (smoothing) {
        for (std::vector<int>::size_type i = 0; i != _viewer.size(); i++)
            _viewer[i]->getSoRenderManager()->getGLRenderAction()->setSmoothing(true);
    }

    mainSplitter->show();
    setCentralWidget(mainSplitter);

    setDocumentOfViewers(pcDocument);

    // apply the user settings
    setupSettings();
}

SplitView3DInventor::~SplitView3DInventor() = default;

#include "moc_SplitView3DInventor.cpp"
