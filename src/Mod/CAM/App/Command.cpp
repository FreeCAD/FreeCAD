// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#include <cinttypes>
#include <iomanip>
#include <boost/algorithm/string.hpp>

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>
#include <Base/Writer.h>

#include "Command.h"


using namespace Base;
using namespace Path;

TYPESYSTEM_SOURCE(Path::Command, Base::Persistence)

// Constructors & destructors

Command::Command(const char* name, const std::map<std::string, double>& parameters)
    : Name(name)
    , Parameters(parameters)
    , Annotations()
{}

Command::Command(
    const char* name,
    const std::map<std::string, double>& parameters,
    const std::map<std::string, std::variant<std::string, double>>& annotations
)
    : Name(name)
    , Parameters(parameters)
    , Annotations(annotations)
{}

Command::Command()
    : Annotations()
{}

Command::~Command()
{}

// New methods

Placement Command::getPlacement(const Base::Vector3d pos) const
{
    static const std::string x = "X";
    static const std::string y = "Y";
    static const std::string z = "Z";
    static const std::string a = "A";
    static const std::string b = "B";
    static const std::string c = "C";
    Vector3d vec(getParam(x, pos.x), getParam(y, pos.y), getParam(z, pos.z));
    Rotation rot;
    rot.setYawPitchRoll(getParam(a), getParam(b), getParam(c));
    Placement plac(vec, rot);
    return plac;
}

Vector3d Command::getCenter() const
{
    static const std::string i = "I";
    static const std::string j = "J";
    static const std::string k = "K";
    Vector3d vec(getParam(i), getParam(j), getParam(k));
    return vec;
}

double Command::getValue(const std::string& attr) const
{
    std::string a(attr);
    boost::to_upper(a);
    return getParam(a);
}

bool Command::has(const std::string& attr) const
{
    std::string a(attr);
    boost::to_upper(a);
    return Parameters.contains(a);
}

std::string Command::toGCode(int precision, bool padzero) const
{
    std::stringstream str;
    str.fill('0');
    str << Name;
    if (precision < 0) {
        precision = 0;
    }
    double scale = std::pow(10.0, precision + 1);
    std::int64_t iscale = static_cast<std::int64_t>(scale) / 10;
    for (std::map<std::string, double>::const_iterator i = Parameters.begin(); i != Parameters.end();
         ++i) {
        if (i->first == "N") {
            continue;
        }

        str << " " << i->first;

        std::int64_t v = static_cast<std::int64_t>(i->second * scale);
        if (v < 0) {
            v = -v;
            str << '-';  // shall we allow -0 ?
        }
        v += 5;
        v /= 10;
        str << (v / iscale);
        if (!precision) {
            continue;
        }

        int width = precision;
        std::int64_t digits = v % iscale;
        if (!padzero) {
            if (!digits) {
                continue;
            }
            while (digits % 10 == 0) {
                digits /= 10;
                --width;
            }
        }
        str << '.' << std::setw(width) << std::right << digits;
    }

    // Add annotations as a comment if they exist
    if (!Annotations.empty()) {
        str << " ; ";
        bool first = true;
        for (const auto& pair : Annotations) {
            if (!first) {
                str << " ";
            }
            first = false;
            str << pair.first << ":";
            if (std::holds_alternative<std::string>(pair.second)) {
                str << "'" << std::get<std::string>(pair.second) << "'";
            }
            else if (std::holds_alternative<double>(pair.second)) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(6) << std::get<double>(pair.second);
                str << oss.str();
            }
        }
    }

    return str.str();
}

