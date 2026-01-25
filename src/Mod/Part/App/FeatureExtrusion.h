// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/Document.h>

#include <Mod/Part/PartGlobal.h>

#include "FaceMakerCheese.h"
#include "PartFeature.h"
#include "ExtrusionHelper.h"

namespace Part
{

class PartExport Extrusion: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Extrusion);

public:
    Extrusion();

    App::PropertyLink Base;
    App::PropertyVector Dir;
    App::PropertyEnumeration DirMode;
    App::PropertyLinkSub DirLink;
    App::PropertyDistance LengthFwd;
    App::PropertyDistance LengthRev;
    App::PropertyBool Solid;
    App::PropertyBool Reversed;
    App::PropertyBool Symmetric;
    App::PropertyAngle TaperAngle;
    App::PropertyAngle TaperAngleRev;
    App::PropertyString FaceMakerClass;
    App::PropertyEnumeration FaceMakerMode;
    App::PropertyEnumeration InnerWireTaper;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderExtrusion";
    }
    //@}

    /**
     * @brief extrudeShape powers the extrusion feature.
     * @param result: result of extrusion
     * @param source: the shape to be extruded
     * @param params: extrusion parameters
     */
    static void extrudeShape(
        TopoShape& result,
        const TopoShape& source,
        const ExtrusionParameters& params
    );

    /**
     * @brief fetchAxisLink: read AxisLink to obtain the direction and
     * length. Note: this routine is reused in Extrude dialog, hence it
     * is static.
     * @param axisLink (input): the link
     * @param basepoint (output): starting point of edge. Not used by extrude as of now.
     * @param dir (output): direction of axis, with magnitude (length)
     * @return true if link was fetched. false if link was empty. Throws if the
     * link is wrong.
     */
    static bool fetchAxisLink(
        const App::PropertyLinkSub& axisLink,
        Base::Vector3d& basepoint,
        Base::Vector3d& dir
    );

    /**
     * @brief computeFinalParameters: applies mode logic and fetches links, to
     * compute the actual parameters of extrusion. Dir property is updated in
     * the process, hence the function is non-const.
     */
    ExtrusionParameters computeFinalParameters();

    static Base::Vector3d calculateShapeNormal(const App::PropertyLink& shapeLink);
    void onDocumentRestored() override;
    void Restore(Base::XMLReader& reader) override;

public:  // mode enumerations
    enum eDirMode
    {
        dmCustom,
        dmEdge,
        dmNormal
    };
    static const char* eDirModeStrings[];
    static const char* eInnerWireTaperStrings[];

protected:
    void setupObject() override;
    void onChanged(const App::Property* prop) override;
};

/**
 * @brief FaceMakerExtrusion provides legacy compounding-structure-ignorant behavior of facemaker of
 * Part Extrude. Strengths: makes faces with holes Weaknesses: can't make islands in holes. Ignores
 * compounding nesting. All faces must be on same plane.
 */
class FaceMakerExtrusion: public FaceMakerCheese
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    std::string getUserFriendlyName() const override;
    std::string getBriefExplanation() const override;

#if OCC_VERSION_HEX >= 0x070600
    void Build(const Message_ProgressRange& theRange = Message_ProgressRange()) override;
#else
    void Build() override;
#endif

protected:
    void Build_Essence() override
    {}
};

}  // namespace Part
