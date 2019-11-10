/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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
    FC_VIEW_PARAM(EnablePreselection,bool,Bool,true) \
    FC_VIEW_PARAM(RenderCache,int,Int,0) \
    FC_VIEW_PARAM(RandomColor,bool,Bool,false) \
    FC_VIEW_PARAM(BoundingBoxColor,unsigned long,Unsigned,0xffffffff) \
    FC_VIEW_PARAM(AnnotationTextColor,unsigned long,Unsigned,0xffffffff) \
    FC_VIEW_PARAM(HighlightColor,unsigned long,Unsigned,0xe1e114ff) \
    FC_VIEW_PARAM(SelectionColor,unsigned long,Unsigned,0x1cad1cff) \
    FC_VIEW_PARAM(MarkerSize,int,Int,9) \
    FC_VIEW_PARAM(DefaultLinkColor,unsigned long,Unsigned,0x66FFFFFF) \
    FC_VIEW_PARAM(DefaultShapeLineColor,unsigned long,Unsigned,0x191919FF) \
    FC_VIEW_PARAM(DefaultShapeColor,unsigned long,Unsigned,0xCCCCCCFF) \
    FC_VIEW_PARAM(DefaultShapeLineWidth,int,Int,2) \
    FC_VIEW_PARAM(CoinCycleCheck,bool,Bool,true) \
    FC_VIEW_PARAM(EnablePropertyViewForInactiveDocument,bool,Bool,true) \
    FC_VIEW_PARAM(ShowSelectionBoundingBox,bool,Bool,false) \
    FC_VIEW_PARAM(UpdateSelectionVisual,bool,Bool,true) \
    FC_VIEW_PARAM(LinkChildrenDirect,bool,Bool,true) \
    FC_VIEW_PARAM2(ShowSelectionOnTop,bool,Bool,false) \
    FC_VIEW_PARAM(SelectionLineThicken,double,Float,1.0) \
    FC_VIEW_PARAM(PickRadius,double,Float,5.0) \
    FC_VIEW_PARAM(SelectionTransparency,double,Float,0.5) \
    FC_VIEW_PARAM(SelectionLinePattern,int,Int,0) \
    FC_VIEW_PARAM(SelectionHiddenLineWidth,double,Float,1.0) \
    FC_VIEW_PARAM(SelectionBBoxLineWidth,double,Float,3.0) \
    FC_VIEW_PARAM(ShowHighlightEdgeOnly,bool,Bool,false) \
    FC_VIEW_PARAM(PreSelectionDelay,double,Float,0.1) \
    FC_VIEW_PARAM(SelectionPickThreshold,int,Int,50) \
    FC_VIEW_PARAM(UseNewRayPick,bool,Bool,true) \

#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def) \
    _ctype get##_name() const { return _name; }\
    void set##_name(_ctype _v) { handle->Set##_type(#_name,_v); _name=_v; }

#undef FC_VIEW_PARAM2
#define FC_VIEW_PARAM2(_name,_ctype,_type,_def) \
    FC_VIEW_PARAM(_name,_ctype,_type,_def)\
    void on##_name##Changed();\

    FC_VIEW_PARAMS

private:
#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def) \
    _ctype _name;

#undef FC_VIEW_PARAM2
#define FC_VIEW_PARAM2 FC_VIEW_PARAM

    FC_VIEW_PARAMS
    ParameterGrp::handle handle;
};

#undef FC_VIEW_PARAM
#undef FC_VIEW_PARAM2

} // namespace Gui

#endif // GUI_VIEW_PARAMS_H
