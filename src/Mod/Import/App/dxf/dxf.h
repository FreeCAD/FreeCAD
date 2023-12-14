// dxf.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
// modified 2018 wandererfan

#ifndef _dxf_h_
#define _dxf_h_

#ifdef _MSC_VER
#pragma warning(disable : 4251)
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

#include <Base/Vector3D.h>
#include <Base/Console.h>
#include <App/Color.h>
#include <Mod/Import/ImportGlobal.h>


typedef int ColorIndex_t;  // DXF color index

typedef enum
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
    eParsecs
} eDxfUnits_t;


// spline data for reading
struct SplineData
{
    double norm[3] = {0, 0, 0};
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
    double nVert;
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
typedef enum
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
} eDXFVersion_t;
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
    void putLine(const Base::Vector3d s,
                 const Base::Vector3d e,
                 std::ostringstream* outStream,
                 const std::string handle,
                 const std::string ownerHandle);
    void putText(const char* text,
                 const Base::Vector3d location1,
                 const Base::Vector3d location2,
                 const double height,
                 const int horizJust,
                 std::ostringstream* outStream,
                 const std::string handle,
                 const std::string ownerHandle);
    void putArrow(Base::Vector3d arrowPos,
                  Base::Vector3d barb1Pos,
                  Base::Vector3d barb2Pos,
                  std::ostringstream* outStream,
                  const std::string handle,
                  const std::string ownerHandle);

    //! copy boiler plate file
    std::string getPlateFile(std::string fileSpec);
    void setDataDir(std::string s)
    {
        m_dataDir = s;
    }
    std::string getHandle();
    std::string getEntityHandle();
    std::string getLayerHandle();
    std::string getBlockHandle();
    std::string getBlkRecordHandle();

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

