/***************************************************************************
 *   Copyright (c) 2014 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <sstream>
#endif

#include <Base/Exception.h>
#include <Base/FileInfo.h>

#include <Interface_Static.hxx>
#include <Message_Messenger.hxx>
#include <Message_PrinterOStream.hxx>
#include <STEPControl_Reader.hxx>
#include <Standard_Version.hxx>
#include <StepData_StepModel.hxx>

#include "StepShape.h"


using namespace Import;

StepShape::StepShape() = default;

StepShape::~StepShape() = default;

int StepShape::read(const char* fileName)
{
    STEPControl_Reader aReader;

    Base::FileInfo fi(fileName);

    if (!fi.exists()) {
        std::stringstream str;
        str << "File '" << fileName << "' does not exist!";
        throw Base::FileException(str.str().c_str());
    }

    if (aReader.ReadFile((Standard_CString)fileName) != IFSelect_RetDone) {
        throw Base::FileException("Cannot open STEP file");
    }


    Handle(TColStd_HSequenceOfTransient) list = aReader.GiveList();

    // Use method StepData_StepModel::NextNumberForLabel to find its rank with the following:
    // Standard_CString label = "#...";
    Handle(StepData_StepModel) model = aReader.StepModel();


    std::cout << "dump of step header:" << std::endl;
#if OCC_VERSION_HEX < 0x070401
    Handle(Message_PrinterOStream) mstr = new Message_PrinterOStream();
    Handle(Message_Messenger) msg = new Message_Messenger(mstr);
    model->DumpHeader(msg);
#else
    model->DumpHeader(std::cout);
#endif

    for (int nent = 1; nent <= model->NbEntities(); nent++) {
        Handle(Standard_Transient) entity = model->Entity(nent);
        std::cout << "label entity " << nent << ":";
#if OCC_VERSION_HEX < 0x070401
        model->PrintLabel(entity, msg);
#else
        model->PrintLabel(entity, std::cout);
#endif
        std::cout << ";" << entity->DynamicType()->Name() << std::endl;
    }

    return 0;
}
