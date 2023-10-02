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

#include <Base/Console.h>
#include <Base/Interpreter.h>


extern struct PyMethodDef JtReader_methods[];


extern "C" {
void AppJtReaderExport initJtReader()
{

    static struct PyModuleDef JtReaderAPIDef =
        {PyModuleDef_HEAD_INIT, "JtReader", 0, -1, JtReader_methods, NULL, NULL, NULL, NULL};
    PyModule_Create(&JtReaderAPIDef);

    // load dependent module
    Base::Interpreter().loadModule("Mesh");

    // Initialize JtTk
    // JtkEntityFactory::init();

    // Note, non-evaluation JT Open Toolkit licensees must uncomment the
    // following line, inserting their "Sold_To_ID". Each licensee has a
    // unique Sold_To_ID issued by UGS Corp.
    //
    // JtkEntityFactory::registerCustomer( 1103193 );
    // JtkEntityFactory::registerCustomer(1103103);

    Base::Console().Log("Loading JtReader module... done\n");

    return;
}


}  // extern "C" {
