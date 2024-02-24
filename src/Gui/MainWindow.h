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


#ifndef GUI_MAINWINDOW_H
#define GUI_MAINWINDOW_H

#include <QEvent>
#include <QMainWindow>
#include <QMdiArea>

#include "Window.h"

class QMimeData;
class QUrl;
class QMdiSubWindow;

namespace App {
class Document;
}

namespace Gui {

class BaseView;
class CommandManager;
class Document;
class MacroManager;
class MDIView;

namespace DockWnd {
    class HelpView;
} //namespace DockWnd

class GuiExport UrlHandler : public QObject
{
    Q_OBJECT

public:
    explicit UrlHandler(QObject* parent = nullptr)
        : QObject(parent){
    }
    ~UrlHandler() override = default;
    virtual void openUrl(App::Document*, const QUrl&) {
    }
};

/**
 * The MainWindow class provides a main window with menu bar, toolbars, dockable windows,
 * a status bar and mainly a workspace for the MDI windows.
 * @author Werner Mayer
 */
class GuiExport MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum ConfirmSaveResult {
        Cancel = 0,
        Save,
        SaveAll,
        Discard,
        DiscardAll
    };
    /**
     * Constructs an empty main window. For default \a parent is 0, as there usually is
     * no toplevel window there.
     */
    explicit MainWindow(QWidget * parent = nullptr, Qt::WindowFlags f = Qt::Window);
    /** Destroys the object and frees any allocated resources. */
    ~MainWindow() override;
    /**
     * Filters events if this object has been installed as an event filter for the watched object.
     */
    bool eventFilter(QObject* o, QEvent* e) override;
    /**
     * Adds an MDI window \a view to the main window's workspace and adds a new tab
     * to the tab bar.
     */
    void addWindow(MDIView* view);
    /**
     * Removes an MDI window from the main window's workspace and its associated tab without
     * deleting the widget. If the main windows does not have such a window nothing happens.
     */
    void removeWindow(MDIView* view, bool close=true);
    /**
     * Returns a list of all MDI windows in the worpspace.
     */
    QList<QWidget*> windows(QMdiArea::WindowOrder order = QMdiArea::CreationOrder) const;
    /**
     * Returns the internal QMdiArea instance.
     */
    QMdiArea *getMdiArea() const;
    /**
     * Can be called after the caption of an MDIView has changed to update the tab's caption.
     */
    void tabChanged(MDIView* view);
    /**
     * Returns the active MDI window or 0 if there is none.
     */
    MDIView* activeWindow() const;
    /**
     * Sets the active window to \a view.
     */
    void setActiveWindow(MDIView* view);
    /**
     * MRU: Appends \a file to the list of recent files.
     */
    void appendRecentFile(const QString& filename);
    /**
     * MRU: Appends \a macro to the list of recent macros.
     */
    void appendRecentMacro(const QString& filename);
    /**
     * Returns true that the context menu contains the 'Customize...' menu item.
     */
    QMenu * createPopupMenu() override;

    /** @name Splasher and access methods */
    //@{
    /** Gets the one and only instance. */
    static MainWindow* getInstance();
    /** Starts the splasher at startup. */
    void startSplasher();
    /** Stops the splasher after startup. */
    void stopSplasher();
    /* The image of the About dialog, it might be empty. */
    QPixmap aboutImage() const;
    /* The image of the splash screen of the application. */
    QPixmap splashImage() const;
    /** Shows the online documentation. */
    void showDocumentation(const QString& help);
    //@}

    /** @name Layout Methods
     */
    //@{
    /// Loads the main window settings.
    void loadWindowSettings();
    /// Saves the main window settings.
    void saveWindowSettings(bool canDelay = false);
    //@}

    /** @name Menu
     */
    //@{
    /// Set menu for dock windows.
    void setDockWindowMenu(QMenu*);
    /// Set menu for toolbars.
    void setToolBarMenu(QMenu*);
    /// Set menu for sub-windows
    void setWindowsMenu(QMenu*);
    //@}

    /** @name MIME data handling
     */
    //@{
    /** Create mime data from selected objects */
    QMimeData * createMimeDataFromSelection () const;
    /** Check if mime data contains object data */
    bool canInsertFromMimeData (const QMimeData * source) const;
    /** Insert the objects into the active document. If no document exists
     * one gets created.
     */
    void insertFromMimeData (const QMimeData * source);
    /**
     * Load files from the given URLs into the given document. If the document is 0
     * one gets created automatically if needed.
     *
     * If a url handler is registered that supports its scheme it will be delegated
     * to this handler. This mechanism allows to change the default behaviour.
     */
    void loadUrls(App::Document*, const QList<QUrl>&);
    /**
     * Sets the \a handler for the given \a scheme.
     * If setUrlHandler() is used to set a new handler for a scheme which already has a handler,
     * the existing handler is simply replaced with the new one. Since MainWindow does not take
     * ownership of handlers, no objects are deleted when a handler is replaced.
     */
    void setUrlHandler(const QString &scheme, UrlHandler* handler);
    /**
     * Removes a previously set URL handler for the specified \a scheme.
     */
    void unsetUrlHandler(const QString &scheme);
    //@}

    void updateActions(bool delay = false);

    enum StatusType {None, Err, Wrn, Pane, Msg, Log, Tmp, Critical};
    void showStatus(int type, const QString & message);

