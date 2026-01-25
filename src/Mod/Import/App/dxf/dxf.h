// SPDX-License-Identifier: BSD-3-Clause

// dxf.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
// modified 2018 wandererfan

#pragma once

#ifdef _MSC_VER
# pragma warning(disable : 4251)
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <Base/Interpreter.h>
#include <Base/Matrix.h>
#include <Base/Vector3D.h>
#include <Base/Console.h>
#include <Base/Color.h>
#include <Mod/Import/ImportGlobal.h>

// For some reason Cpplint complains about some of the categories used by Clang-tidy
// However, cpplint also does not seem to use NOLINTE BEGIN and NOLINT END so we must use
// NOLINT NEXT LINE on each occurrence. [spaces added to avoid being seen by lint]
using ColorIndex_t = int;  // DXF color index

// The C++ version we use does not support designated initiailzers, so we have a class to set this
// up
class DxfUnits
{
public:
    using eDxfUnits_t = enum
    {
        eUnspecified = 0,  // Unspecified (No units)
        eInches,
        eFeet,
        eMiles,
        eMillimeters,
        eCentimeters,
        eMeters,
        eKilometers,
        eMicroinches,
        eMils,
        eYards,
        eAngstroms,
        eNanometers,
        eMicrons,
        eDecimeters,
        eDekameters,
        eHectometers,
        eGigameters,
        eAstronomicalUnits,
        eLightYears,
        eParsecs,
        kMaxUnit
    };

private:
    // NOLINTNEXTLINE(readability/nolint)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    DxfUnits()
    {
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        m_factors[eInches] = 25.4;
        m_factors[eFeet] = 25.4 * 12;
        m_factors[eMiles] = 1609344.0;
        m_factors[eMillimeters] = 1.0;
        m_factors[eCentimeters] = 10.0;
        m_factors[eMeters] = 1000.0;
        m_factors[eKilometers] = 1000000.0;
        m_factors[eMicroinches] = 25.4 / 1000.0;
        m_factors[eMils] = 25.4 / 1000.0;
        m_factors[eYards] = 3 * 12 * 25.4;
        m_factors[eAngstroms] = 0.0000001;
        m_factors[eNanometers] = 0.000001;
        m_factors[eMicrons] = 0.001;
        m_factors[eDecimeters] = 100.0;
        m_factors[eDekameters] = 10000.0;
        m_factors[eHectometers] = 100000.0;
        m_factors[eGigameters] = 1000000000000.0;
        m_factors[eAstronomicalUnits] = 149597870690000.0;
        m_factors[eLightYears] = 9454254955500000000.0;
        m_factors[eParsecs] = 30856774879000000000.0;
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    }

public:
    static double Factor(eDxfUnits_t enumValue)
    {
        // NOLINTNEXTLINE(readability/nolint)
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        return Instance.m_factors[enumValue];
    }
    static bool IsValid(eDxfUnits_t enumValue)
    {
        return enumValue > eUnspecified && enumValue <= eParsecs;
    }

private:
    static const DxfUnits Instance;
    // NOLINTNEXTLINE(readability/nolint)
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
    double m_factors[kMaxUnit];
};

// spline data for reading
struct SplineData
{
    Base::Vector3d norm;
    int degree = 0;
    int knots = 0;
    int control_points = 0;
    int fit_points = 0;
    int flag = 0;
    std::list<double> starttanx;
    std::list<double> starttany;
    std::list<double> starttanz;
    std::list<double> endtanx;
    std::list<double> endtany;
    std::list<double> endtanz;
    std::list<double> knot;
    std::list<double> weight;
    std::list<double> controlx;
    std::list<double> controly;
    std::list<double> controlz;
    std::list<double> fitx;
    std::list<double> fity;
    std::list<double> fitz;
};

//***************************
// data structures for writing
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
struct point3D
{
    double x;
    double y;
    double z;
};

struct SplineDataOut
{
    point3D norm;
    int degree;
    int knots;
    int control_points;
    int fit_points;
    int flag;
    point3D starttan;
    point3D endtan;
    std::vector<double> knot;
    std::vector<double> weight;
    std::vector<point3D> control;
    std::vector<point3D> fit;
};

