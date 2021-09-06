/***************************************************************************
 *   Copyright (c) 2019 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHER_SOLVERGEOMETRYEXTENSION_H
#define SKETCHER_SOLVERGEOMETRYEXTENSION_H

#include <Mod/Part/App/GeometryExtension.h>

namespace Sketcher
{

class SketcherExport SolverGeometryExtension : public Part::GeometryExtension
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    enum SolverStatus {
        FullyConstraint = 0,
        NotFullyConstraint = 1,
        NumSolverStatus
    };

    enum ParameterStatus {
        Dependent = 0,
        Independent = 1,
        NumParameterStatus
    };

    SolverGeometryExtension();
    SolverGeometryExtension(long cid);
    virtual ~SolverGeometryExtension() override = default;

    virtual std::unique_ptr<Part::GeometryExtension> copy(void) const override;

    virtual PyObject *getPyObject(void) override;

    SolverStatus getGeometry() const  {return ( Edge == Independent &&
                                                Start == Independent &&
                                                End == Independent &&
                                                Mid == Independent) ? FullyConstraint : NotFullyConstraint;}

    ParameterStatus getEdge() const  {return Edge;}
    void setEdge(ParameterStatus status) {Edge = status;}

    ParameterStatus getStart() const  {return Start;}
    void setStart(ParameterStatus status) {Start = status;}

    ParameterStatus getMid() const  {return Mid;}
    void setMid(ParameterStatus status) {Mid = status;}

    ParameterStatus getEnd() const  {return End;}
    void setEnd(ParameterStatus status) {End = status;}

    void init(ParameterStatus status) {
        Edge = status;
        Start = status;
        Mid = status;
        End = status;
    }

protected:
    virtual void copyAttributes(Part::GeometryExtension * cpy) const override;

private:
    SolverGeometryExtension(const SolverGeometryExtension&) = default;

private:
    ParameterStatus    Edge;
    ParameterStatus    Start;
    ParameterStatus    Mid;
    ParameterStatus    End;
};

} //namespace Sketcher


#endif // SKETCHER_SOLVERGEOMETRYEXTENSION_H
