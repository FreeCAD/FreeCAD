/****************************************************************************
 *   Copyright (c) 2021 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "../PreCompiled.h"
#include <Base/Console.h>
#include "Renderer.h"

FC_LOG_LEVEL_INIT("Renderer", true, true)

using namespace Gui;

namespace {

std::vector<RendererLib*> _rendererLibs;
std::map<std::string, RendererLib*> _rendererTypes;

const std::map<std::string, RendererLib*> &rendererTypes()
{
    if (_rendererTypes.empty()) {
        for (auto lib : _rendererLibs) {
            for (const auto &type : lib->types())
                _rendererTypes[type] = lib;
        }
    }
    return _rendererTypes;
}

} // anonymous namespace

void RendererFactory::registerLib(RendererLib *lib)
{
    _rendererTypes.clear();
    _rendererLibs.push_back(lib);
}

std::vector<std::string> RendererFactory::types()
{
    std::vector<std::string> res;
    for (auto &v : rendererTypes())
        res.push_back(v.first);
    return res;
}

std::unique_ptr<Renderer> RendererFactory::create(
        const std::string &type, QOpenGLWidget *widget)
{
    std::unique_ptr<Renderer> res;
    auto it = rendererTypes().find(type);
    if (it == rendererTypes().end()) {
        if (type.size())
            FC_WARN("Renderer '" << type << "' no supported");
    } else
        res = it->second->create(type, widget);
    return res;
}
