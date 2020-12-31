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

#ifndef GUI_EXPR_PARAMS_H
#define GUI_EXPR_PARAMS_H


#include <Base/Parameter.h>
#include <App/DynamicProperty.h>

namespace Gui {

/** Convenient class to obtain expression related parameters 
 *
 * The parameters are under group "User parameter:BaseApp/Preferences/Expression"
 *
 * To add a new parameter, add a new line under FC_EXPR_PARAMS using macro
 *
 * @code
 *      FC_EXPR_PARAM(parameter_name, c_type, parameter_type, default_value, documentation)
 * @endcode
 *
 */
class GuiExport ExprParams: public ParameterGrp::ObserverType {
public:
    ExprParams();
    virtual ~ExprParams();
    void OnChange(Base::Subject<const char*> &, const char* sReason);
    static ExprParams *instance();

    ParameterGrp::handle getHandle() {
        return handle;
    }

#define FC_EXPR_PARAMS \
    FC_EXPR_PARAM(CompleterCaseSensitive,bool,Bool,false,\
       QT_TRANSLATE_NOOP("ExprParams","Expression completer with case sensistive")) \
    FC_EXPR_PARAM(CompleterMatchExact,bool,Bool,false,\
       QT_TRANSLATE_NOOP("ExprParams","Expression completer match exact")) \
    FC_EXPR_PARAM(NoSystemBackground,bool,Bool,true,\
       QT_TRANSLATE_NOOP("ExprParams","Enable in place expressiong editing")) \
    FC_EXPR_PARAM(EditorTrigger,std::string,ASCII,"=",\
       QT_TRANSLATE_NOOP("ExprParams","Expression editor trigger character")) \
    FC_EXPR_PARAM(AutoHideEditorIcon,bool,Bool,true,\
       QT_TRANSLATE_NOOP("ExprParams","Only show editor icon on mouse over")) \
    FC_EXPR_PARAM(AllowReturn,bool,Bool,false,\
       QT_TRANSLATE_NOOP("ExprParams","Allow return key in expression edit box")) \

#undef FC_EXPR_PARAM
#define FC_EXPR_PARAM(_name,_ctype,_type,_def,_doc) \
    static const _ctype & get##_name() \
        { return instance()->_##_name; }\
    static const _ctype & _name () \
        { return instance()->_##_name; }\
    static void set##_name(const _ctype &_v) \
        { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    static void update##_name(ExprParams *self) \
        { self->_##_name = self->handle->Get##_type(#_name,_def); }\
    static const char *doc##_name(); \

    FC_EXPR_PARAMS

private:
#undef FC_EXPR_PARAM
#define FC_EXPR_PARAM(_name,_ctype,_type,_def,_doc) \
    _ctype _##_name;

    FC_EXPR_PARAMS
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(ExprParams*),App::CStringHasher,App::CStringHasher> funcs;
};

#undef FC_EXPR_PARAM

} // namespace Gui

#endif // GUI_EXPR_PARAMS_H