struct LWPolyDataOut
{
    int nVert;
    int Flag;
    double Width;
    double Elev;
    double Thick;
    std::vector<point3D> Verts;
    std::vector<double> StartWidth;
    std::vector<double> EndWidth;
    std::vector<double> Bulge;
    point3D Extr;
};

// Statistics reporting structure
struct DxfImportStats
{
    double importTimeSeconds = 0.0;
    std::string dxfVersion;
    std::string dxfEncoding;
    std::string scalingSource;
    std::string fileUnits;
    double finalScalingFactor = 1.0;
    std::map<std::string, int> entityCounts;
    std::map<std::string, std::string> importSettings;
    std::map<std::string, std::vector<std::pair<int, std::string>>> unsupportedFeatures;
    std::map<std::string, int> systemBlockCounts;

    int totalEntitiesCreated = 0;
};


// "using" for enums is not supported by all platforms
// https://stackoverflow.com/questions/41167119/how-to-fix-a-wsubobject-linkage-warning
enum eDXFGroupCode_t
{
    eObjectType = 0,
    ePrimaryText = 1,
    eName = 2,
    eExtraText = 3,
    eHandle = 5,
    eLinetypeName = 6,
    eTextStyleName = 7,
    eLayerName = 8,
    eVariableName = 9,
    ePrimaryPoint = 10,
    ePoint2 = 11,
    ePoint3 = 12,
    ePoint4 = 13,
    ePoint5 = 14,
    eFloat1 = 40,
    eFloat2 = 41,
    eFloat3 = 42,
    eFloat4 = 43,
    eAngleDegrees1 = 50,
    eAngleDegrees2 = 51,
    eColor = 62,
    eCoordinateSpace = 67,
    eInteger1 = 70,
    eInteger2 = 71,
    eInteger3 = 72,
    eInteger4 = 73,
    eInteger5 = 74,
    eUCSOrigin = 110,
    eUCSXDirection = 111,
    eUCSYDirection = 112,
    eExtrusionDirection = 210,
    eComment = 999,

    // The following apply to points and directions in text DXF files to identify the three
    // coordinates
    eXOffset = 0,
    eYOffset = 10,
    eZOffset = 20
};
enum eDXFVersion_t
{
    RUnknown,
    ROlder,
    R10,
    R11_12,
    R13,
    R14,
    R2000,
    R2004,
    R2007,
    R2010,
    R2013,
    R2018,
    RNewer,
};
enum eDimensionType_t
{
    eLinear = 0,  // Rotated, Horizontal, or Vertical
    eAligned = 1,
    eAngular = 2,
    eDiameter = 3,
    eRadius = 4,
    eAngular3Point = 5,
    eOrdinate = 6,
    eTypeMask = 0xF,
    eOnlyBlockReference = 32,
    eOrdianetIsXType = 64,
    eUserTextLocation = 128
};
//********************

class ImportExport CDxfWrite
{
private:
    std::ofstream* m_ofs;
    bool m_fail;
    std::ostringstream* m_ssBlock;
    std::ostringstream* m_ssBlkRecord;
    std::ostringstream* m_ssEntity;
    std::ostringstream* m_ssLayer;

protected:
    static Base::Vector3d toVector3d(const double* coordinatesXYZ)
    {
        // NOLINTNEXTLINE(readability/nolint)
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return Base::Vector3d(coordinatesXYZ[0], coordinatesXYZ[1], coordinatesXYZ[2]);
    }

    void putLine(
        const Base::Vector3d& start,
        const Base::Vector3d& end,
        std::ostringstream* outStream,
        const std::string& handle,
        const std::string& ownerHandle
    );
    void putText(
        const char* text,
        const Base::Vector3d& location1,
        const Base::Vector3d& location2,
        double height,
        int horizJust,
        std::ostringstream* outStream,
        const std::string& handle,
        const std::string& ownerHandle
    );
    void putArrow(
        Base::Vector3d& arrowPos,
        Base::Vector3d& barb1Pos,
        Base::Vector3d& barb2Pos,
        std::ostringstream* outStream,
        const std::string& handle,
        const std::string& ownerHandle
    );

