// dxf.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
// modified 2018 wandererfan

#ifndef _dxf_h_
#define _dxf_h_

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
    double norm[3];
    int degree;
    int knots;
    int control_points;
    int fit_points;
    int flag;
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

class CDxfWrite
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
    ImportExport CDxfWrite(const char* filepath);
    ImportExport ~CDxfWrite();

    ImportExport void init();
    ImportExport void endRun();

    ImportExport bool Failed()
    {
        return m_fail;
    }
    //    void setOptions(void);
    //    bool isVersionValid(int vers);
    ImportExport std::string getLayerName()
    {
        return m_layerName;
    }
    ImportExport void setLayerName(std::string s);
    ImportExport void setVersion(int v)
    {
        m_version = v;
    }
    ImportExport void setPolyOverride(bool b)
    {
        m_polyOverride = b;
    }
    ImportExport void addBlockName(std::string s, std::string blkRecordHandle);

    ImportExport void writeLine(const double* s, const double* e);
    ImportExport void writePoint(const double*);
    ImportExport void writeArc(const double* s, const double* e, const double* c, bool dir);
    ImportExport void writeEllipse(const double* c,
                                   double major_radius,
                                   double minor_radius,
                                   double rotation,
                                   double start_angle,
                                   double end_angle,
                                   bool endIsCW);
    ImportExport void writeCircle(const double* c, double radius);
    ImportExport void writeSpline(const SplineDataOut& sd);
    ImportExport void writeLWPolyLine(const LWPolyDataOut& pd);
    ImportExport void writePolyline(const LWPolyDataOut& pd);
    ImportExport void writeVertex(double x, double y, double z);
    ImportExport void writeText(const char* text,
                                const double* location1,
                                const double* location2,
                                const double height,
                                const int horizJust);
    ImportExport void writeLinearDim(const double* textMidPoint,
                                     const double* lineDefPoint,
                                     const double* extLine1,
                                     const double* extLine2,
                                     const char* dimText,
                                     int type);
    ImportExport void writeLinearDimBlock(const double* textMidPoint,
                                          const double* lineDefPoint,
                                          const double* extLine1,
                                          const double* extLine2,
                                          const char* dimText,
                                          int type);
    ImportExport void writeAngularDim(const double* textMidPoint,
                                      const double* lineDefPoint,
                                      const double* startExt1,
                                      const double* endExt1,
                                      const double* startExt2,
                                      const double* endExt2,
                                      const char* dimText);
    ImportExport void writeAngularDimBlock(const double* textMidPoint,
                                           const double* lineDefPoint,
                                           const double* startExt1,
                                           const double* endExt1,
                                           const double* startExt2,
                                           const double* endExt2,
                                           const char* dimText);
    ImportExport void writeRadialDim(const double* centerPoint,
                                     const double* textMidPoint,
                                     const double* arcPoint,
                                     const char* dimText);
    ImportExport void writeRadialDimBlock(const double* centerPoint,
                                          const double* textMidPoint,
                                          const double* arcPoint,
                                          const char* dimText);
    ImportExport void writeDiametricDim(const double* textMidPoint,
                                        const double* arcPoint1,
                                        const double* arcPoint2,
                                        const char* dimText);
    ImportExport void writeDiametricDimBlock(const double* textMidPoint,
                                             const double* arcPoint1,
                                             const double* arcPoint2,
                                             const char* dimText);

    ImportExport void writeDimBlockPreamble();
    ImportExport void writeBlockTrailer();

    ImportExport void writeHeaderSection();
    ImportExport void writeTablesSection();
    ImportExport void writeBlocksSection();
    ImportExport void writeEntitiesSection();
    ImportExport void writeObjectsSection();
    ImportExport void writeClassesSection();

    ImportExport void makeLayerTable();
    ImportExport void makeBlockRecordTableHead();
    ImportExport void makeBlockRecordTableBody();
    ImportExport void makeBlockSectionHead();
};

