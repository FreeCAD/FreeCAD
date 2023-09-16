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
# ifdef FC_OS_WIN32
# define WIN32_LEAN_AND_MEAN
# define NOMINMAX
# include <windows.h>
# endif
# include <QApplication>
# include <QCalendarWidget>
# include <QColorDialog>
# include <QCryptographicHash>
# include <QObject>
# include <QEventLoop>
# include <QFontMetrics>
# include <QFuture>
# include <QFutureWatcher>
# include <QtConcurrentMap>
# include <QLabel>
# include <QInputDialog>
# include <QMessageBox>
# include <QTimer>
# include <QImage>
# include <QImageReader>
# include <QPainter>
# include <QPainterPath>
# include <QThread>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoImage.h>
# include <Inventor/nodes/SoCone.h>
# include <cmath>
# include <boost/thread/thread.hpp>
# include <boost/thread/mutex.hpp>
# include <boost/thread/condition_variable.hpp>
# include <boost/thread/future.hpp>
# include <boost/bind/bind.hpp>
# include <memory>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Tools.h>
#include <Base/Console.h>
#include <Base/Sequencer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/WaitCursor.h>

#include <Mod/Sandbox/App/DocumentThread.h>
#include <Mod/Sandbox/App/DocumentProtector.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/Degeneration.h>
#include "Workbench.h"
#include "GLGraphicsView.h"
#include "TaskPanelView.h"

namespace bp = boost::placeholders;

DEF_STD_CMD(CmdSandboxDocumentThread)

CmdSandboxDocumentThread::CmdSandboxDocumentThread()
  :Command("Sandbox_Thread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Run several threads");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_Thread";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool1";
}

void CmdSandboxDocumentThread::activated(int)
{
    App::GetApplication().newDocument("Thread");
    for (int i=0; i<5; i++) {
        Sandbox::DocumentThread* dt = new Sandbox::DocumentThread();
        dt->setObjectName(QString::fromLatin1("MyMesh_%1").arg(i));
        QObject::connect(dt, SIGNAL(finished()), dt, SLOT(deleteLater()));
        dt->start();
    }
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxDocumentTestThread)

CmdSandboxDocumentTestThread::CmdSandboxDocumentTestThread()
  :Command("Sandbox_TestThread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Test thread");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_TestThread";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool1";
}

void CmdSandboxDocumentTestThread::activated(int)
{
    App::GetApplication().newDocument("Thread");
    Sandbox::DocumentTestThread* dt = new Sandbox::DocumentTestThread();
    QObject::connect(dt, SIGNAL(finished()), dt, SLOT(deleteLater()));
    dt->start();
}

// -------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdSandboxDocumentSaveThread)

CmdSandboxDocumentSaveThread::CmdSandboxDocumentSaveThread()
  :Command("Sandbox_SaveThread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Save thread");
    sToolTipText  = QT_TR_NOOP("Sandbox save function");
    sWhatsThis    = "Sandbox_SaveThread";
    sStatusTip    = QT_TR_NOOP("Sandbox save function");
}

void CmdSandboxDocumentSaveThread::activated(int)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    Sandbox::DocumentSaverThread* dt = new Sandbox::DocumentSaverThread(doc);
    QObject::connect(dt, SIGNAL(finished()), dt, SLOT(deleteLater()));
    dt->start();
}

bool CmdSandboxDocumentSaveThread::isActive()
{
    return App::GetApplication().getActiveDocument() != 0;
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxDocThreadWithSeq)

CmdSandboxDocThreadWithSeq::CmdSandboxDocThreadWithSeq()
  :Command("Sandbox_SeqThread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Thread and sequencer");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_SeqThread";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool2";
}

void CmdSandboxDocThreadWithSeq::activated(int)
{
    App::GetApplication().newDocument("Thread");
    Sandbox::DocumentThread* dt = new Sandbox::DocumentThread();
    dt->setObjectName(QString::fromLatin1("MyMesh"));
    QObject::connect(dt, SIGNAL(finished()), dt, SLOT(deleteLater()));
    dt->start();
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
    (void)val;
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxDocThreadBusy)

CmdSandboxDocThreadBusy::CmdSandboxDocThreadBusy()
  :Command("Sandbox_BlockThread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Thread and no sequencer");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_BlockThread";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool3";
}

void CmdSandboxDocThreadBusy::activated(int)
{
    App::GetApplication().newDocument("Thread");
    Sandbox::DocumentThread* dt = new Sandbox::DocumentThread();
    dt->setObjectName(QString::fromLatin1("MyMesh"));
    QObject::connect(dt, SIGNAL(finished()), dt, SLOT(deleteLater()));
    dt->start();
#ifdef FC_DEBUG
    int max = 10000;
#else
    int max = 100000000;
#endif
    double val=0;
    for (int i=0; i<max; i++) {
        for (int j=0; j<max; j++) {
            val = sin(0.12345);
        }
    }
    (void)val;
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxDocumentNoThread)

CmdSandboxDocumentNoThread::CmdSandboxDocumentNoThread()
  :Command("Sandbox_NoThread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("GUI thread");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_NoThread";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool4";
}

void CmdSandboxDocumentNoThread::activated(int)
{
    App::GetApplication().newDocument("Thread");
    App::Document* doc = App::GetApplication().getActiveDocument();
    Sandbox::DocumentProtector dp(doc);
    App::DocumentObject* obj = dp.addObject("Mesh::Cube", "MyCube");
    (void)obj;
    dp.recompute();
    App::GetApplication().closeDocument("Thread");
    // this forces an exception
    App::DocumentObject* obj2 = dp.addObject("Mesh::Cube", "MyCube");
    (void)obj2;
    dp.recompute();
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxWorkerThread)

