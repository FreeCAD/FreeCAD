/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#include "PreCompiled.h"
#include <App/Application.h>
#include "ViewProvider.h"
#include "ExprParams.h"

using namespace Gui;

ExprParams::ExprParams() {
    handle = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
    handle->Attach(this);
#undef FC_EXPR_PARAM
#define FC_EXPR_PARAM(_name,_ctype,_type,_def,_doc) \
    _##_name = handle->Get##_type(#_name,_def);\
    funcs[#_name] = &ExprParams::update##_name;

    FC_EXPR_PARAMS
}

ExprParams::~ExprParams() {
}

#undef FC_EXPR_PARAM
#define FC_EXPR_PARAM(_name,_ctype,_type,_def,_doc) \
const char *ExprParams::doc##_name() { return _doc; }

FC_EXPR_PARAMS

void ExprParams::OnChange(Base::Subject<const char*> &, const char* sReason) {
    if(!sReason)
        return;
    auto it = funcs.find(sReason);
    if(it == funcs.end())
        return;
    it->second(this);
}

ExprParams *ExprParams::instance() {
    static ExprParams *inst;
    if(!inst)
        inst = new ExprParams;
    return inst;
}

