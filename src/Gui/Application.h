/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef APPLICATION_H
#define APPLICATION_H

#include <QPixmap>
#include <string>
#include <vector>

#define  putpix()

#include <App/Application.h>

class QCloseEvent;

namespace Gui{
class BaseView;
class CommandManager;
class Document;
class MacroManager;
class MDIView;
class MainWindow;
class MenuItem;
class ViewProvider;

/** The Applcation main class
 * This is the central class of the GUI 
 * @author Jürgen Riegel, Werner Mayer
 */
class GuiExport Application 
{
public:
    /// construction
    Application(bool GUIenabled);
    /// destruction
    ~Application();

    /** @name methods for support of files */
    //@{
    /// open a file
    void open(const char* FileName, const char* Module);
    /// import a file into the document DocName
    void importFrom(const char* FileName, const char* DocName, const char* Module);
    /// Export objects from the document DocName to a single file
    void exportTo(const char* FileName, const char* DocName, const char* Module);
    //@}


    /** @name methods for View handling */
    //@{
    /// send Messages to the active view
    bool sendMsgToActiveView(const char* pMsg, const char** ppReturn=0);
    /// send Messages test to the active view
    bool sendHasMsgToActiveView(const char* pMsg);
    /// Attach a view (get called by the FCView constructor)
    void attachView(Gui::BaseView* pcView);
    /// Detach a view (get called by the FCView destructor)
    void detachView(Gui::BaseView* pcView);
    /// get called if a view gets activated, this manage the whole activation scheme
    void viewActivated(Gui::MDIView* pcView);
    /// call update to all docuemnts an all views (costly!)
    void onUpdate(void);
    /// call update to all views of the active document
    void updateActive(void);
    //@}

    /** @name Signals of the Application */
    //@{
    /// signal on new Document
    boost::signal<void (const Gui::Document&)> signalNewDocument;
    /// signal on deleted Document
    boost::signal<void (const Gui::Document&)> signalDeleteDocument;
    /// signal on relabeling Document
    boost::signal<void (const Gui::Document&)> signalRelabelDocument;
    /// signal on renaming Document
    boost::signal<void (const Gui::Document&)> signalRenameDocument;
    /// signal on activating Document
    boost::signal<void (const Gui::Document&)> signalActiveDocument;
    /// signal on new Object
    boost::signal<void (const Gui::ViewProvider&)> signalNewObject;
    /// signal on deleted Object
    boost::signal<void (const Gui::ViewProvider&)> signalDeletedObject;
    /// signal on changed object property
    boost::signal<void (const Gui::ViewProvider&, const App::Property&)> signalChangedObject;
    /// signal on renamed Object
    boost::signal<void (const Gui::ViewProvider&)> signalRenamedObject;
    /// signal on activated Object
    boost::signal<void (const Gui::ViewProvider&)> signalActivatedObject;
    /// signal on activated workbench
    boost::signal<void (const char*)> signalActivateWorkbench;
    /// signal on added workbench
    boost::signal<void (const char*)> signalAddWorkbench;
    /// signal on removed workbench
    boost::signal<void (const char*)> signalRemoveWorkbench;
    /// signal on activating view
    boost::signal<void (const Gui::MDIView*)> signalActivateView;
    //@}

    /** @name methods for Document handling */
    //@{
protected:
    /// Observer message from the Application
    void slotNewDocument(const App::Document&);
    void slotDeleteDocument(const App::Document&);
    void slotRelabelDocument(const App::Document&);
    void slotRenameDocument(const App::Document&);
    void slotActiveDocument(const App::Document&);
    void slotNewObject(const ViewProvider&);
    void slotDeletedObject(const ViewProvider&);
    void slotChangedObject(const ViewProvider&, const App::Property& Prop);
    void slotRenamedObject(const ViewProvider&);
    void slotActivatedObject(const ViewProvider&);

public:
    /// message when a GuiDocument is about to vanish
    void onLastWindowClosed(Gui::Document* pcDoc);
    /// Getter for the active document
    Gui::Document* activeDocument(void) const;
    /// Set the active document
    void setActiveDocument(Gui::Document* pcDocument);
    /** Retrieves a pointer to the Gui::Document whose App::Document has the name \a name.
    * If no such document exists 0 is returned.
    */
    Gui::Document* getDocument(const char* name) const;
    /** Retrieves a pointer to the Gui::Document whose App::Document matches to \a pDoc.
    * If no such document exists 0 is returned.
    */
    Gui::Document* getDocument(const App::Document* pDoc) const;
    /// Shows the associated view provider of the given object
    void showViewProvider(const App::DocumentObject*);
    /// Hides the associated view provider of the given object
    void hideViewProvider(const App::DocumentObject*);
    /// Get the view provider of the given object
    Gui::ViewProvider* getViewProvider(const App::DocumentObject*) const;
    //@}