CmdSandboxWorkerThread::CmdSandboxWorkerThread()
  :Command("Sandbox_WorkerThread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Worker thread");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_WorkerThread";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool1";
}

void CmdSandboxWorkerThread::activated(int)
{
    Sandbox::WorkerThread* wt = new Sandbox::WorkerThread();
    QObject::connect(wt, SIGNAL(finished()), wt, SLOT(deleteLater()));
    wt->start();
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxPythonLockThread)

CmdSandboxPythonLockThread::CmdSandboxPythonLockThread()
  :Command("Sandbox_PythonLockThread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Locked Python threads");
    sToolTipText  = QT_TR_NOOP("Use Python's thread module where each thread is locked");
    sWhatsThis    = "Sandbox_PythonLockThread";
    sStatusTip    = QT_TR_NOOP("Use Python's thread module where each thread is locked");
}

void CmdSandboxPythonLockThread::activated(int)
{
    doCommand(Doc,
        "import thread, time, Sandbox\n"
        "def adder(doc):\n"
        "    lock.acquire()\n"
        "    dp=Sandbox.DocumentProtector(doc)\n"
        "    dp.addObject(\"Mesh::Ellipsoid\",\"Mesh\")\n"
        "    dp.recompute()\n"
        "    lock.release()\n"
        "\n"
        "lock=thread.allocate_lock()\n"
        "doc=App.newDocument()\n"
        "for i in range(2):\n"
        "    thread.start_new(adder,(doc,))\n"
        "\n"
        "time.sleep(1)\n"
    );
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxPythonNolockThread)

CmdSandboxPythonNolockThread::CmdSandboxPythonNolockThread()
  :Command("Sandbox_NolockPython")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Unlocked Python threads");
    sToolTipText  = QT_TR_NOOP("Use Python's thread module where each thread is unlocked");
    sWhatsThis    = "Sandbox_NolockPython";
    sStatusTip    = QT_TR_NOOP("Use Python's thread module where each thread is unlocked");
}

void CmdSandboxPythonNolockThread::activated(int)
{
    doCommand(Doc,
        "import thread, time, Sandbox\n"
        "def adder(doc):\n"
        "    dp=Sandbox.DocumentProtector(doc)\n"
        "    dp.addObject(\"Mesh::Ellipsoid\",\"Mesh\")\n"
        "    dp.recompute()\n"
        "\n"
        "doc=App.newDocument()\n"
        "for i in range(2):\n"
        "    thread.start_new(adder,(doc,))\n"
        "\n"
        "time.sleep(1)\n"
    );
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxPySideThread)

CmdSandboxPySideThread::CmdSandboxPySideThread()
  :Command("Sandbox_PySideThread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("PySide threads");
    sToolTipText  = QT_TR_NOOP("Use PySide's thread module");
    sWhatsThis    = "Sandbox_PySideThread";
    sStatusTip    = QT_TR_NOOP("Use PySide's thread module");
}

void CmdSandboxPySideThread::activated(int)
{
    doCommand(Doc,
        "from PySide import QtCore; import Sandbox\n"
        "class Thread(QtCore.QThread):\n"
        "    def run(self):\n"
        "        dp=Sandbox.DocumentProtector(doc)\n"
        "        dp.addObject(\"Mesh::Ellipsoid\",\"Mesh\")\n"
        "        dp.recompute()\n"
        "\n"
        "doc=App.newDocument()\n"
        "threads=[]\n"
        "for i in range(2):\n"
        "    thread=Thread()\n"
        "    threads.append(thread)\n"
        "    thread.start()\n"
        "\n"
    );
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxPythonThread)

CmdSandboxPythonThread::CmdSandboxPythonThread()
  :Command("Sandbox_PythonThread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Python threads");
    sToolTipText  = QT_TR_NOOP("Use class PythonThread running Python code in its run() method");
    sWhatsThis    = "Sandbox_PythonThread";
    sStatusTip    = QT_TR_NOOP("Use class PythonThread running Python code in its run() method");
}

void CmdSandboxPythonThread::activated(int)
{
    App::GetApplication().newDocument("Thread");
    for (int i=0; i<5; i++) {
        Sandbox::PythonThread* pt = new Sandbox::PythonThread();
        pt->setObjectName(QString::fromLatin1("MyMesh_%1").arg(i));
        QObject::connect(pt, SIGNAL(finished()), pt, SLOT(deleteLater()));
        pt->start();
    }
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxPythonMainThread)

CmdSandboxPythonMainThread::CmdSandboxPythonMainThread()
  :Command("Sandbox_PythonMainThread")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Python main thread");
    sToolTipText  = QT_TR_NOOP("Run python code in main thread");
    sWhatsThis    = "Sandbox_PythonMainThread";
    sStatusTip    = QT_TR_NOOP("Run python code in main thread");
}

void CmdSandboxPythonMainThread::activated(int)
{
    doCommand(Doc,
        "import Sandbox\n"
        "doc=App.newDocument()\n"
        "dp=Sandbox.DocumentProtector(doc)\n"
        "dp.addObject(\"Mesh::Ellipsoid\",\"Mesh\")\n"
        "dp.recompute()\n"
    );
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxDocThreadWithDialog)

