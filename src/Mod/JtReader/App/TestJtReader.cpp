// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2014 Juergen Riegel <juergen.riegel@web.de>                            *
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


#include <vector>


#include <Base/Console.h>
#include <Base/FileInfo.h>

#include "FcLodHandler.h"
#include "TestJtReader.h"


TestJtReader::TestJtReader()
{}


TestJtReader::~TestJtReader()
{}

void TestJtReader::read(void)
{
    // const std::vector<TOC_Entry>& toc = readToc();

    for (std::vector<TOC_Entry>::const_iterator i = TocEntries.begin(); i != TocEntries.end(); ++i) {
        int segType = i->getSegmentType();

        if (segType == 7) {
            FcLodHandler handler;

            readLodSegment(*i, handler);
        }


        Base::Console().log(i->toString().c_str());
    }
}
