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

void ConsoleObserverFile::SendLog(const std::string& msg, LogStyle level)
{
    std::map<LogStyle, std::string> prefixes{
        {LogStyle::Warning, "Wrn: "},
        {LogStyle::Message, "Msg: "},
        {LogStyle::Error, "Err: "},
        {LogStyle::Log, "Log: "}};

    cFileStream << prefixes[level] << msg;
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

template<typename T>
auto colorize(T msgType, const char *msg, FILE* dest)
{
    if constexpr (sysType == SysTypes::Windows) {
        std::map<LogStyle, int> formats{
            {LogStyle::Warning, fGreen | fBlue},
            {LogStyle::Error, fRed | fIntensity},
            {LogStyle::Log, fRed | fGreen}};
        setConsoleTextAttr(formats[msgType]);
        fprintf(dest, "%s", msg);
        setConsoleTextAttr(fRed | fGreen | fBlue);
    }
    else {
        if constexpr (isLinuxBased) {
            std::map<LogStyle, char *> formats{
                {LogStyle::Warning, "\033[1;33m"},
                {LogStyle::Error, "\033[1;31m"},
                {LogStyle::Log, "\033[1;36m"},
            };
            fprintf(dest, "%s", formats[msgType]);
            fprintf(dest, "%s", msg);
            fprintf(dest, "%s", "\033[0m");
        }
        else {
            fprintf(dest, "%s", msg);
        }
    }
}

void ConsoleObserverStd::SendLog(const std::string &msg, LogStyle level)
{
    if (level == LogStyle::Message) {
        printf("%s", msg.c_str());
        return;
    }
    if( ! useColorStderr){
        fprintf(stderr, "%s", msg.c_str());
        return;
    }
    colorize(level, msg.c_str(), stderr);
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
