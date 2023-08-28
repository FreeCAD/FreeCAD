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
#include <map>
#include <string>

#define  putpix()

#include <App/Application.h>

class QCloseEvent;
class SoNode;

namespace Gui{
class BaseView;
class CommandManager;
class Document;
class MacroManager;
class MDIView;
class MainWindow;
class MenuItem;
class PreferencePackManager;
class ViewProvider;
class ViewProviderDocumentObject;

/** The Application main class
 * This is the central class of the GUI
 * @author Jürgen Riegel, Werner Mayer
 */
class GuiExport Application
{
public:
    enum Status {
        UserInitiatedOpenDocument = 0
    };

    /// construction
    explicit Application(bool GUIenabled);
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
    /// Reload a partial opened document
    App::Document *reopen(App::Document *doc);
    //@}


    /** @name methods for View handling */
    //@{
    /// send Messages to the active view
    bool sendMsgToActiveView(const char* pMsg, const char** ppReturn=nullptr);
    /// send Messages test to the active view
    bool sendHasMsgToActiveView(const char* pMsg);
    /// send Messages to the focused view
    bool sendMsgToFocusView(const char* pMsg, const char** ppReturn=nullptr);
    /// send Messages test to the focused view
    bool sendHasMsgToFocusView(const char* pMsg);
    /// Attach a view (get called by the FCView constructor)
    void attachView(Gui::BaseView* pcView);
    /// Detach a view (get called by the FCView destructor)
    void detachView(Gui::BaseView* pcView);
    /// get called if a view gets activated, this manage the whole activation scheme
    void viewActivated(Gui::MDIView* pcView);
    /// call update to all documents and all views (costly!)
    void onUpdate();
    /// call update to all views of the active document
    void updateActive();
    /// call update to all command actions
    void updateActions(bool delay = false);
    //@}

    /** @name Signals of the Application */
    //@{
    /// signal on new Document
    boost::signals2::signal<void (const Gui::Document&, bool)> signalNewDocument;
    /// signal on deleted Document
    boost::signals2::signal<void (const Gui::Document&)> signalDeleteDocument;
    /// signal on relabeling Document
    boost::signals2::signal<void (const Gui::Document&)> signalRelabelDocument;
    /// signal on renaming Document
    boost::signals2::signal<void (const Gui::Document&)> signalRenameDocument;
    /// signal on activating Document
    boost::signals2::signal<void (const Gui::Document&)> signalActiveDocument;
    /// signal on new Object
    boost::signals2::signal<void (const Gui::ViewProvider&)> signalNewObject;
    /// signal on deleted Object
    boost::signals2::signal<void (const Gui::ViewProvider&)> signalDeletedObject;
    /// signal on changed Object
    boost::signals2::signal<void (const Gui::ViewProvider&, const App::Property&)> signalBeforeChangeObject;
    /// signal on changed object property
    boost::signals2::signal<void (const Gui::ViewProvider&, const App::Property&)> signalChangedObject;
    /// signal on renamed Object
    boost::signals2::signal<void (const Gui::ViewProvider&)> signalRelabelObject;
    /// signal on activated Object
    boost::signals2::signal<void (const Gui::ViewProvider&)> signalActivatedObject;
    /// signal on activated workbench
    boost::signals2::signal<void (const char*)> signalActivateWorkbench;
    /// signal on added/removed workbench
    boost::signals2::signal<void ()> signalRefreshWorkbenches;
    /// signal on show hidden items
    boost::signals2::signal<void (const Gui::Document&)> signalShowHidden;
    /// signal on activating view
    boost::signals2::signal<void (const Gui::MDIView*)> signalActivateView;
    /// signal on entering in edit mode
    boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&)> signalInEdit;
    /// signal on leaving edit mode
    boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&)> signalResetEdit;
    /// signal on changing user edit mode
    boost::signals2::signal<void (int)> signalUserEditModeChanged;
    //@}

    /** @name methods for Document handling */
    //@{
protected:
    /// Observer message from the Application
    void slotNewDocument(const App::Document&,bool);
    void slotDeleteDocument(const App::Document&);
    void slotRelabelDocument(const App::Document&);
    void slotRenameDocument(const App::Document&);
    void slotActiveDocument(const App::Document&);
    void slotShowHidden(const App::Document&);
    void slotNewObject(const ViewProvider&);
    void slotDeletedObject(const ViewProvider&);
    void slotChangedObject(const ViewProvider&, const App::Property& Prop);
    void slotRelabelObject(const ViewProvider&);
    void slotActivatedObject(const ViewProvider&);
    void slotInEdit(const Gui::ViewProviderDocumentObject&);
    void slotResetEdit(const Gui::ViewProviderDocumentObject&);

