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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <algorithm>
# include <QApplication>
# include <QBuffer>
# include <QByteArray>
# include <QClipboard>
# include <QMimeData>
# include <QCloseEvent>
# include <QContextMenuEvent>
# include <QDesktopWidget>
# include <QDockWidget>
# include <QFontMetrics>
# include <QLabel>
# include <QMdiSubWindow>
# include <QMessageBox>
# include <QPainter>
# include <QSettings>
# include <QSignalMapper>
# include <QStatusBar>
# include <QTimer>
# include <QToolBar>
# include <QWhatsThis>
#endif

#include <boost/signals.hpp>
#include <boost/bind.hpp>

// FreeCAD Base header
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Persistence.h>
#include <Base/Stream.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <App/Application.h>
#include <App/DocumentObject.h>

#include "MainWindow.h"
#include "Application.h"
#include "Assistant.h"
#include "DownloadDialog.h"
#include "DownloadManager.h"
#include "WaitCursor.h"

#include "Action.h"
#include "Command.h"

#include "ToolBoxManager.h"
#include "DockWindowManager.h"
#include "ToolBarManager.h"
#include "WorkbenchManager.h"
#include "Workbench.h"

#include "Window.h" 
#include "View.h"
#include "Macro.h"
#include "ProgressBar.h"

#include "WidgetFactory.h"
#include "BitmapFactory.h"
#include "Splashscreen.h"

#include "Tree.h"
#include "PropertyView.h"
#include "SelectionView.h"
#include "TaskPanelView.h"
#include "MenuManager.h"
//#include "ToolBox.h"
#include "HelpView.h"
#include "ReportView.h"
#include "CombiView.h"
#include "PythonConsole.h"

#include "DlgTipOfTheDayImp.h"
#include "DlgUndoRedo.h"
#include "DlgOnlineHelpImp.h"

#include "Language/Translator.h"
#include "GuiInitScript.h"

#include "Document.h"
#include "MergeDocuments.h"
#include "ViewProviderExtern.h"

#include "SpaceballEvent.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

#if defined(Q_OS_WIN32)
#define slots
#include <private/qmainwindowlayout_p.h>
#include <private/qwidgetresizehandler_p.h>
#endif

using namespace Gui;
using namespace Gui::DockWnd;
using namespace std;


MainWindow* MainWindow::instance = 0L;

namespace Gui {

// Pimpl class
struct MainWindowP
{
    QLabel* sizeLabel;
    QLabel* actionLabel;
    QTimer* actionTimer;
    QTimer* activityTimer;
    QTimer* visibleTimer;
#if !defined (NO_USE_QT_MDI_AREA)
    QMdiArea* mdiArea;
#else
    QWorkspace* workspace;
    QTabBar* tabs;
#endif
    QPointer<MDIView> activeView;
    QSignalMapper* windowMapper;
    QSplashScreen* splashscreen;
    StatusBarObserver* status;
    bool whatsthis;
    QString whatstext;
    Assistant* assistant;
};

class MDITabbar : public QTabBar
{
public:
    MDITabbar( QWidget * parent = 0 ) : QTabBar(parent)
    {
        menu = new QMenu(this);
        // For Qt 4.2.x the tabs might be very wide
#if QT_VERSION >= 0x040200
        setDrawBase(false);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
#endif
    }

    ~MDITabbar()
    {
        delete menu;
    }

protected:
    void contextMenuEvent ( QContextMenuEvent * e )
    {
        menu->clear();
        CommandManager& cMgr = Application::Instance->commandManager();
        if (tabRect(currentIndex()).contains(e->pos()))
            cMgr.getCommandByName("Std_CloseActiveWindow")->addTo(menu);
        cMgr.getCommandByName("Std_CloseAllWindows")->addTo(menu);
        menu->addSeparator();
        cMgr.getCommandByName("Std_CascadeWindows")->addTo(menu);
        cMgr.getCommandByName("Std_ArrangeIcons")->addTo(menu);
        cMgr.getCommandByName("Std_TileWindows")->addTo(menu);
        menu->addSeparator();
        cMgr.getCommandByName("Std_Windows")->addTo(menu);
        menu->popup(e->globalPos());
    }

private:
    QMenu* menu;
};

#if defined(Q_OS_WIN32)
class MainWindowTabBar : public QTabBar
{
public:
    MainWindowTabBar(QWidget *parent) : QTabBar(parent)
    {
        setExpanding(false);
    }
protected:
    bool event(QEvent *e)
    {
        // show the tooltip if tab is too small to fit label
        if (e->type() != QEvent::ToolTip)
            return QTabBar::event(e);
        QSize size = this->size();
        QSize hint = sizeHint();
        if (shape() == QTabBar::RoundedWest || shape() == QTabBar::RoundedEast) {
            size.transpose();
            hint.transpose();
        }
        if (size.width() < hint.width())
            return QTabBar::event(e);
        e->accept();
        return true;
    }
    void tabInserted (int index)
    {
        // get all dock windows
        QList<QDockWidget*> dw = getMainWindow()->findChildren<QDockWidget*>();
        for (QList<QDockWidget*>::iterator it = dw.begin(); it != dw.end(); ++it) {
            // compare tab text and window title to get the right dock window
            if (this->tabText(index) == (*it)->windowTitle()) {
                QWidget* dock = (*it)->widget();
                if (dock) {
                    QIcon icon = dock->windowIcon();
                    if (!icon.isNull())
                        setTabIcon(index, icon);
                }
                break;
            }
        }
    }
};
#endif

} // namespace Gui


/* TRANSLATOR Gui::MainWindow */

MainWindow::MainWindow(QWidget * parent, Qt::WFlags f)
  : QMainWindow( parent, f/*WDestructiveClose*/ )
{
    d = new MainWindowP;
    d->splashscreen = 0;
    d->activeView = 0;
    d->whatsthis = false;
    d->assistant = new Assistant();

    // global access 
    instance = this;

    // Create the layout containing the workspace and a tab bar
#if defined(NO_USE_QT_MDI_AREA)
    QFrame* vbox = new QFrame(this);
    vbox->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setMargin(1);
    vbox->setLayout(layout);

    d->workspace = new QWorkspace();
    d->workspace->setScrollBarsEnabled( true );
    QPixmap backgnd((const char**) background);
    d->workspace->setBackground(backgnd);

    d->tabs = new MDITabbar();
    d->tabs->setShape(QTabBar:: RoundedSouth);

    layout->addWidget(d->workspace);
    layout->addWidget(d->tabs);
    setCentralWidget(vbox);
#else
    d->mdiArea = new QMdiArea();
#if QT_VERSION >= 0x040500
    d->mdiArea->setTabPosition(QTabWidget::South);
    d->mdiArea->setViewMode(QMdiArea::TabbedView);
    QTabBar* tab = d->mdiArea->findChild<QTabBar*>();
    if (tab) {
        // 0000636: Two documents close
#if QT_VERSION < 0x040800
        connect(tab, SIGNAL(tabCloseRequested(int)),
                this, SLOT(tabCloseRequested(int)));
#endif
        tab->setTabsClosable(true);
        // The tabs might be very wide
        tab->setExpanding(false);
    }
#endif
    d->mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);
    d->mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    d->mdiArea->setBackground(QBrush(QColor(160,160,160)));
    setCentralWidget(d->mdiArea);
