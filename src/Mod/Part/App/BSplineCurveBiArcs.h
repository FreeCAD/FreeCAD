// SPDX-License-Identifier: BSD-3-Clause

// This file is released under the BSD license
//
// Copyright (c) 2009, Daniel Heeks
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice, this
//      list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright notice, this
//      list of conditions and the following disclaimer in the documentation and/or
//      other materials provided with the distribution.
//    * Neither the name of Daniel Heeks nor the names of its contributors may be used
//      to endorse or promote products derived from this software without specific prior
//      written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <list>

#include <Mod/Part/PartGlobal.h>


namespace Part
{

class Geometry;

class PartExport BSplineCurveBiArcs
{
public:
    BSplineCurveBiArcs(const Handle(Geom_Curve) &);
    std::list<Geometry*> toBiArcs(double tolerance) const;

private:
    void createArcs(
        double tolerance,
        std::list<Geometry*>& new_spans,
        const gp_Pnt& p_start,
        const gp_Vec& v_start,
        double t_start,
        double t_end,
        gp_Pnt& p_end,
        gp_Vec& v_end
    ) const;
    enum class Type
    {
        SingleArc,
        SplitCurve,
        SingleLine
    };

    Type calculateBiArcPoints(
        double t_start,
        const gp_Pnt& p0,
        gp_Vec v_start,
        double t_end,
        const gp_Pnt& p4,
        gp_Vec v_end,
        gp_Pnt& p1,
        gp_Pnt& p2,
        gp_Pnt& p3
    ) const;

private:
    Handle(Geom_Curve) myCurve;
};

}  // namespace Part
