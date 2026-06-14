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


#pragma once

#include <map>
#include <string>
#include <variant>
#include <Base/Persistence.h>
#include <Base/Placement.h>
#include <Base/Vector3D.h>
#include <Mod/CAM/PathGlobal.h>

namespace Path
{
/** The representation of a cnc command in a path */
class PathExport Command: public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    // constructors
    Command();
    Command(const char* name, const std::map<std::string, double>& parameters);
    Command(
        const char* name,
        const std::map<std::string, double>& parameters,
        const std::map<std::string, std::variant<std::string, double>>& annotations
    );
    ~Command() override;
    // from base class
    unsigned int getMemSize() const override;
    void Save(Base::Writer& /*writer*/) const override;
    void Restore(Base::XMLReader& /*reader*/) override;

    // specific methods
    Base::Placement getPlacement(
        const Base::Vector3d pos = Base::Vector3d()
    ) const;                           // returns a placement from the x,y,z,a,b,c parameters
    Base::Vector3d getCenter() const;  // returns a 3d vector from the i,j,k parameters
    void setCenter(
        const Base::Vector3d&,
        bool clockwise = true
    );  // sets the center coordinates and the command name
    std::string toGCode(
        int precision = 6,
        bool padzero = true
    ) const;                                // returns a GCode string representation of the command
    void setFromGCode(const std::string&);  // sets the parameters from the contents of the given
                                            // GCode string
    void setFromPlacement(const Base::Placement&);  // sets the parameters from the contents of the
                                                    // given placement
    bool has(const std::string&) const;  // returns true if the given string exists in the parameters
    Command transform(const Base::Placement&);       // returns a transformed copy of this command
    double getValue(const std::string& name) const;  // returns the value of a given parameter
    void scaleBy(double factor);  // scales the receiver - use for imperial/metric conversions

    // annotation methods
    void setAnnotation(
        const std::string& key,
        const std::string& value
    );  // sets a string annotation
    void setAnnotation(const std::string& key,
                       double value);                         // sets a numeric annotation
    std::string getAnnotation(const std::string& key) const;  // gets an annotation value as string
    std::string getAnnotationString(const std::string& key) const;  // gets string annotation
    double getAnnotationDouble(
        const std::string& key,
        double fallback = 0.0
    ) const;  // gets numeric annotation
    std::variant<std::string, double> getAnnotationValue(
        const std::string& key
    ) const;                                                       // gets raw annotation value
    bool hasAnnotation(const std::string& key) const;              // checks if annotation exists
    Command& setAnnotations(const std::string& annotationString);  // sets annotations from string and
                                                                   // returns reference for chaining

    // this assumes the name is upper case
    inline double getParam(const std::string& name, double fallback = 0.0) const
    {
        auto it = Parameters.find(name);
        return it == Parameters.end() ? fallback : it->second;
    }

    // attributes
    std::string Name;
    std::map<std::string, double> Parameters;
    std::map<std::string, std::variant<std::string, double>> Annotations;
};

}  // namespace Path
