/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include <QActionGroup>
#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFontMetrics>
#include <QKeySequence>
#include <QLabel>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QOpenGLWidget>
#include <QPainter>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QScreen>
#include <QSettings>
#include <QSignalMapper>
#include <QStatusBar>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QUrlQuery>
#include <QWhatsThis>
#include <QWindow>
#include <QPushButton>


#if defined(Q_OS_WIN)
# if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#  include <QtPlatformHeaders/QWindowsWindowFunctions>
# else
#  include <qpa/qplatformwindow_p.h>
# endif
#endif

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/ImagePlane.h>
#include <App/SafeMode.h>
#include <Base/ConsoleObserver.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <DAGView/DAGView.h>
#include <TaskView/TaskView.h>

#include "MainWindow.h"
#include "Action.h"
#include "Assistant.h"
#include "BitmapFactory.h"
#include "ComboView.h"
#include "Command.h"
#include "DockWindowManager.h"
#include "DownloadManager.h"
#include "FileDialog.h"
#include "InputHintWidget.h"
#include "MenuManager.h"
#include "ModuleIO.h"
#include "NotificationArea.h"
#include "OverlayManager.h"
#include "ProgressBar.h"
#include "PropertyView.h"
#include "PythonConsole.h"
#include "ReportView.h"
#include "SelectionView.h"
#include "SplashScreen.h"
#include "StatusBarLabel.h"
#include "ToolBarManager.h"
#include "ToolBoxManager.h"
#include "Tree.h"
#include "WaitCursor.h"
#include "WorkbenchManager.h"
#include "Workbench.h"

#include "MergeDocuments.h"
#include "ViewProviderExtern.h"

#include "SpaceballEvent.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "Dialogs/DlgObjectSelection.h"

#include <Base/Color.h>
#include "QtWidgets.h"

FC_LOG_LEVEL_INIT("MainWindow", false, true, true)

#if defined(Q_OS_WIN32)
# define slots
#endif

using namespace Gui;
using namespace Gui::DockWnd;
using namespace std;


MainWindow* MainWindow::instance = nullptr;

namespace Gui
{

/**
 * The CustomMessageEvent class is used to send messages as events in the methods
 * Error(), Warning() and Message() of the StatusBarObserver class to the main window
 * to display them on the status bar instead of printing them directly to the status bar.
 *
 * This makes the usage of StatusBarObserver thread-safe.
 * @author Werner Mayer
 */
class CustomMessageEvent: public QEvent
{
public:
    CustomMessageEvent(int t, const QString& s, int timeout = 0)
        : QEvent(QEvent::User)
        , _type(t)
        , msg(s)
        , _timeout(timeout)
    {}
    ~CustomMessageEvent() override = default;
    int type() const
    {
        return _type;
    }
    const QString& message() const
    {
        return msg;
    }
    int timeout() const
    {
        return _timeout;
    }

private:
    int _type;
    QString msg;
    int _timeout;
};

/**
 * The DimensionWidget class is aiming at providing a widget used in the status bar that will:
 *  - Allow application to display dimension information such as the viewportsize
 *  - Provide a popup menu allowing user to change the used unit schema (and update if changed
 * elsewhere)
 */
class DimensionWidget: public QPushButton, WindowParameter
{
    Q_OBJECT

public:
    explicit DimensionWidget(QWidget* parent)
        : QPushButton(parent)
        , WindowParameter("Units")
    {
        setFlat(true);
        setText(qApp->translate("Gui::MainWindow", "Dimension"));
        setMinimumWidth(120);

        // create the action buttons
        auto* menu = new QMenu(this);
        auto* actionGrp = new QActionGroup(menu);

        auto setAction = [&, index {0}](const std::string&) mutable {
            QAction* action = menu->addAction(QStringLiteral("UnitSchema%1").arg(index));
            actionGrp->addAction(action);
            action->setCheckable(true);
            action->setData(index++);
        };
        auto descriptions = Base::UnitsApi::getDescriptions();
        std::for_each(descriptions.begin(), descriptions.end(), setAction);

        QObject::connect(actionGrp, &QActionGroup::triggered, this, [this](QAction* action) {
            int userSchema = action->data().toInt();
            setUserSchema(userSchema);
            // Force PropertyEditor refresh until we find a better way.  Q_EMIT something?
            const auto views = getMainWindow()->findChildren<PropertyView*>();
            for (auto view : views) {
                bool show = view->showAll();
                view->setShowAll(!show);
                view->setShowAll(show);
            }
        });
        setMenu(menu);
        retranslateUi();
        unitChanged();
        getWindowParameter()->Attach(this);
    }

    ~DimensionWidget() override
    {
        getWindowParameter()->Detach(this);
    }

    void OnChange(Base::Subject<const char*>& rCaller, const char* sReason) override
    {
        Q_UNUSED(rCaller)
        if (strcmp(sReason, "UserSchema") == 0) {
            unitChanged();
        }
    }

    void changeEvent(QEvent* event) override
    {
        if (event->type() == QEvent::LanguageChange) {
            retranslateUi();
        }
        else {
            QPushButton::changeEvent(event);
        }
    }

    void setUserSchema(int userSchema)
    {
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (doc != nullptr) {
            if (doc->UnitSystem.getValue() != userSchema) {
                doc->UnitSystem.setValue(userSchema);
            }
        }
        else {
            getWindowParameter()->SetInt("UserSchema", userSchema);
        }

        unitChanged();
        Base::UnitsApi::setSchema(userSchema);
        // Update the main window to show the unit change
        Gui::Application::Instance->onUpdate();
    }

private:
    void unitChanged()
    {
        ParameterGrp::handle hGrpu = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Units"
        );
        bool ignore = hGrpu->GetBool("IgnoreProjectSchema", false);
        App::Document* doc = App::GetApplication().getActiveDocument();
        int userSchema = getWindowParameter()->GetInt("UserSchema", 0);
        if (doc != nullptr && !ignore) {
            userSchema = doc->UnitSystem.getValue();
        }
        auto actions = menu()->actions();
        if (Q_UNLIKELY(userSchema < 0 || userSchema >= actions.size())) {
            userSchema = 0;
        }
        actions[userSchema]->setChecked(true);
    }

    void retranslateUi()
    {
        auto actions = menu()->actions();
        auto addAction = [&, index {0}](const std::string& action) mutable {
            actions[index++]->setText(QString::fromStdString(action));
        };
        auto descriptions = Base::UnitsApi::getDescriptions();
        assert(actions.size() <= static_cast<qsizetype>(descriptions.size()));
        std::for_each(descriptions.begin(), descriptions.end(), addAction);
    }
};

// -------------------------------------
// Pimpl class
struct MainWindowP
{
    DimensionWidget* sizeLabel;
    QLabel* actionLabel;
    InputHintWidget* hintLabel;
    QLabel* rightSideLabel;
    QTimer* actionTimer;
    QTimer* statusTimer;
    QTimer* activityTimer;
    QTimer saveStateTimer;
    QTimer restoreStateTimer;
    QMdiArea* mdiArea;
    QPointer<MDIView> activeView;
    QSignalMapper* windowMapper;
    SplashScreen* splashscreen;
    StatusBarObserver* status;
    bool whatsthis;
    QString whatstext;
    Assistant* assistant;
    int currentStatusType = 100;
    int actionUpdateDelay = 0;
    QMap<QString, QPointer<UrlHandler>> urlHandler;
    std::string hiddenDockWindows;
    boost::signals2::scoped_connection connParam;
    ParameterGrp::handle hGrp;
    bool _restoring = false;
    QTime _showNormal;
    void restoreWindowState(const QByteArray&);
};

}  // namespace Gui

/* TRANSLATOR Gui::MainWindow */

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags f)
    : QMainWindow(parent, f /*WDestructiveClose*/)
{
    d = new MainWindowP;
    d->splashscreen = nullptr;
    d->activeView = nullptr;
    d->whatsthis = false;
    d->assistant = new Assistant();

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // this forces QT to switch to OpenGL mode, this prevents delay and flickering of the window
    // after opening project and prevent issues with double initialization of the window
    //
    // https://stackoverflow.com/questions/76026196/how-to-force-qt-to-use-the-opengl-window-type
    auto _OpenGLWidget = new QOpenGLWidget(this);
    _OpenGLWidget->move(QPoint(-100, -100));
#endif

    // global access
    instance = this;

    d->connParam = App::GetApplication().GetUserParameter().signalParamChanged.connect(
        [this](ParameterGrp* Param, ParameterGrp::ParamType, const char* Name, const char*) {
            if (Param != d->hGrp || !Name) {
                return;
            }
            if (boost::equals(Name, "StatusBar")) {
                if (auto sb = getMainWindow()->statusBar()) {
                    sb->setVisible(d->hGrp->GetBool("StatusBar", sb->isVisible()));
                }
            }
            else if (boost::equals(Name, "MainWindowState")) {
                OverlayManager::instance()->reload(OverlayManager::ReloadMode::ReloadPause);
                d->restoreStateTimer.start(100);
            }
        }
    );

    d->hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow"
    );
    d->saveStateTimer.setSingleShot(true);
    connect(&d->saveStateTimer, &QTimer::timeout, [this]() { this->saveWindowSettings(); });

    d->restoreStateTimer.setSingleShot(true);
    connect(&d->restoreStateTimer, &QTimer::timeout, [this]() {
        d->restoreWindowState(QByteArray::fromBase64(d->hGrp->GetASCII("MainWindowState").c_str()));
        ToolBarManager::getInstance()->restoreState();
        OverlayManager::instance()->reload(OverlayManager::ReloadMode::ReloadResume);
    });

    // support for grouped dragging of dockwidgets
    // https://woboq.com/blog/qdockwidget-changes-in-56.html
    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);

    // Create the layout containing the workspace and a tab bar
    d->mdiArea = new QMdiArea();
    // Movable tabs
    d->mdiArea->setTabsMovable(true);
    d->mdiArea->setTabPosition(QTabWidget::South);
    d->mdiArea->setViewMode(QMdiArea::TabbedView);
    auto tab = d->mdiArea->findChild<QTabBar*>();
    if (tab) {
        tab->setTabsClosable(true);
        // The tabs might be very wide
        tab->setExpanding(false);
        tab->setObjectName(QStringLiteral("mdiAreaTabBar"));
    }
    d->mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);
#ifndef HAS_QTBUG_129596
    d->mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