// derive a class from this and implement it's virtual functions
class CDxfRead
{
private:
    std::ifstream* m_ifs;

    bool m_fail;
    char m_str[1024];
    char m_unused_line[1024];
    eDxfUnits_t m_eUnits;
    bool m_measurement_inch;
    char m_layer_name[1024];
    char m_section_name[1024];
    char m_block_name[1024];
    bool m_ignore_errors;


    std::map<std::string, ColorIndex_t>
        m_layer_ColorIndex_map;  // Mapping from layer name -> layer color index
    const ColorIndex_t ColorBylayer = 256;
    const ColorIndex_t ColorByBlock = 0;

    bool ReadUnits();
    bool ReadLayer();
    bool ReadLine();
    bool ReadText();
    bool ReadArc();
    bool ReadCircle();
    bool ReadEllipse();
    bool ReadPoint();
    bool ReadSpline();
    bool ReadLwPolyLine();
    bool ReadPolyLine();
    bool ReadVertex(double* pVertex, bool* bulge_found, double* bulge);
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
    bool ReadBlockInfo();
    bool ReadVersion();
    bool ReadDWGCodePage();
    bool ResolveEncoding();

    void get_line();
    void put_line(const char* value);
    void ResolveColorIndex();

protected:
    ColorIndex_t m_ColorIndex;
    eDXFVersion_t m_version;  // Version from $ACADVER variable in DXF
    const char* (CDxfRead::*stringToUTF8)(const char*) const;

private:
    const std::string* m_CodePage;  // Code Page name from $DWGCODEPAGE or null if none/not read yet
    // The following was going to be python's canonical name for the encoding, but this is (a) not
    // easily found and (b) does not speed up finding the encoding object.
    const std::string* m_encoding;  // A name for the encoding implied by m_version and m_CodePage
    const char* UTF8ToUTF8(const char* encoded) const;
    const char* GeneralToUTF8(const char* encoded) const;

public:
    ImportExport CDxfRead(const char* filepath);  // this opens the file
    ImportExport virtual ~CDxfRead();             // this closes the file

    ImportExport bool Failed()
    {
        return m_fail;
    }
    ImportExport void DoRead(
        const bool ignore_errors = false);  // this reads the file and calls the following functions

    ImportExport double mm(double value) const;

    ImportExport bool IgnoreErrors() const
    {
        return (m_ignore_errors);
    }

    ImportExport virtual void OnReadLine(const double* /*s*/, const double* /*e*/, bool /*hidden*/)
    {}
    ImportExport virtual void OnReadPoint(const double* /*s*/)
    {}
    ImportExport virtual void
    OnReadText(const double* /*point*/, const double /*height*/, const char* /*text*/)
    {}
    ImportExport virtual void OnReadArc(const double* /*s*/,
                                        const double* /*e*/,
                                        const double* /*c*/,
                                        bool /*dir*/,
                                        bool /*hidden*/)
    {}
    ImportExport virtual void
    OnReadCircle(const double* /*s*/, const double* /*c*/, bool /*dir*/, bool /*hidden*/)
    {}
    ImportExport virtual void OnReadEllipse(const double* /*c*/,
                                            double /*major_radius*/,
                                            double /*minor_radius*/,
                                            double /*rotation*/,
                                            double /*start_angle*/,
                                            double /*end_angle*/,
                                            bool /*dir*/)
    {}
    ImportExport virtual void OnReadSpline(struct SplineData& /*sd*/)
    {}
    ImportExport virtual void OnReadInsert(const double* /*point*/,
                                           const double* /*scale*/,
                                           const char* /*name*/,
                                           double /*rotation*/)
    {}
    ImportExport virtual void OnReadDimension(const double* /*s*/,
                                              const double* /*e*/,
                                              const double* /*point*/,
                                              double /*rotation*/)
    {}
    ImportExport virtual void AddGraphics() const
    {}

    ImportExport std::string LayerName() const;
};
#endif