    //! copy boiler plate file
    std::string getPlateFile(std::string fileSpec);
    void setDataDir(const std::string& dirName)
    {
        m_dataDir = dirName;
    }
    std::string getHandle();
    std::string getEntityHandle();
    std::string getLayerHandle();
    std::string getBlockHandle();
    std::string getBlkRecordHandle();
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    std::string m_optionSource;
    int m_version;
    int m_handle;
    int m_entityHandle;
    int m_layerHandle;
    int m_blockHandle;
    int m_blkRecordHandle;
    bool m_polyOverride;

    std::string m_saveModelSpaceHandle;
    std::string m_savePaperSpaceHandle;
    std::string m_saveBlockRecordTableHandle;
    std::string m_saveBlkRecordHandle;
    std::string m_currentBlock;
    std::string m_dataDir;
    std::string m_layerName;
    std::vector<std::string> m_layerList;
    std::vector<std::string> m_blockList;
    std::vector<std::string> m_blkRecordList;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

public:
    explicit CDxfWrite(const char* filepath);
    CDxfWrite(const CDxfWrite&) = delete;
    CDxfWrite(const CDxfWrite&&) = delete;
    CDxfWrite& operator=(const CDxfWrite&) = delete;
    CDxfWrite& operator=(const CDxfWrite&&) = delete;
    ~CDxfWrite();

    void init();
    void endRun();

    bool Failed() const
    {
        return m_fail;
    }
    //    void setOptions(void);
    //    bool isVersionValid(int vers);
    std::string getLayerName()
    {
        return m_layerName;
    }
    void setLayerName(std::string name);
    void setVersion(int version)
    {
        m_version = version;
    }
    void setPolyOverride(bool setting)
    {
        m_polyOverride = setting;
    }
    void addBlockName(const std::string& name, const std::string& blkRecordHandle);

    void writeLine(const double* start, const double* end);
    void writePoint(const double*);
    void writeArc(const double* start, const double* end, const double* center, bool dir);
    void writeEllipse(
        const double* center,
        double major_radius,
        double minor_radius,
        double rotation,
        double start_angle,
        double end_angle,
        bool endIsCW
    );
    void writeCircle(const double* center, double radius);
    void writeSpline(const SplineDataOut& sd);
    void writeLWPolyLine(const LWPolyDataOut& pd);
    void writePolyline(const LWPolyDataOut& pd);
    // NOLINTNEXTLINE(readability/nolint)
    // NOLINTNEXTLINE(readability-identifier-length)
    void writeVertex(double x, double y, double z);
    void writeText(
        const char* text,
        const double* location1,
        const double* location2,
        double height,
        int horizJust
    );
    void writeLinearDim(
        const double* textMidPoint,
        const double* lineDefPoint,
        const double* extLine1,
        const double* extLine2,
        const char* dimText,
        int type
    );
    void writeLinearDimBlock(
        const double* textMidPoint,
        const double* lineDefPoint,
        const double* extLine1,
        const double* extLine2,
        const char* dimText,
        int type
    );
    void writeAngularDim(
        const double* textMidPoint,
        const double* lineDefPoint,
        const double* startExt1,
        const double* endExt1,
        const double* startExt2,
        const double* endExt2,
        const char* dimText
    );
    void writeAngularDimBlock(
        const double* textMidPoint,
        const double* lineDefPoint,
        const double* startExt1,
        const double* endExt1,
        const double* startExt2,
        const double* endExt2,
        const char* dimText
    );
    void writeRadialDim(
        const double* centerPoint,
        const double* textMidPoint,
        const double* arcPoint,
        const char* dimText
    );
    void writeRadialDimBlock(
        const double* centerPoint,
        const double* textMidPoint,
        const double* arcPoint,
        const char* dimText
    );
    void writeDiametricDim(
        const double* textMidPoint,
        const double* arcPoint1,
        const double* arcPoint2,
        const char* dimText
    );
    void writeDiametricDimBlock(
        const double* textMidPoint,
        const double* arcPoint1,
        const double* arcPoint2,
        const char* dimText
    );

