/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
#include "Application.h"
#include "Document.h"
#include "DocumentParams.h"

using namespace App;

DocumentParams::DocumentParams() {
    handle = GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Document");
    handle->Attach(this);
#undef FC_DOCUMENT_PARAM
#define FC_DOCUMENT_PARAM(_name,_ctype,_type,_def) \
    _##_name = handle->Get##_type(#_name,_def);\
    funcs[#_name] = &DocumentParams::update##_name;

#undef FC_DOCUMENT_PARAM2
#define FC_DOCUMENT_PARAM2 FC_DOCUMENT_PARAM
    FC_DOCUMENT_PARAMS
}

DocumentParams::~DocumentParams() {
}

void DocumentParams::OnChange(Base::Subject<const char*> &, const char* sReason) {
    if(!sReason)
        return;
    auto it = funcs.find(sReason);
    if(it == funcs.end())
        return;
    it->second(this);
}

DocumentParams *DocumentParams::instance() {
    static DocumentParams *inst;
    if(!inst)
        inst = new DocumentParams;
    return inst;
}

