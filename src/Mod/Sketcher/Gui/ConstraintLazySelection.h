// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Turan Furkan Topak <furkan1795@gmail.com>          *
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

#include <optional>
#include <string>
#include <vector>

#include <Base/Type.h>
#include <Gui/Selection/SelectionObject.h>
#include <Mod/Sketcher/App/GeoEnum.h>

namespace Gui
{
class Command;
class Document;
}  // namespace Gui

namespace SketcherGui
{

class ViewProviderSketch;

struct SelIdPair
{
    int GeoId = Sketcher::GeoEnum::GeoUndef;
    Sketcher::PointPos PosId = Sketcher::PointPos::none;
    int LazyExternalId = -1;
    bool IsLazyExternal = false;
    bool LazyExternalVertex = false;
    std::string LazyExternalSourceObjectName;
    std::string LazyExternalSubelement;
    bool LazyExternalIntersection = false;
};

ViewProviderSketch* getActiveSketchGui(Gui::Document* guiDocument);

struct LazyExternalSubelement
{
    int id = -1;
    bool vertex = false;
};

bool selectionPairsEqual(const SelIdPair& lhs, const SelIdPair& rhs);

std::optional<LazyExternalSubelement> parseLazyExternalSubelement(const std::string& subName);


class ActivatedLazySelection
{
public:
    ActivatedLazySelection(
        Gui::Command& command,
        Gui::Document* guiDocument,
        bool includeLazyExternalVertices = false,
        bool* sharedCommandActive = nullptr
    );
    ActivatedLazySelection(const ActivatedLazySelection&) = delete;
    ActivatedLazySelection& operator=(const ActivatedLazySelection&) = delete;
    ActivatedLazySelection(ActivatedLazySelection&& other) noexcept;
    ActivatedLazySelection& operator=(ActivatedLazySelection&&) = delete;
    ~ActivatedLazySelection();

    const std::vector<Gui::SelectionObject>& getSelection() const;

    void openCommand(const char* name);
    void commitCommand();
    void abortCommand();

private:
    void begin(Gui::Document* guiDocument, bool includeLazyExternalVertices);
    void abortLazyExternalCommand();
    bool isLazyExternalCommandActive() const;
    void setLazyExternalCommandActive(bool active);

    Gui::Command* command = nullptr;
    bool* sharedCommandActive = nullptr;
    std::vector<Gui::SelectionObject> selection;
    bool localCommandActive = false;
    bool abortOnDestroy = true;
};

class LazyExternalSelectionResolver
{
public:
    static bool hasPendingLazyExternalSelection(
        ViewProviderSketch* sketchgui,
        bool includeEdges = true,
        bool includeVertices = false
    );

    static bool materializePendingLazyExternalSelection(
        ViewProviderSketch* sketchgui,
        bool includeLazyExternalVertices
    );

    static void appendPendingLazyExternalSelectionPairs(
        ViewProviderSketch* sketchgui,
        std::vector<SelIdPair>& points,
        std::vector<SelIdPair>& curves,
        bool includeEdges = true,
        bool includeVertices = true
    );

    static void clearPendingLazyExternalSelection(ViewProviderSketch* sketchgui);

    static bool makeLazyExternalSelectionPair(
        ViewProviderSketch* sketchgui,
        int lazyExternalId,
        bool lazyExternalVertex,
        SelIdPair& item,
        std::string* subName = nullptr,
        Base::Type* geoType = nullptr
    );

    static void setLazyExternalSelectionSelected(
        ViewProviderSketch* sketchgui,
        const SelIdPair& item,
        bool selected
    );

    static bool materializeLazyExternalSelectionPairs(
        ViewProviderSketch* sketchgui,
        std::vector<SelIdPair>& items,
        bool preserveLazyExternalIdentity = false
    );
};

}  // namespace SketcherGui
