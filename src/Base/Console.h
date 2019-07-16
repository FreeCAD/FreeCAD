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
#include <map>
#include <string>
#include <cstring>
#include <sstream>
#include <chrono>

//**************************************************************************
// Logging levels

#ifdef FC_DEBUG
/// switch on the logging of python object creation and destruction
#  undef FC_LOGPYOBJECTS
/// switch on the logging of Feature update and execution
#  define FC_LOGFEATUREUPDATE
/// switch on the logging of the Update execution through Doc, App, GuiApp and GuiDoc
#  undef FC_LOGUPDATECHAIN
#endif

/////////////////////////////////////////////////////////////////////////////////////

/** \page LogLevelPage Tag based log helpers
 * Simple tag based log and timing helper macros and functions.
 *
 * \section Motivation
 *
 * FreeCAD Base::Console() is capable of outputting to different targets, and has
 * some basic enable/disable control of different types of logs. There is,
 * however, no easy way to use logging facility for various FC modules.  This set
 * of helper macros and function is aimed to provide a unified logging (and timing)
 * interface.  The interface is mainly designed for C++ code.  Python code can
 * also take some advantage of log level control interface. The developer can
 * now leave their logging code permanently active in the source code without
 * impact on performance, and the log can be easily turned on/off dynamically
 * using Python console for debugging purpose, even in release build.
 *
 * \section SampleUsage Sample Usage
 *
 * A set of macros is provided to ease the usage of tag based log. All the
 * macros are defined in <Base/Console.h>. At the beginning of your C++ source,
 * you need to initialize the log level of your chosen tag using,
 *
 * \code{.c}
 * FC_LOG_LEVEL_INIT(tag)
 * \endcode
 *
 * It makes sense to use the same tag in multiple files for easy log level
 * control. Please check \ref Customization if You want more than one tag inside
 * the same source file.
 *
 * Predefined log levels are,
 *
 * \code{.c}
 * #define FC_LOGLEVEL_ERR 0
 * #define FC_LOGLEVEL_WARN 1
 * #define FC_LOGLEVEL_MSG 2
 * #define FC_LOGLEVEL_LOG 3
 * #define FC_LOGLEVEL_TRACE 4
 * \endcode
 *
 * Bigger log level integer values have lower priorities. There is a special log
 * level,
 *
 * \code{.c}
 * #define FC_LOGLEVEL_DEFAULT -1
 * \endcode
 *
 * Actually, any negative log level behave the same, which is for tags
 * that are not previously configured by user. The actual log level applied is
 * controlled by \c Base::Console().SetDefaultLogLevel(). Python
 * developers/end-users can configure the default log level by calling
 *
 * \code{.py}
 * FreeCAD.setLogLevel('Default',level)
 * FreeCAD.setLogLevel('DebugDefault',level)
 * \endcode
 *
 * where \c level is either a string of value <tt>Error, Warning, Message, Log,
 * Trace</tt> or an integer value. By default, on release build, the default log
 * level is \c FC_LOGLEVEL_MSG, and on debug build, \c FC_LOGLEVEL_LOG.
 *
 * Python code can call \c FreeCAD.setLogLevel(tag,level) to configure any other
 * tag log levels, and \c FreeCAD.getLogLevel(tag), which outputs only integer
 * log level.
 *
 * You can fine tune how the log is output by passing extra parameters to
 * #FC_LOG_LEVEL_INIT(). All the extra parameters are boolean value, which are
 * shown blew along with their default values.
 *
 * \code{.c}
 * FC_LOG_LEVEL_INIT(tag, print_tag=true, print_src=false,
 *          print_time=false, add_eol=true, refresh=false)
 * \endcode
 *
 * You can dynamically modify the log style as well, by changing these
 * variables before the actual log output macro. See \ref Customization for
 * more details
 *
 * \code{.c}
 * FC_LOG_INSTANCE.refresh = true; // print time for each log, default false.
 * FC_LOG_INSTANCE.print_src = true; // print file and line number, default false.
 * FC_LOG_INSTANCE.print_tag = false; // do not print tag, default true
 * FC_LOG_INSTANCE.add_eol = false; // do not add eol
 * FC_LOG_INSTANCE.refresh = true; // refresh GUI after each log
 * \endcode
 *
 * Be careful with 'refresh' option. Its current implementation calls
 * QCoreApplication::sendPostedEvent() which may cause some unexpected
 * behavior, especially when called inside a destructor.
 *
 * The actual logging macros are
 *
 * \code
 * FC_ERR(msg)
 * FC_WARN(msg)
 * FC_MSG(msg)
 * FC_LOG(msg)
 * FC_TRACE(msg)
 * \endcode
 *
 * The logging macros correspond to existing Base::Console() output functions,
 * and \c FC_TRACE uses Base::Console().Log(), same as \c FC_LOG. These macros
 * checks the log level defined in \c FC_LOG_LEVEL_INIT to decide whether to
 * print log or not. \c msg here shall be a C++ streaming expression. End of
 * line will be automatically appended by default.
 *
 * \code
 * FC_ERR("error: " << code << ". exiting")
 * \endcode
 *
 * \section TimingHelper Timing Helpers
 *
 * This set of macros is for helping C++ code to time lengthy operations.
 * Examples:
 *
 * \code{.c}
 * void operation() {
 *      FC_TIME_INIT(t);
 *
 *      //do stuff
 *
 *      FC_TIME_LOG(t,"operation done.");
 * }
 * \endcode
 *
 * This will output in console something like,
 *
 * \code
 * operation done. time: 1.12s
 * \endcode
 *
 * Every time you call \c FC_TIME_LOG it will calculate the time duration
 * between this call and the last \c FC_TIME_LOG or \c FC_TIME_INIT.  Time
 * variable \c t will then be updated to the current time. You can also use
 * <tt>FC_TIME_MSG, FC_TIME_TRACE</tt> similar to <tt>FC_MSG and FC_TRACE</tt>.
 *
 * To time operation in multiple stages,
 *
 * \code{.cpp}
 * void operation() {
 *      FC_TIME_INIT2(t,t1);
 *
 *      //do stage 1
 *
 *      FC_TIME_LOG(t1,"stage1");
 *
 *      //do stage 2
 *
 *      FC_TIME_LOG(t1,"stage2");
 *
 *      // do other stuff
 *
 *      FC_TIME_LOG(t,"total");
 * }
 * \endcode
 *
 * Will output something like,
 * \code
 * stage1 time: 1.2s
 * stage2 time: 2.3s
 * total time: 4.0s
 * \endcode
 *
 * To time operation in multiple functions,
 *
 * \code{.cpp}
 * class Timing {
 *      FC_DURATION_DECLARE(d1)
 *      FC_DURATION_DECLARE(d1_1)
 *      FC_DURATION_DECLARE(d1_2)
 *      FC_DURATION_DECLARE(d2);
 *
 *      Timing() {
 *          FC_DURATION_INIT(d1);
 *          FC_DURATION_INIT(d1_1);
 *          FC_DURATION_INIT(d1_2);
 *          FC_DURATION_INIT(d2);
 *      }
 * };
 *
 * void operation1(Timing &timing) {
 *
 *      FC_TIME_INIT(t);
 *
 *      for(...) {
 *          FC_TIME_INIT(t1);
 *
 *          //do setp 1
 *
 *          FC_DURATION_PLUS(timing.d1_1,t1);
 *
 *          // do step 2
 *
 *          FC_DURATION_PLUS(timing.d1_2,t1);
 *      }
 *
 *      // do other stuff
 *
 *      FC_DRUATION_PLUS(timing.d1, t);
 * }
 *
 * void operation2(Timing &timing) {
 *
 *      FC_TIME_INIT(t);
 *
 *      // do stuff
 *
 *      FC_DRUATION_PLUS(timing.d2, t);
 * }
 *
 * void operation() {
 *
 *      Timing timing;
 *
 *      FC_TIME_INIT(t);
 *
 *      for(...) {
 *          operation1(timing);
 *
 *          // do other stuff
 *
 *          operation2(timing);
 *      }
 *
 *      FC_DURATION_LOG(timing.d1_1,"operation 1 step 1");
 *      FC_DURATION_LOG(timing.d1_2,"operation 1 step 2");
 *      FC_DURATION_LOG(timing.d1,"operation 1");
 *      FC_DURATION_LOG(timing.d2,"operation 2");
 *      FC_TIME_LOG(t,"operation total");
 * }
 * \endcode
 *
 * You can also use <tt>FC_DURATION_MSG, FC_DURATION_TRACE</tt> as usual.
 *
 * If you use only macros provided here to do timing, the entire timing code
 * can be compiled out by defining \c FC_LOG_NO_TIMING before including
 * \c App/Console.h.
 *
 * \section Customization
 *
 * Most of the logging facilities are exposed through macros. This section
 * briefs how they are implemented under the hood in case you want
 * customization.  A new function GetLogLevel(tag) is added to Base::Console()
 * to let C++ developer query a log level for an arbitrary string tag. The
 * function returns a pointer to an integer representing the log level. Python
 * developer or end-user can set/get the same tag based log level using
 * FreeCAD.setLogLevel/getLogLevel. Any change to the log level is reflected
 * through the pointer returned by Base::Console().GetLogLevel(). What
 * \c FC_LOG_LEVEL_INIT(tag) does is to define a class Base::LogLevel, and then
 * a file static instance of that class to store the pointer to the desired tag
 * log level. The class and instance name is predefined. Various log macros
 * will check that instance to query log level. If you some how want to have
 * more than one tag inside the same source file, use the following macros to
 * define a second instance of name \c instance_name
 *
 * \code
 * _FC_LOG_LEVEL_INIT(instance_name,tag)
 * \endcode
 *
 * Then, define a second set of logging macros as
 *
 * \code{.c}
 * #define MY_MSG(_msg) _FC_PRINT(instance_name,FC_LOGLEVEL_MSG,Message,_msg)
 * #define MY_WARN(_msg) _FC_PRINT(instance_name,FC_LOGLEVEL_WARN,Warning,_msg)
 * #define MY_ERR(_msg) _FC_PRINT(instance_name,FC_LOGLEVEL_ERR,Error,_msg)
 * #define MY_LOG(_msg) _FC_PRINT(instance_name,FC_LOGLEVEL_LOG,Log,_msg)
 * #define MY_TRACE(_msg) _FC_PRINT(instance_name,FC_LOGLEVEL_TRACE,Log,_msg)
 * \endcode
 *
 * Note, replace \c instance_name with your actual desired name.
 *
 * You can also define your own log levels the same way. Macro
 * #_FC_PRINT(_instance,_l,_func,_msg) checks to see if the log shall proceed,
 * where \c _instance is the static loglevel instance name (default is
 * \c FC_LOG_INSTANCE), and \c _l is the log level constant to be checked,
 * \c _func is the Base::Console() function to print the log.
 *
 */