    void writeDimBlockPreamble();
    void writeBlockTrailer();

    void writeHeaderSection();
    void writeTablesSection();
    void writeBlocksSection();
    void writeEntitiesSection();
    void writeObjectsSection();
    void writeClassesSection();

    void makeLayerTable();
    void makeBlockRecordTableHead();
    void makeBlockRecordTableBody();
    void makeBlockSectionHead();
};

// derive a class from this and implement it's virtual functions
class ImportExport CDxfRead
{
private:
    // Low-level reader members
    std::ifstream* m_ifs;  // TODO: gsl::owner<ifstream>
    // https://stackoverflow.com/questions/41167119/how-to-fix-a-wsubobject-linkage-warning
    eDXFGroupCode_t m_record_type = eObjectType;
    std::string m_record_data;
    bool m_not_eof = true;
    int m_line = 0;
    bool m_repeat_last_record = false;
    int m_current_entity_line_number = 0;
    std::string m_current_entity_name;
    std::string m_current_entity_handle;

    // The scaling from DXF units to millimetres.
    // This does not include the dxfScaling option
    // This has the value 0.0 if no units have been specified.
    // If it is still 0 after reading the HEADER section, it iw set to comething sensible.
    double m_unitScalingFactor = 0.0;

protected:
    DxfImportStats m_stats;
    // An additional scaling factor which can be modified before readDXF is called, and will be
    // incorporated into m_unitScalingFactor.
    void SetAdditionalScaling(double scaling)
    {
        m_additionalScaling = scaling <= 0.0 ? 1.0 : scaling;
    }

private:
    // The following are options which can be altered before DoRead is called.

    // An additional scaling factor to apply after any scaling implied by the DXF file header
    // (dxfScaling)
    double m_additionalScaling = 1.0;

    // This group of options control which attributes are preserved on import

protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    // Place entities in drawing layers as coded in the DXF file.
    // Otherwise place objects as top-level drawing objects (but still honour BYLAYER attributes).
    // (dxfUseDraftVisGroups)
    bool m_preserveLayers = true;

    // Colour entities as coded in the DXF file.
    // Otherwise leave everything as the default FreeCAD colours.
    // (dxfGetOriginalColors)
    bool m_preserveColors = true;
    // Control object simplification (preserve types and individual identities)
    using eEntityMergeType_t = enum
    {
        // Merge shapes (lines, arcs, circles, etc) into a single compound object when attributes
        // match
        MergeShapes,
        // Make individual dearing shapes for each DXF entity
        SingleShapes,
        // Make appropriately-types Draft objects for each DXF entity, giving superior property
        // access.
        DraftObjects
    };
    // This is a combination of ui options:
    //    "Group layers into blocks" (groupLayers) checkbox which means MergeShapes,
    //    "Simple part shapes" (dxfCreatePart) radio button which means SingleShapes
    //    "Draft objects" (dxfCreateDraft) radio button which means DraftObjects
    //    We do not support (dxfCreateSketch).
    //    (joingeometry) is described as being slow so I think the implication is that it only joins
    // linear entities that meet end-to-end to form them into wire objects. As such it is
    // intermediate between MergeShapes and SingleShapes and is not implemented.
    // TODO: The options structure allows conflicting settings for these options because
    // (groupLayers) and (joingeometry) are separate options, not part of the radio button set. The
    // others are excluded from having several set by the UI structure but code can set multiple
    // conflicting values.
    eEntityMergeType_t m_mergeOption = DraftObjects;

    // This group of options controls the import of certain types of entities or entities having
    // certain properties

