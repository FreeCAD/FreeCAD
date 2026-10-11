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

#include <BRepAdaptor_Curve.hxx>
#include <TopoDS_Wire.hxx>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

namespace PartDesign
{

static constexpr size_t ThreadClass_ISOmetric_data_size_utils = 25;
static constexpr size_t ThreadRunout_size_utils = 24;

class PartDesignExport ThreadUtils
{
public:
    TopoDS_Shape makeThread(
        const gp_Vec& xDir,
        const gp_Vec& zDir,
        double length,
        const int threadType,
        const int threadSize,
        const int threadPitch,
        const int leftHanded,
        App::PropertyEnumeration& ThreadClass,
        const bool isInternalThread,
        const bool tapered,
        const double taperedAngle,
        const bool UseCustomThreadClearance,
        const double CustomThreadClearance,
        const double farEndTrim,
        // Distance from the nominal entrance to the useful far end. Zero retains
        // the legacy reference at the useful far end for existing callers.
        const double usefulThreadLength = 0.0
    );
    App::DocumentObjectExecReturn* validateParameters(const App::PropertyLinkSub& LateralFace);
    bool isInternalFace(const App::PropertyLinkSub& faceProp, const TopoDS_Shape& solid);
    gp_Vec getThreadZAxis(const App::PropertyLinkSub& LateralFace) const;
    gp_Pnt getThreadAxisOrigin(const App::PropertyLinkSub& LateralFace);
    gp_Vec computePerpendicular(const gp_Vec&) const;
    gp_Pnt getThreadStartPoint(
        const App::PropertyLinkSub& lateralFace,
        const App::PropertyLinkSub& startPlane
    );

    using ThreadDescription = struct
    {
        const char* designation;
        double diameter;
        double pitch;
        double TapDrill;
    };
    static const std::vector<ThreadUtils::ThreadDescription> threadDescription[];

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
    std::vector<std::string> getThreadDiameters(const int threadType) const;
    std::vector<std::string> getThreadMinorDiameters(const int threadType);
    double getMinorDiameter(const int threadType, const int size, const int pitch) const;
    gp_Pnt getThreadStartPoint(const App::PropertyLinkSub& lateralFace, const gp_Dir& zDir) const;
    gp_Dir getThreadAxisDir(const App::PropertyLinkSub& LateralFace);
    /// Major/minor diameters are measured at startPoint; length follows the face axis.
    Part::TopoShape reduceExternalThreadBase(
        Part::TopoShape base,
        const App::PropertyLinkSub& lateralFace,
        double majorDiameter,
        double minorDiameter,
        double length,
        const gp_Pnt& startPoint,
        bool tapered,
        double taperedAngle
    );
    gp_Pnt getPlaneLineIntersection(const gp_Pln& plane, const gp_Lin& line);
    std::vector<gp_Pnt> findLineCurveIntersections(const gp_Lin& line, const BRepAdaptor_Curve& curve);
    std::vector<std::string> getThreadPitches(const int threadType, const int threadDiameter) const;
    double getThreadPitch(const int threadType, const int threadDiameter, const int threadPitch) const;
    struct NormalizedThreadSupport
    {
        double xAtYZero;
        double slope;
    };
    struct ThreadEndTrims
    {
        double nearEnd;
        double farEnd;
    };
    static ThreadEndTrims calculateThreadEndTrims(
        const TopoDS_Wire& normalizedProfile,
        double pitch,
        const NormalizedThreadSupport& nearSupport,
        const NormalizedThreadSupport& farSupport
    );
    ThreadEndTrims getThreadEndTrims(
        int threadType,
        int sizeIndex,
        int pitchIndex,
        bool isInternalThread,
        double supportDiameter,
        double usefulThreadLength,
        bool tapered,
        double taperedAngle,
        bool useCustomThreadClearance,
        double customThreadClearance,
        App::PropertyEnumeration& threadClass,
        bool referenceAtStart = false,
        // Support radius change per unit distance along +zDir.
        double supportTaperSlope = 0.0
    ) const;
    std::vector<std::string> getThreadClasses(const int threadType, const bool isInternal) const;
    std::string getThreadDesignations(
        const int threadType,
        const int threadDiameter,
        const int threadPitch
    );

    double getThroughAllLength(TopoShape base) const;
    double getUpToGeometryLength(const App::PropertyLinkSub& upToGeometry, const App::PropertyLinkSub& lateralFace, const App::PropertyLinkSub& startPlane);
    double getUpToFirstLength(
        const App::PropertyLinkSub& lateralFace,
        const App::PropertyLinkSub& startFace,
        Part::TopoShape base,
        const int threadType,
        const int threadSize,
        const bool isInternalThread,
        const bool UseCustomThreadClearance,
        const double CustomThreadClearance,
        App::PropertyEnumeration& ThreadClass,
        const bool tapered,
        const double taperedAngle
    );
    gp_Pnt getThreadEndPoint(
        const App::PropertyLinkSub& lateralFace,
        const App::PropertyLinkSub& upToGeometry,
        const gp_Pnt& startPoint
    );
    gp_Pnt getThreadFarPoint(const App::PropertyLinkSub& lateralFace, const gp_Dir& zDir) const;
    double getThreadProfileAngle();
    bool isThreadConical(const int threadType) const;
    /// Signed semi-angle in radians relative to the lateral face axis; zero for cylinders.
    double getConicalAngle(const App::PropertyLinkSub& lateralFace);
    static const char* ThreadTypeEnums[];
    static const char* ThreadTypeNameEnums[];
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