#endif

    // labels and progressbar
    d->status = new StatusBarObserver();
    d->actionLabel = new QLabel(statusBar());
    d->actionLabel->setMinimumWidth(120);
    d->sizeLabel = new QLabel(tr("Dimension"), statusBar());
    d->sizeLabel->setMinimumWidth(120);
    statusBar()->addWidget(d->actionLabel, 0);
    QProgressBar* progressBar = Gui::Sequencer::instance()->getProgressBar(statusBar());
    statusBar()->addPermanentWidget(progressBar, 0);
    statusBar()->addPermanentWidget(d->sizeLabel, 0);

    // clears the action label
    d->actionTimer = new QTimer( this );
    connect(d->actionTimer, SIGNAL(timeout()), d->actionLabel, SLOT(clear()));

    // update gui timer
    d->activityTimer = new QTimer(this);
    connect(d->activityTimer, SIGNAL(timeout()),this, SLOT(updateActions()));
    d->activityTimer->setSingleShot(true);
    d->activityTimer->start(300);

    // show main window timer
    d->visibleTimer = new QTimer(this);
    connect(d->visibleTimer, SIGNAL(timeout()),this, SLOT(showMainWindow()));
    d->visibleTimer->setSingleShot(true);

    d->windowMapper = new QSignalMapper(this);

    // connection between workspace, window menu and tab bar
#if !defined (NO_USE_QT_MDI_AREA)
    connect(d->windowMapper, SIGNAL(mapped(QWidget *)),
            this, SLOT(onSetActiveSubWindow(QWidget*)));
    connect(d->mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(onWindowActivated(QMdiSubWindow* )));
#else
    connect(d->windowMapper, SIGNAL(mapped(QWidget *)),
            d->workspace, SLOT(setActiveWindow(QWidget* )));
    connect(d->workspace, SIGNAL(windowActivated(QWidget *)),
            this, SLOT(onWindowActivated(QWidget* )));
    connect(d->tabs, SIGNAL(currentChanged(int)),
            this, SLOT(onTabSelected(int)));
#endif

    DockWindowManager* pDockMgr = DockWindowManager::instance();

    // Show all dockable windows over the workbench facility
    //
#if 0
    // Toolbox
    ToolBox* toolBox = new ToolBox(this);
    toolBox->setObjectName(QT_TRANSLATE_NOOP("QDockWidget","Toolbox"));
    pDockMgr->registerDockWindow("Std_ToolBox", toolBox);
    ToolBoxManager::getInstance()->setToolBox( toolBox );

    // Help View
    QString home = Gui::Dialog::DlgOnlineHelpImp::getStartpage();
    HelpView* pcHelpView = new HelpView( home, this );
    pDockMgr->registerDockWindow("Std_HelpView", pcHelpView);

    // TaskPanel view
    TaskPanelView* pcTaskPanelView = new TaskPanelView(0, this);
    pcTaskPanelView->setObjectName
        (QString::fromAscii(QT_TRANSLATE_NOOP("QDockWidget","Task View")));
    pcTaskPanelView->setMinimumWidth(210);
    pDockMgr->registerDockWindow("Std_TaskPanelView", pcTaskPanelView);
#endif

    // Tree view
    TreeDockWidget* tree = new TreeDockWidget(0, this);
    tree->setObjectName
        (QString::fromAscii(QT_TRANSLATE_NOOP("QDockWidget","Tree view")));
    tree->setMinimumWidth(210);
    pDockMgr->registerDockWindow("Std_TreeView", tree);

    // Property view
    PropertyDockView* pcPropView = new PropertyDockView(0, this);
    pcPropView->setObjectName
        (QString::fromAscii(QT_TRANSLATE_NOOP("QDockWidget","Property view")));
    pcPropView->setMinimumWidth(210);
    pDockMgr->registerDockWindow("Std_PropertyView", pcPropView);

    // Selection view
    SelectionView* pcSelectionView = new SelectionView(0, this);
    pcSelectionView->setObjectName
        (QString::fromAscii(QT_TRANSLATE_NOOP("QDockWidget","Selection view")));
    pcSelectionView->setMinimumWidth(210);
    pDockMgr->registerDockWindow("Std_SelectionView", pcSelectionView);

    // Combo view
    CombiView* pcCombiView = new CombiView(0, this);
    pcCombiView->setObjectName(QString::fromAscii(QT_TRANSLATE_NOOP("QDockWidget","Combo View")));
    pcCombiView->setMinimumWidth(150);
    pDockMgr->registerDockWindow("Std_CombiView", pcCombiView);

#if QT_VERSION < 0x040500
    // Report view
    Gui::DockWnd::ReportView* pcReport = new Gui::DockWnd::ReportView(this);
    pcReport->setObjectName
        (QString::fromAscii(QT_TRANSLATE_NOOP("QDockWidget","Report view")));
    pDockMgr->registerDockWindow("Std_ReportView", pcReport);
#else
    // Report view (must be created before PythonConsole!)
    ReportOutput* pcReport = new ReportOutput(this);
    pcReport->setWindowIcon(BitmapFactory().pixmap("MacroEditor"));
    pcReport->setObjectName
        (QString::fromAscii(QT_TRANSLATE_NOOP("QDockWidget","Report view")));
    pDockMgr->registerDockWindow("Std_ReportView", pcReport);

    // Python console
    PythonConsole* pcPython = new PythonConsole(this);
    pcPython->setWordWrapMode(QTextOption::NoWrap);
    pcPython->setWindowIcon(Gui::BitmapFactory().pixmap("python_small"));
    pcPython->setObjectName
        (QString::fromAscii(QT_TRANSLATE_NOOP("QDockWidget","Python console")));
    pDockMgr->registerDockWindow("Std_PythonView", pcPython);

#if defined(Q_OS_WIN32)
    // add our own QTabBar-derived class to the main window layout
    // NOTE: This uses some private stuff from QMainWindow which doesn't
    // seem to be accessible on all platforms.
    QMainWindowLayout* l = static_cast<QMainWindowLayout*>(this->layout());
    for (int i=0; i<5; i++) {
        MainWindowTabBar* result = new MainWindowTabBar(this);
        result->setDrawBase(true);
        result->setElideMode(Qt::ElideRight);
        result->hide(); // avoid to show horizontal bar in top left area
        //result->setDocumentMode(_documentMode);
        connect(result, SIGNAL(currentChanged(int)), l, SLOT(tabChanged()));
        l->unusedTabBars << result;
    }
#endif
#endif

    // accept drops on the window, get handled in dropEvent, dragEnterEvent
    setAcceptDrops(true);
    statusBar()->showMessage(tr("Ready"), 2001);
}

MainWindow::~MainWindow()
{
    delete d->status;
    delete d;
    instance = 0;
}

MainWindow* MainWindow::getInstance()
{
    // MainWindow has a public constructor
    return instance;
}

QMenu* MainWindow::createPopupMenu ()
{
    QMenu* menu = QMainWindow::createPopupMenu();
    Workbench* wb = WorkbenchManager::instance()->active();
    if (wb) {
        MenuItem item;
        wb->createMainWindowPopupMenu(&item);
        if (item.hasItems()) {
            menu->addSeparator();
            QList<MenuItem*> items = item.getItems();
            for (QList<MenuItem*>::iterator it = items.begin(); it != items.end(); ++it) {
                if ((*it)->command() == "Separator") {
                    menu->addSeparator();
                }
                else {
                    Command* cmd = Application::Instance->commandManager().getCommandByName((*it)->command().c_str());
                    if (cmd) cmd->addTo(menu);
                }
            }
        }
    }

    return menu;
}

