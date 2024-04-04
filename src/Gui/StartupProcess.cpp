// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QDir>
#include <QImageReader>
#include <QLabel>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QStatusBar>
#include <QWindow>
#include <Inventor/SoDB.h>
#endif

#include "StartupProcess.h"
#include "Application.h"
#include "AutoSaver.h"
#include "DlgCheckableMessageBox.h"
#include "FileDialog.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Language/Translator.h"
#include <App/Application.h>


using namespace Gui;


StartupProcess::StartupProcess() = default;

void StartupProcess::setupApplication()
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#endif

    // Automatic scaling for legacy apps (disable once all parts of GUI are aware of HiDpi)
    ParameterGrp::handle hDPI =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/HighDPI");
    bool disableDpiScaling = hDPI->GetBool("DisableDpiScaling", false);
    if (disableDpiScaling) {
#ifdef FC_OS_WIN32
        SetProcessDPIAware(); // call before the main event loop
#endif
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
#endif
    }
    else {
        // Enable automatic scaling based on pixel density of display (added in Qt 5.6)
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0) && defined(Q_OS_WIN)
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    }

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    //Enable support for highres images (added in Qt 5.1, but off by default)
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // Use software rendering for OpenGL
    ParameterGrp::handle hOpenGL =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/OpenGL");
    bool useSoftwareOpenGL = hOpenGL->GetBool("UseSoftwareOpenGL", false);
    if (useSoftwareOpenGL) {
        QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    // By default (on platforms that support it, see docs for
    // Qt::AA_CompressHighFrequencyEvents) QT applies compression
    // for high frequency events (mouse move, touch, window resizes)
    // to keep things smooth even when handling the event takes a
    // while (e.g. to calculate snapping).
    // However, tablet pen move events (and mouse move events
    // synthesised from those) are not compressed by default (to
    // allow maximum precision when e.g. hand-drawing curves),
    // leading to unacceptable slowdowns using a tablet pen. Enable
    // compression for tablet events here to solve that.
    QCoreApplication::setAttribute(Qt::AA_CompressTabletEvents);
#endif
}

void StartupProcess::execute()
{
    setLibraryPath();
    setStyleSheetPaths();
    setImagePaths();
    registerEventType();
    setThemePaths();
    setupFileDialog();
}

void StartupProcess::setLibraryPath()
{
    QString plugin;
    plugin = QString::fromStdString(App::Application::getHomePath());
    plugin += QLatin1String("/plugins");
    QCoreApplication::addLibraryPath(plugin);
}

void StartupProcess::setStyleSheetPaths()
{
    // setup the search paths for Qt style sheets
    QStringList qssPaths;
    qssPaths << QString::fromUtf8(
        (App::Application::getUserAppDataDir() + "Gui/Stylesheets/").c_str())
            << QString::fromUtf8((App::Application::getResourceDir() + "Gui/Stylesheets/").c_str())
            << QLatin1String(":/stylesheets");
    QDir::setSearchPaths(QString::fromLatin1("qss"), qssPaths);
    // setup the search paths for Qt overlay style sheets
    QStringList qssOverlayPaths;
    qssOverlayPaths << QString::fromUtf8((App::Application::getUserAppDataDir()
                        + "Gui/Stylesheets/overlay").c_str())
                    << QString::fromUtf8((App::Application::getResourceDir()
                        + "Gui/Stylesheets/overlay").c_str());
    QDir::setSearchPaths(QStringLiteral("overlay"), qssOverlayPaths);
}

void StartupProcess::setImagePaths()
{
    // set search paths for images
    QStringList imagePaths;
    imagePaths << QString::fromUtf8((App::Application::getUserAppDataDir() + "Gui/images").c_str())
            << QString::fromUtf8((App::Application::getUserAppDataDir() + "pixmaps").c_str())
            << QLatin1String(":/icons");
    QDir::setSearchPaths(QString::fromLatin1("images"), imagePaths);
}

void StartupProcess::registerEventType()
{
    // register action style event type
    ActionStyleEvent::EventType = QEvent::registerEventType(QEvent::User + 1);
}