public:
    /// message when a GuiDocument is about to vanish
    void onLastWindowClosed(Gui::Document* pcDoc);
    /// Getter for the active document
    Gui::Document* activeDocument() const;
    /// Set the active document
    void setActiveDocument(Gui::Document* pcDocument);
    /// Getter for the editing document
    Gui::Document* editDocument() const;
    Gui::MDIView* editViewOfNode(SoNode *node) const;
    /// Set editing document, which will reset editing of all other document
    void setEditDocument(Gui::Document* pcDocument);
    /** Retrieves a pointer to the Gui::Document whose App::Document has the name \a name.
    * If no such document exists 0 is returned.
    */
    Gui::Document* getDocument(const char* name) const;
    /** Retrieves a pointer to the Gui::Document whose App::Document matches to \a pDoc.
    * If no such document exists 0 is returned.
    */
    Gui::Document* getDocument(const App::Document* pDoc) const;
    /// Getter for the active view of the active document or null
    Gui::MDIView* activeView() const;
    /// Activate a view of the given type of the active document
    void activateView(const Base::Type&, bool create=false);
    /// Shows the associated view provider of the given object
    void showViewProvider(const App::DocumentObject*);
    /// Hides the associated view provider of the given object
    void hideViewProvider(const App::DocumentObject*);
    /// Get the view provider of the given object
    Gui::ViewProvider* getViewProvider(const App::DocumentObject*) const;
    //@}

    /// true when the application shutting down
    bool isClosing();

    void checkForDeprecatedSettings();
    void checkForPreviousCrashes();

    /** @name workbench handling */
    //@{
    /// Activate a named workbench
    bool activateWorkbench(const char* name);
    QPixmap workbenchIcon(const QString&) const;
    QString workbenchToolTip(const QString&) const;
    QString workbenchMenuText(const QString&) const;
    QStringList workbenches() const;
    void setupContextMenu(const char* recipient, MenuItem*) const;
    //@}

    /** @name Appearance */
    //@{
    /// Activate a stylesheet
    void setStyleSheet(const QString& qssFile, bool tiledBackground);
    QString replaceVariablesInQss(QString qssText);
    //@}

    /** @name User Commands */
    //@{
    /// Get macro manager
    Gui::MacroManager *macroManager();
    /// Reference to the command manager
    Gui::CommandManager &commandManager();
    /// helper which create the commands
    void createStandardOperations();
    //@}

    Gui::PreferencePackManager* prefPackManager();

    /** @name Init, Destruct an Access methods */
    //@{
    /// some kind of singleton
    static Application* Instance;
    static void initApplication();
    static void initTypes();
    static void initOpenInventor();
    static void runInitGuiScript();
    static void runApplication();
    void tryClose( QCloseEvent * e );
    //@}

    /// return the status bits
    bool testStatus(Status pos) const;
    /// set the status bits
    void setStatus(Status pos, bool on);

    /** @name User edit mode */
    //@{
protected:
    // the below std::map is a translation of 'EditMode' enum in ViewProvider.h
    // to add a new edit mode, it should first be added there
    // this is only used for GUI user interaction (menu, toolbar, Python API)
    const std::map<int, std::pair<std::string, std::string>> userEditModes {
        {0,
         std::make_pair(
             QT_TRANSLATE_NOOP("EditMode", "Default"),
             QT_TRANSLATE_NOOP("EditMode",
                               "The object will be edited using the mode defined internally to be "
                               "the most appropriate for the object type"))},
        {1,
         std::make_pair(QT_TRANSLATE_NOOP("EditMode", "Transform"),
                        QT_TRANSLATE_NOOP("EditMode",
                                          "The object will have its placement editable with the "
                                          "Std TransformManip command"))},
        {2,
         std::make_pair(QT_TRANSLATE_NOOP("EditMode", "Cutting"),
                        QT_TRANSLATE_NOOP("EditMode",
                                          "This edit mode is implemented as available but "
                                          "currently does not seem to be used by any object"))},
        {3,
         std::make_pair(QT_TRANSLATE_NOOP("EditMode", "Color"),
                        QT_TRANSLATE_NOOP("EditMode",
                                          "The object will have the color of its individual faces "
                                          "editable with the Part FaceColors command"))},
    };
    int userEditMode = userEditModes.begin()->first;

public:
    std::map <int, std::pair<std::string,std::string>> listUserEditModes() const { return userEditModes; }
    int getUserEditMode(const std::string &mode = "") const;
    std::pair<std::string,std::string> getUserEditModeUIStrings(int mode = -1) const;
    bool setUserEditMode(int mode);
    bool setUserEditMode(const std::string &mode);
    //@}