void Command::setFromGCode(const std::string& str)
{
    Parameters.clear();
    Annotations.clear();

    // Check for annotation comment and split the string
    std::string gcode_part = str;
    std::string annotation_part;

    auto comment_pos = str.find("; ");
    if (comment_pos != std::string::npos) {
        gcode_part = str.substr(0, comment_pos);
        annotation_part = str.substr(comment_pos + 1);  // length of "; "
    }

    std::string mode = "none";
    std::string key;
    std::string value;
    for (unsigned int i = 0; i < gcode_part.size(); i++) {
        if ((isdigit(gcode_part[i])) || (gcode_part[i] == '-') || (gcode_part[i] == '.')) {
            value += gcode_part[i];
        }
        else if (isalpha(gcode_part[i])) {
            if (mode == "command") {
                if (!key.empty() && !value.empty()) {
                    std::string cmd = key + value;
                    boost::to_upper(cmd);
                    Name = cmd;
                    key = "";
                    value = "";
                    mode = "argument";
                }
                else {
                    throw Base::BadFormatError("Badly formatted GCode command");
                }
                mode = "argument";
            }
            else if (mode == "none") {
                mode = "command";
            }
            else if (mode == "argument") {
                if (!key.empty() && !value.empty()) {
                    double val = std::atof(value.c_str());
                    boost::to_upper(key);
                    Parameters[key] = val;
                    key = "";
                    value = "";
                }
                else {
                    throw Base::BadFormatError("Badly formatted GCode argument");
                }
            }
            else if (mode == "comment") {
                value += gcode_part[i];
            }
            key = gcode_part[i];
        }
        else if (gcode_part[i] == '(') {
            mode = "comment";
        }
        else if (gcode_part[i] == ')') {
            key = "(";
            value += ")";
        }
        else {
            // add non-ascii characters only if this is a comment
            if (mode == "comment") {
                value += gcode_part[i];
            }
        }
    }

    // Parse annotations if found
    if (!annotation_part.empty()) {
        setAnnotations(annotation_part);
    }

    if (!key.empty() && !value.empty()) {
        if ((mode == "command") || (mode == "comment")) {
            std::string cmd = key + value;
            if (mode == "command") {
                boost::to_upper(cmd);
            }
            Name = cmd;
        }
        else {
            double val = std::atof(value.c_str());
            boost::to_upper(key);
            Parameters[key] = val;
        }
    }
    else {
        throw Base::BadFormatError("Badly formatted GCode argument");
    }
}

void Command::setFromPlacement(const Base::Placement& plac)
{
    Name = "G1";
    Parameters.clear();
    static const std::string x = "X";
    static const std::string y = "Y";
    static const std::string z = "Z";
    static const std::string a = "A";
    static const std::string b = "B";
    static const std::string c = "C";
    double xval, yval, zval, aval, bval, cval;
    xval = plac.getPosition().x;
    yval = plac.getPosition().y;
    zval = plac.getPosition().z;
    plac.getRotation().getYawPitchRoll(aval, bval, cval);
    if (xval != 0.0) {
        Parameters[x] = xval;
    }
    if (yval != 0.0) {
        Parameters[y] = yval;
    }
    if (zval != 0.0) {
        Parameters[z] = zval;
    }
    if (aval != 0.0) {
        Parameters[a] = aval;
    }
    if (bval != 0.0) {
        Parameters[b] = bval;
    }
    if (cval != 0.0) {
        Parameters[c] = cval;
    }
}

void Command::setCenter(const Base::Vector3d& pos, bool clockwise)
{
    if (clockwise) {
        Name = "G2";
    }
    else {
        Name = "G3";
    }
    static const std::string i = "I";
    static const std::string j = "J";
    static const std::string k = "K";
    double ival, jval, kval;
    ival = pos.x;
    jval = pos.y;
    kval = pos.z;
    Parameters[i] = ival;
    Parameters[j] = jval;
    Parameters[k] = kval;
}

Command Command::transform(const Base::Placement& other)
{
    Base::Placement plac = getPlacement();
    plac *= other;
    double xval, yval, zval, aval, bval, cval;
    xval = plac.getPosition().x;
    yval = plac.getPosition().y;
    zval = plac.getPosition().z;
    plac.getRotation().getYawPitchRoll(aval, bval, cval);
    Command c = Command();
    c.Name = Name;
    for (std::map<std::string, double>::const_iterator i = Parameters.begin(); i != Parameters.end();
         ++i) {
        std::string k = i->first;
        double v = i->second;
        if (k == "X") {
            v = xval;
        }
        if (k == "Y") {
            v = yval;
        }
        if (k == "Z") {
            v = zval;
        }
        if (k == "A") {
            v = aval;
        }
        if (k == "B") {
            v = bval;
        }
        if (k == "C") {
            v = cval;
        }
        c.Parameters[k] = v;
    }
    return c;
}