public:
    explicit CDxfWrite(const char* filepath);
    ~CDxfWrite();

    void init();
    void endRun();

    bool Failed()
    {
        return m_fail;
    }
    //    void setOptions(void);
    //    bool isVersionValid(int vers);
    std::string getLayerName()
    {
        return m_layerName;
    }
    void setLayerName(std::string s);
    void setVersion(int v)
    {
        m_version = v;
    }
    void setPolyOverride(bool b)
    {
        m_polyOverride = b;
    }
    void addBlockName(std::string s, std::string blkRecordHandle);

    void writeLine(const double* s, const double* e);
    void writePoint(const double*);
    void writeArc(const double* s, const double* e, const double* c, bool dir);
    void writeEllipse(const double* c,
                      double major_radius,
                      double minor_radius,
                      double rotation,
                      double start_angle,
                      double end_angle,
                      bool endIsCW);
    void writeCircle(const double* c, double radius);
    void writeSpline(const SplineDataOut& sd);
    void writeLWPolyLine(const LWPolyDataOut& pd);
    void writePolyline(const LWPolyDataOut& pd);
    void writeVertex(double x, double y, double z);
    void writeText(const char* text,
                   const double* location1,
                   const double* location2,
                   const double height,
                   const int horizJust);
    void writeLinearDim(const double* textMidPoint,
                        const double* lineDefPoint,
                        const double* extLine1,
                        const double* extLine2,
                        const char* dimText,
                        int type);
    void writeLinearDimBlock(const double* textMidPoint,
                             const double* lineDefPoint,
                             const double* extLine1,
                             const double* extLine2,
                             const char* dimText,
                             int type);
    void writeAngularDim(const double* textMidPoint,
                         const double* lineDefPoint,
                         const double* startExt1,
                         const double* endExt1,
                         const double* startExt2,
                         const double* endExt2,
                         const char* dimText);
    void writeAngularDimBlock(const double* textMidPoint,
                              const double* lineDefPoint,
                              const double* startExt1,
                              const double* endExt1,
                              const double* startExt2,
                              const double* endExt2,
                              const char* dimText);
    void writeRadialDim(const double* centerPoint,
                        const double* textMidPoint,
                        const double* arcPoint,
                        const char* dimText);
    void writeRadialDimBlock(const double* centerPoint,
                             const double* textMidPoint,
                             const double* arcPoint,
                             const char* dimText);
    void writeDiametricDim(const double* textMidPoint,
                           const double* arcPoint1,
                           const double* arcPoint2,
                           const char* dimText);
    void writeDiametricDimBlock(const double* textMidPoint,
                                const double* arcPoint1,
                                const double* arcPoint2,
                                const char* dimText);

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
    std::ifstream* m_ifs;
    int m_record_type = 0;
    char m_record_data[1024] = "";
    bool m_not_eof = true;
    int m_line = 0;
    bool m_repeat_last_record = false;

    // file-level options/properties
    eDxfUnits_t m_eUnits = eMillimeters;
    bool m_measurement_inch = false;

    // The following provide a state when reading any entity: If m_block_name is not empty the
    // entity is in a BLOCK being defined, and LayerName() will return "BLOCKS xxxx" where xxxx is
    // the block name. Otherwise m_layer_name will be the layer name from the entity records
    // (default to "0") and LayerName() will return "ENTITIES xxxx" where xxxx is the layer name.
    // This is clunky but it is a non-private interface and so difficult to change.
    char m_layer_name[1024] = "0";
    char m_block_name[1024] = "";


    // Error-handling control
    bool m_ignore_errors = true;
    bool m_fail = false;

    std::map<std::string, ColorIndex_t>
        m_layer_ColorIndex_map;  // Mapping from layer name -> layer color index
    const ColorIndex_t ColorBylayer = 256;
    const ColorIndex_t ColorByBlock = 0;

    // Readers for various parts of the DXF file.
    // Section readers (sections are identified by the type-2 (name) record they start with and each
    // has its own reader function
    bool ReadHeaderSection();
    bool ReadTablesSection();
    bool ReadBlocksSection();
    bool ReadEntitiesSection();

    bool ReadIgnoredSection();

    // The Tables section consists of several tables (again identified by their type-2 record asfter
    // th 0-TABLE record) each with its own reader
    bool ReadLayerTable();

    // Some tables we ignore completely, using this method, which some of the above are
    // inline-defined to.
    bool ReadIgnoredTable();

    bool ReadUnits();
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
    typedef struct
    {
        double location[3];
        double bulge;
    } VertexInfo;

    bool OnReadPolyline(std::list<VertexInfo>&, int flags);
    void OnReadArc(double start_angle,
                   double end_angle,
                   double radius,
                   const double* c,
                   double z_extrusion_dir,
                   bool hidden);
    void OnReadCircle(const double* c, double radius, bool hidden);
    void OnReadEllipse(const double* c,
                       const double* m,
                       double ratio,
                       double start_angle,
                       double end_angle);
    bool ReadInsert();
    bool ReadDimension();
    bool ReadUnknownEntity();
    // Helper for reading common attributes for entities
    void InitializeAttributes();
    void InitializeCommonEntityAttributes();
    void Setup3DVectorAttribute(int x_record_type, double destination[3]);
    void SetupScaledDoubleAttribute(int record_type, double& destination);
    void SetupScaledDoubleIntoList(int record_type, std::list<double>& destination);
    void Setup3DCoordinatesIntoLists(int x_record_type,
                                     std::list<double>& x_destination,
                                     std::list<double>& y_destination,
                                     std::list<double>& z_destination);
    void SetupStringAttribute(int record_type, char* destination);
    void SetupStringAttribute(int record_type, std::string& destination);
    std::map<int, std::pair<void (*)(CDxfRead*, void*), void*>> m_coordinate_attributes;
    static void ProcessScaledDouble(CDxfRead* object, void* target);
    static void ProcessScaledDoubleIntoList(CDxfRead* object, void* target);
    static void ProcessString(CDxfRead* object, void* target);
    static void ProcessStdString(CDxfRead* object, void* target);
    // For all types T where strean >> x and x = 0 works
    template<typename T>
    void SetupValueAttribute(int record_type, T& target);
    // TODO: Once all compilers used for FreeCAD support class-level template specializations,
    // SetupValueAttribute could have specializations and replace SetupStringAttribute etc.
    // The template specialization is required to handle the (char *) case, which would
    // otherwise try to read the actual pointer from the stream, or... what?
    // The specialization would also handle the default value when it cannot be zero.
    template<typename T>
    static void ProcessValue(CDxfRead* object, void* target);

    bool ProcessAttribute();
    void ProcessAllAttributes();

    bool ReadBlockInfo();
    bool ReadVersion();
    bool ReadDWGCodePage();
    bool ResolveEncoding();

    bool get_next_record();
    void repeat_last_record();