void MainWindow::arrangeIcons()
{
#if !defined (NO_USE_QT_MDI_AREA)
    d->mdiArea->tileSubWindows();
#else
    d->workspace->arrangeIcons();
#endif
}

void MainWindow::tile()
{
#if !defined (NO_USE_QT_MDI_AREA)
    d->mdiArea->tileSubWindows();
#else
    d->workspace->tile();
#endif
}

void MainWindow::cascade()
{
#if !defined (NO_USE_QT_MDI_AREA)
    d->mdiArea->cascadeSubWindows();
#else
    d->workspace->cascade();
#endif
}

void MainWindow::closeActiveWindow ()
{
#if !defined (NO_USE_QT_MDI_AREA)
    d->mdiArea->closeActiveSubWindow();
#else
    d->workspace->closeActiveWindow();
#endif
}

void MainWindow::closeAllWindows ()
{
#if !defined (NO_USE_QT_MDI_AREA)
    d->mdiArea->closeAllSubWindows();
#else
    d->workspace->closeAllWindows();
#endif
}

void MainWindow::activateNextWindow ()
{
#if !defined (NO_USE_QT_MDI_AREA)
    d->mdiArea->activateNextSubWindow();
#else
    d->workspace->activateNextWindow();
#endif
}

void MainWindow::activatePreviousWindow ()
{
#if !defined (NO_USE_QT_MDI_AREA)
    d->mdiArea->activatePreviousSubWindow();
#else
    d->workspace->activatePreviousWindow();
#endif
}

void MainWindow::activateWorkbench(const QString& name)
{
    // emit this signal
    workbenchActivated(name);
}

void MainWindow::whatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

void MainWindow::showDocumentation(const char* Article)
{
    QString help;
    if (Article && Article[0] != '\0')
        help = QString::fromUtf8("%1.html").arg(QLatin1String(Article));
    d->assistant->showDocumentation(help);
}

bool MainWindow::event(QEvent *e)
{
    if (e->type() == QEvent::EnterWhatsThisMode) {
        // Unfortunately, for top-level widgets such as menus or dialogs we
        // won't be notified when the user clicks the link in the hypertext of
        // the what's this text. Thus, we have to install the main window to
        // the application to observe what happens in eventFilter().
        d->whatstext.clear();
        if (!d->whatsthis) {
            d-> whatsthis = true;
            qApp->installEventFilter(this);
        }
    }
    else if (e->type() == QEvent::LeaveWhatsThisMode) {
        // Here we can't do anything because this event is sent
        // before the WhatThisClicked event is sent. Thus, we handle
        // this in eventFilter().
    }
    else if (e->type() == QEvent::WhatsThisClicked) {
        QWhatsThisClickedEvent* wt = static_cast<QWhatsThisClickedEvent*>(e);
        showDocumentation((const char*)wt->href().toUtf8());
    }
    else if (e->type() == QEvent::ApplicationWindowIconChange) {
        // if application icon changes apply it to the main window and the "About..." dialog
        this->setWindowIcon(QApplication::windowIcon());
        Command* about = Application::Instance->commandManager().getCommandByName("Std_About");
        if (about) {
            Action* action = about->getAction();
            if (action) action->setIcon(QApplication::windowIcon());
        }
    }
    else if (e->type() == Spaceball::ButtonEvent::ButtonEventType) {
        Spaceball::ButtonEvent *buttonEvent = dynamic_cast<Spaceball::ButtonEvent *>(e);
        if (!buttonEvent)
            return true;
        buttonEvent->setHandled(true);
        //only going to respond to button press events.
        if (buttonEvent->buttonStatus() != Spaceball::BUTTON_PRESSED)
            return true;
        ParameterGrp::handle group = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                GetGroup("Spaceball")->GetGroup("Buttons");
        QByteArray groupName(QVariant(buttonEvent->buttonNumber()).toByteArray());
        if (group->HasGroup(groupName.data())) {
            ParameterGrp::handle commandGroup = group->GetGroup(groupName.data());
            std::string commandName(commandGroup->GetASCII("Command"));
            if (commandName.empty())
                return true;
            else
                Application::Instance->commandManager().runCommandByName(commandName.c_str());
        }
        else
            return true;
    }
    else if (e->type() == Spaceball::MotionEvent::MotionEventType) {
        Spaceball::MotionEvent *motionEvent = dynamic_cast<Spaceball::MotionEvent *>(e);
        if (!motionEvent)
            return true;
        motionEvent->setHandled(true);
        Gui::Document *doc = Application::Instance->activeDocument();
        if (!doc)
            return true;
        View3DInventor *temp = dynamic_cast<View3DInventor *>(doc->getActiveView());
        if (!temp)
            return true;
        View3DInventorViewer *view = temp->getViewer();
        if (!view)
            return true;
        QWidget *viewWidget = view->getGLWidget();
        if (viewWidget) {
            Spaceball::MotionEvent anotherEvent(*motionEvent);
            qApp->sendEvent(viewWidget, &anotherEvent);
        }
        return true;
    }
    return QMainWindow::event(e);
}

