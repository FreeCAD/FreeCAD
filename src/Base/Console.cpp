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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <time.h>
# include <stdio.h>
# if defined(FC_OS_WIN32)
#  include <io.h>
#  include <windows.h>
# elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX)
#  include <unistd.h>
# endif
# include "fcntl.h"
#endif

#include "Console.h"
#include "Exception.h"
#include "PyObjectBase.h"
#include <QCoreApplication>

using namespace Base;




//=========================================================================

namespace Base {
    
class ConsoleEvent : public QEvent {
public:
    ConsoleSingleton::FreeCAD_ConsoleMsgType msgtype;
    std::string msg;

    ConsoleEvent(ConsoleSingleton::FreeCAD_ConsoleMsgType type, const std::string& msg)
        : QEvent(QEvent::User), msgtype(type), msg(msg)
    {
    }
    ~ConsoleEvent()
    {
    }
};

class ConsoleOutput : public QObject
{
public:
    static ConsoleOutput* getInstance() {
        if (!instance)
            instance = new ConsoleOutput;
        return instance;
    }
    static void destruct() {
        delete instance;
        instance = 0;
    }

    void customEvent(QEvent* ev) {
        if (ev->type() == QEvent::User) {
            ConsoleEvent* ce = static_cast<ConsoleEvent*>(ev);
            switch (ce->msgtype) {
            case ConsoleSingleton::MsgType_Txt:
                Console().NotifyMessage(ce->msg.c_str());
                break;
            case ConsoleSingleton::MsgType_Log:
                Console().NotifyLog(ce->msg.c_str());
                break;
            case ConsoleSingleton::MsgType_Wrn:
                Console().NotifyWarning(ce->msg.c_str());
                break;
            case ConsoleSingleton::MsgType_Err:
                Console().NotifyError(ce->msg.c_str());
                break;
            }
        }
    }

private:
    ConsoleOutput()
    {
    }
    ~ConsoleOutput()
    {
    }

    static ConsoleOutput* instance;
};

ConsoleOutput* ConsoleOutput::instance = 0;

}

//**************************************************************************
// Construction destruction


ConsoleSingleton::ConsoleSingleton(void)
  : _bVerbose(true)
  , _bCanRefresh(true)
  , connectionMode(Direct)
#ifdef FC_DEBUG
  ,_defaultLogLevel(FC_LOGLEVEL_LOG)
#else
  ,_defaultLogLevel(FC_LOGLEVEL_MSG)
#endif
{
    // make sure this object is part of the main thread
    ConsoleOutput::getInstance();
}

ConsoleSingleton::~ConsoleSingleton()
{
    ConsoleOutput::destruct();
    for(std::set<ConsoleObserver * >::iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter)
        delete (*Iter);
}


//**************************************************************************
// methods

/**  
 *  sets the console in a special mode
 */
void ConsoleSingleton::SetConsoleMode(ConsoleMode m)
{
    if (m & Verbose)
        _bVerbose = true;
}

/**  
 *  unsets the console from a special mode
 */
void ConsoleSingleton::UnsetConsoleMode(ConsoleMode m)
{
    if (m & Verbose)
        _bVerbose = false;
}

/**
 * \a type can be OR'ed with any of the FreeCAD_ConsoleMsgType flags to enable -- if \a b is true --
 * or to disable -- if \a b is false -- a console observer with name \a sObs.
 * The return value is an OR'ed value of all message types that have changed their state. For example
 * @code
 * // switch off warnings and error messages
 * ConsoleMsgFlags ret = Base::Console().SetEnabledMsgType("myObs", 
 *                             ConsoleMsgType::MsgType_Wrn|ConsoleMsgType::MsgType_Err, false);
 * // do something without notifying observer myObs
 * ...
 * // restore the former configuration again
 * Base::Console().SetEnabledMsgType("myObs", ret, true);
 * @endcode
 * switches off warnings and error messages and restore the state before the modification.
 * If the observer \a sObs doesn't exist then nothing happens.
 */
