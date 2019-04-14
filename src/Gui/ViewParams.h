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

/** Convenient class to obtain view provider related parameters */
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
    FC_VIEW_PARAM(DefaultShapeColor,unsigned long,Unsigned,0xCCCCCC00) \
    FC_VIEW_PARAM(DefaultShapeLineWidth,int,Int,2) \
    FC_VIEW_PARAM(CoinCycleCheck,bool,Bool,true) \

#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def) \
    _ctype get##_name() const { return _name; }\
    void set##_name(_ctype _v) { handle->Set##_type(#_name,_v); _name=_v; }

    FC_VIEW_PARAMS

private:
#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def) \
    _ctype _name;

    FC_VIEW_PARAMS
    ParameterGrp::handle handle;
};

#undef FC_VIEW_PARAM

} // namespace Gui

#endif // GUI_VIEW_PARAMS_H
