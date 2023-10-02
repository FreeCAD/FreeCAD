/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <boost_signals2.hpp>

#include <Base/Type.h>
#include <Gui/Application.h>

/** @defgroup CommandMacros Helper macros for running commands through Python interpreter */
//@{

/** Runs a command for accessing document attribute or method
 * @param _type: type of document, Gui or App
 * @param _doc: pointer to a document
 * @param _cmd: command string, streamable
 *
 * Example:
 * @code{.cpp}
 *      _FCMD_DOC_CMD(Gui,doc,"getObject('" << objName << "')");
 * @endcode
 *
 * Translates to command (assuming doc's name is 'DocName', and
 * and objName contains value 'ObjName'):
 * @code{.py}
 *       Gui.getDocument('DocName').getObject('ObjName')
 * @endcode
 */
#define _FCMD_DOC_CMD(_type,_doc,_cmd) do{\
    auto __doc = _doc;\
    if(__doc && __doc->getName()) {\
        std::ostringstream _str;\
        _str << #_type ".getDocument('" << __doc->getName() << "')." << _cmd;\
        Gui::Command::runCommand(Gui::Command::Doc,_str.str().c_str());\
    }\
}while(0)

/** Runs a command for accessing App.Document attribute or method
 *
 * @param _doc: pointer to a document
 * @param _cmd: command string, streamable
 * @sa _FCMD_DOC_CMD()
 */
#define FCMD_DOC_CMD(_doc,_cmd) _FCMD_DOC_CMD(App,_doc,_cmd)

/** Runs a command for accessing an object's document attribute or method
 * @param _type: type of the document, Gui or App
 * @param _obj: pointer to a DocumentObject
 * @param _cmd: command string, streamable
 */
#define _FCMD_OBJ_DOC_CMD(_type,_obj,_cmd) do{\
    auto __obj = _obj;\
    if(__obj)\
        _FCMD_DOC_CMD(_type,__obj->getDocument(),_cmd);\
}while(0)

/** Runs a command for accessing an object's App::Document attribute or method
 * @param _obj: pointer to a DocumentObject
 * @param _cmd: command string, streamable
 */
#define FCMD_OBJ_DOC_CMD(_obj,_cmd) _FCMD_OBJ_DOC_CMD(App,_obj,_cmd)

/** Runs a command for accessing an object's Gui::Document attribute or method
 * @param _obj: pointer to a DocumentObject
 * @param _cmd: command string, streamable
 */
#define FCMD_VOBJ_DOC_CMD(_obj,_cmd) _FCMD_OBJ_DOC_CMD(Gui,_obj,_cmd)

/** Runs a command for accessing a document/view object's attribute or method
 * @param _type: type of the object, Gui or App
 * @param _obj: pointer to a DocumentObject
 * @param _cmd: command string, streamable
 *
 * Example:
 * @code{.cpp}
 *      _FCMD_OBJ_CMD(Gui,obj,"Visibility = " << (visible?"True":"False"));
 * @endcode
 *
 * Translates to command (assuming obj's document name is 'DocName', obj's name
 * is 'ObjName', and visible is true):
 * @code{.py}
 *       Gui.getDocument('DocName').getObject('ObjName').Visibility = True
 * @endcode
 */
#define _FCMD_OBJ_CMD(_type,_cmd_type,_obj,_cmd) do{\
    auto __obj = _obj;\
    if(__obj && __obj->getNameInDocument()) {\
        std::ostringstream _str;\
        _str << #_type ".getDocument('" << __obj->getDocument()->getName() \
             << "').getObject('" <<  __obj->getNameInDocument() << "')." << _cmd;\
        Gui::Command::runCommand(Gui::Command::_cmd_type,_str.str().c_str());\
    }\
}while(0)

/** Runs a command for accessing an document object's attribute or method
 * @param _obj: pointer to a DocumentObject
 * @param _cmd: command string, streamable
 * @sa _FCMD_OBJ_CMD()
 */
#define FCMD_OBJ_CMD(_obj,_cmd) _FCMD_OBJ_CMD(App,Doc,_obj,_cmd)

/** Runs a command for accessing an view object's attribute or method
 * @param _obj: pointer to a DocumentObject
 * @param _cmd: command string, streamable
 * @sa _FCMD_OBJ_CMD()
 */