public:
    //---------------------------------------------------------------------
    // python exports goes here +++++++++++++++++++++++++++++++++++++++++++
    //---------------------------------------------------------------------
    // static python wrapper of the exported functions
    static PyObject* sActivateWorkbenchHandler (PyObject *self,PyObject *args); // activates a workbench object
    static PyObject* sAddWorkbenchHandler      (PyObject *self,PyObject *args); // adds a new workbench handler to a list
    static PyObject* sRemoveWorkbenchHandler   (PyObject *self,PyObject *args); // removes a workbench handler from the list
    static PyObject* sGetWorkbenchHandler      (PyObject *self,PyObject *args); // retrieves the workbench handler
    static PyObject* sListWorkbenchHandlers    (PyObject *self,PyObject *args); // retrieves a list of all workbench handlers
    static PyObject* sActiveWorkbenchHandler   (PyObject *self,PyObject *args); // retrieves the active workbench object
    static PyObject* sAddResPath               (PyObject *self,PyObject *args); // adds a path where to find resources
    static PyObject* sAddLangPath              (PyObject *self,PyObject *args); // adds a path to a qm file
    static PyObject* sAddIconPath              (PyObject *self,PyObject *args); // adds a path to an icon file
    static PyObject* sAddIcon                  (PyObject *self,PyObject *args); // adds an icon to the cache
    static PyObject* sGetIcon                  (PyObject *self,PyObject *args); // get an icon from the cache
    static PyObject* sIsIconCached             (PyObject *self,PyObject *args); // check if an icon is cached

    static PyObject* sSendActiveView           (PyObject *self,PyObject *args);
    static PyObject* sSendFocusView            (PyObject *self,PyObject *args);

    static PyObject* sGetMainWindow            (PyObject *self,PyObject *args);
    static PyObject* sUpdateGui                (PyObject *self,PyObject *args);
    static PyObject* sUpdateLocale             (PyObject *self,PyObject *args);
    static PyObject* sGetLocale                (PyObject *self,PyObject *args);
    static PyObject* sSetLocale                (PyObject *self,PyObject *args);
    static PyObject* sSupportedLocales         (PyObject *self,PyObject *args);
    static PyObject* sCreateDialog             (PyObject *self,PyObject *args);
    static PyObject* sAddPreferencePage        (PyObject *self,PyObject *args);

    static PyObject* sRunCommand               (PyObject *self,PyObject *args);
    static PyObject* sAddCommand               (PyObject *self,PyObject *args);

    static PyObject* sHide                     (PyObject *self,PyObject *args); // deprecated
    static PyObject* sShow                     (PyObject *self,PyObject *args); // deprecated
    static PyObject* sHideObject               (PyObject *self,PyObject *args); // hide view provider object
    static PyObject* sShowObject               (PyObject *self,PyObject *args); // show view provider object

    static PyObject* sOpen                     (PyObject *self,PyObject *args); // open Python scripts
    static PyObject* sInsert                   (PyObject *self,PyObject *args); // open Python scripts
    static PyObject* sExport                   (PyObject *self,PyObject *args);
    static PyObject* sReload                   (PyObject *self,PyObject *args); // reload FCStd file
    static PyObject* sLoadFile                 (PyObject *self,PyObject *args); // open all types of files

    static PyObject* sCoinRemoveAllChildren    (PyObject *self,PyObject *args);

    static PyObject* sActiveDocument           (PyObject *self,PyObject *args);
    static PyObject* sSetActiveDocument        (PyObject *self,PyObject *args);
    static PyObject* sActiveView               (PyObject *self,PyObject *args);
    static PyObject* sActivateView             (PyObject *self,PyObject *args);
    static PyObject* sGetDocument              (PyObject *self,PyObject *args);
    static PyObject* sEditDocument             (PyObject *self,PyObject *args);

    static PyObject* sDoCommand                (PyObject *self,PyObject *args);
    static PyObject* sDoCommandGui             (PyObject *self,PyObject *args);
    static PyObject* sAddModule                (PyObject *self,PyObject *args);

    static PyObject* sShowDownloads            (PyObject *self,PyObject *args);
    static PyObject* sShowPreferences          (PyObject *self,PyObject *args);

    static PyObject* sCreateViewer             (PyObject *self,PyObject *args);
    static PyObject* sGetMarkerIndex           (PyObject *self,PyObject *args);

    static PyObject* sAddDocObserver           (PyObject *self,PyObject *args);
    static PyObject* sRemoveDocObserver        (PyObject *self,PyObject *args);

    static PyObject* sListUserEditModes        (PyObject *self,PyObject *args);
    static PyObject* sGetUserEditMode          (PyObject *self,PyObject *args);
    static PyObject* sSetUserEditMode          (PyObject *self,PyObject *args);

    static PyMethodDef    Methods[];

private:
    struct ApplicationP* d;
    /// workbench python dictionary
    PyObject*             _pcWorkbenchDictionary;
};

} //namespace Gui

#endif