CmdSandboxDocThreadWithDialog::CmdSandboxDocThreadWithDialog()
  :Command("Sandbox_Dialog")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Thread and modal dialog");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_Dialog";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool7";
}

void CmdSandboxDocThreadWithDialog::activated(int)
{
    App::GetApplication().newDocument("Thread");
    Sandbox::DocumentThread* dt = new Sandbox::DocumentThread();
    dt->setObjectName(QString::fromLatin1("MyMesh"));
    QObject::connect(dt, SIGNAL(finished()), dt, SLOT(deleteLater()));
    dt->start();
    //QFileDialog::getOpenFileName();
    QColorDialog::getColor(Qt::white,Gui::getMainWindow());
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdSandboxDocThreadWithFileDlg)

CmdSandboxDocThreadWithFileDlg::CmdSandboxDocThreadWithFileDlg()
  :Command("Sandbox_FileDialog")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Thread and file dialog");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_FileDialog";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool7";
}

void CmdSandboxDocThreadWithFileDlg::activated(int)
{
    App::GetApplication().newDocument("Thread");
    Sandbox::DocumentThread* dt = new Sandbox::DocumentThread();
    dt->setObjectName(QString::fromLatin1("MyMesh"));
    QObject::connect(dt, SIGNAL(finished()), dt, SLOT(deleteLater()));
    dt->start();
    QFileDialog::getOpenFileName();
}

// -------------------------------------------------------------------------------

class CmdSandboxEventLoop : public Gui::Command
{
public:
    CmdSandboxEventLoop();
    const char* className() const
    { return "CmdSandboxEventLoop"; }
protected:
    void activated(int iMsg);
    bool isActive(void);
private:
    QEventLoop loop;
};


CmdSandboxEventLoop::CmdSandboxEventLoop()
  :Command("Sandbox_EventLoop")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Local event loop");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_EventLoop";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool6";
}

void CmdSandboxEventLoop::activated(int)
{
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    timer.start(5000); // 5s timeout
    loop.exec();
    Base::Console().Message("CmdSandboxEventLoop: timeout\n");
}

bool CmdSandboxEventLoop::isActive(void)
{
    return (!loop.isRunning());
}

// -------------------------------------------------------------------------------

class CmdSandboxMeshLoader : public Gui::Command
{
public:
    CmdSandboxMeshLoader();
    const char* className() const
    { return "CmdSandboxMeshLoader"; }
protected:
    void activated(int iMsg);
    bool isActive(void);
private:
    QEventLoop loop;
};

CmdSandboxMeshLoader::CmdSandboxMeshLoader()
  :Command("Sandbox_MeshLoad")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Load mesh in thread");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_MeshLoad";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool6";
}

void CmdSandboxMeshLoader::activated(int)
{
    // use current path as default
    QStringList filter;
    filter << QObject::tr("All Mesh Files (*.stl *.ast *.bms *.obj)");
    filter << QObject::tr("Binary STL (*.stl)");
    filter << QObject::tr("ASCII STL (*.ast)");
    filter << QObject::tr("Binary Mesh (*.bms)");
    filter << QObject::tr("Alias Mesh (*.obj)");
    filter << QObject::tr("Inventor V2.1 ascii (*.iv)");
    //filter << "Nastran (*.nas *.bdf)";
    filter << QObject::tr("All Files (*.*)");

    // Allow multi selection
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
        QObject::tr("Import mesh"), QString(), filter.join(QLatin1String(";;")));

    Sandbox::MeshLoaderThread thread(fn);
    QObject::connect(&thread, SIGNAL(finished()), &loop, SLOT(quit()));

    thread.start();
    loop.exec();

    Base::Reference<Mesh::MeshObject> data = thread.getMesh();
    App::Document* doc = App::GetApplication().getActiveDocument();
    Mesh::Feature* mesh = static_cast<Mesh::Feature*>(doc->addObject("Mesh::Feature","Mesh"));
    mesh->Mesh.setValuePtr((Mesh::MeshObject*)data);
    mesh->purgeTouched();
}

bool CmdSandboxMeshLoader::isActive(void)
{
    return (hasActiveDocument() && !loop.isRunning());
}

// -------------------------------------------------------------------------------

Base::Reference<Mesh::MeshObject> loadMesh(const QString& s)
{
    Mesh::MeshObject* mesh = new Mesh::MeshObject();
    mesh->load((const char*)s.toUtf8());
    return mesh;
}

DEF_STD_CMD_A(CmdSandboxMeshLoaderBoost)

CmdSandboxMeshLoaderBoost::CmdSandboxMeshLoaderBoost()
  :Command("Sandbox_MeshLoaderBoost")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Load mesh in boost-thread");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_MeshLoaderBoost";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool6";
}

void CmdSandboxMeshLoaderBoost::activated(int)
{
    // use current path as default
    QStringList filter;
    filter << QObject::tr("All Mesh Files (*.stl *.ast *.bms *.obj)");
    filter << QObject::tr("Binary STL (*.stl)");
    filter << QObject::tr("ASCII STL (*.ast)");
    filter << QObject::tr("Binary Mesh (*.bms)");
    filter << QObject::tr("Alias Mesh (*.obj)");
    filter << QObject::tr("Inventor V2.1 ascii (*.iv)");
    //filter << "Nastran (*.nas *.bdf)";
    filter << QObject::tr("All Files (*.*)");

    // Allow multi selection
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
        QObject::tr("Import mesh"), QString(), filter.join(QLatin1String(";;")));

    boost::packaged_task< Base::Reference<Mesh::MeshObject> > pt
        (boost::bind(&loadMesh, fn));
    boost::unique_future< Base::Reference<Mesh::MeshObject> > fi=pt.get_future();
    boost::thread task(boost::move(pt)); // launch task on a thread
    fi.wait(); // wait for it to be finished

    App::Document* doc = App::GetApplication().getActiveDocument();
    Mesh::Feature* mesh = static_cast<Mesh::Feature*>(doc->addObject("Mesh::Feature","Mesh"));
    mesh->Mesh.setValuePtr((Mesh::MeshObject*)fi.get());
    mesh->purgeTouched();
}