#define FCMD_VOBJ_CMD(_obj,_cmd) _FCMD_OBJ_CMD(Gui,Gui,_obj,_cmd)

/** Runs a command for accessing a document object's attribute or method
 * @param _cmd: command string, supporting printf like formatter
 * @param _obj: pointer to a DocumentObject
 *
 * Example:
 * @code{.cpp}
 *      FCMD_OBJ_CMD2("Visibility = %s", obj, visible?"True":"False");
 * @endcode
 *
 * Translates to command (assuming obj's document name is 'DocName', obj's name
 * is 'ObjName', and visible is true):
 * @code{.py}
 *       App.getDocument('DocName').getObject('ObjName').Visibility = True
 * @endcode
 */
#define FCMD_OBJ_CMD2(_cmd,_obj,...) do{\
    auto __obj = _obj;\
    if(__obj && __obj->getNameInDocument()) {\
        Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument('%s').getObject('%s')." _cmd,\
                __obj->getDocument()->getName(),__obj->getNameInDocument(),## __VA_ARGS__);\
    }\
}while(0)

/** Runs a command for accessing a view object's attribute or method
 * @param _cmd: command string, supporting printf like formatter
 * @param _obj: pointer to a DocumentObject
 * @sa FCMD_OBJ_CMD2()
 */
#define FCMD_VOBJ_CMD2(_cmd,_obj,...) do{\
    auto __obj = _obj;\
    if(__obj && __obj->getNameInDocument()) {\
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.getDocument('%s').getObject('%s')." _cmd,\
                __obj->getDocument()->getName(),__obj->getNameInDocument(),## __VA_ARGS__);\
    }\
}while(0)

/** Runs a command to start editing a give object
 * @param _obj: pointer to a DocumentObject
 *
 * Unlike other FCMD macros, this macro editing the object using the current
 * active document, instead of the object's owner document. This allows
 * in-place editing an object, which may be brought in through linking to an
 * external group.
 */
#define FCMD_SET_EDIT(_obj) do{\
    auto __obj = _obj;\
    if(__obj && __obj->getNameInDocument()) {\
        Gui::Command::doCommand(Gui::Command::Gui,\
            "Gui.ActiveDocument.setEdit(App.getDocument('%s').getObject('%s'), %i)",\
            __obj->getDocument()->getName(), __obj->getNameInDocument(), Gui::Application::Instance->getUserEditMode());\
    }\
}while(0)


/// Hides an object
#define FCMD_OBJ_HIDE(_obj) FCMD_OBJ_CMD(_obj,"Visibility = False")

/// Shows an object
#define FCMD_OBJ_SHOW(_obj) FCMD_OBJ_CMD(_obj,"Visibility = True")

//@}

class QWidget;
class QByteArray;

using PyObject = struct _object;

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


void CreateStdCommands();
void CreateDocCommands();
void CreateFeatCommands();
void CreateMacroCommands();
void CreateViewStdCommands();
void CreateWindowStdCommands();
void CreateStructureCommands();
void CreateTestCommands();
void CreateLinkCommands();


/** The CommandBase class
 * This lightweight class is the base class of all commands in FreeCAD. It represents the link between the FreeCAD
 * command framework and the QAction world of Qt.
 * @author Werner Mayer
 */
class GuiExport CommandBase
{
protected:
    explicit CommandBase(const char* sMenu, const char* sToolTip=nullptr, const char* sWhat=nullptr,
                const char* sStatus=nullptr, const char* sPixmap=nullptr, const char* sAccel=nullptr);
    virtual ~CommandBase();

public:
    /**
     * Returns the Action object of this command, or 0 if it doesn't exist.
     */
    Action*  getAction() const;

    /** @name Methods to override when creating a new command */
    //@{
protected:
    /// Creates the used Action when adding to a widget. The default implementation does nothing.
    virtual Action * createAction();

public:
    /// Reassigns QAction stuff after the language has changed.
    virtual void languageChange() = 0;
    /// Updates the QAction with respect to the passed mode.
    virtual void updateAction(int mode) = 0;
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
     *  They set up the most important properties of the command.
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
    std::string displayText;
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
 * @author Jürgen Riegel
 */
class GuiExport Command : public CommandBase
{
protected:
    explicit Command(const char* name);
    ~Command() override;

protected:
    /** @name Methods to override when creating a new command
     */
    //@{
    /// Methods which gets called when activated, needs to be reimplemented!
    virtual void activated(int iMsg)=0;
    /// Creates the used Action
    Action * createAction() override;
    /// Applies the menu text, tool and status tip to the passed action object
    void applyCommandData(const char* context, Action* );
    const char* keySequenceToAccel(int) const;
    void printConflictingAccelerators() const;
    //@}

public:
    /** @name interface used by the CommandManager and the Action */
    //@{
    /// CommandManager is a friend
    friend class CommandManager;
    /// Override this method if your Cmd is not always active
    virtual bool isActive(){return true;}
    /// Get somtile called to check the state of the command
    void testActive();
    /// Enables or disables the command
    void setEnabled(bool);
    /// Command trigger source
    enum TriggerSource {
        /// No external trigger, e.g. invoked through Python
        TriggerNone,
        /// Command triggered by an action
        TriggerAction,
        /// Command triggered by a child action inside an action group
        TriggerChildAction,
    };
    /// Return the current command trigger source
    TriggerSource triggerSource() const {return _trigger;}
    /** Called to invoke the command
     *
     * @param index: in case of group command, this is the index of the child
     *               command.  For checkable command, this indicates the
     *               checkable state.
     * @param trigger: indicate the command triggering source, see TriggerSource.
     */
    void invoke (int index, TriggerSource trigger=TriggerNone);
    /// adds this command to arbitrary widgets
    void addTo(QWidget *);
    void addToGroup(ActionGroup *, bool checkable);
    void addToGroup(ActionGroup *);
    /// Create the action if not exist
    void initAction();
    //@}


    /** @name Helper methods to get important classes */
    //@{
    /// Get pointer to the Application Window
    static Application*  getGuiApplication();
    /// Get a reference to the selection
    static Gui::SelectionSingleton&  getSelection();
    /// Get pointer to the active gui document
    Gui::Document*  getActiveGuiDocument() const;
    /** Get pointer to the named or active App document
     *  Returns a pointer to the named document or the active
     *  document when no name is given. NULL is returned
     *  when the name does not exist or no document is active!
     */
    App::Document*  getDocument(const char* Name=nullptr) const;
    /// checks if the active view is of a special type or derived
    bool isViewOfType(Base::Type t) const;
    /// returns the named feature or the active one from the active document or NULL
    App::DocumentObject*  getObject(const char* Name) const;
    /// returns a python command string to retrieve an object from a document
    static std::string getObjectCmd(const char *Name, const App::Document *doc=nullptr,
            const char *prefix=nullptr, const char *postfix=nullptr, bool gui=false);
    /// returns a python command string to retrieve the given object
    static std::string getObjectCmd(const App::DocumentObject *obj,
            const char *prefix=nullptr, const char *postfix=nullptr, bool gui=false);
    /** Get unique Feature name from the active document
     *
     *  @param BaseName: the base name
     *  @param obj: if not zero, then request the unique name in the document of
     *  the given object.
     */
    std::string getUniqueObjectName(const char *BaseName, const App::DocumentObject *obj=nullptr) const;
    //@}

    /** @name Helper methods for the Undo/Redo and Update handling */
    //@{
    /// Open a new Undo transaction on the active document
    static void openCommand(const char* sName=nullptr);
    /// Commit the Undo transaction on the active document
    static void commitCommand();
    /// Abort the Undo transaction on the active document
    static void abortCommand();
    /// Check if an Undo transaction is open on the active document
    static bool hasPendingCommand();
    /// Updates the (active) document (propagate changes)
    static void updateActive();
    /// Updates the (all or listed) documents (propagate changes)
    static void updateAll(std::list<Gui::Document*> cList);
    /// Checks if the active object of the active document is valid
    static bool isActiveObjectValid();
    /// Translate command
    void languageChange() override;
    /// Updates the QAction with respect to the passed mode.
    void updateAction(int mode) override;
    /// Setup checkable actions based on current TriggerSource
    void setupCheckable(int iMsg);
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
    /// Print to Python console the current Python calling source file and line number
    static void printPyCaller();
    /// Print to Python console the current calling source file and line number
    static void printCaller(const char *file, int line);

    // ISO C++11 requires at least one argument for the "..." in a variadic macro
    // https://en.wikipedia.org/wiki/Variadic_macro#Example
    /** Convenience macro to run a command with printf like formatter
     *
     * @sa Command::_doCommand()
     */
#ifdef _MSC_VER
#define doCommand(_type,...) _doCommand(__FILE__,__LINE__,_type,##__VA_ARGS__)
#else
#define doCommand(...) _doCommand(__FILE__,__LINE__,__VA_ARGS__)
#endif

    /** Run a command with printf like formatter
     *
     * @param file: the calling file path (for debugging purpose)
     * @param line: the calling line number (for debugging purpose)
     * @param eType: command type, See DoCmd_Type
     * @param sCmd: command string that supports printf like formatter
     *
     * You can use the convenience macro doCommand() to automate \c file and \c
     * line arguments. You may also want to use various helper @ref CommandMacros.
     */
    static void _doCommand(const char *file, int line, DoCmd_Type eType,const char* sCmd,...);

    /** Convenience macro to run a command
     *
     * @sa Command::_runCommand()
     */
#define runCommand(_type,_cmd) _runCommand(__FILE__,__LINE__,_type,_cmd)

    /** Run a command
     *
     * @param file: the calling file path (for debugging purpose)
     * @param line: the calling line number (for debugging purpose)
     * @param eType: command type, See DoCmd_Type
     * @param sCmd: command string
     *
     * @sa _doCommand()
     */
    static void _runCommand(const char *file, int line, DoCmd_Type eType,const char* sCmd);

    /** Run a command
     *
     * @param file: the calling file path (for debugging purpose)
     * @param line: the calling line number (for debugging purpose)
     * @param eType: command type, See DoCmd_Type
     * @param sCmd: command string
     *
     * @sa _doCommand()
     */
    static void _runCommand(const char *file, int line, DoCmd_Type eType,const QByteArray& sCmd);

    /// import an external (or own) module only once
    static void addModule(DoCmd_Type eType,const char* sModuleName);

    /** Convenience macro to assure the switch to a certain workbench
     *
     * @sa _assureWorkbench()
     */
#define assureWorkbench(_name) _assureWorkbench(__FILE__,__LINE__,_name)

    /** Assures the switch to a certain workbench
     *
     * @param file: the calling file path (for debugging purpose)
     * @param line: the calling line number (for debugging purpose)
     * @param sName: workbench name
     *
     * @return Return the current active workbench name before switching.
     *
     * If already in the workbench, does nothing.
     */
    static std::string _assureWorkbench(const char *file, int line, const char * sName);
    //@}

    /** @name Methods for copying visiual properties */
    //@{
    /// Convenience macro to copy visual properties
#define copyVisual(...) _copyVisual(__FILE__,__LINE__,__VA_ARGS__)
    static void _copyVisual(const char *file, int line, const char* to, const char* attr, const char* from);
    static void _copyVisual(const char *file, int line, const char* to, const char* attr_to, const char* from, const char* attr_from);
    static void _copyVisual(const char *file, int line, const App::DocumentObject *to, const char *attr, const App::DocumentObject *from);
    static void _copyVisual(const char *file, int line, const App::DocumentObject *to, const char *attr_to, const App::DocumentObject *from, const char *attr_from);
    //@}

    /// Get Python tuple from object and sub-elements
    static std::string getPythonTuple(const std::string& name, const std::vector<std::string>& subnames);
    /// translate a string to a python string literal (needed e.g. in file names for windows...)
    const std::string strToPython(const char* Str);
    const std::string strToPython(const std::string &Str){
        return strToPython(Str.c_str());
    }

    /** @name Helper methods to generate help pages */
    //@{
    /// returns the begin of a online help page
    const char * beginCmdHelp();
    /// returns the end of a online help page
    const char * endCmdHelp();
    /// Get the help URL
    virtual const char* getHelpUrl() const { return sHelpUrl; }
    //@}

    /** @name Helper methods for the Active tests */
    //@{
    /// true when there is a document
    bool hasActiveDocument() const;
    /// true when there is a document and a Feature with Name
    bool hasObject(const char* Name);
    //@}

    /** @name checking of internal state */
    //@{
    /// returns the name to which the command belongs
    const char* getAppModuleName() const {return sAppModule;}
    void setAppModuleName(const char*);
    /// Get the command name
    const char* getName() const { return sName; }
    /// Get the name of the grouping of the command
    const char* getGroupName() const { return sGroup; }
    void setGroupName(const char*);
    QString translatedGroupName() const;
    //@}


    /// Override shortcut of this command
    virtual void setShortcut (const QString &);
    /// Obtain the current shortcut of this command
    virtual QString getShortcut() const;

    /** @name arbitrary helper methods */
    //@{
    void adjustCameraPosition();
    //@}

    /// Helper class to disable python console log
    class LogDisabler {
    public:
        LogDisabler() {
            ++Command::_busy;
        }
        ~LogDisabler() {
            --Command::_busy;
        }
    };
    friend class LogDisabler;

private:
    void _invoke(int, bool disablelog);

protected:
    enum CmdType {
        AlterDoc       = 1,  /**< Command change the Document */
        Alter3DView    = 2,  /**< Command change the Gui */
        AlterSelection = 4,  /**< Command change the Selection */
        ForEdit        = 8,  /**< Command is in a special edit mode active */
        NoTransaction  = 16, /**< Do not setup auto transaction */
    };

    /** @name Attributes
     *  Set by the inherited constructor to set up the most important properties
     *  of the command. In the Command constructor are set default values!
     *  The real values should be set in the constructor of the inheriting class.
     */
    //@{
    const char* sAppModule;
    const char* sGroup;
    const char* sName;
    const char* sHelpUrl;
    int         eType;
    /// Indicate if the command shall log to MacroManager
    bool        bCanLog;
    //@}
private:
    static int _busy;
    bool bEnabled;
    static bool _blockCmd;
    /// For storing the current command trigger source
    TriggerSource _trigger = TriggerNone;
};

/** Class to help implement a group command
 *
 * To use this class, simply add children command in the constructor of your
 * derived class by calling addCommand();
 */
class GuiExport GroupCommand : public Command {
public:
    /// Constructor
    explicit GroupCommand(const char *name);

    /** Add child command
     * @param cmd: child command. Pass null pointer to add a separator.
     * @param reg: whether to register the command with CommandManager
     * @return Return the command index.
     */
    int addCommand(Command *cmd = nullptr, bool reg=true);
    /** Add child command
     * @param cmd: child command name.
     * @return Return the found command, or NULL if not found.
     */
    Command *addCommand(const char *cmdName);

    Command *getCommand(int idx) const;
protected:
    bool isCheckable() const;
    void setCheckable(bool);
    bool isExclusive() const;
    void setExclusive(bool);
    bool hasDropDownMenu() const;
    void setDropDownMenu(bool);
    void activated(int iMsg) override;
    Gui::Action * createAction() override;
    void languageChange() override;

    void setup(Action *);

protected:
    bool checkable = true;
    bool exclusive = false;
    bool dropDownMenu = true;
    std::vector<std::pair<Command*,size_t> > cmds;
};

/** The Python command class
 * This is a special type of command class. It's used to bind a Python command class into the
 * FreeCAD command framework.
 * An object of this class gets a reference to the Python command object and manages all the
 * passing between the C++ and the Python world. This includes everything like setting resources such as
 * bitmaps, activation or bindings to the user interface.
 * @see CommandManager
 * @author Jürgen Riegel
 */
class PythonCommand: public Command
{
public:
    PythonCommand(const char* name, PyObject * pcPyCommand, const char* pActivationString);
    ~PythonCommand() override;

protected:
    /** @name Methods reimplemented for Command Framework */
    //@{
    /// Method which gets called when activated
    void activated(int iMsg) override;
    /// if the command is not always active
    bool isActive() override;
    /// Get the help URL
    const char* getHelpUrl() const override;
    //@}

public:
    /** @name Methods to get the properties of the command */
    //@{
    /// Reassigns QAction stuff after the language has changed.
    void languageChange() override;
    const char* className() const override
    { return "PythonCommand"; }
    const char* getWhatsThis  () const override;
    const char* getMenuText   () const override;
    const char* getToolTipText() const override;
    const char* getStatusTip  () const override;
    const char* getPixmap     () const override;
    const char* getAccel      () const override;
    bool isCheckable          () const;
    bool isChecked            () const;
    //@}

protected:
    /// Returns the resource values
    const char* getResource(const char* sName) const;
    /// Creates the used Action
    Action * createAction() override;
    /// a pointer to the Python command object
    PyObject * _pcPyCommand;
    /// the command object resource dictionary
    PyObject * _pcPyResourceDict;
    /// the activation sequence
    std::string Activation;
    //// set the parameters on action creation
    void onActionInit() const;

    boost::signals2::connection connPyCmdInitialized;
};

/** The Python group command class
 * @see CommandManager
 * @author Werner Mayer
 */
class PythonGroupCommand: public Command
{
public:
    PythonGroupCommand(const char* name, PyObject * pcPyCommand);
    ~PythonGroupCommand() override;

protected:
    /** @name Methods reimplemented for Command Framework */
    //@{
    /// Method which gets called when activated
    void activated(int iMsg) override;
    /// if the command is not always active
    bool isActive() override;
    /// Get the help URL
    const char* getHelpUrl() const override;
    /// Creates the used Action
    Action * createAction() override;
    //@}

public:
    /** @name Methods to get the properties of the command */
    //@{
    /// Reassigns QAction stuff after the language has changed.
    void languageChange() override;
    const char* className() const override
    { return "PythonGroupCommand"; }
    const char* getWhatsThis  () const override;
    const char* getMenuText   () const override;
    const char* getToolTipText() const override;
    const char* getStatusTip  () const override;
    const char* getPixmap     () const override;
    const char* getAccel      () const override;
    bool isExclusive          () const;
    bool hasDropDownMenu      () const;
    //@}

protected:
    /// Returns the resource values
    const char* getResource(const char* sName) const;
    //// set the parameters on action creation
    void onActionInit() const;
    /// a pointer to the Python command object
    PyObject * _pcPyCommand;
    /// the command object resources
    PyObject * _pcPyResource;

    boost::signals2::connection connPyCmdInitialized;
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
    explicit MacroCommand(const char* name, bool system = false);
    ~MacroCommand() override;

protected:
    /** @name methods reimplemented for Command Framework */
    //@{
    /// Method which get called when activated
    void activated(int iMsg) override;
    /// Creates the used Action
    Action * createAction() override;
    //@}

public:
    /// Returns the script name
    const char* getScriptName () const { return sScriptName; }
    /// Ignore when language has changed.
    void languageChange() override {}
    const char* className() const override
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
    bool systemMacro;
};

/** The CommandManager class
 *  This class manage all available commands in FreeCAD. All
 *  Commands will registered here, also commands from Application
 *  modules. Also activation / deactivation, Icons Tooltips and so
 *  on are handles here. Further the Building of Toolbars and (Context)
 *  menus (connecting to a QAction) is done here.
 *  @see Command
 *  @author Jürgen Riegel
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
     *  delivers a vector of all commands in the given application module. When no
     *  name is given the standard commands (build in ) are returned.
     *  @see Command
     */
    std::vector <Command*> getModuleCommands(const char *sModName) const;

    /** Returns all commands registered in the manager
     *  delivers a vector of all commands. If you intereted in commands of
     *  of a special app module use GetModuleCommands()
     *  @see Command
     */
    std::vector <Command*> getAllCommands() const;

    /** Returns all commands of a group
     *  delivers a vector of all commands in the given group.
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

    /// method is OBSOLETE use GetModuleCommands() or GetAllCommands()
    const std::map<std::string, Command*>& getCommands() const { return _sCommands; }
    /// get frequently called by the AppWnd to check the commands are active.
    void testActive();

    void addCommandMode(const char* sContext, const char* sName);
    void updateCommands(const char* sContext, int mode);

    /// Return a revision number to check for addition or removal of any command
    int getRevision() const { return _revision; }

    /// Signal on any addition or removal of command
    boost::signals2::signal<void ()> signalChanged;

    /// Signal to Python command on first workbench activation
    boost::signals2::signal<void ()> signalPyCmdInitialized;

    /** 
     * Returns a pointer to a conflicting command, or nullptr if there is no conflict.
     * In the case of multiple conflicts, only the first is returned. 
     * \param accel The accelerator to check
     * \param ignore (optional) A command to ignore matches with
     */
    const Command* checkAcceleratorForConflicts(const char* accel, const Command *ignore = nullptr) const;

    /**
     * Returns the first available command name for a new macro (e.g. starting from 1,
     * examines the existing user preferences for Std_Macro_%1 and returns the lowest
     * available numbered string).
     */
    std::string newMacroName() const;

private:
    /// Destroys all commands in the manager and empties the list.
    void clearCommands();
    std::map<std::string, Command*> _sCommands;
    std::map<std::string, std::list<std::string> > _sCommandModes;

    int _revision = 0;
};

} // namespace Gui