#endif
    d->mdiArea->setBackground(QBrush(QColor(160, 160, 160)));
    setCentralWidget(d->mdiArea);

    statusBar()->setObjectName(QStringLiteral("statusBar"));
    connect(statusBar(), &QStatusBar::messageChanged, this, &MainWindow::statusMessageChanged);

    // labels and progressbar
    d->status = new StatusBarObserver();
    d->actionLabel = new StatusBarLabel(statusBar());
    d->actionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    d->sizeLabel = new DimensionWidget(statusBar());

    statusBar()->addWidget(d->actionLabel, 1);
    QProgressBar* progressBar = Gui::SequencerBar::instance()->getProgressBar(statusBar());
    statusBar()->addPermanentWidget(progressBar, 0);
    statusBar()->addPermanentWidget(d->sizeLabel, 0);

    // hint label
    d->hintLabel = new InputHintWidget(statusBar());
    d->hintLabel->setObjectName(QStringLiteral("hintLabel"));
    //: A context menu action used to show or hide the input hints in the status bar
    d->hintLabel->setWindowTitle(tr("Input Hints"));

    statusBar()->addWidget(d->hintLabel);

    // right side label
    d->rightSideLabel = new StatusBarLabel(statusBar(), "QuickMeasureEnabled");
    d->rightSideLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusBar()->addPermanentWidget(d->rightSideLabel);
    d->rightSideLabel->setObjectName(QStringLiteral("rightSideLabel"));
    //: A context menu action used to enable or disable quick measure in the status bar
    d->rightSideLabel->setWindowTitle(tr("Quick Measure"));

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/NotificationArea"
    );

    auto notificationAreaEnabled = hGrp->GetBool("NotificationAreaEnabled", true);

    if (notificationAreaEnabled) {
        NotificationArea* notificationArea = new NotificationArea(statusBar());
        notificationArea->setObjectName(QStringLiteral("notificationArea"));
        //: A context menu action used to show or hide the 'notificationArea' toolbar widget
        notificationArea->setWindowTitle(tr("Notification Area"));
        notificationArea->setStyleSheet(QStringLiteral("text-align:center;"));
        statusBar()->addPermanentWidget(notificationArea);
    }

    // clears the action label
    d->actionTimer = new QTimer(this);
    d->actionTimer->setObjectName(QStringLiteral("actionTimer"));
    connect(d->actionTimer, &QTimer::timeout, d->actionLabel, &QLabel::clear);

    // clear status type
    d->statusTimer = new QTimer(this);
    d->statusTimer->setObjectName(QStringLiteral("statusTimer"));
    connect(d->statusTimer, &QTimer::timeout, this, &MainWindow::clearStatus);

    // update gui timer
    d->activityTimer = new QTimer(this);
    d->activityTimer->setObjectName(QStringLiteral("activityTimer"));
    connect(d->activityTimer, &QTimer::timeout, this, &MainWindow::_updateActions);
    d->activityTimer->setSingleShot(false);
    d->activityTimer->start(150);

    // update view-sensitive commands when clipboard has changed
    QClipboard* clipbd = QApplication::clipboard();
    connect(clipbd, &QClipboard::dataChanged, this, &MainWindow::updateEditorActions);

    d->windowMapper = new QSignalMapper(this);

    // connection between workspace, window menu and tab bar
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(d->windowMapper, &QSignalMapper::mappedWidget, this, &MainWindow::setActiveSubWindow);
#else
    connect(d->windowMapper, &QSignalMapper::mappedObject, this, [=, this](QObject* object) {
        setActiveSubWindow(qobject_cast<QWidget*>(object));
    });
#endif
    connect(d->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::onWindowActivated);

    setupDockWindows();

    // accept drops on the window, get handled in dropEvent, dragEnterEvent
    setAcceptDrops(true);

    statusBar()->showMessage(tr("Ready"), 2001);
}

MainWindow::~MainWindow()
{
    delete d->status;
    delete d;
    instance = nullptr;
}

MainWindow* MainWindow::getInstance()
{
    // MainWindow has a public constructor
    return instance;
}

// Helper function to update dock widget according to the user parameter
// settings, e.g. register/unregister, enable/disable, show/hide.
template<class T>
static inline void _updateDockWidget(
    const char* name,
    bool enabled,
    bool show,
    Qt::DockWidgetArea pos,
    T callback
)
{
    auto pDockMgr = DockWindowManager::instance();
    auto widget = pDockMgr->findRegisteredDockWindow(name);
    if (!enabled) {
        if (widget) {
            pDockMgr->removeDockWindow(widget);
            pDockMgr->unregisterDockWindow(name);
            widget->deleteLater();
        }
        return;
    }
    // Use callback to perform specific update for each type of dock widget
    widget = callback(widget);
    if (!widget) {
        return;
    }
    DockWindowManager::instance()->registerDockWindow(name, widget);
    if (show) {
        auto dock = pDockMgr->addDockWindow(widget->objectName().toUtf8().constData(), widget, pos);
        if (dock) {
            if (!dock->toggleViewAction()->isChecked()) {
                dock->toggleViewAction()->activate(QAction::Trigger);
            }
            OverlayManager::instance()->refresh(dock);
        }
    }
}

void MainWindow::initDockWindows(bool show)
{
    updateTreeView(show);
    updatePropertyView(show);
    updateComboView(show);
    updateTaskView(show);
    updateDAGView(show);
}

void MainWindow::setupDockWindows()
{
    // Report view must be created before PythonConsole!
    setupReportView();
    setupPythonConsole();
    setupSelectionView();
    setupTaskView();

    initDockWindows(false);

    std::vector<QTabWidget::TabPosition> tabPos
        = {QTabWidget::North, QTabWidget::South, QTabWidget::West, QTabWidget::East};
    long value = d->hGrp->GetInt("LeftDockWidgetAreaTabPos", long(tabPos.front()));
    if (value >= 0 && value < long(tabPos.size())) {
        setTabPosition(Qt::LeftDockWidgetArea, tabPos[value]);
    }
}

bool MainWindow::setupTaskView()
{
    // Task view
    if (d->hiddenDockWindows.find("Std_TaskView") == std::string::npos) {
        // clang-format off
        auto group = App::GetApplication().GetUserParameter()
                      .GetGroup("BaseApp")
                     ->GetGroup("Preferences")
                     ->GetGroup("DockWindows")
                     ->GetGroup("TaskView");
        // clang-format on
        auto taskView = new Gui::TaskView::TaskView(this);
        bool restore = group->GetBool("RestoreWidth", taskView->shouldRestoreWidth());
        taskView->setRestoreWidth(restore);
        taskView->setObjectName(QStringLiteral("Tasks"));
        taskView->setWindowTitle(QDockWidget::tr("Tasks"));
        taskView->setMinimumWidth(210);

        DockWindowManager* pDockMgr = DockWindowManager::instance();
        pDockMgr->registerDockWindow("Std_TaskView", taskView);
        return true;
    }

    return false;
}

bool MainWindow::setupSelectionView()
{
    // Selection view
    if (d->hiddenDockWindows.find("Std_SelectionView") == std::string::npos) {
        auto pcSelectionView = new SelectionView(nullptr, this);
        pcSelectionView->setObjectName(QStringLiteral("Selection view"));
        pcSelectionView->setWindowTitle(QDockWidget::tr("Selection View"));
        pcSelectionView->setMinimumWidth(210);

        DockWindowManager* pDockMgr = DockWindowManager::instance();
        pDockMgr->registerDockWindow("Std_SelectionView", pcSelectionView);
        return true;
    }

    return false;
}

bool MainWindow::setupReportView()
{
    // Report view
    if (d->hiddenDockWindows.find("Std_ReportView") == std::string::npos) {
        auto pcReport = new ReportOutput(this);
        pcReport->setWindowIcon(BitmapFactory().pixmap("MacroEditor"));
        pcReport->setObjectName(QStringLiteral("Report view"));
        pcReport->setWindowTitle(QDockWidget::tr("Report View"));

        DockWindowManager* pDockMgr = DockWindowManager::instance();
        pDockMgr->registerDockWindow("Std_ReportView", pcReport);

        auto rvObserver = new ReportOutputObserver(pcReport);
        qApp->installEventFilter(rvObserver);
        return true;
    }

    return false;
}

bool MainWindow::setupPythonConsole()
{
    // Python console
    if (d->hiddenDockWindows.find("Std_PythonView") == std::string::npos) {
        auto pcPython = new PythonConsole(this);
        pcPython->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
        pcPython->setObjectName(QStringLiteral("Python console"));
        pcPython->setWindowTitle(QDockWidget::tr("Python Console"));

        DockWindowManager* pDockMgr = DockWindowManager::instance();
        pDockMgr->registerDockWindow("Std_PythonView", pcPython);
        return true;
    }

    return false;
}

bool MainWindow::checkFirstRun()
{
    ParameterGrp::handle hGrpRF = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/RecentFiles"
    );
    auto RecentFilesCount = hGrpRF->GetInt("RecentFiles");
    ParameterGrp::handle hGrpFS2024 = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start"
    );
    auto firstStart = hGrpFS2024->GetBool("FirstStart2024", true);  // NOLINT
    if (firstStart && RecentFilesCount < 1) {
        return true;
    }
    return false;
}


void MainWindow::moveToDefaultPosition(QRect rect, QPoint pos)
{
    int x1 {}, x2 {}, y1 {}, y2 {};
    // make sure that the main window is not totally out of the visible rectangle
    rect.getCoords(&x1, &y1, &x2, &y2);
    const int offsetX = 30;
    const int offsetY = 10;
    pos.setX(qMin(qMax(pos.x(), x1 - this->width() + offsetX), x2 - offsetX));
    pos.setY(qMin(qMax(pos.y(), y1 - offsetY), y2 - offsetY));
    this->move(pos);
}


bool MainWindow::updateTreeView(bool show)
{
    if (d->hiddenDockWindows.find("Std_TreeView") == std::string::npos) {
        ParameterGrp::handle group = App::GetApplication()
                                         .GetUserParameter()
                                         .GetGroup("BaseApp")
                                         ->GetGroup("Preferences")
                                         ->GetGroup("DockWindows")
                                         ->GetGroup("TreeView");
        bool enabled = group->GetBool("Enabled", false);
        _updateDockWidget("Std_TreeView", enabled, show, Qt::RightDockWidgetArea, [](QWidget* widget) {
            if (widget) {
                return widget;
            }

            auto tree = new TreeDockWidget(0, getMainWindow());
            tree->setObjectName(QStringLiteral("Tree view"));
            tree->setWindowTitle(QDockWidget::tr("Tree View"));
            tree->setMinimumWidth(210);
            widget = tree;
            return widget;
        });

        return enabled;
    }

    return false;
}