bool CmdSandboxMeshLoaderBoost::isActive(void)
{
    return hasActiveDocument();
}

DEF_STD_CMD_A(CmdSandboxMeshLoaderFuture)

CmdSandboxMeshLoaderFuture::CmdSandboxMeshLoaderFuture()
  :Command("Sandbox_MeshLoaderFuture")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Load mesh in QFuture");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_MeshLoaderFuture";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool6";
}

void CmdSandboxMeshLoaderFuture::activated(int)
{
    // use current path as default
    QStringList filter;
    filter << QObject::tr("All Mesh Files (*.stl *.ast *.bms *.obj)");
    filter << QObject::tr("Binary STL (*.stl)");
    filter << QObject::tr("ASCII STL (*.ast)");
    filter << QObject::tr("Binary Mesh (*.bms)");
    filter << QObject::tr("Alias Mesh (*.obj)");
    filter << QObject::tr("Inventor V2.1 ascii (*.iv)");
    //filter << "Nastran (*.nas *.bdf)";
    filter << QObject::tr("All Files (*.*)");

    // Allow multi selection
    QStringList fn = Gui::FileDialog::getOpenFileNames(Gui::getMainWindow(),
        QObject::tr("Import mesh"), QString(), filter.join(QLatin1String(";;")));

    QFuture< Base::Reference<Mesh::MeshObject> > future = QtConcurrent::mapped
        (fn, loadMesh);

    QFutureWatcher< Base::Reference<Mesh::MeshObject> > watcher;
    watcher.setFuture(future);

    // keep it responsive during computation
    QEventLoop loop;
    QObject::connect(&watcher, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    App::Document* doc = App::GetApplication().getActiveDocument();
    for (QFuture< Base::Reference<Mesh::MeshObject> >::const_iterator it = future.begin(); it != future.end(); ++it) {
        Mesh::Feature* mesh = static_cast<Mesh::Feature*>(doc->addObject("Mesh::Feature","Mesh"));
        mesh->Mesh.setValuePtr((Mesh::MeshObject*)(*it));
        mesh->purgeTouched();
    }
}

bool CmdSandboxMeshLoaderFuture::isActive(void)
{
    return hasActiveDocument();
}

namespace Mesh {
typedef Base::Reference<const MeshObject> MeshObjectConstRef;
typedef std::list<MeshObjectConstRef> MeshObjectConstRefList;
typedef std::vector<MeshObjectConstRef> MeshObjectConstRefArray;
}

struct MeshObject_greater  : public std::binary_function<const Mesh::MeshObjectConstRef&,
                                                         const Mesh::MeshObjectConstRef&, bool>
{
    bool operator()(const Mesh::MeshObjectConstRef& x,
                    const Mesh::MeshObjectConstRef& y) const
    {
        return x->countFacets() > y->countFacets();
    }
};

class MeshTestJob
{

public:
    MeshTestJob()
    {
    }
    ~MeshTestJob()
    {
    }

    Mesh::MeshObject* run(const std::vector<Mesh::MeshObjectConstRef>& meshdata)
    {
        std::vector<Mesh::MeshObjectConstRef> meshes = meshdata;
        if (meshes.empty())
            return 0; // nothing todo
        Mesh::MeshObjectConstRef myMesh = 0;
        std::sort(meshes.begin(), meshes.end(), MeshObject_greater());
        myMesh = meshes.front();

        if (meshes.size() > 1) {
            MeshCore::MeshKernel kernel;

            // copy the data of the first mesh, this will be the new model then
            kernel = myMesh->getKernel();
            for (std::vector<Mesh::MeshObjectConstRef>::iterator it = meshes.begin(); it != meshes.end(); ++it) {
                if (*it != myMesh) {
                    Base::Console().Message("MeshTestJob::run() in thread: %p\n", QThread::currentThreadId());
                }
            }

            // avoid to copy the data
            Mesh::MeshObject* mesh = new Mesh::MeshObject();
            mesh->swap(kernel);
            return mesh;
        }
        else {
            Mesh::MeshObject* mesh = new Mesh::MeshObject();
            mesh->setKernel(myMesh->getKernel());
            return mesh;
        }
    }
};

DEF_STD_CMD_A(CmdSandboxMeshTestJob)

CmdSandboxMeshTestJob::CmdSandboxMeshTestJob()
  : Command("Sandbox_MeshTestJob")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Test mesh job");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_MeshTestJob";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
    sPixmap       = "Std_Tool7";
}

