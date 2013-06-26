/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef BASE_TOOLS_H
#define BASE_TOOLS_H

#include <functional>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <string>

namespace Base
{

template <class T>
struct iotaGen
{
public:
    T operator()() { return n++; }
    iotaGen(T v) : n(v) {}

private:
    T n;
};

// ----------------------------------------------------------------------------

template <class T>
class manipulator
{
    T i_;
    std::ostream& (*f_)(std::ostream&, T);

public:
    manipulator(std::ostream& (*f)(std::ostream&, T), T i) : i_(i), f_(f)
    {
    }
    friend std::ostream& operator<<( std::ostream& os, manipulator m)
    {
        return m.f_(os, m.i_);
    }
};

inline std::ostream& tabsN(std::ostream& os, int n)
{
    for (int i=0;i<n;i++)
        os << "\t";
    return os;
}

inline std::ostream& blanksN(std::ostream& os, int n)
{
    for (int i=0;i<n;i++)
        os << " ";
    return os;
}

inline manipulator<int> tabs(int n)
{
    return manipulator<int>(&tabsN, n);
}

inline manipulator<int> blanks(int n)
{
    return manipulator<int>(&blanksN, n);
}

// ----------------------------------------------------------------------------

template<class T>
inline T clamp (T num, T lower, T upper)
{
    return std::max<T>(std::min<T>(upper,num),lower);
}

template<class T>
inline T sgn (T t)
{
    if (t == 0)
        return T(0);
    else
        return (t > 0) ? T(1) : T(-1);
}

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

template<class T>
inline T toRadians(T d)
{
    return static_cast<T>((d*M_PI)/180.0);
}

template<class T>
inline T toDegrees(T r)
{
    return static_cast<T>((r/M_PI)*180.0);
}

template<class T>
inline T fmod(T numerator, T denominator)
{
    T modulo = std::fmod(numerator, denominator);
    return (modulo >= T(0)) ? modulo : modulo + denominator;
}

// ----------------------------------------------------------------------------

class BaseExport StopWatch
{
public:
    StopWatch();
    ~StopWatch();

    void start();
    int elapsed();
    std::string toString(int ms) const;

private:
    struct Private;
    Private* d;
};

// ----------------------------------------------------------------------------

struct BaseExport Tools
{
    static std::string getUniqueName(const std::string&, const std::vector<std::string>&,int d=0);
    static std::string addNumber(const std::string&, unsigned int, int d=0);
    static std::string getIdentifier(const std::string&);
    static std::wstring widen(const std::string& str);
    static std::string narrow(const std::wstring& str);
};

} // namespace Base

#endif // BASE_TOOLS_H