bool MainWindow::updatePropertyView(bool show)
{
    // Property view
    if (d->hiddenDockWindows.find("Std_PropertyView") == std::string::npos) {
        ParameterGrp::handle group = App::GetApplication()
                                         .GetUserParameter()
                                         .GetGroup("BaseApp")
                                         ->GetGroup("Preferences")
                                         ->GetGroup("DockWindows")
                                         ->GetGroup("PropertyView");
        bool enabled = group->GetBool("Enabled", false);
        _updateDockWidget("Std_PropertyView", enabled, show, Qt::RightDockWidgetArea, [](QWidget* widget) {
            if (widget) {
                return widget;
            }

            auto pcPropView = new PropertyDockView(0, getMainWindow());
            pcPropView->setObjectName(QStringLiteral("Property view"));
            pcPropView->setWindowTitle(QDockWidget::tr("Property View"));
            pcPropView->setMinimumWidth(210);
            widget = pcPropView;
            return widget;
        });

        return enabled;
    }

    return false;
}

bool MainWindow::updateTaskView(bool show)
{
    // Task List (task watcher).
    if (d->hiddenDockWindows.find("Std_TaskWatcher") == std::string::npos) {
        // work through parameter.
        ParameterGrp::handle group = App::GetApplication().GetUserParameter().GetGroup(
            "BaseApp/Preferences/DockWindows/TaskWatcher"
        );
        bool enabled = group->GetBool("Enabled", false);
        group->SetBool("Enabled", enabled);  // ensure entry exists.
        _updateDockWidget("Std_TaskWatcher", enabled, show, Qt::RightDockWidgetArea, [](QWidget* widget) {
            if (widget) {
                return widget;
            }

            widget = new TaskView::TaskView(getMainWindow());
            widget->setObjectName(QStringLiteral("Task List"));
            widget->setWindowTitle(QDockWidget::tr("Task List"));
            return widget;
        });

        return enabled;
    }

    return false;
}

bool MainWindow::updateComboView(bool show)
{
    // Combo view
    if (d->hiddenDockWindows.find("Std_ComboView") == std::string::npos) {
        ParameterGrp::handle group = App::GetApplication()
                                         .GetUserParameter()
                                         .GetGroup("BaseApp")
                                         ->GetGroup("Preferences")
                                         ->GetGroup("DockWindows")
                                         ->GetGroup("ComboView");
        bool enable = group->GetBool("Enabled", true);
        _updateDockWidget("Std_ComboView", enable, show, Qt::LeftDockWidgetArea, [](QWidget* widget) {
            auto pcComboView = qobject_cast<ComboView*>(widget);
            if (widget) {
                return widget;
            }

            pcComboView = new ComboView(nullptr, getMainWindow());
            pcComboView->setObjectName(QStringLiteral("Model"));
            pcComboView->setWindowTitle(QDockWidget::tr("Model"));
            pcComboView->setMinimumWidth(150);
            widget = pcComboView;
            return widget;
        });

        return enable;
    }

    return false;
}

bool MainWindow::updateDAGView(bool show)
{
    // Dag View.
    if (d->hiddenDockWindows.find("Std_DAGView") == std::string::npos) {
        ParameterGrp::handle group = App::GetApplication()
                                         .GetUserParameter()
                                         .GetGroup("BaseApp")
                                         ->GetGroup("Preferences")
                                         ->GetGroup("DockWindows")
                                         ->GetGroup("DAGView");
        bool enabled = group->GetBool("Enabled", false);
        _updateDockWidget("Std_DAGView", enabled, show, Qt::RightDockWidgetArea, [](QWidget* widget) {
            if (widget) {
                return widget;
            }

            auto dagDockWindow = new DAG::DockWindow(nullptr, getMainWindow());
            dagDockWindow->setObjectName(QStringLiteral("DAG View"));
            dagDockWindow->setWindowTitle(QDockWidget::tr("DAG View"));
            widget = dagDockWindow;
            return widget;
        });

        return enabled;
    }

    return false;
}

QMenu* MainWindow::createPopupMenu()
{
    QMenu* menu = new QMenu(this);
    populateDockWindowMenu(menu);
    menu->addSeparator();
    populateToolBarMenu(menu);
    menu->addSeparator();
    Workbench* wb = WorkbenchManager::instance()->active();
    if (wb) {
        MenuItem item;
        wb->createMainWindowPopupMenu(&item);
        if (item.hasItems()) {
            menu->addSeparator();
            QList<MenuItem*> items = item.getItems();
            for (const auto& item : items) {
                if (item->command() == "Separator") {
                    menu->addSeparator();
                }
                else {
                    Command* cmd = Application::Instance->commandManager().getCommandByName(
                        item->command().c_str()
                    );
                    if (cmd) {
                        cmd->addTo(menu);
                    }
                }
            }
        }
    }

    return menu;
}

void MainWindow::tile()
{
    d->mdiArea->tileSubWindows();
}

void MainWindow::cascade()
{
    d->mdiArea->cascadeSubWindows();
}

void MainWindow::closeActiveWindow()
{
    d->mdiArea->closeActiveSubWindow();
}

int MainWindow::confirmSave(App::Document* doc, QWidget* parent, bool addCheckbox)
{
    QMessageBox box(parent ? parent : this);
    box.setObjectName(QStringLiteral("confirmSave"));
    box.setIcon(QMessageBox::Question);
    box.setWindowFlags(box.windowFlags() | Qt::WindowStaysOnTopHint);
    box.setWindowTitle(QObject::tr("Unsaved Document"));
    const QString docName = QString::fromStdString(doc->Label.getStrValue());
    const QString text
        = (!docName.isEmpty()
               ? QObject::tr("Save all changes to document '%1' before closing?").arg(docName)
               : QObject::tr("Save all changes to document before closing?"));
    box.setText(text);


    box.setInformativeText(QObject::tr("Otherwise, all changes will be lost."));
    box.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel | QMessageBox::Save);
    box.setDefaultButton(QMessageBox::Save);
    box.setEscapeButton(QMessageBox::Cancel);

    QCheckBox checkBox(QObject::tr("Apply to all"));
    ParameterGrp::handle hGrp;
    if (addCheckbox) {
        hGrp = App::GetApplication()
                   .GetUserParameter()
                   .GetGroup("BaseApp")
                   ->GetGroup("Preferences")
                   ->GetGroup("General");
        checkBox.setChecked(hGrp->GetBool("ConfirmAll", false));
        checkBox.blockSignals(true);
        box.addButton(&checkBox, QMessageBox::ResetRole);
    }

    // add shortcuts
    QAbstractButton* saveBtn = box.button(QMessageBox::Save);
    if (saveBtn->shortcut().isEmpty()) {
        QString text = saveBtn->text();
        text.prepend(QLatin1Char('&'));
        saveBtn->setShortcut(QKeySequence::mnemonic(text));
    }

    QAbstractButton* discardBtn = box.button(QMessageBox::Discard);
    if (discardBtn->shortcut().isEmpty()) {
        QString text = discardBtn->text();
        text.prepend(QLatin1Char('&'));
        discardBtn->setShortcut(QKeySequence::mnemonic(text));
    }

    int res = ConfirmSaveResult::Cancel;
    box.adjustSize();  // Silence warnings from Qt on Windows

    // activates the last used MDI view of the closing document
    MDIView* activeView = this->activeWindow();
    App::Document* activeDoc = (activeView ? activeView->getAppDocument() : nullptr);
    if (activeDoc != doc) {
        const QList<QWidget*> listOfMDIs = this->windows();
        for (QWidget* widget : listOfMDIs) {
            auto mdiView = qobject_cast<MDIView*>(widget);
            if (mdiView != nullptr && mdiView->getAppDocument() == doc) {
                this->setActiveWindow(mdiView);
            }
        }
    }

    switch (box.exec()) {
        case QMessageBox::Save:
            res = checkBox.isChecked() ? ConfirmSaveResult::SaveAll : ConfirmSaveResult::Save;
            break;
        case QMessageBox::Discard:
            res = checkBox.isChecked() ? ConfirmSaveResult::DiscardAll : ConfirmSaveResult::Discard;
            break;
    }
    if (addCheckbox && res) {
        hGrp->SetBool("ConfirmAll", checkBox.isChecked());
    }
    return res;
}

bool MainWindow::closeAllDocuments(bool close)
{
    auto docs = App::GetApplication().getDocuments();
    try {
        docs = App::Document::getDependentDocuments(docs, true);
    }
    catch (Base::Exception& e) {
        e.reportException();
    }

    bool checkModify = true;
    bool saveAll = false;
    int failedSaves = 0;

    // moves the active document to the front
    MDIView* activeView = this->activeWindow();
    App::Document* activeDoc = (activeView ? activeView->getAppDocument() : nullptr);
    if (activeDoc != nullptr) {
        for (auto it = ++docs.begin(); it != docs.end(); it++) {
            if (*it == activeDoc) {
                docs.erase(it);
                docs.insert(docs.begin(), activeDoc);
            }
        }
    }

    for (auto doc : docs) {
        auto gdoc = Application::Instance->getDocument(doc);
        if (!gdoc) {
            continue;
        }
        if (!gdoc->canClose(false)) {
            return false;
        }
        if (!gdoc->isModified() || doc->testStatus(App::Document::PartialDoc)
            || doc->testStatus(App::Document::TempDoc)) {
            continue;
        }
        bool save = saveAll;
        if (!save && checkModify) {
            int res = confirmSave(doc, this, docs.size() > 1);
            switch (res) {
                case ConfirmSaveResult::Cancel:
                    return false;
                case ConfirmSaveResult::SaveAll:
                    saveAll = true;
                    /* FALLTHRU */
                case ConfirmSaveResult::Save:
                    save = true;
                    break;
                case ConfirmSaveResult::DiscardAll:
                    checkModify = false;
            }
        }

        if (save && !gdoc->save()) {
            failedSaves++;
        }
    }

    if (failedSaves > 0) {
        int ret = QMessageBox::question(
            getMainWindow(),
            QObject::tr("%1 Document(s) not saved").arg(QString::number(failedSaves)),
            QObject::tr("Some documents could not be saved. Cancel closing?"),
            QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Discard
        );
        if (ret == QMessageBox::Cancel) {
            return false;
        }
    }

    if (close) {
        App::GetApplication().closeAllDocuments();
    }

    return true;
}

void MainWindow::activateNextWindow()
{
    auto tab = d->mdiArea->findChild<QTabBar*>();
    if (tab && tab->count() > 0) {
        int index = (tab->currentIndex() + 1) % tab->count();
        tab->setCurrentIndex(index);
    }
}

