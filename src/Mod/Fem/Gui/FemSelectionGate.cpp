/******************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)            *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include "PreCompiled.h"

#include "FemSelectionGate.h"


using namespace FemGui;
using namespace Gui;

bool FemSelectionGate::allow(App::Document* /*pDoc*/,
                             App::DocumentObject* /*pObj*/,
                             const char* sSubName)
{
    if (!sSubName || sSubName[0] == '\0') {
        return false;
    }

    if (sSubName[0] == 'E' && sSubName[1] == 'l' && sSubName[2] == 'e' && sSubName[3] == 'm'
        && (Type == Element || Type == NodeElement)) {
        return true;
    }
    if (sSubName[0] == 'N' && sSubName[1] == 'o' && sSubName[2] == 'd' && sSubName[3] == 'e'
        && (Type == Node || Type == NodeElement)) {
        return true;
    }

    return false;
}
