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

#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif
#include <functional>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <boost_signals2.hpp>
#include <QString>

// ----------------------------------------------------------------------------

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
inline T clamp(T num, T lower, T upper)
{
    return std::max<T>(std::min<T>(upper, num), lower);
}

template<class T>
inline T sgn(T t)
{
    if (t == 0) {
        return T(0);
    }

    return (t > 0) ? T(1) : T(-1);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

template<class T>
inline T toRadians(T d)
{
    return static_cast<T>((d * M_PI) / 180.0);
}

template<class T>
inline T toDegrees(T r)
{
    return static_cast<T>((r / M_PI) * 180.0);
}

template<class T>
inline T fmod(T numerator, T denominator)
{
    T modulo = std::fmod(numerator, denominator);
    return (modulo >= T(0)) ? modulo : modulo + denominator;
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
    using Connection = boost::signals2::connection;
    using ConnectionBlock = boost::signals2::shared_connection_block;
    ConnectionBlock blocker;

public:
    ConnectionBlocker(Connection& c)
        : blocker(c)
    {}
    ~ConnectionBlocker() = default;
};
// NOLINTEND

// ----------------------------------------------------------------------------


class BaseExport UniqueNameManager
{
protected:
    // This method returns the position of the start of the suffix (or name.cend() if no
    // suffix). It must return the same suffix lentgh (name.size() - returnValue) for both
    // unique names (one containing digits) and the corresponding base name (with no digits).
    virtual std::string::const_iterator GetNameSuffixStartPosition(const std::string& name) const
    {
        return name.cend();
    }

private:
    class PiecewiseSparseIntegerSet
    {
    public:
        PiecewiseSparseIntegerSet()
        {}

    private:
        // Each pair being <lowest, count> represents the span of integers from lowest to
        // (lowest+count-1) inclusive
        using etype = std::pair<uint, uint>;
        // This span comparer class is analogous to std::less and treats overlapping spans as being
        // neither greater nor less than each other
        class comparer
        {
        public:
            bool operator()(const etype& lhs, const etype& rhs) const
            {
                // The equality case here is when lhs is below and directly adjacent to rhs.
                return rhs.first - lhs.first >= lhs.second;
            }
        };
        // Spans is the set of spans. Adjacent spans are coalesced so there are always gaps between
        // the entries.
        std::set<etype, comparer> Spans;
        using iterator = typename std::set<etype, comparer>::iterator;
        using const_iterator = typename std::set<etype, comparer>::const_iterator;

    public:
        void Add(uint value);
        void Remove(uint value);
        bool Contains(uint value) const;
        bool Any() const
        {
            return Spans.size() != 0;
        }
        void Clear()
        {
            Spans.clear();
        }
        uint Next() const
        {
            if (Spans.size() == 0) {
                return 0;
            }
            iterator last = Spans.end();
            --last;
            return last->first + last->second;
        }
    };
    // Keyed as UniqueSeeds[baseName][digitCount][digitValue] iff that seed is taken.
    // We need the double-indexing so that Name01 and Name001 can both be indexed, although we only
    // ever allocate off the longest for each name i.e. UniqueSeeds[baseName].size()-1 digits.
    std::map<std::string, std::vector<PiecewiseSparseIntegerSet>> UniqueSeeds;

public:
    std::tuple<uint, uint> decomposeName(const std::string& name,
                                         std::string& baseNameOut,
                                         std::string& nameSuffixOut) const;

    UniqueNameManager()
    {}

    // Register a name in the collection. It is an error (detected only by assertions) to register a
    // name more than once. The effect if undetected is that the second registration will have no
    // effect
    void addExactName(const std::string& name);
    std::string makeUniqueName(const std::string& modelName, int minDigits = 0) const;

    // Remove a registered name so it can be generated again.
    // Nothing happens if you try to remove a non-registered name.
    void removeExactName(const std::string& name);

    bool containsName(const std::string& name) const;

    void clear()
    {
        UniqueSeeds.clear();
    }
};
struct BaseExport Tools
{
    static std::string getIdentifier(const std::string&);
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


}  // namespace Base

#endif  // BASE_TOOLS_H