bool MainWindow::eventFilter(QObject* o, QEvent* e)
{
    if (o != this) {
        if (e->type() == QEvent::WindowStateChange) {
            // notify all mdi views when the active view receives a show normal, show minimized 
            // or show maximized event 
            MDIView * view = qobject_cast<MDIView*>(o);
            if (view) { // emit this signal
                Qt::WindowStates oldstate = static_cast<QWindowStateChangeEvent*>(e)->oldState();
                Qt::WindowStates newstate = view->windowState();
                if (oldstate != newstate)
                    windowStateChanged(view);
            }
        }

        // We don't want to show the bubble help for the what's this text but want to
        // start the help viewer with the according key word.
        // Thus, we have to observe WhatThis events if called for a widget, use its text and
        // must avoid to make the bubble widget visible.
        if (e->type() == QEvent::WhatsThis) {
            if (!o->isWidgetType())
                return false;
            // clicked on a widget in what's this mode
            QWidget * w = static_cast<QWidget *>(o);
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
                QKeyEvent* ke = static_cast<QKeyEvent*>(e);
                if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_F1)
                    whatthis = true;
            }
            else if (e->type() == QEvent::MouseButtonRelease)
                whatthis = true;
            else if (e->type() == QEvent::EnterWhatsThisMode)
                whatthis = true;
            if (whatthis) {
                QAction* cur = static_cast<QMenu*>(o)->activeAction();
                if (cur) {
                    // get the help text for later usage
                    QString s = cur->whatsThis();
                    if (s.isEmpty())
                        s = static_cast<QMenu*>(o)->whatsThis();
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
            static_cast<QWidget *>(o)->setAttribute(Qt::WA_OutsideWSRange);
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
#if !defined (NO_USE_QT_MDI_AREA)
    bool isempty = d->mdiArea->subWindowList().isEmpty();
    QMdiSubWindow* child = new QMdiSubWindow(d->mdiArea->viewport());
    child->setAttribute(Qt::WA_DeleteOnClose);
    child->setWidget(view);
    child->setWindowIcon(view->windowIcon());
    QMenu* menu = child->systemMenu();
    QAction* action = menu->addAction(tr("Close All"));
    connect(action, SIGNAL(triggered()), d->mdiArea, SLOT(closeAllSubWindows()));
    d->mdiArea->addSubWindow(child);
#else
    QWidget* active = d->workspace->activeWindow();
    d->workspace->addWindow(view);
#if defined(Q_OS_WIN32)
    // avoid dragging problem with not maximized mdi childs which have embedded a GL window
    QWidget* p = view->parentWidget();
    if (p) {
        QWidgetResizeHandler* handler = p->findChild<QWidgetResizeHandler*>();
        if (handler) handler->setMovingEnabled(false);
    }
#endif
#endif
    connect(view, SIGNAL(message(const QString&, int)),
            this, SLOT(showMessage(const QString&, int)));
    connect(this, SIGNAL(windowStateChanged(MDIView*)),
            view, SLOT(windowStateChanged(MDIView*)));

    // listen to the incoming events of the view
    view->installEventFilter(this);

    // show the very first window in maximized mode
#if !defined (NO_USE_QT_MDI_AREA)
    if (isempty)
#else
    if (d->workspace->windowList().isEmpty())
#endif
        view->showMaximized();
    else
        view->show();

#if defined(NO_USE_QT_MDI_AREA)
    // look if the window was already inserted
    for (int i=0; i < d->tabs->count(); i++) {
        if (d->tabs->tabData(i).value<QWidget*>() == view)
            return;
    }

    // being informed when the view is destroyed
    connect(view, SIGNAL(destroyed()),
            this, SLOT(onWindowDestroyed()));

    // add a new tab to our tabbar
    int index=-1;
    index = d->tabs->addTab(view->windowIcon(), view->windowTitle());
    d->tabs->setTabToolTip(index, view->windowTitle());
    QVariant var; var.setValue<QWidget*>(view);
    d->tabs->setTabData(index, var);

    tabChanged(view);
    if (d->tabs->count() == 1)
        d->tabs->show(); // invoke show() for the first tab
    d->tabs->update();
    d->tabs->setCurrentIndex(index);
#endif

#if defined (NO_USE_QT_MDI_AREA)
    // With the old QWorkspace class we have some strange update problem
    // when adding a 3d view and the first view is a web view or text view.
    static bool do_hack=true;
    MDIView *active_mdi = qobject_cast<MDIView*>(active);
    if (do_hack && active_mdi && active_mdi->getTypeId() != view->getTypeId()) {
        d->workspace->setActiveWindow(active);
        d->workspace->setActiveWindow(view);
        do_hack = false; // needs to be done only once
    }
#endif
}

/**
 * Removes the instance of Gui::MDiView from the main window and sends am event
 * to the parent widget, a QMdiSubWindow to delete itself.
 * If you want to avoid that the Gui::MDIView instance gets destructed too you
 * must reparent it afterwards, e.g. set parent to NULL.
 */
void MainWindow::removeWindow(Gui::MDIView* view)
{
    // free all connections
    disconnect(view, SIGNAL(message(const QString&, int)),
               this, SLOT(showMessage(const QString&, int )));
    disconnect(this, SIGNAL(windowStateChanged(MDIView*)),
               view, SLOT(windowStateChanged(MDIView*)));
    view->removeEventFilter(this);

#if defined(NO_USE_QT_MDI_AREA)
    for (int i = 0; i < d->tabs->count(); i++) {
        if (d->tabs->tabData(i).value<QWidget*>() == view) {
            d->tabs->removeTab(i);
            if (d->tabs->count() == 0)
                d->tabs->hide(); // no view open any more
            break;
        }
    }
#endif

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

#if defined(NO_USE_QT_MDI_AREA)
    // this view is not under control of the main window any more
    disconnect(view, SIGNAL(destroyed()),
               this, SLOT(onWindowDestroyed()));
#else
    QWidget* parent = view->parentWidget();
    // The call of 'd->mdiArea->removeSubWindow(parent)' causes the QMdiSubWindow
    // to loose its parent and thus the notification in QMdiSubWindow::closeEvent
    // of other mdi windows to get maximized if this window is maximized will fail.
    // However, we must let it here otherwise deleting MDI child views directly can
    // cause other problems.
    d->mdiArea->removeSubWindow(parent);
    parent->deleteLater();
#endif
}

void MainWindow::tabChanged(MDIView* view)
{
#if defined(NO_USE_QT_MDI_AREA)
    for (int i = 0; i < d->tabs->count(); i++) {
        if (d->tabs->tabData(i).value<QWidget*>() == view) {
            QString cap = view->windowTitle();
            int lastIndex = cap.lastIndexOf(QString::fromAscii("[*]"));
            if (lastIndex > 0) {
                cap = cap.left(lastIndex);
                if (view->isWindowModified())
                    cap = QString::fromAscii("%1*").arg(cap);
            }
            d->tabs->setTabToolTip(i, cap);
            
            // remove path separators
            int pos = cap.lastIndexOf(QLatin1Char('/'));
            cap = cap.mid( pos+1 );
            pos = cap.lastIndexOf(QLatin1Char('\\'));
            cap = cap.mid( pos+1 );

            d->tabs->setTabText(i, cap);
            break;
        }
    }
#endif
}

#if defined(NO_USE_QT_MDI_AREA)
void MainWindow::onWindowDestroyed()
{
    QObject* view = (QObject*)sender();
    for (int i = 0; i < d->tabs->count(); i++) {
        if (d->tabs->tabData(i).value<QWidget*>() == view) {
            d->tabs->removeTab(i);
            if (d->tabs->count() == 0)
                d->tabs->hide(); // no view open any more
            break;
        }
    }
}

void MainWindow::onTabSelected(int i)
{
    QVariant var = d->tabs->tabData(i);
    if (var.isValid() && var.canConvert<QWidget*>()) {
        QWidget* view = var.value<QWidget*>();
        if (view && !view->hasFocus())
            view->setFocus();
    }
}

void MainWindow::setActiveWindow(MDIView* view)
{
    d->workspace->setActiveWindow(view);
    d->activeView = view;
    Application::Instance->viewActivated(view);
}
#else
void MainWindow::tabCloseRequested(int index)
{
    QTabBar* tab = d->mdiArea->findChild<QTabBar*>();
    if (index < 0 || index >= tab->count())
        return;

    const QList<QMdiSubWindow *> subWindows = d->mdiArea->subWindowList();
    Q_ASSERT(index < subWindows.size());

    QMdiSubWindow *subWindow = d->mdiArea->subWindowList().at(index);
    Q_ASSERT(subWindow);
    subWindow->close();
}

void MainWindow::onSetActiveSubWindow(QWidget *window)
{
    if (!window)
        return;
    d->mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}

void MainWindow::setActiveWindow(MDIView* view)
{
    onSetActiveSubWindow(view->parentWidget());
    d->activeView = view;
    Application::Instance->viewActivated(view);
}
#endif

#if !defined (NO_USE_QT_MDI_AREA)
void MainWindow::onWindowActivated(QMdiSubWindow* w)
#else
void MainWindow::onWindowActivated(QWidget* w)
#endif
{
#if !defined (NO_USE_QT_MDI_AREA)
    if (!w) return;
    MDIView* view = dynamic_cast<MDIView*>(w->widget());
#else
     MDIView* view = dynamic_cast<MDIView*>(w);
#endif

    // Even if windowActivated() signal is emitted mdi doesn't need to be a top-level window.
    // This happens e.g. if two windows are top-level and one of them gets docked again.
    // QWorkspace emits the signal then even though the other window is in front.
    // The consequence is that the docked window becomes the active window and not the undocked
    // window on top. This means that all accel events, menu and toolbar actions get redirected
    // to the (wrong) docked window.
    // But just testing whether the view is active and ignore it if not leads to other more serious problems -
    // at least under Linux. It seems to be a problem with the window manager.
    // Under Windows it seems to work though it's not really sure that it works reliably.
    // Result: So, we accept the first problem to be sure to avoid the second one.
    if ( !view /*|| !mdi->isActiveWindow()*/ ) 
        return; // either no MDIView or no valid object or no top-level window

    // set active the appropriate window (it needs not to be part of mdiIds, e.g. directly after creation)
    d->activeView = view;
    Application::Instance->viewActivated(view);

#if defined(NO_USE_QT_MDI_AREA)
    // set the appropriate tab to the new active window
    for (int i = 0; i < d->tabs->count(); i++) {
        if (d->tabs->tabData(i).value<QWidget*>() == view) {
            d->tabs->blockSignals(true);
            d->tabs->setCurrentIndex(i);
            d->tabs->blockSignals(false);
            break;
        }
    }
#endif
}

void MainWindow::onWindowsMenuAboutToShow()
{
#if !defined (NO_USE_QT_MDI_AREA)
    QList<QMdiSubWindow*> windows = d->mdiArea->subWindowList(QMdiArea::CreationOrder);
    QWidget* active = d->mdiArea->activeSubWindow();
#else
    QList<QWidget*> windows = d->workspace->windowList(QWorkspace::CreationOrder);
    QWidget* active = d->workspace->activeWindow();
#endif

    // We search for the 'Std_WindowsMenu' command that provides the list of actions
    CommandManager& cMgr = Application::Instance->commandManager();
    Command* cmd = cMgr.getCommandByName("Std_WindowsMenu");
    QList<QAction*> actions = qobject_cast<ActionGroup*>(cmd->getAction())->actions();

    // do the connection only once
    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;
        QAction* last = actions.isEmpty() ? 0 : actions.last();
        for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
            if (*it == last)
                break; // this is a separator
            connect(*it, SIGNAL(triggered()), d->windowMapper, SLOT(map()));
        }
    }

    int numWindows = std::min<int>(actions.count()-1, windows.count());
    for (int index = 0; index < numWindows; index++) {
        QWidget* child = windows.at(index);
        QAction* action = actions.at(index);
        QString text;
        QString title = child->windowTitle();
        int lastIndex = title.lastIndexOf(QString::fromAscii("[*]"));
        if (lastIndex > 0) {
            title = title.left(lastIndex);
            if (child->isWindowModified())
                title = QString::fromAscii("%1*").arg(title);
        }
        if (index < 9)
            text = QString::fromAscii("&%1 %2").arg(index+1).arg(title);
        else
            text = QString::fromAscii("%1 %2").arg(index+1).arg(title);
        action->setText(text);
        action->setVisible(true);
        action->setChecked(child == active);
        d->windowMapper->setMapping(action, child);
    }

    // if less windows than actions
    for (int index = numWindows; index < actions.count(); index++)
        actions[index]->setVisible(false);
    // show the separator
    if (numWindows > 0)
        actions.last()->setVisible(true);
}