    // Import text and dimensions (dxftext)
    bool m_importAnnotations = true;
    // Import Point tntities
    bool m_importPoints = true;
    // Import entities in Paper space
    bool m_importPaperSpaceEntities = false;
    // Import hidden blocks (the will not affect the result for standard import because such blocks
    // are never INSERTed but a custom derived class might want to access these block definitions.
    bool m_importHiddenBlocks = false;
    // Import content on Frozen layers
    bool m_importFrozenLayers = false;
    // Import content on Hidden layers. Note that an INSERT on a hidden layer would still be
    // expanded, but the resulting entities would not appear if placed on a hidden layer.
    bool m_importHiddenLayers = true;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    // TODO: options still to implement:
    // Import Hatch boundaries as wires (FC doesn't support hatching (???) but this option will draw
    // a polygon of the boundary of
    //     the hatched area
    // Render Polylines with width. If the width were constant we could use the View line width
    // property except that is in
    //     pixels and does not scale. The legacy importer replaces the polyline with a face whose
    //     side edges are the original polyline offset by half the width. If the polyline is open
    //     these edges are joined by endcaps, otherwise they each join to their ends to form a face
    //     with a hole in it.

    // TODO: Other options we might want:
    // Import SOLIDs (as faces)

    struct VertexInfo
    {
        Base::Vector3d location;
        double bulge = 0;
    };
    bool ExplodePolyline(std::list<VertexInfo>&, int flags);
    bool ReadBlockContents();
    bool SkipBlockContents();

private:
    // Error-handling control
    bool m_ignore_errors = true;
    bool m_fail = false;

    // These could be private to CommonEntityAttributes but in the long run I want to make all
    // the DXF definitions available to the CDxfWrite as well.
    // The Default values are used if a Layer definition by error is missing an attribute,
    // and also if we synthesize a layer that has no definition.
    static const ColorIndex_t ColorBylayer = 256;
    static const ColorIndex_t ColorByBlock = 0;
    static const ColorIndex_t DefaultColor = 0;
    static const std::string LineTypeByLayer;
    static const std::string LineTypeByBlock;
    static const std::string DefaultLineType;

    // Readers for various parts of the DXF file.
    bool ReadSection();
    // Section readers (sections are identified by the type-2 (name) record they start with and each
    // has its own reader function
    bool ReadHeaderSection();
    bool ReadTablesSection();
    bool ReadBlocksSection();

protected:
    virtual bool ReadEntitiesSection();

private:
    bool ReadIgnoredSection();

    // The Header section consists of multipel variables, only a few of which we give special
    // handling.
    bool ReadVariable();
    bool ReadVersion();
    bool ReadDWGCodePage();

    // The Tables section consists of several tables (again identified by their type-2 record asfter
    // th 0-TABLE record) each with its own reader
    bool ReadLayerTable();

    // Some tables we ignore completely, using this method, which some of the above are
    // inline-defined to.
    bool ReadIgnoredTable();

    bool ReadLayer();

    bool ReadEntity();  // Identify entity type and read it
    // Readers for specific entity types
    bool ReadLine();
    bool ReadText();
    bool ReadArc();
    bool ReadCircle();
    bool ReadEllipse();
    bool ReadPoint();
    bool ReadSpline();
    bool ReadLwPolyLine();
    bool ReadPolyLine();

    void OnReadArc(
        double start_angle,
        double end_angle,
        double radius,
        const Base::Vector3d& center,
        double z_extrusion_dir,
        bool hidden
    );
    void OnReadCircle(const Base::Vector3d& center, double radius, bool hidden);
    void OnReadEllipse(
        const Base::Vector3d& center,
        const Base::Vector3d& majorAxisEnd,
        double ratio,
        double start_angle,
        double end_angle
    );
    bool ReadInsert();
    bool ReadDimension();
    bool ReadUnknownEntity();

