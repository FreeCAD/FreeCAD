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


#include "PreCompiled.h"
#ifndef _PreComp_
#include <sstream>
#include <locale>
#include <iostream>
#include <QDateTime>
#endif

#include "PyExport.h"
#include "Interpreter.h"
#include "Tools.h"

void Base::UniqueNameManager::PiecewiseSparseIntegerSet::Add(uint value)
{
    etype newSpan(value, 1);
    iterator above = Spans.lower_bound(newSpan);
    if (above != Spans.end() && above->first <= value) {
        // The found span includes value so there is nothing to do as it is already in the set.
        return;
    }

    // Set below to the next span down, if any
    iterator below;
    if (above == Spans.begin()) {
        below = Spans.end();
    }
    else {
        below = above;
        --below;
    }

    if (above != Spans.end() && below != Spans.end()
        && above->first - below->first + 1 == below->second) {
        // below and above have a gap of exactly one between them, and this must be value
        // so we coalesce the two spans (and the gap) into one.
        newSpan = etype(below->first, below->second + above->second + 1);
        Spans.erase(above);
        above = Spans.erase(below);
    }
    if (below != Spans.end() && value - below->first == below->second) {
        // value is adjacent to the end of below, so just expand below by one
        newSpan = etype(below->first, below->second + 1);
        above = Spans.erase(below);
    }
    else if (above != Spans.end() && above->first - value == 1) {
        // value is adjacent to the start of above, so juse expand above down by one
        newSpan = etype(above->first - 1, above->second + 1);
        above = Spans.erase(above);
    }
    // else  value is not adjacent to any existing span, so just make anew span for it
    Spans.insert(above, newSpan);
}
void Base::UniqueNameManager::PiecewiseSparseIntegerSet::Remove(uint value)
{
    etype newSpan(value, 1);
    iterator at = Spans.lower_bound(newSpan);
    if (at == Spans.end() || at->first > value) {
        // The found span does not include value so there is nothing to do, as it is already not in
        // the set.
        return;
    }
    if (at->second == 1) {
        // value is the only in this span, just remove the span
        Spans.erase(at);
    }
    else if (at->first == value) {
        // value is the first in this span, trim the lower end
        etype replacement(at->first + 1, at->second - 1);
        Spans.insert(Spans.erase(at), replacement);
    }
    else if (value - at->first == at->second - 1) {
        // value is the last in this span, trim the upper end
        etype replacement(at->first, at->second - 1);
        Spans.insert(Spans.erase(at), replacement);
    }
    else {
        // value is in the moddle of the span, so we must split it.
        etype firstReplacement(at->first, value - at->first);
        etype secondReplacement(value + 1, at->second - ((value + 1) - at->first));
        // Because erase returns the iterator after the erased element, and insert returns the
        // iterator for the inserted item, we want to insert secondReplacement first.
        Spans.insert(Spans.insert(Spans.erase(at), secondReplacement), firstReplacement);
    }
}
bool Base::UniqueNameManager::PiecewiseSparseIntegerSet::Contains(uint value) const
{
    iterator at = Spans.lower_bound(etype(value, 1));
    return at != Spans.end() && at->first <= value;
}

std::tuple<uint, uint> Base::UniqueNameManager::decomposeName(const std::string& name,
                                                              std::string& baseNameOut,
                                                              std::string& nameSuffixOut) const
{
    std::string::const_iterator suffixStart = GetNameSuffixStartPosition(name);
    nameSuffixOut = name.substr(suffixStart - name.cbegin());
    std::string::const_iterator digitsStart = suffixStart;
    while (digitsStart > name.cbegin() && isdigit(digitsStart[-1])) {
        --digitsStart;
    }
    baseNameOut = name.substr(0, digitsStart - name.cbegin());
    uint digitCount = suffixStart - digitsStart;
    if (digitCount == 0) {
        // No digits in name
        return std::tuple<uint, uint> {0, 0};
    }
    else {
        return std::tuple<uint, uint> {
            digitCount,
            std::stol(name.substr(digitsStart - name.cbegin(), digitCount))};
    }
}
void Base::UniqueNameManager::addExactName(const std::string& name)
{
    std::string baseName;
    std::string nameSuffix;
    uint digitCount;
    uint digitsValue;
    std::tie(digitCount, digitsValue) = decomposeName(name, baseName, nameSuffix);
    baseName += nameSuffix;
    auto baseNameEntry = UniqueSeeds.find(baseName);
    if (baseNameEntry == UniqueSeeds.end()) {
        // First use of baseName
        baseNameEntry = UniqueSeeds
                            .insert(std::pair<std::string, std::vector<PiecewiseSparseIntegerSet>>(
                                baseName,
                                std::vector<PiecewiseSparseIntegerSet>()))
                            .first;
    }
    if (digitCount >= baseNameEntry->second.size()) {
        // First use of this digitCount
        baseNameEntry->second.resize(digitCount + 1);
    }
    PiecewiseSparseIntegerSet& baseNameAndDigitCountEntry = baseNameEntry->second[digitCount];
    // Name should not already be there
    assert(!baseNameAndDigitCountEntry.Contains(digitsValue));
    baseNameAndDigitCountEntry.Add(digitsValue);
}
std::string Base::UniqueNameManager::makeUniqueName(const std::string& modelName,
                                                    int minDigits) const
{
    std::string namePrefix;
    std::string nameSuffix;
    decomposeName(modelName, namePrefix, nameSuffix);
    std::string baseName = namePrefix + nameSuffix;
    auto baseNameEntry = UniqueSeeds.find(baseName);
    if (baseNameEntry == UniqueSeeds.end()) {
        // First use of baseName, just return it with no unique digits
        return baseName;
    }
    // We don't care about the digit count of the suggested name, we always use at least the most
    // digits ever used before.
    int digitCount = baseNameEntry->second.size() - 1;
    uint digitsValue;
    if (digitCount < minDigits) {
        // Caller is asking for more digits than we have in any registered name.
        // We start the longer digit string at 000...0001 even though we might have shorter strings
        // with larger numeric values.
        digitCount = minDigits;
        digitsValue = 1;
    }
    else {
        digitsValue = baseNameEntry->second[digitCount].Next();
    }
    std::string digits = std::to_string(digitsValue);
    if (digitCount > digits.size()) {
        namePrefix += std::string(digitCount - digits.size(), '0');
    }
    return namePrefix + digits + nameSuffix;
}

