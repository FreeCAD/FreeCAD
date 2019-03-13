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

#include <boost/regex.hpp>

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Exception.h>

// KDL stuff - at the moment, not used
//#include "Mod/Robot/App/kdl_cp/path_line.hpp"
//#include "Mod/Robot/App/kdl_cp/path_circle.hpp"
//#include "Mod/Robot/App/kdl_cp/rotational_interpolation_sa.hpp"
//#include "Mod/Robot/App/kdl_cp/utilities/error.h"

#include "Path.h"

using namespace Path;
using namespace Base;

TYPESYSTEM_SOURCE(Path::Toolpath , Base::Persistence);

Toolpath::Toolpath()
{
}

Toolpath::Toolpath(const Toolpath& otherPath)
    : vpcCommands(otherPath.vpcCommands.size())
    , center(otherPath.center)
{
    *this = otherPath;
    recalculate();
}

Toolpath::~Toolpath()
{
    clear();
}

Toolpath &Toolpath::operator=(const Toolpath& otherPath)
{
    if (this == &otherPath)
        return *this;

    clear();
    vpcCommands.resize(otherPath.vpcCommands.size());
    int i = 0;
    for (std::vector<Command*>::const_iterator it=otherPath.vpcCommands.begin();it!=otherPath.vpcCommands.end();++it,i++) {
        vpcCommands[i] = new Command(**it);
    }
    center = otherPath.center;
    recalculate();
    return *this;
}

void Toolpath::clear(void) 
{
    for(std::vector<Command*>::iterator it = vpcCommands.begin();it!=vpcCommands.end();++it)
        delete ( *it );
    vpcCommands.clear();
    recalculate();
}

void Toolpath::addCommand(const Command &Cmd)
{
    Command *tmp = new Command(Cmd);
    vpcCommands.push_back(tmp);
    recalculate();
}

void Toolpath::insertCommand(const Command &Cmd, int pos)
{
    if (pos == -1) {
        addCommand(Cmd);
    } else if (pos <= static_cast<int>(vpcCommands.size())) {
        Command *tmp = new Command(Cmd);
        vpcCommands.insert(vpcCommands.begin()+pos,tmp);
    } else {
        throw Base::IndexError("Index not in range");
    }
    recalculate();
}

void Toolpath::deleteCommand(int pos)
{
    if (pos == -1) {
        //delete(*vpcCommands.rbegin()); // causes crash
        vpcCommands.pop_back();
    } else if (pos <= static_cast<int>(vpcCommands.size())) {
        vpcCommands.erase (vpcCommands.begin()+pos);
    } else {
        throw Base::IndexError("Index not in range");
    }
    recalculate();
}

double Toolpath::getLength()
{
    if(vpcCommands.size()==0)
        return 0;
    double l = 0;
    Vector3d last(0,0,0);
    Vector3d next;
    for(std::vector<Command*>::const_iterator it = vpcCommands.begin();it!=vpcCommands.end();++it) {
        std::string name = (*it)->Name;
        next = (*it)->getPlacement().getPosition();
        if ( (name == "G0") || (name == "G00") || (name == "G1") || (name == "G01") ) {
            // straight line
            l += (next - last).Length();
            last = next;
        } else if ( (name == "G2") || (name == "G02") || (name == "G3") || (name == "G03") ) {
            // arc
            Vector3d center = (*it)->getCenter();
            double radius = (last - center).Length();
            double angle = (next - center).GetAngle(last - center);
            l += angle * radius;
            last = next;
        }
    }
    return l;
}

static void bulkAddCommand(const std::string &gcodestr, std::vector<Command*> &commands, bool &inches)
{
    Command *cmd = new Command();
    cmd->setFromGCode(gcodestr);
    if ("G20" == cmd->Name) {
        inches = true;
        delete cmd;
    } else if ("G21" == cmd->Name) {
        inches = false;
        delete cmd;
    } else {
        if (inches) {
            cmd->scaleBy(25.4);
        }
        commands.push_back(cmd);
    }
}

void Toolpath::setFromGCode(const std::string instr)
{
    clear();
    
    // remove comments
    //boost::regex e("\\(.*?\\)");
    //std::string str = boost::regex_replace(instr, e, "");
    std::string str(instr);
    
    // split input string by () or G or M commands
    std::string mode = "command";
    std::size_t found = str.find_first_of("(gGmM");
    int last = -1;
    bool inches = false;
    while (found != std::string::npos)
    {
        if (str[found] == '(') {
            // start of comment
            if ( (last > -1) && (mode == "command") ) {
                // before opening a comment, add the last found command
                std::string gcodestr = str.substr(last, found-last);
                bulkAddCommand(gcodestr, vpcCommands, inches);
            }
            mode = "comment";
            last = found;
            found = str.find_first_of(")", found+1);
        } else if (str[found] == ')') {
            // end of comment
            std::string gcodestr = str.substr(last, found-last+1);
            bulkAddCommand(gcodestr, vpcCommands, inches);
            last = -1;
            found = str.find_first_of("(gGmM", found+1);
            mode = "command";
        } else if (mode == "command") {
            // command
            if (last > -1) {
                std::string gcodestr = str.substr(last, found-last);
                bulkAddCommand(gcodestr, vpcCommands, inches);
            }
            last = found;
            found = str.find_first_of("(gGmM", found+1);
        }
    }
    // add the last command found, if any
    if (last > -1) {
        if (mode == "command") {
            std::string gcodestr = str.substr(last,std::string::npos);
            bulkAddCommand(gcodestr, vpcCommands, inches);
        }
    }
    recalculate();
}

