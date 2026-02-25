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


#pragma once


#include <Base/Console.h>

namespace Gui
{

/** The console window class
 *  This class opens a console window when instantiated
 *  and redirects the stdio streams to it as long it exists.
 *  This is for Windows only!
 *  After instantiation it automatically registers itself at
 *  the FCConsole class and gets all the FCConsoleObserver
 *  messages. The class must not used directly! Only the
 *  FCConsole class is allowed!
 *  @see FCConsole
 *  \author Jürgen Riegel
 */
class GuiExport GUIConsole: public Base::ILogger
{
public:
    /// Constructor
    GUIConsole();
    /// Destructor
    ~GUIConsole() override;
    void sendLog(
        const std::string& notifiername,
        const std::string& msg,
        Base::LogStyle level,
        Base::IntendedRecipient recipient,
        Base::ContentType content
    ) override;
    const char* name() override
    {
        return "GUIConsole";
    }

protected:
    static const unsigned int s_nMaxLines;
    static unsigned int s_nRefCount;
};

}  // namespace Gui
