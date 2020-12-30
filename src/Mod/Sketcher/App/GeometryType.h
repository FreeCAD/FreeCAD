#ifndef SKETCHER_GEOTYPE_H
#define SKETCHER_GEOTYPE_H

#include <Base/BaseClass.h>


namespace Sketcher
{
    class GeometryType {
    public:
        enum Value {
        None    = 0,
        Point   = 1, // 1 Point(start), 2 Parameters(x,y)
        Line    = 2, // 2 Points(start,end), 4 Parameters(x1,y1,x2,y2)
        Arc     = 3, // 3 Points(start,end,mid), (4)+5 Parameters((x1,y1,x2,y2),x,y,r,a1,a2)
        Circle  = 4, // 1 Point(mid), 3 Parameters(x,y,r)
        Ellipse = 5,  // 1 Point(mid), 5 Parameters(x,y,r1,r2,phi)  phi=angle xaxis of ellipse with respect of sketch xaxis
        ArcOfEllipse = 6,
        ArcOfHyperbola = 7,
        ArcOfParabola = 8,
        BSpline = 9
        };
        GeometryType() = default;
        constexpr GeometryType(Value t) : value(t) { }
        constexpr explicit GeometryType(int t) : value((Value)t) { }

        explicit operator bool() = delete;
        operator int() const {return value;}

        const char * str() const;
        static GeometryType from(const char * const str);
        static GeometryType from(const Base::Type & geometryClassType);

    private:
        Value value;
    };
}

#endif // SKETCHER_GEOTYPE_H