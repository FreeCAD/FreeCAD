// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

// Std. configurations
#include <array>
#include <cassert>
#include <chrono>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <FCGlobal.h>

#include <fmt/printf.h>

// Python stuff
using PyObject = struct _object;
using PyMethodDef = struct PyMethodDef;

// FIXME: Even with parameter packs this is necessary for MSYS2
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

//**************************************************************************
// Logging levels

#ifdef FC_DEBUG
/// switch on the logging of python object creation and destruction
# undef FC_LOGPYOBJECTS
/// switch on the logging of Feature update and execution
# define FC_LOGFEATUREUPDATE
/// switch on the logging of the Update execution through Doc, App, GuiApp and GuiDoc
# undef FC_LOGUPDATECHAIN
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
 * FC_LOG_LEVEL_INIT(tag, print_tag=true, print_src=0,
 *          print_time=false, add_eol=true, refresh=false)
 * \endcode
 *
 * You can dynamically modify the log style as well, by changing these
 * variables before the actual log output macro. See \ref Customization for
 * more details
 *
 * \code{.c}
 * FC_LOG_INSTANCE.refresh = true; // print time for each log, default false.
 *
 * // print file and line number, default 0, if set to 2 then print python
 * // source from current call frame.
 * FC_LOG_INSTANCE.print_src = 1;
 *
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

// NOLINTBEGIN(bugprone-reserved-identifier,bugprone-macro-parentheses,cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
// clang-format off
#define FC_LOGLEVEL_DEFAULT -1
#define FC_LOGLEVEL_ERR 0
#define FC_LOGLEVEL_WARN 1
#define FC_LOGLEVEL_MSG 2
#define FC_LOGLEVEL_LOG 3
#define FC_LOGLEVEL_TRACE 4

#define _FC_LOG_LEVEL_INIT(_name, _tag, ...) static Base::LogLevel _name(_tag, ##__VA_ARGS__);

#ifndef FC_LOG_INSTANCE
#define FC_LOG_INSTANCE _s_fclvl
#endif

#define FC_LOG_LEVEL_INIT(_tag, ...) _FC_LOG_LEVEL_INIT(FC_LOG_INSTANCE, _tag, ##__VA_ARGS__)

#define __FC_PRINT(_instance, _l, _func, _notifier, _msg, _file, _line)                            \
    do {                                                                                           \
        if (_instance.isEnabled(_l)) {                                                             \
            std::stringstream _str;                                                                \
            _instance.prefix(_str, _file, _line) << _msg;                                          \
            if (_instance.add_eol)                                                                 \
                _str << '\n';                                                                      \
            Base::Console()._func(_notifier, _str.str().c_str());                                  \
            if (_instance.refresh)                                                                 \
                Base::Console().refresh();                                                         \
        }                                                                                          \
    } while (0)

#define _FC_PRINT(_instance, _l, _func, _msg)                                                      \
    __FC_PRINT(_instance, _l, _func, std::string(), _msg, __FILE__, __LINE__)

#define FC_MSG(_msg) _FC_PRINT(FC_LOG_INSTANCE, FC_LOGLEVEL_MSG, message, _msg)
#define FC_WARN(_msg) _FC_PRINT(FC_LOG_INSTANCE, FC_LOGLEVEL_WARN, developerWarning, _msg)
#define FC_ERR(_msg) _FC_PRINT(FC_LOG_INSTANCE, FC_LOGLEVEL_ERR, developerError, _msg)
#define FC_LOG(_msg) _FC_PRINT(FC_LOG_INSTANCE, FC_LOGLEVEL_LOG, log, _msg)
#define FC_TRACE(_msg) _FC_PRINT(FC_LOG_INSTANCE, FC_LOGLEVEL_TRACE, log, _msg)

#define _FC_MSG(_file, _line, _msg)                                                                \
    __FC_PRINT(FC_LOG_INSTANCE, FC_LOGLEVEL_MSG, message, std::string(), _msg, _file, _line)
#define _FC_WARN(_file, _line, _msg)                                                               \
    __FC_PRINT(FC_LOG_INSTANCE, FC_LOGLEVEL_WARN, developerWarning, std::string(), _msg, _file, _line)
