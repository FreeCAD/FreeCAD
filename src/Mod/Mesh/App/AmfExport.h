/***************************************************************************
 *   Copyright (c) Ian Rees                  <ian.rees@gmail.com>          *
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

#ifndef MESH_AMFEXPORTER_H
#define MESH_AMFEXPORTER_H

#include "Core/MeshKernel.h"

#ifndef _PreComp_
    #include <ostream>
#endif  //  #ifndef _PreComp_

namespace Mesh
{

/// Used for exporting Additive Manufacturing Format (AMF) files
/*!
 * The constructor and destructor write the beginning and end of the AMF,
 * addObject() is used to add... objects!
 */
class AmfExporter
{
    public:
        /// Writes AMF header
        AmfExporter(const char *fileName);

        /// Writes AMF footer
        ~AmfExporter();

        /// Writes an object tag with data from passed-in mesh
        /*!
         * \return -1 on error, or the index of the object in AMF file
         */
        int addObject(const MeshCore::MeshKernel &meshKernel);
        
    private:
        std::ostream *outputStreamPtr;
        int nextObjectIndex;

    /// Helper for putting Base::Vector3f objects into a std::map in addObject()
    class VertLess
    {
        public:
        bool operator()(const Base::Vector3f &a, const Base::Vector3f &b) const
        {
            if (a.x == b.x) {
                if (a.y == b.y) {
                    if (a.z == b.z) {
                        return false;
                    } else {
                        return a.z < b.z;
                    }
                } else {
                    return a.y < b.y;
                }
            } else {
                return a.x < b.x;
            }
        }
    };
};  // class AmfExporter

} // namespace Mesh
#endif // MESH_AMFEXPORTER_H
