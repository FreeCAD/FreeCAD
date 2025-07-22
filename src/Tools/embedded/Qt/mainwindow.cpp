
#include <Python.h>
#include <QtGui>
#include <QtWidgets>
#include <sstream>
#include <string>
#if defined(Q_WS_X11)
#include <QX11EmbedContainer>
#endif

#include "mainwindow.h"

MainWindow::MainWindow()
{
    createActions();
    createMenus();
#if defined(Q_WS_X11)
    setCentralWidget(new QX11EmbedContainer(this));
#else
    setCentralWidget(new QWidget(this));
#endif
}

void MainWindow::createActions()
{
    loadAct = new QAction(tr("&Load"), this);
    loadAct->setStatusTip(tr("Load FreeCAD module"));
    connect(loadAct, SIGNAL(triggered()), this, SLOT(loadFreeCAD()));

    newAct = new QAction(tr("New document"), this);
    newAct->setStatusTip(tr("Create new document in the FreeCAD module"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newDocument()));
    newAct->setDisabled(true);

    embedAct = new QAction(tr("Embed"), this);
    embedAct->setStatusTip(tr("Embed FreeCAD as child window"));
    connect(embedAct, SIGNAL(triggered()), this, SLOT(embedWindow()));
    embedAct->setDisabled(true);

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("Free&CAD"));
    editMenu->addAction(loadAct);
    editMenu->addAction(newAct);
    editMenu->addAction(embedAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
}

void MainWindow::loadFreeCAD()
{
    QString path = QFileDialog::getExistingDirectory(this, "FreeCAD module path");
    if (!path.isEmpty()) {
        path.replace('\\', '/');
        PyObject* main = PyImport_AddModule("__main__");
        PyObject* dict = PyModule_GetDict(main);
        std::stringstream cmd;
        cmd << "import sys,os\n"
            << "sys.path.append(\"" << (const char*)path.toLatin1() << "\")\n"
            << "os.chdir(\"" << (const char*)path.toLatin1() << "\")\n"
            << "import FreeCADGui\n"
            << "FreeCADGui.showMainWindow()\n";

        PyObject* result = PyRun_String(cmd.str().c_str(), Py_file_input, dict, dict);
        if (result) {
            Py_DECREF(result);
            loadAct->setDisabled(true);
            newAct->setEnabled(true);
            embedAct->setEnabled(true);
        }
        else {
            PyObject *ptype, *pvalue, *ptrace;
            PyErr_Fetch(&ptype, &pvalue, &ptrace);
            PyObject* pystring = PyObject_Str(pvalue);
            const char* error = PyUnicode_AsUTF8(pystring);
            QMessageBox::warning(this, "Error", error);
            Py_DECREF(pystring);
        }
        Py_DECREF(dict);
    }
}

void MainWindow::newDocument()
{
    PyObject* main = PyImport_AddModule("__main__");
    PyObject* dict = PyModule_GetDict(main);
    const char* cmd = "FreeCAD.newDocument()\n";

    PyObject* result = PyRun_String(cmd, Py_file_input, dict, dict);
    if (result) {
        Py_DECREF(result);
    }
    else {
        PyObject *ptype, *pvalue, *ptrace;
        PyErr_Fetch(&ptype, &pvalue, &ptrace);
        PyObject* pystring = PyObject_Str(pvalue);
        const char* error = PyUnicode_AsUTF8(pystring);
        QMessageBox::warning(this, "Error", error);
        Py_DECREF(pystring);
    }
    Py_DECREF(dict);
}

void MainWindow::embedWindow()
{
    PyObject* main = PyImport_AddModule("__main__");
    PyObject* dict = PyModule_GetDict(main);
    std::stringstream cmd;
    cmd << "class BlankWorkbench (Workbench):\n"
        << "   MenuText = \"Blank\"\n"
        << "   ToolTip = \"Blank workbench\"\n"
        << "\n"
        << "   def Initialize(self):\n"
        << "      return\n"
        << "   def GetClassName(self):\n"
        << "      return \"Gui::BlankWorkbench\"\n"
        << "\n"
        << "FreeCADGui.addWorkbench(BlankWorkbench)\n"
        << "FreeCADGui.activateWorkbench(\"BlankWorkbench\")\n"
        << "\n";

#if defined(Q_WS_X11) || defined(Q_OS_WIN)
    WId hwnd = this->centralWidget()->winId();
    cmd << "FreeCADGui.embedToWindow(\"" << hwnd << "\")\n"
        << "\n";
#endif

    PyObject* result = PyRun_String(cmd.str().c_str(), Py_file_input, dict, dict);
    if (result) {
        Py_DECREF(result);

#if !defined(Q_WS_X11)
        // This is a workaround for the lack of a replacement of QX11EmbedWidget with Qt5
        QWidget* mw = nullptr;
        for (auto it : qApp->topLevelWidgets()) {
            if (it->inherits("Gui::MainWindow")) {
                mw = it;
                break;
            }
        }
        if (mw) {
            QVBoxLayout* vb = new QVBoxLayout();
            centralWidget()->setLayout(vb);
            vb->addWidget(mw);
        }
#endif

        embedAct->setDisabled(true);
    }
    else {
        PyObject *ptype, *pvalue, *ptrace;
        PyErr_Fetch(&ptype, &pvalue, &ptrace);
        PyObject* pystring = PyObject_Str(pvalue);
        const char* error = PyUnicode_AsUTF8(pystring);
        QMessageBox::warning(this, "Error", error);
        Py_DECREF(pystring);
    }
    Py_DECREF(dict);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About"), tr("Demonstrates remote control of FreeCAD"));
}