#define _FC_ERR(_file, _line, _msg)                                                                \
    __FC_PRINT(FC_LOG_INSTANCE, FC_LOGLEVEL_ERR, developerError, std::string(), _msg, _file, _line)
#define _FC_LOG(_file, _line, _msg)                                                                \
    __FC_PRINT(FC_LOG_INSTANCE, FC_LOGLEVEL_LOG, log, std::string(), _msg, _file, _line)
#define _FC_TRACE(_file, _line, _msg)                                                              \
    __FC_PRINT(FC_LOG_INSTANCE, FC_LOGLEVEL_TRACE, log, std::string(), _msg, _file, _line)

#define FC_XYZ(_pt) '(' << (_pt).X() << ", " << (_pt).Y() << ", " << (_pt).Z() << ')'
#define FC_xy(_pt) '(' << (_pt).x << ", " << (_pt).y << ')'
#define FC_xyz(_pt) '(' << (_pt).x << ", " << (_pt).y << ", " << (_pt).z << ')'

#ifndef FC_LOG_NO_TIMING
#define FC_TIME_CLOCK high_resolution_clock
#define FC_TIME_POINT std::chrono::FC_TIME_CLOCK::time_point
#define FC_DURATION std::chrono::duration<double>

#define _FC_TIME_INIT(_t) _t = std::chrono::FC_TIME_CLOCK::now()
#define FC_TIME_INIT(_t) FC_TIME_POINT _FC_TIME_INIT(_t)
#define FC_TIME_INIT2(_t1, _t2) FC_TIME_INIT(_t1), _t2 = _t1
#define FC_TIME_INIT3(_t1, _t2, _t3) FC_TIME_INIT(_t1), _t2 = _t1, _t3 = _t1

#define _FC_DURATION_PRINT(_l, _d, _msg) FC_##_l(_msg << " time: " << _d.count() << 's');

#define FC_DURATION_MSG(_d, _msg) _FC_DURATION_PRINT(MSG, _d, _msg)
#define FC_DURATION_LOG(_d, _msg) _FC_DURATION_PRINT(LOG, _d, _msg)
#define FC_DURATION_TRACE(_d, _msg) _FC_DURATION_PRINT(TRACE, _d, _msg)

#define _FC_TIME_PRINT(_l, _t, _msg) _FC_DURATION_PRINT(_l, Base::GetDuration(_t), _msg);

#define FC_TIME_MSG(_t, _msg) _FC_TIME_PRINT(MSG, _t, _msg)
#define FC_TIME_LOG(_t, _msg) _FC_TIME_PRINT(LOG, _t, _msg)
#define FC_TIME_TRACE(_t, _msg) _FC_TIME_PRINT(TRACE, _t, _msg)

#define FC_DURATION_DECLARE(_d) FC_DURATION _d
#define FC_DURATION_DECLARE2(_d, _d1) FC_DURATION_DECLARE(_d), _d1
#define FC_DURATION_DECLARE3(_d, _d1) FC_DURATION_DECLARE2(_d, _d1), _d2

#define FC_DURATION_INIT(_d) _d = FC_DURATION(0)
#define FC_DURATION_INIT2(_d, _d1) _d = _d1 = FC_DURATION(0)
#define FC_DURATION_INIT3(_d, _d1, _d2) _d = _d1 = _d2 = FC_DURATION(0)

#define FC_DURATION_DECL_INIT(_d) FC_DURATION _d(0)
#define FC_DURATION_DECL_INIT2(_d, _d1) FC_DURATION_DECL_INIT(_d), _d1(0)
#define FC_DURATION_DECL_INIT3(_d, _d1) FC_DURATION_DECL_INIT2(_d, _d1), _d3(0)

#define FC_DURATION_PLUS(_d, _t) _d += Base::GetDuration(_t)

#else  // FC_LOG_NO_TIMING
#define FC_TIME_POINT
#define _FC_TIME_INIT(...)                                                                         \
    do {                                                                                           \
    } while (0)
#define FC_TIME_INIT(...)                                                                          \
    do {                                                                                           \
    } while (0)
#define FC_TIME_INIT2(...)                                                                         \
    do {                                                                                           \
    } while (0)
#define FC_TIME_INIT3(...)                                                                         \
    do {                                                                                           \
    } while (0)