void CmdSandboxMeshTestJob::activated(int)
{
    Mesh::MeshObjectConstRefList meshes;
    App::Document* app_doc = App::GetApplication().getActiveDocument();
    std::vector<Mesh::Feature*> meshObj = Gui::Selection().getObjectsOfType<Mesh::Feature>(app_doc->getName());
    for (std::vector<Mesh::Feature*>::iterator it = meshObj.begin(); it != meshObj.end(); ++it) {
        meshes.push_back((*it)->Mesh.getValuePtr());
    }

    int iteration = 1;
    while (meshes.size() > 1) {
        int numJobs = QThread::idealThreadCount();
        if (numJobs < 0) numJobs = 2;

        while (numJobs > (int)(meshes.size()+1)/2)
            numJobs /= 2;
        numJobs = std::max<int>(1, numJobs);

        // divide all meshes we have into several groups
        std::vector<Mesh::MeshObjectConstRefArray> mesh_groups;
        while (numJobs > 0) {
            int size = (int)meshes.size();
            int count = size / numJobs;
            --numJobs;
            Mesh::MeshObjectConstRefArray meshes_per_job;
            for (int i=0; i<count; i++) {
                meshes_per_job.push_back(meshes.front());
                meshes.pop_front();
            }
            mesh_groups.push_back(meshes_per_job);
        }

        // run the actual multi-threaded mesh test
        Base::Console().Message("Mesh test (step %d)...\n",iteration++);
        MeshTestJob meshJob;
        QFuture<Mesh::MeshObject*> mesh_future = QtConcurrent::mapped
            (mesh_groups, boost::bind(&MeshTestJob::run, &meshJob, bp::_1));

        // keep it responsive during computation
        QFutureWatcher<Mesh::MeshObject*> mesh_watcher;
        mesh_watcher.setFuture(mesh_future);
        QEventLoop loop;
        QObject::connect(&mesh_watcher, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        for (QFuture<Mesh::MeshObject*>::const_iterator it = mesh_future.begin(); it != mesh_future.end(); ++it) {
            meshes.push_back(Mesh::MeshObjectConstRef(*it));
        }
    }

    if (meshes.empty()) {
        Base::Console().Error("The mesh test failed to create a valid mesh.\n");
        return;
    }
}

bool CmdSandboxMeshTestJob::isActive(void)
{
    return hasActiveDocument();
}

class MeshThread : public QThread
{
public:
    MeshThread(Base::Reference<Mesh::MeshObject> m, QObject* p=0)
        : QThread(p), mesh(m)
    {}

protected:
    void run()
    {
        for (int i=0; i<100;i++) {
            Base::Reference<Mesh::MeshObject> new_ref;
            new_ref = mesh;
        }
    }

private:
    Base::Reference<Mesh::MeshObject> mesh;
};

DEF_STD_CMD_A(CmdSandboxMeshTestRef)

CmdSandboxMeshTestRef::CmdSandboxMeshTestRef()
  : Command("Sandbox_MeshTestRef")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Test mesh reference");
    sToolTipText  = QT_TR_NOOP("Sandbox Test function");
    sWhatsThis    = "Sandbox_MeshTestRef";
    sStatusTip    = QT_TR_NOOP("Sandbox Test function");
}

void CmdSandboxMeshTestRef::activated(int)
{
    Gui::WaitCursor wc;
    std::vector< std::shared_ptr<QThread> > threads;
    Base::Reference<Mesh::MeshObject> mesh(new Mesh::MeshObject);
    int num = mesh.getRefCount();

    for (int i=0; i<10; i++) {
        std::shared_ptr<QThread> trd(new MeshThread(mesh));
        trd->start();
        threads.push_back(trd);
    }

    QTimer timer;
    QEventLoop loop;
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    timer.start(2000);
    loop.exec();
    threads.clear();

    Mesh::MeshObject* ptr = (Mesh::MeshObject*)mesh;
    if (!ptr)
        Base::Console().Error("Object deleted\n");
    if (num != mesh.getRefCount())
        Base::Console().Error("Reference count is %d\n",mesh.getRefCount());
}

bool CmdSandboxMeshTestRef::isActive(void)
{
    return true;
}

//===========================================================================
// Std_GrabWidget
//===========================================================================
DEF_STD_CMD_A(CmdTestGrabWidget)

CmdTestGrabWidget::CmdTestGrabWidget()
  : Command("Std_GrabWidget")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Grab widget";
    sToolTipText    = "Grab widget";
    sWhatsThis      = "Std_GrabWidget";
    sStatusTip      = sToolTipText;
}

void CmdTestGrabWidget::activated(int)
{
    QCalendarWidget* c = new QCalendarWidget();
    c->hide();
    QPixmap p = c->grab(c->rect());
    QLabel* label = new QLabel();
    label->resize(c->size());
    label->setPixmap(p);
    label->show();
    delete c;
}

bool CmdTestGrabWidget::isActive(void)
{
    return true;
}

//===========================================================================
// Std_ImageNode
//===========================================================================
DEF_3DV_CMD(CmdTestImageNode)

class RenderArea : public QWidget
{
private:
    QPainterPath path;
    int penWidth;
    QColor penColor;

public:
    RenderArea(const QPainterPath &path, QWidget *parent=0)
      : QWidget(parent), path(path), penColor(0,0,127)
    {
        penWidth = 2;
        setBackgroundRole(QPalette::Base);
    }

    QSize minimumSizeHint() const
    {
        return QSize(50, 50);
    }

    QSize sizeHint() const
    {
        return QSize(100, 30);
    }

    void setFillRule(Qt::FillRule rule)
    {
        path.setFillRule(rule);
        update();
    }

    void setPenWidth(int width)
    {
        penWidth = width;
        update();
    }