void Command::scaleBy(double factor)
{
    for (std::map<std::string, double>::const_iterator i = Parameters.begin(); i != Parameters.end();
         ++i) {
        switch (i->first[0]) {
            case 'X':
            case 'Y':
            case 'Z':
            case 'I':
            case 'J':
            case 'R':
            case 'Q':
            case 'F':
                Parameters[i->first] = i->second * factor;
                break;
        }
    }
}

void Command::setAnnotation(const std::string& key, const std::string& value)
{
    Annotations[key] = value;
}

void Command::setAnnotation(const std::string& key, double value)
{
    Annotations[key] = value;
}

std::string Command::getAnnotation(const std::string& key) const
{
    auto it = Annotations.find(key);
    if (it != Annotations.end()) {
        if (std::holds_alternative<std::string>(it->second)) {
            return std::get<std::string>(it->second);
        }
        else if (std::holds_alternative<double>(it->second)) {
            return std::to_string(std::get<double>(it->second));
        }
    }
    return "";
}

std::string Command::getAnnotationString(const std::string& key) const
{
    auto it = Annotations.find(key);
    if (it != Annotations.end() && std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }
    return "";
}

double Command::getAnnotationDouble(const std::string& key, double fallback) const
{
    auto it = Annotations.find(key);
    if (it != Annotations.end() && std::holds_alternative<double>(it->second)) {
        return std::get<double>(it->second);
    }
    return fallback;
}

std::variant<std::string, double> Command::getAnnotationValue(const std::string& key) const
{
    auto it = Annotations.find(key);
    if (it != Annotations.end()) {
        return it->second;
    }
    return std::string("");
}

bool Command::hasAnnotation(const std::string& key) const
{
    return Annotations.find(key) != Annotations.end();
}

Command& Command::setAnnotations(const std::string& annotationString)
{
    std::istringstream iss(annotationString);
    std::string token;
    while (iss >> token) {
        auto pos = token.find(':');
        if (pos != std::string::npos) {
            std::string key = token.substr(0, pos);
            std::string value = token.substr(pos + 1);

            // If value starts and ends with single quote, treat as string
            if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
                Annotations[key] = value.substr(1, value.size() - 2);
            }
            else {
                // Try to parse as double, if successful store as double, otherwise as string
                try {
                    size_t processed = 0;
                    double numValue = std::stod(value, &processed);
                    if (processed == value.length()) {
                        Annotations[key] = numValue;
                    }
                    else {
                        Annotations[key] = value;
                    }
                }
                catch (const std::exception&) {
                    Annotations[key] = value;
                }
            }
        }
    }
    return *this;
}

unsigned int Command::getMemSize() const
{
    return toGCode().size();
}

void Command::Save(Base::Writer& writer) const
{
    // this will only get used if saved as XML (probably never)
    writer.Stream() << writer.ind() << "<Command gcode=\"" << toGCode() << "\"";

    if (!Annotations.empty()) {
        writer.Stream() << " annotations=\"";
        bool first = true;
        for (const auto& pair : Annotations) {
            if (!first) {
                writer.Stream() << " ";
            }
            first = false;
            writer.Stream() << pair.first << ":";
            if (std::holds_alternative<std::string>(pair.second)) {
                writer.Stream() << "'" << std::get<std::string>(pair.second) << "'";
            }
            else if (std::holds_alternative<double>(pair.second)) {
                writer.Stream() << std::fixed << std::setprecision(6)
                                << std::get<double>(pair.second);
            }
        }
        writer.Stream() << "\"";
    }

    writer.Stream() << " />" << std::endl;
}

void Command::Restore(Base::XMLReader& reader)
{
    reader.readElement("Command");
    std::string gcode = reader.getAttribute<const char*>("gcode");
    setFromGCode(gcode);

    Annotations.clear();

    std::string attr;
    try {
        attr = reader.getAttribute<const char*>("annotations");
    }
    catch (...) {
        return;  // No annotations
    }

    std::istringstream iss(attr);
    std::string token;
    while (iss >> token) {
        auto pos = token.find(':');
        if (pos != std::string::npos) {
            std::string key = token.substr(0, pos);
            std::string value = token.substr(pos + 1);

            // If value starts and ends with single quote, treat as string
            if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
                Annotations[key] = value.substr(1, value.size() - 2);
            }
            else {
                try {
                    double d = std::stod(value);
                    Annotations[key] = d;
                }
                catch (...) {
                    Annotations[key] = value;
                }
            }
        }
    }
}
