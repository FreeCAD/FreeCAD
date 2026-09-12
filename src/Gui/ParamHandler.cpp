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


#include "ParamHandler.h"

using namespace Gui;

ParamHandlers::ParamHandlers()
{}

ParamHandlers::~ParamHandlers()
{}

void ParamHandlers::ensureConnected()
{
    if (conn.connected()) {
        return;
    }

    conn = App::GetApplication().GetUserParameter().signalParamChanged.connect(
        [this](ParameterGrp* Param, ParameterGrp::ParamType, const char* Name, const char*) {
            if (!Param || !Name) {
                return;
            }

            const ParamKey changedKey(Param, Name);
            std::set<std::shared_ptr<ParamHandler>> matched;

            if (auto it = handlers.find(changedKey); it != handlers.end()) {
                matched.insert(it->second);
            }

            auto range = groupHandlers.equal_range(ParameterGrp::handle(Param));
            for (auto it = range.first; it != range.second; ++it) {
                matched.insert(it->second);
            }

            for (const auto& handler : matched) {
                if (handler->onChange(&changedKey)) {
                    pendings.insert(handler);
                    timer.start(100);
                }
            }
        },
        fastsignals::advanced_tag()
    );

    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [this]() {
        for (const auto& v : pendings) {
            v->onTimer();
        }
        pendings.clear();
    });
}

void ParamHandlers::addHandler(const ParamKey& key, const std::shared_ptr<ParamHandler>& handler)
{
    ensureConnected();
    handlers[key] = handler;
}

void ParamHandlers::addGroupHandler(ParameterGrp* hGrp, const std::shared_ptr<ParamHandler>& handler)
{
    ensureConnected();
    groupHandlers.emplace(ParameterGrp::handle(hGrp), handler);
}
