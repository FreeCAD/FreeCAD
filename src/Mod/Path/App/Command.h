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
        Base::Placement getPlacement (void); // returns a placement from the x,y,z,a,b,c parameters
        Base::Vector3d getCenter (void); // returns a 3d vector from the i,j,k parameters
        void setCenter(const Base::Vector3d&, bool clockwise=true); // sets the center coordinates and the command name
        std::string toGCode (void) const; // returns a GCode string representation of the command
        void setFromGCode (std::string); // sets the parameters from the contents of the given GCode string
        void setFromPlacement (const Base::Placement&); // sets the parameters from the contents of the given placement
        const bool has(const std::string); // returns true if the given string exists in the parameters
        Command transform(const Base::Placement); // returns a transformed copy of this command
        const double getValue(const std::string); // returns the value of a given parameter

        // attributes
        std::string Name;
        std::map<std::string,double> Parameters;
    };
    
} //namespace Path

#endif // PATH_COMMAND_H