void StartupProcess::setThemePaths()
{
    ParameterGrp::handle hTheme = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Bitmaps/Theme");
#if !defined(Q_OS_LINUX)
    QIcon::setThemeSearchPaths(QIcon::themeSearchPaths()
                            << QString::fromLatin1(":/icons/FreeCAD-default"));
    QIcon::setThemeName(QLatin1String("FreeCAD-default"));
#else
    // Option to opt-out from using a Linux desktop icon theme.
    // https://forum.freecad.org/viewtopic.php?f=4&t=35624
    bool themePaths = hTheme->GetBool("ThemeSearchPaths",true);
    if (!themePaths) {
        QStringList searchPaths;
        searchPaths.prepend(QString::fromUtf8(":/icons"));
        QIcon::setThemeSearchPaths(searchPaths);
        QIcon::setThemeName(QLatin1String("FreeCAD-default"));
    }
#endif

    std::string searchpath = hTheme->GetASCII("SearchPath");
    if (!searchpath.empty()) {
        QStringList searchPaths = QIcon::themeSearchPaths();
        searchPaths.prepend(QString::fromUtf8(searchpath.c_str()));
        QIcon::setThemeSearchPaths(searchPaths);
    }

    std::string name = hTheme->GetASCII("Name");
    if (!name.empty()) {
        QIcon::setThemeName(QString::fromLatin1(name.c_str()));
    }
}

void StartupProcess::setupFileDialog()
{
#if defined(FC_OS_LINUX)
    // See #0001588
    QString path = FileDialog::restoreLocation();
    FileDialog::setWorkingDirectory(QDir::currentPath());
    FileDialog::saveLocation(path);
#else
    FileDialog::setWorkingDirectory(FileDialog::restoreLocation());
#endif
}

// ------------------------------------------------------------------------------------------------

StartupPostProcess::StartupPostProcess(MainWindow* mw, Application& guiApp, QApplication* app)
    : mainWindow{mw}
    , guiApp{guiApp}
    , qtApp(app)
{
}

void StartupPostProcess::setLoadFromPythonModule(bool value)
{
    loadFromPythonModule = value;
}

void StartupPostProcess::execute()
{
    setWindowTitle();
    setProcessMessages();
    setAutoSaving();
    setToolBarIconSize();
    setWheelEventFilter();
    setLocale();
    setCursorFlashing();
    checkOpenGL();
    loadOpenInventor();
    setBranding();
    showMainWindow();
    activateWorkbench();
}

void StartupPostProcess::setWindowTitle()
{
    // empty window title QString sets default title (app + version)
    mainWindow->setWindowTitle(QString());
}

void StartupPostProcess::setProcessMessages()
{
    if (!loadFromPythonModule) {
        QObject::connect(qtApp, SIGNAL(messageReceived(const QList<QByteArray> &)),
                         mainWindow, SLOT(processMessages(const QList<QByteArray> &)));
    }
}

void StartupPostProcess::setAutoSaving()
{
    ParameterGrp::handle hDocGrp = WindowParameter::getDefaultParameter()->GetGroup("Document");
    int timeout = int(hDocGrp->GetInt("AutoSaveTimeout", 15L)); // 15 min
    if (!hDocGrp->GetBool("AutoSaveEnabled", true)) {
        timeout = 0;
    }

    AutoSaver::instance()->setTimeout(timeout * 60000);  // NOLINT
    AutoSaver::instance()->setCompressed(hDocGrp->GetBool("AutoSaveCompressed", true));
}

void StartupPostProcess::setToolBarIconSize()
{
    // set toolbar icon size
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    int size = int(hGrp->GetInt("ToolbarIconSize", 0));
    // must not be lower than this
    if (size >= 16) {  // NOLINT
        mainWindow->setIconSize(QSize(size,size));
    }
}

void StartupPostProcess::setWheelEventFilter()
{
    // filter wheel events for combo boxes
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    if (hGrp->GetBool("ComboBoxWheelEventFilter", false)) {
        auto filter = new WheelEventFilter(qtApp);
        qtApp->installEventFilter(filter);
    }
}

