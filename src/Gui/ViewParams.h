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
    FC_VIEW_PARAM(DefaultShapePointSize,int,Int,2) \
    FC_VIEW_PARAM(CoinCycleCheck,bool,Bool,true) \
    FC_VIEW_PARAM(EnablePropertyViewForInactiveDocument,bool,Bool,true) \
    FC_VIEW_PARAM(ShowSelectionBoundingBox,bool,Bool,false) \
    FC_VIEW_PARAM(UpdateSelectionVisual,bool,Bool,true) \
    FC_VIEW_PARAM(LinkChildrenDirect,bool,Bool,true) \
    FC_VIEW_PARAM2(ShowSelectionOnTop,bool,Bool,true) \
    FC_VIEW_PARAM(TransparencyOnTop,double,Float,0.5) \
    FC_VIEW_PARAM(PartialHighlightOnFullSelect,bool,Bool,false) \
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
    FC_VIEW_PARAM(ViewSelectionExtendFactor,double,Float,0.5) \
    FC_VIEW_PARAM(UseTightBoundingBox,bool,Bool,true) \
    FC_VIEW_PARAM(UseBoundingBoxCache,bool,Bool,true) \
    FC_VIEW_PARAM(RenderProjectedBBox,bool,Bool,true) \
    FC_VIEW_PARAM(SelectionFaceWire,bool,Bool,false) \
    FC_VIEW_PARAM(NewDocumentCameraScale,double,Float,100.0) \
    FC_VIEW_PARAM(MaxOnTopSelections,int,Int,20) \
    FC_VIEW_PARAM2(MapChildrenPlacement,bool,Bool,false) \
    FC_VIEW_PARAM(NaviWidgetSize,int,Int,132) \
    FC_VIEW_PARAM2(CornerNaviCube,int,Int,1) \
    FC_VIEW_PARAM2(DockOverlayAutoView,bool,Bool,true) \
    FC_VIEW_PARAM2(DockOverlayExtraState,bool,Bool,false) \
    FC_VIEW_PARAM(DockOverlayDelay,int,Int,200) \
    FC_VIEW_PARAM(DockOverlayRevealDelay,int,Int,2000) \
    FC_VIEW_PARAM(DockOverlayActivateOnHover,bool,Bool,true) \
    FC_VIEW_PARAM(DockOverlayMouseThrough,bool,Bool,true) \
    FC_VIEW_PARAM2(DockOverlayCheckNaviCube,bool,Bool,true) \
    FC_VIEW_PARAM(EditingTransparency,double,Float,0.5) \
    FC_VIEW_PARAM(EditingAutoTransparent,bool,Bool,true) \
    FC_VIEW_PARAM(HiddenLineTransparency,double,Float,0.4) \
    FC_VIEW_PARAM(HiddenLineFaceColor,unsigned long,Unsigned,0xffffffff) \
    FC_VIEW_PARAM(HiddenLineOverrideFaceColor,bool,Bool,true) \
    FC_VIEW_PARAM(HiddenLineColor,unsigned long,Unsigned,0x000000ff) \
    FC_VIEW_PARAM(HiddenLineOverrideColor,bool,Bool,true) \
    FC_VIEW_PARAM(HiddenLineBackground,unsigned long,Unsigned,0xffffffff) \
    FC_VIEW_PARAM(HiddenLineOverrideBackground,bool,Bool,false) \
    FC_VIEW_PARAM(HiddenLineShaded, bool, Bool, false) \
    FC_VIEW_PARAM(StatusMessageTimeout, int, Int, 5000) \
    FC_VIEW_PARAM(ShadowFlatLines, bool, Bool, true) \
    FC_VIEW_PARAM(ShadowSpotLight, bool, Bool, false) \
    FC_VIEW_PARAM(ShadowLightIntensity, double, Float, 0.8) \
    FC_VIEW_PARAM(ShadowLightDirectionX, double, Float, -1.0) \
    FC_VIEW_PARAM(ShadowLightDirectionY, double, Float, -1.0) \
    FC_VIEW_PARAM(ShadowLightDirectionZ, double, Float, -1.0) \
    FC_VIEW_PARAM(ShadowLightColor, unsigned long, Unsigned, 0xf0fdffff) \
    FC_VIEW_PARAM(ShadowShowGround, bool, Bool, true) \
    FC_VIEW_PARAM(ShadowGroundScale, double, Float, 2.0) \
    FC_VIEW_PARAM(ShadowGroundColor, unsigned long, Unsigned, 0x7d7d7dff) \
    FC_VIEW_PARAM(ShadowGroundShininess, double, Float, 0.8) \
    FC_VIEW_PARAM(ShadowExtraRedraw, bool, Bool, true) \

#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def) \
    static _ctype get##_name() { return instance()->_name; }\
    static void set##_name(_ctype _v) { instance()->handle->Set##_type(#_name,_v); instance()->_name=_v; }\
    static void update##_name(ViewParams *self) { self->_name = self->handle->Get##_type(#_name,_def); }\

#undef FC_VIEW_PARAM2
#define FC_VIEW_PARAM2(_name,_ctype,_type,_def) \
    static _ctype get##_name() { return instance()->_name; }\
    static void set##_name(_ctype _v) { instance()->handle->Set##_type(#_name,_v); instance()->_name=_v; }\
    void on##_name##Changed();\
    static void update##_name(ViewParams *self) { \
        self->_name = self->handle->Get##_type(#_name,_def); \
        self->on##_name##Changed();\
    }\

    FC_VIEW_PARAMS

    static bool highlightIndicesOnFullSelect() {
        return getShowSelectionOnTop() && getPartialHighlightOnFullSelect();
    }

private:
#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def) \
    _ctype _name;

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
