// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <FCGlobal.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <ostream>
#include <string>
#include <vector>
#include <fastsignals/signal.h>

class QString;

namespace Base
{

template<class T>
struct iotaGen
{
public:
    T operator()()
    {
        return n++;
    }
    explicit iotaGen(T v)
        : n(v)
    {}

private:
    T n;
};

// ----------------------------------------------------------------------------

template<class T>
class manipulator
{
    T i_;
    std::ostream& (*f_)(std::ostream&, T);

public:
    manipulator(std::ostream& (*f)(std::ostream&, T), T i)
        : i_(i)
        , f_(f)
    {}
    friend std::ostream& operator<<(std::ostream& os, manipulator m)
    {
        return m.f_(os, m.i_);
    }
};

inline std::ostream& tabsN(std::ostream& os, int n)
{
    for (int i = 0; i < n; i++) {
        os << "\t";
    }
    return os;
}

inline std::ostream& blanksN(std::ostream& os, int n)
{
    for (int i = 0; i < n; i++) {
        os << " ";
    }
    return os;
}

inline manipulator<int> tabs(int n)
{
    return {&tabsN, n};
}

inline manipulator<int> blanks(int n)
{
    return {&blanksN, n};
}

// ----------------------------------------------------------------------------

template<class T>
    requires std::is_arithmetic_v<T>
inline T clamp(T num, T lower, T upper)
{
    return std::clamp<T>(num, lower, upper);
}

/// Returns -1, 0 or 1 depending on if the value is negative, zero or positive
/// As this function might be used in hot paths, it uses branchless implementation
template<typename T>
constexpr std::enable_if_t<std::is_arithmetic_v<T> && std::is_signed_v<T>, T> sgn(T val)
{
    int oneIfPositive = int(0 < val);
    int oneIfNegative = int(val < 0);
    return T(oneIfPositive - oneIfNegative);  // 0/1 - 0/1 = -1/0/1
}

/// Convert degrees to radians, allow deduction for floating point types
template<std::floating_point T>
constexpr T toRadians(T degrees)
{
    constexpr auto degToRad = std::numbers::pi_v<T> / T(180);
    return degrees * degToRad;
}

/// Convert degrees to radians, allow **explicit-only** for any arithmetic type
template<typename T>
    requires(std::is_arithmetic_v<T> && !std::floating_point<T>)
constexpr T toRadians(std::type_identity_t<T> degrees)
{
    using ResultT = std::conditional_t<std::is_integral_v<T>, double, T>;
    return static_cast<T>(toRadians<ResultT>(static_cast<ResultT>(degrees)));
}

/// Convert radians to degrees, allow deduction for floating point types
template<std::floating_point T>
constexpr T toDegrees(T radians)
{
    constexpr auto radToDeg = T(180) / std::numbers::pi_v<T>;
    return radians * radToDeg;
}

/// Convert radians to degrees, allow **explicit-only** for any arithmetic type
template<typename T>
    requires(std::is_arithmetic_v<T> && !std::floating_point<T>)
constexpr T toDegrees(std::type_identity_t<T> radians)
{
    using ResultT = std::conditional_t<std::is_integral_v<T>, double, T>;
    return static_cast<T>(toDegrees<ResultT>(static_cast<ResultT>(radians)));
}

inline float fromPercent(const long value)
{
    return std::roundf(static_cast<float>(value)) / 100.0F;
}

inline long toPercent(float value)
{
    return std::lround(100.0 * value);
}

template<std::floating_point T>
inline T fmod(T numerator, T denominator)
{
    T modulo = std::fmod(numerator, denominator);
    return (modulo >= T(0)) ? modulo : modulo + denominator;
}

template<std::floating_point T>
inline T angularDist(T v1, T v2)
{
    return std::min(std::fabs(v1 - v2), 360 - std::fabs(v1 - v2));
}

// Returns a value between [0, 360) or (-180, 180] depending on if the
// minimum value was positive or negetive. This is done because the taper angle
// values in FreeCAD usually treat values like -10 and 350 differently
template<std::floating_point T>
inline double clampAngle(T value, T min, T max, T precision)
{
    // Normalize the angles between 0 and 360
    value = Base::fmod(value, 360.0);
    T nMin = Base::fmod(min, 360.0);
    T nMax = Base::fmod(max, 360.0);

    if (std::abs(nMax - nMin) > precision) {
        if (nMax > nMin) {
            if (value < nMin || value > nMax) {
                value = angularDist(value, nMin) > angularDist(value, nMax) ? nMax : nMin;
            }
        }
        else {
            if (value < nMin && value > nMax) {
                value = angularDist(value, nMin) > angularDist(value, nMax) ? nMax : nMin;
            }
        }
    }

    if (min >= 0.0) {
        // Return in [0, 360)
        return value;
    }

    // Map to (-180, 180]
    if (value > 180.0) {
        value = value - 360;
    }
    return value;
}


// ----------------------------------------------------------------------------

// NOLINTBEGIN
template<typename Flag = bool>
struct FlagToggler
{

