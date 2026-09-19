// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <clocale>

#include <QApplication>
#include <QByteArray>
#include <QLocale>

#include <src/App/InitApplication.h>

#include <Gui/Application.h>

namespace tests
{

/** Ensure Qt can create a QApplication before QTEST / widget construction.
 *
 * On Windows pixi/conda, missing QT_PLUGIN_PATH often makes QApplication abort
 * with STATUS_STACK_BUFFER_OVERRUN (0xc0000409) when the platform plugin cannot
 * load. Prefer an existing env; otherwise point at CONDA_PREFIX Library/plugins
 * and default QT_QPA_PLATFORM to offscreen for headless CTest.
 *
 * Pair with QTEST_APPLESS_MAIN so this runs before any QApplication.
 */
inline void prepareQtGuiTestEnvironment()
{
    if (!qEnvironmentVariableIsSet("QT_PLUGIN_PATH")) {
        const QByteArray conda = qgetenv("CONDA_PREFIX");
        if (!conda.isEmpty()) {
#ifdef _WIN32
            qputenv("QT_PLUGIN_PATH", conda + "/Library/plugins");
#else
            qputenv("QT_PLUGIN_PATH", conda + "/plugins");
#endif
        }
    }

    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
}

/** Minimal Qt GUI environment for QTest executables (widgets, dialogs). */
inline void initQtGuiTest(bool needGuiApplication = true)
{
    prepareQtGuiTestEnvironment();

    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char appName[] = "FreeCADGuiTest";
        static char* argv[] = {appName, nullptr};
        (void)new QApplication(argc, argv);
    }

    initApplication();

    // Match Gui::Application::runApplication numeric formatting.
    setlocale(LC_NUMERIC, "C");
    QLocale::setDefault(QLocale(QLocale::C, QLocale::AnyCountry));

    if (needGuiApplication && !Gui::Application::Instance) {
        new Gui::Application(false);
    }
}

}  // namespace tests