void MainWindow::activatePreviousWindow()
{
    auto tab = d->mdiArea->findChild<QTabBar*>();
    if (tab && tab->count() > 0) {
        int index = (tab->currentIndex() + tab->count() - 1) % tab->count();
        tab->setCurrentIndex(index);
    }
}

void MainWindow::activateWorkbench(const QString& name)
{
    // remember workbench by tab (if enabled)

    const ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    const bool saveWB = hGrp->GetBool("SaveWBbyTab", false);
    MDIView* view = activeWindow();
    if (view && saveWB) {
        view->setProperty("ownWB", name);
    }

    // emit this signal
    Q_EMIT workbenchActivated(name);
    updateActions(true);
}

void MainWindow::whatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

void MainWindow::showDocumentation(const QString& help)
{
    Base::PyGILStateLocker lock;
    try {
        PyObject* module = PyImport_ImportModule("Help");
        if (module) {
            Py_DECREF(module);
            Gui::Command::addModule(Gui::Command::Gui, "Help");
            Gui::Command::doCommand(Gui::Command::Gui, "Help.show(\"%s\")", help.toStdString().c_str());
        }
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

bool MainWindow::event(QEvent* e)
{
    if (e->type() == QEvent::EnterWhatsThisMode) {
        // Unfortunately, for top-level widgets such as menus or dialogs we
        // won't be notified when the user clicks the link in the hypertext of
        // the what's this text. Thus, we have to install the main window to
        // the application to observe what happens in eventFilter().
        d->whatstext.clear();
        if (!d->whatsthis) {
            d->whatsthis = true;
            qApp->installEventFilter(this);
        }
    }
    else if (e->type() == QEvent::LeaveWhatsThisMode) {
        // Here we can't do anything because this event is sent
        // before the WhatThisClicked event is sent. Thus, we handle
        // this in eventFilter().
    }
    else if (e->type() == QEvent::WhatsThisClicked) {
        auto wt = static_cast<QWhatsThisClickedEvent*>(e);
        showDocumentation(wt->href());
    }
    else if (e->type() == QEvent::ApplicationWindowIconChange) {
        // if application icon changes apply it to the main window and the "About..." dialog
        this->setWindowIcon(QApplication::windowIcon());
        Command* about = Application::Instance->commandManager().getCommandByName("Std_About");
        if (about) {
            Action* action = about->getAction();
            if (action) {
                action->setIcon(QApplication::windowIcon());
            }
        }
    }
    else if (e->type() == Spaceball::ButtonEvent::ButtonEventType) {
        auto buttonEvent = dynamic_cast<Spaceball::ButtonEvent*>(e);
        if (!buttonEvent) {
            return true;
        }
        buttonEvent->setHandled(true);
        // only going to respond to button press events.
        if (buttonEvent->buttonStatus() != Spaceball::BUTTON_PRESSED) {
            return true;
        }
        ParameterGrp::handle group = App::GetApplication()
                                         .GetUserParameter()
                                         .GetGroup("BaseApp")
                                         ->GetGroup("Spaceball")
                                         ->GetGroup("Buttons");
        QByteArray groupName(QVariant(buttonEvent->buttonNumber()).toByteArray());
        if (group->HasGroup(groupName.data())) {
            ParameterGrp::handle commandGroup = group->GetGroup(groupName.data());
            std::string commandName(commandGroup->GetASCII("Command"));
            if (commandName.empty()) {
                return true;
            }
            else {
                Application::Instance->commandManager().runCommandByName(commandName.c_str());
            }
        }
        else {
            return true;
        }
    }
    else if (e->type() == Spaceball::MotionEvent::MotionEventType) {
        auto motionEvent = dynamic_cast<Spaceball::MotionEvent*>(e);
        if (!motionEvent) {
            return true;
        }
        motionEvent->setHandled(true);
        Gui::Document* doc = Application::Instance->activeDocument();
        if (!doc) {
            return true;
        }
        auto temp = dynamic_cast<View3DInventor*>(doc->getActiveView());
        if (!temp) {
            return true;
        }
        View3DInventorViewer* view = temp->getViewer();
        if (view) {
            Spaceball::MotionEvent anotherEvent(*motionEvent);
            qApp->sendEvent(view, &anotherEvent);
        }
        return true;
    }
    else if (e->type() == QEvent::StatusTip) {
        // make sure warning and error message don't get blocked by tooltips
        if (std::abs(d->currentStatusType) <= MainWindow::Wrn) {
            return true;
        }
    }
    return QMainWindow::event(e);
}

bool MainWindow::eventFilter(QObject* o, QEvent* e)
{
    if (o != this) {
        if (e->type() == QEvent::WindowStateChange) {
            // notify all mdi views when the active view receives a show normal, show minimized
            // or show maximized event
            auto view = dynamic_cast<MDIView*>(o);
            if (view) {  // emit this signal
                Qt::WindowStates oldstate = static_cast<QWindowStateChangeEvent*>(e)->oldState();
                Qt::WindowStates newstate = view->windowState();
                if (oldstate != newstate) {
                    Q_EMIT windowStateChanged(view);
                }
            }
        }

        // We don't want to show the bubble help for the what's this text but want to
        // start the help viewer with the according key word.
        // Thus, we have to observe WhatThis events if called for a widget, use its text and
        // must avoid to make the bubble widget visible.
        if (e->type() == QEvent::WhatsThis) {
            if (!o->isWidgetType()) {
                return false;
            }
            // clicked on a widget in what's this mode
            auto w = static_cast<QWidget*>(o);
            d->whatstext = w->whatsThis();
        }
        if (e->type() == QEvent::WhatsThisClicked) {
            // if the widget is a top-level window
            if (o->isWidgetType() && qobject_cast<QWidget*>(o)->isWindow()) {
                // re-direct to the widget
                QApplication::sendEvent(this, e);
            }
        }
        // special treatment for menus because they directly call QWhatsThis::showText()
        // whereby we must be informed for which action the help should be shown
        if (o->inherits("QMenu") && QWhatsThis::inWhatsThisMode()) {
            bool whatthis = false;
            if (e->type() == QEvent::KeyPress) {
                auto ke = static_cast<QKeyEvent*>(e);
                if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter
                    || ke->key() == Qt::Key_F1) {
                    whatthis = true;
                }
            }
            else if (e->type() == QEvent::MouseButtonRelease) {
                whatthis = true;
            }
            else if (e->type() == QEvent::EnterWhatsThisMode) {
                whatthis = true;
            }
            if (whatthis) {
                QAction* cur = static_cast<QMenu*>(o)->activeAction();
                if (cur) {
                    // get the help text for later usage
                    QString s = cur->whatsThis();
                    if (s.isEmpty()) {
                        s = static_cast<QMenu*>(o)->whatsThis();
                    }
                    d->whatstext = s;
                }
            }
        }
        if (o->inherits("QWhatsThat") && e->type() == QEvent::Show) {
            // the bubble help should become visible which we avoid by marking the widget
            // that it is out of range. Instead of, we show the help viewer
            if (!d->whatstext.isEmpty()) {
                QWhatsThisClickedEvent e(d->whatstext);
                QApplication::sendEvent(this, &e);
            }
            static_cast<QWidget*>(o)->setAttribute(Qt::WA_OutsideWSRange);
            o->deleteLater();
            return true;
        }
        if (o->inherits("QWhatsThat") && e->type() == QEvent::Hide) {
            // leave what's this mode
            if (d->whatsthis) {
                d->whatsthis = false;
                d->whatstext.clear();
                qApp->removeEventFilter(this);
            }
        }
    }

    return QMainWindow::eventFilter(o, e);
}

void MainWindow::addWindow(MDIView* view)
{
    // make workspace parent of view

    bool isempty = d->mdiArea->subWindowList().isEmpty();
    auto child = qobject_cast<QMdiSubWindow*>(view->parentWidget());
    if (!child) {
        child = new QMdiSubWindow(d->mdiArea->viewport());
        child->setAttribute(Qt::WA_DeleteOnClose);
        child->setWidget(view);
        child->setWindowIcon(view->windowIcon());
        QMenu* menu = child->systemMenu();

        // See StdCmdCloseActiveWindow (#0002631)
        QList<QAction*> acts = menu->actions();
        for (auto& act : acts) {
            if (act->shortcut() == QKeySequence(QKeySequence::Close)) {
                act->setShortcuts(QList<QKeySequence>());
                break;
            }
        }

        QAction* action = menu->addAction(tr("Close All"));
        connect(action, &QAction::triggered, d->mdiArea, &QMdiArea::closeAllSubWindows);
        d->mdiArea->addSubWindow(child);
    }

    connect(view, &MDIView::message, this, &MainWindow::showMessage);
    connect(this, &MainWindow::windowStateChanged, view, &MDIView::windowStateChanged);

    // listen to the incoming events of the view
    view->installEventFilter(this);

    // Show the new window. This will also call onWindowActivated. The very first window is shown in
    // maximized mode.

    if (isempty) {
        view->showMaximized();
    }
    else {
        view->show();
    }
}

/**
 * Removes the instance of Gui::MDIView from the main window and sends n event
 * to the parent widget, a QMdiSubWindow to delete itself.
 * If you want to avoid that the Gui::MDIView instance gets destructed too you
 * must reparent it afterwards, e.g. set parent to NULL.
 */
void MainWindow::removeWindow(Gui::MDIView* view, bool close)
{
    if (view->currentViewMode() != MDIView::Child) {
        FC_WARN("tried to remove an MDIView that is not currently in child mode");
        return;
    }

    // free all connections
    disconnect(view, &MDIView::message, this, &MainWindow::showMessage);
    disconnect(this, &MainWindow::windowStateChanged, view, &MDIView::windowStateChanged);

    view->removeEventFilter(this);

    // check if the focus widget is a child of the view
    QWidget* foc = this->focusWidget();
    if (foc) {
        QWidget* par = foc->parentWidget();
        while (par) {
            if (par == view) {
                foc->clearFocus();
                break;
            }
            par = par->parentWidget();
        }
    }

    QWidget* parent = view->parentWidget();

    // The call of 'd->mdiArea->removeSubWindow(parent)' causes the QMdiSubWindow
    // to lose its parent and thus the notification in QMdiSubWindow::closeEvent
    // of other mdi windows to get maximized if this window is maximized will fail.
    // However, we must let it here otherwise deleting MDI child views directly can
    // cause other problems.
    //
    // The above mentioned problem can be fixed by setParent(0) which triggers a
    // ChildRemoved event being handled properly inside QMidArea::viewportEvent()
    //
    auto subwindow = qobject_cast<QMdiSubWindow*>(parent);
    if (subwindow && d->mdiArea->subWindowList().contains(subwindow)) {
        subwindow->setParent(nullptr);
        subwindow->deleteLater();

        assert(!d->mdiArea->subWindowList().contains(subwindow));
    }

    if (close) {
        parent->deleteLater();
    }
    updateActions();
}

void MainWindow::tabChanged(MDIView* view)
{
    Q_UNUSED(view)
    updateActions();
}

void MainWindow::tabCloseRequested(int index)
{
    auto tab = d->mdiArea->findChild<QTabBar*>();
    if (index < 0 || index >= tab->count()) {
        return;
    }

    const QList<QMdiSubWindow*> subWindows = d->mdiArea->subWindowList();
    Q_ASSERT(index < subWindows.size());

    QMdiSubWindow* subWindow = d->mdiArea->subWindowList().at(index);
    Q_ASSERT(subWindow);
    subWindow->close();
    updateActions();
}

void MainWindow::setActiveSubWindow(QWidget* window)
{
    auto mdi = qobject_cast<QMdiSubWindow*>(window);
    if (!mdi) {
        return;
    }

    auto view = qobject_cast<MDIView*>(mdi->widget());
    if (view) {
        setActiveWindow(view);
    }
    else {
        // check for non mdiview widgets ie. the plot widget
        d->mdiArea->setActiveSubWindow(mdi);
        if (mdi->widget()) {
            mdi->widget()->setFocus();
        }
    }
}

void MainWindow::setActiveWindow(MDIView* view)
{
    if (!view) {
        return;
    }

    // always update the focus and active sub window

    // We need the explicit call to setFocus because it seems the focus window and the
    // activeSubWindow in the QMdiView can diverge when calling setActiveWindow while the MainWindow
    // is not currently active. In this case Qt will later set the previous focus window as active,
    // which will call onWindowActivated and activate the wrong window. This e.g. happens when
    // switching from a 3d view to a spreadsheet using the "Windows..." dialog or when docking a
    // spreadsheet that was in top-level/fullscreen mode. Why this could only be reproduced with a
    // spreadsheet remains a mystery.

    view->setFocus();

    auto subwindow = qobject_cast<QMdiSubWindow*>(view->parentWidget());
    if (subwindow) {
        d->mdiArea->setActiveSubWindow(subwindow);
    }

    // if active view changed, notify rest of the application

    if (view == d->activeView) {
        return;
    }

    d->activeView = view;
    Application::Instance->viewActivated(view);

    // activate/remember workbench by tab (if enabled)

    const ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    const bool saveWB = hGrp->GetBool("SaveWBbyTab", false);
    if (saveWB) {
        const QString currWb = view->property("ownWB").toString();
        if (!currWb.isEmpty()) {
            this->activateWorkbench(currWb);
        }
        else {
            const std::string name = WorkbenchManager::instance()->active()->name();
            view->setProperty("ownWB", QString::fromStdString(name));
        }
    }

    // update actions

    updateActions();
}

void MainWindow::onWindowActivated(QMdiSubWindow* mdi)
{
    if (!mdi) {
        setWindowTitle(QString());
        setWindowModified(false);
        return;
    }

    // set active the appropriate window (it needs not to be part of mdiIds, e.g. directly after
    // creation)

    auto view = dynamic_cast<MDIView*>(mdi->widget());
    setActiveWindow(view);
}

void MainWindow::onWindowsMenuAboutToShow()
{
    QList<QMdiSubWindow*> windows = d->mdiArea->subWindowList(QMdiArea::CreationOrder);
    QWidget* active = d->mdiArea->activeSubWindow();

    // We search for the 'Std_WindowsMenu' command that provides the list of actions
    CommandManager& cMgr = Application::Instance->commandManager();
    Command* cmd = cMgr.getCommandByName("Std_WindowsMenu");
    QList<QAction*> actions = qobject_cast<ActionGroup*>(cmd->getAction())->actions();

    // do the connection only once
    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;
        QAction* last = actions.isEmpty() ? 0 : actions.last();
        for (const auto& action : actions) {
            if (action == last) {
                break;  // this is a separator
            }
            connect(action, &QAction::triggered, d->windowMapper, qOverload<>(&QSignalMapper::map));
        }
    }

    int numWindows = std::min<int>(actions.count() - 1, windows.count());
    for (int index = 0; index < numWindows; index++) {
        QWidget* child = windows.at(index);
        QAction* action = actions.at(index);
        QString text;
        QString title = child->windowTitle();
        int lastIndex = title.lastIndexOf(QStringLiteral("[*]"));
        if (lastIndex > 0) {
            title = title.left(lastIndex);
            if (child->isWindowModified()) {
                title = QStringLiteral("%1*").arg(title);
            }
        }
        if (index < 9) {
            text = QStringLiteral("&%1 %2").arg(index + 1).arg(title);
        }
        else {
            text = QStringLiteral("%1 %2").arg(index + 1).arg(title);
        }
        action->setText(text);
        action->setVisible(true);
        action->setChecked(child == active);
        d->windowMapper->setMapping(action, child);
    }

    // if less windows than actions
    for (int index = numWindows; index < actions.count(); index++) {
        actions[index]->setVisible(false);
    }
    // show the separator
    if (numWindows > 0) {
        actions.last()->setVisible(true);
    }
}