std::string Toolpath::toGCode(void) const
{
    std::string result;
    for (std::vector<Command*>::const_iterator it=vpcCommands.begin();it!=vpcCommands.end();++it) {
        result += (*it)->toGCode();
        result += "\n";
    }
    return result;
}    

void Toolpath::recalculate(void) // recalculates the path cache
{
    
    if(vpcCommands.size()==0)
        return;
        
    // TODO recalculate the KDL stuff. At the moment, this is unused.

#if 0
    // delete the old and create a new one
    if(pcPath) 
        delete (pcPath);
        
    pcPath = new KDL::Path_Composite();
    
    KDL::Path *tempPath;
    KDL::Frame Last;

    try {
        // handle the first waypoint differently
        bool first=true;

        for(std::vector<Command*>::const_iterator it = vpcCommands.begin();it!=vpcCommands.end();++it) {
            if(first){
                Last = toFrame((*it)->getPlacement());
                first = false;
            }else{
                Base::Placement p = (*it)->getPlacement();
                KDL::Frame Next = toFrame(p);
                std::string name = (*it)->Name;
                Vector3d zaxis(0,0,1);

                if ( (name == "G0") || (name == "G1") || (name == "G01") ) {
                    // line segment
                    tempPath = new KDL::Path_Line(Last, Next, new KDL::RotationalInterpolation_SingleAxis(), 1.0, true);
                    pcPath->Add(tempPath);
                    Last = Next;
                } else if ( (name == "G2") || (name == "G02") ) {
                    // clockwise arc
                    Vector3d fcenter = (*it)->getCenter();
                    KDL::Vector center(fcenter.x,fcenter.y,fcenter.z);
                    Vector3d fnorm;
                    p.getRotation().multVec(zaxis,fnorm);
                    KDL::Vector norm(fnorm.x,fnorm.y,fnorm.z);
                    Vector3d fstart = toPlacement(Last).getPosition();
                    Vector3d fend = toPlacement(Last).getPosition();
                    Rotation frot(fstart-fcenter,fend-fcenter);
                    double q0,q1,q2,q3;
                    frot.getValue(q0,q1,q2,q3);
                    KDL::Rotation rot;
                    rot.Quaternion(q0,q1,q2,q3);
                    tempPath = new KDL::Path_Circle(Last, center, norm, rot, 0.0, new KDL::RotationalInterpolation_SingleAxis(), 1.0, true);
                    pcPath->Add(tempPath);
                    Last = Next;
                }
            }
        }
    } catch (KDL::Error &e) {
        throw Base::RuntimeError(e.Description());
    }
#endif
}

// reimplemented from base class

unsigned int Toolpath::getMemSize (void) const
{
    return toGCode().size();
}

void Toolpath::setCenter(const Base::Vector3d &c)
{
    center = c;
    recalculate();
}

static void saveCenter(Writer &writer, const Base::Vector3d &center)
{
    writer.Stream() << writer.ind() << "<Center x=\"" << center.x << "\" y=\"" << center.y << "\" z=\"" << center.z << "\"/>" << std::endl;
}

void Toolpath::Save (Writer &writer) const
{
    if (writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<Path count=\"" <<  getSize() << "\" version=\"" << SchemaVersion << "\">" << std::endl;
        writer.incInd();
        saveCenter(writer, center);
        for(unsigned int i = 0; i < getSize(); i++) {
            vpcCommands[i]->Save(writer);
        }
        writer.decInd();
    } else {
        writer.Stream() << writer.ind()
            << "<Path file=\"" << writer.addFile((writer.ObjectName+".nc").c_str(), this) << "\" version=\"" << SchemaVersion << "\">" << std::endl;
        writer.incInd();
        saveCenter(writer, center);
        writer.decInd();
    }
    writer.Stream() << writer.ind() << "</Path>" << std::endl;
}

void Toolpath::SaveDocFile (Base::Writer &writer) const
{
    if (toGCode().empty())
        return;
    writer.Stream() << toGCode();
}

void Toolpath::Restore(XMLReader &reader)
{
    reader.readElement("Path");
    std::string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(),this);
    }
}

void Toolpath::RestoreDocFile(Base::Reader &reader)
{
    std::string gcode;
    std::string line;
    while (reader >> line) { 
        gcode += line;
        gcode += " ";
    }
    setFromGCode(gcode);

}




 
