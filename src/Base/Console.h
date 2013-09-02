/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2002                        *   
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        * 
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        * 
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/
 



#ifndef BASE_CONSOLE_H
#define BASE_CONSOLE_H

// Std. configurations
#include <Base/PyExport.h>
#include <Base/Stream.h>

//#pragma warning(disable: 4786)  // specifier longer then 255 chars
#include <assert.h>
#include <set>
#include <string>

//**************************************************************************
// Loging levels

#ifdef FC_DEBUG
/// switch on the loging of python object creation and destruction
#  undef FC_LOGPYOBJECTS 
/// switch on the loging of Feature update and execution
#  define FC_LOGFEATUREUPDATE 
/// switch on the loging of the Update execution through Doc, App, GuiApp and GuiDoc
#  undef FC_LOGUPDATECHAIN 
#endif

 
namespace Base {
class ConsoleSingleton;
}; // namespace Base

typedef Base::ConsoleSingleton ConsoleMsgType;
typedef unsigned int ConsoleMsgFlags;

namespace Base {

/** The console observer class
 *  This class distribute the Messages issued to the FCConsole class. 
 *  If you need to catch some of the Messages you need to inherit from
 *  this class and implement some of the methods.
 *  @see Console
  */
class BaseExport ConsoleObserver
{
public:
    ConsoleObserver()
        :bErr(true),bMsg(true),bLog(true),bWrn(true) {}
    virtual ~ConsoleObserver() {}
    /// get calles when a Warning is issued
    virtual void Warning(const char *){};
    /// get calles when a Message is issued
    virtual void Message(const char *){};
    /// get calles when a Error is issued
    virtual void Error  (const char *)=0;
    /// get calles when a Log Message is issued
    virtual void Log    (const char *){};

    virtual const char *Name(void){return 0L;}
    bool bErr,bMsg,bLog,bWrn;
};


/** The console class
 *  This class manage all the stdio stuff. This includes 
 *  Messages, Warnings, Log entries and Errors. The incomming
 *  Messages are distributed with the FCConsoleObserver. The
 *  FCConsole class itself makes no IO, it's more like a manager.
 *  \par
 *  ConsoleSingleton is a singleton! That means you can access the only
 *  instance of the class from every where in c++ by simply using:
 *  \code
 *  #include <Base/Console.h>
 *  Base::Console().Log("Stage: %d",i);
 *  \endcode
 *  \par
 *  ConsoleSingleton is abel to switch between several modes to, e.g. switch
 *  the logging on or off, or treat Warnings as Errors, and so on...
 *  @see ConsoleObserver
 */
class BaseExport ConsoleSingleton
{

public:

    // exported functions goes here +++++++++++++++++++++++++++++++++++++++
    /// Prints a Message 
    virtual void Message ( const char * pMsg, ... ) ;
    /// Prints a warning Message 
    virtual void Warning ( const char * pMsg, ... ) ;
    /// Prints a error Message 
    virtual void Error   ( const char * pMsg, ... ) ;
    /// Prints a log Message 
    virtual void Log     ( const char * pMsg, ... ) ;

    /// Delivers a time/date string 
    const char* Time(void);
    /// Attaches an Observer to FCConsole
    void AttachObserver(ConsoleObserver *pcObserver);
    /// Detaches an Observer from FCConsole
    void DetachObserver(ConsoleObserver *pcObserver);
    /// enumaration for the console modes
    enum ConsoleMode{
        Verbose = 1,	// supress Log messages
    };

    enum FreeCAD_ConsoleMsgType { 
        MsgType_Txt = 1, 
        MsgType_Log = 2, 
        MsgType_Wrn = 4, 
        MsgType_Err = 8 
    } ;

    /// Change mode
    void SetMode(ConsoleMode m);
    /// Change mode
    void UnsetMode(ConsoleMode m);
    /// Enables or disables message types of a cetain console observer
    ConsoleMsgFlags SetEnabledMsgType(const char* sObs, ConsoleMsgFlags type, bool b);
    /// Enables or disables message types of a cetain console observer
    bool IsMsgTypeEnabled(const char* sObs, FreeCAD_ConsoleMsgType type) const;

    /// singleton 
    static ConsoleSingleton &Instance(void);

    // retrieval of an observer by name
    ConsoleObserver *Get(const char *Name) const;

    static PyMethodDef    Methods[]; 

protected:
    // python exports goes here +++++++++++++++++++++++++++++++++++++++++++	
    // static python wrapper of the exported functions
    static PyObject *sPyLog      (PyObject *self,PyObject *args,PyObject *kwd);
    static PyObject *sPyMessage  (PyObject *self,PyObject *args,PyObject *kwd);
    static PyObject *sPyWarning  (PyObject *self,PyObject *args,PyObject *kwd);
    static PyObject *sPyError    (PyObject *self,PyObject *args,PyObject *kwd);
    static PyObject *sPySetStatus(PyObject *self,PyObject *args,PyObject *kwd);
    static PyObject *sPyGetStatus(PyObject *self,PyObject *args,PyObject *kwd);

    bool _bVerbose;

    // Singleton!
    ConsoleSingleton(void);
    virtual ~ConsoleSingleton();

private:
    // singleton
    static void Destruct(void);
    static ConsoleSingleton *_pcSingleton;

    // observer processing 
    void NotifyMessage(const char *sMsg);
    void NotifyWarning(const char *sMsg);
    void NotifyError  (const char *sMsg);
    void NotifyLog    (const char *sMsg);

    // observer list
    std::set<ConsoleObserver * > _aclObservers;
};

/** Access to the Console
 *  This method is used to gain access to the one and only instance of 
 *  the ConsoleSingleton class.
 */  
inline ConsoleSingleton &Console(void){
    return ConsoleSingleton::Instance();
}


//=========================================================================
// some special observers

/** The LoggingConsoleObserver class
 *  This class is used by the main modules to write Console messages and logs to a file
 */  
class BaseExport ConsoleObserverFile : public ConsoleObserver
{
public:
    ConsoleObserverFile(const char *sFileName);
    virtual ~ConsoleObserverFile();
    virtual void Warning(const char *sWarn);
    virtual void Message(const char *sMsg);
    virtual void Error  (const char *sErr);
    virtual void Log    (const char *sLog);
    const char* Name(void){return "File";}

protected:
    Base::ofstream cFileStream;
};

/** The CmdConsoleObserver class
 *  This class is used by the main modules to write Console messages and logs the system con.
 */
class BaseExport ConsoleObserverStd: public ConsoleObserver
{
public:
    ConsoleObserverStd();
    virtual ~ConsoleObserverStd();
    virtual void Warning(const char *sWarn);
    virtual void Message(const char *sMsg); 
    virtual void Error  (const char *sErr); 
    virtual void Log    (const char *sErr); 
    const char* Name(void){return "Console";}
};

class BaseExport RedirectStdOutput : public std::streambuf
{
public:
    RedirectStdOutput();

protected:
    int overflow(int c = EOF);
    int sync();

private:
    std::string buffer;
};

class BaseExport RedirectStdError : public std::streambuf
{
public:
    RedirectStdError();

protected:
    int overflow(int c = EOF);
    int sync();

private:
    std::string buffer;
};

class BaseExport RedirectStdLog : public std::streambuf
{
public:
    RedirectStdLog();

protected:
    int overflow(int c = EOF);
    int sync();

private:
    std::string buffer;
};


} // namespace Base 

#endif // BASE_CONSOLE_H