#define _FC_DURATION_PRINT(...)                                                                    \
    do {                                                                                           \
    } while (0)
#define _FC_TIME(_t)                                                                               \
    do {                                                                                           \
    } while (0)
#define FC_DURATION_PRINT(...)                                                                     \
    do {                                                                                           \
    } while (0)
#define FC_DURATION
#define FC_DURATION_INIT(...)                                                                      \
    do {                                                                                           \
    } while (0)
#define FC_DURATION_INIT1(...)                                                                     \
    do {                                                                                           \
    } while (0)
#define FC_DURATION_INIT2(...)                                                                     \
    do {                                                                                           \
    } while (0)
#define FC_DURATION_DECLARE(...)
#define FC_DURATION_DECLARE1(...)
#define FC_DURATION_DECLARE2(...)
#define FC_DURATION_DECL_INIT(...)                                                                 \
    do {                                                                                           \
    } while (0)
#define FC_DURATION_DECL_INIT2(...)                                                                \
    do {                                                                                           \
    } while (0)
#define FC_DURATION_DECL_INIT3(...)                                                                \
    do {                                                                                           \
    } while (0)
#define FC_DURATION_PLUS(...)                                                                      \
    do {                                                                                           \
    } while (0)

#endif  // FC_LOG_NO_TIMING
// clang-format on
// NOLINTEND(bugprone-reserved-identifier,bugprone-macro-parentheses,cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

using ConsoleMsgFlags = unsigned int;

namespace Base
{

#ifndef FC_LOG_NO_TIMING
inline FC_DURATION GetDuration(FC_TIME_POINT& tp)
{
    const auto tnow = std::chrono::FC_TIME_CLOCK::now();
    const auto dc = std::chrono::duration_cast<FC_DURATION>(tnow - tp);
    tp = tnow;
    return dc;
}
#endif

/** Used to identify log level*/
enum class LogStyle
{
    Warning,
    Message,
    Error,
    Log,
    Critical,      // Special message to mark critical notifications
    Notification,  // Special message for notifications to the user (e.g. educational)
};

/** Used to indicate the intended recipient of a message (warning, error, ...).
    While it is possible to create a custom message via Console().Send, console provides convenience
    functions for most common combinations.

    See documentation of ConsoleSingleton class for more details.

    @see ConsoleSingleton
    */
enum class IntendedRecipient
{
    All,        // All recipients, Developers and Users (One notification covers all)
    User,       // User only (notification intended only for a user)
    Developer,  // Developer only (notification intended only for a developer)
};

/** Used to indicate the translation state of a message (warning, error, ...).
    While it is possible to create a custom message via Console().Send, console provides convenience
    functions for most common combinations.

    See documentation of ConsoleSingleton class for more details.

    @see ConsoleSingleton
    */
enum class ContentType
{
    Untranslated,    // Not translated, but translatable
    Translated,      // Already translated
    Untranslatable,  // Cannot and should not be translated (Dynamic content, trace,...)
};

/** The Logger Interface
 *  This class describes an Interface for logging within FreeCAD. If you want to add a new
 *  "sink" to FreeCAD's logging mechanism, then inherit this class. You'll also need to
 *  register your derived class with ConsoleSingleton.
 *
 *  @see ConsoleSingleton
 */
class BaseExport ILogger
{
public:
    ILogger() = default;
    ILogger(const ILogger&) = delete;
    ILogger(ILogger&&) = delete;
    ILogger& operator=(const ILogger&) = delete;
    ILogger& operator=(ILogger&&) = delete;
    virtual ~ILogger() = 0;

    /** Used to send a Log message at the given level.
     * @param notifiername A string identifying the entity generating the notification.
     * It may be the Label of the App::Document or the full label of the App::DocumentObject.
     * @param msg The message to be notified.
     * @param level A valid level (Log, Message, Error, Notification, CriticalNotification, ...)
     * @param recipient A valid intended recipient (All, Developer, User).
     * Observers may decide to process only notifications when a certain recipient is intended.
     * @param content A valid content property (Untranslated, Translatable, Untranslatable).
     * Observers may decide not to process notifications if they are not translated or cannot be
     * translated (are untranslatable). Or conversely, may decide not to process already translated
     * notifications. It is up to the intended behaviour of the observer.
     */
    virtual void sendLog(
        const std::string& notifiername,
        const std::string& msg,
        LogStyle level,
        IntendedRecipient recipient,
        ContentType content
    ) = 0;

