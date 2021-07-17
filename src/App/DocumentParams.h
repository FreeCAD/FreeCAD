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

#ifndef DOCUMENT_PARAMS_H
#define DOCUMENT_PARAMS_H

#include <Base/Parameter.h>
#include "DynamicProperty.h"

namespace App {

/** Convenient class to obtain Document related parameters 
 *
 * The parameters are under group "User parameter:BaseApp/Preferences/Document"
 *
 * To add a new parameter, add a new line under FC_DOCUMENT_PARAMS using macro
 *
 * @code
 *      FC_DOCUMENT_PARAM(parameter_name, c_type, parameter_type, default_value)
 * @endcode
 *
 * If there is special handling on parameter change, use FC_DOCUMENT_PARAM2()
 * instead, and add a function with the following signature in DocumentParams.cpp,
 *
 * @code
 *      void DocumentParams:on<ParamName>Changed()
 * @endcode
 */
class AppExport DocumentParams: public ParameterGrp::ObserverType {
public:
    DocumentParams();
    virtual ~DocumentParams();
    void OnChange(Base::Subject<const char*> &, const char* sReason);
    static DocumentParams *instance();

    ParameterGrp::handle getHandle() {
        return handle;
    }

#define FC_DOCUMENT_PARAMS \
    FC_DOCUMENT_PARAM(prefAuthor, std::string, ASCII, "") \
    FC_DOCUMENT_PARAM(prefSetAuthorOnSave, bool, Bool, false) \
    FC_DOCUMENT_PARAM(prefCompany, std::string, ASCII, "") \
    FC_DOCUMENT_PARAM(prefLicenseType, int, Int, 0) \
    FC_DOCUMENT_PARAM(prefLicenseUrl, std::string, ASCII, "") \
    FC_DOCUMENT_PARAM(CompressionLevel, int, Int, 3) \
    FC_DOCUMENT_PARAM(CheckExtension, bool, Bool, true) \
    FC_DOCUMENT_PARAM(ForceXML, int, Int, 3) \
    FC_DOCUMENT_PARAM(SplitXML, bool, Bool, true) \
    FC_DOCUMENT_PARAM(PreferBinary, bool, Bool, false) \
    FC_DOCUMENT_PARAM(AutoRemoveFile, bool, Bool, true) \
    FC_DOCUMENT_PARAM(BackupPolicy, bool, Bool, true) \
    FC_DOCUMENT_PARAM(CreateBackupFiles, bool, Bool, true) \
    FC_DOCUMENT_PARAM(UseFCBakExtension, bool, Bool, false) \
    FC_DOCUMENT_PARAM(SaveBackupDateFormat, std::string, ASCII, "%Y%m%d-%H%M%S") \
    FC_DOCUMENT_PARAM(CountBackupFiles, int, Int, 1) \
    FC_DOCUMENT_PARAM(OptimizeRecompute, bool, Bool, true) \
    FC_DOCUMENT_PARAM(CanAbortRecompute, bool, Bool, true) \
    FC_DOCUMENT_PARAM(UseHasher, bool, Bool, true) \
    FC_DOCUMENT_PARAM(ViewObjectTransaction, bool, Bool, false) \
    FC_DOCUMENT_PARAM(WarnRecomputeOnRestore, bool, Bool, true) \
    FC_DOCUMENT_PARAM(NoPartialLoading, bool, Bool, false) \
    FC_DOCUMENT_PARAM(ThumbnailNoBackground, bool, Bool, false) \
    FC_DOCUMENT_PARAM(ThumbnailSampleSize, int, Int, 0) \
    FC_DOCUMENT_PARAM(GeoGroupAllowCrossLink, bool, Bool, true) \
    FC_DOCUMENT_PARAM(DuplicateLabels, bool, Bool, false) \
    FC_DOCUMENT_PARAM(TransactionOnRecompute, bool, Bool, false) \
    FC_DOCUMENT_PARAM(RelativeStringID, bool, Bool, true) \

#undef FC_DOCUMENT_PARAM
#define FC_DOCUMENT_PARAM(_name,_ctype,_type,_def) \
    static const _ctype & _name() { return instance()->_##_name; }\
    static void set_##_name(_ctype _v) { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    static void update##_name(DocumentParams *self) { self->_##_name = self->handle->Get##_type(#_name,_def); }\

#undef FC_DOCUMENT_PARAM2
#define FC_DOCUMENT_PARAM2(_name,_ctype,_type,_def) \
    static const _ctype & _name() { return instance()->_##_name; }\
    static void set_##_name(_ctype _v) { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    void on##_name##Changed();\
    static void update##_name(DocumentParams *self) { \
        self->_##_name = self->handle->Get##_type(#_name,_def); \
        self->on##_name##Changed();\
    }\

    FC_DOCUMENT_PARAMS

private:
#undef FC_DOCUMENT_PARAM
#define FC_DOCUMENT_PARAM(_name,_ctype,_type,_def) \
    _ctype _##_name;

#undef FC_DOCUMENT_PARAM2
#define FC_DOCUMENT_PARAM2 FC_DOCUMENT_PARAM

    FC_DOCUMENT_PARAMS
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(DocumentParams*),App::CStringHasher,App::CStringHasher> funcs;
};

#undef FC_DOCUMENT_PARAM
#undef FC_DOCUMENT_PARAM2

} // namespace App

#endif // DOCUMENT_PARAMS_H