void StartupPostProcess::setLocale()
{
    // For values different to 1 and 2 use the OS locale settings
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    auto localeFormat = hGrp->GetInt("UseLocaleFormatting", 0);
    if (localeFormat == 1) {
        Translator::instance()->setLocale(
            hGrp->GetASCII("Language", Translator::instance()->activeLanguage().c_str()));
    }
    else if (localeFormat == 2) {
        Translator::instance()->setLocale("C");
    }

}

void StartupPostProcess::setCursorFlashing()
{
    // set text cursor blinking state
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    int blinkTime = hGrp->GetBool("EnableCursorBlinking", true) ? -1 : 0;
    QApplication::setCursorFlashTime(blinkTime);
}

void StartupPostProcess::checkOpenGL()
{
    QWindow window;
    window.setSurfaceType(QWindow::OpenGLSurface);
    window.create();

    QOpenGLContext context;
    if (context.create()) {
        context.makeCurrent(&window);
        if (!context.functions()->hasOpenGLFeature(QOpenGLFunctions::Framebuffers)) {
            Base::Console().Log("This system does not support framebuffer objects\n");
        }
        if (!context.functions()->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures)) {
            Base::Console().Log("This system does not support NPOT textures\n");
        }

        int major = context.format().majorVersion();
        int minor = context.format().minorVersion();

#ifdef NDEBUG
        // In release mode, issue a warning to users that their version of OpenGL is
        // potentially going to cause problems
        if (major < 2) {
            auto message =
                QObject::tr("This system is running OpenGL %1.%2. "
                            "FreeCAD requires OpenGL 2.0 or above. "
                            "Please upgrade your graphics driver and/or card as required.")
                    .arg(major)
                    .arg(minor)
                + QStringLiteral("\n");
            Base::Console().Warning(message.toStdString().c_str());
            Dialog::DlgCheckableMessageBox::showMessage(
                QCoreApplication::applicationName() + QStringLiteral(" - ")
                    + QObject::tr("Invalid OpenGL Version"),
                message);
        }
#endif
        const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        Base::Console().Log("OpenGL version is: %d.%d (%s)\n", major, minor, glVersion);
    }
}

void StartupPostProcess::loadOpenInventor()
{
    bool loadedInventor = false;
    if (loadFromPythonModule) {
        loadedInventor = SoDB::isInitialized();
    }

    if (!loadedInventor) {
        // init the Inventor subsystem
        Application::initOpenInventor();
    }
}

void StartupPostProcess::setBranding()
{
    QString home = QString::fromStdString(App::Application::getHomePath());

    const std::map<std::string,std::string>& cfg = App::Application::Config();
    std::map<std::string,std::string>::const_iterator it;
    it = cfg.find("WindowTitle");
    if (it != cfg.end()) {
        QString title = QString::fromUtf8(it->second.c_str());
        mainWindow->setWindowTitle(title);
    }
    it = cfg.find("WindowIcon");
    if (it != cfg.end()) {
        QString path = QString::fromUtf8(it->second.c_str());
        if (QDir(path).isRelative()) {
            path = QFileInfo(QDir(home), path).absoluteFilePath();
        }
        QApplication::setWindowIcon(QIcon(path));
    }
    it = cfg.find("ProgramLogo");
    if (it != cfg.end()) {
        QString path = QString::fromUtf8(it->second.c_str());
        if (QDir(path).isRelative()) {
            path = QFileInfo(QDir(home), path).absoluteFilePath();
        }
        QPixmap px(path);
        if (!px.isNull()) {
            auto logo = new QLabel();
            logo->setPixmap(px.scaledToHeight(32));
            mainWindow->statusBar()->addPermanentWidget(logo, 0);
            logo->setFrameShape(QFrame::NoFrame);
        }
    }
}

void StartupPostProcess::setImportImageFormats()
{
    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    std::stringstream str;
    str << "Image formats (";
    for (const auto& ext : supportedFormats) {
        str << "*." << ext.constData() << " *." << ext.toUpper().constData() << " ";
    }
    str << ")";

    std::string filter = str.str();
    App::GetApplication().addImportType(filter.c_str(), "FreeCADGui");
}

