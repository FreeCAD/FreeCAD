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


#ifndef PATH_Path_H
#define PATH_Path_H

#include "Command.h"
#include "Mod/Robot/App/kdl_cp/path_composite.hpp"
#include "Mod/Robot/App/kdl_cp/frames_io.hpp"
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

namespace Path
{

    /** The representation of a CNC Toolpath */
    
    class PathExport Toolpath : public Base::Persistence
    {
        TYPESYSTEM_HEADER();
    
        public:
            Toolpath();
            Toolpath(const Toolpath&);
            ~Toolpath();
            
            Toolpath &operator=(const Toolpath&);
        
            // from base class
            virtual unsigned int getMemSize (void) const;
            virtual void Save (Base::Writer &/*writer*/) const;
            virtual void Restore(Base::XMLReader &/*reader*/);
            void SaveDocFile (Base::Writer &writer) const;
            void RestoreDocFile(Base::Reader &reader);
        
            // interface
            void clear(void); // clears the internal data
            void addCommand(const Command &Cmd); // adds a command at the end
            void insertCommand(const Command &Cmd, int); // inserts a command
            void deleteCommand(int); // deletes a command
            double getLength(void); // return the Length (mm) of the Path
            void recalculate(void); // recalculates the points
            void setFromGCode(const std::string); // sets the path from the contents of the given GCode string
            std::string toGCode(void) const; // gets a gcode string representation from the Path
            
            // shortcut functions
            unsigned int getSize(void) const{return vpcCommands.size();}
            const std::vector<Command*> &getCommands(void)const{return vpcCommands;}
            const Command &getCommand(unsigned int pos)const {return *vpcCommands[pos];}
        
        protected:
            std::vector<Command*> vpcCommands;
            KDL::Path_Composite *pcPath;
            
        inline  KDL::Frame toFrame(const Base::Placement &To){
            return KDL::Frame(KDL::Rotation::Quaternion(To.getRotation()[0],
                                                        To.getRotation()[1],
                                                        To.getRotation()[2],
                                                        To.getRotation()[3]),
                                                        KDL::Vector(To.getPosition()[0],
                                                        To.getPosition()[1],
                                                        To.getPosition()[2]));
        }
        inline  Base::Placement toPlacement(const KDL::Frame &To){
            double x,y,z,w;
            To.M.GetQuaternion(x,y,z,w);
            return Base::Placement(Base::Vector3d(To.p[0],To.p[1],To.p[2]),Base::Rotation(x,y,z,w));
        }
    };

} //namespace Path


#endif // PATH_Path_H
