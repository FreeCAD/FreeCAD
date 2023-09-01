/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef PARTDESIGN_Hole_H
#define PARTDESIGN_Hole_H

#include <optional>
#include <App/PropertyUnits.h>
#include "json_fwd.hpp"
#include "FeatureSketchBased.h"

class Property;

namespace Base {
class XMLReader;
}

namespace PartDesign
{

static constexpr size_t ThreadClass_ISOmetric_data_size = 25;
static constexpr size_t ThreadRunout_size = 24;

class PartDesignExport Hole : public ProfileBased
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Hole);

public:
    Hole();

    App::PropertyBool           Threaded;
    App::PropertyBool           ModelThread;
    App::PropertyLength         ThreadPitch;
    App::PropertyEnumeration    ThreadType;
    App::PropertyEnumeration    ThreadSize;
    App::PropertyEnumeration    ThreadClass;
    App::PropertyEnumeration    ThreadFit;
    App::PropertyLength         Diameter;
    App::PropertyEnumeration    ThreadDirection;
    App::PropertyEnumeration    HoleCutType;
    App::PropertyBool           HoleCutCustomValues;
    App::PropertyLength         HoleCutDiameter;
    App::PropertyLength         HoleCutDepth;
    App::PropertyAngle          HoleCutCountersinkAngle;
    App::PropertyEnumeration    DepthType;
    App::PropertyLength         Depth;
    App::PropertyEnumeration    ThreadDepthType;
    App::PropertyLength         ThreadDepth;
    App::PropertyEnumeration    DrillPoint;
    App::PropertyAngle          DrillPointAngle;
    App::PropertyBool           DrillForDepth;
    App::PropertyBool           Tapered;
    App::PropertyAngle          TaperedAngle;
    App::PropertyBool           UseCustomThreadClearance;
    App::PropertyLength         CustomThreadClearance;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;

    /// returns the type name of the view provider
    const char* getViewProviderName() const override {
        return "PartDesignGui::ViewProviderHole";
    }
    //@}
    short mustExecute() const override;

    using ThreadDescription = struct {
        const char * designation;
        double diameter;
        double pitch;
        double CoreHole;
    };
    static const ThreadDescription threadDescription[][171];

    static const double metricHoleDiameters[36][4];

    using UTSClearanceDefinition = struct {
        std::string designation;
        double close;
        double normal;
        double loose;
    };
    static const UTSClearanceDefinition UTSHoleDiameters[22];

    void Restore(Base::XMLReader & reader) override;

    virtual void updateProps();

protected:
    void onChanged(const App::Property* prop) override;
    static const App::PropertyAngle::Constraints floatAngle;

private:
    static const char* DepthTypeEnums[];
    static const char* ThreadDepthTypeEnums[];
    static const char* ThreadTypeEnums[];
    static const char* ClearanceMetricEnums[];
    static const char* ClearanceUTSEnums[];
    static const char* DrillPointEnums[];
    static const char* ThreadDirectionEnums[];

    /* "None" thread profile */
    static const char* HoleCutType_None_Enums[];
    static const char* ThreadSize_None_Enums[];
    static const char* ThreadClass_None_Enums[];

    /* ISO metric coarse profile */
    static std::vector<std::string> HoleCutType_ISOmetric_Enums;
    static const char* ThreadSize_ISOmetric_Enums[];
    static const char* ThreadClass_ISOmetric_Enums[];
    static const double ThreadClass_ISOmetric_data[ThreadClass_ISOmetric_data_size][2];

    /* ISO metric fine profile */
    static std::vector<std::string> HoleCutType_ISOmetricfine_Enums;
    static const char* ThreadSize_ISOmetricfine_Enums[];
    static const char* ThreadClass_ISOmetricfine_Enums[];

    /* UNC profile */
    static const char* HoleCutType_UNC_Enums[];
    static const char* ThreadSize_UNC_Enums[];
    static const char* ThreadClass_UNC_Enums[];

    /* UNF profile */
    static const char* HoleCutType_UNF_Enums[];
    static const char* ThreadSize_UNF_Enums[];
    static const char* ThreadClass_UNF_Enums[];

    /* UNEF profile */
    static const char* HoleCutType_UNEF_Enums[];
    static const char* ThreadSize_UNEF_Enums[];
    static const char* ThreadClass_UNEF_Enums[];

    static const double ThreadRunout[ThreadRunout_size][2];

    /* Counter-xxx */
//public:
    // Dimension for counterbore
    struct CounterBoreDimension {
        std::string thread;
        double diameter;
        double depth;
        static const CounterBoreDimension nothing;
    };
    // Dimension for countersink
    struct CounterSinkDimension {
        std::string thread;
        double diameter;
        static const CounterSinkDimension nothing;
    };

    // cut dimensions for a screwtype
    class CutDimensionSet {
    public:
        enum CutType { Counterbore, Countersink };
        enum ThreadType { Metric, MetricFine };

        CutDimensionSet():cut_type(Counterbore),thread_type(Metric),angle(0.0) {}
        CutDimensionSet(const std::string &nme,
              std::vector<CounterBoreDimension> &&d, CutType cut, ThreadType thread, double angle = 0.0);
        CutDimensionSet(const std::string &nme,
              std::vector<CounterSinkDimension> &&d, CutType cut, ThreadType thread, double angle = 0.0);

        const CounterBoreDimension &get_bore(const std::string &t) const;
        const CounterSinkDimension &get_sink(const std::string &t) const;

        std::vector<CounterBoreDimension> bore_data;
        std::vector<CounterSinkDimension> sink_data;
        CutType cut_type;
        ThreadType thread_type;
        std::string name;
        double angle;
    };

    class CutDimensionKey {
        std::string thread_type;
        std::string cut_name;
    public:
        CutDimensionKey() = default;
        CutDimensionKey(const std::string &t, const std::string &c);
        bool operator<(const CutDimensionKey &b) const;
    };

    std::map<CutDimensionKey, CutDimensionSet> HoleCutTypeMap;

    const CutDimensionSet& find_cutDimensionSet(const std::string &t,
          const std::string &c);

    const CutDimensionSet& find_cutDimensionSet(const CutDimensionKey &k);

    void addCutType(const CutDimensionSet& dimensions);
    bool isDynamicCounterbore(const std::string &thread, const std::string &holeCutType);
    bool isDynamicCountersink(const std::string &thread, const std::string &holeCutType);
    void updateHoleCutParams();
    std::optional<double> determineDiameter() const;
    void updateDiameterParam();
    void updateThreadDepthParam();
    void readCutDefinitions();

    double getThreadClassClearance() const;
    double getThreadRunout(int mode = 1) const;
    double getThreadPitch() const;
    void rotateToNormal(const gp_Dir& helixAxis, const gp_Dir& normalAxis, TopoDS_Shape& helixShape) const;
    gp_Vec computePerpendicular(const gp_Vec&) const;
    TopoDS_Shape makeThread(const gp_Vec&, const gp_Vec&, double);
    TopoDS_Compound findHoles(const TopoDS_Shape& profileshape, const TopoDS_Shape& protohole) const;

    // helpers for nlohmann json
    friend void from_json(const nlohmann::json &j, CounterBoreDimension &t);
    friend void from_json(const nlohmann::json &j, CounterSinkDimension &t);
    friend void from_json(const nlohmann::json &j, CutDimensionSet &t);
};

} //namespace PartDesign


#endif // PART_Hole_H