/** The Command Macro Standard
 *  This macro makes it easier to define a new command.
 *  The parameters are the class name.
 *  @author Jürgen Riegel
 */
#define DEF_STD_CMD(X) class X : public Gui::Command \
{\
public:\
    X();\
    virtual ~X(){}\
    virtual const char* className() const\
    { return #X; }\
protected: \
    virtual void activated(int iMsg);\
private:\
    X(const X&) = delete;\
    X(X&&) = delete;\
    X& operator= (const X&) = delete;\
    X& operator= (X&&) = delete;\
};

/** The Command Macro Standard + isActive()
 *  This macro makes it easier to define a new command.
 *  The parameters are the class name
 *  @author Jürgen Riegel
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
private:\
    X(const X&) = delete;\
    X(X&&) = delete;\
    X& operator= (const X&) = delete;\
    X& operator= (X&&) = delete;\
};

/** The Command Macro Standard + createAction()
 *  This macro makes it easier to define a new command.
 *  The parameters are the class name
 *  @author Jürgen Riegel
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
private:\
    X(const X&) = delete;\
    X(X&&) = delete;\
    X& operator= (const X&) = delete;\
    X& operator= (X&&) = delete;\
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
private:\
    X(const X&) = delete;\
    X(X&&) = delete;\
    X& operator= (const X&) = delete;\
    X& operator= (X&&) = delete;\
};

/** The Command Macro Standard + isActive() + updateAction()
 *  This macro makes it easier to define a new command.
 *  The parameters are the class name
 *  @author Werner Mayer
 */
#define DEF_STD_CMD_AU(X) class X : public Gui::Command \
{\
public:\
    X();\
    virtual ~X(){}\
    virtual void updateAction(int mode); \
    virtual const char* className() const\
    { return #X; }\
protected: \
    virtual void activated(int iMsg);\
    virtual bool isActive(void);\
private:\
    X(const X&) = delete;\
    X(X&&) = delete;\
    X& operator= (const X&) = delete;\
    X& operator= (X&&) = delete;\
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
private:\
    X(const X&) = delete;\
    X(X&&) = delete;\
    X& operator= (const X&) = delete;\
    X& operator= (X&&) = delete;\
};

/** The Command Macro Standard + isActive() + createAction()
 *  + languageChange() + updateAction()
 *  This macro makes it easier to define a new command.
 *  The parameters are the class name
 *  @author Werner Mayer
 */
#define DEF_STD_CMD_ACLU(X) class X : public Gui::Command \
{\
public:\
    X();\
    virtual ~X(){}\
    virtual void languageChange(); \
    virtual void updateAction(int mode); \
    virtual const char* className() const\
    { return #X; }\
protected: \
    virtual void activated(int iMsg);\
    virtual bool isActive(void);\
    virtual Gui::Action * createAction(void);\
private:\
    X(const X&) = delete;\
    X(X&&) = delete;\
    X& operator= (const X&) = delete;\
    X& operator= (X&&) = delete;\
};

/** The Command Macro view
 *  This macro makes it easier to define a new command for the 3D View
 *  It activate the command only when a 3DView is active.
 *  The parameters are the class name
 *  @author Jürgen Riegel
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
private:\
    X(const X&) = delete;\
    X(X&&) = delete;\
    X& operator= (const X&) = delete;\
    X& operator= (X&&) = delete;\
};

#endif // GUI_COMMAND_H
