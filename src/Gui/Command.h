/***************************************************************************
 *   Copyright (c) 2002 J�rgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_COMMAND_H
#define GUI_COMMAND_H


#include <list>
#include <map>
#include <string>
#include <vector>

#include <Base/Type.h>

class QWidget;
typedef struct _object PyObject;

namespace App
{
  class Document;
  class DocumentObject;
}

namespace Gui {

class Action;
class Application;
class CommandManager;
class Command;
class ActionGroup;
class Document;
class SelectionSingleton;
class MDIView;


void CreateStdCommands(void);
void CreateDocCommands(void);
void CreateFeatCommands(void);
void CreateMacroCommands(void);
void CreateViewStdCommands(void);
void CreateWindowStdCommands(void);
void CreateTestCommands(void);


/** The CommandBase class
 * This lightweigt class is the base class of all commands in FreeCAD. It represents the link between the FreeCAD
 * command framework and the QAction world of Qt.
 * @author Werner Mayer
 */
class GuiExport CommandBase 
{
public:
    CommandBase(const char* sMenu, const char* sToolTip=0, const char* sWhat=0, 
                const char* sStatus=0, const char* sPixmap=0, const char* sAccel=0);
    virtual ~CommandBase();

    /**
     * Returns the Action object of this command, or 0 if it doesn't exist.
     */
    Action*  getAction() const;

    /** @name Methods to override when creating a new command */
    //@{
protected:
    /// Creates the used Action when adding to a widget. The default implementation does nothing.
    virtual Action * createAction(void);
public:
    /// Reassigns QAction stuff after the language has changed. 
    virtual void languageChange() = 0;
    /// The C++ class name is needed as context for the translation framework
    virtual const char* className() const = 0;
    //@}

    /** @name Methods to get the properties of the command */
    //@{
    virtual const char* getMenuText   () const { return sMenuText;    }
    virtual const char* getToolTipText() const { return sToolTipText; }
    virtual const char* getStatusTip  () const { return sStatusTip;   }
    virtual const char* getWhatsThis  () const { return sWhatsThis;   }
    virtual const char* getPixmap     () const { return sPixmap;      }
    virtual const char* getAccel      () const { return sAccel;       }
    //@}

    /** @name Methods to set the properties of the command */
    //@{
    void setWhatsThis  (const char*);
    void setMenuText   (const char*);
    void setToolTipText(const char*);
    void setStatusTip  (const char*);
    void setPixmap     (const char*);
    void setAccel      (const char*);
    //@}

protected:
    /** @name Attributes set by the inherited constructor.
     *
     *  They set up the most important properties  of the command.
     *  In the constructor are set default values. 
     *  The real values should be set in the constructor of the inheriting class.
     */
    //@{
    const char* sMenuText;
    const char* sToolTipText;
    const char* sWhatsThis;
    const char* sStatusTip;
    const char* sPixmap;
    const char* sAccel;
    //@}
protected:
    Action *_pcAction; /**< The Action item. */
};

/** The Command class.
 *
 * This class is mostly used for commands implemented directly in C++ (see PythonCommand).
 * It contains also a lot of helper methods to make implementing commands for FreeCAD as easy as possible.
 *
 * @note This class is intended to handle the GUI interaction like:
 * - starting a dialog
 * - doing view and window stuff
 * - anything else, especially altering the document must be done on application level. See doCommand() for details.
 *
 * @see CommandManager
 * @author J�rgen Riegel
 */
class GuiExport Command : public CommandBase
{
public:
    Command(const char* name);
    virtual ~Command();

protected:
    /** @name Methods to override when creating a new command
     */
    //@{
    /// Methods which gets called when activated, needs to be reimplemented!
    virtual void activated(int iMsg)=0;
    /// Overite this method if your Cmd is not always active
    virtual bool isActive(void){return true;} 
    /// Creates the used Action
    virtual Action * createAction(void);
    /// Applies the menu text, tool and status tip to the passed action object
    void applyCommandData(Action* );
    const char* keySequenceToAccel(int) const;
    void adjustCameraPosition();
    //@}

public:
    /** @name interface used by the CommandManager and the Action */
    //@{
    /// CommandManager is a friend
    friend class CommandManager;
    /// Get somtile called to check the state of the command
    void testActive(void);
    /// get called by the QAction
    void invoke (int); 
    /// adds this command to arbitrary widgets
    void addTo(QWidget *);
    //@}