void MainWindow::onToolBarMenuAboutToShow()
{
    QMenu* menu = static_cast<QMenu*>(sender());
    menu->clear();
    QList<QToolBar*> dock = this->findChildren<QToolBar*>();
    for (QList<QToolBar*>::Iterator it = dock.begin(); it != dock.end(); ++it) {
        if ((*it)->parentWidget() == this) {
            QAction* action = (*it)->toggleViewAction();
            action->setToolTip(tr("Toggles this toolbar"));
            action->setStatusTip(tr("Toggles this toolbar"));
            action->setWhatsThis(tr("Toggles this toolbar"));
            menu->addAction(action);
        }
    }
}

void MainWindow::onDockWindowMenuAboutToShow()
{
    QMenu* menu = static_cast<QMenu*>(sender());
    menu->clear();
    QList<QDockWidget*> dock = this->findChildren<QDockWidget*>();
    for (QList<QDockWidget*>::Iterator it = dock.begin(); it != dock.end(); ++it) {
        QAction* action = (*it)->toggleViewAction();
        action->setToolTip(tr("Toggles this dockable window"));
        action->setStatusTip(tr("Toggles this dockable window"));
        action->setWhatsThis(tr("Toggles this dockable window"));
        menu->addAction(action);
    }
}

#if !defined (NO_USE_QT_MDI_AREA)
QList<QWidget*> MainWindow::windows(QMdiArea::WindowOrder order) const
{
    QList<QWidget*> mdis;
    QList<QMdiSubWindow*> wnds = d->mdiArea->subWindowList(order);
    for (QList<QMdiSubWindow*>::iterator it = wnds.begin(); it != wnds.end(); ++it) {
        mdis << (*it)->widget();
    }
    return mdis;
}
#else
QList<QWidget*> MainWindow::windows(QWorkspace::WindowOrder order) const
{
    return d->workspace->windowList(order);
}
#endif

// set text to the pane
void MainWindow::setPaneText(int i, QString text)
{
    if (i==1) {
        d->actionLabel->setText(text);
        d->actionTimer->setSingleShot(true);
        d->actionTimer->start(5000);
    }
    else if (i==2) {
        d->sizeLabel->setText(text);
    }
}

MDIView* MainWindow::activeWindow(void) const
{
    // each activated window notifies this main window when it is activated
    return d->activeView;
}

void MainWindow::closeEvent (QCloseEvent * e)
{
    Application::Instance->tryClose(e);
    if (e->isAccepted()) {
        // Send close event to all non-modal dialogs
        QList<QDialog*> dialogs = this->findChildren<QDialog*>();
        // It is possible that closing a dialog internally closes further dialogs. Thus,
        // we have to check the pointer before.
        QList< QPointer<QDialog> > dialogs_ptr;
        for (QList<QDialog*>::iterator it = dialogs.begin(); it != dialogs.end(); ++it) {
            dialogs_ptr.append(*it);
        }
        for (QList< QPointer<QDialog> >::iterator it = dialogs_ptr.begin(); it != dialogs_ptr.end(); ++it) {
            if (!(*it).isNull())
                (*it)->close();
        }
        QList<MDIView*> mdis = this->findChildren<MDIView*>();
        // Force to close any remaining (passive) MDI child views
        for (QList<MDIView*>::iterator it = mdis.begin(); it != mdis.end(); ++it) {
            (*it)->hide();
            (*it)->deleteLater();
        }
        d->activityTimer->stop();
        saveWindowSettings();
        delete d->assistant;
        d->assistant = 0;

        /*emit*/ mainWindowClosed();
        qApp->quit(); // stop the event loop
    }
}

void MainWindow::showEvent(QShowEvent  * /*e*/)
{
    // needed for logging
    std::clog << "Show main window" << std::endl;
    d->visibleTimer->start(15000);
}