ConsoleMsgFlags ConsoleSingleton::SetEnabledMsgType(const char* sObs, ConsoleMsgFlags type, bool b)
{
    ConsoleObserver* pObs = Get(sObs);
    if ( pObs ){
        ConsoleMsgFlags flags=0;

        if ( type&MsgType_Err ){
            if ( pObs->bErr != b )
                flags |= MsgType_Err;
            pObs->bErr = b;
        }
        if ( type&MsgType_Wrn ){
            if ( pObs->bWrn != b )
                flags |= MsgType_Wrn;
            pObs->bWrn = b;
        }
        if ( type&MsgType_Txt ){
            if ( pObs->bMsg != b )
                flags |= MsgType_Txt;
            pObs->bMsg = b;
        }
        if ( type&MsgType_Log ){
            if ( pObs->bLog != b )
                flags |= MsgType_Log;
            pObs->bLog = b;
        }
        return flags;
    }
    else {
        return 0;
    }
}

bool ConsoleSingleton::IsMsgTypeEnabled(const char* sObs, FreeCAD_ConsoleMsgType type) const
{
    ConsoleObserver* pObs = Get(sObs);
    if (pObs) {
        switch (type) {
        case MsgType_Txt:
            return pObs->bMsg;
        case MsgType_Log:
            return pObs->bLog;
        case MsgType_Wrn:
            return pObs->bWrn;
        case MsgType_Err:
            return pObs->bErr;
        default:
            return false;
        }
    }
    else {
        return false;
    }
}

void ConsoleSingleton::SetConnectionMode(ConnectionMode mode)
{
    connectionMode = mode;
}

/** Prints a Message
 *  This method issues a Message. 
 *  Messages are used to show some non vital information. That means when
 *  FreeCAD is running in GUI mode a Message appears on the status bar. 
 *  In console mode a message is printed to the console. 
 *  \par
 *  You can use a printf like interface like:
 *  \code
 *  Console().Message("Doing something important %d times\n",i);
 *  \endcode
 *  @see Warning
 *  @see Error
 *  @see Log
 */
void ConsoleSingleton::Message( const char *pMsg, ... )
{
    char format[4024];
    const unsigned int format_len = 4024;

    va_list namelessVars;
    va_start(namelessVars, pMsg);  // Get the "..." vars
    vsnprintf(format, format_len, pMsg, namelessVars);
    va_end(namelessVars);

    if (connectionMode == Direct)
        NotifyMessage(format);
    else
        QCoreApplication::postEvent(ConsoleOutput::getInstance(), new ConsoleEvent(MsgType_Txt, format));
}

/** Prints a Message
 *  This method issues a Warning. 
 *  Messages are used to get the users attention. That means when
 *  FreeCAD is in GUI mode a Message Box pops up. In console
 *  mode a colored message is returned to the console! Don't use this carelessly.
 *  For information purposes the 'Log' or 'Message' methods are more appropriate.
 *  \par
 *  You can use a printf like interface like:
 *  \code
 *  Console().Warning("Some defects in %s, loading anyway\n",str);
 *  \endcode
 *  @see Message
 *  @see Error
 *  @see Log
 */
void ConsoleSingleton::Warning( const char *pMsg, ... )
{
    char format[4024];
    const unsigned int format_len = 4024;

    va_list namelessVars;
    va_start(namelessVars, pMsg);  // Get the "..." vars
    vsnprintf(format, format_len, pMsg, namelessVars);
    va_end(namelessVars);

    if (connectionMode == Direct)
        NotifyWarning(format);
    else
        QCoreApplication::postEvent(ConsoleOutput::getInstance(), new ConsoleEvent(MsgType_Wrn, format));
}