    /**
     * Returns whether a LogStyle category is active or not
     */
    bool isActive(const LogStyle category) const
    {
        switch (category) {
            case LogStyle::Log:
                return bLog;
            case LogStyle::Warning:
                return bWrn;
            case LogStyle::Error:
                return bErr;
            case LogStyle::Message:
                return bMsg;
            case LogStyle::Critical:
                return bCritical;
            case LogStyle::Notification:
                return bNotification;
        }

        return false;
    }

    virtual const char* name()
    {
        return nullptr;
    }
    bool bErr {true};
    bool bMsg {true};
    bool bLog {true};
    bool bWrn {true};
    bool bCritical {true};
    bool bNotification {false};
};


/** The console class
 *  This class manage all the stdio stuff. This includes
 *  Messages, Warnings, Log entries, Errors, Criticals, Notifications. The incoming Messages are
 *  distributed with the FCConsoleObserver. The FCConsole class itself makes no IO, it's more like
 *  a manager.
 *  \par
 *  ConsoleSingleton is a singleton! That means you can access the only
 *  instance of the class from every where in c++ by simply using:
 *  \code
 *  #include <Base/Console.h>
 *  Base::Console().log("Stage: %d",i);
 *  \endcode
 *  \par
 *  ConsoleSingleton is able to switch between several modes to, e.g. switch
 *  the logging on or off, or treat Warnings as Errors, and so on...
 *  @see ConsoleObserver
 *
 *  When sending error, warnings and notifications, preferably a notifier string should be provided,
 *  this enables the observer to identify the source. Any string is possible. It is common to use
 *  the DocumentObject::getFullLabel() or the label of the Document, when available.
 *  When using the Notification Framework of the Gui (Gui/Notifications.h) and a DocumentObject is
 *  passed as notifier, getFullLabel() is used. See example below.
 *
 *  It is important for the different observers to know whether an error/warning message is intended
 *  for a user, for a developer, or for both. This enables observers to provide distinctive
 *  behaviour. Similarly, different observers may treat differently messages if they carry
 *  translated, untranslated, or untranslatable messages.
 *
 *  While it is possible to create a tailored message, ConsoleSingleton provides convenience
 *  functions for most common messages. A custom message can be created as follows:
 *  \code
 *  Send<Base::LogStyle::Error,
 *       Base::IntendedRecipient::Developer,
 *       Base::ContentType::Untranslatable>("OCCT", e.what());
 *  \endcode
 *
 *  That code is equivalent to:
 *  \code
 *  DeveloperError("OCCT", e.what());
 *  \endcode
 *
 *  These convenience functions cover most common cases:
 *  - Unqualified convenience functions, such as error() and warning(), produce messages intended to
 *  both User and Developer with an untranslated message.
 *  - Functions qualified with Developer, such as DeveloperError are intended for a Developer and
 *  are untranslatable. Functions qualified with User, such as userError are intended only for the
 *  User and a untranslated (leaving the responsibility to the observer to find the translation).
 *  - Functions qualified with Translated, such as TranslatedError, are intended for the User and
 *  the message is already translated.
 *
 *  An observer receiving an Untranslatable or Translated message should not attempt to translate
 *  it.
 *
 *  However, an observer receiving a Translatable message may or may not attempt to translate
 *  it, e.g. depending on whether it intends to show the message to a Log or on the UI.
 *
 *  Observers shall use the QT translation context "Notifications" to attempt to translate an
 *  Untranslated message, if they need the localized version. Users shall mark Untranslated messages
 *  for translation.
 *
 *  Untranslated messages have the inherent advantage that can be processed in English by observers
 *  needing the English version, while enabling other observers to retrieve a translated version.
 *
 *  However, untranslated messages have the limitation that need to be known to the QT framework, so
 *  in practice, they need be static strings (as opposed to dynamically generated ones).
 *
 *  Example:
 *  \code
 *  Base::Console().userError(this->getFullName(), QT_TRANSLATE_NOOP("Notifications",
 *                            "Impossible to migrate Parabolas!!\n"));
 *  \endcode
 *
 *  Currently dynamically generated strings can only be properly translated at the source. This is
 *  often the case in legacy UI code, where localized strings are already available. For these
 *  cases the solution is to indicate the translated status. For example:
 *  \code
 *  Base::Console().translatedUserError(
 *                              this->getFullName(),
 *                              QObject::tr("The selected edge already has a Block constraint!"));
 *  \endcode
 *
 *  However, many of this legacy UI messages were previously a QMessageBox, and it is more
 *  convenient to use the Translations Framework provided by Gui/Notifications.h, as it takes the
 *  same arguments as QMessageBox. Additionally, the Notifications Framework takes into account the
 *  user preferences to receive intrusive (pop-ups) or non-intrusive (notification
 *  area) notifications.
 *
 */
class BaseExport ConsoleSingleton
{
public:
    // exported functions goes here +++++++++++++++++++++++++++++++++++++++

