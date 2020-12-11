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

#ifndef PART_PARAMS_H
#define PART_PARAMS_H

#include <QTimer>
#include <Base/Parameter.h>
#include <App/DynamicProperty.h>

namespace PartGui {

/** Convenient class to obtain Mod/Part related parameters 
 *
 * The parameters are under group "User parameter:BaseApp/Preferences/Mod/Part"
 *
 * To add a new parameter, add a new line under FC_PART_PARAMS using macro
 *
 * @code
 *      FC_PART_PARAM(parameter_name, c_type, parameter_type, default_value)
 * @endcode
 *
 * If there is special handling on parameter change, use FC_PART_PARAM2()
 * instead, and add a function with the following signature in PartParams.cpp,
 *
 * @code
 *      void PartParams:on<ParamName>Changed()
 * @endcode
 */
class PartGuiExport PartParams: public QObject, public ParameterGrp::ObserverType
{
    Q_OBJECT
public:
    PartParams();
    virtual ~PartParams();
    void OnChange(Base::Subject<const char*> &, const char* sReason);
    static PartParams *instance();

    ParameterGrp::handle getHandle() {
        return handle;
    }

#define FC_PART_PARAMS \
    FC_PART_PARAM(NormalsFromUVNodes, bool, Bool, true) \
    FC_PART_PARAM(TwoSideRendering, bool, Bool, true) \
    FC_PART_PARAM2(MinimumDeviation, double, Float, 0.05) \
    FC_PART_PARAM2(MeshDeviation, double, Float, 0.2) \
    FC_PART_PARAM2(MeshAngularDeflection, double, Float, 28.65) \
    FC_PART_PARAM2(MinimumAngularDeflection, double, Float, 5.0) \
    FC_PART_PARAM2(OverrideTessellation, bool, Bool, false) \
    FC_PART_PARAM(MapFaceColor, bool, Bool, true) \
    FC_PART_PARAM(MapLineColor, bool, Bool, false) \
    FC_PART_PARAM(MapPointColor, bool, Bool, false) \
    FC_PART_PARAM(MapTransparency, bool, Bool, false) \
    FC_PART_PARAM(AutoGridScale, bool, Bool, false) \
    FC_PART_PARAM(PreviewAddColor,unsigned long,Unsigned,0x64ffff30) \
    FC_PART_PARAM(PreviewSubColor,unsigned long,Unsigned,0xff646430) \
    FC_PART_PARAM(PreviewDressColor,unsigned long,Unsigned,0xff64ff30) \
    FC_PART_PARAM(PreviewOnEdit,bool,Bool,true) \
    FC_PART_PARAM(EditOnTop,bool,Bool,false) \

#undef FC_PART_PARAM
#define FC_PART_PARAM(_name,_ctype,_type,_def) \
    static const _ctype & _name() { return instance()->_##_name; }\
    static void set_##_name(_ctype _v) { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    static void update##_name(PartParams *self) { self->_##_name = self->handle->Get##_type(#_name,_def); }\

#undef FC_PART_PARAM2
#define FC_PART_PARAM2(_name,_ctype,_type,_def) \
    static const _ctype & _name() { return instance()->_##_name; }\
    static void set_##_name(_ctype _v) { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    void on##_name##Changed();\
    static void update##_name(PartParams *self) { \
        self->_##_name = self->handle->Get##_type(#_name,_def); \
        self->on##_name##Changed();\
    }\

    FC_PART_PARAMS

private Q_SLOTS:
    void updateVisual();

private:
#undef FC_PART_PARAM
#define FC_PART_PARAM(_name,_ctype,_type,_def) \
    _ctype _##_name;

#undef FC_PART_PARAM2
#define FC_PART_PARAM2 FC_PART_PARAM

    FC_PART_PARAMS
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(PartParams*),App::CStringHasher,App::CStringHasher> funcs;
    QTimer timer;
};

#undef FC_PART_PARAM
#undef FC_PART_PARAM2

} // namespace PartGui

#endif // PART_PARAMS_H