    void setPenColor(const QColor &color)
    {
        penColor = color;
        update();
    }

    void paintEvent(QPaintEvent *)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.scale(width() / 100.0, height() / 100.0);
        painter.translate(50.0, 50.0);
        painter.translate(-50.0, -50.0);

        painter.setPen(QPen(penColor, penWidth, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
        painter.setBrush(QBrush(QColor(0,85,255), Qt::SolidPattern));
        painter.drawPath(path);
        painter.setPen(Qt::white);
        painter.drawText(25, 40, 70, 20, Qt::AlignHCenter|Qt::AlignVCenter,
            QString::fromLatin1("Distance: 2.784mm"));
    }
};

#ifdef Q_OS_WIN32
class GDIWidget : public QWidget
{
public:
    GDIWidget(QWidget* parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NativeWindow);
    }
    QPaintEngine *paintEngine() const {
        return 0;
    }
protected:
    void paintEvent(QPaintEvent *event) {

        HWND hWnd = (HWND)this->winId();
        HDC hdc = GetDC(hWnd);
        SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW));
        Rectangle(hdc, 0, 0, width(), height());
        RECT rect = {0, 0, width(), height() };
        DrawTextA(hdc, "Hello World!", 12, &rect,
        DT_SINGLELINE | DT_VCENTER | DT_CENTER);
        ReleaseDC(hWnd, hdc);
     }
};
#endif

CmdTestImageNode::CmdTestImageNode()
  : Command("Std_ImageNode")
{
    sGroup          = "Standard-Test";
    sMenuText       = "SoImage node";
    sToolTipText    = "SoImage node";
    sWhatsThis      = "Std_ImageNode";
    sStatusTip      = sToolTipText;
}

void CmdTestImageNode::activated(int)
{
    QString text = QString::fromLatin1("Distance: 2.7jgiorjgor84mm");
    QFont font;
    QFontMetrics fm(font);
    int w = Gui::QtTools::horizontalAdvance(fm, text);
    int h = fm.height();


    QPainterPath roundRectPath;

    roundRectPath.moveTo(100.0, 5.0);
    roundRectPath.arcTo(90.0, 0.0, 10.0, 10.0, 0.0, 90.0);
    roundRectPath.lineTo(5.0, 0.0);
    roundRectPath.arcTo(0.0, 0.0, 10.0, 10.0, 90.0, 90.0);
    roundRectPath.lineTo(0.0, 95.0);
    roundRectPath.arcTo(0.0, 90.0, 10.0, 10.0, 180.0, 90.0);
    roundRectPath.lineTo(95.0, 100.0);
    roundRectPath.arcTo(90.0, 90.0, 10.0, 10.0, 270.0, 90.0);
    roundRectPath.closeSubpath();


    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
    SoImage* node = new SoImage();

    QImage image(w+10,h+10,QImage::Format_ARGB32_Premultiplied);// = p.toImage();
    image.fill(0x00000000);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(QColor(0,0,127), 2, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    painter.setBrush(QBrush(QColor(0,85,255), Qt::SolidPattern));
    QRectF rectangle(0.0, 0.0, w+10, h+10);
    painter.drawRoundedRect(rectangle, 5, 5);

    painter.setPen(QColor(255,255,255));
    painter.drawText(5,h+3, text);
    painter.end();

    SoSFImage texture;
    Gui::BitmapFactory().convert(image, texture);
    node->image = texture;
    SoAnnotation* anno = new SoAnnotation();
    anno->addChild(node);
    static_cast<SoGroup*>(viewer->getSceneGraph())->addChild(anno);
}

//===========================================================================
// Sandbox_GDIWidget
//===========================================================================
DEF_STD_CMD(CmdTestGDIWidget)

CmdTestGDIWidget::CmdTestGDIWidget()
  : Command("Sandbox_GDIWidget")
{
    sGroup          = "Standard-Test";
    sMenuText       = "GDI widget";
    sToolTipText    = "GDI widget";
    sWhatsThis      = "Sandbox_GDIWidget";
    sStatusTip      = sToolTipText;
}

void CmdTestGDIWidget::activated(int)
{
#ifdef Q_OS_WIN32
    GDIWidget* gdi = new GDIWidget(Gui::getMainWindow());
    gdi->show();
    gdi->resize(200,200);
    gdi->move(400,400);
    gdi->setAttribute(Qt::WA_DeleteOnClose);
#endif
}

//===========================================================================
// Sandbox_RedirectPaint
//===========================================================================
DEF_STD_CMD(CmdTestRedirectPaint)

CmdTestRedirectPaint::CmdTestRedirectPaint()
  : Command("Sandbox_RedirectPaint")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Redirect paint";
    sToolTipText    = "Redirect paint";
    sWhatsThis      = "Sandbox_RedirectPaint";
    sStatusTip      = sToolTipText;
}

void CmdTestRedirectPaint::activated(int)
{
    QCalendarWidget* cal = new QCalendarWidget();
    cal->setWindowTitle(QString::fromLatin1("QCalendarWidget"));
    cal->show();
    QPixmap img(cal->size());
    cal->render(&img);

    QLabel* label = new QLabel();
    label->setPixmap(img);
    label->show();
    label->setWindowTitle(QString::fromLatin1("QLabel"));
}

//===========================================================================
// Sandbox_CryptographicHash
//===========================================================================
DEF_STD_CMD(CmdTestCryptographicHash)