    /** Sends a message of type LogStyle (Message, Warning, Error, Log, Critical or Notification).
        This function is used by all specific convenience functions (Send(), Message(), Warning(),
       Error(), Log(), Critical and UserNotification, without or without notifier id).

        Notification can be direct or via queue.
    */
    template<LogStyle, IntendedRecipient = IntendedRecipient::All, ContentType = ContentType::Untranslated, typename... Args>
    void send(const std::string& notifiername, const char* pMsg, Args&&... args);

    /// Prints a Message
    template<typename... Args>
    void message(const char* pMsg, Args&&... args);
    /// Prints a warning Message
    template<typename... Args>
    void warning(const char* pMsg, Args&&... args);
    /// Prints a error Message
    template<typename... Args>
    void error(const char* pMsg, Args&&... args);
    /// Prints a log Message
    template<typename... Args>
    void log(const char* pMsg, Args&&... args);
    /// Prints a Critical Message
    template<typename... Args>
    void critical(const char* pMsg, Args&&... args);
    /// Sends a User Notification
    template<typename... Args>
    void userNotification(const char* pMsg, Args&&... args);
    /// Sends an already translated User Notification
    template<typename... Args>
    void userTranslatedNotification(const char* pMsg, Args&&... args);


    /// Prints a Message with source indication
    template<typename... Args>
    void message(const std::string& notifier, const char* pMsg, Args&&... args);
    /// Prints a warning Message with source indication
    template<typename... Args>
    void warning(const std::string& notifier, const char* pMsg, Args&&... args);
    template<typename... Args>
    void developerWarning(const std::string& notifier, const char* pMsg, Args&&... args);
    template<typename... Args>
    void userWarning(const std::string& notifier, const char* pMsg, Args&&... args);
    template<typename... Args>
    void translatedUserWarning(const std::string& notifier, const char* pMsg, Args&&... args);
    /// Prints a error Message with source indication
    template<typename... Args>
    void error(const std::string& notifier, const char* pMsg, Args&&... args);
    template<typename... Args>
    void developerError(const std::string& notifier, const char* pMsg, Args&&... args);
    template<typename... Args>
    /// A noexcept DeveloperError for use in destructors. When compiled in debug, terminates via an
    /// assert. In release, the exception is silently caught and dropped.
    void destructorError(const std::string& notifier, const char* pMsg, Args&&... args) noexcept;
    template<typename... Args>
    void userError(const std::string& notifier, const char* pMsg, Args&&... args);
    template<typename... Args>
    void translatedUserError(const std::string& notifier, const char* pMsg, Args&&... args);
    /// Prints a log Message with source indication
    template<typename... Args>
    void log(const std::string& notifier, const char* pMsg, Args&&... args);
    /// Prints a Critical Message with source indication
    template<typename... Args>
    void critical(const std::string& notifier, const char* pMsg, Args&&... args);
    /// Sends a User Notification with source indication
    template<typename... Args>
    void userNotification(const std::string& notifier, const char* pMsg, Args&&... args);
    /// Sends an already translated User Notification with source indication
    template<typename... Args>
    void userTranslatedNotification(const std::string& notifier, const char* pMsg, Args&&... args);