// Remove a rgistered name so it can be generated again.
// Nothing happens if you try to remove a non-registered name.
void Base::UniqueNameManager::removeExactName(const std::string& name)
{
    std::string baseName;
    std::string nameSuffix;
    uint digitCount;
    uint digitsValue;
    std::tie(digitCount, digitsValue) = decomposeName(name, baseName, nameSuffix);
    baseName += nameSuffix;
    auto baseNameEntry = UniqueSeeds.find(baseName);
    if (baseNameEntry == UniqueSeeds.end()) {
        // name must not be registered, so nothing to do.
        return;
    }
    int maxDigitCount = baseNameEntry->second.size();
    if (digitCount >= maxDigitCount) {
        // First use of this digitCount, name must not be registered, so nothing to do.
        return;
    }
    PiecewiseSparseIntegerSet& baseNameAndDigitCountEntry = baseNameEntry->second[digitCount];
    baseNameAndDigitCountEntry.Remove(digitsValue);
    // Prune empty vector entries
    while (--maxDigitCount >= 0 && !baseNameEntry->second[maxDigitCount].Any())
        ;
    if (maxDigitCount < 0) {
        UniqueSeeds.erase(baseName);
    }
    else {
        baseNameEntry->second.resize(maxDigitCount + 1);
    }
}

bool Base::UniqueNameManager::containsName(const std::string& name) const
{
    std::string baseName;
    std::string nameSuffix;
    uint digitCount;
    uint digitsValue;
    std::tie(digitCount, digitsValue) = decomposeName(name, baseName, nameSuffix);
    baseName += nameSuffix;
    auto baseNameEntry = UniqueSeeds.find(baseName);
    if (baseNameEntry == UniqueSeeds.end()) {
        // base name is not registered
        return false;
    }
    if (digitCount >= baseNameEntry->second.size()) {
        // First use of this digitCount, name must not be registered, so not in collection
        return false;
    }
    return baseNameEntry->second[digitCount].Contains(digitsValue);
}
std::string Base::Tools::getIdentifier(const std::string& name)
{
    if (name.empty()) {
        return "_";
    }
    // check for first character whether it's a digit
    std::string CleanName = name;
    if (!CleanName.empty() && CleanName[0] >= 48 && CleanName[0] <= 57) {
        CleanName[0] = '_';
    }
    // strip illegal chars
    for (char& it : CleanName) {
        if (!((it >= 48 && it <= 57) ||    // number
              (it >= 65 && it <= 90) ||    // uppercase letter
              (it >= 97 && it <= 122))) {  // lowercase letter
            it = '_';                      // it's neither number nor letter
        }
    }

    return CleanName;
}

std::wstring Base::Tools::widen(const std::string& str)
{
    std::wostringstream wstm;
    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(wstm.getloc());
    for (char i : str) {
        wstm << ctfacet.widen(i);
    }
    return wstm.str();
}

std::string Base::Tools::narrow(const std::wstring& str)
{
    std::ostringstream stm;
    const std::ctype<char>& ctfacet = std::use_facet<std::ctype<char>>(stm.getloc());
    for (wchar_t i : str) {
        stm << ctfacet.narrow(i, 0);
    }
    return stm.str();
}

std::string Base::Tools::escapedUnicodeFromUtf8(const char* s)
{
    Base::PyGILStateLocker lock;
    std::string escapedstr;

    PyObject* unicode = PyUnicode_FromString(s);
    if (!unicode) {
        return escapedstr;
    }

    PyObject* escaped = PyUnicode_AsUnicodeEscapeString(unicode);
    if (escaped) {
        escapedstr = std::string(PyBytes_AsString(escaped));
        Py_DECREF(escaped);
    }

    Py_DECREF(unicode);
    return escapedstr;
}