void MainWindow::hideEvent(QHideEvent  * /*e*/)
{
    // needed for logging
    std::clog << "Hide main window" << std::endl;
    d->visibleTimer->stop();
}

void MainWindow::showMainWindow()
{
    // Under certain circumstances it can happen that at startup the main window
    // appears for a short moment and disappears immediately. The workaround
    // starts a timer to check for the visibility of the main window and call
    // ShowWindow() if needed.
    // So far, this phenomena only appeared with Qt4.1.4
#if defined(Q_WS_WIN) && (QT_VERSION == 0x040104)
    WId id = this->winId();
    ShowWindow(id, SW_SHOW);
    std::cout << "Force to show main window" << std::endl;
#endif
}

void MainWindow::delayedStartup()
{
    // processing all command line files
    try {
        App::Application::processCmdLineFiles();
    }
    catch (const Base::SystemExitException&) {
        throw;
    }

    const std::map<std::string,std::string>& cfg = App::Application::Config();
    std::map<std::string,std::string>::const_iterator it = cfg.find("StartHidden");
    if (it != cfg.end()) {
        QApplication::quit();
        return;
    }

    // Create new document?
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Document");
    if (hGrp->GetBool("CreateNewDoc", false)) {
        App::GetApplication().newDocument();
    }

    Application::Instance->checkForPreviousCrashes();
}

void MainWindow::appendRecentFile(const QString& filename)
{
    RecentFilesAction *recent = this->findChild<RecentFilesAction *>
        (QString::fromAscii("recentFiles"));
    if (recent) {
        recent->appendFile(filename);
    }
}

void MainWindow::updateActions()
{
    static QTime cLastCall;

    if (cLastCall.elapsed() > 250 && isVisible()) {
        Application::Instance->commandManager().testActive();
        cLastCall.start();
    }

    d->activityTimer->setSingleShot(true);
    d->activityTimer->start(300);	
}

void MainWindow::switchToTopLevelMode()
{
    QList<QDockWidget*> dw = this->findChildren<QDockWidget*>();
    for (QList<QDockWidget*>::Iterator it = dw.begin(); it != dw.end(); ++it) {
        (*it)->setParent(0, Qt::Window);
        (*it)->show();
    }
    QList<QWidget*> mdi = getMainWindow()->windows();
    for (QList<QWidget*>::Iterator it = mdi.begin(); it != mdi.end(); ++it) {
        (*it)->setParent(0, Qt::Window);
        (*it)->show();
    }
}

void MainWindow::switchToDockedMode()
{
    // Search for all top-level MDI views
    QWidgetList toplevel = QApplication::topLevelWidgets();
    for (QWidgetList::Iterator it = toplevel.begin(); it != toplevel.end(); ++it) {
        Gui::MDIView* view = qobject_cast<MDIView*>(*it);
        if (view)
            view->setCurrentViewMode(MDIView::Child);
    }
}

void MainWindow::loadWindowSettings()
{
    QString vendor = QString::fromAscii(App::Application::Config()["ExeVendor"].c_str());
    QString application = QString::fromAscii(App::Application::Config()["ExeName"].c_str());
    QString version = QString::fromAscii(App::Application::Config()["ExeVersion"].c_str());
    int major = (QT_VERSION >> 0x10) & 0xff;
    int minor = (QT_VERSION >> 0x08) & 0xff;
    QString qtver = QString::fromAscii("Qt%1.%2").arg(major).arg(minor);
    QSettings config(vendor, application);

    config.beginGroup(version);
    config.beginGroup(qtver);
    this->resize(config.value(QString::fromAscii("Size"), this->size()).toSize());
    QPoint pos = config.value(QString::fromAscii("Position"), this->pos()).toPoint();
    QRect rect = QApplication::desktop()->availableGeometry();
    int x1,x2,y1,y2;
    // make sure that the main window is not totally out of the visible rectangle
    rect.getCoords(&x1, &y1, &x2, &y2);
    pos.setX(qMin(qMax(pos.x(),x1-this->width()+30),x2-30));
    pos.setY(qMin(qMax(pos.y(),y1-10),y2-10));
    this->move(pos);

    // tmp. disable the report window to suppress some bothering warnings
    Base::Console().SetEnabledMsgType("ReportOutput", ConsoleMsgType::MsgType_Wrn, false);
    this->restoreState(config.value(QString::fromAscii("MainWindowState")).toByteArray());
    std::clog << "Main window restored" << std::endl;
    Base::Console().SetEnabledMsgType("ReportOutput", ConsoleMsgType::MsgType_Wrn, true);

    bool max = config.value(QString::fromAscii("Maximized"), false).toBool();
    max ? showMaximized() : show();

    statusBar()->setVisible(config.value(QString::fromAscii("StatusBar"), true).toBool());
    config.endGroup();
    config.endGroup();

    ToolBarManager::getInstance()->restoreState();
    std::clog << "Toolbars restored" << std::endl;
}

void MainWindow::saveWindowSettings()
{
    QString vendor = QString::fromAscii(App::Application::Config()["ExeVendor"].c_str());
    QString application = QString::fromAscii(App::Application::Config()["ExeName"].c_str());
    QString version = QString::fromAscii(App::Application::Config()["ExeVersion"].c_str());
    int major = (QT_VERSION >> 0x10) & 0xff;
    int minor = (QT_VERSION >> 0x08) & 0xff;
    QString qtver = QString::fromAscii("Qt%1.%2").arg(major).arg(minor);
    QSettings config(vendor, application);

    config.beginGroup(version);
    config.beginGroup(qtver);
    config.setValue(QString::fromAscii("Size"), this->size());
    config.setValue(QString::fromAscii("Position"), this->pos());
    config.setValue(QString::fromAscii("Maximized"), this->isMaximized());
    config.setValue(QString::fromAscii("MainWindowState"), this->saveState());
    config.setValue(QString::fromAscii("StatusBar"), this->statusBar()->isVisible());
    config.endGroup();
    config.endGroup();

    DockWindowManager::instance()->saveState();
    ToolBarManager::getInstance()->saveState();
}

void MainWindow::startSplasher(void)
{
    // startup splasher
    // when running in verbose mode no splasher
    if (!(App::Application::Config()["Verbose"] == "Strict") && 
         (App::Application::Config()["RunMode"] == "Gui")) {
        ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().
            GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("General");
        // first search for an external imahe file
        if (hGrp->GetBool("ShowSplasher", true)) {
            d->splashscreen = new SplashScreen(this->splashImage());
            d->splashscreen->show();
        }
        else
            d->splashscreen = 0;
    }
}

void MainWindow::stopSplasher(void)
{
    if (d->splashscreen) {
        d->splashscreen->finish(this);
        delete d->splashscreen;
        d->splashscreen = 0;
    }
}

