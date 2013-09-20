/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <gp_Vec.hxx>
# include <QFileInfo>
# include <Inventor/SoInput.h>
# include <Inventor/nodes/SoNode.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <vector>
# include <Inventor/nodes/SoPerspectiveCamera.h>
#endif

#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/EditorView.h>
#include <Gui/TextEdit.h>
#include <Gui/MainWindow.h>
#include <Gui/MainWindow.h>
#include <Gui/View.h>

#include <Mod/Raytracing/App/PovTools.h>
#include <Mod/Raytracing/App/LuxTools.h>
#include "PovrayHighlighter.h"

using namespace RaytracingGui;
using namespace Raytracing;

/// open pov file
static PyObject *
open(PyObject *self, PyObject *args)
{
    // only used to open Povray files
    const char* Name;
    const char* DocName;
    if (!PyArg_ParseTuple(args, "s|s",&Name, &DocName))
        return NULL;
    PY_TRY {
        QString fileName = QString::fromUtf8(Name);
        QFileInfo fi;
        fi.setFile(fileName);
        QString ext = fi.completeSuffix().toLower();
        QList<Gui::EditorView*> views = Gui::getMainWindow()->findChildren<Gui::EditorView*>();
        for (QList<Gui::EditorView*>::Iterator it = views.begin(); it != views.end(); ++it) {
            if ((*it)->fileName() == fileName) {
                (*it)->setFocus();
                Py_Return;
            }
        }

        if (ext == QLatin1String("pov") || ext == QLatin1String("inc")) {
            Gui::TextEditor* editor = new Gui::TextEditor();
            editor->setSyntaxHighlighter(new PovrayHighlighter(editor));
            Gui::EditorView* edit = new Gui::EditorView(editor, Gui::getMainWindow());
            edit->open(fileName);
            edit->resize(400, 300);
            Gui::getMainWindow()->addWindow(edit);
        }
    } PY_CATCH;

    Py_Return;
}

/// return the camera definition of the active view
static PyObject *
povViewCamera(PyObject *self, PyObject *args)
{
    // no arguments
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    PY_TRY {
        std::string out;
        const char* ppReturn=0;

        Gui::Application::Instance->sendMsgToActiveView("GetCamera",&ppReturn);

        SoNode* rootNode;
        SoInput in;
        in.setBuffer((void*)ppReturn,std::strlen(ppReturn));
        SoDB::read(&in,rootNode);

        if (!rootNode || !rootNode->getTypeId().isDerivedFrom(SoCamera::getClassTypeId()))
            throw Base::Exception("CmdRaytracingWriteCamera::activated(): Could not read "
                                  "camera information from ASCII stream....\n");

        // root-node returned from SoDB::readAll() has initial zero
        // ref-count, so reference it before we start using it to
        // avoid premature destruction.
        SoCamera * Cam = static_cast<SoCamera*>(rootNode);
        Cam->ref();

        SbRotation camrot = Cam->orientation.getValue();

        SbVec3f upvec(0, 1, 0); // init to default up vector
        camrot.multVec(upvec, upvec);

        SbVec3f lookat(0, 0, -1); // init to default view direction vector
        camrot.multVec(lookat, lookat);

        SbVec3f pos = Cam->position.getValue();
        float Dist = Cam->focalDistance.getValue();

        // making gp out of the Coin stuff
        gp_Vec gpPos(pos.getValue()[0],pos.getValue()[1],pos.getValue()[2]);
        gp_Vec gpDir(lookat.getValue()[0],lookat.getValue()[1],lookat.getValue()[2]);
        lookat *= Dist;
        lookat += pos;
        gp_Vec gpLookAt(lookat.getValue()[0],lookat.getValue()[1],lookat.getValue()[2]);
        gp_Vec gpUp(upvec.getValue()[0],upvec.getValue()[1],upvec.getValue()[2]);

        // call the write method of PovTools....
        out = PovTools::getCamera(CamDef(gpPos,gpDir,gpLookAt,gpUp));

        return Py::new_reference_to(Py::String(out));
    } PY_CATCH;
}

/// return a luxrender camera definition of the active view
static PyObject *
luxViewCamera(PyObject *self, PyObject *args)
{
    // no arguments
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    PY_TRY {
        std::string out;
        const char* ppReturn=0;

        Gui::Application::Instance->sendMsgToActiveView("GetCamera",&ppReturn);

        SoNode* rootNode;
        SoInput in;
        in.setBuffer((void*)ppReturn,std::strlen(ppReturn));
        SoDB::read(&in,rootNode);

        if (!rootNode || !rootNode->getTypeId().isDerivedFrom(SoCamera::getClassTypeId()))
            throw Base::Exception("CmdRaytracingWriteCamera::activated(): Could not read "
                                  "camera information from ASCII stream....\n");

        // root-node returned from SoDB::readAll() has initial zero
        // ref-count, so reference it before we start using it to
        // avoid premature destruction.
        SoCamera * Cam = static_cast<SoCamera*>(rootNode);
        Cam->ref();

        SbRotation camrot = Cam->orientation.getValue();

        SbVec3f upvec(0, 1, 0); // init to default up vector
        camrot.multVec(upvec, upvec);

        SbVec3f lookat(0, 0, -1); // init to default view direction vector
        camrot.multVec(lookat, lookat);

        SbVec3f pos = Cam->position.getValue();
        float Dist = Cam->focalDistance.getValue();

        // making gp out of the Coin stuff
        gp_Vec gpPos(pos.getValue()[0],pos.getValue()[1],pos.getValue()[2]);
        gp_Vec gpDir(lookat.getValue()[0],lookat.getValue()[1],lookat.getValue()[2]);
        lookat *= Dist;
        lookat += pos;
        gp_Vec gpLookAt(lookat.getValue()[0],lookat.getValue()[1],lookat.getValue()[2]);
        gp_Vec gpUp(upvec.getValue()[0],upvec.getValue()[1],upvec.getValue()[2]);

        // call the write method of PovTools....
        out = LuxTools::getCamera(CamDef(gpPos,gpDir,gpLookAt,gpUp));

        return Py::new_reference_to(Py::String(out));
    } PY_CATCH;
}

/* registration table  */
struct PyMethodDef RaytracingGui_methods[] = {
    {"open"       ,open    ,METH_VARARGS,
     "open(string) -- Create a new text document and load the file into the document."},
    {"insert"     ,open    ,METH_VARARGS,
     "insert(string,string) -- Create a new text document and load the file into the document."},
    {"povViewCamera"     ,povViewCamera    ,METH_VARARGS,
     "string povViewCamera() -- returns the povray camera definition of the active 3D view."},
    {"luxViewCamera"     ,luxViewCamera    ,METH_VARARGS,
     "string luxViewCamera() -- returns the luxrender camera definition of the active 3D view."},
    {NULL, NULL}
};
