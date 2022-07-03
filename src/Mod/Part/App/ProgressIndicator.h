/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PART_PROGRESSINDICATOR_H
#define PART_PROGRESSINDICATOR_H

#include <memory>

#include <Message_ProgressIndicator.hxx>
#include <Standard_Version.hxx>

#include <Base/Sequencer.h>
#include <Mod/Part/PartGlobal.h>


namespace Part {

#if OCC_VERSION_HEX < 0x070500
class PartExport ProgressIndicator : public Message_ProgressIndicator
{
public:
    ProgressIndicator (int theMaxVal = 100);
    virtual ~ProgressIndicator ();

    virtual Standard_Boolean Show (const Standard_Boolean theForce = Standard_True);
    virtual Standard_Boolean UserBreak();

private:
    std::unique_ptr<Base::SequencerLauncher> myProgress;
};
#endif

}

#endif // PART_PROGRESSINDICATOR_H