/** Prints a Message
 *  This method issues an Error which makes some execution impossible. 
 *  Errors are used to get the users attention. That means when FreeCAD 
 *  is running in GUI mode an Error Message Box pops up. In console
 *  mode a colored message is printed to the console! Don't use this carelessly. 
 *  For information purposes the 'Log' or 'Message' methods are more appropriate.
 *  \par
 *  You can use a printf like interface like:
 *  \code
 *  Console().Error("Something really bad in %s happened\n",str);
 *  \endcode
 *  @see Message
 *  @see Warning
 *  @see Log
 */
void ConsoleSingleton::Error( const char *pMsg, ... )
{
    char format[4024];
    const unsigned int format_len = 4024;

    va_list namelessVars;
    va_start(namelessVars, pMsg);  // Get the "..." vars
    vsnprintf(format, format_len, pMsg, namelessVars);
    va_end(namelessVars);

    if (connectionMode == Direct)
        NotifyError(format);
    else
        QCoreApplication::postEvent(ConsoleOutput::getInstance(), new ConsoleEvent(MsgType_Err, format));
}


/** Prints a Message
 *  This method is appropriate for development and tracking purposes.
 *  It can be used to track execution of algorithms and functions.
 *  The normal user doesn't need to see it, it's more for developers 
 *  and experienced users. So in normal user mode the logging is switched off.
 *  \par
 *  You can use a printf-like interface for example:
 *  \code
 *  Console().Log("Execute part %d in algorithm %s\n",i,str);
 *  \endcode
 *  @see Message
 *  @see Warning
 *  @see Error
 */


void ConsoleSingleton::Log( const char *pMsg, ... )
{
    char format[4024];
    const unsigned int format_len = 4024;

    if (_bVerbose)
    {
        va_list namelessVars;
        va_start(namelessVars, pMsg);  // Get the "..." vars
        vsnprintf(format, format_len, pMsg, namelessVars);
        va_end(namelessVars);

        if (connectionMode == Direct)
            NotifyLog(format);
        else
            QCoreApplication::postEvent(ConsoleOutput::getInstance(), new ConsoleEvent(MsgType_Log, format));
    }
}


/** Delivers the time/date
 *  This method gives you a string with the actual time/date. You can
 *  use that for Log() calls to make timestamps.
 *  @return Const string with the date/time
 */
const char* ConsoleSingleton::Time(void)
{
    struct tm *newtime;
    time_t aclock;
    time( &aclock );                 // Get time in seconds 
    newtime = localtime( &aclock );  // Convert time to struct tm form 
    char* st = asctime( newtime );
    st[24] = 0;
    return st;
}



//**************************************************************************
// Observer stuff

/** Attaches an Observer to Console
 *  Use this method to attach a ConsoleObserver derived class to 
 *  the Console. After the observer is attached all messages will also
 *  be forwarded to it.
 *  @see ConsoleObserver
 */
void ConsoleSingleton::AttachObserver(ConsoleObserver *pcObserver)
{
    // double insert !!
    assert(_aclObservers.find(pcObserver) == _aclObservers.end() );

    _aclObservers.insert(pcObserver);
}

/** Detaches an Observer from Console
 *  Use this method to detach a ConsoleObserver derived class.
 *  After detaching you can destruct the Observer or reinsert it later.
 *  @see ConsoleObserver
 */
void ConsoleSingleton::DetachObserver(ConsoleObserver *pcObserver)
{
    _aclObservers.erase(pcObserver);
}

void ConsoleSingleton::NotifyMessage(const char *sMsg)
{
    for(std::set<ConsoleObserver * >::iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter) {
        if((*Iter)->bMsg)
            (*Iter)->Message(sMsg);   // send string to the listener
    }
}

void ConsoleSingleton::NotifyWarning(const char *sMsg)
{
    for(std::set<ConsoleObserver * >::iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter) {
        if((*Iter)->bWrn)
            (*Iter)->Warning(sMsg);   // send string to the listener
    }
}

