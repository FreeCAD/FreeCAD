// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <memory>

#include <Message_ProgressIndicator.hxx>
#include <Standard_Version.hxx>

#include <Base/Sequencer.h>
#include <Mod/Part/PartGlobal.h>


namespace Part
{

class PartExport ProgressIndicator: public Message_ProgressIndicator
{
public:
    ProgressIndicator();
    ~ProgressIndicator() override;

    void Show(const Message_ProgressScope& theScope, const Standard_Boolean isForce) override;
    Standard_Boolean UserBreak() override;
    void Reset() override;

private:
    std::size_t currentStep {0};
    std::unique_ptr<Base::SequencerLauncher> progress;
};

}  // namespace Part
