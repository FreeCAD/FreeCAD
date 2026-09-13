// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QtTest/QtTest>

#include <Python.h>

#include <memory>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventorViewer.h>

#include <src/App/InitApplication.h>

class ViewerLifetimeTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        Gui::Application::initApplication();
        Gui::Application::initOpenInventor();
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->SetBool("ShowNaviCube", false);
        _guiApplication = std::make_unique<Gui::Application>(false);
        _mainWindow = std::make_unique<Gui::MainWindow>();
    }

    void wrapperIsInvalidatedAndNativeReferenceReleased()
    {
        PyGILState_STATE gilState = PyGILState_Ensure();
        PyObject* wrapper = nullptr;

        {
            auto viewer = std::make_unique<Gui::View3DInventorViewer>(nullptr);
            wrapper = viewer->getPyObject();
            QCOMPARE(Py_REFCNT(wrapper), 2);
            viewer.reset();
        }

        QCOMPARE(Py_REFCNT(wrapper), 1);
        PyObject* representation = PyObject_Repr(wrapper);
        QVERIFY(representation == nullptr);
        QVERIFY(PyErr_ExceptionMatches(PyExc_RuntimeError));
        PyErr_Clear();

        Py_DECREF(wrapper);
        PyGILState_Release(gilState);
    }

private:
    std::unique_ptr<Gui::Application> _guiApplication;
    std::unique_ptr<Gui::MainWindow> _mainWindow;
};

QTEST_MAIN(ViewerLifetimeTest)

#include "ViewerLifetime.moc"