    // Helper for reading common attributes for entities
    void InitializeAttributes();
    void InitializeCommonEntityAttributes();
    void Setup3DVectorAttribute(eDXFGroupCode_t x_record_type, Base::Vector3d& destination);
    void Setup3DDirectionAttribute(eDXFGroupCode_t x_record_type, Base::Vector3d& destination);
    void SetupScaledDoubleAttribute(eDXFGroupCode_t record_type, double& destination);
    void SetupScaledDoubleIntoList(eDXFGroupCode_t record_type, std::list<double>& destination);
    void Setup3DCoordinatesIntoLists(
        eDXFGroupCode_t x_record_type,
        std::list<double>& x_destination,
        std::list<double>& y_destination,
        std::list<double>& z_destination
    );
    void SetupStringAttribute(eDXFGroupCode_t record_type, std::string& destination);
    std::map<int, std::pair<void (*)(CDxfRead*, void*), void*>> m_coordinate_attributes;
    static void ProcessScaledDouble(CDxfRead* object, void* target);
    static void ProcessScaledDoubleIntoList(CDxfRead* object, void* target);
    static void ProcessStdString(CDxfRead* object, void* target);
    static void ProcessLayerReference(CDxfRead* object, void* target);
    // For all types T where strean >> x and x = 0 works
    template<typename T>
    void SetupValueAttribute(eDXFGroupCode_t record_type, T& destination);
    // TODO: Once all compilers used for FreeCAD support class-level template specializations,
    // SetupValueAttribute could have specializations and replace SetupStringAttribute etc.
    // The template specialization is required to handle the (char *) case, which would
    // otherwise try to read the actual pointer from the stream, or... what?
    // The specialization would also handle the default value when it cannot be zero.
    template<typename T>
    static void ProcessValue(CDxfRead* object, void* target)
    {
        ParseValue<T>(object, target);
    }
    template<typename T>
    static bool ParseValue(CDxfRead* object, void* target);

    bool ProcessAttribute();
    void ProcessAllAttributes();
    void ProcessAllEntityAttributes();
    void ResolveEntityAttributes();

    bool ReadBlockInfo();
    bool ResolveEncoding();

    bool get_next_record();
    void repeat_last_record();

    bool (CDxfRead::*stringToUTF8)(std::string&) const = &CDxfRead::UTF8ToUTF8;

protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    eDXFVersion_t m_version = RUnknown;  // Version from $ACADVER variable in DXF
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    // Although this is called "ImportError" it is just a wrapper to write a warning eithout any
    // additional information such as a line number and as such, may be split into a basic
    // message-writer and something that adds a line number.
    //
    // The "Developer" methods stick a line break into the output window.
    // The "User" methods show up in the notification popup window.
    // "Critical" causes a popup messagebox
    // "Warnings" show up in yellow in the output window and with a warning icon in the notification
    // popup "Error" show up in red in the output window and with an error icon in the notification
    // popup "Notification" show up in black in the output window and an information icon in the
    // notification popup "Log" goes to a log somewhere and not to the screen/user at all

    template<typename... args>
    static void ImportError(const char* format, args&&... argValues)
    {
        Base::ConsoleSingleton::instance().warning(format, std::forward<args>(argValues)...);
    }
    template<typename... args>
    static void ImportObservation(const char* format, args&&... argValues)
    {
        Base::ConsoleSingleton::instance().message(format, std::forward<args>(argValues)...);
    }
    template<typename... args>
    void UnsupportedFeature(const char* format, args&&... argValues);

private:
    std::string m_CodePage;  // Code Page name from $DWGCODEPAGE or null if none/not read yet
    // The following was going to be python's canonical name for the encoding, but this is (a) not
    // easily found and (b) does not speed up finding the encoding object.
    std::string m_encoding;  // A name for the encoding implied by m_version and m_CodePage
    bool UTF8ToUTF8(std::string& encoded) const;
    bool GeneralToUTF8(std::string& encoded) const;

    // Compare with specific object name for eObjectType records
    bool IsObjectName(const char* testName) const
    {
        return m_record_data == testName;
    }
    // Compare with specific variable name for eVariableName records
    bool IsVariableName(const char* testName) const
    {
        return m_record_data == testName;
    }

    // Layer management
protected:
    class Layer
    {
    public:
        Layer(const std::string& name, ColorIndex_t color, std::string&& lineType)
            : Name(name)
            , Color(color < 0 ? -color : color)
            , LineType(lineType)
            , Hidden(color < 0)
        {}
        Layer(const Layer&) = delete;
        Layer(Layer&&) = delete;
        void operator=(const Layer&) = delete;
        void operator=(Layer&&) = delete;
        virtual ~Layer() = default;
        const std::string Name;
        const ColorIndex_t Color;
        const std::string LineType;
        const bool Hidden;
    };
    std::map<std::string, Layer*> Layers;