    // Notify a message directly to observers
    template<LogStyle, IntendedRecipient = IntendedRecipient::All, ContentType = ContentType::Untranslated>
    void notify(const std::string& notifiername, const std::string& msg);

    /// Attaches an Observer to FCConsole
    void attachObserver(ILogger* pcObserver);
    /// Detaches an Observer from FCConsole
    void detachObserver(ILogger* pcObserver);

    /// enumeration for the console modes
    enum ConsoleMode
    {
        Verbose = 1,  // suppress Log messages
    };
    enum ConnectionMode
    {
        Direct = 0,
        Queued = 1
    };

    enum FreeCAD_ConsoleMsgType
    {
        MsgType_Txt = 1,
        MsgType_Log = 2,  // ConsoleObserverStd sends this and higher to stderr
        MsgType_Wrn = 4,
        MsgType_Err = 8,
        MsgType_Critical = 16,      // Special message to notify critical information
        MsgType_Notification = 32,  // Special message to for notifications to the user
    };

    /// Enables or disables message types of a certain console observer
    ConsoleMsgFlags setEnabledMsgType(const char* sObs, ConsoleMsgFlags type, bool on) const;
    /// Checks if message types of a certain console observer are enabled
    bool isMsgTypeEnabled(const char* sObs, FreeCAD_ConsoleMsgType type) const;
    void setConnectionMode(ConnectionMode mode);

    int* getLogLevel(const char* tag, bool create = true);

    void setDefaultLogLevel(const int level)
    {
        _defaultLogLevel = level;
    }

    int logLevel(const int level) const
    {
        return level < 0 ? _defaultLogLevel : level;
    }

    /// singleton
    static ConsoleSingleton& instance();

    // retrieval of an observer by name
    ILogger* get(const char* Name) const;

    static PyMethodDef Methods[];

    void refresh() const;
    void enableRefresh(bool enable);

    constexpr FreeCAD_ConsoleMsgType getConsoleMsg(LogStyle style);

private:
    // python exports goes here +++++++++++++++++++++++++++++++++++++++++++
    // static python wrapper of the exported functions
    static PyObject* sPyLog(PyObject* self, PyObject* args);
    static PyObject* sPyMessage(PyObject* self, PyObject* args);
    static PyObject* sPyWarning(PyObject* self, PyObject* args);
    static PyObject* sPyDeveloperWarning(PyObject* self, PyObject* args);
    static PyObject* sPyUserWarning(PyObject* self, PyObject* args);
    static PyObject* sPyTranslatedUserWarning(PyObject* self, PyObject* args);
    static PyObject* sPyError(PyObject* self, PyObject* args);
    static PyObject* sPyDeveloperError(PyObject* self, PyObject* args);
    static PyObject* sPyUserError(PyObject* self, PyObject* args);
    static PyObject* sPyTranslatedUserError(PyObject* self, PyObject* args);
    static PyObject* sPyCritical(PyObject* self, PyObject* args);
    static PyObject* sPyNotification(PyObject* self, PyObject* args);
    static PyObject* sPyTranslatedNotification(PyObject* self, PyObject* args);
    static PyObject* sPySetStatus(PyObject* self, PyObject* args);
    static PyObject* sPyGetStatus(PyObject* self, PyObject* args);
    static PyObject* sPyGetObservers(PyObject* self, PyObject* args);

    bool _bCanRefresh {true};
    ConnectionMode connectionMode {Direct};

    // Singleton!
    ConsoleSingleton();
    ~ConsoleSingleton();

public:
    ConsoleSingleton(const ConsoleSingleton&) = delete;
    ConsoleSingleton(ConsoleSingleton&&) = delete;
    ConsoleSingleton& operator=(const ConsoleSingleton&) = delete;
    ConsoleSingleton& operator=(ConsoleSingleton&&) = delete;

private:
    void postEvent(
        FreeCAD_ConsoleMsgType type,
        IntendedRecipient recipient,
        ContentType content,
        const std::string& notifiername,
        const std::string& msg
    );
    void notifyPrivate(
        LogStyle category,
        IntendedRecipient recipient,
        ContentType content,
        const std::string& notifiername,
        const std::string& msg
    ) const;

    // singleton
    static void Destruct();
    static ConsoleSingleton* _pcSingleton;  // NOLINT

