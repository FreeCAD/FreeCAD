/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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

#endif
#include <cinttypes>
#include <iomanip>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <Base/Vector3D.h>
#include <Base/Rotation.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include "Command.h"

using namespace Base;
using namespace Path;

TYPESYSTEM_SOURCE(Path::Command , Base::Persistence);

// Constructors & destructors

Command::Command(const char* name,
                 const std::map<std::string, double>& parameters)
:Name(name),Parameters(parameters)
{
}

Command::Command()
{
}

Command::~Command()
{
}

// New methods

Placement Command::getPlacement (void) const
{
    static const std::string x = "X";
    static const std::string y = "Y";
    static const std::string z = "Z";
    static const std::string a = "A";
    static const std::string b = "B";
    static const std::string c = "C";
    Vector3d vec(getParam(x),getParam(y),getParam(z));
    Rotation rot;
    rot.setYawPitchRoll(getParam(a),getParam(b),getParam(c));
    Placement plac(vec,rot);
    return plac;
}

Vector3d Command::getCenter (void) const
{
    static const std::string i = "I";
    static const std::string j = "J";
    static const std::string k = "K";
    Vector3d vec(getParam(i),getParam(j),getParam(k));
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
    return Parameters.count(a) > 0;
}

std::string Command::toGCode (int precision, bool padzero) const
{
    std::stringstream str;
    str.fill('0');
    str << Name;
    if(precision<0) 
        precision = 0;
    double scale = std::pow(10.0,precision+1);
    std::int64_t iscale = static_cast<std::int64_t>(scale)/10;
    for(std::map<std::string,double>::const_iterator i = Parameters.begin(); i != Parameters.end(); ++i) {
        if(i->first == "N") continue;

        str << " " << i->first;

        std::int64_t v = static_cast<std::int64_t>(i->second*scale);
        if(v<0) {
            v = -v;
            str << '-'; //shall we allow -0 ?
        }
        v+=5;
        v /= 10;
        str << (v/iscale);
        if(!precision) continue;

        int width = precision;
        std::int64_t digits = v%iscale;
        if(!padzero) {
            if(!digits) continue;
            while(digits%10 == 0) {
                digits/=10;
                --width;
            }
        }
        str << '.' << std::setw(width) << std::right << digits;
    }
    return str.str();
}

void Command::setFromGCode (const std::string& str)
{
    Parameters.clear();
    std::string mode = "none";
    std::string key;
    std::string value;
    for (unsigned int i=0; i < str.size(); i++) {
        if ( (isdigit(str[i])) || (str[i] == '-') || (str[i] == '.') ) {
            value += str[i];
        } else if (isalpha(str[i])) {
            if (mode == "command") {
                if (!key.empty() && !value.empty()) {
                    std::string cmd = key + value;
                    boost::to_upper(cmd);
                    Name = cmd;
                    key = "";
                    value = "";
                    mode = "argument";
                } else {
                    throw Base::BadFormatError("Badly formatted GCode command");
                }
                mode = "argument";
            } else if (mode == "none") {
                mode = "command";
            } else if (mode == "argument") {
                if (!key.empty() && !value.empty()) {
                    double val = std::atof(value.c_str());
                    boost::to_upper(key);
                    Parameters[key] = val;
                    key = "";
                    value = "";
                } else {
                    throw Base::BadFormatError("Badly formatted GCode argument");
                }
            } else if (mode == "comment") {
                value += str[i];
            }
            key = str[i];
        } else if (str[i] == '(') {
            mode = "comment";
        } else if (str[i] == ')') {
            key = "(";
            value += ")";
        } else {
            // add non-ascii characters only if this is a comment
            if (mode == "comment") {
                value += str[i];
            }
        }
    }
    if (!key.empty() && !value.empty()) {
        if ( (mode == "command") || (mode == "comment") ) {
            std::string cmd = key + value;
            if (mode == "command")
                boost::to_upper(cmd);
            Name = cmd;
        } else {
            double val = std::atof(value.c_str());
            boost::to_upper(key);
            Parameters[key] = val;
        }
    } else {
        throw Base::BadFormatError("Badly formatted GCode argument");
    }
}

void Command::setFromPlacement (const Base::Placement &plac)
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
    plac.getRotation().getYawPitchRoll(aval,bval,cval);
    if (xval != 0.0)
        Parameters[x] = xval;
    if (yval != 0.0)
        Parameters[y] = yval;
    if (zval != 0.0)
        Parameters[z] = zval;
    if (aval != 0.0)
        Parameters[a] = aval;
    if (bval != 0.0)
        Parameters[b] = bval;
    if (cval != 0.0)
        Parameters[c] = cval;
}

void Command::setCenter(const Base::Vector3d &pos, bool clockwise)
{
    if (clockwise) {
        Name = "G2";
    } else {
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

Command Command::transform(const Base::Placement other)
{
    Base::Placement plac = getPlacement();
    plac *= other;
    double xval, yval, zval, aval, bval, cval;
    xval = plac.getPosition().x;
    yval = plac.getPosition().y;
    zval = plac.getPosition().z;
    plac.getRotation().getYawPitchRoll(aval,bval,cval);
    Command c = Command();
    c.Name = Name;
    for(std::map<std::string,double>::const_iterator i = Parameters.begin(); i != Parameters.end(); ++i) {
        std::string k = i->first;
        double v = i->second;
        if (k == "X")
            v = xval;
        if (k == "Y")
            v = yval;
        if (k == "Z")
            v = zval;
        if (k == "A")
            v = aval;
        if (k == "B")
            v = bval;
        if (k == "C")
            v = cval;
        c.Parameters[k] = v;
    }
    return c;
}

void Command::scaleBy(double factor)
{
    for(std::map<std::string, double>::const_iterator i = Parameters.begin(); i != Parameters.end(); ++i) {
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

// Reimplemented from base class

unsigned int Command::getMemSize (void) const
{
    return toGCode().size();
}

void Command::Save (Writer &writer) const
{
    // this will only get used if saved as XML (probably never)
    writer.Stream() << writer.ind() << "<Command "
                    << "gcode=\"" << toGCode() << "\" />";
    writer.Stream()<< std::endl;
}

void Command::Restore(XMLReader &reader)
{
    reader.readElement("Command");
    std::string gcode = reader.getAttribute("gcode");
    setFromGCode(gcode);
}

