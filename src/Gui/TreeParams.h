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

#ifndef GUI_TREE_PARAMS_H
#define GUI_TREE_PARAMS_H


#include <Base/Parameter.h>
#include <App/DynamicProperty.h>

namespace Gui {

/** Helper class to read/write tree view options
 *
 * The parameters are stored under group "User parameter:BaseApp/Preferences/TreeView".
 * Call TreeParams::ParamName/setParamName() to get/set parameter.
 * To add a new parameter, add a new line under FC_TREE_PARAM using macro
 *
 * @code
 *      FC_TREE_PARAM(parameter_name, c_type, parameter_type, default_value, documentation)
 * @endcode
 *
 * If there is special handling on parameter change, use FC_TREE_PARAM2()
 * instead, and add a function with the following signature in Tree.cpp,
 *
 * @code
 *      void TreeParams:on<ParamName>Changed()
 * @endcode
 */
class GuiExport TreeParams: public ParameterGrp::ObserverType {
public:
    TreeParams();
    virtual ~TreeParams();
    void OnChange(Base::Subject<const char*> &, const char* sReason);
    static TreeParams *instance();

    ParameterGrp::handle getHandle() {
        return handle;
    }

#define FC_TREE_PARAMS \
    FC_TREE_PARAM2(SyncSelection,bool,Bool,true, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM2(CheckBoxesSelection,bool,Bool,false, \
       QT_TRANSLATE_NOOP("TreeParams", "Show checkbox for each item in the tree view"))\
    FC_TREE_PARAM(SyncView,bool,Bool,true, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(PreSelection,bool,Bool,true, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(SyncPlacement,bool,Bool,false, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(RecordSelection,bool,Bool,true, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM2(DocumentMode,int,Int,2, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(StatusTimeout,int,Int,100, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(SelectionTimeout,int,Int,100, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(PreSelectionTimeout,int,Int,500, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(PreSelectionDelay,int,Int,700, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(PreSelectionMinDelay,int,Int,200, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(RecomputeOnDrop,bool,Bool,true, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(KeepRootOrder,bool,Bool,true, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(TreeActiveAutoExpand,bool,Bool,true, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(TreeActiveColor,unsigned long, Unsigned, 3873898495, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(TreeActiveBold,bool, Bool, true, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(TreeActiveItalic,bool, Bool, false, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(TreeActiveUnderlined,bool, Bool, false, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(TreeActiveOverlined,bool, Bool, false, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(Indentation,int,Int,0, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM(LabelExpression,bool,Bool,false, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM2(IconSize,int,Int,0, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM2(FontSize,int,Int,0, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM2(ItemSpacing,int,Int,2, \
       QT_TRANSLATE_NOOP("TreeParams", ""))\
    FC_TREE_PARAM2(ItemBackground,unsigned long,Unsigned,0, \
       QT_TRANSLATE_NOOP("TreeParams", "Tree view item background. Only effecitve in overlay."))\
    FC_TREE_PARAM2(ItemBackgroundPadding,int,Int,10, \
       QT_TRANSLATE_NOOP("TreeParams", "Tree view item background padding."))\
    FC_TREE_PARAM2(HideColumn,bool,Bool,false, \
       QT_TRANSLATE_NOOP("TreeParams", "Hide extra tree view column for item description."))\
    FC_TREE_PARAM(HideScrollBar,bool,Bool,true, \
        QT_TRANSLATE_NOOP("TreeParams", "Hide tree view scroll bar in dock overlay"))\
    FC_TREE_PARAM(HideHeaderView,bool,Bool,true, \
        QT_TRANSLATE_NOOP("TreeParams", "Hide tree view header view in dock overlay"))\
    FC_TREE_PARAM2(ResizableColumn,bool,Bool,false, \
       QT_TRANSLATE_NOOP("TreeParams", "Allow tree view columns to be manually resized"))\
    FC_TREE_PARAM(ColumnSize1,int,Int,0, "") \
    FC_TREE_PARAM(ColumnSize2,int,Int,0, "") \

#undef FC_TREE_PARAM
#define FC_TREE_PARAM(_name,_ctype,_type,_def,_doc) \
    static const _ctype & get##_name() \
        { return instance()->_##_name; }\
    static const _ctype & _name () \
        { return instance()->_##_name; }\
    static void set##_name(const _ctype &_v) \
        { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    static void update##_name(TreeParams *self) \
        { self->_##_name = self->handle->Get##_type(#_name,_def); }\
    static const char *doc##_name(); \

#undef FC_TREE_PARAM2
#define FC_TREE_PARAM2(_name,_ctype,_type,_def,_doc) \
    static const _ctype & get##_name() \
        { return instance()->_##_name; }\
    static const _ctype & _name () \
        { return instance()->_##_name; }\
    static void set##_name(const _ctype &_v) \
        { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    void on##_name##Changed();\
    static void update##_name(TreeParams *self) { \
        self->_##_name = self->handle->Get##_type(#_name,_def); \
        self->on##_name##Changed();\
    }\
    static const char *doc##_name(); \

    FC_TREE_PARAMS

private:
#undef FC_TREE_PARAM
#define FC_TREE_PARAM(_name,_ctype,_type,_def,_doc) \
    _ctype _##_name;

#undef FC_TREE_PARAM2
#define FC_TREE_PARAM2 FC_TREE_PARAM

    FC_TREE_PARAMS
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(TreeParams*),App::CStringHasher,App::CStringHasher> funcs;
};

#undef FC_TREE_PARAM
#undef FC_TREE_PARAM2

} // namespace Gui

#endif // GUI_TREE_PARAMS_H
