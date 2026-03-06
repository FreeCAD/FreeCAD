/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <CXX/Extensions.hxx>

#include "SoFCOffscreenRenderer.h"


namespace Gui
{

class SoQtOffscreenRendererPy: public Py::PythonClass<SoQtOffscreenRendererPy>
{
public:
    static void init_type();

    SoQtOffscreenRendererPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kwds);
    ~SoQtOffscreenRendererPy() override;

    Py::Object repr() override;

    Py::Object setViewportRegion(const Py::Tuple&);
    Py::Object getViewportRegion();

    Py::Object setBackgroundColor(const Py::Tuple&);
    Py::Object getBackgroundColor();

    Py::Object setNumPasses(const Py::Tuple&);
    Py::Object getNumPasses();

    Py::Object setInternalTextureFormat(const Py::Tuple&);
    Py::Object getInternalTextureFormat();

    Py::Object render(const Py::Tuple&);

    Py::Object writeToImage(const Py::Tuple&);
    Py::Object getWriteImageFiletypeInfo();

private:
    SoQtOffscreenRenderer renderer;
};

}  // namespace Gui
