 /****************************************************************************
  *   Copyright (c) 2023 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
  *                                                                          *
  *   This file is part of the FreeCAD CAx development system.               *
  *                                                                          *
  *   FreeCAD is free software: you can redistribute it and/or modify it     *
  *   under the terms of the GNU Lesser General Public License as            *
  *   published by the Free Software Foundation, either version 2.1 of the   *
  *   License, or (at your option) any later version.                        *
  *                                                                          *
  *   FreeCAD is distributed in the hope that it will be useful, but         *
  *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
  *   Lesser General Public License for more details.                        *
  *                                                                          *
  *   You should have received a copy of the GNU Lesser General Public       *
  *   License along with FreeCAD. If not, see                                *
  *   <https://www.gnu.org/licenses/>.                                       *
  *                                                                          *
  ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "ParamHandler.h"

using namespace Gui;

ParamHandlers::ParamHandlers()
{
}

ParamHandlers::~ParamHandlers()
{
}

void ParamHandlers::addHandler(const ParamKey &key, const std::shared_ptr<ParamHandler> &handler)
{
    if (handlers.empty()) {
        conn = App::GetApplication().GetUserParameter().signalParamChanged.connect(
            [this](ParameterGrp *Param, ParameterGrp::ParamType, const char *Name, const char *) {
                if (!Param || !Name)
                    return;
                auto it =  handlers.find(ParamKey(Param, Name));
                if (it != handlers.end() && it->second->onChange(&it->first)) {
                    pendings.insert(it->second);
                    timer.start(100);
                }
            });

        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, [this]() {
            for (const auto &v : pendings) {
                v->onTimer();
            }
            pendings.clear();
        });
    }
    handlers[key] = handler;
}
