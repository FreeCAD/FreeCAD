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
#include <Standard_Version.hxx>
#include <TColStd_IndexedDataMapOfStringString.hxx>
#if OCC_VERSION_HEX >= 0x070500
#include <Message_ProgressRange.hxx>
#include <RWGltf_CafWriter.hxx>
#endif
#endif

#include "WriterGltf.h"
#include <Base/Exception.h>
#include <Mod/Part/App/encodeFilename.h>

using namespace Import;

WriterGltf::WriterGltf(const Base::FileInfo& file)  // NOLINT
    : file {file}
{}

void WriterGltf::write(Handle(TDocStd_Document) hDoc) const  // NOLINT
{
    std::string utf8Name = file.filePath();
    std::string name8bit = Part::encodeFilename(utf8Name);

#if OCC_VERSION_HEX >= 0x070500
    TColStd_IndexedDataMapOfStringString aMetadata;
    RWGltf_CafWriter aWriter(name8bit.c_str(), file.hasExtension("glb"));
    aWriter.SetTransformationFormat(RWGltf_WriterTrsfFormat_Compact);
    // https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#coordinate-system-and-units
    aWriter.ChangeCoordinateSystemConverter().SetInputLengthUnit(0.001);  // NOLINT
    aWriter.ChangeCoordinateSystemConverter().SetInputCoordinateSystem(RWMesh_CoordinateSystem_Zup);
#if OCC_VERSION_HEX >= 0x070700
    aWriter.SetParallel(true);
#endif
    Standard_Boolean ret = aWriter.Perform(hDoc, aMetadata, Message_ProgressRange());
    if (!ret) {
        throw Base::FileException("Cannot save to file: ", file);
    }
#else
    throw Base::RuntimeError("gITF support requires OCCT 7.5.0 or later");
#endif
}
