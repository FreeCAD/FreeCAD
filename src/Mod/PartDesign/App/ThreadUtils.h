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
        const int threadType,
        const int threadSize,
        const int leftHanded,
        App::PropertyEnumeration& ThreadClass,
        const bool isInternalThread
    );
    App::DocumentObjectExecReturn* validateParameters(const App::PropertyLinkSub& LateralFace);
    bool isInternalFace(const App::PropertyLinkSub& faceProp, const TopoDS_Shape& solid);
    gp_Vec getThreadZAxis(const App::PropertyLinkSub& LateralFace);
    gp_Pnt getThreadAxisOrigin(const App::PropertyLinkSub& LateralFace);
    gp_Vec computePerpendicular(const gp_Vec&) const;
    gp_Pnt getThreadStartPoint(const App::PropertyLinkSub& lateralFace, const App::PropertyLinkSub& startPlane);

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
    std::vector<std::string> getThreadDiameters(const int threadType);
    std::vector<std::string> getThreadMinorDiameters(const int threadType);
    double getMinorDiameter(const int threadType, const int size);
    gp_Pnt getThreadStartPoint(const App::PropertyLinkSub& lateralFace, const gp_Dir& zDir);
    gp_Dir getThreadAxisDir(const App::PropertyLinkSub& LateralFace);
    Part::TopoShape reduceExternalThreadBase(Part::TopoShape base, const App::PropertyLinkSub& lateralFace, double majorDiameter, double minorDiameter, double length);
    gp_Pnt getPlaneLineIntersection(const gp_Pln& plane, const gp_Lin& line);
    std::vector<gp_Pnt> findLineCurveIntersections(const gp_Lin& line, const BRepAdaptor_Curve& curve);
    std::vector<std::string> getThreadPitches(const int threadType, const int threadDiameter);
    std::string getThreadDesignations(
        const int threadType,
        const int threadDiameter,
        const int threadPitch
    );

    double getThroughAllLength() const;
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

    void executeReadThreadDefinitions(){
        library.readThreadDefinitions();
    }

    struct ThreadDefinition 
    {
        std::string id;
        std::string name;
        std::string description;
        std::string threadType;
        std::filesystem::path file;
        int depthType;
        std::vector<std::string> sketches;
        std::vector<std::string> spreadsheets;
        std::vector<std::string> sizes; //diameters
        std::vector<std::string> minorDiameters;
        std::vector<std::string> pitches;
        std::vector<std::string> designations;
        std::vector<std::string> tapDrills;

        ThreadDefinition() : depthType(0) {}

        ThreadDefinition(const std::string& n, const std::string& desc)
            : name(n), description(desc), depthType(0) {}

        ThreadDefinition(
            const std::string& n, 
            const std::string& desc, 
            const std::string& type,
            int depth = 0
        ) : name(n), description(desc), threadType(type), depthType(depth) {}
    };

    static std::optional<ThreadDefinition> findMetadata(App::Document* doc);

    std::vector<std::string> getThreadTypeEnums();
    std::vector<std::string> getThreadTypeNameEnums();

    const std::vector<ThreadDefinition>& getThreadDefinitions() const
    {
        return library.getDefinitions();
    }
    std::vector<std::string> getThreadTypeName2Enums();
    double getCylinderDiameter(const TopoDS_Face& face);
    double getLateralFaceDiameter(const App::PropertyLinkSub& lateralFace);
    int findNearestThreadSize(const int threadType, const double size);
    int findNearestMinorThreadSize(const int threadType, const double diameter);
private:
    double getThreadClassClearance(int threadType, int threadSize, App::PropertyEnumeration& ThreadClass) const;
    void rotateToNormal(const gp_Dir& helixAxis, const gp_Dir& normalAxis, TopoDS_Shape& helixShape) const;
    static const char* ThreadDepthTypeEnums[];

    //TODO: ThreadLibrary should be static member or singleton to improve performance
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
            std::optional<ThreadDefinition> readThreadDocument(App::Document* doc);
            void findSpreadsheets(App::Document* doc, ThreadDefinition& definition);
    };

    ThreadLibrary library;
};

}  // namespace PartDesign