    Flag& flag;
    bool toggled;

    FlagToggler(Flag& _flag)
        : flag(_flag)
        , toggled(true)
    {
        flag = !flag;
    }

    FlagToggler(Flag& _flag, Flag check)
        : flag(_flag)
        , toggled(check == _flag)
    {
        if (toggled) {
            flag = !flag;
        }
    }

    ~FlagToggler()
    {
        if (toggled) {
            flag = !flag;
        }
    }
};

// ----------------------------------------------------------------------------

template<typename Status, class Object>
class ObjectStatusLocker
{
public:
    ObjectStatusLocker(Status s, Object* o, bool value = true)
        : status(s)
        , obj(o)
    {
        old_value = obj->testStatus(status);
        obj->setStatus(status, value);
    }
    ~ObjectStatusLocker()
    {
        obj->setStatus(status, old_value);
    }

private:
    Status status;
    Object* obj;
    bool old_value;
};

// ----------------------------------------------------------------------------

class StateLocker
{
public:
    StateLocker(bool& flag, bool value = true)
        : lock(flag)
    {
        old_value = lock;
        lock = value;
    }  // NOLINT
    ~StateLocker()
    {
        lock = old_value;
    }

private:
    bool& lock;
    bool old_value;
};

// ----------------------------------------------------------------------------

template<typename T>
class BitsetLocker
{
public:
    BitsetLocker(T& flags, std::size_t flag, bool value = true)
        : flags(flags)
        , flag(flag)
    {
        oldValue = flags.test(flag);
        flags.set(flag, value);
    }
    ~BitsetLocker()
    {
        flags.set(flag, oldValue);
    }

private:
    T& flags;
    std::size_t flag;
    bool oldValue;
};

// ----------------------------------------------------------------------------

class ConnectionBlocker
{
    fastsignals::shared_connection_block blocker;

public:
    ConnectionBlocker(fastsignals::advanced_connection& c)
        : blocker(c)
    {}
    ~ConnectionBlocker() = default;
};
// NOLINTEND

// ----------------------------------------------------------------------------

struct BaseExport Tools
{
    /**
     * Given an arbitrary string, ensure that it conforms to Python3 identifier rules, replacing
     * invalid characters with an underscore. If the first character is invalid, prepends an
     * underscore to the name. See https://unicode.org/reports/tr31/ for complete naming rules.
     * @param String to be checked and sanitized.
     * @return A std::string that is a valid Python 3 identifier.
     */
    static std::string getIdentifier(const std::string& name);
    static std::wstring widen(const std::string& str);
    static std::string narrow(const std::wstring& str);
    static std::string escapedUnicodeFromUtf8(const char* s);
    static std::string escapedUnicodeToUtf8(const std::string& s);
    static std::string escapeQuotesFromString(const std::string& s);

    static QString escapeEncodeString(const QString& s);
    static std::string escapeEncodeString(const std::string& s);
    static QString escapeEncodeFilename(const QString& s);
    static std::string escapeEncodeFilename(const std::string& s);

    /**
     * @brief quoted Creates a quoted string.
     * @param String to be quoted.
     * @return A quoted std::string.
     */
    static std::string quoted(const char*);
    /**
     * @brief quoted Creates a quoted string.
     * @param String to be quoted.
     * @return A quoted std::string.
     */
    static std::string quoted(const std::string&);

    static constexpr bool isNullOrEmpty(const char* str)
    {
        return !str || str[0] == '\0';
    }

    /**
     * @brief joinList
     * Join the vector of strings \a vec using the separator \a sep
     * @param vec
     * @param sep
     * @return
     */
    static std::string joinList(const std::vector<std::string>& vec, const std::string& sep = ", ");

    static std::string currentDateTimeString();

    static std::vector<std::string> splitSubName(const std::string& subname);
};

struct BaseExport ZipTools
{
    /**
     * @brief rewrite Rewrite a zip file under a new name.
     */
    static void rewrite(const std::string& source, const std::string& target);
};


/**
 * Helper struct to define inline overloads for the visitor pattern in std::visit.
 *
 * It uses type deduction to infer the type from the expression and creates a dedicated type that
 * essentially is callable using any overload supplied.
 *
 * @code
 * using Base::Overloads;
 *
 * const auto visitor = Overloads
 * {
 *     [](int i){ std::print("int = {}\n", i); },
 *     [](std::string_view s){ std::println("string = “{}”", s); },
 *     [](const Base&){ std::println("base"); },
 * };
 * @endcode
 *
 * @see https://en.cppreference.com/w/cpp/utility/variant/visit
 *
 * @tparam Ts Types for functions that will be used for overloads
 */
template<class... Ts>
struct Overloads: Ts...
{
    using Ts::operator()...;
};

template<class... Ts>
Overloads(Ts...) -> Overloads<Ts...>;

}  // namespace Base