    void initDockWindows(bool show);

    bool isRestoringWindowState() const;

public Q_SLOTS:
    /**
     * Updates the standard actions of a text editor such as Cut, Copy, Paste, Undo and Redo.
     */
    void updateEditorActions();
    /**
     * Sets text to the pane in the status bar.
     */
    void setPaneText(int i, QString text);
    /**
     * Sets the userschema in the status bar
    */
    void setUserSchema(int userSchema);
    /**
     * Arranges all child windows in a tile pattern.
     */
    void tile();
    /**
     * Arranges all the child windows in a cascade pattern.
     */
    void cascade();
    /**
     * Closes the child window that is currently active.
     */
    void closeActiveWindow ();
    /**
     * Closes all document window.
     */
    bool closeAllDocuments (bool close=true);
    /** Pop up a message box asking for saving document
     */
    int confirmSave(const char *docName, QWidget *parent=nullptr, bool addCheckBox=false);
    /**
     * Activates the next window in the child window chain.
     */
    void activateNextWindow ();
    /**
     * Activates the previous window in the child window chain.
     */
    void activatePreviousWindow ();
    /**
     * Just emits the workbenchActivated() signal to notify all receivers.
     */
    void activateWorkbench(const QString&);
    /**
     * Starts the what's this mode.
     */
    void whatsThis();
    void switchToTopLevelMode();
    void switchToDockedMode();

    void statusMessageChanged();

    void showMessage (const QString & message, int timeout = 0);

    // Set main window title
    void setWindowTitle(const QString& string);

protected:
    /**
     * This method checks if the main window can be closed by checking all open documents and views.
     */
    void closeEvent (QCloseEvent * e) override;
    void showEvent  (QShowEvent  * e) override;
    void hideEvent  (QHideEvent  * e) override;
    void timerEvent (QTimerEvent *  ) override {
        Q_EMIT timeEvent();
    }
    void customEvent(QEvent      * e) override;
    bool event      (QEvent      * e) override;
    /**
     * Try to interpret dropped elements.
     */
    void dropEvent  (QDropEvent  * e) override;
    /**
     * Checks if a mime source object can be interpreted.
     */
    void dragEnterEvent(QDragEnterEvent * e) override;
    /**
     * This method is called from the Qt framework automatically whenever a
     * QTranslator object has been installed. This allows to translate all
     * relevant user visible text.
     */
    void changeEvent(QEvent *e) override;

private:
    void setupDockWindows();
    bool setupTaskView();
    bool setupSelectionView();
    bool setupReportView();
    bool setupPythonConsole();
    bool updateTreeView(bool show);
    bool updatePropertyView(bool show);
    bool updateTaskView(bool show);
    bool updateComboView(bool show);
    bool updateDAGView(bool show);

    static void renderDevBuildWarning(QPainter &painter, const QPoint startPosition, const QSize maxSize);

private Q_SLOTS:
    /**
     * \internal
     */
    void onSetActiveSubWindow(QWidget *window);
    /**
     * Activates the associated tab to this widget.
     */
    void onWindowActivated(QMdiSubWindow*);
    /**
     * Close tab at position index.
     */
    void tabCloseRequested(int index);
    /**
     * Fills up the menu with the current windows in the workspace.
     */
    void onWindowsMenuAboutToShow();
    /**
     * Fills up the menu with the current toolbars.
     */
    void onToolBarMenuAboutToShow();
    /**
     * Fills up the menu with the current dock windows.
     */
    void onDockWindowMenuAboutToShow();
    /**
     * This method gets frequently activated and test the commands if they are still active.
     */
    void _updateActions();
    /**
     * \internal
     */
    void delayedStartup();
    /**
     * \internal
     */
    void processMessages(const QList<QByteArray> &);
    /**
     * \internal
     */
    void clearStatus();

Q_SIGNALS:
    void timeEvent();
    void windowStateChanged(QWidget*);
    void workbenchActivated(const QString&);
    void mainWindowClosed();

private:
    /// some kind of singleton
    static MainWindow* instance;
    struct MainWindowP* d;
};

inline MainWindow* getMainWindow()
{
    return MainWindow::getInstance();
}

// -------------------------------------------------------------

/** The status bar observer displays the text on the status bar of the main window
 * in an appropriate color. Normal text messages are black, warnings are orange and
 * error messages are in red. Log messages are completely ignored.
 * The class is implemented to be thread-safe.
 * @see Console
 * @see ILogger
 * @author Werner Mayer
 */
class StatusBarObserver: public WindowParameter, public Base::ILogger
{
public:
    StatusBarObserver();
    ~StatusBarObserver() override;

    /** Observes its parameter group. */
    void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override;

    void SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level,
                 Base::IntendedRecipient recipient, Base::ContentType content) override;

    /// name of the observer
    const char *Name() override {return "StatusBar";}

    friend class MainWindow;
private:
    QString msg, wrn, err, critical;
};

// -------------------------------------------------------------

/** This is a helper class needed when a style sheet is restored or cleared.
 * @author Werner Mayer
 */
class ActionStyleEvent : public QEvent
{
public:
    static int EventType;
    enum Style {Restore, Clear};

    explicit ActionStyleEvent(Style type);
    Style getType() const;

private:
    Style type;
};

} // namespace Gui

#endif // GUI_MAINWINDOW_H