CmdTestCryptographicHash::CmdTestCryptographicHash()
  : Command("Sandbox_CryptographicHash")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Cryptographic Hash";
    sToolTipText    = "Cryptographic Hash";
    sWhatsThis      = "Sandbox_CryptographicHash";
    sStatusTip      = sToolTipText;
}

void CmdTestCryptographicHash::activated(int)
{
    QByteArray data = "FreeCAD";
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    QMessageBox::information(0,QLatin1String("Hash of: FreeCAD"),QString::fromLatin1(hash));
}

//===========================================================================
// Sandbox_WidgetShape
//===========================================================================
DEF_3DV_CMD(CmdTestWidgetShape)

CmdTestWidgetShape::CmdTestWidgetShape()
  : Command("Sandbox_WidgetShape")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Widget shape";
    sToolTipText    = "Widget shape";
    sWhatsThis      = "Sandbox_WidgetShape";
    sStatusTip      = sToolTipText;
}

void CmdTestWidgetShape::activated(int)
{
    SandboxGui::SoWidgetShape* shape = new SandboxGui::SoWidgetShape;
    shape->setWidget(new QCalendarWidget());
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
    static_cast<SoGroup*>(viewer->getSceneGraph())->addChild(shape);
}

// -------------------------------------------------------------------------------

DEF_STD_CMD(CmdMengerSponge)

CmdMengerSponge::CmdMengerSponge()
  :Command("Sandbox_MengerSponge")
{
    sAppModule    = "Sandbox";
    sGroup        = QT_TR_NOOP("Sandbox");
    sMenuText     = QT_TR_NOOP("Menger sponge");
    sToolTipText  = QT_TR_NOOP("Menger sponge");
    sWhatsThis    = "Sandbox_MengerSponge";
    sStatusTip    = QT_TR_NOOP("Menger sponge");
}

struct Param {
    int level;
    float x,y,z;
    Param(int l, float x, float y, float z)
        : level(l), x(x), y(y), z(z)
    {
    }
};

typedef Base::Reference<Mesh::MeshObject> MeshObjectRef;

MeshObjectRef globalBox;

// Create a Box and Place it a coords (x,y,z)
MeshObjectRef PlaceBox(float x, float y, float z)
{
    MeshObjectRef mesh = new Mesh::MeshObject(*globalBox);
    Base::Matrix4D m;
    m.move(x,y,z);
    mesh->getKernel().Transform(m);
    return mesh;
}

MeshObjectRef Sierpinski(int level, float x0, float y0, float z0)
{
    float boxnums = std::pow(3.0f,level);
    float thirds = boxnums / 3;
    float twothirds = thirds * 2;

    QList<float> rangerx, rangery, rangerz;
    if (level == 0) {
        rangerx << x0;
        rangery << y0;
        rangerz << z0;
    }
    else {
        rangerx << x0 << (x0 + thirds) << (x0 + twothirds);
        rangery << y0 << (y0 + thirds) << (y0 + twothirds);
        rangerz << z0 << (z0 + thirds) << (z0 + twothirds);
    }

    int block = 1;
    QList<int> skip; skip << 5 << 11 << 13 << 14 << 15 << 17 << 23;
    MeshObjectRef mesh = new Mesh::MeshObject();

    for (QList<float>::iterator i = rangerx.begin(); i != rangerx.end(); ++i) {
        for (QList<float>::iterator j = rangery.begin(); j != rangery.end(); ++j) {
            for (QList<float>::iterator k = rangerz.begin(); k != rangerz.end(); ++k) {
                if (!skip.contains(block)) {
                    if (level > 0)
                        mesh->addMesh(*Sierpinski(level-1,*i,*j,*k));
                    else
                        mesh->addMesh(*PlaceBox(*i,*j,*k));
                }
                block++;
            }
        }
    }

    return mesh;
}

MeshObjectRef runSierpinski(const Param& p)
{
    return Sierpinski(p.level,p.x,p.y,p.z);
}