    /** @name Helper methods to get important classes */
    //@{
    /// Get pointer to the Application Window
    static Application*  getGuiApplication(void);   
    /// Get a reference to the selection 
    Gui::SelectionSingleton&  getSelection(void);
    /// Get pointer to the active gui document
    Gui::Document*  getActiveGuiDocument(void) const;
    /** Get pointer to the named or active App document
     *  Returns a pointer to the named document or the active
     *  document when no name is given. NULL is returned
     *  when the name does not exist or no document is active!
     */
    App::Document*  getDocument(const char* Name=0) const;
    /// checks if the active view is of a special type or derived
    bool isViewOfType(Base::Type t) const;
    /// returns the named feature or the active one from the active document or NULL
    App::DocumentObject*  getObject(const char* Name) const;
    /// Get unique Feature name from the active document 
    std::string getUniqueObjectName(const char *BaseName) const;
    //@}

    /** @name Helper methods for the Undo/Redo and Update handling */
    //@{
    /// Open a new Undo transaction on the active document
    static void openCommand(const char* sName=0);
    /// Commit the Undo transaction on the active document
    static void commitCommand(void);
    /// Abort the Undo transaction on the active document
    static void abortCommand(void);
    /// Updates the (active) document (propagate changes)
    static void updateActive(void);
    /// Updates the (all or listed) documents (propagate changes)
    static void updateAll(std::list<Gui::Document*> cList);
    /// Checks if the active object of the active document is valid
    static bool isActiveObjectValid(void);
    /// Translate command
    void languageChange();
    //@}

    /** @name Helper methods for issuing commands to the Python interpreter */
    //@{
    /// types of application level actions for DoCommand()
    enum DoCmd_Type {
        /// Action alters the document
        Doc,
        /// Action alters only the application
        App,
        /// Action alters only the Gui
        Gui
    };
    /// Blocks all command objects
    static void blockCommand(bool);
    /// Run a App level Action 
    static void doCommand(DoCmd_Type eType,const char* sCmd,...);
    static void runCommand(DoCmd_Type eType,const char* sCmd);
    static void copyVisual(const char* to, const char* attr, const char* from);
    static void copyVisual(const char* to, const char* attr_to, const char* from, const char* attr_from);
    /// import an external module only once 
    //static void addModule(const char* sModuleName);
    /// translate a string to a python string literal (needed e.g. in file names for windows...)
    const std::string strToPython(const char* Str);
    const std::string strToPython(const std::string &Str){return strToPython(Str.c_str());};
    //@}

    /** @name Helper methods to generate help pages */
    //@{
    /// returns the begin of a online help page
    const char * beginCmdHelp(void);
    /// returns the end of a online help page
    const char * endCmdHelp(void);
    /// Get the help URL
    virtual const char* getHelpUrl(void) const { return sHelpUrl; }
    //@}

    /** @name Helper methods for the Active tests */
    //@{
    /// true when there is a document
    bool hasActiveDocument(void) const;
    /// true when there is a document and a Feature with Name
    bool hasObject(const char* Name);
    //@}

    /** @name checking of internal state */
    //@{
    /// returns the name to which the command belongs
    const char* getAppModuleName(void) const {return sAppModule;}
    /// Get the command name
    const char* getName() const { return sName; }
    /// Get the name of the grouping of the command
    const char* getGroupName() const { return sGroup; }
    //@}

protected:
    enum CmdType {
        AlterDoc       = 1,  /**< Command change the Document */
        Alter3DView    = 2,  /**< Command change the Gui */
        AlterSelection = 4,  /**< Command change the Selection */
        ForEdit        = 8   /**< Command is in a special edit mode active */
    };