    // observer list
    std::set<ILogger*> _aclObservers;

    std::map<std::string, int> _logLevels;
    int _defaultLogLevel;

    friend class ConsoleOutput;
};

/** Access to the Console
 *  This method is used to gain access to the one and only instance of
 *  the ConsoleSingleton class.
 */
inline ConsoleSingleton& Console()
{
    return ConsoleSingleton::instance();
}

constexpr ConsoleSingleton::FreeCAD_ConsoleMsgType ConsoleSingleton::getConsoleMsg(LogStyle style)
{
    constexpr std::array msgTypes {
        // In order of LogStyle
        MsgType_Wrn,
        MsgType_Txt,
        MsgType_Err,
        MsgType_Log,
        MsgType_Critical,
        MsgType_Notification
    };

    return msgTypes.at(static_cast<std::size_t>(style));
}

class BaseExport ConsoleRefreshDisabler
{
public:
    ConsoleRefreshDisabler()
    {
        Console().enableRefresh(false);
    }

    ~ConsoleRefreshDisabler()
    {
        Console().enableRefresh(true);
    }

    ConsoleRefreshDisabler(const ConsoleRefreshDisabler&) = delete;
    ConsoleRefreshDisabler(ConsoleRefreshDisabler&&) = delete;
    ConsoleRefreshDisabler& operator=(const ConsoleRefreshDisabler&) = delete;
    ConsoleRefreshDisabler& operator=(ConsoleRefreshDisabler&&) = delete;
};


/** LogLevel helper class */
class BaseExport LogLevel
{
public:
    std::string tag;
    int& lvl;
    bool print_tag;
    int print_src;
    bool print_time;
    bool add_eol;
    bool refresh;

    LogLevel(
        const char* tag,
        const bool print_tag = true,
        const int print_src = 0,
        const bool print_time = false,
        const bool add_eol = true,
        const bool refresh = false
    )
        : tag(tag)
        , lvl(*Console().getLogLevel(tag))
        , print_tag(print_tag)
        , print_src(print_src)
        , print_time(print_time)
        , add_eol(add_eol)
        , refresh(refresh)
    {}

    bool isEnabled(const int lev) const
    {
        return lev <= level();
    }

    int level() const
    {
        return Console().logLevel(lvl);
    }

    std::stringstream& prefix(std::stringstream& str, const char* src, int line);
};
}  // namespace Base

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
 *  @see Critical
 *  @see UserNotification
 *  @see UserTranslatedNotification
 */
