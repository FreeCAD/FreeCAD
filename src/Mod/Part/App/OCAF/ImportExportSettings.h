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

#ifndef PART_OCAF_IMPORTEXPORTSETTINGS_H
#define PART_OCAF_IMPORTEXPORTSETTINGS_H

#include <memory>
#include <Mod/Part/App/Interface.h>
#include <Base/Parameter.h>


namespace Part
{

namespace STEP {
class ImportExportSettings;
using ImportExportSettingsPtr = std::shared_ptr<ImportExportSettings>;
}

namespace IGES {
class ImportExportSettings;
using ImportExportSettingsPtr = std::shared_ptr<ImportExportSettings>;
}

namespace OCAF
{

class PartExport ImportExportSettings
{
public:
    enum class ImportMode {
        SingleDocument = 0,
        GroupPerDocument = 1,
        GroupPerDirectory = 2,
        ObjectPerDocument = 3,
        ObjectPerDirectory = 4,
    };

    static void initialize();
    ImportExportSettings();

    STEP::ImportExportSettingsPtr getSTEPSettings() const;
    IGES::ImportExportSettingsPtr getIGESSettings() const;

    void setReadShapeCompoundMode(bool);
    bool getReadShapeCompoundMode() const;

    void setExportHiddenObject(bool);
    bool getExportHiddenObject() const;

    void setImportHiddenObject(bool);
    bool getImportHiddenObject() const;

    void setExportLegacy(bool);
    bool getExportLegacy() const;

    void setExportKeepPlacement(bool);
    bool getExportKeepPlacement() const;

    void setUseLinkGroup(bool);
    bool getUseLinkGroup() const;

    void setUseBaseName(bool);
    bool getUseBaseName() const;

    void setReduceObjects(bool);
    bool getReduceObjects() const;

    void setExpandCompound(bool);
    bool getExpandCompound() const;

    void setShowProgress(bool);
    bool getShowProgress() const;

    void setImportMode(ImportMode);
    ImportMode getImportMode() const;

private:
    static void initGeneral(Base::Reference<ParameterGrp> hGrp);
    static void initSTEP(Base::Reference<ParameterGrp> hGrp);
    static void initIGES(Base::Reference<ParameterGrp> hGrp);

private:
    mutable STEP::ImportExportSettingsPtr step;
    mutable IGES::ImportExportSettingsPtr iges;
    ParameterGrp::handle pGroup;
};

} //namespace OCAF
} //namespace Part

#endif
