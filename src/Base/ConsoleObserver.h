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




#ifndef BASE_CONSOLEOBSERVER_H
#define BASE_CONSOLEOBSERVER_H

#include <Base/Console.h>
#include <Base/Stream.h>

namespace Base {

//=========================================================================
// some special observers

/** The LoggingConsoleObserver class
 *  This class is used by the main modules to write Console messages and logs to a file
 */
class BaseExport ConsoleObserverFile : public ILogger
{
public:
    ConsoleObserverFile(const char *sFileName);
    ~ConsoleObserverFile() override;

    void SendLog(const std::string& message, LogStyle level) override;
    const char* Name(void) override {return "File";}

protected:
    Base::ofstream cFileStream;
};

/** The CmdConsoleObserver class
 *  This class is used by the main modules to write Console messages and logs the system con.
 */
class BaseExport ConsoleObserverStd: public ILogger
{
public:
    ConsoleObserverStd();
    ~ConsoleObserverStd() override;
    void SendLog(const std::string& message, LogStyle level) override;
    const char* Name(void) override {return "Console";}
protected:
    bool useColorStderr;
private:
    void Warning(const char *sWarn);
    void Message(const char *sMsg);
    void Error  (const char *sErr);
    void Log    (const char *sErr);
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

#endif // BASE_CONSOLEOBSERVER_H