#define FC_LOGLEVEL_DEFAULT -1
#define FC_LOGLEVEL_ERR 0
#define FC_LOGLEVEL_WARN 1
#define FC_LOGLEVEL_MSG 2
#define FC_LOGLEVEL_LOG 3
#define FC_LOGLEVEL_TRACE 4

#define _FC_LOG_LEVEL_INIT(_name,_tag,...) \
    static Base::LogLevel _name(_tag,## __VA_ARGS__);

#ifndef FC_LOG_INSTANCE
#   define FC_LOG_INSTANCE _s_fclvl
#endif

#define FC_LOG_LEVEL_INIT(_tag,...) \
    _FC_LOG_LEVEL_INIT(FC_LOG_INSTANCE, _tag, ## __VA_ARGS__)

#define _FC_PRINT(_instance,_l,_func,_msg) do{\
    if(_instance.isEnabled(_l)) {\
        std::stringstream str;\
        _instance.prefix(str,__FILE__,__LINE__) << _msg;\
        if(_instance.add_eol) \
            str<<std::endl;\
        Base::Console()._func("%s",str.str().c_str());\
        if(_instance.refresh) Base::Console().Refresh();\
    }\
}while(0)

#define FC_MSG(_msg) _FC_PRINT(FC_LOG_INSTANCE,FC_LOGLEVEL_MSG,Message,_msg)
#define FC_WARN(_msg) _FC_PRINT(FC_LOG_INSTANCE,FC_LOGLEVEL_WARN,Warning,_msg)
#define FC_ERR(_msg) _FC_PRINT(FC_LOG_INSTANCE,FC_LOGLEVEL_ERR,Error,_msg)
#define FC_LOG(_msg) _FC_PRINT(FC_LOG_INSTANCE,FC_LOGLEVEL_LOG,Log,_msg)
#define FC_TRACE(_msg) _FC_PRINT(FC_LOG_INSTANCE,FC_LOGLEVEL_TRACE,Log,_msg)
#define FC_XYZ(_pt) '('<<(_pt).X()<<", " << (_pt).Y()<<", " << (_pt).Z()<<')'
#define FC_XY(_pt) '('<<(_pt).x<<", " << (_pt).y<<')'

#ifndef FC_LOG_NO_TIMING
#   define FC_TIME_CLOCK high_resolution_clock
#   define FC_TIME_POINT std::chrono::FC_TIME_CLOCK::time_point
#   define FC_DURATION std::chrono::duration<double>

#   define _FC_TIME_INIT(_t) _t=std::chrono::FC_TIME_CLOCK::now()
#   define FC_TIME_INIT(_t) FC_TIME_POINT _FC_TIME_INIT(_t)
#   define FC_TIME_INIT2(_t1,_t2) FC_TIME_INIT(_t1),_t2=_t1
#   define FC_TIME_INIT3(_t1,_t2,_t3) FC_TIME_INIT(_t1),_t2=_t1,_t3=_t1

#   define _FC_DURATION_PRINT(_l,_d,_msg) \
        FC_##_l(_msg<< " time: " << _d.count()<<'s');

#   define FC_DURATION_MSG(_d,_msg) _FC_DURATION_PRINT(MSG,_d,_msg)
#   define FC_DURATION_LOG(_d,_msg) _FC_DURATION_PRINT(LOG,_d,_msg)
#   define FC_DURATION_TRACE(_d,_msg) _FC_DURATION_PRINT(TRACE,_d,_msg)

#   define _FC_TIME_PRINT(_l,_t,_msg) \
        _FC_DURATION_PRINT(_l,Base::GetDuration(_t),_msg);

#   define FC_TIME_MSG(_t,_msg) _FC_TIME_PRINT(MSG,_t,_msg)
#   define FC_TIME_LOG(_t,_msg) _FC_TIME_PRINT(LOG,_t,_msg)
#   define FC_TIME_TRACE(_t,_msg) _FC_TIME_PRINT(TRACE,_t,_msg)

#   define FC_DURATION_DECLARE(_d) FC_DURATION _d
#   define FC_DURATION_DECLARE2(_d,_d1) FC_DURATION_DECLARE(_d),_d1
#   define FC_DURATION_DECLARE3(_d,_d1) FC_DURATION_DECLARE2(_d,_d1),_d2

#   define FC_DURATION_INIT(_d) _d=FC_DURATION(0)
#   define FC_DURATION_INIT2(_d,_d1) _d=_d1=FC_DURATION(0)
#   define FC_DURATION_INIT3(_d,_d1,_d2) _d=_d1=_d2=FC_DURATION(0)

#   define FC_DURATION_DECL_INIT(_d) FC_DURATION _d(0)
#   define FC_DURATION_DECL_INIT2(_d,_d1) FC_DURATION_DECL_INIT(_d),_d1(0)
#   define FC_DURATION_DECL_INIT3(_d,_d1) FC_DURATION_DECL_INIT2(_d,_d1),_d3(0)

#   define FC_DURATION_PLUS(_d,_t) _d += Base::GetDuration(_t)

#else //FC_LOG_NO_TIMING
#   define FC_TIME_POINT
#   define _FC_TIME_INIT(...) do{}while(0)
#   define FC_TIME_INIT(...) do{}while(0)
#   define FC_TIME_INIT2(...) do{}while(0)
#   define FC_TIME_INIT3(...) do{}while(0)
#   define _FC_DURATION_PRINT(...) do{}while(0)
#   define _FC_TIME(_t) do{}while(0)
#   define FC_DURATION_PRINT(...) do{}while(0)
#   define FC_DURATION
#   define FC_DURATION_INIT(...) do{}while(0)
#   define FC_DURATION_INIT1(...) do{}while(0)
#   define FC_DURATION_INIT2(...) do{}while(0)
#   define FC_DURATION_DECLARE(...)
#   define FC_DURATION_DECLARE1(...)
#   define FC_DURATION_DECLARE2(...)
#   define FC_DURATION_DECL_INIT(...) do{}while(0)
#   define FC_DURATION_DECL_INIT2(...) do{}while(0)
#   define FC_DURATION_DECL_INIT3(...) do{}while(0)
#   define FC_DURATION_PLUS(...) do{}while(0)

#endif //FC_LOG_NO_TIMING

namespace Base {
class ConsoleSingleton;
} // namespace Base

typedef Base::ConsoleSingleton ConsoleMsgType;
typedef unsigned int ConsoleMsgFlags;

namespace Base {

#ifndef FC_LOG_NO_TIMING
    inline FC_DURATION GetDuration(FC_TIME_POINT &t)
    {
        auto tnow = std::chrono::FC_TIME_CLOCK::now();
        auto d = std::chrono::duration_cast<FC_DURATION>(tnow-t);
        t = tnow;
        return d;
    }
#endif

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
        :bErr(true),bMsg(true),bLog(true),bWrn(true){}
    virtual ~ConsoleObserver() {}
    /// get calls when a Warning is issued
    virtual void Warning(const char *){}
    /// get calls when a Message is issued
    virtual void Message(const char *){}
    /// get calls when a Error is issued
    virtual void Error  (const char *)=0;
    /// get calls when a Log Message is issued
    virtual void Log    (const char *){}

    virtual const char *Name(void){return 0L;}
    bool bErr,bMsg,bLog,bWrn;
};


/** The console class
 *  This class manage all the stdio stuff. This includes
 *  Messages, Warnings, Log entries and Errors. The incoming
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
 *  ConsoleSingleton is able to switch between several modes to, e.g. switch
 *  the logging on or off, or treat Warnings as Errors, and so on...
 *  @see ConsoleObserver
 */
class BaseExport ConsoleSingleton
{

public:
    static const unsigned int BufferSize = 4024;
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
        Verbose = 1,	// suppress Log messages
    };
    enum ConnectionMode {
        Direct = 0,
        Queued =1
    };

    enum FreeCAD_ConsoleMsgType {
        MsgType_Txt = 1,
        MsgType_Log = 2, // ConsoleObserverStd sends this and higher to stderr
        MsgType_Wrn = 4,
        MsgType_Err = 8
    };

    /// Change mode
    void SetConsoleMode(ConsoleMode m);
    /// Change mode
    void UnsetConsoleMode(ConsoleMode m);
    /// Enables or disables message types of a certain console observer
    ConsoleMsgFlags SetEnabledMsgType(const char* sObs, ConsoleMsgFlags type, bool b);
    /// Enables or disables message types of a certain console observer
    bool IsMsgTypeEnabled(const char* sObs, FreeCAD_ConsoleMsgType type) const;
    void SetConnectionMode(ConnectionMode mode);

    int *GetLogLevel(const char *tag, bool create=true);

    void SetDefaultLogLevel(int level) {
        _defaultLogLevel = level;
    }

    inline int LogLevel(int level) const{
        return level<0?_defaultLogLevel:level;
    }

    /// singleton
    static ConsoleSingleton &Instance(void);

    // retrieval of an observer by name
    ConsoleObserver *Get(const char *Name) const;

    static PyMethodDef    Methods[];

    void Refresh();
    void EnableRefresh(bool enable);

protected:
    // python exports goes here +++++++++++++++++++++++++++++++++++++++++++
    // static python wrapper of the exported functions
    static PyObject *sPyLog      (PyObject *self,PyObject *args);
    static PyObject *sPyMessage  (PyObject *self,PyObject *args);
    static PyObject *sPyWarning  (PyObject *self,PyObject *args);
    static PyObject *sPyError    (PyObject *self,PyObject *args);
    static PyObject *sPySetStatus(PyObject *self,PyObject *args);
    static PyObject *sPyGetStatus(PyObject *self,PyObject *args);

    bool _bVerbose;
    bool _bCanRefresh;
    ConnectionMode connectionMode;

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

    std::map<std::string, int> _logLevels;
    int _defaultLogLevel;

    friend class ConsoleOutput;
};