    virtual Layer* MakeLayer(const std::string& name, ColorIndex_t color, std::string&& lineType);

    // Entity attribute management
    class CommonEntityAttributes
    {
    public:
        Layer* m_Layer {nullptr};
        ColorIndex_t m_Color {0};
        std::string m_LineType;
        bool m_paperSpace {false};
        void SetDefaults()
        {
            // There is some uncertainty in the documentation. If the line type is omitted it should
            // act as by-layer.
            // The question is whether an explicit line type "BYLAYER" should also mean by-layer. If
            // this is the case we should set m_LineType to "BYLAYER" and only check for "BYLAYER"
            // in ResolveByLayerAttributes. If this is not the case and "BYLAYER" can actually refer
            // to a line type of that name, then we should init m_LineType to empty and check only
            // for a zero-length line type to trigger by-layer line type. An experiment in R13
            // reveals that it is not possible to make a line type BYLAYER so the former is what we
            // do.
            m_Color = ColorBylayer;
            m_LineType = LineTypeByLayer;
            m_paperSpace = false;
        }
        void ResolveBylayerAttributes(const CDxfRead& reader)
        {
            // This should be called after the entire entity is read in case the layer reference
            // name comes after a BYLAYER color. Also we can't be sure of BYLAYER line type until
            // the entity ends
            if (m_Color == ColorBylayer) {
                m_Color = m_Layer != nullptr ? m_Layer->Color : DefaultColor;
            }
            if (m_LineType == LineTypeByLayer) {
                m_LineType = m_Layer != nullptr ? m_Layer->LineType : DefaultLineType;
            }
            // We this point we not longer have any use for m_Layer except as a destination for the
            // entity. If the import is not preserving layers, we clear out that field here so all
            // entities get placed at the drawing root and are subject to compounding despite being
            // originally in different layers.
            if (!reader.m_preserveColors) {
                m_Color = DefaultColor;
            }
            if (!reader.m_preserveLayers) {
                m_Layer = nullptr;
            }
        }
        void ResolveByBlockAttributes(const CommonEntityAttributes& insertAttributes)
        {
            if (m_Color == ColorByBlock) {
                m_Color = insertAttributes.m_Color;
            }
            if (m_LineType == LineTypeByBlock) {
                m_LineType = insertAttributes.m_LineType;
            }
        }
        bool operator<(const CommonEntityAttributes& second) const
        {
            // TODO: This is here so we can use CommonEntityAttributes as a key in a std::map
            // It's been a while and I don't know if it is (currently) considered suspect to compare
            // for inequality two pointers that don't point within the same array, but I'm also not
            // sure what to do instead.
            return m_Layer < second.m_Layer
                || (m_Layer == second.m_Layer
                    && (m_Color < second.m_Color
                        || (m_Color == second.m_Color
                            && (m_LineType < second.m_LineType
                                || (m_LineType == second.m_LineType && !m_paperSpace
                                    && second.m_paperSpace)))));
        }
    };
    CommonEntityAttributes m_entityAttributes;
    // Coordinate transform management.
private:
    // The normal vector as read from the eExtrusionDirection attributes, defaults to (0, 0, 1)
    // Reading this is set up in ReadEntity and done by ProcessAll(Entity)Attributes
    Base::Vector3d EntityNormalVector;

