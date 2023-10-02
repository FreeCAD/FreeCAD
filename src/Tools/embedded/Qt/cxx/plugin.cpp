#include "PreCompiled.h"

#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include <QVector>

#include <App/Application.h>
#include <Base/Factory.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>


PyMODINIT_FUNC FreeCAD_init()
{
    static QVector<char*> argv;
#if defined(_DEBUG)
    argv << "FreeCADApp_d.dll" << 0;
#else
    argv << "FreeCADApp.dll" << 0;
#endif

    App::Application::Config()["RunMode"] = "Gui";
    App::Application::Config()["Console"] = "0";
    App::Application::Config()["ExeVendor"] = "FreeCAD";
    App::Application::Config()["SplashScreen"] = "freecadsplash";

    App::Application::init(1, argv.data());
    Gui::Application::initApplication();
    Gui::Application* app = new Gui::Application(true);

    Gui::MainWindow* mw = new Gui::MainWindow(qApp->activeWindow());
    mw->show();

    app->initOpenInventor();
    app->runInitGuiScript();
}

/* A test function for the plugin to load a mesh and call "getVal()" */
PyMODINIT_FUNC FreeCAD_test(const char* path)
{
    try {  // Use FreeCADGui here, not Gui
        Base::Interpreter().runString("FreeCADGui.activateWorkbench(\"MeshWorkbench\")");
        Base::Interpreter().runString("import Mesh");
        Base::Interpreter().runStringArg("Mesh.insert(u\"%s\", \"%s\")", path, "Document");
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(0, "Exception", e.what());
    }
}
