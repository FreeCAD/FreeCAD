/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer@users.sourceforge.net>        *
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


#pragma once

#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <tchar.h>
#include <thumbcache.h>
#include <windows.h>

STDAPI_(ULONG) DllAddRef();
STDAPI_(ULONG) DllRelease();
STDAPI_(HINSTANCE) DllInstance();

// {4BBBEAB5-BE00-41f4-A209-FE838660B9B1}
#define szCLSID_SampleThumbnailProvider L"{4BBBEAB5-BE00-41f4-A209-FE838660B9B1}"
DEFINE_GUID(CLSID_SampleThumbnailProvider,
            0x4bbbeab5,
            0xbe00,
            0x41f4,
            0xa2,
            0x9,
            0xfe,
            0x83,
            0x86,
            0x60,
            0xb9,
            0xb1);
