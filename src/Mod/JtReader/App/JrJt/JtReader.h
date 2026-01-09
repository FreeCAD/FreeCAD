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


#pragma once

#include "TOC_Entry.h"
#include <string>
#include <vector>

class LodHandler;


class JtReader
{
public:
    JtReader();
    ~JtReader();

    void setFile(const std::string fileName);

    const std::vector<TOC_Entry>& readToc();

    void readSegment(int tocIndex);

    void readLodSegment(const TOC_Entry&, LodHandler&);

    static const GUID TriStripSetShapeLODElement_ID;


protected:
    std::string _fileName;
    vector<TOC_Entry> TocEntries;
};
