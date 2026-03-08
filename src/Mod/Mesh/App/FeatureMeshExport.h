// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

#include <Mod/Mesh/MeshGlobal.h>

namespace Mesh
{

/**
 * The Export class writes any supported mesh format into a file.
 * @author Werner Mayer
 */
class MeshExport Export: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Mesh::Export);

public:
    Export();

    App::PropertyLink Source;
    App::PropertyString FileName;
    App::PropertyString Format;
    const char* getViewProviderName() const override
    {
        return "MeshGui::ViewProviderExport";
    }

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    //@}
};

}  // namespace Mesh
