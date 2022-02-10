/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#ifndef GUI_VIEW_PARAMS_H
#define GUI_VIEW_PARAMS_H


#include <Base/Parameter.h>
#include <App/DynamicProperty.h>

namespace Gui {

/** Convenient class to obtain view provider related parameters
 *
 * The parameters are under group "User parameter:BaseApp/Preferences/View"
 *
 * To add a new parameter, add a new line under FC_VIEW_PARAMS using macro
 *
 * @code
 *      FC_VIEW_PARAM(parameter_name, c_type, parameter_type, default_value)
 * @endcode
 *
 * If there is special handling on parameter change, use FC_VIEW_PARAM2()
 * instead, and add a function with the following signature in ViewParams.cpp,
 *
 * @code
 *      void ViewParams:on<ParamName>Changed()
 * @endcode
 */
class GuiExport ViewParams: public ParameterGrp::ObserverType {
public:
    ViewParams();
    virtual ~ViewParams();
    void OnChange(Base::Subject<const char*> &, const char* sReason);
    static ViewParams *instance();

    ParameterGrp::handle getHandle() {
        return handle;
    }

#define FC_VIEW_PARAMS \
    FC_VIEW_PARAM(UseNewSelection,bool,Bool,true) \
    FC_VIEW_PARAM(UseSelectionRoot,bool,Bool,true) \
    FC_VIEW_PARAM(EnableSelection,bool,Bool,true) \
    FC_VIEW_PARAM(RenderCache,int,Int,0) \
    FC_VIEW_PARAM(RandomColor,bool,Bool,false) \
    FC_VIEW_PARAM(BoundingBoxColor,unsigned long,Unsigned,4294967295UL) \
    FC_VIEW_PARAM(AnnotationTextColor,unsigned long,Unsigned,4294967295UL) \
    FC_VIEW_PARAM(MarkerSize,int,Int,9) \
    FC_VIEW_PARAM(DefaultLinkColor,unsigned long,Unsigned,0x66FFFF00) \
    FC_VIEW_PARAM(DefaultShapeLineColor,unsigned long,Unsigned,421075455UL) \
    FC_VIEW_PARAM(DefaultShapeVertexColor,unsigned long,Unsigned,421075455UL) \
    FC_VIEW_PARAM(DefaultShapeColor,unsigned long,Unsigned,0xCCCCCC00) \
    FC_VIEW_PARAM(DefaultShapeLineWidth,int,Int,2) \
    FC_VIEW_PARAM(DefaultShapePointSize,int,Int,2) \
    FC_VIEW_PARAM(CoinCycleCheck,bool,Bool,true) \
    FC_VIEW_PARAM(EnablePropertyViewForInactiveDocument,bool,Bool,true) \
    FC_VIEW_PARAM(ShowSelectionBoundingBox,bool,Bool,false) \
    FC_VIEW_PARAM(PropertyViewTimer, unsigned long, Unsigned, 100) \
    FC_VIEW_PARAM2(DefaultFontSize, int , Int, 0)\

#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def) \
    static const _ctype &get##_name() { return instance()->_##_name; }\
    static const _ctype &_name() { return instance()->_##_name; }\
    static _ctype default##_name() { return _def; }\
    static void remove##_name() {instance()->handle->Remove##_type(#_name);}\
    static void set##_name(const _ctype &_v) { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    static void update##_name(ViewParams *self) { self->_##_name = self->handle->Get##_type(#_name,_def); }\

#undef FC_VIEW_PARAM2
#define FC_VIEW_PARAM2(_name,_ctype,_type,_def) \
    static const _ctype &get##_name() { return instance()->_##_name; }\
    static const _ctype &_name() { return instance()->_##_name; }\
    static _ctype default##_name() { return _def; }\
    static void set##_name(const _ctype &_v) { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    static void remove##_name() {instance()->handle->Remove##_type(#_name);}\
    void on##_name##Changed();\
    static void update##_name(ViewParams *self) { \
        auto _v = self->handle->Get##_type(#_name,_def); \
        if (self->_##_name != _v) {\
            self->_##_name = _v;\
            self->on##_name##Changed();\
        }\
    }\

    FC_VIEW_PARAMS

    static int appDefaultFontSize();

    static void init();

private:
#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def) \
    _ctype _##_name;

#undef FC_VIEW_PARAM2
#define FC_VIEW_PARAM2 FC_VIEW_PARAM

    FC_VIEW_PARAMS
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(ViewParams*),App::CStringHasher,App::CStringHasher> funcs;
};

#undef FC_VIEW_PARAM
#undef FC_VIEW_PARAM2

} // namespace Gui

#endif // GUI_VIEW_PARAMS_H