protected:
    // common entity properties. Some properties are accumulated local to the reader method and
    // passed to the ReadXxxx virtual method. Others are collected here as private values and also
    // passed to ReadXxxx. Finally some of the attributes are accessed using references to
    // public/protected fields or methods (such as LayerName()). Altogether a bit of a mishmash.
    ColorIndex_t m_ColorIndex = 0;
    char m_LineType[1024] = "";
    eDXFVersion_t m_version = RUnknown;  // Version from $ACADVER variable in DXF
    const char* (CDxfRead::*stringToUTF8)(const char*) const = &CDxfRead::UTF8ToUTF8;
    // Although this is called "ImportWarning" it is just a wrapper to write a warning eithout any
    // additional information such as a line number and as such, may be split into a basic
    // message-writer and something that adds a line number.
    template<typename... args>
    void ImportError(const char* format, args... argValues) const
    {
        Base::ConsoleSingleton::Instance().Warning(format, argValues...);
    }
    void UnsupportedFeature(const char* format, ...);

private:
    std::map<std::string, std::pair<int, int>> m_unsupportedFeaturesNoted;
    const std::string* m_CodePage =
        nullptr;  // Code Page name from $DWGCODEPAGE or null if none/not read yet
    // The following was going to be python's canonical name for the encoding, but this is (a) not
    // easily found and (b) does not speed up finding the encoding object.
    const std::string* m_encoding =
        nullptr;  // A name for the encoding implied by m_version and m_CodePage
    const char* UTF8ToUTF8(const char* encoded) const;
    const char* GeneralToUTF8(const char* encoded) const;

public:
    explicit CDxfRead(const char* filepath);  // this opens the file
    virtual ~CDxfRead();                      // this closes the file

    bool Failed()
    {
        return m_fail;
    }
    void DoRead(
        const bool ignore_errors = false);  // this reads the file and calls the following functions

    double mm(double value) const;

    bool IgnoreErrors() const
    {
        return (m_ignore_errors);
    }

    virtual void OnReadLine(const double* /*s*/, const double* /*e*/, bool /*hidden*/)
    {}
    virtual void OnReadPoint(const double* /*s*/)
    {}
    virtual void OnReadText(const double* /*point*/,
                            const double /*height*/,
                            const char* /*text*/,
                            const double /*rotation*/)
    {}
    virtual void OnReadArc(const double* /*s*/,
                           const double* /*e*/,
                           const double* /*c*/,
                           bool /*dir*/,
                           bool /*hidden*/)
    {}
    virtual void
    OnReadCircle(const double* /*s*/, const double* /*c*/, bool /*dir*/, bool /*hidden*/)
    {}
    virtual void OnReadEllipse(const double* /*c*/,
                               double /*major_radius*/,
                               double /*minor_radius*/,
                               double /*rotation*/,
                               double /*start_angle*/,
                               double /*end_angle*/,
                               bool /*dir*/)
    {}
    virtual void OnReadSpline(struct SplineData& /*sd*/)
    {}
    virtual void OnReadInsert(const double* /*point*/,
                              const double* /*scale*/,
                              const char* /*name*/,
                              double /*rotation*/)
    {}
    virtual void OnReadDimension(const double* /*s*/,
                                 const double* /*e*/,
                                 const double* /*point*/,
                                 double /*rotation*/)
    {}
    virtual void AddGraphics() const
    {}

    // These give the derived class access to common object properties
    std::string LayerName() const;
    bool LineTypeIsHidden() const
    {
        return m_LineType[0] == 'h' || m_LineType[0] == 'H';
    }
    App::Color ObjectColor() const;  // as rgba value
};
#endif