void MainWindow::onToolBarMenuAboutToShow()
{
    auto menu = static_cast<QMenu*>(sender());
    menu->clear();
    populateToolBarMenu(menu);

    menu->addSeparator();

    Application::Instance->commandManager().getCommandByName("Std_ToggleToolBarLock")->addTo(menu);
}

void MainWindow::populateToolBarMenu(QMenu* menu)
{
    QList<QToolBar*> toolbars = this->findChildren<QToolBar*>();
    for (const auto& toolbar : toolbars) {
        if (auto parent = toolbar->parentWidget()) {
            if (parent == this || parent == statusBar() || parent->parentWidget() == statusBar()
                || parent->parentWidget() == menuBar()) {
                QAction* action = toolbar->toggleViewAction();
                action->setToolTip(tr("Toggles this toolbar"));
                action->setStatusTip(tr("Toggles this toolbar"));
                action->setWhatsThis(tr("Toggles this toolbar"));
                menu->addAction(action);
            }
        }
    }
}

void MainWindow::onDockWindowMenuAboutToShow()
{
    auto menu = static_cast<QMenu*>(sender());
    menu->clear();
    populateDockWindowMenu(menu);
}

void MainWindow::populateDockWindowMenu(QMenu* menu)
{
    QList<QDockWidget*> dock = this->findChildren<QDockWidget*>();
    for (auto& it : dock) {
        QAction* action = it->toggleViewAction();
        action->setToolTip(tr("Toggles this dockable window"));
        action->setStatusTip(tr("Toggles this dockable window"));
        action->setWhatsThis(tr("Toggles this dockable window"));
        menu->addAction(action);
    }
}

void MainWindow::setDockWindowMenu(QMenu* menu)
{
    connect(menu, &QMenu::aboutToShow, this, &MainWindow::onDockWindowMenuAboutToShow);
}

void MainWindow::setToolBarMenu(QMenu* menu)
{
    connect(menu, &QMenu::aboutToShow, this, &MainWindow::onToolBarMenuAboutToShow);
}

void MainWindow::setWindowsMenu(QMenu* menu)
{
    connect(menu, &QMenu::aboutToShow, this, &MainWindow::onWindowsMenuAboutToShow);
}

QList<QWidget*> MainWindow::windows(QMdiArea::WindowOrder order) const
{
    QList<QWidget*> mdis;
    QList<QMdiSubWindow*> wnds = d->mdiArea->subWindowList(order);
    for (const auto& wnd : wnds) {
        mdis << wnd->widget();
    }
    return mdis;
}

MDIView* MainWindow::activeWindow() const
{
    // each activated window notifies this main window when it is activated
    return d->activeView;
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    Application::Instance->tryClose(e);
    if (e->isAccepted()) {
        // Send close event to all non-modal dialogs
        QList<QDialog*> dialogs = this->findChildren<QDialog*>();
        // It is possible that closing a dialog internally closes further dialogs. Thus,
        // we have to check the pointer before.
        QVector<QPointer<QDialog>> dialogs_ptr;
        for (const auto& dialog : dialogs) {
            dialogs_ptr.append(dialog);
        }
        for (auto& it : dialogs_ptr) {
            if (!it.isNull()) {
                it->close();
            }
        }
        QList<MDIView*> mdis = this->findChildren<MDIView*>();
        // Force to close any remaining (passive) MDI child views
        for (auto& mdi : mdis) {
            mdi->hide();
            mdi->deleteLater();
        }

        if (Workbench* wb = WorkbenchManager::instance()->active()) {
            wb->removeTaskWatcher();
        }

        Q_EMIT mainWindowClosed();
        d->activityTimer->stop();

        // https://forum.freecad.org/viewtopic.php?f=8&t=67748
        // When the session manager jumps in it can happen that the closeEvent()
        // function is triggered twice and for the second call the main window might be
        // invisible. In this case the window settings shouldn't be saved.
        if (isVisible()) {
            saveWindowSettings();
        }

        delete d->assistant;
        d->assistant = nullptr;

        // See createMimeDataFromSelection
        QVariant prop = this->property("x-documentobject-file");
        if (!prop.isNull()) {
            Base::FileInfo fi((const char*)prop.toByteArray());
            if (fi.exists()) {
                fi.deleteFile();
            }
        }

        if (this->property("QuitOnClosed").isValid()) {
            QApplication::closeAllWindows();
            qApp->processEvents();  // flush all pending deferredDelete events
            qApp->quit();           // stop the event loop
        }
    }
}

void MainWindow::showEvent(QShowEvent* e)
{
    std::clog << "Show main window" << std::endl;
    QMainWindow::showEvent(e);
}

void MainWindow::hideEvent(QHideEvent* e)
{
    std::clog << "Hide main window" << std::endl;
    QMainWindow::hideEvent(e);
}

void MainWindow::processMessages(const QList<QString>& msg)
{
    // handle all the messages to open files
    try {
        WaitCursor wc;
        std::list<std::string> files;
        QString action = QStringLiteral("OpenFile:");
        for (const auto& it : msg) {
            if (it.startsWith(action)) {
                files.emplace_back(it.mid(action.size()).toStdString());
            }
        }
        files = App::Application::processFiles(files);
        for (const auto& file : files) {
            QString filename = QString::fromUtf8(file.c_str(), file.size());
            FileDialog::setWorkingDirectory(filename);
        }
    }
    catch (const Base::SystemExitException&) {
    }
}