    /** @name Attributes 
     *  Set by the inherited constructor to set up the most important properties 
     *  of the command. In the Command constructor are set default values! 
     *  The real values should be set in the constructor of the inhereting class.
     */
    //@{
    const char* sAppModule;
    const char* sGroup;
    const char* sName;
    const char* sHelpUrl;
    int         eType;
    //@}
private:
    static bool _blockCmd;
};

/** The Python command class
 * This is a special type of command class. It's used to bind a Python command class into the 
 * FreeCAD command framework.
 * An object of this class gets a reference to the Python command object and manages all the 
 * passing between the C++ and the Python world. This includes everything like setting resources such as
 * bitmaps, activation or bindings to the user interface.
 * @see CommandManager
 * @author J�rgen Riegel
 */
class PythonCommand: public Command
{
public:
    PythonCommand(const char* name,PyObject * pcPyCommand, const char* pActivationString);
    virtual ~PythonCommand() {}

protected:
    /** @name Methods reimplemented for Command Framework */
    //@{
    /// Method which gets called when activated
    virtual void activated(int iMsg);
    /// if the command is not always active
    virtual bool isActive(void);
    /// Get the help URL
    const char* getHelpUrl(void);
    /// Creates the used Action
    virtual Action * createAction(void);
    //@}

public:
    /** @name Methods to get the properties of the command */
    //@{
    /// Reassigns QAction stuff after the language has changed. 
    void languageChange();
    const char* className() const
    { return "PythonCommand"; }
    const char* getWhatsThis  () const;
    const char* getMenuText   () const;
    const char* getToolTipText() const;
    const char* getStatusTip  () const;
    const char* getPixmap     () const;
    const char* getAccel      () const;
    //@}

protected:
    /// Returns the resource values
    const char* getResource(const char* sName) const;
    /// a pointer to the Python command object
    PyObject * _pcPyCommand;
    /// the command object resource dictionary
    PyObject * _pcPyResourceDict;
    /// the activation sequence
    std::string Activation;
};


/** The script command class
 * This is a special type of command class. Its used to bind a macro or Python script to the 
 * FreeCAD command framework.
 * An object of this class gets a string to the place where the script is in the file system.
 * Unlike the other commands the resources can be set by several methods.
 * @see Command
 * @see CommandManager
 * @author Werner Mayer
 */
class MacroCommand: public Command
{
public:
    MacroCommand(const char* name);
    virtual ~MacroCommand() {}

protected:
    /** @name methods reimplemented for Command Framework */
    //@{
    /// Method which get called when activated
    void activated(int iMsg);
    /// Creates the used Action
    Action * createAction(void);
    //@}

public:
    /// Returns the script name
    const char* getScriptName () const { return sScriptName; }
    /// Ignore when language has changed. 
    void languageChange() {}
    const char* className() const
    { return "Gui::MacroCommand"; }
 
    /** @name Methods to set the properties of the Script Command */
    //@{
    /// Sets the script name
    void setScriptName ( const char* );
    //@}

    /** @name Methods to load and save macro commands. */
    //@{
    /** Loads all macros command from the preferences. */
    static void load();
    /** Saves all macros command to the preferences. */
    static void save();
    //@}

protected:
    const char* sScriptName;
};

/** The CommandManager class
 *  This class manage all available commands in FreeCAD. All 
 *  Commands will registered here, also commands from Application
 *  modules. Also activation / deactivation, Icons Tooltips and so
 *  on are handles here. Further the Building of Toolbars and (Context) 
 *  menus (connecting to a QAction) is done here.
 *  @see Command
 *  @author J�rgen Riegel
 */
class GuiExport CommandManager
{
public:
    /// Construction
    CommandManager();
    /// Destruction
    ~CommandManager();
    /// Insert a new command into the manager
    void addCommand(Command* pCom);
    /// Remove a command from the manager
    void removeCommand(Command* pCom);

    /// Adds the given command to a given widget
    bool addTo(const char* Name, QWidget* pcWidget);

    /** Returns all commands of a special App Module
     *  delivers a vector of all comands in the given application module. When no 
     *  name is given the standard commands (build in ) are returned.
     *  @see Command
     */
    std::vector <Command*> getModuleCommands(const char *sModName) const;