/** Access to the Console
 *  This method is used to gain access to the one and only instance of
 *  the ConsoleSingleton class.
 */
inline ConsoleSingleton &Console(void){
    return ConsoleSingleton::Instance();
}

class BaseExport ConsoleRefreshDisabler {
public:
    ConsoleRefreshDisabler() {
        Console().EnableRefresh(false);
    }

    ~ConsoleRefreshDisabler() {
        Console().EnableRefresh(true);
    }
};


/** LogLevel helper class */
class BaseExport LogLevel {
public:
    std::string tag;
    int &lvl;
    bool print_tag;
    bool print_src;
    bool print_time;
    bool add_eol;
    bool refresh;

    LogLevel(const char *tag, bool print_tag=true, bool print_src=false,
            bool print_time=false, bool add_eol=true, bool refresh=false)
        :tag(tag),lvl(*Console().GetLogLevel(tag))
        ,print_tag(print_tag),print_src(print_src),print_time(print_time)
        ,add_eol(add_eol),refresh(refresh)
    {}

    bool isEnabled(int l) {
        return l<=level();
    }

    int level() const {
        return Console().LogLevel(lvl);
    }

    std::stringstream &prefix(std::stringstream &str, const char *src, int line);
};


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
protected:
    bool useColorStderr;
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
