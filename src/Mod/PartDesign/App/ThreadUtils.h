// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Caio Venâncio <caio.venancio784@gmail.com>          *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

namespace PartDesign
{

static constexpr size_t ThreadClass_ISOmetric_data_size_utils = 25;
static constexpr size_t ThreadRunout_size_utils = 24;

class ThreadUtils
{
public:
    TopoDS_Shape makeThread(
        const gp_Vec& xDir,
        const gp_Vec& zDir,
        double length,
        const App::PropertyEnumeration& ThreadType,
        const App::PropertyEnumeration& ThreadSize
    );
    App::DocumentObjectExecReturn* validateParameters(const App::PropertyLinkSub& LateralFace);
    gp_Vec getThreadZAxis(const App::PropertyLinkSub& LateralFace);
    gp_Vec computePerpendicular(const gp_Vec&) const;

    using ThreadDescription = struct
    {
        const char* designation;
        double diameter;
        double pitch;
        double TapDrill;
    };
    static const std::vector<ThreadUtils::ThreadDescription> threadDescription[];

    std::vector<std::string> getThreadTypeEnums();
    std::vector<std::string> getDepthTypeEnums();
    std::vector<std::string> getThreadClass_None_Enums();
    std::vector<std::string> getThreadClass_ISOmetric_Enums();
    std::vector<std::string> getThreadClass_ISOmetricfine_Enums();
    std::vector<std::string> getThreadClass_UNC_Enums();
    std::vector<std::string> getThreadClass_UNF_Enums();
    std::vector<std::string> getThreadClass_UNEF_Enums();
    std::vector<std::string> getThreadClass_BSW_Enums();
    std::vector<std::string> getThreadClass_BSF_Enums();
    std::vector<std::string> getThreadDirectionEnums();
    std::vector<std::string> getThreadDiameters(const int threadType);
    std::vector<std::string> getThreadPitches(const int threadType, const int threadDiameter);
    std::string getThreadDesignations(
        const int threadType,
        const int threadDiameter,
        const int threadPitch
    );

    double getThroughAllLength() const;
    static const char* ThreadTypeEnums[];
    static const char* DepthTypeEnums[];
    static const char* ThreadDirectionEnums[];

    /* "None" thread profile */
    static const char* ThreadClass_None_Enums[];
    
    /* ISO metric coarse profile */
    static const char* ThreadClass_ISOmetric_Enums[];
    static const double ThreadClass_ISOmetric_data[ThreadClass_ISOmetric_data_size_utils][2];

    /* ISO metric fine profile */
    static const char* ThreadClass_ISOmetricfine_Enums[];

    /* UNC profile */
    static const char* ThreadClass_UNC_Enums[];

    /* UNF profile */
    static const char* ThreadClass_UNF_Enums[];

    /* UNEF profile */
    static const char* ThreadClass_UNEF_Enums[];

    /* NPT profile */

    /* BSP profile */

    /* BSW profile */
    static const char* ThreadClass_BSW_Enums[];

    /* BSF profile */
    static const char* ThreadClass_BSF_Enums[];

    static const double ThreadRunout[ThreadRunout_size_utils][2];

    private:
    static const char* ThreadDepthTypeEnums[];
};

}  // namespace PartDesign