    /** Returns all commands registered in the manager
     *  delivers a vector of all comands. If you intereted in commands of
     *  of a special app module use GetModuleCommands()
     *  @see Command
     */
    std::vector <Command*> getAllCommands(void) const;

    /** Returns all commands of a group
     *  delivers a vector of all comands in the given group.
     */
    std::vector <Command*> getGroupCommands(const char *sGrpName) const;

    /** Returns the command registered in the manager with the name sName
     *  If nothing is found it returns a null pointer
     *  @see Command
     */
    Command* getCommandByName(const char* sName) const;

    /**
     * Runs the command
     */
    void runCommandByName (const char* sName) const;

    /// method is OBSOLET use GetModuleCommands() or GetAllCommands()
    const std::map<std::string, Command*>& getCommands() const { return _sCommands; }
    /// get frequently called by the AppWnd to check the commands are active.
    void testActive(void);
private:
    /// Destroys all commands in the manager and empties the list.
    void clearCommands();
    std::map<std::string,Command*> _sCommands;
};

} // namespace Gui


/** The Command Macro Standard
 *  This macro makes it easier to define a new command.
 *  The parameters are the class name.
 *  @author J�rgen Riegel
 */
#define DEF_STD_CMD(X) class X : public Gui::Command \
{\
public:\
    X();\
    virtual const char* className() const\
    { return #X; }\
protected: \
    virtual void activated(int iMsg);\
};

/** The Command Macro Standard + isActive()
 *  This macro makes it easier to define a new command.
 *  The parameters are the class name
 *  @author J�rgen Riegel
 */
#define DEF_STD_CMD_A(X) class X : public Gui::Command \
{\
public:\
    X();\
    virtual ~X(){}\
    virtual const char* className() const\
    { return #X; }\
protected: \
    virtual void activated(int iMsg);\
    virtual bool isActive(void);\
};

/** The Command Macro Standard + createAction()
 *  This macro makes it easier to define a new command.
 *  The parameters are the class name
 *  @author J�rgen Riegel
 */
#define DEF_STD_CMD_C(X) class X : public Gui::Command \
{\
public:\
    X();\
    virtual ~X(){}\
    virtual const char* className() const\
    { return #X; }\
protected: \
    virtual void activated(int iMsg);\
    virtual Gui::Action * createAction(void);\
};

/** The Command Macro Standard + isActive() + createAction()
 *  This macro makes it easier to define a new command.
 *  The parameters are the class name
 *  @author Werner Mayer
 */
#define DEF_STD_CMD_AC(X) class X : public Gui::Command \
{\
public:\
    X();\
    virtual ~X(){}\
    virtual const char* className() const\
    { return #X; }\
protected: \
    virtual void activated(int iMsg);\
    virtual bool isActive(void);\
    virtual Gui::Action * createAction(void);\
};

/** The Command Macro Standard + isActive() + createAction()
 *  + languageChange()
 *  This macro makes it easier to define a new command.
 *  The parameters are the class name
 *  @author Werner Mayer
 */
#define DEF_STD_CMD_ACL(X) class X : public Gui::Command \
{\
public:\
    X();\
    virtual ~X(){}\
    virtual void languageChange(); \
    virtual const char* className() const\
    { return #X; }\
protected: \
    virtual void activated(int iMsg);\
    virtual bool isActive(void);\
    virtual Gui::Action * createAction(void);\
};

/** The Command Macro view
 *  This macro makes it easier to define a new command for the 3D View
 *  It activate the command only when a 3DView is active.
 *  The parameters are the class name
 *  @author J�rgen Riegel
 */
#define DEF_3DV_CMD(X) class X : public Gui::Command \
{\
public:\
    X();\
    virtual ~X(){}\
    virtual const char* className() const\
    { return #X; }\
protected: \
    virtual void activated(int iMsg);\
    virtual bool isActive(void)\
    {\
        Gui::MDIView* view = Gui::getMainWindow()->activeWindow();\
        return view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId());\
    }\
};

#endif // GUI_COMMAND_H
