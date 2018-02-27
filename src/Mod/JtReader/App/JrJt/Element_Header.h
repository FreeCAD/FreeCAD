/***************************************************************************
*   Copyright (c) Juergen Riegel         (juergen.riegel@web.de) 2014     *
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

#ifndef Element_Header_HEADER
#define Element_Header_HEADER

#include <istream>
#include <stdint.h>
#include <assert.h>

#include "Context.h"
#include "GUID.h"
#include "UChar.h"
#include "I32.h"



using namespace std;


struct Element_Header
{
	Element_Header(){};

	Element_Header(Context& cont, bool zLib=false)
	{
		read(cont, zLib);
	};

	inline void read(Context& cont ,bool zLib=false)
	{
		// only zip less implemented so far...
		assert(zLib == false);

		Element_Length.read(cont);
		Object_Type_ID.read(cont);
		Object_Base_Type.read(cont);
	};

	I32  Element_Length;
	GUID Object_Type_ID;
	UChar Object_Base_Type;


};



#endif







