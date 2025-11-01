/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Interface_Static.hxx>
#endif

#include "ImportExportSettings.h"
#include <App/Application.h>


namespace Part {
namespace GLTF {

ImportExportSettings::ImportExportSettings()
{
    pGroup = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part/glTF");
}

void ImportExportSettings::setVisibleExportDialog(bool on)
{
    pGroup->SetBool("VisibleExportDialog", on);
}

bool ImportExportSettings::isVisibleExportDialog() const
{
    return pGroup->GetBool("VisibleExportDialog", true);
}

void ImportExportSettings::setVisibleImportDialog(bool on)
{
    pGroup->SetBool("VisibleImportDialog", on);
}

bool ImportExportSettings::isVisibleImportDialog() const
{
    return pGroup->GetBool("VisibleImportDialog", true);
}

bool ImportExportSettings::isTessellationOnly() const
{
    return pGroup->GetBool("MeshOnly", true);
}

void ImportExportSettings::setTessellationOnly(bool on)
{
    pGroup->SetBool("MeshOnly", on);
}

bool ImportExportSettings::getRefinement() const
{
    return pGroup->GetBool("Refine", false);
}

void ImportExportSettings::setRefinement(bool on)
{
    pGroup->SetBool("Refine", on);
}

bool ImportExportSettings::getSkipEmptyNodes() const
{
    return pGroup->GetBool("SkipEmptyNodes", true);
}

void ImportExportSettings::setSkipEmptyNodes(bool on)
{
    pGroup->SetBool("SkipEmptyNodes", on);
}

bool ImportExportSettings::getDoublePrecision() const
{
    return pGroup->GetBool("DoublePrecision", false);
}

void ImportExportSettings::setDoublePrecision(bool on)
{
    pGroup->SetBool("DoublePrecision", on);
}

bool ImportExportSettings::getLoadAllScenes() const
{
    return pGroup->GetBool("LoadAllScenes", false);
}

void ImportExportSettings::setLoadAllScenes(bool on)
{
    pGroup->SetBool("LoadAllScenes", on);
}

bool ImportExportSettings::getMultiThreadedExport() const
{
    return pGroup->GetBool("MultiThreadedExport", false);
}

void ImportExportSettings::setMultiThreadedExport(bool on)
{
    pGroup->SetBool("MultiThreadedExport", on);
}

bool ImportExportSettings::getMultiThreadedImport() const
{
    return pGroup->GetBool("MultiThreadedImport", false);
}

void ImportExportSettings::setMultiThreadedImport(bool on)
{
    pGroup->SetBool("MultiThreadedImport", on);
}

bool ImportExportSettings::getPrintDebugMessages() const
{
    return pGroup->GetBool("PrintDebug", false);
}

void ImportExportSettings::setPrintDebugMessages(bool on)
{
    pGroup->SetBool("PrintDebug", on);
}

bool ImportExportSettings::getUVCoords() const
{
    return pGroup->GetBool("ExportUVCoords", false);
}

void ImportExportSettings::setUVCoords(bool on)
{
    pGroup->SetBool("ExportUVCoords", on);
}

bool ImportExportSettings::getMergeFaces() const
{
    return pGroup->GetBool("MergeFaces", false);
}

void ImportExportSettings::setMergeFaces(bool on)
{
    pGroup->SetBool("MergeFaces", on);
}

} // namespace IGES
} // namespace Part