void ConsoleSingleton::NotifyError(const char *sMsg)
{
    for(std::set<ConsoleObserver * >::iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter) {
        if((*Iter)->bErr)
            (*Iter)->Error(sMsg);   // send string to the listener
    }
}

void ConsoleSingleton::NotifyLog(const char *sMsg)
{
    for(std::set<ConsoleObserver * >::iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter) {
        if((*Iter)->bLog)
            (*Iter)->Log(sMsg);   // send string to the listener
    }
}

ConsoleObserver *ConsoleSingleton::Get(const char *Name) const
{
    const char* OName;
    for(std::set<ConsoleObserver * >::const_iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter) {
        OName = (*Iter)->Name();   // get the name
        if(OName && strcmp(OName,Name) == 0)
            return *Iter;
    }
    return 0;
}

int *ConsoleSingleton::GetLogLevel(const char *tag, bool create) {
    if(!tag) tag = "";
    if(_logLevels.find(tag) != _logLevels.end())
        return &_logLevels[tag];
    if(!create) return 0;
    int &ret = _logLevels[tag];
    ret = -1;
    return &ret;
}

void ConsoleSingleton::Refresh() {
    if(_bCanRefresh)
        QCoreApplication::sendPostedEvents();
}

void ConsoleSingleton::EnableRefresh(bool enable) {
    _bCanRefresh = enable;
}

//**************************************************************************
// Singleton stuff

ConsoleSingleton * ConsoleSingleton::_pcSingleton = 0;

void ConsoleSingleton::Destruct(void)
{
    // not initialized or double destructed!
    assert(_pcSingleton);
    delete _pcSingleton;
    _pcSingleton=0;
}

ConsoleSingleton & ConsoleSingleton::Instance(void)
{
    // not initialized?
    if(!_pcSingleton)
    {
        _pcSingleton = new ConsoleSingleton();
    }
    return *_pcSingleton;
}

//**************************************************************************
// Python stuff

// ConsoleSingleton Methods						// Methods structure
PyMethodDef ConsoleSingleton::Methods[] = {
    {"PrintMessage",         (PyCFunction) ConsoleSingleton::sPyMessage, METH_VARARGS,
     "PrintMessage(string) -- Print a message to the output"},
    {"PrintLog",             (PyCFunction) ConsoleSingleton::sPyLog, METH_VARARGS,
     "PrintLog(string) -- Print a log message to the output"},
    {"PrintError"  ,         (PyCFunction) ConsoleSingleton::sPyError, METH_VARARGS,
     "PrintError(string) -- Print an error message to the output"},
    {"PrintWarning",         (PyCFunction) ConsoleSingleton::sPyWarning, METH_VARARGS,
     "PrintWarning -- Print a warning to the output"},
    {"SetStatus",            (PyCFunction) ConsoleSingleton::sPySetStatus, METH_VARARGS,
     "Set the status for either Log, Msg, Wrn or Error for an observer"},
    {"GetStatus",            (PyCFunction) ConsoleSingleton::sPyGetStatus, METH_VARARGS,
     "Get the status for either Log, Msg, Wrn or Error for an observer"},
    {NULL, NULL, 0, NULL}		/* Sentinel */
};


