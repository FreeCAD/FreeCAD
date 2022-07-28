/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# if defined(FC_OS_WIN32)
#  include <windows.h>
# elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX)
#  include <unistd.h>
# endif
# include <cstring>
# include <functional>
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
        instance = nullptr;
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

ConsoleOutput* ConsoleOutput::instance = nullptr;

}

//**************************************************************************
// Construction destruction


ConsoleSingleton::ConsoleSingleton()
  : _bVerbose(true)
  , _bCanRefresh(true)
  , connectionMode(Direct)
#ifdef FC_DEBUG
  ,_defaultLogLevel(FC_LOGLEVEL_LOG)
#else
  ,_defaultLogLevel(FC_LOGLEVEL_MSG)
#endif
{
}

ConsoleSingleton::~ConsoleSingleton()
{
    ConsoleOutput::destruct();
    for (std::set<ILogger * >::iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter)
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
 *                       Base:ConsoleSingleton::MsgType_Wrn|Base::ConsoleSingleton::MsgType_Err, false);
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
    ILogger* pObs = Get(sObs);
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
    ILogger* pObs = Get(sObs);
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

    return false;
}

void ConsoleSingleton::SetConnectionMode(ConnectionMode mode)
{
    connectionMode = mode;

    // make sure this method gets called from the main thread
    if (connectionMode == Queued) {
        ConsoleOutput::getInstance();
    }
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
#define FC_CONSOLE_FMT(_type,_type2) \
    char format[BufferSize];\
    format[sizeof(format)-4] = '.';\
    format[sizeof(format)-3] = '.';\
    format[sizeof(format)-2] = '\n';\
    format[sizeof(format)-1] = 0;\
    const unsigned int format_len = sizeof(format)-4;\
    va_list namelessVars;\
    va_start(namelessVars, pMsg);\
    vsnprintf(format, format_len, pMsg, namelessVars);\
    format[sizeof(format)-5] = '.';\
    va_end(namelessVars);\
    if (connectionMode == Direct)\
        Notify##_type(format);\
    else\
        QCoreApplication::postEvent(ConsoleOutput::getInstance(), new ConsoleEvent(MsgType_##_type2, format));

    FC_CONSOLE_FMT(Message,Txt);
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
    FC_CONSOLE_FMT(Warning,Wrn);
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
    FC_CONSOLE_FMT(Error,Err);
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
    if (_bVerbose)
    {
        FC_CONSOLE_FMT(Log,Log);
    }
}



//**************************************************************************
// Observer stuff

/** Attaches an Observer to Console
 *  Use this method to attach a ILogger derived class to
 *  the Console. After the observer is attached all messages will also
 *  be forwarded to it.
 *  @see ILogger
 */
void ConsoleSingleton::AttachObserver(ILogger *pcObserver)
{
    // double insert !!
    assert(_aclObservers.find(pcObserver) == _aclObservers.end() );

    _aclObservers.insert(pcObserver);
}

/** Detaches an Observer from Console
 *  Use this method to detach a ILogger derived class.
 *  After detaching you can destruct the Observer or reinsert it later.
 *  @see ILogger
 */
void ConsoleSingleton::DetachObserver(ILogger *pcObserver)
{
    _aclObservers.erase(pcObserver);
}

void ConsoleSingleton::NotifyMessage(const char *sMsg)
{
    for (std::set<ILogger * >::iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter) {
        if ((*Iter)->bMsg)
            (*Iter)->SendLog(sMsg, LogStyle::Message);   // send string to the listener
    }
}

void ConsoleSingleton::NotifyWarning(const char *sMsg)
{
    for (std::set<ILogger * >::iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter) {
        if ((*Iter)->bWrn)
            (*Iter)->SendLog(sMsg, LogStyle::Warning);   // send string to the listener
    }
}

void ConsoleSingleton::NotifyError(const char *sMsg)
{
    for (std::set<ILogger * >::iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter) {
        if ((*Iter)->bErr)
            (*Iter)->SendLog(sMsg, LogStyle::Error);   // send string to the listener
    }
}

void ConsoleSingleton::NotifyLog(const char *sMsg)
{
    for (std::set<ILogger * >::iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter) {
        if ((*Iter)->bLog)
            (*Iter)->SendLog(sMsg, LogStyle::Log);   // send string to the listener
    }
}

ILogger *ConsoleSingleton::Get(const char *Name) const
{
    const char* OName;
    for (std::set<ILogger * >::const_iterator Iter=_aclObservers.begin();Iter!=_aclObservers.end();++Iter) {
        OName = (*Iter)->Name();   // get the name
        if (OName && strcmp(OName,Name) == 0)
            return *Iter;
    }
    return nullptr;
}

int *ConsoleSingleton::GetLogLevel(const char *tag, bool create) {
    if (!tag) tag = "";
    if (_logLevels.find(tag) != _logLevels.end())
        return &_logLevels[tag];
    if (!create)
        return nullptr;
    int &ret = _logLevels[tag];
    ret = -1;
    return &ret;
}

void ConsoleSingleton::Refresh() {
    if (_bCanRefresh)
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}

void ConsoleSingleton::EnableRefresh(bool enable) {
    _bCanRefresh = enable;
}

//**************************************************************************
// Singleton stuff

ConsoleSingleton * ConsoleSingleton::_pcSingleton = nullptr;

void ConsoleSingleton::Destruct()
{
    // not initialized or double destructed!
    assert(_pcSingleton);
    delete _pcSingleton;
    _pcSingleton=nullptr;
}

ConsoleSingleton & ConsoleSingleton::Instance()
{
    // not initialized?
    if (!_pcSingleton)
    {
        _pcSingleton = new ConsoleSingleton();
    }
    return *_pcSingleton;
}

//**************************************************************************
// Python stuff

// ConsoleSingleton Methods						// Methods structure
PyMethodDef ConsoleSingleton::Methods[] = {
    {"PrintMessage",         ConsoleSingleton::sPyMessage, METH_VARARGS,
     "PrintMessage(obj) -> None\n\n"
     "Print a message to the output.\n\n"
     "obj : object\n    The string representation is printed."},
    {"PrintLog",             ConsoleSingleton::sPyLog, METH_VARARGS,
     "PrintLog(obj) -> None\n\n"
     "Print a log message to the output.\n\n"
     "obj : object\n    The string representation is printed."},
    {"PrintError",           ConsoleSingleton::sPyError, METH_VARARGS,
     "PrintError(obj) -> None\n\n"
     "Print an error message to the output.\n\n"
     "obj : object\n    The string representation is printed."},
    {"PrintWarning",         ConsoleSingleton::sPyWarning, METH_VARARGS,
     "PrintWarning(obj) -> None\n\n"
     "Print a warning message to the output.\n\n"
     "obj : object\n    The string representation is printed."},
    {"SetStatus",            ConsoleSingleton::sPySetStatus, METH_VARARGS,
     "SetStatus(observer, type, status) -> None\n\n"
     "Set the status for either 'Log', 'Msg', 'Wrn' or 'Error' for an observer.\n\n"
     "observer : str\n    Logging interface name.\n"
     "type : str\n    Message type.\n"
     "status : bool"},
    {"GetStatus",            ConsoleSingleton::sPyGetStatus, METH_VARARGS,
     "GetStatus(observer, type) -> bool or None\n\n"
     "Get the status for either 'Log', 'Msg', 'Wrn' or 'Error' for an observer.\n"
     "Returns None if the specified observer doesn't exist.\n\n"
     "observer : str\n    Logging interface name.\n"
     "type : str\n    Message type."},
    {"GetObservers",      ConsoleSingleton::sPyGetObservers, METH_VARARGS,
     "GetObservers() -> list of str\n\n"
     "Get the names of the current logging interfaces."},
    {nullptr, nullptr, 0, nullptr}		/* Sentinel */
};

namespace {
PyObject* FC_PYCONSOLE_MSG(std::function<void(const char*)> func, PyObject* args)
{
    PyObject *output;
    if (!PyArg_ParseTuple(args, "O", &output))
        return nullptr;
    PY_TRY {
        const char* string = nullptr;
        PyObject* unicode = nullptr;
        if (PyUnicode_Check(output)) {
            string = PyUnicode_AsUTF8(output);
        }
        else {
            unicode = PyObject_Str(output);
            if (unicode)
                string = PyUnicode_AsUTF8(unicode);
        }
        if (string)
            func(string);            /*process message*/
        Py_XDECREF(unicode);
    }
    PY_CATCH
    Py_Return;
}
}

PyObject *ConsoleSingleton::sPyMessage(PyObject * /*self*/, PyObject *args)
{
    return FC_PYCONSOLE_MSG([](const char* msg) {
        Instance().Message("%s", msg);
    }, args);
}

PyObject *ConsoleSingleton::sPyWarning(PyObject * /*self*/, PyObject *args)
{
    return FC_PYCONSOLE_MSG([](const char* msg) {
        Instance().Warning("%s", msg);
    }, args);
}

PyObject *ConsoleSingleton::sPyError(PyObject * /*self*/, PyObject *args)
{
    return FC_PYCONSOLE_MSG([](const char* msg) {
        Instance().Error("%s", msg);
    }, args);
}

PyObject *ConsoleSingleton::sPyLog(PyObject * /*self*/, PyObject *args)
{
    return FC_PYCONSOLE_MSG([](const char* msg) {
        Instance().Log("%s", msg);
    }, args);
}

PyObject *ConsoleSingleton::sPyGetStatus(PyObject * /*self*/, PyObject *args)
{
    char *pstr1;
    char *pstr2;
    if (!PyArg_ParseTuple(args, "ss", &pstr1, &pstr2))
        return nullptr;

    PY_TRY{
        bool b=false;
        ILogger *pObs = Instance().Get(pstr1);
        if (!pObs)
            Py_Return;

        if (strcmp(pstr2,"Log") == 0)
            b = pObs->bLog;
        else if (strcmp(pstr2,"Wrn") == 0)
            b = pObs->bWrn;
        else if (strcmp(pstr2,"Msg") == 0)
            b = pObs->bMsg;
        else if (strcmp(pstr2,"Err") == 0)
            b = pObs->bErr;
        else
            Py_Error(Base::PyExc_FC_GeneralError,"Unknown message type (use 'Log', 'Err', 'Msg' or 'Wrn')");

        return PyBool_FromLong(b ? 1 : 0);
    }
    PY_CATCH;
}

PyObject *ConsoleSingleton::sPySetStatus(PyObject * /*self*/, PyObject *args)
{
    char *pstr1;
    char *pstr2;
    PyObject* pyStatus;
    if (!PyArg_ParseTuple(args, "ssO!", &pstr1, &pstr2, &PyBool_Type, &pyStatus))
        return nullptr;

    PY_TRY{
        bool status = asBoolean(pyStatus);
        ILogger *pObs = Instance().Get(pstr1);
        if (pObs) {
            if (strcmp(pstr2,"Log") == 0)
                pObs->bLog = status;
            else if (strcmp(pstr2,"Wrn") == 0)
                pObs->bWrn = status;
            else if (strcmp(pstr2,"Msg") == 0)
                pObs->bMsg = status;
            else if (strcmp(pstr2,"Err") == 0)
                pObs->bErr = status;
            else
                Py_Error(Base::PyExc_FC_GeneralError,"Unknown message type (use 'Log', 'Err', 'Msg' or 'Wrn')");

            Py_Return;
        }
        else {
            Py_Error(Base::PyExc_FC_GeneralError,"Unknown logger type");
        }

    }
    PY_CATCH;
}

PyObject *ConsoleSingleton::sPyGetObservers(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        Py::List list;
        for (auto i : Instance()._aclObservers)
            list.append(Py::String(i->Name() ? i->Name() : ""));

        return Py::new_reference_to(list);
    }
    PY_CATCH
}

Base::ILogger::~ILogger()
{}
