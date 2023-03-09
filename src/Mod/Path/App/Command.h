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


#ifndef PATH_COMMAND_H
#define PATH_COMMAND_H

#include <map>
#include <string>
#include <Base/Persistence.h>
#include <Base/Placement.h>
#include <Base/Vector3D.h>

namespace Path
{
    /** The representation of a cnc command in a path */
    class PathExport Command : public Base::Persistence
    {
    TYPESYSTEM_HEADER();

    public:
        //constructors
        Command();
        Command(const char* name,
                const std::map<std::string,double>& parameters);
        ~Command();
        // from base class
        virtual unsigned int getMemSize (void) const;
        virtual void Save (Base::Writer &/*writer*/) const;
        virtual void Restore(Base::XMLReader &/*reader*/);

        // specific methods
        // returns a placement from the x,y,z,a,b,c parameters
        Base::Placement getPlacement (const Base::Vector3d pos = Base::Vector3d()) const;
        // returns a 3d vector from the i,j,k parameters
        Base::Vector3d getCenter (void) const;
        // sets the center coordinates and the command name
        void setCenter(const Base::Vector3d&, bool clockwise=true);
        // returns a GCode string representation of the command
        std::string toGCode (int precision=6, bool padzero=true) const;
        // sets the parameters from the contents of the given GCode string
        void setFromGCode (const std::string&);
        // sets the parameters from the contents of the given placement
        void setFromPlacement (const Base::Placement&);
        // returns true if the given string exists in the parameters
        bool has(const std::string&) const;
        // returns a transformed copy of this command
        Command transform(const Base::Placement&);
        // returns the value of a given parameter
        double getValue(const std::string &name) const;
        // scales the receiver - use for imperial/metric conversions
        void scaleBy(double factor);

        // this assumes the name is upper case
        inline double getParam(const std::string &name, double fallback = 0.0) const {
            auto it = Parameters.find(name);
            return it==Parameters.end() ? fallback : it->second;
        }

        // attributes
        std::string Name;
        std::map<std::string,double> Parameters;
    };

} //namespace Path

#endif // PATH_COMMAND_H
