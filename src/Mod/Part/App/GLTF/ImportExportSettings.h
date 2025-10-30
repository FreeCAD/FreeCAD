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

#ifndef PART_GLTF_IMPORTEXPORTSETTINGS_H
#define PART_GLTF_IMPORTEXPORTSETTINGS_H

#include <Mod/Part/App/Interface.h>
#include <Base/Parameter.h>


namespace Part
{

namespace GLTF {

class PartExport ImportExportSettings
{
public:
    ImportExportSettings();

    void setVisibleExportDialog(bool);
    bool isVisibleExportDialog() const;

    void setVisibleImportDialog(bool);
    bool isVisibleImportDialog() const;

    bool getRefinement() const;
    void setRefinement(bool) const;

    bool getSkipEmptyNodes() const;
    void setSkipEmptyNodes(bool) const;

    bool getDoublePrecision() const;
    void setDoublePrecision(bool) const;

    bool getLoadAllScenes() const;
    void setLoadAllScenes(bool) const;

    bool getMultiThreadedExport() const;
    void setMultiThreadedExport(bool) const;

    bool getMultiThreadedImport() const;
    void setMultiThreadedImport(bool) const;

    bool getPrintDebugMessages() const;
    void setPrintDebugMessages(bool) const;

    bool getUVCoords() const;
    void setUVCoords(bool) const;

    bool getMergeFaces() const;
    void setMergeFaces(bool) const;

private:
    ParameterGrp::handle pGroup;
};

} //namespace GLTF
} //namespace Part

#endif