PyObject *ConsoleSingleton::sPyMessage(PyObject * /*self*/, PyObject *args)
{
    PyObject *output;
    if (!PyArg_ParseTuple(args, "O", &output))
        return NULL;

#if PY_MAJOR_VERSION >= 3
    const char* string=0;
    PyObject* unicode=0;
    if (PyUnicode_Check(output)) {
        string = PyUnicode_AsUTF8(output);
    }
    else {
        unicode = PyObject_Str(output);
        if (unicode)
            string = PyUnicode_AsUTF8(unicode);
    }
#else
    const char* string=0;
    PyObject* unicode=0;
    if (PyUnicode_Check(output)) {
        unicode = PyUnicode_AsEncodedObject(output, "utf-8", "strict");
        if (unicode)
            string = PyString_AsString(unicode);
    }
    else if (PyString_Check(output)) {
        string = PyString_AsString(output);
    }
    else {
        unicode = PyObject_Str(output);
        if (unicode)
            string = PyString_AsString(unicode);
    }
#endif

    PY_TRY {
        if (string)
            Instance().Message("%s",string);            // process message
    } PY_CATCH;

    Py_XDECREF(unicode);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *ConsoleSingleton::sPyWarning(PyObject * /*self*/, PyObject *args)
{
    PyObject *output;
    if (!PyArg_ParseTuple(args, "O", &output))
        return NULL;

#if PY_MAJOR_VERSION >= 3
    const char* string=0;
    PyObject* unicode=0;
    if (PyUnicode_Check(output)) {
        string = PyUnicode_AsUTF8(output);
    }
    else {
        unicode = PyObject_Str(output);
        if (unicode)
            string = PyUnicode_AsUTF8(unicode);
    }
#else
    const char* string=0;
    PyObject* unicode=0;
    if (PyUnicode_Check(output)) {
        unicode = PyUnicode_AsEncodedObject(output, "utf-8", "strict");
        if (unicode)
            string = PyString_AsString(unicode);
    }
    else if (PyString_Check(output)) {
        string = PyString_AsString(output);
    }
    else {
        unicode = PyObject_Str(output);
        if (unicode)
            string = PyString_AsString(unicode);
    }
#endif

    PY_TRY {
        if (string)
            Instance().Warning("%s",string);            // process message
    } PY_CATCH;

    Py_XDECREF(unicode);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *ConsoleSingleton::sPyError(PyObject * /*self*/, PyObject *args)
{
    PyObject *output;
    if (!PyArg_ParseTuple(args, "O", &output))
        return NULL;

#if PY_MAJOR_VERSION >= 3
    const char* string=0;
    PyObject* unicode=0;
    if (PyUnicode_Check(output)) {
        string = PyUnicode_AsUTF8(output);
    }
    else {
        unicode = PyObject_Str(output);
        if (unicode)
            string = PyUnicode_AsUTF8(unicode);
    }
#else
    const char* string=0;
    PyObject* unicode=0;
    if (PyUnicode_Check(output)) {
        unicode = PyUnicode_AsEncodedObject(output, "utf-8", "strict");
        if (unicode)
            string = PyString_AsString(unicode);
    }
    else if (PyString_Check(output)) {
        string = PyString_AsString(output);
    }
    else {
        unicode = PyObject_Str(output);
        if (unicode)
            string = PyString_AsString(unicode);
    }
#endif

    PY_TRY {
        if (string)
            Instance().Error("%s",string);            // process message
    } PY_CATCH;

    Py_XDECREF(unicode);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *ConsoleSingleton::sPyLog(PyObject * /*self*/, PyObject *args)
{
    PyObject *output;
    if (!PyArg_ParseTuple(args, "O", &output))
        return NULL;

#if PY_MAJOR_VERSION >= 3
    const char* string=0;
    PyObject* unicode=0;
    if (PyUnicode_Check(output)) {
        string = PyUnicode_AsUTF8(output);
    }
    else {
        unicode = PyObject_Str(output);
        if (unicode)
            string = PyUnicode_AsUTF8(unicode);
    }
#else
    const char* string=0;
    PyObject* unicode=0;
    if (PyUnicode_Check(output)) {
        unicode = PyUnicode_AsEncodedObject(output, "utf-8", "strict");
        if (unicode)
            string = PyString_AsString(unicode);
    }
    else if (PyString_Check(output)) {
        string = PyString_AsString(output);
    }
    else {
        unicode = PyObject_Str(output);
        if (unicode)
            string = PyString_AsString(unicode);
    }
#endif

    PY_TRY {
        if (string)
            Instance().Log("%s",string);            // process message
    } PY_CATCH;

    Py_XDECREF(unicode);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *ConsoleSingleton::sPyGetStatus(PyObject * /*self*/, PyObject *args)
{
    char *pstr1;
    char *pstr2;
    if (!PyArg_ParseTuple(args, "ss", &pstr1, &pstr2))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    PY_TRY{
        bool b=false;
        ConsoleObserver *pObs = Instance().Get(pstr1);
        if(!pObs)
        {
            Py_INCREF(Py_None);
            return Py_None;
        }

        if(strcmp(pstr2,"Log") == 0)
            b = pObs->bLog;
        else if(strcmp(pstr2,"Wrn") == 0)
            b = pObs->bWrn;
        else if(strcmp(pstr2,"Msg") == 0)
            b = pObs->bMsg;
        else if(strcmp(pstr2,"Err") == 0)
            b = pObs->bErr;
        
        return Py_BuildValue("i",b?1:0);
    }PY_CATCH;
}

PyObject *ConsoleSingleton::sPySetStatus(PyObject * /*self*/, PyObject *args)
{
    char *pstr1;
    char *pstr2;
    int  Bool;
    if (!PyArg_ParseTuple(args, "ssi", &pstr1, &pstr2,&Bool))   // convert args: Python->C 
        return NULL;                                              // NULL triggers exception 

    PY_TRY{
        ConsoleObserver *pObs = Instance().Get(pstr1);
        if(pObs)
        {
            if(strcmp(pstr2,"Log") == 0)
                pObs->bLog = (Bool==0)?false:true;
            else if(strcmp(pstr2,"Wrn") == 0)
                pObs->bWrn = (Bool==0)?false:true;
            else if(strcmp(pstr2,"Msg") == 0)
                pObs->bMsg = (Bool==0)?false:true;
            else if(strcmp(pstr2,"Err") == 0)
                pObs->bErr = (Bool==0)?false:true;
            else
                Py_Error(Base::BaseExceptionFreeCADError,"Unknown Message Type (use Log, Err, Msg or Wrn)");

            Py_INCREF(Py_None);
            return Py_None;
        } else {
            Py_Error(Base::BaseExceptionFreeCADError,"Unknown Console Type");                     
    }

    } PY_CATCH;
}

//=========================================================================
// some special observers

ConsoleObserverFile::ConsoleObserverFile(const char *sFileName)
  : cFileStream(Base::FileInfo(sFileName)) // can be in UTF8
{
    if (!cFileStream.is_open())
        Console().Warning("Cannot open log file '%s'.\n", sFileName);
    // mark the file as a UTF-8 encoded file
    unsigned char bom[3] = {0xef, 0xbb, 0xbf};
    cFileStream.write((const char*)bom,3*sizeof(char));
}

ConsoleObserverFile::~ConsoleObserverFile()
{
    cFileStream.close();
}

void ConsoleObserverFile::Warning(const char *sWarn)
{
    cFileStream << "Wrn: " << sWarn;
    cFileStream.flush();
}

void ConsoleObserverFile::Message(const char *sMsg)
{
    cFileStream << "Msg: " << sMsg;
    cFileStream.flush();
}

void ConsoleObserverFile::Error  (const char *sErr)
{
    cFileStream << "Err: " << sErr;
    cFileStream.flush();
}

void ConsoleObserverFile::Log    (const char *sLog)
{
    cFileStream << "Log: " << sLog;
    cFileStream.flush();
}


ConsoleObserverStd::ConsoleObserverStd() :
#   if defined(FC_OS_WIN32)
    useColorStderr(true)
#   elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    useColorStderr( isatty(STDERR_FILENO) )
#   else
    useColorStderr(false)
#   endif
{
    bLog = false;
}

ConsoleObserverStd::~ConsoleObserverStd()
{
}

void ConsoleObserverStd::Message(const char *sMsg)
{
    printf("%s",sMsg);
}

void ConsoleObserverStd::Warning(const char *sWarn)
{
    if (useColorStderr) {
#   if defined(FC_OS_WIN32)
        ::SetConsoleTextAttribute(::GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_GREEN| FOREGROUND_BLUE);
#   elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        fprintf(stderr, "\033[1;33m");
#   endif
    }

    fprintf(stderr, "%s", sWarn);

    if (useColorStderr) {
#   if defined(FC_OS_WIN32)
        ::SetConsoleTextAttribute(::GetStdHandle(STD_ERROR_HANDLE),FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
#   elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        fprintf(stderr, "\033[0m");
#   endif
    }
}

void ConsoleObserverStd::Error  (const char *sErr)
{
    if (useColorStderr) {
#   if defined(FC_OS_WIN32)
        ::SetConsoleTextAttribute(::GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED|FOREGROUND_INTENSITY );
#   elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        fprintf(stderr, "\033[1;31m");
#   endif
    }

    fprintf(stderr, "%s", sErr);

    if (useColorStderr) {
#   if defined(FC_OS_WIN32)
        ::SetConsoleTextAttribute(::GetStdHandle(STD_ERROR_HANDLE),FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
#   elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        fprintf(stderr, "\033[0m");
#   endif
    }
}

void ConsoleObserverStd::Log    (const char *sErr)
{
    if (useColorStderr) {
#   if defined(FC_OS_WIN32)
        ::SetConsoleTextAttribute(::GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED |FOREGROUND_GREEN);
#   elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        fprintf(stderr, "\033[1;36m");
#   endif
    }

    fprintf(stderr, "%s", sErr);

    if (useColorStderr) {
#   if defined(FC_OS_WIN32)
        ::SetConsoleTextAttribute(::GetStdHandle(STD_ERROR_HANDLE),FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
#   elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        fprintf(stderr, "\033[0m");
#   endif
    }
}

RedirectStdOutput::RedirectStdOutput() 
{
    buffer.reserve(80);
}

int RedirectStdOutput::overflow(int c)
{
    if (c != EOF)
        buffer.push_back((char)c);
    return c;
}

int RedirectStdOutput::sync()
{
    // Print as log as this might be verbose
    if (!buffer.empty()) {
        Base::Console().Log("%s", buffer.c_str());
        buffer.clear();
    }
    return 0;
}

RedirectStdLog::RedirectStdLog() 
{
    buffer.reserve(80);
}

int RedirectStdLog::overflow(int c)
{
    if (c != EOF)
        buffer.push_back((char)c);
    return c;
}

int RedirectStdLog::sync()
{
    // Print as log as this might be verbose
    if (!buffer.empty()) {
        Base::Console().Log("%s", buffer.c_str());
        buffer.clear();
    }
    return 0;
}

RedirectStdError::RedirectStdError() 
{
    buffer.reserve(80);
}

int RedirectStdError::overflow(int c)
{
    if (c != EOF)
        buffer.push_back((char)c);
    return c;
}

int RedirectStdError::sync()
{
    if (!buffer.empty()) {
        Base::Console().Error("%s", buffer.c_str());
        buffer.clear();
    }
    return 0;
}

//---------------------------------------------------------

std::stringstream &LogLevel::prefix(std::stringstream &str, const char *src, int line)
{
    static FC_TIME_POINT s_tstart;
    static bool s_timing = false;
    if(print_time) {
        if(!s_timing) {
            s_timing = true;
            _FC_TIME_INIT(s_tstart);
        }
        auto tnow = std::chrono::FC_TIME_CLOCK::now();
        auto d = std::chrono::duration_cast<FC_DURATION>(tnow-s_tstart);
        str << d.count() << ' ';
    }
    if(print_tag) str << '<' << tag << "> ";
    if(print_src) {
        const char *_f = std::strrchr(src, '/');
        str << (_f?_f+1:src)<<"("<<line<<"): ";
    }
    return str;
}
