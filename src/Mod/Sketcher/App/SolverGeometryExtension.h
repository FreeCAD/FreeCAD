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

#include <Base/Exception.h>
#include <Mod/Part/App/GeometryExtension.h>
#include <Mod/Sketcher/SketcherGlobal.h>

#include "GeoEnum.h"


namespace Sketcher
{

class SketcherExport SolverGeometryExtension: public Part::GeometryExtension
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum SolverStatus
    {
        FullyConstraint = 0,
        NotFullyConstraint = 1,
        NumSolverStatus
    };

    enum ParameterStatus
    {
        Dependent = 0,
        Independent = 1,
        NumParameterStatus
    };

    class PointParameterStatus
    {
    public:
        explicit PointParameterStatus(ParameterStatus status)
        {
            setStatus(status);
        }
        PointParameterStatus(ParameterStatus statusx, ParameterStatus statusy)
        {
            setStatus(statusx, statusy);
        }

        PointParameterStatus(const PointParameterStatus&) = default;
        PointParameterStatus& operator=(const PointParameterStatus&) = default;
        PointParameterStatus(PointParameterStatus&&) = default;
        PointParameterStatus& operator=(PointParameterStatus&&) = default;

        ParameterStatus getStatus() const
        {
            return (xstatus == Independent && ystatus == Independent) ? Independent : Dependent;
        }
        ParameterStatus getStatusx() const
        {
            return xstatus;
        }
        ParameterStatus getStatusy() const
        {
            return ystatus;
        }

        bool isXDoF()
        {
            return xstatus == Dependent;
        }
        bool isYDoF()
        {
            return ystatus == Dependent;
        }

        int getDoFs()
        {
            bool xfree = isXDoF();
            bool yfree = isYDoF();

            if (xfree && yfree) {
                return 2;
            }
            else if (xfree || yfree) {
                return 1;
            }
            else {
                return 0;
            }
        }

        void setStatus(ParameterStatus status)
        {
            xstatus = status;
            ystatus = status;
        }
        void setStatus(ParameterStatus statusx, ParameterStatus statusy)
        {
            xstatus = statusx;
            ystatus = statusy;
        }
        void setStatusx(ParameterStatus statusx)
        {
            xstatus = statusx;
        }
        void setStatusy(ParameterStatus statusy)
        {
            ystatus = statusy;
        }

    private:
        ParameterStatus xstatus;
        ParameterStatus ystatus;
    };

    class EdgeParameterStatus
    {
    public:
        EdgeParameterStatus() = default;

        void init(int nparams)
        {
            pstatus.resize(nparams, ParameterStatus::Dependent);
        }

        ParameterStatus getStatus() const
        {
            return std::all_of(pstatus.begin(),
                               pstatus.end(),
                               [](const auto& v) {
                                   return v == Independent;
                               })
                ? Independent
                : Dependent;
        }

        void setStatus(ParameterStatus status)
        {
            std::fill(pstatus.begin(), pstatus.end(), status);
        }

        void setStatus(int index, ParameterStatus status)
        {
            if (index >= int(pstatus.size())) {
                pstatus.resize(index + 1, ParameterStatus::Dependent);
            }

            pstatus.at(index) = status;
        };

    protected:
        std::vector<ParameterStatus> pstatus;
    };

    class Point: public EdgeParameterStatus
    {
    public:
        Point() = default;
    };

    class Line: public EdgeParameterStatus
    {
    public:
        Line() = default;
    };

    class Arc: public EdgeParameterStatus
    {
    public:
        Arc() = default;

        ParameterStatus getRadiusStatus() const
        {
            return pstatus[0];
        }
        ParameterStatus getStartParameter() const
        {
            return pstatus[1];
        }
        ParameterStatus getEndParameter() const
        {
            return pstatus[2];
        }
    };

    class Circle: public EdgeParameterStatus
    {
    public:
        Circle() = default;

        ParameterStatus getRadiusStatus() const
        {
            return pstatus[0];
        }
        bool isRadiusDoF() const
        {
            return pstatus[0] == Dependent;
        }
    };

    class ArcOfEllipse: public EdgeParameterStatus
    {
    public:
        ArcOfEllipse() = default;

        ParameterStatus getFocusXStatus() const
        {
            return pstatus[0];
        }
        ParameterStatus getFocusYStatus() const
        {
            return pstatus[1];
        }
        ParameterStatus getFocusMinorRadiusStatus() const
        {
            return pstatus[2];
        }
        ParameterStatus getStartParameter() const
        {
            return pstatus[3];
        }
        ParameterStatus getEndParameter() const
        {
            return pstatus[4];
        }

        bool isFocusDoF() const
        {
            return pstatus[0] == Dependent || pstatus[1] == Dependent;
        }
        bool isMinorRadiusDoF() const
        {
            return (pstatus[2] == Dependent);
        }
    };

    class Ellipse: public EdgeParameterStatus
    {
    public:
        Ellipse() = default;

        ParameterStatus getFocusXStatus() const
        {
            return pstatus[0];
        }
        ParameterStatus getFocusYStatus() const
        {
            return pstatus[1];
        }
        ParameterStatus getFocusMinorRadiusStatus() const
        {
            return pstatus[2];
        }

        bool isFocusDoF() const
        {
            return pstatus[0] == Dependent || pstatus[1] == Dependent;
        }
        bool isMinorRadiusDoF() const
        {
            return (pstatus[2] == Dependent);
        }
    };