template<typename... Args>
void Base::ConsoleSingleton::message(const char* pMsg, Args&&... args)
{
    message(std::string(""), pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::message(const std::string& notifier, const char* pMsg, Args&&... args)
{
    send<LogStyle::Message>(notifier, pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::warning(const char* pMsg, Args&&... args)
{
    warning(std::string(""), pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::warning(const std::string& notifier, const char* pMsg, Args&&... args)
{
    send<LogStyle::Warning>(notifier, pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::developerWarning(const std::string& notifier, const char* pMsg, Args&&... args)
{
    send<LogStyle::Warning, IntendedRecipient::Developer, ContentType::Untranslatable>(
        notifier,
        pMsg,
        std::forward<Args>(args)...
    );
}

template<typename... Args>
void Base::ConsoleSingleton::userWarning(const std::string& notifier, const char* pMsg, Args&&... args)
{
    send<LogStyle::Warning, IntendedRecipient::User, ContentType::Untranslated>(
        notifier,
        pMsg,
        std::forward<Args>(args)...
    );
}

template<typename... Args>
void Base::ConsoleSingleton::translatedUserWarning(
    const std::string& notifier,
    const char* pMsg,
    Args&&... args
)
{
    send<LogStyle::Warning, IntendedRecipient::User, ContentType::Translated>(
        notifier,
        pMsg,
        std::forward<Args>(args)...
    );
}

template<typename... Args>
void Base::ConsoleSingleton::error(const char* pMsg, Args&&... args)
{
    error(std::string(""), pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::error(const std::string& notifier, const char* pMsg, Args&&... args)
{
    send<LogStyle::Error>(notifier, pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::developerError(const std::string& notifier, const char* pMsg, Args&&... args)
{
    send<LogStyle::Error, IntendedRecipient::Developer, ContentType::Untranslatable>(
        notifier,
        pMsg,
        std::forward<Args>(args)...
    );
}

template<typename... Args>
void Base::ConsoleSingleton::destructorError(
    const std::string& notifier,
    const char* pMsg,
    Args&&... args
) noexcept
{
    try {
        send<LogStyle::Error, IntendedRecipient::Developer, ContentType::Untranslatable>(
            notifier,
            pMsg,
            std::forward<Args>(args)...
        );
    }
    catch (...) {
        assert("An exception was thrown while attempting console output in a destructor" && false);
    }
}

template<typename... Args>
void Base::ConsoleSingleton::userError(const std::string& notifier, const char* pMsg, Args&&... args)
{
    send<LogStyle::Error, IntendedRecipient::User, ContentType::Untranslated>(
        notifier,
        pMsg,
        std::forward<Args>(args)...
    );
}

template<typename... Args>
void Base::ConsoleSingleton::translatedUserError(
    const std::string& notifier,
    const char* pMsg,
    Args&&... args
)
{
    send<LogStyle::Error, IntendedRecipient::User, ContentType::Translated>(
        notifier,
        pMsg,
        std::forward<Args>(args)...
    );
}

template<typename... Args>
void Base::ConsoleSingleton::critical(const char* pMsg, Args&&... args)
{
    critical(std::string(""), pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::critical(const std::string& notifier, const char* pMsg, Args&&... args)
{
    send<LogStyle::Critical>(notifier, pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::userNotification(const char* pMsg, Args&&... args)
{
    userNotification(std::string(""), pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::userNotification(const std::string& notifier, const char* pMsg, Args&&... args)
{
    send<LogStyle::Notification, IntendedRecipient::User, ContentType::Untranslated>(
        notifier,
        pMsg,
        std::forward<Args>(args)...
    );
}

template<typename... Args>
void Base::ConsoleSingleton::userTranslatedNotification(const char* pMsg, Args&&... args)
{
    userTranslatedNotification(std::string(""), pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::userTranslatedNotification(
    const std::string& notifier,
    const char* pMsg,
    Args&&... args
)
{
    send<LogStyle::Notification, IntendedRecipient::User, ContentType::Translated>(
        notifier,
        pMsg,
        std::forward<Args>(args)...
    );
}

template<typename... Args>
void Base::ConsoleSingleton::log(const char* pMsg, Args&&... args)
{
    log(std::string(""), pMsg, std::forward<Args>(args)...);
}

template<typename... Args>
void Base::ConsoleSingleton::log(const std::string& notifier, const char* pMsg, Args&&... args)
{
    send<LogStyle::Log>(notifier, pMsg, std::forward<Args>(args)...);
}

template<
    Base::LogStyle category,
    Base::IntendedRecipient recipient /*= Base::IntendedRecipient::All*/,
    Base::ContentType contenttype /*= Base::ContentType::Untranslated*/,
    typename... Args>
void Base::ConsoleSingleton::send(const std::string& notifiername, const char* pMsg, Args&&... args)
{
    std::string format;
    try {
        format = fmt::sprintf(pMsg, args...);
    }
    catch (fmt::format_error& e) {
        // We can't allow an exception to propagate out of this method, which gets used in some
        // destructors. Instead, make the string's contents the error message that fmt::sprintf gave
        // us.
        format = std::string("ERROR: Invalid format string or arguments provided.\n");
        format += e.what();
    }

    if (connectionMode == Direct) {
        notify<category, recipient, contenttype>(notifiername, format);
    }
    else {

        const auto type = getConsoleMsg(category);

        postEvent(type, recipient, contenttype, notifiername, format);
    }
}

template<
    Base::LogStyle category,
    Base::IntendedRecipient recipient /*= Base::IntendedRecipient::All*/,
    Base::ContentType contenttype /*= Base::ContentType::Untranslated*/>
void Base::ConsoleSingleton::notify(const std::string& notifiername, const std::string& msg)
{
    notifyPrivate(category, recipient, contenttype, notifiername, msg);
}

#if defined(__clang__)
# pragma clang diagnostic pop
#endif
