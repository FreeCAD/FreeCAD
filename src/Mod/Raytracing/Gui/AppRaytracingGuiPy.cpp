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

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/EditorView.h>
#include <Gui/TextEdit.h>
#include <Gui/MainWindow.h>
#include <Gui/View.h>

#include <Mod/Raytracing/App/PovTools.h>
#include <Mod/Raytracing/App/LuxTools.h>
#include "PovrayHighlighter.h"

namespace RaytracingGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("RaytracingGui")
    {
        add_varargs_method("open",&Module::open,
            "open(string) -- Create a new text document and load the file into the document."
        );
        add_varargs_method("insert",&Module::open,
            "insert(string,string) -- Create a new text document and load the file into the document."
        );
        add_varargs_method("povViewCamera",&Module::povViewCamera,
            "string povViewCamera() -- returns the povray camera definition of the active 3D view."
        );
        add_varargs_method("luxViewCamera",&Module::luxViewCamera,
            "string luxViewCamera() -- returns the luxrender camera definition of the active 3D view."
        );
        initialize("This module is the RaytracingGui module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object open(const Py::Tuple& args)
    {
        // only used to open Povray files
        char* Name;
        const char* DocName;
        if (!PyArg_ParseTuple(args.ptr(), "et|s","utf-8",&Name,&DocName))
            throw Py::Exception();
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);
        try {
            QString fileName = QString::fromUtf8(EncodedName.c_str());
            QFileInfo fi;
            fi.setFile(fileName);
            QString ext = fi.completeSuffix().toLower();
            QList<Gui::EditorView*> views = Gui::getMainWindow()->findChildren<Gui::EditorView*>();
            for (QList<Gui::EditorView*>::Iterator it = views.begin(); it != views.end(); ++it) {
                if ((*it)->fileName() == fileName) {
                    (*it)->setFocus();
                    return Py::None();
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
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
        return Py::None();
    }

    Py::Object povViewCamera(const Py::Tuple& args)
    {
        // no arguments
        if (!PyArg_ParseTuple(args.ptr(), ""))
            throw Py::Exception();
        try {
            std::string out;
            const char* ppReturn=nullptr;

            Gui::Document* doc = Gui::Application::Instance->activeDocument();
            if (doc) {
                doc->sendMsgToFirstView(Gui::MDIView::getClassTypeId(), "GetCamera", &ppReturn);
            }
            else {
                throw Py::RuntimeError("No active document found");
            }

            if (!ppReturn) {
                throw Py::RuntimeError("Could not read camera information from active view");
            }

            SoNode* rootNode;
            SoInput in;
            in.setBuffer((void*)ppReturn,std::strlen(ppReturn));
            SoDB::read(&in,rootNode);

            if (!rootNode || !rootNode->getTypeId().isDerivedFrom(SoCamera::getClassTypeId())) {
                throw Py::RuntimeError("Could not read camera information from ASCII stream");
            }

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
            Cam->unref(); // free memory

            // making gp out of the Coin stuff
            gp_Vec gpPos(pos.getValue()[0],pos.getValue()[1],pos.getValue()[2]);
            gp_Vec gpDir(lookat.getValue()[0],lookat.getValue()[1],lookat.getValue()[2]);
            lookat *= Dist;
            lookat += pos;
            gp_Vec gpLookAt(lookat.getValue()[0],lookat.getValue()[1],lookat.getValue()[2]);
            gp_Vec gpUp(upvec.getValue()[0],upvec.getValue()[1],upvec.getValue()[2]);

            // getting image format
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Raytracing");
            int width = hGrp->GetInt("OutputWidth", 800);
            int height = hGrp->GetInt("OutputHeight", 600);

            // call the write method of PovTools....
            out = Raytracing::PovTools::getCamera(Raytracing::CamDef(gpPos,gpDir,gpLookAt,gpUp),width,height);

            return Py::String(out);
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
    }

    Py::Object luxViewCamera(const Py::Tuple& args)
    {
        // no arguments
        if (!PyArg_ParseTuple(args.ptr(), ""))
            throw Py::Exception();
        try {
            std::string out;
            const char* ppReturn=nullptr;

            Gui::Document* doc = Gui::Application::Instance->activeDocument();
            if (doc) {
                doc->sendMsgToFirstView(Gui::MDIView::getClassTypeId(), "GetCamera", &ppReturn);
            }
            else {
                throw Py::RuntimeError("No active document found");
            }

            if (!ppReturn) {
                throw Py::RuntimeError("Could not read camera information from active view");
            }

            SoNode* rootNode;
            SoInput in;
            in.setBuffer((void*)ppReturn,std::strlen(ppReturn));
            SoDB::read(&in,rootNode);

            if (!rootNode || !rootNode->getTypeId().isDerivedFrom(SoCamera::getClassTypeId())) {
                throw Py::RuntimeError("Could not read camera information from ASCII stream");
            }

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
            Cam->unref(); // free memory

            // making gp out of the Coin stuff
            gp_Vec gpPos(pos.getValue()[0],pos.getValue()[1],pos.getValue()[2]);
            gp_Vec gpDir(lookat.getValue()[0],lookat.getValue()[1],lookat.getValue()[2]);
            lookat *= Dist;
            lookat += pos;
            gp_Vec gpLookAt(lookat.getValue()[0],lookat.getValue()[1],lookat.getValue()[2]);
            gp_Vec gpUp(upvec.getValue()[0],upvec.getValue()[1],upvec.getValue()[2]);

            // call the write method of PovTools....
            out = Raytracing::LuxTools::getCamera(Raytracing::CamDef(gpPos,gpDir,gpLookAt,gpUp));

            return Py::String(out);
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace RaytracingGui