    static constexpr double ArbitraryAxisAlgorithmThreshold = 1 / 64.0;

protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    // The transform implied by the Normal vector of the entity in EntityNormalVector.
    // This is a pure 3d rotation transformation (that is, no scaling or translation)
    // which maps the Z axis to the normal vector from the DXF records,
    // and the X and Y axes according to the Arbitrary Axis Algorithm in the DXF documentation
    // This is calculated from EntityNormalVector in ResolveEntityAttributes which is
    // called by either ProcessAllEntityAttributes or custom-processed entities after they
    // are finished calling ProcessAttribute.
    Base::Matrix4D OCSOrientationTransform;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

public:
    explicit CDxfRead(const std::string& filepath);  // this opens the file
    CDxfRead(const CDxfRead&) = delete;
    CDxfRead(const CDxfRead&&) = delete;
    CDxfRead& operator=(const CDxfRead&) = delete;
    CDxfRead& operator=(const CDxfRead&&) = delete;
    virtual ~CDxfRead();

    bool Failed() const
    {
        return m_fail;
    }
    void setImportTime(double seconds)
    {
        m_stats.importTimeSeconds = seconds;
    }
    void DoRead(bool ignore_errors = false);  // this reads the file and calls the following functions
    virtual void StartImport()
    {}
    virtual void FinishImport()
    {}

private:
    double mm(double value) const
    {
        if (m_unitScalingFactor == 0.0) {
            // No scaling factor has been specified.
            // TODO: Resolve this once we know the HEADER is complete
            return value;
        }
        return m_unitScalingFactor * value;
    }

public:
    bool IgnoreErrors() const
    {
        return (m_ignore_errors);
    }

    virtual bool OnReadBlock(const std::string& /*name*/, int /*flags*/)
    {
        return SkipBlockContents();
    }
    virtual void OnReadLine(const Base::Vector3d& /*start*/, const Base::Vector3d& /*end*/, bool /*hidden*/)
    {}
    virtual void OnReadPoint(const Base::Vector3d& /*start*/)
    {}
    virtual void OnReadText(
        const Base::Vector3d& /*point*/,
        const double /*height*/,
        const std::string& /*text*/,
        const double /*rotation*/
    )
    {}
    virtual void OnReadArc(
        const Base::Vector3d& /*start*/,
        const Base::Vector3d& /*end*/,
        const Base::Vector3d& /*center*/,
        bool /*dir*/,
        bool /*hidden*/
    )
    {}
    virtual void OnReadCircle(const Base::Vector3d& /*start*/, const Base::Vector3d& /*center*/, bool /*dir*/, bool /*hidden*/)
    {}
    virtual void OnReadEllipse(
        const Base::Vector3d& /*center*/,
        double /*major_radius*/,
        double /*minor_radius*/,
        double /*rotation*/,
        double /*start_angle*/,
        double /*end_angle*/,
        bool /*dir*/
    )
    {}
    virtual void OnReadSpline(struct SplineData& /*sd*/)
    {}
    virtual void OnReadInsert(
        const Base::Vector3d& /*point*/,
        const Base::Vector3d& /*scale*/,
        const std::string& /*name*/,
        double /*rotation*/
    )
    {}
    virtual void OnReadDimension(
        const Base::Vector3d& /*start*/,
        const Base::Vector3d& /*end*/,
        const Base::Vector3d& /*point*/,
        int /*dimensionType*/,
        double /*rotation*/
    )
    {}
    virtual void OnReadPolyline(std::list<VertexInfo>& /*vertices*/, int /*flags*/)
    {}

    // These give the derived class access to common object properties
    bool LineTypeIsHidden() const
    {
        return m_entityAttributes.m_LineType[0] == 'h' || m_entityAttributes.m_LineType[0] == 'H';
    }
    static Base::Color ObjectColor(ColorIndex_t colorIndex);  // as rgba value

#ifdef DEBUG
protected:
    static PyObject* PyObject_GetAttrString(PyObject* o, const char* attr_name)
    {
        PyObject* result = ::PyObject_GetAttrString(o, attr_name);
        if (result == nullptr) {
            ImportError("Unable to get Attribute '%s'\n", attr_name);
            PyErr_Clear();
        }
        return result;
    }
    static void PyObject_SetAttrString(PyObject* o, const char* attr_name, PyObject* v)
    {
        if (::PyObject_SetAttrString(o, attr_name, v) != 0) {
            ImportError("Unable to set Attribute '%s'\n", attr_name);
            PyErr_Clear();
        }
    }
#endif
};