QPixmap MainWindow::splashImage() const
{
    QPixmap splash_image;
    QDir dir(QString::fromUtf8(App::Application::Config()["UserAppData"].c_str()));
    QFileInfo fi(dir.filePath(QString::fromAscii("pixmaps/splash_image.png")));
    if (fi.isFile() && fi.exists())
        splash_image.load(fi.filePath(), "PNG");
    if (splash_image.isNull())
        splash_image = Gui::BitmapFactory().pixmap(App::Application::Config()["SplashScreen"].c_str());

    // include application name and version number
    std::map<std::string,std::string>::const_iterator tc = App::Application::Config().find("SplashInfoColor");
    if (tc != App::Application::Config().end()) {
        QString title = qApp->applicationName();
        QString major   = QString::fromAscii(App::Application::Config()["BuildVersionMajor"].c_str());
        QString minor   = QString::fromAscii(App::Application::Config()["BuildVersionMinor"].c_str());
        QString version = QString::fromAscii("%1.%2").arg(major).arg(minor);

        QPainter painter;
        painter.begin(&splash_image);
        QFont fontExe = painter.font();
        fontExe.setPointSize(20);
        QFontMetrics metricExe(fontExe);
        int l = metricExe.width(title);
        int w = splash_image.width();
        int h = splash_image.height();

        QFont fontVer = painter.font();
        fontVer.setPointSize(12);
        QFontMetrics metricVer(fontVer);
        int v = metricVer.width(version);

        QColor color;
        color.setNamedColor(QString::fromAscii(tc->second.c_str()));
        if (color.isValid()) {
            painter.setPen(color);
            painter.setFont(fontExe);
            painter.drawText(w-(l+v+10),h-20, title);
            painter.setFont(fontVer);
            painter.drawText(w-(v+5),h-20, version);
            painter.end();
        }
    }

    return splash_image;
}

void MainWindow::showTipOfTheDay(bool force)
{
    // tip of the day?
    ParameterGrp::handle
    hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
            GetGroup("Preferences")->GetGroup("General");

    const std::map<std::string,std::string>& config = App::Application::Config();
    std::map<std::string, std::string>::const_iterator tp = config.find("HideTipOfTheDay");
    bool tip = (tp == config.end());

    tip = hGrp->GetBool("Tipoftheday", tip);
    if (tip || force) {
        Gui::Dialog::DlgTipOfTheDayImp dlg(instance);
        dlg.exec();
    }
}

/**
 * Drops the event \a e and tries to open the files.
 */
void MainWindow::dropEvent (QDropEvent* e)
{
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        // pass no document to let create a new one if needed
        loadUrls(0, data->urls());
    }
    else {
        QMainWindow::dropEvent(e);
    }
}

void MainWindow::dragEnterEvent (QDragEnterEvent * e)
{
    // Here we must allow uri drafs and check them in dropEvent
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
#if 0
#ifdef QT_NO_OPENSSL
        QList<QUrl> urls = data->urls();
        for (QList<QUrl>::ConstIterator it = urls.begin(); it != urls.end(); ++it) {
            if (it->scheme().toLower() == QLatin1String("https")) {
                e->ignore();
                return;
            }
        }
#endif
#endif
        e->accept();
    }
    else {
        e->ignore();
    }
}

QMimeData * MainWindow::createMimeDataFromSelection () const
{
    std::vector<SelectionSingleton::SelObj> selobj = Selection().getCompleteSelection();
    std::map< App::Document*, std::vector<App::DocumentObject*> > objs;
    for (std::vector<SelectionSingleton::SelObj>::iterator it = selobj.begin(); it != selobj.end(); ++it) {
        if (it->pObject && it->pObject->getDocument()) {
            objs[it->pObject->getDocument()].push_back(it->pObject);
        }
    }

    if (objs.empty())
        return 0;

    std::vector<App::DocumentObject*> sel; // selected
    std::vector<App::DocumentObject*> all; // object sub-graph
    for (std::map< App::Document*, std::vector<App::DocumentObject*> >::iterator it = objs.begin(); it != objs.end(); ++it) {
        std::vector<App::DocumentObject*> dep = it->first->getDependencyList(it->second);
        sel.insert(sel.end(), it->second.begin(), it->second.end());
        all.insert(all.end(), dep.begin(), dep.end());
    }

    if (all.size() > sel.size()) {
        int ret = QMessageBox::question(getMainWindow(),
            tr("Object dependencies"),
            tr("The selected objects have a dependency to unselected objects.\n"
               "Do you want to copy them, too?"),
            QMessageBox::Yes,QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            sel = all;
        }
    }

    unsigned int memsize=1000; // ~ for the meta-information
    for (std::vector<App::DocumentObject*>::iterator it = sel.begin(); it != sel.end(); ++it)
        memsize += (*it)->getMemSize();

    // if less than ~10 MB
    bool use_buffer=(memsize < 0xA00000);
    QByteArray res;
    try {
        res.reserve(memsize);
    }
    catch (const Base::MemoryException&) {
        use_buffer = false;
    }

    WaitCursor wc;
    QString mime;
    if (use_buffer) {
        mime = QLatin1String("application/x-documentobject");
        Base::ByteArrayOStreambuf buf(res);
        std::ostream str(&buf);
        // need this instance to call MergeDocuments::Save()
        App::Document* doc = sel.front()->getDocument();
        MergeDocuments mimeView(doc);
        doc->exportObjects(sel, str);
    }
    else {
        mime = QLatin1String("application/x-documentobject-file");
        static Base::FileInfo fi(Base::FileInfo::getTempFileName());
        Base::ofstream str(fi, std::ios::out | std::ios::binary);
        // need this instance to call MergeDocuments::Save()
        App::Document* doc = sel.front()->getDocument();
        MergeDocuments mimeView(doc);
        doc->exportObjects(sel, str);
        str.close();
        res = fi.filePath().c_str();
    }

    QMimeData *mimeData = new QMimeData();
    mimeData->setData(mime,res);
    return mimeData;
}

bool MainWindow::canInsertFromMimeData (const QMimeData * source) const
{
    if (!source)
        return false;
    return source->hasUrls() || 
        source->hasFormat(QLatin1String("application/x-documentobject")) ||
        source->hasFormat(QLatin1String("application/x-documentobject-file"));
}

void MainWindow::insertFromMimeData (const QMimeData * mimeData)
{
    if (!mimeData)
        return;
    if (mimeData->hasFormat(QLatin1String("application/x-documentobject"))) {
        QByteArray res = mimeData->data(QLatin1String("application/x-documentobject"));
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) doc = App::GetApplication().newDocument();

        Base::ByteArrayIStreambuf buf(res);
        std::istream in(0);
        in.rdbuf(&buf);
        MergeDocuments mimeView(doc);
        mimeView.importObjects(in);
    }
    else if (mimeData->hasFormat(QLatin1String("application/x-documentobject-file"))) {
        QByteArray res = mimeData->data(QLatin1String("application/x-documentobject-file"));
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) doc = App::GetApplication().newDocument();

        Base::FileInfo fi((const char*)res);
        Base::ifstream str(fi, std::ios::in | std::ios::binary);
        MergeDocuments mimeView(doc);
        mimeView.importObjects(str);
        str.close();
    }
    else if (mimeData->hasUrls()) {
        // load the files into the active document if there is one, otherwise let create one
        loadUrls(App::GetApplication().getActiveDocument(), mimeData->urls());
    }
}

