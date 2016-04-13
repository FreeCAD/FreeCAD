/***************************************************************************
*   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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


#include <iostream>
#include <fstream>

#include <Base/Console.h>
#include <Base/FileInfo.h>

#include "TestJtReader.h"
#include "FcLodHandler.h"


TestJtReader::TestJtReader()
{
}


TestJtReader::~TestJtReader()
{
}

void TestJtReader::read(void)
{
	//const std::vector<TOC_Entry>& toc = readToc();

	for (std::vector<TOC_Entry>::const_iterator i = TocEntries.begin(); i != TocEntries.end(); ++i){
		int segType = i->getSegmentType();

		if (segType == 7){
			FcLodHandler handler;

			readLodSegment(*i, handler);


		}
			

		Base::Console().Log(i->toString().c_str());
	}




}
