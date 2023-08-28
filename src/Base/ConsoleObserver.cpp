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
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# include <cstring>
# include <Python.h>
#endif

#include <frameobject.h>

#include "ConsoleObserver.h"
#include "Interpreter.h"


using namespace Base;


//=========================================================================
// some special observers

ConsoleObserverFile::ConsoleObserverFile(const char *sFileName)
  : cFileStream(Base::FileInfo(sFileName)) // can be in UTF8
{
    if (!cFileStream.is_open())
        Console().Warning("Cannot open log file '%s'.\n", sFileName);
    // mark the file as a UTF-8 encoded file
    unsigned char bom[3] = {0xef, 0xbb, 0xbf};
    cFileStream.write(reinterpret_cast<const char*>(bom), 3*sizeof(char));
}

ConsoleObserverFile::~ConsoleObserverFile()
{
    cFileStream.close();
}

void ConsoleObserverFile::SendLog(const std::string& notifiername, const std::string& msg, LogStyle level,
                                  IntendedRecipient recipient, ContentType content)
{
    (void) notifiername;

    // Do not log translated messages, or messages intended only to the user to log file
    if(recipient == IntendedRecipient::User || content == ContentType::Translated)
        return;
    
    std::string prefix;
    switch(level){
        case LogStyle::Warning:
            prefix = "Wrn: ";
            break;
        case LogStyle::Message:
            prefix = "Msg: ";
            break;
        case LogStyle::Error:
            prefix = "Err: ";
            break;
        case LogStyle::Log:
            prefix = "Log: ";
            break;
        case LogStyle::Critical:
            prefix = "Critical: ";
            break;
        default:
            break;
    }

    cFileStream << prefix << msg;
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

ConsoleObserverStd::~ConsoleObserverStd() = default;

void ConsoleObserverStd::SendLog(const std::string& notifiername, const std::string& msg, LogStyle level,
                                 IntendedRecipient recipient, ContentType content)
{
    (void) notifiername;

    // Do not log translated messages, or messages intended only to the user to std log
    if(recipient == IntendedRecipient::User || content == ContentType::Translated)
        return;
    
    switch(level){
        case LogStyle::Warning:
            this->Warning(msg.c_str());
            break;
        case LogStyle::Message:
            this->Message(msg.c_str());
            break;
        case LogStyle::Error:
            this->Error(msg.c_str());
            break;
        case LogStyle::Log:
            this->Log(msg.c_str());
            break;
        case LogStyle::Critical:
            this->Critical(msg.c_str());
            break;
        default:
            break;
    }
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

void ConsoleObserverStd::Critical(const char *sCritical)
{
    if (useColorStderr) {
        #   if defined(FC_OS_WIN32)
        ::SetConsoleTextAttribute(::GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE);
        #   elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        fprintf(stderr, "\033[1;33m");
        #   endif
    }

    fprintf(stderr, "%s", sCritical);

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
        buffer.push_back(static_cast<char>(c));
    return c;
}

int RedirectStdOutput::sync()
{
    // Print as log as this might be verbose
    if (!buffer.empty() && buffer.back() == '\n') {
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
        buffer.push_back(static_cast<char>(c));
    return c;
}

int RedirectStdLog::sync()
{
    // Print as log as this might be verbose
    if (!buffer.empty() && buffer.back() == '\n') {
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
        buffer.push_back(static_cast<char>(c));
    return c;
}

int RedirectStdError::sync()
{
    if (!buffer.empty() && buffer.back() == '\n') {
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
    if (print_time) {
        if (!s_timing) {
            s_timing = true;
            _FC_TIME_INIT(s_tstart);
        }
        auto tnow = std::chrono::FC_TIME_CLOCK::now();
        auto d = std::chrono::duration_cast<FC_DURATION>(tnow-s_tstart);
        str << d.count() << ' ';
    }
    if (print_tag) str << '<' << tag << "> ";
    if (print_src==2) {
        Base::PyGILStateLocker lock;
        PyFrameObject* frame = PyEval_GetFrame();
        if (frame) {
            line = PyFrame_GetLineNumber(frame);
#if PY_VERSION_HEX < 0x030b0000
            src = PyUnicode_AsUTF8(frame->f_code->co_filename);
#else
            PyCodeObject* code = PyFrame_GetCode(frame);
            src = PyUnicode_AsUTF8(code->co_filename);
            Py_DECREF(code);
#endif
        }
    }
    if (print_src && src && src[0]) {
#ifdef FC_OS_WIN32
        const char *_f = std::strrchr(src, '\\');
#else
        const char *_f = std::strrchr(src, '/');
#endif
        str << (_f?_f+1:src)<<"("<<line<<"): ";
    }
    return str;
}
