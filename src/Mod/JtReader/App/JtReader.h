// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2007 Juergen Riegel <juergen.riegel@web.de>                            *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef __JtReader_h__
#define __JtReader_h__


/** simple facet structure */
struct SimpleMeshFacet
{
    float p1[3], p2[3], p3[3], n[3];
};

/** Reads a JT File an build up the internal data structure
 * imports all the meshes of all Parts, recursing the Assamblies.
 */
void readFile(const char* FileName, int iLods = 0);

/** Write the read Part to a file */
void writeAsciiSTL(const char* FileName);

/** start the iterator on the result */
const SimpleMeshFacet* iterStart(void);

/** get the faces or 0 at end */
const SimpleMeshFacet* iterGetNext(void);

/** size of the result */
unsigned int iterSize(void);

/** clears the internal structure */
void clearData(void);

#endif  // __JtReader_h__
