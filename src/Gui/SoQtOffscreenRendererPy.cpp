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

#include "PreCompiled.h"
#include <Base/Interpreter.h>

#include "SoQtOffscreenRendererPy.h"


using namespace Gui;

SoQtOffscreenRendererPy::SoQtOffscreenRendererPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kwds)
    : Py::PythonClass<SoQtOffscreenRendererPy>(self, args, kwds), renderer(SbViewportRegion())
{
    this->setViewportRegion(args);
}

SoQtOffscreenRendererPy::~SoQtOffscreenRendererPy() = default;

Py::Object SoQtOffscreenRendererPy::repr()
{
    std::stringstream s;
    s << "<SoQtOffscreenRenderer at " << this << ">";
    return Py::String(s.str());
}

Py::Object SoQtOffscreenRendererPy::setViewportRegion(const Py::Tuple& args)
{
    short w, h;
    if (!PyArg_ParseTuple(args.ptr(), "hh", &w, &h)) {
        throw Py::Exception();
    }

    renderer.setViewportRegion(SbViewportRegion(w, h));
    return Py::None();
}
PYCXX_VARARGS_METHOD_DECL(SoQtOffscreenRendererPy, setViewportRegion)

Py::Object SoQtOffscreenRendererPy::getViewportRegion()
{
    const SbViewportRegion& vpr = renderer.getViewportRegion();
    SbVec2s size = vpr.getWindowSize();

    return Py::TupleN(Py::Long(size[0]), Py::Long(size[1]));
}
PYCXX_NOARGS_METHOD_DECL(SoQtOffscreenRendererPy, getViewportRegion)

Py::Object SoQtOffscreenRendererPy::setBackgroundColor(const Py::Tuple& args)
{
    float r, g, b, a = 1.0f;
    if (!PyArg_ParseTuple(args.ptr(), "fff|f", &r, &g, &b, &a)) {
        throw Py::Exception();
    }

    renderer.setBackgroundColor(SbColor4f(r, g, b, a));

    return Py::None();
}
PYCXX_VARARGS_METHOD_DECL(SoQtOffscreenRendererPy, setBackgroundColor)

Py::Object SoQtOffscreenRendererPy::getBackgroundColor()
{
    SbColor4f color = renderer.getBackgroundColor();
    return Py::TupleN(Py::Float(color[0]), Py::Float(color[1]), Py::Float(color[2]), Py::Float(color[3]));
}
PYCXX_NOARGS_METHOD_DECL(SoQtOffscreenRendererPy, getBackgroundColor)

Py::Object SoQtOffscreenRendererPy::setNumPasses(const Py::Tuple& args)
{
    int num;
    if (!PyArg_ParseTuple(args.ptr(), "i", &num)) {
        throw Py::Exception();
    }

    renderer.setNumPasses(num);

    return Py::None();
}
PYCXX_VARARGS_METHOD_DECL(SoQtOffscreenRendererPy, setNumPasses)

Py::Object SoQtOffscreenRendererPy::getNumPasses()
{
    return Py::Long(renderer.getNumPasses());
}
PYCXX_NOARGS_METHOD_DECL(SoQtOffscreenRendererPy, getNumPasses)

Py::Object SoQtOffscreenRendererPy::setInternalTextureFormat(const Py::Tuple& args)
{
    unsigned int format;
    if (!PyArg_ParseTuple(args.ptr(), "I", &format)) {
        throw Py::Exception();
    }

    renderer.setInternalTextureFormat(format);

    return Py::None();
}
PYCXX_VARARGS_METHOD_DECL(SoQtOffscreenRendererPy, setInternalTextureFormat)

Py::Object SoQtOffscreenRendererPy::getInternalTextureFormat()
{
    return Py::Long(static_cast<unsigned long>(renderer.internalTextureFormat()));
}
PYCXX_NOARGS_METHOD_DECL(SoQtOffscreenRendererPy, getInternalTextureFormat)

Py::Object SoQtOffscreenRendererPy::render(const Py::Tuple& args)
{
    PyObject* proxy;
    if (!PyArg_ParseTuple(args.ptr(), "O", &proxy)) {
        throw Py::Exception();
    }

    try {
        void* ptr = nullptr;
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoNode *", proxy, &ptr, 0);
        auto node = static_cast<SoNode*>(ptr);
        bool ok = false;
        if (node) {
            ok = renderer.render(node);
        }
        return Py::Boolean(ok);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        throw Py::Exception();
    }
}
PYCXX_VARARGS_METHOD_DECL(SoQtOffscreenRendererPy, render)

Py::Object SoQtOffscreenRendererPy::writeToImage(const Py::Tuple& args)
{
    const char* filename;
    if (!PyArg_ParseTuple(args.ptr(), "s", &filename)) {
        throw Py::Exception();
    }

    QImage img;
    renderer.writeToImage(img);
    img.save(QString::fromUtf8(filename));

    return Py::None();
}
PYCXX_VARARGS_METHOD_DECL(SoQtOffscreenRendererPy, writeToImage)

Py::Object SoQtOffscreenRendererPy::getWriteImageFiletypeInfo()
{
    QStringList list = renderer.getWriteImageFiletypeInfo();
    Py::Tuple tuple(list.size());
    int index = 0;
    for (const auto& item : list) {
        tuple[index++] = Py::String(item.toStdString());
    }

    return tuple;
}
PYCXX_NOARGS_METHOD_DECL(SoQtOffscreenRendererPy, getWriteImageFiletypeInfo)

void SoQtOffscreenRendererPy::init_type()
{
    behaviors().name("Gui.SoQtOffscreenRenderer");
    behaviors().doc("Python interface for SoQtOffscreenRenderer");

    // you must have overwritten the virtual functions
    behaviors().supportRepr();

    PYCXX_ADD_VARARGS_METHOD(setViewportRegion, setViewportRegion, "setViewportRegion(int, int)");
    PYCXX_ADD_NOARGS_METHOD(getViewportRegion, getViewportRegion, "getViewportRegion() -> tuple");
    PYCXX_ADD_VARARGS_METHOD(setBackgroundColor, setBackgroundColor, "setBackgroundColor(float, float, float, [float])");
    PYCXX_ADD_NOARGS_METHOD(getBackgroundColor, getBackgroundColor, "getBackgroundColor() -> tuple");
    PYCXX_ADD_VARARGS_METHOD(setNumPasses, setNumPasses, "setNumPasses(int)");
    PYCXX_ADD_NOARGS_METHOD(getNumPasses, getNumPasses, "getNumPasses() -> int");
    PYCXX_ADD_VARARGS_METHOD(setInternalTextureFormat, setInternalTextureFormat, "setInternalTextureFormat(int)");
    PYCXX_ADD_NOARGS_METHOD(getInternalTextureFormat, getInternalTextureFormat, "getInternalTextureFormat() -> int");
    PYCXX_ADD_VARARGS_METHOD(render, render, "render(node)");
    PYCXX_ADD_VARARGS_METHOD(writeToImage, writeToImage, "writeToImage(string)");
    PYCXX_ADD_NOARGS_METHOD(getWriteImageFiletypeInfo, getWriteImageFiletypeInfo, "getWriteImageFiletypeInfo() -> tuple");

    behaviors().readyType();
}