void MainWindow::delayedStartup()
{
    // automatically run unit tests in Gui
    if (App::Application::Config()["RunMode"] == "Internal") {
        QTimer::singleShot(1000, this, [] {
            try {
                string command = "import sys\n"
                                 "import FreeCAD\n"
                                 "import QtUnitGui\n\n"
                                 "testCase = FreeCAD.ConfigGet(\"TestCase\")\n"
                                 "QtUnitGui.addTest(testCase)\n"
                                 "QtUnitGui.setTest(testCase)\n"
                                 "result = QtUnitGui.runTest()\n"
                                 "sys.stdout.flush()\n";
                if (App::Application::Config()["ExitTests"] == "yes") {
                    command += "sys.exit(0 if result else 1)";
                }
                Base::Interpreter().runString(command.c_str());
            }
            catch (const Base::SystemExitException&) {
                throw;
            }
            catch (const Base::Exception& e) {
                e.reportException();
            }
        });
        return;
    }

    // processing all command line files
    try {
        std::list<std::string> files = App::Application::getCmdLineFiles();
        files = App::Application::processFiles(files);
        for (const auto& file : files) {
            QString filename = QString::fromUtf8(file.c_str(), file.size());
            FileDialog::setWorkingDirectory(filename);
        }
    }
    catch (const Base::SystemExitException&) {
        throw;
    }

    if (Application::hiddenMainWindow()) {
        QApplication::quit();
        return;
    }

    // TODO: Check for deprecated settings
    Application::Instance->checkForDeprecatedSettings();

    // Create new document?
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Document");
    if (hGrp->GetBool("CreateNewDoc", false)) {
        if (App::GetApplication().getDocuments().empty()) {
            Application::Instance->commandManager().runCommandByName("Std_New");
            // This document is autoCreated
            App::Document* newDoc = App::GetApplication().getActiveDocument();
            newDoc->setAutoCreated(true);
        }
    }

    if (hGrp->GetBool("RecoveryEnabled", true)) {
        Application::Instance->checkForPreviousCrashes();
    }

    if (SafeMode::SafeModeEnabled()) {
        auto safeModePopup = QMessageBox(
            QMessageBox::Information,
            tr("Safe mode enabled"),
            tr("FreeCAD is now running in safe mode."),
            QMessageBox::Ok
        );
        safeModePopup.setInformativeText(
            tr("Safe mode temporarily disables your configurations and addons."
               " Restart the application to exit safe mode.")
        );
        safeModePopup.exec();
    }
}

void MainWindow::appendRecentFile(const QString& filename)
{
    auto recent = this->findChild<RecentFilesAction*>(QStringLiteral("recentFiles"));
    if (recent) {
        recent->appendFile(filename);
    }
}

void MainWindow::appendRecentMacro(const QString& filename)
{
    auto recent = this->findChild<RecentMacrosAction*>(QStringLiteral("recentMacros"));
    if (recent) {
        recent->appendFile(filename);
    }
}

void MainWindow::updateActions(bool delay)
{
    // make it safe to call before the main window is actually created
    if (!instance) {
        return;
    }

    if (!d->activityTimer->isActive()) {
        // If for some reason updateActions() is called from a worker thread
        // we must avoid to directly call QTimer::start() because this leaves
        // the whole application in a weird state
        if (d->activityTimer->thread() != QThread::currentThread()) {
            QMetaObject::invokeMethod(d->activityTimer, "start", Qt::QueuedConnection, Q_ARG(int, 150));
        }
        else {
            d->activityTimer->start(150);
        }
    }
    else if (delay) {
        if (!d->actionUpdateDelay) {
            d->actionUpdateDelay = 1;
        }
    }
    else {
        d->actionUpdateDelay = -1;
    }
}

void MainWindow::_updateActions()
{
    if (isVisible() && d->actionUpdateDelay <= 0) {
        FC_LOG("update actions");
        d->activityTimer->stop();
        Application::Instance->commandManager().testActive();
    }

    d->actionUpdateDelay = 0;

    if (auto view = activeWindow()) {
        setWindowTitle(view->buildWindowTitle());
        if (auto document = view->getGuiDocument()) {
            setWindowModified(document->isModified());
        }
    }
}

void MainWindow::updateEditorActions()
{
    Command* cmd = nullptr;
    CommandManager& mgr = Application::Instance->commandManager();

    cmd = mgr.getCommandByName("Std_Cut");
    if (cmd) {
        cmd->testActive();
    }

    cmd = mgr.getCommandByName("Std_Copy");
    if (cmd) {
        cmd->testActive();
    }

    cmd = mgr.getCommandByName("Std_Paste");
    if (cmd) {
        cmd->testActive();
    }

    cmd = mgr.getCommandByName("Std_Undo");
    if (cmd) {
        cmd->testActive();
    }

    cmd = mgr.getCommandByName("Std_Redo");
    if (cmd) {
        cmd->testActive();
    }
}

void MainWindow::switchToTopLevelMode()
{
    QList<QDockWidget*> dw = this->findChildren<QDockWidget*>();
    for (auto& it : dw) {
        it->setParent(nullptr, Qt::Window);
        it->show();
    }
    QList<QWidget*> mdi = getMainWindow()->windows();
    for (auto& it : mdi) {
        it->setParent(nullptr, Qt::Window);
        it->show();
    }
}

void MainWindow::switchToDockedMode()
{
    // Search for all top-level MDI views
    QWidgetList toplevel = QApplication::topLevelWidgets();
    for (const auto& it : toplevel) {
        auto view = dynamic_cast<MDIView*>(it);
        if (view) {
            view->setCurrentViewMode(MDIView::Child);
        }
    }
}

void MainWindow::loadWindowSettings()
{
    QString vendor = QString::fromUtf8(App::Application::Config()["ExeVendor"].c_str());
    QString application = QString::fromUtf8(App::Application::Config()["ExeName"].c_str());
    int major = (QT_VERSION >> 0x10) & 0xff;
    int minor = (QT_VERSION >> 0x08) & 0xff;
    QString qtver = QStringLiteral("Qt%1.%2").arg(major).arg(minor);
    QSettings config(vendor, application);

    QRect rect = QApplication::primaryScreen()->availableGeometry();
    int maxHeight = rect.height();
    int maxWidth = rect.width();

    config.beginGroup(qtver);
    QPoint pos = config.value(QStringLiteral("Position"), this->pos()).toPoint();
    maxWidth -= pos.x();
    maxHeight -= pos.y();
    QSize size = config.value(QStringLiteral("Size"), QSize(maxWidth, maxHeight)).toSize();
    bool max = config.value(QStringLiteral("Maximized"), false).toBool();
    bool showStatusBar = config.value(QStringLiteral("StatusBar"), true).toBool();
    QByteArray windowState = config.value(QStringLiteral("MainWindowState")).toByteArray();
    config.endGroup();


    std::string geometry = d->hGrp->GetASCII("Geometry");
    std::istringstream iss(geometry);
    int x, y, w, h;
    if (iss >> x >> y >> w >> h) {
        pos = QPoint(x, y);
        size = QSize(w, h);
    }

    max = d->hGrp->GetBool("Maximized", max);
    showStatusBar = d->hGrp->GetBool("StatusBar", showStatusBar);
    std::string wstate = d->hGrp->GetASCII("MainWindowState");
    if (!wstate.empty()) {
        windowState = QByteArray::fromBase64(wstate.c_str());
    }

    resize(size);
    // TODO: Hotfix to be removed as soon as possible after 1.1.0 Release
#ifdef FC_OS_WIN64
    if (checkFirstRun()) {
        const int topLeftXY = 10;
        this->move(topLeftXY, topLeftXY);
    }
    else {
        moveToDefaultPosition(rect, pos);
    }
#else
    // TODO: Hotfix to be removed as soon as possible after 1.1.0 Release
    if (QGuiApplication::platformName() == QString::fromStdString("wayland") && checkFirstRun()) {
        const int topLeftXY = 10;
        this->move(topLeftXY, topLeftXY);
    }
    else {  // all Linux x11 and Mac
        moveToDefaultPosition(rect, pos);
    }
#endif

    Base::StateLocker guard(d->_restoring);

    d->restoreWindowState(windowState);
    std::clog << "Main window restored" << std::endl;

    max ? showMaximized() : show();

    // make menus and tooltips usable in fullscreen under Windows, see issue #7563
#if defined(Q_OS_WIN)
# if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (QWindow* win = this->windowHandle()) {
        QWindowsWindowFunctions::setHasBorderInFullScreen(win, true);
    }
# else
    using namespace QNativeInterface::Private;
    if (auto* windowsWindow = dynamic_cast<QWindowsWindow*>(this->windowHandle())) {
        windowsWindow->setHasBorderInFullScreen(true);
    }
# endif
#endif

    statusBar()->setVisible(showStatusBar);

    setAttribute(Qt::WA_AlwaysShowToolTips);

    ToolBarManager::getInstance()->restoreState();
    std::clog << "Toolbars restored" << std::endl;

    OverlayManager::instance()->restore();
}

bool MainWindow::isRestoringWindowState() const
{
    return d->_restoring;
}

void MainWindowP::restoreWindowState(const QByteArray& windowState)
{
    if (windowState.isEmpty()) {
        return;
    }

    Base::StateLocker guard(_restoring);

    // tmp. disable the report window to suppress some bothering warnings
    if (Base::Console().isMsgTypeEnabled("ReportOutput", Base::ConsoleSingleton::MsgType_Wrn)) {
        Base::Console().setEnabledMsgType("ReportOutput", Base::ConsoleSingleton::MsgType_Wrn, false);
        getMainWindow()->restoreState(windowState);
        Base::Console().setEnabledMsgType("ReportOutput", Base::ConsoleSingleton::MsgType_Wrn, true);
    }
    else {
        getMainWindow()->restoreState(windowState);
    }

    Base::ConnectionBlocker block(connParam);
    // as a notification for user code on window state restore
    hGrp->SetBool("WindowStateRestored", !hGrp->GetBool("WindowStateRestored", false));
}