    class ArcOfHyperbola: public EdgeParameterStatus
    {
    public:
        ArcOfHyperbola() = default;

        ParameterStatus getFocusXStatus() const
        {
            return pstatus[0];
        }
        ParameterStatus getFocusYStatus() const
        {
            return pstatus[1];
        }
        ParameterStatus getFocusMinorRadiusStatus() const
        {
            return pstatus[2];
        }
        ParameterStatus getStartParameter() const
        {
            return pstatus[3];
        }
        ParameterStatus getEndParameter() const
        {
            return pstatus[4];
        }
    };

    class ArcOfParabola: public EdgeParameterStatus
    {
    public:
        ArcOfParabola() = default;

        ParameterStatus getFocusXStatus() const
        {
            return pstatus[0];
        }
        ParameterStatus getFocusYStatus() const
        {
            return pstatus[1];
        }
    };

    class BSpline: public EdgeParameterStatus
    {
    public:
        BSpline() = default;

        ParameterStatus getPoleXStatus(int poleindex) const
        {
            int npoles = pstatus.size() / 3;

            if (poleindex < npoles) {
                return pstatus[poleindex * 2];
            }

            THROWM(Base::IndexError, "Pole index out of range")
        }

        ParameterStatus getPoleYStatus(int poleindex) const
        {
            int npoles = pstatus.size() / 3;

            if (poleindex < npoles) {
                return pstatus[poleindex * 2 + 1];
            }

            THROWM(Base::IndexError, "Pole index out of range")
        }

        ParameterStatus getWeightStatus(int weightindex) const
        {
            int nweights = pstatus.size() / 3;

            if (weightindex < nweights) {
                return pstatus[nweights * 2 + weightindex];
            }

            THROWM(Base::IndexError, "Weight index out of range")
        }
    };


    SolverGeometryExtension();
    ~SolverGeometryExtension() override = default;

    std::unique_ptr<Part::GeometryExtension> copy() const override;

    PyObject* getPyObject() override;

    void notifyAttachment(Part::Geometry* geo) override;

    SolverStatus getGeometry() const
    {
        return (Edge.getStatus() == Independent && Start.getStatus() == Independent
                && End.getStatus() == Independent && Mid.getStatus() == Independent)
            ? FullyConstraint
            : NotFullyConstraint;
    }

    ParameterStatus getEdge() const
    {
        return Edge.getStatus();
    }
    Point& getPoint();
    Line& getLine();
    Arc& getArc();
    Circle& getCircle();
    ArcOfEllipse& getArcOfEllipse();
    Ellipse& getEllipse();
    ArcOfHyperbola& getArcOfHyperbola();
    ArcOfParabola& getArcOfParabola();
    BSpline& getBSpline();
    EdgeParameterStatus getEdgeParameters()
    {
        return Edge;
    }
    void setEdge(ParameterStatus status)
    {
        Edge.setStatus(status);
    }
    void setEdge(int paramindex, ParameterStatus status)
    {
        Edge.setStatus(paramindex, status);
    }

    ParameterStatus getStart() const
    {
        return Start.getStatus();
    }
    PointParameterStatus getStartPoint() const
    {
        return Start;
    }
    void setStart(ParameterStatus xstatus, ParameterStatus ystatus)
    {
        Start.setStatus(xstatus, ystatus);
    }
    void setStartx(ParameterStatus xstatus)
    {
        Start.setStatusx(xstatus);
    }
    void setStarty(ParameterStatus ystatus)
    {
        Start.setStatusy(ystatus);
    }

    ParameterStatus getMid() const
    {
        return Mid.getStatus();
    }
    PointParameterStatus getMidPoint() const
    {
        return Mid;
    }
    void setMid(ParameterStatus xstatus, ParameterStatus ystatus)
    {
        Mid.setStatus(xstatus, ystatus);
    }
    void setMidx(ParameterStatus xstatus)
    {
        Mid.setStatusx(xstatus);
    }
    void setMidy(ParameterStatus ystatus)
    {
        Mid.setStatusy(ystatus);
    }

    ParameterStatus getEnd() const
    {
        return End.getStatus();
    }
    PointParameterStatus getEndPoint() const
    {
        return End;
    }

    void setEnd(ParameterStatus xstatus, ParameterStatus ystatus)
    {
        End.setStatus(xstatus, ystatus);
    }
    void setEndx(ParameterStatus xstatus)
    {
        End.setStatusx(xstatus);
    }
    void setEndy(ParameterStatus ystatus)
    {
        End.setStatusy(ystatus);
    }

    PointParameterStatus getPoint(Sketcher::PointPos pos) const;

    void init(ParameterStatus status)
    {
        Edge.setStatus(status);
        Start.setStatus(status);
        Mid.setStatus(status);
        End.setStatus(status);
    }

protected:
    void copyAttributes(Part::GeometryExtension* cpy) const override;

private:
    SolverGeometryExtension(const SolverGeometryExtension&) = default;

    void ensureType(const Base::Type& type);

private:
    EdgeParameterStatus Edge;

    PointParameterStatus Start;
    PointParameterStatus Mid;
    PointParameterStatus End;

    Base::Type GeometryType;
};

}  // namespace Sketcher


#endif  // SKETCHER_SOLVERGEOMETRYEXTENSION_H
