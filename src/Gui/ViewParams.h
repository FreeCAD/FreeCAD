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
 *      FC_VIEW_PARAM(parameter_name, c_type, parameter_type, default_value, documentation)
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
    FC_VIEW_PARAM(UseNewSelection,bool,Bool,true, "") \
    FC_VIEW_PARAM(UseSelectionRoot,bool,Bool,true, "") \
    FC_VIEW_PARAM(EnableSelection,bool,Bool,true, "") \
    FC_VIEW_PARAM(EnablePreselection,bool,Bool,true, "") \
    FC_VIEW_PARAM(RenderCache,int,Int,0, "") \
    FC_VIEW_PARAM(RandomColor,bool,Bool,false, "") \
    FC_VIEW_PARAM(BoundingBoxColor,unsigned long,Unsigned,0xffffffff, "") \
    FC_VIEW_PARAM(AnnotationTextColor,unsigned long,Unsigned,0xffffffff, "") \
    FC_VIEW_PARAM(HighlightColor,unsigned long,Unsigned,0xe1e114ff, "") \
    FC_VIEW_PARAM(SelectionColor,unsigned long,Unsigned,0x1cad1cff, "") \
    FC_VIEW_PARAM(MarkerSize,int,Int,9, "") \
    FC_VIEW_PARAM(DefaultLinkColor,unsigned long,Unsigned,0x66FFFFFF, "") \
    FC_VIEW_PARAM(DefaultShapeLineColor,unsigned long,Unsigned,0x191919FF, "") \
    FC_VIEW_PARAM(DefaultShapeColor,unsigned long,Unsigned,0xCCCCCCFF, "") \
    FC_VIEW_PARAM(DefaultShapeLineWidth,int,Int,2, "") \
    FC_VIEW_PARAM(DefaultShapePointSize,int,Int,2, "") \
    FC_VIEW_PARAM(CoinCycleCheck,bool,Bool,true, "") \
    FC_VIEW_PARAM(EnablePropertyViewForInactiveDocument,bool,Bool,true, "") \
    FC_VIEW_PARAM(ShowSelectionBoundingBox,bool,Bool,false, \
       QT_TRANSLATE_NOOP("ViewParams","Show selection bounding box")) \
    FC_VIEW_PARAM(UpdateSelectionVisual,bool,Bool,true, "") \
    FC_VIEW_PARAM(LinkChildrenDirect,bool,Bool,true, "") \
    FC_VIEW_PARAM2(ShowSelectionOnTop,bool,Bool,true, \
       QT_TRANSLATE_NOOP("ViewParams","Show selection always on top")) \
    FC_VIEW_PARAM(SelectElementOnTop,bool,Bool,false, \
       QT_TRANSLATE_NOOP("ViewParams","Do box/lasso element selection on already selected object(s)," \
                                      "if SelectionOnTop is enabled.")) \
    FC_VIEW_PARAM(TransparencyOnTop,double,Float,0.5, \
       QT_TRANSLATE_NOOP("ViewParams","Transparency for the selected object when being shown on top."))\
    FC_VIEW_PARAM(HiddenLineSelectionOnTop,bool,Bool,true, \
       QT_TRANSLATE_NOOP("ViewParams","Enable hidden line/point selection when SelectionOnTop is active."))\
    FC_VIEW_PARAM(PartialHighlightOnFullSelect,bool,Bool,false, \
       QT_TRANSLATE_NOOP("ViewParams","Enable partial highlight on full selection for object that supports it."))\
    FC_VIEW_PARAM(SelectionLineThicken,double,Float,1.0, \
       QT_TRANSLATE_NOOP("ViewParams","Muplication factor to increase the width the selected line."))\
    FC_VIEW_PARAM(PickRadius,double,Float,5.0, "") \
    FC_VIEW_PARAM(SelectionLinePattern,int,Int,0, "") \
    FC_VIEW_PARAM(SelectionHiddenLineWidth,double,Float,1.0, \
       QT_TRANSLATE_NOOP("ViewParams","Width of the hidden line."))\
    FC_VIEW_PARAM(SelectionBBoxLineWidth,double,Float,3.0, "") \
    FC_VIEW_PARAM(ShowHighlightEdgeOnly,bool,Bool,false, \
       QT_TRANSLATE_NOOP("ViewParams","Show pre-selection highlight edge only"))\
    FC_VIEW_PARAM(PreSelectionDelay,double,Float,0.1, "") \
    FC_VIEW_PARAM(SelectionPickThreshold,int,Int,50, "") \
    FC_VIEW_PARAM(UseNewRayPick,bool,Bool,true, "") \
    FC_VIEW_PARAM(ViewSelectionExtendFactor,double,Float,0.5, "") \
    FC_VIEW_PARAM(UseTightBoundingBox,bool,Bool,true, \
       QT_TRANSLATE_NOOP("ViewParams", "Show more accurate bounds when using bounding box selection style"))\
    FC_VIEW_PARAM(UseBoundingBoxCache,bool,Bool,true, "") \
    FC_VIEW_PARAM(RenderProjectedBBox,bool,Bool,true, \
       QT_TRANSLATE_NOOP("ViewParams", "Show projected bounding box that is aligned to axes of\n"\
                                       "global coordinate space"))\
    FC_VIEW_PARAM(SelectionFaceWire,bool,Bool,false, \
       QT_TRANSLATE_NOOP("ViewParams", "Show hidden tirangulation wires for selected face"))\
    FC_VIEW_PARAM(NewDocumentCameraScale,double,Float,100.0, "") \
    FC_VIEW_PARAM(MaxOnTopSelections,int,Int,20, "") \
    FC_VIEW_PARAM2(MapChildrenPlacement,bool,Bool,false, \
       QT_TRANSLATE_NOOP("ViewParams", "Map child object into parent's coordinate space when showing on top.\n"\
                             "Note that once activated, this option will also activate option ShowOnTop.\n"\
                             "WARNING! This is an experimental option. Please use with caution."))\
    FC_VIEW_PARAM(NaviWidgetSize,int,Int,132, "") \
    FC_VIEW_PARAM2(CornerNaviCube,int,Int,1, "") \
    FC_VIEW_PARAM2(DockOverlayAutoView,bool,Bool,true, "") \
    FC_VIEW_PARAM2(DockOverlayExtraState,bool,Bool,false, "") \
    FC_VIEW_PARAM(DockOverlayDelay,int,Int,200, \
        QT_TRANSLATE_NOOP("ViewParams", "Overlay dock (re)layout delay."))\
    FC_VIEW_PARAM(DockOverlayRevealDelay,int,Int,2000, "")\
    FC_VIEW_PARAM(DockOverlayActivateOnHover,bool,Bool,true, \
        QT_TRANSLATE_NOOP("ViewParams", "Show auto hidden dock overlay on mouse over.\n"\
                                        "If disabled, then show on mouse click."))\
    FC_VIEW_PARAM(DockOverlayMouseThrough,bool,Bool,false, \
        QT_TRANSLATE_NOOP("ViewParams", "Enable mouse pass through dock overlay when holding 'ALT' key."))\
    FC_VIEW_PARAM(DockOverlayAutoMouseThrough,bool,Bool,true, \
        QT_TRANSLATE_NOOP("ViewParams", "Auto mouse click through transparent part of dock overlay."))\
    FC_VIEW_PARAM(DockOverlayAlphaRadius,int,Int,2, \
        QT_TRANSLATE_NOOP("ViewParams", "If auto mouse click through is enabled, then this radius\n" \
                                        "defines a region of alpha test under the mouse cursor.\n" \
                                        "Auto click through is only activated if all pixels within\n" \
                                        "the region are non-opaque."))\
    FC_VIEW_PARAM2(DockOverlayCheckNaviCube,bool,Bool,true, \
        QT_TRANSLATE_NOOP("ViewParams", "Leave space for Navigation Cube in dock overlay"))\
    FC_VIEW_PARAM(DockOverlayHintTriggerSize,int,Int,30, \
        QT_TRANSLATE_NOOP("ViewParams", "Auto hide hint visual display triggering width"))\
    FC_VIEW_PARAM(DockOverlayHintSize,int,Int,8, \
        QT_TRANSLATE_NOOP("ViewParams", "Auto hide hint visual display size"))\
    FC_VIEW_PARAM(DockOverlayHintTabBar,bool,Bool,false, \
        QT_TRANSLATE_NOOP("ViewParams", "Show tab bar on mouse over when auto hide"))\
    FC_VIEW_PARAM2(DockOverlayHideTabBar,bool,Bool,true, \
        QT_TRANSLATE_NOOP("ViewParams", "Hide tab bar in dock overlay"))\
    FC_VIEW_PARAM(DockOverlayHintDelay,int,Int,200, \
        QT_TRANSLATE_NOOP("ViewParams", "Delay before show hint visual"))\
    FC_VIEW_PARAM(DockOverlayAnimationDuration,int,Int,200, \
        QT_TRANSLATE_NOOP("ViewParams", "Auto hide animation duration, 0 to disable"))\
    FC_VIEW_PARAM(DockOverlayAnimationCurve,int,Int,7, \
        QT_TRANSLATE_NOOP("ViewParams", "Auto hide animation curve type"))\
    FC_VIEW_PARAM(DockOverlayHideScrollBar,bool,Bool,true, \
        QT_TRANSLATE_NOOP("ViewParams", "Hide tree/property view scroll bar in dock overlay"))\
    FC_VIEW_PARAM(DockOverlayHideHeaderView,bool,Bool,true, \
        QT_TRANSLATE_NOOP("ViewParams", "Hide tree view header view in dock overlay"))\
    FC_VIEW_PARAM(EditingTransparency,double,Float,0.5, \
       QT_TRANSLATE_NOOP("ViewParams", "Automatically make all object transparent except the one in edit"))\
    FC_VIEW_PARAM(EditingAutoTransparent,bool,Bool,true, "") \
    FC_VIEW_PARAM(HiddenLineTransparency,double,Float,0.4, \
       QT_TRANSLATE_NOOP("ViewParams","Override transparency of all objects in the scene."))\
    FC_VIEW_PARAM(HiddenLineFaceColor,unsigned long,Unsigned,0xffffffff, "") \
    FC_VIEW_PARAM(HiddenLineOverrideFaceColor,bool,Bool,true, \
       QT_TRANSLATE_NOOP("ViewParams","Enable preselection and highlight by specified color."))\
    FC_VIEW_PARAM(HiddenLineColor,unsigned long,Unsigned,0x000000ff, "") \
    FC_VIEW_PARAM(HiddenLineOverrideColor,bool,Bool,true, \
       QT_TRANSLATE_NOOP("ViewParams","Enable selection highlighting and use specified color"))\
    FC_VIEW_PARAM(HiddenLineBackground,unsigned long,Unsigned,0xffffffff, "") \
    FC_VIEW_PARAM(HiddenLineOverrideBackground,bool,Bool,false, "") \
    FC_VIEW_PARAM(HiddenLineShaded, bool, Bool, false, "") \
    FC_VIEW_PARAM(StatusMessageTimeout, int, Int, 5000, "") \
    FC_VIEW_PARAM(ShadowFlatLines, bool, Bool, true, \
       QT_TRANSLATE_NOOP("ViewParams","Draw object with 'Flat lines' style when shadow is enabled.")) \
    FC_VIEW_PARAM(ShadowDisplayMode, int, Int, 2, \
       QT_TRANSLATE_NOOP("ViewParams","Override view object display mode when shadow is enabled.")) \
    FC_VIEW_PARAM(ShadowSpotLight, bool, Bool, false, \
       QT_TRANSLATE_NOOP("ViewParams","Whether to use spot light or directional light.")) \
    FC_VIEW_PARAM(ShadowLightIntensity, double, Float, 0.8, "") \
    FC_VIEW_PARAM(ShadowLightDirectionX, double, Float, -1.0, "") \
    FC_VIEW_PARAM(ShadowLightDirectionY, double, Float, -1.0, "") \
    FC_VIEW_PARAM(ShadowLightDirectionZ, double, Float, -1.0, "") \
    FC_VIEW_PARAM(ShadowLightColor, unsigned long, Unsigned, 0xf0fdffff, "") \
    FC_VIEW_PARAM(ShadowShowGround, bool, Bool, true, \
       QT_TRANSLATE_NOOP("ViewParams","Whether to show auto generated ground face. You can specify you own ground\n"\
                                      "object by changing its view property 'ShadowStyle' to 'Shadowed', meaning\n"\
                                      "that it will only receive but not cast shadow."))\
    FC_VIEW_PARAM(ShadowGroundBackFaceCull, bool, Bool, true, \
       QT_TRANSLATE_NOOP("ViewParams","Whether to show the ground when viewing from under the ground face"))\
    FC_VIEW_PARAM(ShadowGroundScale, double, Float, 2.0, \
       QT_TRANSLATE_NOOP("ViewParams","The auto generated ground face is determined by the scene bounding box\n"\
                                      "multiplied by this scale"))\
    FC_VIEW_PARAM(ShadowGroundColor, unsigned long, Unsigned, 0x7d7d7dff, "") \
    FC_VIEW_PARAM(ShadowGroundBumpMap, std::string, ASCII, "", "") \
    FC_VIEW_PARAM(ShadowGroundTexture, std::string, ASCII, "", "") \
    FC_VIEW_PARAM(ShadowGroundTextureSize, double, Float, 100.0, \
       QT_TRANSLATE_NOOP("ViewParams","Specifies the physcal length of the ground texture image size.\n"\
                                      "Texture mappings beyond this size will be wrapped around"))\
    FC_VIEW_PARAM(ShadowGroundTransparency, double, Float, 0.0, \
       QT_TRANSLATE_NOOP("ViewParams","Specifics the ground transparency. When set to 0, the non-shadowed part\n"\
                                      "of the ground will be complete transparent, showing only the shadowed part\n"\
                                      "of the ground with some transparency."))\
    FC_VIEW_PARAM(ShadowGroundShading, bool, Bool, true, \
        QT_TRANSLATE_NOOP("ViewParams","Render ground with shading. If disabled, the ground and the shadow casted\n" \
                                       "on ground will not change shading when viewing in different angle."))\
    FC_VIEW_PARAM(ShadowExtraRedraw, bool, Bool, true, "") \
    FC_VIEW_PARAM(ShadowSmoothBorder, int, Int, 0, \
        QT_TRANSLATE_NOOP("ViewParams","Specifies the blur raidus of the shadow edge. Higher number will result in\n" \
                                       "slower rendering speed on scene change. Use a lower 'Precision' value to\n"\
                                       "counter the effect."))\
    FC_VIEW_PARAM(ShadowSpreadSize, int, Int, 0, \
        QT_TRANSLATE_NOOP("ViewParams","Specifies the spread size for a soft shadow. The resulting spread size is\n"\
                                       "dependent on the model scale"))\
    FC_VIEW_PARAM(ShadowSpreadSampleSize, int, Int, 0, \
        QT_TRANSLATE_NOOP("ViewParams","Specifies the sample size used for rendering shadow spread. A value 0\n"\
                                       "corresponds to a sampling square of 2x2. And 1 corresponds to 3x3, etc.\n"\
                                       "The bigger the size the slower the rendering speed. You can use a lower\n"\
                                       "'Precision' value to counter the effect."))\
    FC_VIEW_PARAM(ShadowPrecision, double, Float, 1.0, \
        QT_TRANSLATE_NOOP("ViewParams","Specifies shadow precision. This parameter affects the internal texture\n"\
                                       "size used to hold the casted shadows. You might want a bigger texture if\n"\
                                       "you want a hard shadow but a smaller one for soft shadow."))\
    FC_VIEW_PARAM(ShadowEpsilon, double, Float, 1e-5, \
        QT_TRANSLATE_NOOP("ViewParams","Epsilon is used to offset the shadow map depth from the model depth.\n"\
                                       "Should be set to as low a number as possible without causing flickering\n"\
                                       "in the shadows or on non-shadowed objects."))\
    FC_VIEW_PARAM(ShadowThreshold, double, Float, 0.0, \
        QT_TRANSLATE_NOOP("ViewParams","Can be used to avoid light bleeding in merged shadows cast from different objects."))\
    FC_VIEW_PARAM(ShadowBoundBoxScale, double, Float, 1.2, \
        QT_TRANSLATE_NOOP("ViewParams","Scene bounding box is used to determine the scale of the shadow texture.\n"\
                                       "You can increase the bounding box scale to avoid execessive clipping of\n"\
                                       "shadows when viewing up close in certain angle."))\
    FC_VIEW_PARAM(ShadowMaxDistance, double, Float, 0.0, \
        QT_TRANSLATE_NOOP("ViewParams","Specifics the clipping distance for when rendering shadows.\n"\
                                       "You can increase the bounding box scale to avoid execessive\n"\
                                       "clipping of shadows when viewing up close in certain angle."))\
    FC_VIEW_PARAM(ShadowTransparentShadow, bool, Bool, false, \
        QT_TRANSLATE_NOOP("ViewParams","Whether to cast shadow from transparent objects."))\
    FC_VIEW_PARAM(PropertyViewTimer, unsigned long, Unsigned, 100, "") \
    FC_VIEW_PARAM(HierarchyAscend, bool, Bool, false, \
        QT_TRANSLATE_NOOP("ViewParams","Enable selection of upper hierarhcy by repeatedly click some already\n"\
                                       "selected sub-element."))\
    FC_VIEW_PARAM(CommandHistorySize, int, Int, 20, \
        QT_TRANSLATE_NOOP("ViewParams","Maximum number of commands saved in history"))\
    FC_VIEW_PARAM(PieMenuIconSize, int, Int, 24, \
        QT_TRANSLATE_NOOP("ViewParams","Pie menu icon size"))\
    FC_VIEW_PARAM(PieMenuRadius, int, Int, 100, \
        QT_TRANSLATE_NOOP("ViewParams","Pie menu radius"))\
    FC_VIEW_PARAM(PieMenuTriggerRadius, int, Int, 60, \
        QT_TRANSLATE_NOOP("ViewParams","Pie menu hover trigger radius"))\
    FC_VIEW_PARAM(PieMenuFontSize, int, Int, 0, \
        QT_TRANSLATE_NOOP("ViewParams","Pie menu font size"))\
    FC_VIEW_PARAM(PieMenuTriggerDelay, int, Int, 200, \
        QT_TRANSLATE_NOOP("ViewParams","Pie menu sub-menu hover trigger delay, 0 to disable"))\
    FC_VIEW_PARAM(PieMenuTriggerAction, bool, Bool, false, \
        QT_TRANSLATE_NOOP("ViewParams","Pie menu action trigger on hover"))\
    FC_VIEW_PARAM(PieMenuAnimationDuration, int, Int, 250, \
        QT_TRANSLATE_NOOP("ViewParams","Pie menu animation duration, 0 to disable"))\
    FC_VIEW_PARAM(PieMenuAnimationCurve, int, Int, 38, \
        QT_TRANSLATE_NOOP("ViewParams","Pie menu animation curve type"))\
    FC_VIEW_PARAM(PieMenuCenterRadius, int, Int, 10, \
        QT_TRANSLATE_NOOP("ViewParams","Pie menu center circle radius, 0 to disable"))\
    FC_VIEW_PARAM(StickyTaskControl, bool, Bool, true, \
        QT_TRANSLATE_NOOP("ViewParams","Makes the task dialog buttons stay at top or bottom of task view."))\
    FC_VIEW_PARAM(ColorOnTop, bool, Bool, true, \
        QT_TRANSLATE_NOOP("ViewParams","Show object on top will editing its color."))\
    FC_VIEW_PARAM(ColorRecompute, bool, Bool, true, \
        QT_TRANSLATE_NOOP("ViewParams","Recompute affected object(s) after editing color."))\

#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def,_doc) \
    static const _ctype &get##_name() { return instance()->_name; }\
    static void set##_name(const _ctype &_v) { instance()->handle->Set##_type(#_name,_v); instance()->_name=_v; }\
    static void update##_name(ViewParams *self) { self->_name = self->handle->Get##_type(#_name,_def); }\
    static const char *doc##_name(); \

#undef FC_VIEW_PARAM2
#define FC_VIEW_PARAM2(_name,_ctype,_type,_def,_doc) \
    static const _ctype &get##_name() { return instance()->_name; }\
    static void set##_name(const _ctype &_v) { instance()->handle->Set##_type(#_name,_v); instance()->_name=_v; }\
    void on##_name##Changed();\
    static void update##_name(ViewParams *self) { \
        self->_name = self->handle->Get##_type(#_name,_def); \
        self->on##_name##Changed();\
    }\
    static const char *doc##_name(); \

    FC_VIEW_PARAMS

    static bool highlightIndicesOnFullSelect() {
        return getShowSelectionOnTop() && getPartialHighlightOnFullSelect();
    }

private:
#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def,_doc) \
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
