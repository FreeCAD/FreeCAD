/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
# include <QDir>
# include <QFileInfo>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>

#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Gui/Command.h>
#include <Gui/WaitCursor.h>

#include "DlgProcessorChooser.h"
#include "ui_DlgProcessorChooser.h"


namespace PathGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("PathGui")
    {
        add_varargs_method("open",&Module::open,
            "open(filename): Opens a GCode file as a new document"
        );
        add_varargs_method("insert",&Module::insert,
            "insert(filename,docname): Imports a given GCode file into the given document"
        );
        add_varargs_method("export",&Module::exporter,
            "export(objectslist,filename): Exports a given list of Path objects to a GCode file"
        );
        initialize("This module is the PathGui module."); // register with Python
    }

    ~Module() override {}

private:
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et","utf-8",&Name))
            throw Py::Exception();
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);
        Base::FileInfo fi(EncodedName);
        if (!fi.exists())
            throw Py::RuntimeError("File not found");

        Gui::WaitCursor wc;
        wc.restoreCursor();

        try {
            std::string path = App::Application::getHomePath();
            path += "Mod/Path/Path/Post/scripts/";
            QDir dir1(QString::fromUtf8(path.c_str()), QString::fromLatin1("*_pre.py"));
            std::string cMacroPath = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
                ->GetASCII("MacroPath",App::Application::getUserMacroDir().c_str());
            QDir dir2(QString::fromUtf8(cMacroPath.c_str()), QString::fromLatin1("*_pre.py"));
            QFileInfoList list = dir1.entryInfoList();
            list << dir2.entryInfoList();
            std::vector<std::string> scripts;
            for (int i = 0; i < list.size(); ++i) {
                QFileInfo fileInfo = list.at(i);
                scripts.push_back(fileInfo.baseName().toStdString());
            }
            std::string processor;
            PathGui::DlgProcessorChooser Dlg(scripts);
            if (Dlg.exec() != QDialog::Accepted) {
                return Py::None();
            }
            processor = Dlg.getProcessor();

            std::ostringstream pre;
            std::ostringstream cmd;
            if (processor.empty()) {
                App::Document *pcDoc = App::GetApplication().newDocument();
                Gui::Command::runCommand(Gui::Command::Gui,"import Path");
                cmd << "Path.read(\"" << EncodedName << "\",\"" << pcDoc->getName() << "\")";
                Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
            } else {
                for (int i = 0; i < list.size(); ++i) {
                    QFileInfo fileInfo = list.at(i);
                    if (fileInfo.baseName().toStdString() == processor) {
                        if (fileInfo.absoluteFilePath().contains(QString::fromLatin1("PathScripts"))) {
                            pre << "from Path.Post.scripts import " << processor;
                        } else {
                            pre << "import " << processor;
                        }
                        Gui::Command::runCommand(Gui::Command::Gui,pre.str().c_str());
                        cmd << processor << ".open(\"" << EncodedName << "\")";
                        Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
                    }
                }
            }
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

    Py::Object insert(const Py::Tuple& args)
    {
        char* Name;
        char* DocName=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "et|s","utf-8",&Name,&DocName))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);
        Base::FileInfo fi(EncodedName);
        if (!fi.exists())
            throw Py::RuntimeError("File not found");

        Gui::WaitCursor wc;
        wc.restoreCursor();

        try {
            std::string path = App::Application::getHomePath();
            path += "Mod/Path/Path/Post/scripts/";
            QDir dir1(QString::fromUtf8(path.c_str()), QString::fromLatin1("*_pre.py"));
            std::string cMacroPath = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
                ->GetASCII("MacroPath",App::Application::getUserMacroDir().c_str());
            QDir dir2(QString::fromUtf8(cMacroPath.c_str()), QString::fromLatin1("*_pre.py"));
            QFileInfoList list = dir1.entryInfoList();
            list << dir2.entryInfoList();
            std::vector<std::string> scripts;
            for (int i = 0; i < list.size(); ++i) {
                QFileInfo fileInfo = list.at(i);
                scripts.push_back(fileInfo.baseName().toStdString());
            }
            std::string processor;
            PathGui::DlgProcessorChooser Dlg(scripts);
            if (Dlg.exec() != QDialog::Accepted) {
                return Py::None();
            }
            processor = Dlg.getProcessor();

            App::Document *pcDoc = nullptr;
            if (DocName)
                pcDoc = App::GetApplication().getDocument(DocName);
            else
                pcDoc = App::GetApplication().getActiveDocument();

            if (!pcDoc) {
                pcDoc = App::GetApplication().newDocument(DocName);
            }

            std::ostringstream pre;
            std::ostringstream cmd;
            if (processor.empty()) {
                Gui::Command::runCommand(Gui::Command::Gui,"import Path");
                cmd << "Path.read(\"" << EncodedName << "\",\"" << pcDoc->getName() << "\")";
                Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
            } else {
                for (int i = 0; i < list.size(); ++i) {
                    QFileInfo fileInfo = list.at(i);
                    if (fileInfo.baseName().toStdString() == processor) {
                        if (fileInfo.absoluteFilePath().contains(QString::fromLatin1("PathScripts"))) {
                            pre << "from Path.Post.scripts import " << processor;
                        } else {
                            pre << "import " << processor;
                        }
                        Gui::Command::runCommand(Gui::Command::Gui,pre.str().c_str());
                        cmd << processor << ".insert(\"" << EncodedName << "\",\"" << pcDoc->getName() << "\")";
                        Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
                    }
                }
            }
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

    Py::Object exporter(const Py::Tuple& args)
    {
        PyObject* object;
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet",&object,"utf-8",&Name))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);
        Gui::WaitCursor wc;
        wc.restoreCursor();

        try {
            Py::Sequence objlist(object);
            if (objlist.size() == 0)
                throw Py::RuntimeError("No object to export");

            std::string path = App::Application::getHomePath();
            path += "Mod/Path/Path/Post/scripts/";
            QDir dir1(QString::fromUtf8(path.c_str()), QString::fromLatin1("*_post.py"));
            std::string cMacroPath = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
                ->GetASCII("MacroPath",App::Application::getUserMacroDir().c_str());
            QDir dir2(QString::fromUtf8(cMacroPath.c_str()), QString::fromLatin1("*_post.py"));
            QFileInfoList list = dir1.entryInfoList();
            list << dir2.entryInfoList();
            std::vector<std::string> scripts;
            for (int i = 0; i < list.size(); ++i) {
                QFileInfo fileInfo = list.at(i);
                scripts.push_back(fileInfo.baseName().toStdString());
            }
            PathGui::DlgProcessorChooser Dlg(scripts, true);
            if (Dlg.exec() != QDialog::Accepted) {
                return Py::None();
            }
            std::string processor = Dlg.getProcessor();
            std::string arguments = Dlg.getArguments();

            std::ostringstream pre;
            std::ostringstream cmd;
            if (processor.empty()) {
                if (objlist.size() > 1) {
                    throw Py::RuntimeError("Cannot export more than one object without using a post script");
                }
                PyObject* item = objlist[0].ptr();
                if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                    App::Document* doc = obj->getDocument();
                    Gui::Command::runCommand(Gui::Command::Gui,"import Path");
                    cmd << "Path.write(FreeCAD.getDocument(\"" << doc->getName() << "\").getObject(\"" << obj->getNameInDocument() << "\"),\"" << EncodedName << "\")";
                    Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
                } else {
                    return Py::None();
                }
            } else {
                for (int i = 0; i < list.size(); ++i) {
                    QFileInfo fileInfo = list.at(i);
                    if (fileInfo.baseName().toStdString() == processor) {
                        if (fileInfo.absoluteFilePath().contains(QString::fromLatin1("PathScripts"))) {
                            pre << "from Path.Post.scripts import " << processor;
                        } else {
                            pre << "import " << processor;
                        }
                        Gui::Command::runCommand(Gui::Command::Gui,pre.str().c_str());
                        cmd << processor << ".export(__objs__,\"" << EncodedName << "\",\"" << arguments << "\")";
                        Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
                    }
                }
            }
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace PathGui
