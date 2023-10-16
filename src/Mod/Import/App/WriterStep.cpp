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
#include <APIHeaderSection_MakeHeader.hxx>
#include <NCollection_Vector.hxx>
#include <STEPCAFControl_Writer.hxx>
#endif

#include "WriterStep.h"
#include <Base/Exception.h>
#include <App/Application.h>
#include <Mod/Part/App/encodeFilename.h>
#include <Mod/Part/App/Interface.h>

using namespace Import;

WriterStep::WriterStep(const Base::FileInfo& file)  // NOLINT
    : file {file}
{}

void WriterStep::write(Handle(TDocStd_Document) hDoc) const  // NOLINT
{
    std::string utf8Name = file.filePath();
    std::string name8bit = Part::encodeFilename(utf8Name);

    STEPCAFControl_Writer writer;
    Part::Interface::writeStepAssembly(Part::Interface::Assembly::On);
    writer.Transfer(hDoc, STEPControl_AsIs);

    APIHeaderSection_MakeHeader makeHeader(writer.ChangeWriter().Model());
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part")
                                             ->GetGroup("STEP");

    // Don't set name because STEP doesn't support UTF-8
    // https://forum.freecad.org/viewtopic.php?f=8&t=52967
    makeHeader.SetAuthorValue(
        1,
        new TCollection_HAsciiString(hGrp->GetASCII("Author", "Author").c_str()));
    makeHeader.SetOrganizationValue(
        1,
        new TCollection_HAsciiString(hGrp->GetASCII("Company").c_str()));
    makeHeader.SetOriginatingSystem(
        new TCollection_HAsciiString(App::Application::getExecutableName().c_str()));
    makeHeader.SetDescriptionValue(1, new TCollection_HAsciiString("FreeCAD Model"));
    IFSelect_ReturnStatus ret = writer.Write(name8bit.c_str());
    if (ret == IFSelect_RetError || ret == IFSelect_RetFail || ret == IFSelect_RetStop) {
        throw Base::FileException("Cannot open file: ", file);
    }
}
