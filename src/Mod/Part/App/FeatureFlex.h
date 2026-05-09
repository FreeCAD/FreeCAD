// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            *
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

#include <BRepAdaptor_Curve.hxx>
#include <Mod/Part/PartGlobal.h>

#include <exprtk.hpp>

#include "PartFeature.h"


namespace Part
{

enum class FlexMode
{
    Bend,
    Twist,
    UserDefined,
};

class DeformExpr
{
    using symbol_table_t = exprtk::symbol_table<double>;
    using expression_t = exprtk::expression<double>;
    using parser_t = exprtk::parser<double>;

public:
    DeformExpr(const std::string& xFunc, const std::string& yFunc, const std::string& zFunc);
    double x(double vx, double vy, double vz);
    double y(double vx, double vy, double vz);
    double z(double vx, double vy, double vz);

private:
    void setValues(double vx, double vy, double vz);
    double valX = 0.;
    double valY = 0.;
    double valZ = 0.;

    symbol_table_t symTable;

    expression_t xexpr;
    expression_t yexpr;
    expression_t zexpr;
};

class PartExport Flex: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Flex);

public:
    Flex();

    App::PropertyLink Base;
    App::PropertyIntegerConstraint Samples;
    App::PropertyEnumeration Mode;
    App::PropertyPosition Origin;
    App::PropertyVector Direction;
    App::PropertyFloat Pitch;
    App::PropertyLinkSub Curve;
    App::PropertyFloat Factor;
    App::PropertyString xFunc;
    App::PropertyString yFunc;
    App::PropertyString zFunc;


    /**
     * @brief The FlexParameters struct is supposed to be filled with final
     * Flex parameters and be passed to FlexShape.
     */
    struct FlexParameters
    {
        int samples {10};
        FlexMode mode {FlexMode::Bend};
        double pitch {10.0};
        BRepAdaptor_Curve curve;
        double factor {1.};
        gp_Ax3 coord;
        DeformExpr funcExpr {"x", "y", "z"};
    };

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderFlex";
    }
    //@}

    bool fetchCurveLink(const App::PropertyLinkSub& curveLink, BRepAdaptor_Curve& curve) const;
    Flex::FlexParameters computeFinalParameters() const;

    /**
     * @brief FlexShape powers the deformation feature.
     * @param source: the shape to be deformed
     * @param params: deformation parameters
     * @return result of deformation
     */
    static TopoShape FlexShape(const TopoShape& source, FlexParameters& params);

    static TopoShape bend(const TopoShape& source, const Flex::FlexParameters& params);
    static TopoShape twist(const TopoShape& source, const Flex::FlexParameters& params);
    static TopoShape userDeform(const TopoShape& source, Flex::FlexParameters& params);

private:
    static App::PropertyIntegerConstraint::Constraints sampleRange;
    static const char* ModeEnums[];
};

}  // namespace Part
