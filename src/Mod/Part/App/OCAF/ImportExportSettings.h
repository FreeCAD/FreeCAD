// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <list>
#include <memory>
#include <Mod/Part/App/Interface.h>
#include <Base/Parameter.h>
#include <Standard_Version.hxx>
#include <Resource_FormatType.hxx>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

namespace STEP
{
class ImportExportSettings;
using ImportExportSettingsPtr = std::shared_ptr<ImportExportSettings>;
}  // namespace STEP

namespace IGES
{
class ImportExportSettings;
using ImportExportSettingsPtr = std::shared_ptr<ImportExportSettings>;
}  // namespace IGES

namespace OCAF
{

class PartExport ImportExportSettings
{
public:
    enum class ImportMode
    {
        SingleDocument = 0,
        GroupPerDocument = 1,
        GroupPerDirectory = 2,
        ObjectPerDocument = 3,
        ObjectPerDirectory = 4,
    };
    struct CodePage
    {
        std::string codePageName;
        Resource_FormatType codePage;
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

    void setImportCodePage(int);
    Resource_FormatType getImportCodePage() const;
    std::list<ImportExportSettings::CodePage> getCodePageList() const;

private:
    static void initGeneral(Base::Reference<ParameterGrp> hGrp);
    static void initSTEP(Base::Reference<ParameterGrp> hGrp);
    static void initIGES(Base::Reference<ParameterGrp> hGrp);

private:
    mutable STEP::ImportExportSettingsPtr step;
    mutable IGES::ImportExportSettingsPtr iges;
    ParameterGrp::handle pGroup;
    // clang-format off
    std::list<CodePage> codePageList {
#if OCC_VERSION_HEX >= 0x070800
        {"No conversion", Resource_FormatType_NoConversion},
        {"Multi-byte UTF-8 encoding", Resource_FormatType_UTF8},
        {"SJIS (Shift Japanese Industrial Standards) encoding", Resource_FormatType_SJIS},
        {"EUC (Extended Unix Code) ", Resource_FormatType_EUC},
        {"GB (Guobiao) encoding for Simplified Chinese", Resource_FormatType_GB},
        {"GBK (Unified Chinese) encoding", Resource_FormatType_GBK},
        {"Big5 (Traditional Chinese) encoding", Resource_FormatType_Big5},
        //{"active system-defined locale; this value is strongly NOT recommended to use", Resource_FormatType_SystemLocale},
        {"ISO 8859-1 (Western European) encoding", Resource_FormatType_iso8859_1},
        {"ISO 8859-2 (Central European) encoding", Resource_FormatType_iso8859_2},
        {"ISO 8859-3 (Turkish) encoding", Resource_FormatType_iso8859_3},
        {"ISO 8859-4 (Northern European) encoding", Resource_FormatType_iso8859_4},
        {"ISO 8859-5 (Cyrillic) encoding", Resource_FormatType_iso8859_5},
        {"ISO 8859-6 (Arabic) encoding", Resource_FormatType_iso8859_6},
        {"ISO 8859-7 (Greek) encoding", Resource_FormatType_iso8859_7},
        {"ISO 8859-8 (Hebrew) encoding", Resource_FormatType_iso8859_8},
        {"ISO 8859-9 (Turkish) encoding", Resource_FormatType_iso8859_9},
        {"ISO 850 (Western European) encoding", Resource_FormatType_CP850},
        {"CP1250 (Central European) encoding", Resource_FormatType_CP1250},
        {"CP1251 (Cyrillic) encoding", Resource_FormatType_CP1251},
        {"CP1252 (Western European) encoding", Resource_FormatType_CP1252},
        {"CP1253 (Greek) encoding", Resource_FormatType_CP1253},
        {"CP1254 (Turkish) encoding", Resource_FormatType_CP1254},
        {"CP1255 (Hebrew) encoding", Resource_FormatType_CP1255},
        {"CP1256 (Arabic) encoding", Resource_FormatType_CP1256},
        {"CP1257 (Baltic) encoding", Resource_FormatType_CP1257},
        {"CP1258 (Vietnamese) encoding", Resource_FormatType_CP1258},
#endif
    };
    // clang-format on
};

}  // namespace OCAF
}  // namespace Part