MeshObjectRef makeParallelMengerSponge(int level, float x0, float y0, float z0)
{
    float boxnums = std::pow(3.0f,level);
    float thirds = boxnums / 3;
    float twothirds = thirds * 2;

    QList<float> rangerx, rangery, rangerz;
    rangerx << x0 << (x0 + thirds) << (x0 + twothirds);
    rangery << y0 << (y0 + thirds) << (y0 + twothirds);
    rangerz << z0 << (z0 + thirds) << (z0 + twothirds);

    int block = 1;
    QList<int> skip; skip << 5 << 11 << 13 << 14 << 15 << 17 << 23;

    // collect the arguments for the algorithm in a list
    QList<Param> args;

    for (QList<float>::iterator i = rangerx.begin(); i != rangerx.end(); ++i) {
        for (QList<float>::iterator j = rangery.begin(); j != rangery.end(); ++j) {
            for (QList<float>::iterator k = rangerz.begin(); k != rangerz.end(); ++k) {
                if (!skip.contains(block)) {
                    args << Param(level-1, *i, *j, *k);
                }
                block++;
            }
        }
    }

    QFuture<MeshObjectRef> future = QtConcurrent::mapped(args, runSierpinski);

    QFutureWatcher<MeshObjectRef> watcher;
    watcher.setFuture(future);

    // keep it responsive during computation
    QEventLoop loop;
    QObject::connect(&watcher, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    MeshObjectRef mesh = new Mesh::MeshObject();
    for (QFuture<MeshObjectRef>::const_iterator it = future.begin(); it != future.end(); ++it) {
        mesh->addMesh(**it);
        (*it)->clear();
    }

    return mesh;
}

void CmdMengerSponge::activated(int)
{
    bool ok;
    int level = QInputDialog::getInt(Gui::getMainWindow(),
        QString::fromLatin1("Menger sponge"),
        QString::fromLatin1("Recursion depth:"),
        3, 1, 5, 1, &ok);
    if (!ok)
        return;
    int ret = QMessageBox::question(Gui::getMainWindow(),
        QString::fromLatin1("Parallel"),
        QString::fromLatin1("Do you want to run this in a thread pool?"),
        QMessageBox::Yes|QMessageBox::No);
    bool parallel=(ret == QMessageBox::Yes);
    float x0=0,y0=0,z0=0;

    globalBox = Mesh::MeshObject::createCube(1,1,1);

    MeshObjectRef mesh;
    if (parallel)
        mesh = makeParallelMengerSponge(level,x0,y0,z0);
    else
        mesh = Sierpinski(level,x0,y0,z0);

    MeshCore::MeshKernel& kernel = mesh->getKernel();

    // remove duplicated points
    MeshCore::MeshFixDuplicatePoints(kernel).Fixup();

    // remove internal facets
    MeshCore::MeshEvalInternalFacets eval(kernel);
    eval.Evaluate();
    kernel.DeleteFacets(eval.GetIndices());

    // repair neighbourhood
    kernel.RebuildNeighbours();

    App::Document* doc = App::GetApplication().newDocument();
    Mesh::Feature* feature = static_cast<Mesh::Feature*>(doc->addObject("Mesh::Feature","MengerSponge"));
    feature->Mesh.setValue(*mesh);
    feature->purgeTouched();
}

DEF_STD_CMD_A(CmdTestGraphicsView)

CmdTestGraphicsView::CmdTestGraphicsView()
  : Command("Std_TestGraphicsView")
{
    sGroup      = QT_TR_NOOP("Standard-Test");
    sMenuText   = QT_TR_NOOP("Create new graphics view");
    sToolTipText= QT_TR_NOOP("Creates a new view window for the active document");
    sStatusTip  = QT_TR_NOOP("Creates a new view window for the active document");
}

void CmdTestGraphicsView::activated(int)
{
    Gui::GraphicsView3D* view3D = new Gui::GraphicsView3D(getActiveGuiDocument(), Gui::getMainWindow());
    view3D->setWindowTitle(QString::fromLatin1("Graphics scene"));
    view3D->setWindowIcon(QApplication::windowIcon());
    view3D->resize(400, 300);
    Gui::getMainWindow()->addWindow(view3D);
}

bool CmdTestGraphicsView::isActive(void)
{
    return (getActiveGuiDocument()!=NULL);
}

//===========================================================================
// Std_TestTaskBox
//===========================================================================
DEF_STD_CMD(CmdTestTaskBox)

CmdTestTaskBox::CmdTestTaskBox()
  : Command("Std_TestTaskBox")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Task box";
    sToolTipText    = "Task box";
    sWhatsThis      = "Std_TestTaskBox";
    sStatusTip      = sToolTipText;
}

void CmdTestTaskBox::activated(int)
{
    QWidget* w = new SandboxGui::TaskPanelView();
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
}


void CreateSandboxCommands()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdSandboxDocumentThread());
    rcCmdMgr.addCommand(new CmdSandboxDocumentTestThread());
    rcCmdMgr.addCommand(new CmdSandboxDocumentSaveThread());
    rcCmdMgr.addCommand(new CmdSandboxDocThreadWithSeq());
    rcCmdMgr.addCommand(new CmdSandboxDocThreadBusy());
    rcCmdMgr.addCommand(new CmdSandboxDocumentNoThread());
    rcCmdMgr.addCommand(new CmdSandboxWorkerThread());
    rcCmdMgr.addCommand(new CmdSandboxPythonLockThread());
    rcCmdMgr.addCommand(new CmdSandboxPythonNolockThread());
    rcCmdMgr.addCommand(new CmdSandboxPySideThread());
    rcCmdMgr.addCommand(new CmdSandboxPythonThread());
    rcCmdMgr.addCommand(new CmdSandboxPythonMainThread());
    rcCmdMgr.addCommand(new CmdSandboxDocThreadWithDialog());
    rcCmdMgr.addCommand(new CmdSandboxDocThreadWithFileDlg());
    rcCmdMgr.addCommand(new CmdSandboxEventLoop);
    rcCmdMgr.addCommand(new CmdSandboxMeshLoader);
    rcCmdMgr.addCommand(new CmdSandboxMeshLoaderBoost);
    rcCmdMgr.addCommand(new CmdSandboxMeshLoaderFuture);
    rcCmdMgr.addCommand(new CmdSandboxMeshTestJob);
    rcCmdMgr.addCommand(new CmdSandboxMeshTestRef);
    rcCmdMgr.addCommand(new CmdTestGrabWidget());
    rcCmdMgr.addCommand(new CmdTestImageNode());
    rcCmdMgr.addCommand(new CmdTestWidgetShape());
    rcCmdMgr.addCommand(new CmdTestGDIWidget());
    rcCmdMgr.addCommand(new CmdTestRedirectPaint());
    rcCmdMgr.addCommand(new CmdTestCryptographicHash());
    rcCmdMgr.addCommand(new CmdMengerSponge());
    rcCmdMgr.addCommand(new CmdTestGraphicsView());
    rcCmdMgr.addCommand(new CmdTestTaskBox());
}