    /// true when the application shuting down
    bool isClosing(void);
    void checkForPreviousCrashes();

    /** @name workbench handling */
    //@{
    /// Activate a named workbench
    bool activateWorkbench(const char* name);
    QPixmap workbenchIcon(const QString&) const;
    QString workbenchToolTip(const QString&) const;
    QString workbenchMenuText(const QString&) const;
    QStringList workbenches(void) const;
    void setupContextMenu(const char* recipient, MenuItem*) const;
    //@}

    /** @name User Commands */
    //@{
    /// Get macro manager
    Gui::MacroManager *macroManager(void);
    /// Reference to the command manager
    Gui::CommandManager &commandManager(void);
    /// Run a Python command
    void runCommand(bool bForce, const char* sCmd,...);
    bool runPythonCode(const char* cmd, bool gui=false, bool pyexc=true);
    /// helper which create the commands
    void createStandardOperations();
    //@}

    /** @name Init, Destruct an Access methods */
    //@{
    /// some kind of singelton
    static Application* Instance;
    static void initApplication(void);
    static void initTypes(void);
    static void runApplication(void);
    void tryClose( QCloseEvent * e );
    //@}

public:
    //---------------------------------------------------------------------
    // python exports goes here +++++++++++++++++++++++++++++++++++++++++++	
    //---------------------------------------------------------------------
    // static python wrapper of the exported functions
    PYFUNCDEF_S(sActivateWorkbenchHandler); // activates a workbench object
    PYFUNCDEF_S(sAddWorkbenchHandler);      // adds a new workbench handler to a list
    PYFUNCDEF_S(sRemoveWorkbenchHandler);   // removes a workbench handler from the list
    PYFUNCDEF_S(sGetWorkbenchHandler);      // retrieves the workbench handler
    PYFUNCDEF_S(sListWorkbenchHandlers);    // retrieves a list of all workbench handlers
    PYFUNCDEF_S(sActiveWorkbenchHandler);   // retrieves the active workbench object
    PYFUNCDEF_S(sAddResPath);               // adds a path where to find resources
    PYFUNCDEF_S(sAddLangPath);              // adds a path to a qm file
    PYFUNCDEF_S(sAddIconPath);              // adds a path to an icon file
    PYFUNCDEF_S(sAddIcon);                  // adds an icon to the cache

    PYFUNCDEF_S(sSendActiveView);

    PYFUNCDEF_S(sGetMainWindow);
    PYFUNCDEF_S(sUpdateGui);
    PYFUNCDEF_S(sUpdateLocale);
    PYFUNCDEF_S(sGetLocale);
    PYFUNCDEF_S(sCreateDialog);
    PYFUNCDEF_S(sAddPreferencePage);

    PYFUNCDEF_S(sRunCommand);
    PYFUNCDEF_S(sAddCommand);

    PYFUNCDEF_S(sHide);                     // deprecated
    PYFUNCDEF_S(sShow);                     // deprecated
    PYFUNCDEF_S(sHideObject);               // hide view provider object
    PYFUNCDEF_S(sShowObject);               // show view provider object

    PYFUNCDEF_S(sOpen);                     // open Python scripts
    PYFUNCDEF_S(sInsert);                   // open Python scripts
    PYFUNCDEF_S(sExport);

    PYFUNCDEF_S(sActiveDocument);
    PYFUNCDEF_S(sGetDocument);

    PYFUNCDEF_S(sDoCommand);
    PYFUNCDEF_S(sAddModule);

    static PyMethodDef    Methods[]; 

private:
    struct ApplicationP* d;
    /// workbench python dictionary
    PyObject*             _pcWorkbenchDictionary;
};

} //namespace Gui

#endif
