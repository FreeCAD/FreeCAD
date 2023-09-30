// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#include <IGESControl_Controller.hxx>
#include <IGESCAFControl_Reader.hxx>
#include <IGESData_GlobalSection.hxx>
#include <IGESData_IGESModel.hxx>
#include <IGESToBRep_Actor.hxx>
#include <Standard_Version.hxx>
#include <Transfer_TransientProcess.hxx>
#include <XSControl_TransferReader.hxx>
#include <XSControl_WorkSession.hxx>
#endif

#include "ReaderIges.h"
#include <Base/Exception.h>
#include <App/Application.h>
#include <Mod/Part/App/encodeFilename.h>
#include <Mod/Part/App/ProgressIndicator.h>

using namespace Import;

ReaderIges::ReaderIges(const Base::FileInfo& file)  // NOLINT
    : file {file}
{}

void ReaderIges::read(Handle(TDocStd_Document) hDoc)  // NOLINT
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part")
                                             ->GetGroup("IGES");
    std::string utf8Name = file.filePath();
    std::string name8bit = Part::encodeFilename(utf8Name);

    IGESControl_Controller::Init();
    IGESCAFControl_Reader aReader;
    // http://www.opencascade.org/org/forum/thread_20603/?forum=3
    aReader.SetReadVisible(hGrp->GetBool("SkipBlankEntities", true));
    aReader.SetColorMode(true);
    aReader.SetNameMode(true);
    aReader.SetLayerMode(true);
    if (aReader.ReadFile(name8bit.c_str()) != IFSelect_RetDone) {
        throw Base::FileException("Cannot read IGES file", file);
    }

#if OCC_VERSION_HEX < 0x070500
    Handle(Message_ProgressIndicator) pi = new Part::ProgressIndicator(100);
    aReader.WS()->MapReader()->SetProgress(pi);
    pi->NewScope(100, "Reading IGES file...");
    pi->Show();
#endif
    aReader.Transfer(hDoc);
#if OCC_VERSION_HEX < 0x070500
    pi->EndScope();
#endif
    // http://opencascade.blogspot.de/2009/03/unnoticeable-memory-leaks-part-2.html
    Handle(IGESToBRep_Actor)::DownCast(aReader.WS()->TransferReader()->Actor())
        ->SetModel(new IGESData_IGESModel);
}