void MainWindow::saveWindowSettings(bool canDelay)
{
    if (isRestoringWindowState()) {
        return;
    }

    if (canDelay) {
        d->saveStateTimer.start(100);
        return;
    }

    QString vendor = QString::fromUtf8(App::Application::Config()["ExeVendor"].c_str());
    QString application = QString::fromUtf8(App::Application::Config()["ExeName"].c_str());
    int major = (QT_VERSION >> 0x10) & 0xff;
    int minor = (QT_VERSION >> 0x08) & 0xff;
    QString qtver = QStringLiteral("Qt%1.%2").arg(major).arg(minor);
    QSettings config(vendor, application);

    Base::ConnectionBlocker block(d->connParam);
    d->hGrp->SetBool("Maximized", this->isMaximized());
    d->hGrp->SetBool("StatusBar", this->statusBar()->isVisible());
    d->hGrp->SetASCII("MainWindowState", this->saveState().toBase64().constData());

    std::ostringstream ss;
    QRect rect(this->pos(), this->size());
    ss << rect.left() << " " << rect.top() << " " << rect.width() << " " << rect.height();
    d->hGrp->SetASCII("Geometry", ss.str().c_str());

    DockWindowManager::instance()->saveState();
    OverlayManager::instance()->save();
    ToolBarManager::getInstance()->saveState();
}

void MainWindow::startSplasher()
{
    // startup splasher
    // when running in verbose mode no splasher
    if (!(App::Application::Config()["Verbose"] == "Strict")
        && (App::Application::Config()["RunMode"] == "Gui")) {
        ParameterGrp::handle hGrp = App::GetApplication()
                                        .GetUserParameter()
                                        .GetGroup("BaseApp")
                                        ->GetGroup("Preferences")
                                        ->GetGroup("General");
        // first search for an external image file
        if (hGrp->GetBool("ShowSplasher", true)) {
            d->splashscreen = new SplashScreen(SplashScreen::splashImage());

            if (!hGrp->GetBool("ShowSplasherMessages", false)) {
                d->splashscreen->setShowMessages(false);
            }

            d->splashscreen->show();
        }
        else {
            d->splashscreen = nullptr;
        }
    }
}

void MainWindow::stopSplasher()
{
    if (d->splashscreen) {
        d->splashscreen->finish(this);
        delete d->splashscreen;
        d->splashscreen = nullptr;
    }
}

/**
 * Drops the event \a e and tries to open the files.
 */
void MainWindow::dropEvent(QDropEvent* e)
{
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        // load the files into the active document if there is one, otherwise let create one
        loadUrls(App::GetApplication().getActiveDocument(), data->urls());
    }
    else {
        QMainWindow::dropEvent(e);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e)
{
    // Here we must allow uri drafs and check them in dropEvent
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        e->accept();
    }
    else {
        e->ignore();
    }
}

static QLatin1String _MimeDocObj("application/x-documentobject");
static QLatin1String _MimeDocObjX("application/x-documentobject-x");
static QLatin1String _MimeDocObjFile("application/x-documentobject-file");
static QLatin1String _MimeDocObjXFile("application/x-documentobject-x-file");

QMimeData* MainWindow::createMimeDataFromSelection() const
{
    std::vector<App::DocumentObject*> sel;
    std::set<App::DocumentObject*> objSet;
    for (auto& s : Selection().getCompleteSelection()) {
        if (s.pObject && s.pObject->isAttachedToDocument() && objSet.insert(s.pObject).second) {
            sel.push_back(s.pObject);
        }
    }
    if (sel.empty()) {
        return nullptr;
    }

    auto all = App::Document::getDependencyList(sel);
    if (all.size() > sel.size()) {
        DlgObjectSelection dlg(sel, getMainWindow());
        if (dlg.exec() != QDialog::Accepted) {
            return nullptr;
        }
        sel = dlg.getSelections();
        if (sel.empty()) {
            return nullptr;
        }
    }

    std::vector<App::Document*> unsaved;
    bool hasXLink = App::PropertyXLink::hasXLink(sel, &unsaved);
    if (!unsaved.empty()) {
        QMessageBox::critical(
            getMainWindow(),
            tr("Unsaved document"),
            tr("The exported object contains external link. Save the document"
               "at least once before exporting.")
        );
        return nullptr;
    }

    unsigned int memsize = 1000;  // ~ for the meta-information
    for (const auto& it : sel) {
        memsize += it->getMemSize();
    }

    // if less than ~10 MB
    bool use_buffer = (memsize < 0xA00000);
    QByteArray res;
    if (use_buffer) {
        try {
            res.reserve(memsize);
        }
        catch (const std::bad_alloc&) {
            use_buffer = false;
        }
    }

    WaitCursor wc;
    QString mime;
    if (use_buffer) {
        mime = hasXLink ? _MimeDocObjX : _MimeDocObj;
        Base::ByteArrayOStreambuf buf(res);
        std::ostream str(&buf);
        // need this instance to call MergeDocuments::Save()
        App::Document* doc = sel.front()->getDocument();
        MergeDocuments mimeView(doc);
        doc->exportObjects(sel, str);
    }
    else {
        mime = hasXLink ? _MimeDocObjXFile : _MimeDocObjFile;
        static Base::FileInfo fi(App::Application::getTempFileName());
        Base::ofstream str(fi, std::ios::out | std::ios::binary);
        // need this instance to call MergeDocuments::Save()
        App::Document* doc = sel.front()->getDocument();
        MergeDocuments mimeView(doc);
        doc->exportObjects(sel, str);
        str.close();
        res = fi.filePath().c_str();

        // store the path name as a custom property and
        // delete this file when closing the application
        const_cast<MainWindow*>(this)->setProperty("x-documentobject-file", res);
    }

    auto mimeData = new QMimeData();
    mimeData->setData(mime, res);
    return mimeData;
}

bool MainWindow::canInsertFromMimeData(const QMimeData* source) const
{
    if (!source) {
        return false;
    }
    return source->hasUrls() || source->hasImage() || source->hasFormat(_MimeDocObj)
        || source->hasFormat(_MimeDocObjX) || source->hasFormat(_MimeDocObjFile)
        || source->hasFormat(_MimeDocObjXFile);
}

void MainWindow::insertFromMimeData(const QMimeData* mimeData)
{
    if (!mimeData) {
        return;
    }

    if (mimeData->hasImage()) {
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            doc = App::GetApplication().newDocument();
        }

        if (!doc) {
            return;
        }

        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (image.isNull()) {
            return;
        }

        std::string tempPath = App::Application::getTempFileName("png");
        if (image.save(QString::fromStdString(tempPath), "PNG")) {
            WaitCursor wc;
            doc->openTransaction("Paste image");

            try {
                std::string objName = doc->getUniqueObjectName("ImagePlane");
                App::DocumentObject* obj = doc->addObject("Image::ImagePlane", objName.c_str());
                if (obj) {
                    obj->Label.setValue("PastedImage");
                    static_cast<Image::ImagePlane*>(obj)->ImageFile.setValue(tempPath.c_str());
                    doc->recompute();
                }
            }
            catch (const Base::Exception& e) {
                doc->abortTransaction();
                e.reportException();
                return;
            }

            doc->commitTransaction();
        }
        else {
            Base::Console().error(
                "Failed to save pasted image to temporary file: %s\n",
                tempPath.c_str()
            );
        }
        return;
    }

    bool fromDoc = false;
    bool hasXLink = false;
    QString format;
    if (mimeData->hasFormat(_MimeDocObj)) {
        format = _MimeDocObj;
    }
    else if (mimeData->hasFormat(_MimeDocObjX)) {
        format = _MimeDocObjX;
        hasXLink = true;
    }
    else if (mimeData->hasFormat(_MimeDocObjFile)) {
        format = _MimeDocObjFile;
        fromDoc = true;
    }
    else if (mimeData->hasFormat(_MimeDocObjXFile)) {
        format = _MimeDocObjXFile;
        fromDoc = true;
        hasXLink = true;
    }
    else {
        if (mimeData->hasUrls()) {
            loadUrls(App::GetApplication().getActiveDocument(), mimeData->urls());
        }
        return;
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        doc = App::GetApplication().newDocument();
    }

    if (hasXLink && !doc->isSaved()) {
        int ret = QMessageBox::question(
            getMainWindow(),
            tr("Unsaved document"),
            tr("To link to external objects, the document must be saved at least once.\n"
               "Save the document now?"),
            QMessageBox::Yes,
            QMessageBox::No
        );
        if (ret != QMessageBox::Yes || !Application::Instance->getDocument(doc)->saveAs()) {
            return;
        }
    }
    if (!fromDoc) {
        QByteArray res = mimeData->data(format);

        doc->openTransaction("Paste");
        Base::ByteArrayIStreambuf buf(res);
        std::istream in(nullptr);
        in.rdbuf(&buf);
        MergeDocuments mimeView(doc);
        std::vector<App::DocumentObject*> newObj = mimeView.importObjects(in);
        std::vector<App::DocumentObjectGroup*> grp
            = Gui::Selection().getObjectsOfType<App::DocumentObjectGroup>();
        if (grp.size() == 1) {
            Gui::Document* gui = Application::Instance->getDocument(doc);
            if (gui) {
                gui->addRootObjectsToGroup(newObj, grp.front());
            }
        }
        doc->commitTransaction();
    }
    else {
        QByteArray res = mimeData->data(format);

        doc->openTransaction("Paste");
        Base::FileInfo fi((const char*)res);
        Base::ifstream str(fi, std::ios::in | std::ios::binary);
        MergeDocuments mimeView(doc);
        std::vector<App::DocumentObject*> newObj = mimeView.importObjects(str);
        str.close();
        std::vector<App::DocumentObjectGroup*> grp
            = Gui::Selection().getObjectsOfType<App::DocumentObjectGroup>();
        if (grp.size() == 1) {
            Gui::Document* gui = Application::Instance->getDocument(doc);
            if (gui) {
                gui->addRootObjectsToGroup(newObj, grp.front());
            }
        }
        doc->commitTransaction();
    }
}

void MainWindow::setUrlHandler(const QString& scheme, Gui::UrlHandler* handler)
{
    d->urlHandler[scheme] = handler;
}

void MainWindow::unsetUrlHandler(const QString& scheme)
{
    d->urlHandler.remove(scheme);
}