    void executeReadThreadDefinitions()
    {
        library.readThreadDefinitions();
    }

    struct ThreadDefinition
    {
        enum class ProfileStatus
        {
            Missing,
            Valid,
            Invalid
        };

        struct Profile
        {
            Part::TopoShape wire;
            double minX;
            double maxX;
            double minY;
            double maxY;
        };

        std::string id;
        std::string name;
        std::string description;
        std::string threadType;
        bool isConical;
        std::filesystem::path file;
        int depthType;
        std::vector<std::string> sketches;
        std::vector<std::string> spreadsheets;
        std::vector<std::string> sizes;  // diameters
        std::vector<std::string> minorDiameters;
        std::vector<std::string> pitches;
        std::vector<std::string> designations;
        std::vector<std::string> tapDrills;
        std::vector<std::string> internalClearances;
        std::vector<std::string> externalClearances;
        std::optional<Profile> profile;
        std::optional<Profile> externalProfile;
        ProfileStatus profileStatus;
        ProfileStatus externalProfileStatus;
        std::string profileDiagnostic;
        std::string externalProfileDiagnostic;

        ThreadDefinition()
            : isConical(false)
            , depthType(0)
            , profileStatus(ProfileStatus::Missing)
            , externalProfileStatus(ProfileStatus::Missing)
        {}

        ThreadDefinition(const std::string& n, const std::string& desc)
            : name(n)
            , description(desc)
            , isConical(false)
            , depthType(0)
            , profileStatus(ProfileStatus::Missing)
            , externalProfileStatus(ProfileStatus::Missing)
        {}

        ThreadDefinition(
            const std::string& n,
            const std::string& desc,
            const std::string& type,
            int depth = 0
        )
            : name(n)
            , description(desc)
            , threadType(type)
            , isConical(false)
            , depthType(depth)
            , profileStatus(ProfileStatus::Missing)
            , externalProfileStatus(ProfileStatus::Missing)
        {}
    };

    static std::optional<ThreadDefinition> findMetadata(App::Document* doc);
    static void findThreadProfiles(
        App::Document* doc,
        ThreadDefinition& definition,
        const std::string& source = "<document>"
    );

    std::vector<std::string> getThreadTypeEnums();
    std::vector<std::string> getThreadTypeNameEnums() const;

    const std::vector<ThreadDefinition>& getThreadDefinitions() const
    {
        return library.getDefinitions();
    }
    std::vector<std::string> getThreadTypeName2Enums();
    double getCylinderDiameter(const TopoDS_Face& face);
    double getLateralFaceDiameter(const App::PropertyLinkSub& lateralFace);
    double getLateralFaceDiameter(
        const App::PropertyLinkSub& lateralFace,
        const App::PropertyLinkSub& startPlane
    );
    bool isConicalFace(const App::PropertyLinkSub& lateralFace) const;
    int findNearestThreadSize(const int threadType, const double size);
    struct ThreadSizeSelection
    {
        int sizeIndex;
        int pitchIndex;
        double minorDiameter;
    };
    std::optional<ThreadSizeSelection> findNearestMinorThreadSize(
        const int threadType,
        const double diameter
    ) const;
private:
    struct ResolvedThreadSelection
    {
        const ThreadDefinition* definition;
        size_t row;
        double majorDiameter;
        double pitch;
    };

    ResolvedThreadSelection resolveThreadSelection(
        int threadType,
        int sizeIndex,
        int pitchIndex
    ) const;
    std::optional<Part::TopoShape> prepareThreadProfile(
        int threadType,
        int sizeIndex,
        int pitchIndex,
        bool isInternalThread,
        double majorRadius,
        const gp_Vec& xDir,
        const gp_Vec& zDir
    ) const;
    Part::TopoShape makeLegacyThreadProfile(
        const std::string& threadType,
        double majorRadius,
        double pitch,
        const gp_Vec& xDir,
        const gp_Vec& zDir
    ) const;
    Part::TopoShape makeExtendedWhitworthThreadProfile(
        double majorRadius,
        double pitch,
        const gp_Vec& xDir,
        const gp_Vec& zDir
    ) const;
    std::optional<double> estimateKnownMinorDiameter(
        const std::string& threadDefinitionId,
        double majorDiameter,
        double pitch
    ) const;
    double getMinorDiameterForRow(
        const ThreadDefinition& definition,
        size_t row,
        bool preferExternalProfile
    ) const;
    double getThreadClassClearance(
        int threadType,
        int threadSize,
        App::PropertyEnumeration& ThreadClass
    ) const;
    void rotateToNormal(const gp_Dir& helixAxis, const gp_Dir& normalAxis, TopoDS_Shape& helixShape) const;
    static const char* ThreadDepthTypeEnums[];

    // TODO: ThreadLibrary should be static member or singleton to improve performance
    class ThreadLibrary
    {
    public:
        ThreadLibrary();
        void readThreadDefinitions();

        const std::vector<ThreadDefinition>& getDefinitions() const
        {
            return definitions;
        }


    private:
        std::vector<ThreadDefinition> definitions;
        std::optional<ThreadDefinition> readThreadDefinition(const Base::FileInfo& file);
        std::optional<ThreadDefinition> readThreadDocument(
            App::Document* doc,
            const std::string& source
        );
        void findSpreadsheets(App::Document* doc, ThreadDefinition& definition);
    };

    ThreadLibrary library;
};

}  // namespace PartDesign