void MainWindow::loadUrls(App::Document* doc, const QList<QUrl>& url)
{
    QStringList files;
    for (QList<QUrl>::ConstIterator it = url.begin(); it != url.end(); ++it) {
        QFileInfo info((*it).toLocalFile());
        if (info.exists() && info.isFile()) {
            if (info.isSymLink())
                info.setFile(info.readLink());
            std::vector<std::string> module = App::GetApplication()
                .getImportModules(info.completeSuffix().toAscii());
            if (module.empty()) {
                module = App::GetApplication()
                    .getImportModules(info.suffix().toAscii());
            }
            if (!module.empty()) {
                // ok, we support files with this extension
                files << info.absoluteFilePath();
            }
            else {
                Base::Console().Message("No support to load file '%s'\n",
                    (const char*)info.absoluteFilePath().toUtf8());
            }
        }
        else if (it->scheme().toLower() == QLatin1String("http")) {
            Gui::Dialog::DownloadManager::getInstance()->download(*it);
        }
//#ifndef QT_NO_OPENSSL
        else if (it->scheme().toLower() == QLatin1String("https")) {
            QUrl url = *it;
            if (it->hasEncodedQueryItem(QByteArray("sid"))) {
                url.removeEncodedQueryItem(QByteArray("sid"));
                url.setScheme(QLatin1String("http"));
            }
            Gui::Dialog::DownloadManager::getInstance()->download(url);
        }
//#endif
        else if (it->scheme().toLower() == QLatin1String("ftp")) {
            Gui::Dialog::DownloadManager::getInstance()->download(*it);
        }
    }

    const char *docName = doc ? doc->getName() : "Unnamed";
    SelectModule::Dict dict = SelectModule::importHandler(files);
    // load the files with the associated modules
    for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
        // if the passed document name doesn't exist the module should create it, if needed
        Application::Instance->importFrom(it.key().toUtf8(), docName, it.value().toAscii());
    }
}

void MainWindow::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        d->sizeLabel->setText(tr("Dimension"));
    
        CommandManager& rclMan = Application::Instance->commandManager();
        std::vector<Command*> cmd = rclMan.getAllCommands();
        for (std::vector<Command*>::iterator it = cmd.begin(); it != cmd.end(); ++it)
            (*it)->languageChange();

        // reload current workbench to retranslate all actions and window titles
        Workbench* wb = WorkbenchManager::instance()->active();
        if (wb) wb->retranslate();
    }
    else {
        QMainWindow::changeEvent(e);
    }
}

void MainWindow::showMessage (const QString& message, int timeout)
{
    QFontMetrics fm(statusBar()->font());
    QString msg = fm.elidedText(message, Qt::ElideMiddle, this->width()/2);
#if QT_VERSION != 0x040801
    this->statusBar()->showMessage(msg, timeout);
#else
    //#0000665: There is a crash under Ubuntu 12.04 (Qt 4.8.1)
    QMetaObject::invokeMethod(statusBar(), "showMessage",
        Qt::QueuedConnection,
        QGenericReturnArgument(),
        Q_ARG(QString,msg),
        Q_ARG(int, timeout));
#endif
}

// -------------------------------------------------------------

namespace Gui {

/**
 * The CustomMessageEvent class is used to send messages as events in the methods  
 * Error(), Warning() and Message() of the StatusBarObserver class to the main window 
 * to display them on the status bar instead of printing them directly to the status bar.
 *
 * This makes the usage of StatusBarObserver thread-safe.
 * @author Werner Mayer
 */
class CustomMessageEvent : public QEvent
{
public:
    enum Type {Msg, Wrn, Err, Log};
    CustomMessageEvent(Type t, const QString& s)
      : QEvent(QEvent::User), _type(t), msg(s)
    { }
    ~CustomMessageEvent()
    { }
    Type type() const
    { return _type; }
    const QString& message() const
    { return msg; }
private:
    Type _type;
    QString msg;
};
}

void MainWindow::customEvent(QEvent* e)
{
    if (e->type() == QEvent::User) {
        Gui::CustomMessageEvent* ce = static_cast<Gui::CustomMessageEvent*>(e);
        QString msg = ce->message();
        if (ce->type() == CustomMessageEvent::Log) {
            if (msg.startsWith(QLatin1String("#Inventor V2.1 ascii "))) {
                Gui::Document *d = Application::Instance->activeDocument();
                if (d) {
                    ViewProviderExtern *view = new ViewProviderExtern();
                    try {
                        view->setModeByString("1",msg.toAscii().constData());
                        d->setAnnotationViewProvider("Vdbg",view);
                    }
                    catch (...) {
                        delete view;
                    }
                }
            }
        }
        else {
            d->actionLabel->setText(msg);
            d->actionTimer->setSingleShot(true);
            d->actionTimer->start(5000);
        }
    }
}

// ----------------------------------------------------------

StatusBarObserver::StatusBarObserver()
  : WindowParameter("OutputWindow")
{
    msg = QString::fromAscii("#000000"); // black
    wrn = QString::fromAscii("#ffaa00"); // orange
    err = QString::fromAscii("#ff0000"); // red
    Base::Console().AttachObserver(this);
    getWindowParameter()->Attach(this);
    getWindowParameter()->NotifyAll();
}

StatusBarObserver::~StatusBarObserver()
{
    getWindowParameter()->Detach(this);
    Base::Console().DetachObserver(this);
}

void StatusBarObserver::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
    if (strcmp(sReason, "colorText") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        this->msg = QColor((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff).name();
    }
    else if (strcmp(sReason, "colorWarning") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        this->wrn = QColor((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff).name();
    }
    else if (strcmp(sReason, "colorError") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        this->err = QColor((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff).name();
    }
}

/** Get called when a message is issued. 
 * The message is displayed on the ststus bar. 
 */
void StatusBarObserver::Message(const char * m)
{
    // Send the event to the main window to allow thread-safety. Qt will delete it when done.
    QString txt = QString::fromAscii("<font color=\"%1\">%2</font>").arg(this->msg).arg(QString::fromUtf8(m));
    CustomMessageEvent* ev = new CustomMessageEvent(CustomMessageEvent::Msg, txt);
    QApplication::postEvent(getMainWindow(), ev);
}

/** Get called when a warning is issued. 
 * The message is displayed on the ststus bar. 
 */
void StatusBarObserver::Warning(const char *m)
{
    // Send the event to the main window to allow thread-safety. Qt will delete it when done.
    QString txt = QString::fromAscii("<font color=\"%1\">%2</font>").arg(this->wrn).arg(QString::fromUtf8(m));
    CustomMessageEvent* ev = new CustomMessageEvent(CustomMessageEvent::Wrn, txt);
    QApplication::postEvent(getMainWindow(), ev);
}

/** Get called when an error is issued. 
 * The message is displayed on the ststus bar. 
 */
void StatusBarObserver::Error  (const char *m)
{
    // Send the event to the main window to allow thread-safety. Qt will delete it when done.
    QString txt = QString::fromAscii("<font color=\"%1\">%2</font>").arg(this->err).arg(QString::fromUtf8(m));
    CustomMessageEvent* ev = new CustomMessageEvent(CustomMessageEvent::Err, txt);
    QApplication::postEvent(getMainWindow(), ev);
}

/** Get called when a log message is issued. 
 * The message is used to create an Inventor node for debug purposes. 
 */
void StatusBarObserver::Log(const char *m)
{
    // Send the event to the main window to allow thread-safety. Qt will delete it when done.
    CustomMessageEvent* ev = new CustomMessageEvent(CustomMessageEvent::Log, QString::fromUtf8(m));
    QApplication::postEvent(getMainWindow(), ev);
}


#include "moc_MainWindow.cpp"
