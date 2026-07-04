// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

namespace PartDesign
{

static constexpr size_t ThreadClass_ISOmetric_data_size_utils = 25;
static constexpr size_t ThreadRunout_size_utils = 24;

class ThreadUtils
{
public:
    TopoDS_Shape makeThread(const gp_Vec& xDir, const gp_Vec& zDir, double length);

    using ThreadDescription = struct
    {
        const char* designation;
        double diameter;
        double pitch;
        double TapDrill;
    };
    static const std::vector<ThreadUtils::ThreadDescription> threadDescription[];

    std::vector<std::string> getThreadTypeEnums();
    std::vector<std::string> getThreadDesignations(const int threadType);
    std::vector<std::string> getThreadPitches(const int threadType, const int threadDiameter);

private:
    static const char* ThreadDepthTypeEnums[];
    static const char* ThreadTypeEnums[];
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
    // static const char* HoleCutType_NPT_Enums[];

    /* BSP profile */
    // static const char* HoleCutType_BSP_Enums[];

    /* BSW profile */
    static const char* ThreadClass_BSW_Enums[];

    /* BSF profile */
    static const char* ThreadClass_BSF_Enums[];

    static const double ThreadRunout[ThreadRunout_size_utils][2];
};

}  // namespace PartDesign
