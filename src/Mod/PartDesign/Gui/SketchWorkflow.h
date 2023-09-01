/**************************************************************************
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

#ifndef PARTDESIGNGUI_SKETCHWORKFLOW_H
#define PARTDESIGNGUI_SKETCHWORKFLOW_H

#include <tuple>
#include <Mod/PartDesign/PartDesignGlobal.h>
#include <Gui/SelectionFilter.h>

namespace App {
class Document;
class DocumentObject;
class GeoFeatureGroupExtension;
}
namespace Gui {
class Document;
}
namespace PartDesign {
class Body;
}
namespace PartDesignGui {

class SketchWorkflow
{
public:
    explicit SketchWorkflow(Gui::Document*);
    void createSketch();

private:
    void tryCreateSketch();
    void createSketchWithModernWorkflow();
    void createSketchWithLegacyWorkflow();
    std::tuple<bool, PartDesign::Body*> shouldCreateBody();
    bool shouldAbort(bool) const;
    std::tuple<Gui::SelectionFilter, Gui::SelectionFilter> getFaceAndPlaneFilter() const;

private:
    Gui::Document* guidocument;
    App::Document* appdocument;
    PartDesign::Body* activeBody{nullptr};
};

} // namespace PartDesignGui

#endif // PARTDESIGNGUI_SKETCHWORKFLOW_H
