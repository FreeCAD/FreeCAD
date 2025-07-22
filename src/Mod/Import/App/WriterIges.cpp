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
#include <IGESCAFControl_Writer.hxx>
#include <IGESData_GlobalSection.hxx>
#include <IGESData_IGESModel.hxx>
#include <IGESToBRep_Actor.hxx>
#endif

#include "WriterIges.h"
#include <Base/Exception.h>
#include <App/Application.h>
#include <Mod/Part/App/encodeFilename.h>
#include <Mod/Part/App/Interface.h>

using namespace Import;

WriterIges::WriterIges(const Base::FileInfo& file)  // NOLINT
    : file {file}
{}

void WriterIges::write(Handle(TDocStd_Document) hDoc) const  // NOLINT
{
    std::string utf8Name = file.filePath();
    std::string name8bit = Part::encodeFilename(utf8Name);

    IGESControl_Controller::Init();
    IGESCAFControl_Writer writer;
    IGESData_GlobalSection header = writer.Model()->GlobalSection();
    header.SetAuthorName(new TCollection_HAsciiString(Part::Interface::writeIgesHeaderAuthor()));
    header.SetCompanyName(new TCollection_HAsciiString(Part::Interface::writeIgesHeaderCompany()));
    header.SetSendName(new TCollection_HAsciiString(Part::Interface::writeIgesHeaderProduct()));
    writer.Model()->SetGlobalSection(header);
    writer.Transfer(hDoc);
    Standard_Boolean ret = writer.Write(name8bit.c_str());
    if (!ret) {
        throw Base::FileException("Cannot open file: ", file);
    }
}