void MainWindow::loadUrls(App::Document* doc, const QList<QUrl>& urls)
{
    QStringList files;
    for (const auto& it : urls) {
        QMap<QString, QPointer<UrlHandler>>::iterator jt = d->urlHandler.find(it.scheme());
        if (jt != d->urlHandler.end() && !jt->isNull()) {
            // delegate the loading to the url handler
            (*jt)->openUrl(doc, it);
            continue;
        }

        QFileInfo info(it.toLocalFile());
        if (info.exists() && info.isFile()) {
            if (info.isSymLink()) {
                info.setFile(info.symLinkTarget());
            }
            std::vector<std::string> module = App::GetApplication().getImportModules(
                info.completeSuffix().toLatin1()
            );
            if (module.empty()) {
                module = App::GetApplication().getImportModules(info.suffix().toLatin1());
            }
            if (!module.empty()) {
                // ok, we support files with this extension
                files << info.absoluteFilePath();
            }
            else {
                Base::Console().message(
                    "No support to load file '%s'\n",
                    (const char*)info.absoluteFilePath().toUtf8()
                );
            }
        }
        else if (it.scheme().toLower() == QLatin1String("http")) {
            Gui::Dialog::DownloadManager* dm = Gui::Dialog::DownloadManager::getInstance();
            dm->download(dm->redirectUrl(it));
        }

        else if (it.scheme().toLower() == QLatin1String("https")) {
            QUrl url = it;
            QUrlQuery urlq(url);
            if (urlq.hasQueryItem(QLatin1String("sid"))) {
                urlq.removeAllQueryItems(QLatin1String("sid"));
                url.setQuery(urlq);
                url.setScheme(QLatin1String("http"));
            }
            Gui::Dialog::DownloadManager* dm = Gui::Dialog::DownloadManager::getInstance();
            dm->download(dm->redirectUrl(url));
        }

        else if (it.scheme().toLower() == QLatin1String("ftp")) {
            Gui::Dialog::DownloadManager::getInstance()->download(it);
        }
    }

    QByteArray docName = doc ? QByteArray(doc->getName())
                             : qApp->translate("StdCmdNew", "Unnamed").toUtf8();
    ModuleIO::importFiles(files, docName);
}

void MainWindow::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        d->sizeLabel->setText(tr("Dimension"));

        CommandManager& rclMan = Application::Instance->commandManager();
        std::vector<Command*> cmd = rclMan.getAllCommands();
        for (auto& it : cmd) {
            it->languageChange();
        }

        // reload current workbench to retranslate all actions and window titles
        Workbench* wb = WorkbenchManager::instance()->active();
        if (wb) {
            wb->retranslate();
        }
    }
    else if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            QMdiSubWindow* mdi = d->mdiArea->currentSubWindow();
            setActiveSubWindow(mdi);
        }
    }
    else {
        QMainWindow::changeEvent(e);
    }
}

void MainWindow::clearStatus()
{
    d->currentStatusType = 100;
    statusBar()->setStyleSheet(QStringLiteral("#statusBar{}"));
}

void MainWindow::statusMessageChanged()
{
    if (d->currentStatusType < 0) {
        d->currentStatusType = -d->currentStatusType;
    }
    else {
        // here probably means the status bar message is changed by QMainWindow
        // internals, e.g. for displaying tooltip and stuff. Set reset what
        // we've changed.
        d->statusTimer->stop();
        clearStatus();
    }
}

void MainWindow::showMessage(const QString& message, int timeout)
{
    if (QApplication::instance()->thread() != QThread::currentThread()) {
        QApplication::postEvent(this, new CustomMessageEvent(MainWindow::Tmp, message, timeout));
        return;
    }
    d->actionLabel->setText(message.simplified());
    if (timeout) {
        d->actionTimer->setSingleShot(true);
        d->actionTimer->start(timeout);
    }
    else {
        d->actionTimer->stop();
    }
}

void MainWindow::setRightSideMessage(const QString& message)
{
    d->rightSideLabel->setText(message.simplified());
}

bool MainWindow::isRightSideMessageVisible() const
{
    return d->rightSideLabel->isVisible();
}

void MainWindow::showStatus(int type, const QString& message)
{
    if (QApplication::instance()->thread() != QThread::currentThread()) {
        QApplication::postEvent(this, new CustomMessageEvent(type, message));
        return;
    }

    if (d->currentStatusType < type) {
        return;
    }

    d->statusTimer->setSingleShot(true);
    // TODO: hardcode?
    int timeout = 5000;
    d->statusTimer->start(timeout);

    QFontMetrics fm(statusBar()->font());
    QString msg = fm.elidedText(message, Qt::ElideMiddle, this->d->actionLabel->width());
    switch (type) {
        case MainWindow::Err:
            statusBar()->setStyleSheet(d->status->err);
            break;
        case MainWindow::Wrn:
            statusBar()->setStyleSheet(d->status->wrn);
            break;
        case MainWindow::Pane:
            statusBar()->setStyleSheet(QStringLiteral("#statusBar{}"));
            break;
        default:
            statusBar()->setStyleSheet(d->status->msg);
            break;
    }
    d->currentStatusType = -type;
    statusBar()->showMessage(msg.simplified(), timeout);
}

void MainWindow::showHints(const std::list<InputHint>& hints)
{
    d->hintLabel->showHints(hints);
}

void MainWindow::hideHints()
{
    d->hintLabel->clearHints();
}

// set text to the pane
void MainWindow::setPaneText(int i, QString text)
{
    if (i == 1) {
        showStatus(MainWindow::Pane, text);
    }
    else if (i == 2) {
        d->sizeLabel->setText(text);
    }
}


void MainWindow::setUserSchema(int userSchema)
{
    d->sizeLabel->setUserSchema(userSchema);
}


void MainWindow::customEvent(QEvent* e)
{
    if (e->type() == QEvent::User) {
        auto ce = static_cast<Gui::CustomMessageEvent*>(e);
        QString msg = ce->message();
        switch (ce->type()) {
            case MainWindow::Log: {
                if (msg.startsWith(QLatin1String("#Inventor V2.1 ascii "))) {
                    Gui::Document* d = Application::Instance->activeDocument();
                    if (d) {
                        auto view = new ViewProviderExtern();
                        try {
                            view->setModeByString("1", msg.toLatin1().constData());
                            d->setAnnotationViewProvider("Vdbg", view);
                        }
                        catch (...) {
                            delete view;
                        }
                    }
                }
                break;
            }
            case MainWindow::Tmp: {
                showMessage(msg, ce->timeout());
                break;
            }
            default:
                showStatus(ce->type(), msg);
        }
    }
    else if (e->type() == ActionStyleEvent::EventType) {
        QList<TaskView::TaskView*> tasks = findChildren<TaskView::TaskView*>();
        if (static_cast<ActionStyleEvent*>(e)->getType() == ActionStyleEvent::Clear) {
            for (auto& task : tasks) {
                task->clearActionStyle();
            }
        }
        else {
            for (auto& task : tasks) {
                task->restoreActionStyle();
            }
        }
    }
}

QMdiArea* MainWindow::getMdiArea() const
{
    return d->mdiArea;
}

void MainWindow::setWindowTitle(const QString& string)
{
    QString title;
    QString appname = QCoreApplication::applicationName();
    if (appname.isEmpty()) {
        appname = QString::fromLatin1(App::Application::Config()["ExeName"].c_str());
    }

    // allow one to disable version number
    ParameterGrp::handle hGen = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/General"
    );
    bool showVersion = hGen->GetBool("ShowVersionInTitle", true);

    if (showVersion) {
        // set main window title with FreeCAD Version
        title = QString::fromStdString(App::Application::getNameWithVersion());
    }
    else {
        title = appname;
    }

    if (SafeMode::SafeModeEnabled()) {
        title = QStringLiteral("%1 (%2)").arg(title, tr("Safe Mode"));
    }

    if (!string.isEmpty()) {
        title = QStringLiteral("[*] %1 - %2").arg(string, title);
    }

    QMainWindow::setWindowTitle(title);
}

// ----------------------------------------------------------

StatusBarObserver::StatusBarObserver()
    : WindowParameter("OutputWindow")
{
    msg = QStringLiteral("#statusBar{color: #000000}");  // black
    wrn = QStringLiteral("#statusBar{color: #ffaa00}");  // orange
    err = QStringLiteral("#statusBar{color: #ff0000}");  // red
    Base::Console().attachObserver(this);
    getWindowParameter()->Attach(this);
    getWindowParameter()->NotifyAll();
}

StatusBarObserver::~StatusBarObserver()
{
    getWindowParameter()->Detach(this);
    Base::Console().detachObserver(this);
}

void StatusBarObserver::OnChange(Base::Subject<const char*>& rCaller, const char* sReason)
{
    ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
    auto format = QStringLiteral("#statusBar{color: %1}");
    if (strcmp(sReason, "colorText") == 0) {
        unsigned long col = rclGrp.GetUnsigned(sReason);
        this->msg = format.arg(Base::Color::fromPackedRGB<QColor>(col).name());
    }
    else if (strcmp(sReason, "colorWarning") == 0) {
        unsigned long col = rclGrp.GetUnsigned(sReason);
        this->wrn = format.arg(Base::Color::fromPackedRGB<QColor>(col).name());
    }
    else if (strcmp(sReason, "colorError") == 0) {
        unsigned long col = rclGrp.GetUnsigned(sReason);
        this->err = format.arg(Base::Color::fromPackedRGB<QColor>(col).name());
    }
    else if (strcmp(sReason, "colorCritical") == 0) {
        unsigned long col = rclGrp.GetUnsigned(sReason);
        this->critical = format.arg(
            QColor((col >> 24) & 0xff, (col >> 16) & 0xff, (col >> 8) & 0xff).name()
        );
    }
}

void StatusBarObserver::sendLog(
    const std::string& notifiername,
    const std::string& msg,
    Base::LogStyle level,
    Base::IntendedRecipient recipient,
    Base::ContentType content
)
{
    (void)notifiername;

    // Do not log untranslated messages, or messages intended only to a developer to status bar
    if (recipient == Base::IntendedRecipient::Developer || content == Base::ContentType::Untranslated
        || content == Base::ContentType::Untranslatable) {
        return;
    }

    int messageType = -1;
    switch (level) {
        case Base::LogStyle::Warning:
            messageType = MainWindow::Wrn;
            break;
        case Base::LogStyle::Message:
            messageType = MainWindow::Msg;
            break;
        case Base::LogStyle::Error:
            messageType = MainWindow::Err;
            break;
        case Base::LogStyle::Log:
            messageType = MainWindow::Log;
            break;
        case Base::LogStyle::Critical:
            messageType = MainWindow::Critical;
            break;
        default:
            break;
    }

    // Send the event to the main window to allow thread-safety. Qt will delete it when done.
    auto ev = new CustomMessageEvent(messageType, QString::fromUtf8(msg.c_str()));
    QApplication::postEvent(getMainWindow(), ev);
}

// -------------------------------------------------------------

int ActionStyleEvent::EventType = -1;

ActionStyleEvent::ActionStyleEvent(Style type)
    : QEvent(QEvent::Type(EventType))
    , type(type)
{}

ActionStyleEvent::Style ActionStyleEvent::getType() const
{
    return type;
}


#include "moc_MainWindow.cpp"
#include "MainWindow.moc"
