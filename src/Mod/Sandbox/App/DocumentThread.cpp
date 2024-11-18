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
# include <QMutexLocker>
#endif

#include "DocumentThread.h"
#include "DocumentProtector.h"

#include <Base/Console.h>
#include <Base/Sequencer.h>
#include <Base/Stream.h>
#include <Base/Writer.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Mesh/App/Mesh.h>

using namespace Sandbox;


DocumentThread::DocumentThread(QObject* parent)
  : QThread(parent)
{
}

DocumentThread::~DocumentThread()
{
}

void DocumentThread::run()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    DocumentProtector dp(doc);
    dp.addObject("Mesh::Ellipsoid", (const char*)objectName().toLatin1());
    dp.recompute();
}

// --------------------------------------

WorkerThread::WorkerThread(QObject* parent)
  : QThread(parent)
{
}

WorkerThread::~WorkerThread()
{
}

void WorkerThread::run()
{
#ifdef FC_DEBUG
    int max = 10000;
#else
    int max = 100000000;
#endif
    Base::SequencerLauncher seq("Do something meaningful...", max);
    double val=0;
    for (int i=0; i<max; i++) {
        for (int j=0; j<max; j++) {
            val = sin(0.12345);
        }
        seq.next(true);
    }
    Q_UNUSED(val);
}

// --------------------------------------

QRecursiveMutex PythonThread::mutex;

PythonThread::PythonThread(QObject* parent)
  : QThread(parent)
{
}

PythonThread::~PythonThread()
{
}

void PythonThread::run()
{
    QMutexLocker mutex_lock(&mutex);
    Base::PyGILStateLocker locker;
    try {
#if 0
        PyObject *module, *dict;
        module = PyImport_AddModule("__main__");
        dict = PyModule_GetDict(module);
        PyObject* dict_copy = PyDict_Copy(dict);

        std::string buf;
        buf = std::string(
            "import Sandbox\n"
            "doc=App.ActiveDocument\n"
            "dp=Sandbox.DocumentProtector(doc)\n"
            "dp.addObject(\"Mesh::Ellipsoid\",\"Mesh\")\n"
            "dp.recompute()\n");
        PyObject *presult = PyRun_String(buf.c_str(), Py_file_input, dict_copy, dict_copy);
        Py_DECREF(dict_copy);
        msleep(10);
#else

        Base::Interpreter().runString(
            "import Sandbox, Mesh, MeshGui\n"
            "doc=App.ActiveDocument\n"
            "dp=Sandbox.DocumentProtector(doc)\n"
            "dp.addObject(\"Mesh::Ellipsoid\",\"Mesh\")\n"
            "dp.recompute()\n");
        msleep(10);
#endif
    }
    catch (const Base::PyException& e) {
        Base::Console().Error(e.what());
    }
}

// --------------------------------------

MeshLoaderThread::MeshLoaderThread(const QString& fn, QObject* parent)
  : QThread(parent), filename(fn)
{
}

MeshLoaderThread::~MeshLoaderThread()
{
}

Base::Reference<Mesh::MeshObject> MeshLoaderThread::getMesh() const
{
    return this->mesh;
}

void MeshLoaderThread::run()
{
    this->mesh = new Mesh::MeshObject();
    this->mesh->load((const char*)filename.toUtf8());
}

// --------------------------------------

PROPERTY_SOURCE(Sandbox::SandboxObject, App::DocumentObject)

SandboxObject::SandboxObject()
{
    ADD_PROPERTY(Integer,(4711)  );
}

SandboxObject::~SandboxObject()
{
}

short SandboxObject::mustExecute(void) const
{
    if (Integer.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *SandboxObject::execute(void)
{
    Base::Console().Message("SandboxObject::execute()\n");
    return 0;
}

void SandboxObject::onChanged(const App::Property* prop)
{
    if (prop == &Integer)
        Base::Console().Message("SandboxObject::onChanged(%d)\n", Integer.getValue());
    App::DocumentObject::onChanged(prop);
}

void SandboxObject::setIntValue(int v)
{
    Base::Console().Message("SandboxObject::setIntValue(%d)\n", v);
    Integer.setValue(v);
}

void SandboxObject::resetValue()
{
    Base::Console().Message("SandboxObject::resetValue()\n");
    Integer.setValue(4711);
}

DocumentTestThread::DocumentTestThread(QObject* parent)
  : QThread(parent)
{
}

DocumentTestThread::~DocumentTestThread()
{
}

void DocumentTestThread::run()
{
    Base::Console().Message("DocumentTestThread::run()\n");
    App::Document* doc = App::GetApplication().getActiveDocument();
    DocumentProtector dp(doc);
    SandboxObject* obj = static_cast<SandboxObject*>(dp.addObject("Sandbox::SandboxObject"));

    DocumentObjectProtector op(obj);
    App::PropertyString Name;Name.setValue("MyLabel");
    op.setProperty("Label", Name);

    App::PropertyInteger Int;Int.setValue(2);
    op.setProperty("Integer", Int);
    op.execute(CallableWithArgs<SandboxObject,int,&SandboxObject::setIntValue>(obj,4));

    dp.recompute();
    op.execute(Callable<SandboxObject,&SandboxObject::resetValue>(obj));
}


DocumentSaverThread::DocumentSaverThread(App::Document* doc, QObject* parent)
  : QThread(parent), doc(doc)
{
}

DocumentSaverThread::~DocumentSaverThread()
{
}

void DocumentSaverThread::run()
{
    std::string uuid = Base::Uuid::createUuid();
    std::string fn = doc->TransientDir.getValue();
    fn += "/";
    fn += uuid;
    fn += ".autosave";
    Base::FileInfo tmp(fn);

    // open extra scope to close ZipWriter properly
    {
        Base::ofstream file(tmp, std::ios::out | std::ios::binary);
        Base::ZipWriter writer(file);

        writer.setComment("FreeCAD Document");
        writer.setLevel(0);
        writer.putNextEntry("Document.xml");

        doc->Save(writer);

        // Special handling for Gui document.
        doc->signalSaveDocument(writer);

        // write additional files
        writer.writeFiles();
    }
}