std::string Base::Tools::escapedUnicodeToUtf8(const std::string& s)
{
    Base::PyGILStateLocker lock;
    std::string string;

    PyObject* unicode =
        PyUnicode_DecodeUnicodeEscape(s.c_str(), static_cast<Py_ssize_t>(s.size()), "strict");
    if (!unicode) {
        return string;
    }
    if (PyUnicode_Check(unicode)) {
        string = PyUnicode_AsUTF8(unicode);
    }
    Py_DECREF(unicode);
    return string;
}

std::string Base::Tools::escapeQuotesFromString(const std::string& s)
{
    std::string result;
    size_t len = s.size();
    for (size_t i = 0; i < len; ++i) {
        switch (s.at(i)) {
            case '\"':
                result += "\\\"";
                break;
            case '\'':
                result += "\\\'";
                break;
            default:
                result += s.at(i);
                break;
        }
    }
    return result;
}

QString Base::Tools::escapeEncodeString(const QString& s)
{
    QString result;
    const int len = s.length();
    result.reserve(int(len * 1.1));
    for (int i = 0; i < len; ++i) {
        if (s.at(i) == QLatin1Char('\\')) {
            result += QLatin1String("\\\\");
        }
        else if (s.at(i) == QLatin1Char('\"')) {
            result += QLatin1String("\\\"");
        }
        else if (s.at(i) == QLatin1Char('\'')) {
            result += QLatin1String("\\\'");
        }
        else {
            result += s.at(i);
        }
    }
    result.squeeze();
    return result;
}

std::string Base::Tools::escapeEncodeString(const std::string& s)
{
    std::string result;
    size_t len = s.size();
    for (size_t i = 0; i < len; ++i) {
        switch (s.at(i)) {
            case '\\':
                result += "\\\\";
                break;
            case '\"':
                result += "\\\"";
                break;
            case '\'':
                result += "\\\'";
                break;
            default:
                result += s.at(i);
                break;
        }
    }
    return result;
}

QString Base::Tools::escapeEncodeFilename(const QString& s)
{
    QString result;
    const int len = s.length();
    result.reserve(int(len * 1.1));
    for (int i = 0; i < len; ++i) {
        if (s.at(i) == QLatin1Char('\"')) {
            result += QLatin1String("\\\"");
        }
        else if (s.at(i) == QLatin1Char('\'')) {
            result += QLatin1String("\\\'");
        }
        else {
            result += s.at(i);
        }
    }
    result.squeeze();
    return result;
}

std::string Base::Tools::escapeEncodeFilename(const std::string& s)
{
    std::string result;
    size_t len = s.size();
    for (size_t i = 0; i < len; ++i) {
        switch (s.at(i)) {
            case '\"':
                result += "\\\"";
                break;
            case '\'':
                result += "\\\'";
                break;
            default:
                result += s.at(i);
                break;
        }
    }
    return result;
}

std::string Base::Tools::quoted(const char* name)
{
    std::stringstream str;
    str << "\"" << name << "\"";
    return str.str();
}

std::string Base::Tools::quoted(const std::string& name)
{
    std::stringstream str;
    str << "\"" << name << "\"";
    return str.str();
}

std::string Base::Tools::joinList(const std::vector<std::string>& vec, const std::string& sep)
{
    std::stringstream str;
    for (const auto& it : vec) {
        str << it << sep;
    }
    return str.str();
}

std::string Base::Tools::currentDateTimeString()
{
    return QDateTime::currentDateTime()
        .toTimeSpec(Qt::OffsetFromUTC)
        .toString(Qt::ISODate)
        .toStdString();
}

std::vector<std::string> Base::Tools::splitSubName(const std::string& subname)
{
    // Turns 'Part.Part001.Body.Pad.Edge1'
    // Into ['Part', 'Part001', 'Body', 'Pad', 'Edge1']
    std::vector<std::string> subNames;
    std::string subName;
    std::istringstream subNameStream(subname);
    while (std::getline(subNameStream, subName, '.')) {
        subNames.push_back(subName);
    }

    // Check if the last character of the input string is the delimiter.
    // If so, add an empty string to the subNames vector.
    // Because the last subname is the element name and can be empty.
    if (!subname.empty() && subname.back() == '.') {
        subNames.push_back("");  // Append empty string for trailing dot.
    }

    return subNames;
}

// ------------------------------------------------------------------------------------------------

void Base::ZipTools::rewrite(const std::string& source, const std::string& target)
{
    Base::PyGILStateLocker lock;
    PyObject* module = PyImport_ImportModule("freecad.utils_zip");
    if (!module) {
        throw Py::Exception();
    }

    Py::Module commands(module, true);
    commands.callMemberFunction("rewrite", Py::TupleN(Py::String(source), Py::String(target)));
}
