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

#include "PreCompiled.h"

#include "FeatureMeshExport.h"
#include "MeshFeature.h"


using namespace Mesh;
using namespace MeshCore;

PROPERTY_SOURCE(Mesh::Export, App::DocumentObject)

Export::Export()
{
    ADD_PROPERTY(Source, (nullptr));
    ADD_PROPERTY(FileName, (""));
    ADD_PROPERTY(Format, (""));
}

short Export::mustExecute() const
{
    if (Source.getValue()) {
        if (Source.isTouched()) {
            return 1;
        }
        if (FileName.isTouched()) {
            return 1;
        }
        if (Format.isTouched()) {
            return 1;
        }
    }
    return 0;
}

App::DocumentObjectExecReturn* Export::execute()
{
    Mesh::Feature* pcFeat = dynamic_cast<Mesh::Feature*>(Source.getValue());
    if (!pcFeat || pcFeat->isError()) {
        return new App::DocumentObjectExecReturn("Cannot export invalid mesh feature");
    }

    pcFeat->Mesh.getValue().save(FileName.getValue());
    return App::DocumentObject::StdReturn;
}
