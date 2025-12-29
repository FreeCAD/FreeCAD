/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Developers                                 *
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

#ifndef EXPORT3DPDF_COMMAND_H
#define EXPORT3DPDF_COMMAND_H

#include <Gui/Command.h>

class StdCmdPrint3dPdf : public Gui::Command
{
public:
    StdCmdPrint3dPdf();
    virtual ~StdCmdPrint3dPdf() = default;
    
    virtual const char* className() const override { return "StdCmdPrint3dPdf"; }
    
protected:
    virtual void activated(int iMsg) override;
    virtual bool isActive() override;
};

#endif // EXPORT3DPDF_COMMAND_H 