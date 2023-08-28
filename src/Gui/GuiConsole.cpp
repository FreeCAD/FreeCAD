/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# ifdef FC_OS_WIN32
#   include "io.h"
#   include <windows.h>
# endif
# include <fcntl.h>
# include <iostream>
#endif

#include "GuiConsole.h"

using namespace Gui;

#ifdef FC_OS_WIN32

const unsigned int GUIConsole::s_nMaxLines = 1000;
unsigned int       GUIConsole::s_nRefCount = 0;

/** Constructor
 *  Open a Top level Window and redirect the
 *  stdin, stdout and stderr stream to it.
 *  Not needed in Linux!
 */
GUIConsole::GUIConsole (void)
{
  if (!s_nRefCount++)
  {
    bLog = false;

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    ::AllocConsole();

    ::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE),&csbi);
     csbi.dwSize.Y = s_nMaxLines;
    ::SetConsoleScreenBufferSize(::GetStdHandle(STD_OUTPUT_HANDLE),csbi.dwSize);
    ::SetConsoleTitleA( "FreeCAD Console");

    *stdout = *::_fdopen(::_open_osfhandle(reinterpret_cast<intptr_t>(::GetStdHandle(STD_OUTPUT_HANDLE)), _O_TEXT), "w");
    ::setvbuf(stdout, 0, _IONBF, 0);

    *stdin = *::_fdopen(::_open_osfhandle(reinterpret_cast<intptr_t>(::GetStdHandle(STD_INPUT_HANDLE)), _O_TEXT), "r");
    ::setvbuf(stdin, 0, _IONBF, 0);

    *stderr = *::_fdopen(::_open_osfhandle(reinterpret_cast<intptr_t>(::GetStdHandle(STD_ERROR_HANDLE)), _O_TEXT), "w");
    ::setvbuf(stderr, 0, _IONBF, 0);
  }
}

/** Destructor
 *  Close the window and redirect the streams back
 */
GUIConsole::~GUIConsole (void)
{
  if (!--s_nRefCount)
      FreeConsole();
}

void GUIConsole::SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level,
                         Base::IntendedRecipient recipient, Base::ContentType content)
{
    (void) notifiername;

    // Do not log translated messages, or messages intended only to the user to std log
    if(recipient == Base::IntendedRecipient::User || content == Base::ContentType::Translated)
        return;

    int color = -1;
    switch(level){
        case Base::LogStyle::Warning:
            color = FOREGROUND_RED | FOREGROUND_GREEN;
            break;
        case Base::LogStyle::Message:
            color = FOREGROUND_GREEN;
            break;
        case Base::LogStyle::Error:
            color = FOREGROUND_RED;
            break;
        case Base::LogStyle::Log:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            break;
        case Base::LogStyle::Critical:
            color = FOREGROUND_RED | FOREGROUND_GREEN;
            break;
        default:
            break;
    }

    ::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), color);
    printf("%s", msg.c_str());
    ::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
}

#else /* FC_OS_LINUX */

// safely ignore GUIConsole::s_nMaxLines and  GUIConsole::s_nRefCount
GUIConsole::GUIConsole () = default;
GUIConsole::~GUIConsole () = default;
void GUIConsole::SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level,
                         Base::IntendedRecipient recipient, Base::ContentType content)
{
    (void) notifiername;

    // Do not log translated messages, or messages intended only to the user to std log
    if(recipient == Base::IntendedRecipient::User || content == Base::ContentType::Translated)
        return;

    switch(level){
        case Base::LogStyle::Warning:
            std::cerr << "Warning: " << msg;
            break;
        case Base::LogStyle::Message:
            std::cout << msg;
            break;
        case Base::LogStyle::Error:
            std::cerr << "Error: " << msg;
            break;
        case Base::LogStyle::Log:
            std::clog << msg;
            break;
        case Base::LogStyle::Critical:
            std::cout << "Critical: " << msg;
            break;
        default:
            break;
    }
}

#endif /* FC_OS_LINUX */