bool StartupPostProcess::hiddenMainWindow() const
{
    const std::map<std::string,std::string>& cfg = App::Application::Config();
    bool hidden = false;
    auto it = cfg.find("StartHidden");
    if (it != cfg.end()) {
        hidden = true;
    }

    return hidden;
}

void StartupPostProcess::showMainWindow()
{
    bool hidden = hiddenMainWindow();

    // show splasher while initializing the GUI
    if (!hidden && !loadFromPythonModule) {
        mainWindow->startSplasher();
    }

    // running the GUI init script
    try {
        Base::Console().Log("Run Gui init script\n");
        Application::runInitGuiScript();
        setImportImageFormats();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("Error in FreeCADGuiInit.py: %s\n", e.what());
        mainWindow->stopSplasher();
        throw;

    }

    // stop splash screen and set immediately the active window that may be of interest
    // for scripts using Python binding for Qt
    mainWindow->stopSplasher();
    qtApp->setActiveWindow(mainWindow);
}

void StartupPostProcess::activateWorkbench()
{
    // Activate the correct workbench
    std::string start = App::Application::Config()["StartWorkbench"];
    Base::Console().Log("Init: Activating default workbench %s\n", start.c_str());
    std::string autoload =
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
            ->GetASCII("AutoloadModule", start.c_str());
    if ("$LastModule" == autoload) {
        start = App::GetApplication()
                    .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
                    ->GetASCII("LastModule", start.c_str());
    }
    else {
        start = autoload;
    }
    // if the auto workbench is not visible then force to use the default workbech
    // and replace the wrong entry in the parameters
    QStringList wb = guiApp.workbenches();
    if (!wb.contains(QString::fromLatin1(start.c_str()))) {
        start = App::Application::Config()["StartWorkbench"];
        if ("$LastModule" == autoload) {
            App::GetApplication()
                .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
                ->SetASCII("LastModule", start.c_str());
        }
        else {
            App::GetApplication()
                .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
                ->SetASCII("AutoloadModule", start.c_str());
        }
    }

    // Call this before showing the main window because otherwise:
    // 1. it shows a white window for a few seconds which doesn't look nice
    // 2. the layout of the toolbars is completely broken
    guiApp.activateWorkbench(start.c_str());

    // show the main window
    if (!hiddenMainWindow()) {
        Base::Console().Log("Init: Showing main window\n");
        mainWindow->loadWindowSettings();
    }

    //initialize spaceball.
    if (auto fcApp = qobject_cast<GUIApplicationNativeEventAware*>(qtApp)) {
        fcApp->initSpaceball(mainWindow);
    }

    setStyleSheet();

    // Now run the background autoload, for workbenches that should be loaded at startup, but not
    // displayed to the user immediately
    autoloadModules(wb);

    // Reactivate the startup workbench
    guiApp.activateWorkbench(start.c_str());
}

void StartupPostProcess::setStyleSheet()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow");
    std::string style = hGrp->GetASCII("StyleSheet");
    if (style.empty()) {
        // check the branding settings
        const auto& config = App::Application::Config();
        auto it = config.find("StyleSheet");
        if (it != config.end()) {
            style = it->second;
        }
    }

    guiApp.setStyleSheet(QLatin1String(style.c_str()), hGrp->GetBool("TiledBackground", false));
}

void StartupPostProcess::autoloadModules(const QStringList& wb)
{
    // Now run the background autoload, for workbenches that should be loaded at startup, but not
    // displayed to the user immediately
    std::string autoloadCSV =
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
            ->GetASCII("BackgroundAutoloadModules", "");

    // Tokenize the comma-separated list and load the requested workbenches if they exist in this
    // installation
    std::stringstream stream(autoloadCSV);
    std::string workbench;
    while (std::getline(stream, workbench, ',')) {
        if (wb.contains(QString::fromLatin1(workbench.c_str()))) {
            guiApp.activateWorkbench(workbench.c_str());
        }
    }
}
