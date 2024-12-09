// dxf.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
// modified 2018 wandererfan

#include "PreCompiled.h"

// required by windows for M_PI definition
#define _USE_MATH_DEFINES
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "dxf.h"
#include <App/Application.h>
#include <App/Color.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>


using namespace std;
static Base::Vector3d MakeVector3d(const double coordinates[3])
{
    // NOLINTNEXTLINE(readability/nolint)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return Base::Vector3d(coordinates[0], coordinates[1], coordinates[2]);
}

CDxfWrite::CDxfWrite(const char* filepath)
    :  // TODO: these should probably be parameters in config file
       // handles:
       // boilerplate 0 - A00
       // used by dxf.cpp A01 - FFFE
       // ACAD HANDSEED  FFFF

    m_fail(false)
    , m_ssBlock(new std::ostringstream())
    , m_ssBlkRecord(new std::ostringstream())
    , m_ssEntity(new std::ostringstream())
    , m_ssLayer(new std::ostringstream())
    , m_version(12)
    , m_handle(0xA00)
    ,  // room for 2560 handles in boilerplate files
    // m_entityHandle(0x300),               //don't need special ranges for handles
    // m_layerHandle(0x30),
    // m_blockHandle(0x210),
    // m_blkRecordHandle(0x110),
    m_polyOverride(false)
    , m_layerName("none")
{
    // start the file
    Base::FileInfo fi(filepath);
    m_ofs = new Base::ofstream(fi, ios::out);

    if (!(*m_ofs)) {
        m_fail = true;
        return;
    }
    m_ofs->imbue(std::locale("C"));

    // use lots of digits to avoid rounding errors
    m_ssEntity->setf(std::ios::fixed);
    m_ssEntity->precision(9);
}

CDxfWrite::~CDxfWrite()
{
    delete m_ofs;
    delete m_ssBlock;
    delete m_ssBlkRecord;
    delete m_ssEntity;
    delete m_ssLayer;
}

void CDxfWrite::init()
{
    writeHeaderSection();
    makeBlockRecordTableHead();
    makeBlockSectionHead();
}

//! assemble pieces into output file
void CDxfWrite::endRun()
{
    makeLayerTable();
    makeBlockRecordTableBody();

    writeClassesSection();
    writeTablesSection();
    writeBlocksSection();
    writeEntitiesSection();
    writeObjectsSection();

    (*m_ofs) << "  0" << endl;
    (*m_ofs) << "EOF";
}

//***************************
// writeHeaderSection
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeHeaderSection()
{
    std::stringstream ss;
    ss << "FreeCAD v" << App::Application::Config()["BuildVersionMajor"] << "."
       << App::Application::Config()["BuildVersionMinor"] << " "
       << App::Application::Config()["BuildRevision"];

    // header & version
    (*m_ofs) << "999" << endl;
    (*m_ofs) << ss.str() << endl;

    // static header content
    ss.str("");
    ss.clear();
    ss << "header" << m_version << ".rub";
    std::string fileSpec = m_dataDir + ss.str();
    (*m_ofs) << getPlateFile(fileSpec);
}

//***************************
// writeClassesSection
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeClassesSection()
{
    if (m_version < 14) {
        return;
    }

    // static classes section content
    std::stringstream ss;
    ss << "classes" << m_version << ".rub";
    std::string fileSpec = m_dataDir + ss.str();
    (*m_ofs) << getPlateFile(fileSpec);
}

//***************************
// writeTablesSection
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeTablesSection()
{
    // static tables section head end content
    std::stringstream ss;
    ss << "tables1" << m_version << ".rub";
    std::string fileSpec = m_dataDir + ss.str();
    (*m_ofs) << getPlateFile(fileSpec);

    (*m_ofs) << (*m_ssLayer).str();

    // static tables section tail end content
    ss.str("");
    ss.clear();
    ss << "tables2" << m_version << ".rub";
    fileSpec = m_dataDir + ss.str();
    (*m_ofs) << getPlateFile(fileSpec);

    if (m_version > 12) {
        (*m_ofs) << (*m_ssBlkRecord).str();
        (*m_ofs) << "  0" << endl;
        (*m_ofs) << "ENDTAB" << endl;
    }
    (*m_ofs) << "  0" << endl;
    (*m_ofs) << "ENDSEC" << endl;
}

//***************************
// makeLayerTable
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::makeLayerTable()
{
    std::string tablehash = getLayerHandle();
    (*m_ssLayer) << "  0" << endl;
    (*m_ssLayer) << "TABLE" << endl;
    (*m_ssLayer) << "  2" << endl;
    (*m_ssLayer) << "LAYER" << endl;
    (*m_ssLayer) << "  5" << endl;
    (*m_ssLayer) << tablehash << endl;
    if (m_version > 12) {
        (*m_ssLayer) << "330" << endl;
        (*m_ssLayer) << 0 << endl;
        (*m_ssLayer) << "100" << endl;
        (*m_ssLayer) << "AcDbSymbolTable" << endl;
    }
    (*m_ssLayer) << " 70" << endl;
    (*m_ssLayer) << m_layerList.size() + 1 << endl;

    (*m_ssLayer) << "  0" << endl;
    (*m_ssLayer) << "LAYER" << endl;
    (*m_ssLayer) << "  5" << endl;
    (*m_ssLayer) << getLayerHandle() << endl;
    if (m_version > 12) {
        (*m_ssLayer) << "330" << endl;
        (*m_ssLayer) << tablehash << endl;
        (*m_ssLayer) << "100" << endl;
        (*m_ssLayer) << "AcDbSymbolTableRecord" << endl;
        (*m_ssLayer) << "100" << endl;
        (*m_ssLayer) << "AcDbLayerTableRecord" << endl;
    }
    (*m_ssLayer) << "  2" << endl;
    (*m_ssLayer) << "0" << endl;
    (*m_ssLayer) << " 70" << endl;
    (*m_ssLayer) << "   0" << endl;
    (*m_ssLayer) << " 62" << endl;
    (*m_ssLayer) << "   7" << endl;
    (*m_ssLayer) << "  6" << endl;
    (*m_ssLayer) << "CONTINUOUS" << endl;

    for (auto& l : m_layerList) {
        (*m_ssLayer) << "  0" << endl;
        (*m_ssLayer) << "LAYER" << endl;
        (*m_ssLayer) << "  5" << endl;
        (*m_ssLayer) << getLayerHandle() << endl;
        if (m_version > 12) {
            (*m_ssLayer) << "330" << endl;
            (*m_ssLayer) << tablehash << endl;
            (*m_ssLayer) << "100" << endl;
            (*m_ssLayer) << "AcDbSymbolTableRecord" << endl;
            (*m_ssLayer) << "100" << endl;
            (*m_ssLayer) << "AcDbLayerTableRecord" << endl;
        }
        (*m_ssLayer) << "  2" << endl;
        (*m_ssLayer) << l << endl;
        (*m_ssLayer) << " 70" << endl;
        (*m_ssLayer) << "    0" << endl;
        (*m_ssLayer) << " 62" << endl;
        (*m_ssLayer) << "    7" << endl;
        (*m_ssLayer) << "  6" << endl;
        (*m_ssLayer) << "CONTINUOUS" << endl;
    }
    (*m_ssLayer) << "  0" << endl;
    (*m_ssLayer) << "ENDTAB" << endl;
}

//***************************
// makeBlockRecordTableHead
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::makeBlockRecordTableHead()
{
    if (m_version < 14) {
        return;
    }
    std::string tablehash = getBlkRecordHandle();
    m_saveBlockRecordTableHandle = tablehash;
    (*m_ssBlkRecord) << "  0" << endl;
    (*m_ssBlkRecord) << "TABLE" << endl;
    (*m_ssBlkRecord) << "  2" << endl;
    (*m_ssBlkRecord) << "BLOCK_RECORD" << endl;
    (*m_ssBlkRecord) << "  5" << endl;
    (*m_ssBlkRecord) << tablehash << endl;
    (*m_ssBlkRecord) << "330" << endl;
    (*m_ssBlkRecord) << "0" << endl;
    (*m_ssBlkRecord) << "100" << endl;
    (*m_ssBlkRecord) << "AcDbSymbolTable" << endl;
    (*m_ssBlkRecord) << "  70" << endl;
    (*m_ssBlkRecord) << (m_blockList.size() + 5) << endl;

    m_saveModelSpaceHandle = getBlkRecordHandle();
    (*m_ssBlkRecord) << "  0" << endl;
    (*m_ssBlkRecord) << "BLOCK_RECORD" << endl;
    (*m_ssBlkRecord) << "  5" << endl;
    (*m_ssBlkRecord) << m_saveModelSpaceHandle << endl;
    (*m_ssBlkRecord) << "330" << endl;
    (*m_ssBlkRecord) << tablehash << endl;
    (*m_ssBlkRecord) << "100" << endl;
    (*m_ssBlkRecord) << "AcDbSymbolTableRecord" << endl;
    (*m_ssBlkRecord) << "100" << endl;
    (*m_ssBlkRecord) << "AcDbBlockTableRecord" << endl;
    (*m_ssBlkRecord) << "  2" << endl;
    (*m_ssBlkRecord) << "*MODEL_SPACE" << endl;
    //        (*m_ssBlkRecord) << "  1"      << endl;
    //        (*m_ssBlkRecord) << " "        << endl;

    m_savePaperSpaceHandle = getBlkRecordHandle();
    (*m_ssBlkRecord) << "  0" << endl;
    (*m_ssBlkRecord) << "BLOCK_RECORD" << endl;
    (*m_ssBlkRecord) << "  5" << endl;
    (*m_ssBlkRecord) << m_savePaperSpaceHandle << endl;
    (*m_ssBlkRecord) << "330" << endl;
    (*m_ssBlkRecord) << tablehash << endl;
    (*m_ssBlkRecord) << "100" << endl;
    (*m_ssBlkRecord) << "AcDbSymbolTableRecord" << endl;
    (*m_ssBlkRecord) << "100" << endl;
    (*m_ssBlkRecord) << "AcDbBlockTableRecord" << endl;
    (*m_ssBlkRecord) << "  2" << endl;
    (*m_ssBlkRecord) << "*PAPER_SPACE" << endl;
    //        (*m_ssBlkRecord) << "  1"      << endl;
    //        (*m_ssBlkRecord) << " "        << endl;
}

//***************************
// makeBlockRecordTableBody
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::makeBlockRecordTableBody()
{
    if (m_version < 14) {
        return;
    }

    int iBlkRecord = 0;
    for (auto& b : m_blockList) {
        (*m_ssBlkRecord) << "  0" << endl;
        (*m_ssBlkRecord) << "BLOCK_RECORD" << endl;
        (*m_ssBlkRecord) << "  5" << endl;
        (*m_ssBlkRecord) << m_blkRecordList.at(iBlkRecord) << endl;
        (*m_ssBlkRecord) << "330" << endl;
        (*m_ssBlkRecord) << m_saveBlockRecordTableHandle << endl;
        (*m_ssBlkRecord) << "100" << endl;
        (*m_ssBlkRecord) << "AcDbSymbolTableRecord" << endl;
        (*m_ssBlkRecord) << "100" << endl;
        (*m_ssBlkRecord) << "AcDbBlockTableRecord" << endl;
        (*m_ssBlkRecord) << "  2" << endl;
        (*m_ssBlkRecord) << b << endl;
        //        (*m_ssBlkRecord) << " 70"      << endl;
        //        (*m_ssBlkRecord) << "    0"      << endl;
        iBlkRecord++;
    }
}

//***************************
// makeBlockSectionHead
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::makeBlockSectionHead()
{
    (*m_ssBlock) << "  0" << endl;
    (*m_ssBlock) << "SECTION" << endl;
    (*m_ssBlock) << "  2" << endl;
    (*m_ssBlock) << "BLOCKS" << endl;
    (*m_ssBlock) << "  0" << endl;
    (*m_ssBlock) << "BLOCK" << endl;
    (*m_ssBlock) << "  5" << endl;
    m_currentBlock = getBlockHandle();
    (*m_ssBlock) << m_currentBlock << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "330" << endl;
        (*m_ssBlock) << m_saveModelSpaceHandle << endl;
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbEntity" << endl;
    }
    (*m_ssBlock) << "  8" << endl;
    (*m_ssBlock) << "0" << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbBlockBegin" << endl;
    }
    (*m_ssBlock) << "  2" << endl;
    (*m_ssBlock) << "*MODEL_SPACE" << endl;
    (*m_ssBlock) << " 70" << endl;
    (*m_ssBlock) << "   0" << endl;
    (*m_ssBlock) << " 10" << endl;
    (*m_ssBlock) << 0.0 << endl;
    (*m_ssBlock) << " 20" << endl;
    (*m_ssBlock) << 0.0 << endl;
    (*m_ssBlock) << " 30" << endl;
    (*m_ssBlock) << 0.0 << endl;
    (*m_ssBlock) << "  3" << endl;
    (*m_ssBlock) << "*MODEL_SPACE" << endl;
    (*m_ssBlock) << "  1" << endl;
    (*m_ssBlock) << " " << endl;
    (*m_ssBlock) << "  0" << endl;
    (*m_ssBlock) << "ENDBLK" << endl;
    (*m_ssBlock) << "  5" << endl;
    (*m_ssBlock) << getBlockHandle() << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "330" << endl;
        (*m_ssBlock) << m_saveModelSpaceHandle << endl;
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbEntity" << endl;
    }
    (*m_ssBlock) << "  8" << endl;
    (*m_ssBlock) << "0" << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbBlockEnd" << endl;
    }

    (*m_ssBlock) << "  0" << endl;
    (*m_ssBlock) << "BLOCK" << endl;
    (*m_ssBlock) << "  5" << endl;
    m_currentBlock = getBlockHandle();
    (*m_ssBlock) << m_currentBlock << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "330" << endl;
        (*m_ssBlock) << m_savePaperSpaceHandle << endl;
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbEntity" << endl;
        (*m_ssBlock) << " 67" << endl;
        (*m_ssBlock) << "1" << endl;
    }
    (*m_ssBlock) << "  8" << endl;
    (*m_ssBlock) << "0" << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbBlockBegin" << endl;
    }
    (*m_ssBlock) << "  2" << endl;
    (*m_ssBlock) << "*PAPER_SPACE" << endl;
    (*m_ssBlock) << " 70" << endl;
    (*m_ssBlock) << "   0" << endl;
    (*m_ssBlock) << " 10" << endl;
    (*m_ssBlock) << 0.0 << endl;
    (*m_ssBlock) << " 20" << endl;
    (*m_ssBlock) << 0.0 << endl;
    (*m_ssBlock) << " 30" << endl;
    (*m_ssBlock) << 0.0 << endl;
    (*m_ssBlock) << "  3" << endl;
    (*m_ssBlock) << "*PAPER_SPACE" << endl;
    (*m_ssBlock) << "  1" << endl;
    (*m_ssBlock) << " " << endl;
    (*m_ssBlock) << "  0" << endl;
    (*m_ssBlock) << "ENDBLK" << endl;
    (*m_ssBlock) << "  5" << endl;
    (*m_ssBlock) << getBlockHandle() << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "330" << endl;
        (*m_ssBlock) << m_savePaperSpaceHandle << endl;
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbEntity" << endl;
        (*m_ssBlock) << " 67" << endl;  // paper_space flag
        (*m_ssBlock) << "    1" << endl;
    }
    (*m_ssBlock) << "  8" << endl;
    (*m_ssBlock) << "0" << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbBlockEnd" << endl;
    }
}

std::string CDxfWrite::getPlateFile(std::string fileSpec)
{
    std::stringstream outString;
    Base::FileInfo fi(fileSpec);
    if (!fi.isReadable()) {
        Base::Console().Message("dxf unable to open %s!\n", fileSpec.c_str());
    }
    else {
        string line;
        ifstream inFile(fi.filePath());

        while (!inFile.eof()) {
            getline(inFile, line);
            if (!inFile.eof()) {
                outString << line << '\n';
            }
        }
    }
    return outString.str();
}

std::string CDxfWrite::getHandle()
{
    m_handle++;
    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
    ss << m_handle;
    return ss.str();
}

std::string CDxfWrite::getEntityHandle()
{
    return getHandle();
    //    m_entityHandle++;
    //    std::stringstream ss;
    //    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
    //    ss << m_entityHandle;
    //    return ss.str();
}

std::string CDxfWrite::getLayerHandle()
{
    return getHandle();
    //    m_layerHandle++;
    //    std::stringstream ss;
    //    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
    //    ss << m_layerHandle;
    //    return ss.str();
}

std::string CDxfWrite::getBlockHandle()
{
    return getHandle();
    //    m_blockHandle++;
    //    std::stringstream ss;
    //    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
    //    ss << m_blockHandle;
    //    return ss.str();
}

std::string CDxfWrite::getBlkRecordHandle()
{
    return getHandle();
    //    m_blkRecordHandle++;
    //    std::stringstream ss;
    //    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
    //    ss << m_blkRecordHandle;
    //    return ss.str();
}

void CDxfWrite::addBlockName(const std::string& name, const std::string& blkRecordHandle)
{
    m_blockList.push_back(name);
    m_blkRecordList.push_back(blkRecordHandle);
}

void CDxfWrite::setLayerName(std::string name)
{
    m_layerName = name;
    m_layerList.push_back(name);
}

void CDxfWrite::writeLine(const double* start, const double* end)
{
    putLine(toVector3d(start),
            toVector3d(end),
            m_ssEntity,
            getEntityHandle(),
            m_saveModelSpaceHandle);
}

void CDxfWrite::putLine(const Base::Vector3d& start,
                        const Base::Vector3d& end,
                        std::ostringstream* outStream,
                        const std::string& handle,
                        const std::string& ownerHandle)
{
    (*outStream) << "  0" << endl;
    (*outStream) << "LINE" << endl;
    (*outStream) << "  5" << endl;
    (*outStream) << handle << endl;
    if (m_version > 12) {
        (*outStream) << "330" << endl;
        (*outStream) << ownerHandle << endl;
        (*outStream) << "100" << endl;
        (*outStream) << "AcDbEntity" << endl;
    }
    (*outStream) << "  8" << endl;           // Group code for layer name
    (*outStream) << getLayerName() << endl;  // Layer number
    if (m_version > 12) {
        (*outStream) << "100" << endl;
        (*outStream) << "AcDbLine" << endl;
    }
    (*outStream) << " 10" << endl;    // Start point of line
    (*outStream) << start.x << endl;  // X in WCS coordinates
    (*outStream) << " 20" << endl;
    (*outStream) << start.y << endl;  // Y in WCS coordinates
    (*outStream) << " 30" << endl;
    (*outStream) << start.z << endl;  // Z in WCS coordinates
    (*outStream) << " 11" << endl;    // End point of line
    (*outStream) << end.x << endl;    // X in WCS coordinates
    (*outStream) << " 21" << endl;
    (*outStream) << end.y << endl;  // Y in WCS coordinates
    (*outStream) << " 31" << endl;
    (*outStream) << end.z << endl;  // Z in WCS coordinates
}


//***************************
// writeLWPolyLine  (Note: LWPolyline might not be supported in R12
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeLWPolyLine(const LWPolyDataOut& pd)
{
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "LWPOLYLINE" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;  // 100 groups are not part of R12
        (*m_ssEntity) << "AcDbPolyline" << endl;
    }
    (*m_ssEntity) << "  8" << endl;           // Group code for layer name
    (*m_ssEntity) << getLayerName() << endl;  // Layer name
    (*m_ssEntity) << " 90" << endl;
    (*m_ssEntity) << pd.nVert << endl;  // number of vertices
    (*m_ssEntity) << " 70" << endl;
    (*m_ssEntity) << pd.Flag << endl;
    (*m_ssEntity) << " 43" << endl;
    (*m_ssEntity) << "0" << endl;  // Constant width opt
    //    (*m_ssEntity) << pd.Width         << endl;    //Constant width opt
    //    (*m_ssEntity) << " 38"            << endl;
    //    (*m_ssEntity) << pd.Elev          << endl;    // Elevation
    //    (*m_ssEntity) << " 39"            << endl;
    //    (*m_ssEntity) << pd.Thick         << endl;    // Thickness
    for (auto& p : pd.Verts) {
        (*m_ssEntity) << " 10" << endl;  // Vertices
        (*m_ssEntity) << p.x << endl;
        (*m_ssEntity) << " 20" << endl;
        (*m_ssEntity) << p.y << endl;
    }
    for (auto& s : pd.StartWidth) {
        (*m_ssEntity) << " 40" << endl;
        (*m_ssEntity) << s << endl;  // Start Width
    }
    for (auto& e : pd.EndWidth) {
        (*m_ssEntity) << " 41" << endl;
        (*m_ssEntity) << e << endl;  // End Width
    }
    for (auto& b : pd.Bulge) {  // Bulge
        (*m_ssEntity) << " 42" << endl;
        (*m_ssEntity) << b << endl;
    }
    //    (*m_ssEntity) << "210"            << endl;    //Extrusion dir
    //    (*m_ssEntity) << pd.Extr.x        << endl;
    //    (*m_ssEntity) << "220"            << endl;
    //    (*m_ssEntity) << pd.Extr.y        << endl;
    //    (*m_ssEntity) << "230"            << endl;
    //    (*m_ssEntity) << pd.Extr.z        << endl;
}

//***************************
// writePolyline
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writePolyline(const LWPolyDataOut& pd)
{
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "POLYLINE" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;
    (*m_ssEntity) << getLayerName() << endl;  // Layer name
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;  // 100 groups are not part of R12
        (*m_ssEntity) << "AcDbPolyline" << endl;
    }
    (*m_ssEntity) << " 66" << endl;
    (*m_ssEntity) << "     1" << endl;  // vertices follow
    (*m_ssEntity) << " 10" << endl;
    (*m_ssEntity) << "0.0" << endl;
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << "0.0" << endl;
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << "0.0" << endl;
    (*m_ssEntity) << " 70" << endl;
    (*m_ssEntity) << "0" << endl;
    for (auto& p : pd.Verts) {
        (*m_ssEntity) << "  0" << endl;
        (*m_ssEntity) << "VERTEX" << endl;
        (*m_ssEntity) << "  5" << endl;
        (*m_ssEntity) << getEntityHandle() << endl;
        (*m_ssEntity) << "  8" << endl;
        (*m_ssEntity) << getLayerName() << endl;
        (*m_ssEntity) << " 10" << endl;
        (*m_ssEntity) << p.x << endl;
        (*m_ssEntity) << " 20" << endl;
        (*m_ssEntity) << p.y << endl;
        (*m_ssEntity) << " 30" << endl;
        (*m_ssEntity) << p.z << endl;
    }
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "SEQEND" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    (*m_ssEntity) << "  8" << endl;
    (*m_ssEntity) << getLayerName() << endl;
}

void CDxfWrite::writePoint(const double* point)
{
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "POINT" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;           // Group code for layer name
    (*m_ssEntity) << getLayerName() << endl;  // Layer name
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbPoint" << endl;
    }
    (*m_ssEntity) << " 10" << endl;
    (*m_ssEntity) << point[0] << endl;  // X in WCS coordinates
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << point[1] << endl;  // Y in WCS coordinates
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << point[2] << endl;  // Z in WCS coordinates
}

//! arc from 3 points - start, end, center. dir true if arc is AntiClockwise. unspecified assumption
//! is that points are on XY plane in coord system OXYZ.
void CDxfWrite::writeArc(const double* start, const double* end, const double* center, bool dir)

{
    double ax = start[0] - center[0];
    double ay = start[1] - center[1];
    double bx = end[0] - center[0];
    double by = end[1] - center[1];

    double start_angle = Base::toDegrees(atan2(ay, ax));
    double end_angle = Base::toDegrees(atan2(by, bx));
    double radius = sqrt(ax * ax + ay * ay);
    if (!dir) {
        double temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "ARC" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;           // Group code for layer name
    (*m_ssEntity) << getLayerName() << endl;  // Layer number
                                              //    (*m_ssEntity) << " 62"          << endl;
                                              //    (*m_ssEntity) << "     0"       << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbCircle" << endl;
    }
    (*m_ssEntity) << " 10" << endl;      // Centre X
    (*m_ssEntity) << center[0] << endl;  // X in WCS coordinates
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << center[1] << endl;  // Y in WCS coordinates
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << center[2] << endl;  // Z in WCS coordinates
    (*m_ssEntity) << " 40" << endl;      //
    (*m_ssEntity) << radius << endl;     // Radius

    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbArc" << endl;
    }
    (*m_ssEntity) << " 50" << endl;
    (*m_ssEntity) << start_angle << endl;  // Start angle
    (*m_ssEntity) << " 51" << endl;
    (*m_ssEntity) << end_angle << endl;  // End angle
}

void CDxfWrite::writeCircle(const double* center, double radius)
{
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "CIRCLE" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;           // Group code for layer name
    (*m_ssEntity) << getLayerName() << endl;  // Layer number
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbCircle" << endl;
    }
    (*m_ssEntity) << " 10" << endl;      // Centre X
    (*m_ssEntity) << center[0] << endl;  // X in WCS coordinates
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << center[1] << endl;  // Y in WCS coordinates
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << center[2] << endl;  // Z in WCS coordinates
    (*m_ssEntity) << " 40" << endl;      //
    (*m_ssEntity) << radius << endl;     // Radius
}

void CDxfWrite::writeEllipse(const double* center,
                             double major_radius,
                             double minor_radius,
                             double rotation,
                             double start_angle,
                             double end_angle,
                             bool endIsCW)
{
    Base::Vector3d m(major_radius * sin(rotation), major_radius * cos(rotation), 0);
    double ratio = minor_radius / major_radius;

    if (!endIsCW) {  // end is NOT CW from start
        double temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "ELLIPSE" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;           // Group code for layer name
    (*m_ssEntity) << getLayerName() << endl;  // Layer number
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEllipse" << endl;
    }
    (*m_ssEntity) << " 10" << endl;      // Centre X
    (*m_ssEntity) << center[0] << endl;  // X in WCS coordinates
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << center[1] << endl;  // Y in WCS coordinates
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << center[2] << endl;  // Z in WCS coordinates
    (*m_ssEntity) << " 11" << endl;      //
    (*m_ssEntity) << m.x << endl;        // Major X
    (*m_ssEntity) << " 21" << endl;
    (*m_ssEntity) << m.y << endl;  // Major Y
    (*m_ssEntity) << " 31" << endl;
    (*m_ssEntity) << m.z << endl;    // Major Z
    (*m_ssEntity) << " 40" << endl;  //
    (*m_ssEntity) << ratio
                  << endl;  // Ratio
                            //    (*m_ssEntity) << "210"       << endl;    //extrusion dir??
                            //    (*m_ssEntity) << "0"         << endl;
                            //    (*m_ssEntity) << "220"       << endl;
                            //    (*m_ssEntity) << "0"         << endl;
                            //    (*m_ssEntity) << "230"       << endl;
                            //    (*m_ssEntity) << "1"         << endl;
    (*m_ssEntity) << " 41" << endl;
    (*m_ssEntity) << start_angle << endl;  // Start angle (radians [0..2pi])
    (*m_ssEntity) << " 42" << endl;
    (*m_ssEntity) << end_angle << endl;  // End angle
}

//***************************
// writeSpline
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeSpline(const SplineDataOut& sd)
{
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "SPLINE" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;           // Group code for layer name
    (*m_ssEntity) << getLayerName() << endl;  // Layer name
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbSpline" << endl;
    }
    (*m_ssEntity) << "210" << endl;
    (*m_ssEntity) << "0" << endl;
    (*m_ssEntity) << "220" << endl;
    (*m_ssEntity) << "0" << endl;
    (*m_ssEntity) << "230" << endl;
    (*m_ssEntity) << "1" << endl;

    (*m_ssEntity) << " 70" << endl;
    (*m_ssEntity) << sd.flag << endl;  // flags
    (*m_ssEntity) << " 71" << endl;
    (*m_ssEntity) << sd.degree << endl;
    (*m_ssEntity) << " 72" << endl;
    (*m_ssEntity) << sd.knots << endl;
    (*m_ssEntity) << " 73" << endl;
    (*m_ssEntity) << sd.control_points << endl;
    (*m_ssEntity) << " 74" << endl;
    (*m_ssEntity) << 0 << endl;

    //    (*m_ssEntity) << " 12"          << endl;
    //    (*m_ssEntity) << sd.starttan.x  << endl;
    //    (*m_ssEntity) << " 22"          << endl;
    //    (*m_ssEntity) << sd.starttan.y  << endl;
    //    (*m_ssEntity) << " 32"          << endl;
    //    (*m_ssEntity) << sd.starttan.z  << endl;
    //    (*m_ssEntity) << " 13"          << endl;
    //    (*m_ssEntity) << sd.endtan.x    << endl;
    //    (*m_ssEntity) << " 23"          << endl;
    //    (*m_ssEntity) << sd.endtan.y    << endl;
    //    (*m_ssEntity) << " 33"          << endl;
    //    (*m_ssEntity) << sd.endtan.z    << endl;

    for (auto& k : sd.knot) {
        (*m_ssEntity) << " 40" << endl;
        (*m_ssEntity) << k << endl;
    }

    for (auto& w : sd.weight) {
        (*m_ssEntity) << " 41" << endl;
        (*m_ssEntity) << w << endl;
    }

    for (auto& center : sd.control) {
        (*m_ssEntity) << " 10" << endl;
        (*m_ssEntity) << center.x << endl;  // X in WCS coordinates
        (*m_ssEntity) << " 20" << endl;
        (*m_ssEntity) << center.y << endl;  // Y in WCS coordinates
        (*m_ssEntity) << " 30" << endl;
        (*m_ssEntity) << center.z << endl;  // Z in WCS coordinates
    }
    for (auto& f : sd.fit) {
        (*m_ssEntity) << " 11" << endl;
        (*m_ssEntity) << f.x << endl;  // X in WCS coordinates
        (*m_ssEntity) << " 21" << endl;
        (*m_ssEntity) << f.y << endl;  // Y in WCS coordinates
        (*m_ssEntity) << " 31" << endl;
        (*m_ssEntity) << f.z << endl;  // Z in WCS coordinates
    }
}

//***************************
// writeVertex
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeVertex(double x, double y, double z)
{
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "VERTEX" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;
    (*m_ssEntity) << getLayerName() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbVertex" << endl;
    }
    (*m_ssEntity) << " 10" << endl;
    (*m_ssEntity) << x << endl;
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << y << endl;
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << z << endl;
    (*m_ssEntity) << " 70" << endl;
    (*m_ssEntity) << 0 << endl;
}

void CDxfWrite::writeText(const char* text,
                          const double* location1,
                          const double* location2,
                          const double height,
                          const int horizJust)
{
    putText(text,
            toVector3d(location1),
            toVector3d(location2),
            height,
            horizJust,
            m_ssEntity,
            getEntityHandle(),
            m_saveModelSpaceHandle);
}

//***************************
// putText
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::putText(const char* text,
                        const Base::Vector3d& location1,
                        const Base::Vector3d& location2,
                        const double height,
                        const int horizJust,
                        std::ostringstream* outStream,
                        const std::string& handle,
                        const std::string& ownerHandle)
{
    (void)location2;

    (*outStream) << "  0" << endl;
    (*outStream) << "TEXT" << endl;
    (*outStream) << "  5" << endl;
    (*outStream) << handle << endl;
    if (m_version > 12) {
        (*outStream) << "330" << endl;
        (*outStream) << ownerHandle << endl;
        (*outStream) << "100" << endl;
        (*outStream) << "AcDbEntity" << endl;
    }
    (*outStream) << "  8" << endl;
    (*outStream) << getLayerName() << endl;
    if (m_version > 12) {
        (*outStream) << "100" << endl;
        (*outStream) << "AcDbText" << endl;
    }
    //    (*outStream) << " 39"          << endl;
    //    (*outStream) << 0              << endl;     //thickness
    (*outStream) << " 10" << endl;  // first alignment point
    (*outStream) << location1.x << endl;
    (*outStream) << " 20" << endl;
    (*outStream) << location1.y << endl;
    (*outStream) << " 30" << endl;
    (*outStream) << location1.z << endl;
    (*outStream) << " 40" << endl;
    (*outStream) << height << endl;
    (*outStream) << "  1" << endl;
    (*outStream) << text << endl;
    //    (*outStream) << " 50"          << endl;
    //    (*outStream) << 0              << endl;    //rotation
    //    (*outStream) << " 41"          << endl;
    //    (*outStream) << 1              << endl;
    //    (*outStream) << " 51"          << endl;
    //    (*outStream) << 0              << endl;

    (*outStream) << "  7" << endl;
    (*outStream) << "STANDARD" << endl;  // style
    //    (*outStream) << " 71"          << endl;  //default
    //    (*outStream) << "0"            << endl;
    (*outStream) << " 72" << endl;
    (*outStream) << horizJust << endl;
    ////    (*outStream) << " 73"          << endl;
    ////    (*outStream) << "0"            << endl;
    (*outStream) << " 11" << endl;  // second alignment point
    (*outStream) << location2.x << endl;
    (*outStream) << " 21" << endl;
    (*outStream) << location2.y << endl;
    (*outStream) << " 31" << endl;
    (*outStream) << location2.z << endl;
    //    (*outStream) << "210"          << endl;
    //    (*outStream) << "0"            << endl;
    //    (*outStream) << "220"          << endl;
    //    (*outStream) << "0"            << endl;
    //    (*outStream) << "230"          << endl;
    //    (*outStream) << "1"            << endl;
    if (m_version > 12) {
        (*outStream) << "100" << endl;
        (*outStream) << "AcDbText" << endl;
    }
}

void CDxfWrite::putArrow(Base::Vector3d& arrowPos,
                         Base::Vector3d& barb1Pos,
                         Base::Vector3d& barb2Pos,
                         std::ostringstream* outStream,
                         const std::string& handle,
                         const std::string& ownerHandle)
{
    (*outStream) << "  0" << endl;
    (*outStream) << "SOLID" << endl;
    (*outStream) << "  5" << endl;
    (*outStream) << handle << endl;
    if (m_version > 12) {
        (*outStream) << "330" << endl;
        (*outStream) << ownerHandle << endl;
        (*outStream) << "100" << endl;
        (*outStream) << "AcDbEntity" << endl;
    }
    (*outStream) << "  8" << endl;
    (*outStream) << "0" << endl;
    (*outStream) << " 62" << endl;
    (*outStream) << "     0" << endl;
    if (m_version > 12) {
        (*outStream) << "100" << endl;
        (*outStream) << "AcDbTrace" << endl;
    }
    (*outStream) << " 10" << endl;
    (*outStream) << barb1Pos.x << endl;
    (*outStream) << " 20" << endl;
    (*outStream) << barb1Pos.y << endl;
    (*outStream) << " 30" << endl;
    (*outStream) << barb1Pos.z << endl;
    (*outStream) << " 11" << endl;
    (*outStream) << barb2Pos.x << endl;
    (*outStream) << " 21" << endl;
    (*outStream) << barb2Pos.y << endl;
    (*outStream) << " 31" << endl;
    (*outStream) << barb2Pos.z << endl;
    (*outStream) << " 12" << endl;
    (*outStream) << arrowPos.x << endl;
    (*outStream) << " 22" << endl;
    (*outStream) << arrowPos.y << endl;
    (*outStream) << " 32" << endl;
    (*outStream) << arrowPos.z << endl;
    (*outStream) << " 13" << endl;
    (*outStream) << arrowPos.x << endl;
    (*outStream) << " 23" << endl;
    (*outStream) << arrowPos.y << endl;
    (*outStream) << " 33" << endl;
    (*outStream) << arrowPos.z << endl;
}

//***************************
// writeLinearDim
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
#define ALIGNED 0
#define HORIZONTAL 1
#define VERTICAL 2
void CDxfWrite::writeLinearDim(const double* textMidPoint,
                               const double* lineDefPoint,
                               const double* extLine1,
                               const double* extLine2,
                               const char* dimText,
                               int type)
{
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "DIMENSION" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;
    (*m_ssEntity) << getLayerName() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbDimension" << endl;
    }
    (*m_ssEntity) << "  2" << endl;
    (*m_ssEntity) << "*" << getLayerName() << endl;  // blockName
    (*m_ssEntity) << " 10" << endl;                  // dimension line definition point
    (*m_ssEntity) << lineDefPoint[0] << endl;
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << lineDefPoint[1] << endl;
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << lineDefPoint[2] << endl;
    (*m_ssEntity) << " 11" << endl;  // text mid point
    (*m_ssEntity) << textMidPoint[0] << endl;
    (*m_ssEntity) << " 21" << endl;
    (*m_ssEntity) << textMidPoint[1] << endl;
    (*m_ssEntity) << " 31" << endl;
    (*m_ssEntity) << textMidPoint[2] << endl;
    if (type == ALIGNED) {
        (*m_ssEntity) << " 70" << endl;
        (*m_ssEntity) << 1 << endl;  // dimType1 = Aligned
    }
    if ((type == HORIZONTAL) || (type == VERTICAL)) {
        (*m_ssEntity) << " 70" << endl;
        (*m_ssEntity) << 32 << endl;  // dimType0 = Aligned + 32 (bit for unique block)?
    }
    //    (*m_ssEntity) << " 71"          << endl;    // not R12
    //    (*m_ssEntity) << 1              << endl;    // attachPoint ??1 = topleft
    (*m_ssEntity) << "  1" << endl;
    (*m_ssEntity) << dimText << endl;
    (*m_ssEntity) << "  3" << endl;
    (*m_ssEntity) << "STANDARD" << endl;  // style
    // linear dims
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbAlignedDimension" << endl;
    }
    (*m_ssEntity) << " 13" << endl;
    (*m_ssEntity) << extLine1[0] << endl;
    (*m_ssEntity) << " 23" << endl;
    (*m_ssEntity) << extLine1[1] << endl;
    (*m_ssEntity) << " 33" << endl;
    (*m_ssEntity) << extLine1[2] << endl;
    (*m_ssEntity) << " 14" << endl;
    (*m_ssEntity) << extLine2[0] << endl;
    (*m_ssEntity) << " 24" << endl;
    (*m_ssEntity) << extLine2[1] << endl;
    (*m_ssEntity) << " 34" << endl;
    (*m_ssEntity) << extLine2[2] << endl;
    if (m_version > 12) {
        if (type == VERTICAL) {
            (*m_ssEntity) << " 50" << endl;
            (*m_ssEntity) << "90" << endl;
        }
        if ((type == HORIZONTAL) || (type == VERTICAL)) {
            (*m_ssEntity) << "100" << endl;
            (*m_ssEntity) << "AcDbRotatedDimension" << endl;
        }
    }

    writeDimBlockPreamble();
    writeLinearDimBlock(textMidPoint, lineDefPoint, extLine1, extLine2, dimText, type);
    writeBlockTrailer();
}

//***************************
// writeAngularDim
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeAngularDim(const double* textMidPoint,
                                const double* lineDefPoint,
                                const double* startExt1,
                                const double* endExt1,
                                const double* startExt2,
                                const double* endExt2,
                                const char* dimText)
{
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "DIMENSION" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;
    (*m_ssEntity) << getLayerName() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbDimension" << endl;
    }
    (*m_ssEntity) << "  2" << endl;
    (*m_ssEntity) << "*" << getLayerName() << endl;  // blockName

    (*m_ssEntity) << " 10" << endl;
    (*m_ssEntity) << endExt2[0] << endl;
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << endExt2[1] << endl;
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << endExt2[2] << endl;

    (*m_ssEntity) << " 11" << endl;
    (*m_ssEntity) << textMidPoint[0] << endl;
    (*m_ssEntity) << " 21" << endl;
    (*m_ssEntity) << textMidPoint[1] << endl;
    (*m_ssEntity) << " 31" << endl;
    (*m_ssEntity) << textMidPoint[2] << endl;

    (*m_ssEntity) << " 70" << endl;
    (*m_ssEntity) << 2 << endl;  // dimType 2 = Angular  5 = Angular 3 point
                                 // +32 for block?? (not R12)
    //    (*m_ssEntity) << " 71"          << endl;    // not R12?  not required?
    //    (*m_ssEntity) << 5              << endl;    // attachPoint 5 = middle
    (*m_ssEntity) << "  1" << endl;
    (*m_ssEntity) << dimText << endl;
    (*m_ssEntity) << "  3" << endl;
    (*m_ssEntity) << "STANDARD" << endl;  // style
    // angular dims
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDb2LineAngularDimension" << endl;
    }
    (*m_ssEntity) << " 13" << endl;
    (*m_ssEntity) << startExt1[0] << endl;
    (*m_ssEntity) << " 23" << endl;
    (*m_ssEntity) << startExt1[1] << endl;
    (*m_ssEntity) << " 33" << endl;
    (*m_ssEntity) << startExt1[2] << endl;

    (*m_ssEntity) << " 14" << endl;
    (*m_ssEntity) << endExt1[0] << endl;
    (*m_ssEntity) << " 24" << endl;
    (*m_ssEntity) << endExt1[1] << endl;
    (*m_ssEntity) << " 34" << endl;
    (*m_ssEntity) << endExt1[2] << endl;

    (*m_ssEntity) << " 15" << endl;
    (*m_ssEntity) << startExt2[0] << endl;
    (*m_ssEntity) << " 25" << endl;
    (*m_ssEntity) << startExt2[1] << endl;
    (*m_ssEntity) << " 35" << endl;
    (*m_ssEntity) << startExt2[2] << endl;

    (*m_ssEntity) << " 16" << endl;
    (*m_ssEntity) << lineDefPoint[0] << endl;
    (*m_ssEntity) << " 26" << endl;
    (*m_ssEntity) << lineDefPoint[1] << endl;
    (*m_ssEntity) << " 36" << endl;
    (*m_ssEntity) << lineDefPoint[2] << endl;
    writeDimBlockPreamble();
    writeAngularDimBlock(textMidPoint,
                         lineDefPoint,
                         startExt1,
                         endExt1,
                         startExt2,
                         endExt2,
                         dimText);
    writeBlockTrailer();
}

//***************************
// writeRadialDim
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeRadialDim(const double* centerPoint,
                               const double* textMidPoint,
                               const double* arcPoint,
                               const char* dimText)
{
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "DIMENSION" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;
    (*m_ssEntity) << getLayerName() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbDimension" << endl;
    }
    (*m_ssEntity) << "  2" << endl;
    (*m_ssEntity) << "*" << getLayerName() << endl;  // blockName
    (*m_ssEntity) << " 10" << endl;                  // arc center point
    (*m_ssEntity) << centerPoint[0] << endl;
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << centerPoint[1] << endl;
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << centerPoint[2] << endl;
    (*m_ssEntity) << " 11" << endl;  // text mid point
    (*m_ssEntity) << textMidPoint[0] << endl;
    (*m_ssEntity) << " 21" << endl;
    (*m_ssEntity) << textMidPoint[1] << endl;
    (*m_ssEntity) << " 31" << endl;
    (*m_ssEntity) << textMidPoint[2] << endl;
    (*m_ssEntity) << " 70" << endl;
    (*m_ssEntity) << 4 << endl;  // dimType 4 = Radius
                                 //    (*m_ssEntity) << " 71"          << endl;    // not R12
    //    (*m_ssEntity) << 1              << endl;    // attachPoint 5 = middle center
    (*m_ssEntity) << "  1" << endl;
    (*m_ssEntity) << dimText << endl;
    (*m_ssEntity) << "  3" << endl;
    (*m_ssEntity) << "STANDARD" << endl;  // style
    // radial dims
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbRadialDimension" << endl;
    }
    (*m_ssEntity) << " 15" << endl;
    (*m_ssEntity) << arcPoint[0] << endl;
    (*m_ssEntity) << " 25" << endl;
    (*m_ssEntity) << arcPoint[1] << endl;
    (*m_ssEntity) << " 35" << endl;
    (*m_ssEntity) << arcPoint[2] << endl;
    (*m_ssEntity) << " 40" << endl;  // leader length????
    (*m_ssEntity) << 0 << endl;

    writeDimBlockPreamble();
    writeRadialDimBlock(centerPoint, textMidPoint, arcPoint, dimText);
    writeBlockTrailer();
}

//***************************
// writeDiametricDim
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeDiametricDim(const double* textMidPoint,
                                  const double* arcPoint1,
                                  const double* arcPoint2,
                                  const char* dimText)
{
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "DIMENSION" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "330" << endl;
        (*m_ssEntity) << m_saveModelSpaceHandle << endl;
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbEntity" << endl;
    }
    (*m_ssEntity) << "  8" << endl;
    (*m_ssEntity) << getLayerName() << endl;
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbDimension" << endl;
    }
    (*m_ssEntity) << "  2" << endl;
    (*m_ssEntity) << "*" << getLayerName() << endl;  // blockName
    (*m_ssEntity) << " 10" << endl;
    (*m_ssEntity) << arcPoint1[0] << endl;
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << arcPoint1[1] << endl;
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << arcPoint1[2] << endl;
    (*m_ssEntity) << " 11" << endl;  // text mid point
    (*m_ssEntity) << textMidPoint[0] << endl;
    (*m_ssEntity) << " 21" << endl;
    (*m_ssEntity) << textMidPoint[1] << endl;
    (*m_ssEntity) << " 31" << endl;
    (*m_ssEntity) << textMidPoint[2] << endl;
    (*m_ssEntity) << " 70" << endl;
    (*m_ssEntity) << 3 << endl;  // dimType 3 = Diameter
                                 //    (*m_ssEntity) << " 71"          << endl;    // not R12
    //    (*m_ssEntity) << 5              << endl;    // attachPoint 5 = middle center
    (*m_ssEntity) << "  1" << endl;
    (*m_ssEntity) << dimText << endl;
    (*m_ssEntity) << "  3" << endl;
    (*m_ssEntity) << "STANDARD" << endl;  // style
    // diametric dims
    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbDiametricDimension" << endl;
    }
    (*m_ssEntity) << " 15" << endl;
    (*m_ssEntity) << arcPoint2[0] << endl;
    (*m_ssEntity) << " 25" << endl;
    (*m_ssEntity) << arcPoint2[1] << endl;
    (*m_ssEntity) << " 35" << endl;
    (*m_ssEntity) << arcPoint2[2] << endl;
    (*m_ssEntity) << " 40" << endl;  // leader length????
    (*m_ssEntity) << 0 << endl;

    writeDimBlockPreamble();
    writeDiametricDimBlock(textMidPoint, arcPoint1, arcPoint2, dimText);
    writeBlockTrailer();
}

//***************************
// writeDimBlockPreamble
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeDimBlockPreamble()
{
    if (m_version > 12) {
        std::string blockName("*");
        blockName += getLayerName();
        m_saveBlkRecordHandle = getBlkRecordHandle();
        addBlockName(blockName, m_saveBlkRecordHandle);
    }

    m_currentBlock = getBlockHandle();
    (*m_ssBlock) << "  0" << endl;
    (*m_ssBlock) << "BLOCK" << endl;
    (*m_ssBlock) << "  5" << endl;
    (*m_ssBlock) << m_currentBlock << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "330" << endl;
        (*m_ssBlock) << m_saveBlkRecordHandle << endl;
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbEntity" << endl;
    }
    (*m_ssBlock) << "  8" << endl;
    (*m_ssBlock) << getLayerName() << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbBlockBegin" << endl;
    }
    (*m_ssBlock) << "  2" << endl;
    (*m_ssBlock) << "*" << getLayerName() << endl;  // blockName
    (*m_ssBlock) << " 70" << endl;
    (*m_ssBlock) << "   1" << endl;
    (*m_ssBlock) << " 10" << endl;
    (*m_ssBlock) << 0.0 << endl;
    (*m_ssBlock) << " 20" << endl;
    (*m_ssBlock) << 0.0 << endl;
    (*m_ssBlock) << " 30" << endl;
    (*m_ssBlock) << 0.0 << endl;
    (*m_ssBlock) << "  3" << endl;
    (*m_ssBlock) << "*" << getLayerName() << endl;  // blockName
    (*m_ssBlock) << "  1" << endl;
    (*m_ssBlock) << " " << endl;
}

//***************************
// writeBlockTrailer
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeBlockTrailer()
{
    (*m_ssBlock) << "  0" << endl;
    (*m_ssBlock) << "ENDBLK" << endl;
    (*m_ssBlock) << "  5" << endl;
    (*m_ssBlock) << getBlockHandle() << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "330" << endl;
        (*m_ssBlock) << m_saveBlkRecordHandle << endl;
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbEntity" << endl;
    }
    //    (*m_ssBlock) << " 67"    << endl;
    //    (*m_ssBlock) << "1"    << endl;
    (*m_ssBlock) << "  8" << endl;
    (*m_ssBlock) << getLayerName() << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbBlockEnd" << endl;
    }
}

//***************************
// writeLinearDimBlock
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeLinearDimBlock(const double* textMidPoint,
                                    const double* lineDefPoint,
                                    const double* extLine1,
                                    const double* extLine2,
                                    const char* dimText,
                                    int type)
{
    Base::Vector3d e1S(MakeVector3d(extLine1));
    Base::Vector3d e2S(MakeVector3d(extLine2));
    // point on DimLine (somewhere!)
    Base::Vector3d dl(MakeVector3d(lineDefPoint));
    Base::Vector3d perp = dl.DistanceToLineSegment(e2S, e1S);
    Base::Vector3d e1E = e1S - perp;
    Base::Vector3d e2E = e2S - perp;
    Base::Vector3d para = e1E - e2E;
    if (type == ALIGNED) {
        // NOP
    }
    else if (type == HORIZONTAL) {
        double x = extLine1[0];
        double y = lineDefPoint[1];
        e1E = Base::Vector3d(x, y, 0.0);
        x = extLine2[0];
        e2E = Base::Vector3d(x, y, 0.0);
        perp = Base::Vector3d(0, -1, 0);  // down
        para = Base::Vector3d(1, 0, 0);   // right
        if (lineDefPoint[1] > extLine1[1]) {
            perp = Base::Vector3d(0, 1, 0);  // up
        }
        if (extLine1[0] > extLine2[0]) {
            para = Base::Vector3d(-1, 0, 0);  // left
        }
    }
    else if (type == VERTICAL) {
        double x = lineDefPoint[0];
        double y = extLine1[1];
        e1E = Base::Vector3d(x, y, 0.0);
        y = extLine2[1];
        e2E = Base::Vector3d(x, y, 0.0);
        perp = Base::Vector3d(1, 0, 0);
        para = Base::Vector3d(0, 1, 0);
        if (lineDefPoint[0] < extLine1[0]) {
            perp = Base::Vector3d(-1, 0, 0);
        }
        if (extLine1[1] > extLine2[1]) {
            para = Base::Vector3d(0, -1, 0);
        }
    }

    double arrowLen = 5.0;                     // magic number
    double arrowWidth = arrowLen / 6.0 / 2.0;  // magic number calc!

    putLine(e2S, e2E, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);

    putLine(e1S, e1E, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);

    putLine(e1E, e2E, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);

    putText(dimText,
            toVector3d(textMidPoint),
            toVector3d(lineDefPoint),
            3.5,
            1,
            m_ssBlock,
            getBlockHandle(),
            m_saveBlkRecordHandle);

    perp.Normalize();
    para.Normalize();
    Base::Vector3d arrowStart = e1E;
    Base::Vector3d barb1 = arrowStart + perp * arrowWidth - para * arrowLen;
    Base::Vector3d barb2 = arrowStart - perp * arrowWidth - para * arrowLen;
    putArrow(arrowStart, barb1, barb2, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);


    arrowStart = e2E;
    barb1 = arrowStart + perp * arrowWidth + para * arrowLen;
    barb2 = arrowStart - perp * arrowWidth + para * arrowLen;
    putArrow(arrowStart, barb1, barb2, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);
}

//***************************
// writeAngularDimBlock
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeAngularDimBlock(const double* textMidPoint,
                                     const double* lineDefPoint,
                                     const double* startExt1,
                                     const double* endExt1,
                                     const double* startExt2,
                                     const double* endExt2,
                                     const char* dimText)
{
    Base::Vector3d e1S(MakeVector3d(startExt1));  // apex
    Base::Vector3d e2S(MakeVector3d(startExt2));
    Base::Vector3d e1E(MakeVector3d(endExt1));
    Base::Vector3d e2E(MakeVector3d(endExt2));
    Base::Vector3d e1 = e1E - e1S;
    Base::Vector3d e2 = e2E - e2S;

    double startAngle = atan2(e2.y, e2.x);
    double endAngle = atan2(e1.y, e1.x);
    double span = fabs(endAngle - startAngle);
    double offset = span * 0.10;
    if (startAngle < 0) {
        startAngle += 2 * M_PI;
    }
    if (endAngle < 0) {
        endAngle += 2 * M_PI;
    }
    Base::Vector3d startOff(cos(startAngle + offset), sin(startAngle + offset), 0.0);
    Base::Vector3d endOff(cos(endAngle - offset), sin(endAngle - offset), 0.0);
    startAngle = Base::toDegrees(startAngle);
    endAngle = Base::toDegrees(endAngle);

    Base::Vector3d linePt(MakeVector3d(lineDefPoint));
    double radius = (e2S - linePt).Length();

    (*m_ssBlock) << "  0" << endl;
    (*m_ssBlock) << "ARC" << endl;  // dimline arc
    (*m_ssBlock) << "  5" << endl;
    (*m_ssBlock) << getBlockHandle() << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "330" << endl;
        (*m_ssBlock) << m_saveBlkRecordHandle << endl;
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbEntity" << endl;
    }
    (*m_ssBlock) << "  8" << endl;
    (*m_ssBlock) << "0" << endl;
    //    (*m_ssBlock) << " 62"          << endl;
    //    (*m_ssBlock) << "     0"       << endl;
    if (m_version > 12) {
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbCircle" << endl;
    }
    (*m_ssBlock) << " 10" << endl;
    (*m_ssBlock) << startExt2[0] << endl;  // arc center
    (*m_ssBlock) << " 20" << endl;
    (*m_ssBlock) << startExt2[1] << endl;
    (*m_ssBlock) << " 30" << endl;
    (*m_ssBlock) << startExt2[2] << endl;
    (*m_ssBlock) << " 40" << endl;
    (*m_ssBlock) << radius << endl;  // radius
    if (m_version > 12) {
        (*m_ssBlock) << "100" << endl;
        (*m_ssBlock) << "AcDbArc" << endl;
    }
    (*m_ssBlock) << " 50" << endl;
    (*m_ssBlock) << startAngle << endl;  // start angle
    (*m_ssBlock) << " 51" << endl;
    (*m_ssBlock) << endAngle << endl;  // end angle

    putText(dimText,
            toVector3d(textMidPoint),
            toVector3d(textMidPoint),
            3.5,
            1,
            m_ssBlock,
            getBlockHandle(),
            m_saveBlkRecordHandle);

    e1.Normalize();
    e2.Normalize();
    Base::Vector3d arrow1Start = e1S + e1 * radius;
    Base::Vector3d arrow2Start = e2S + e2 * radius;

    // wf: idk why the Tan pts have to be reversed.  something to do with CW angles in Dxf?
    Base::Vector3d endTan = e1S + (startOff * radius);
    Base::Vector3d startTan = e2S + (endOff * radius);
    Base::Vector3d tanP1 = (arrow1Start - startTan).Normalize();
    Base::Vector3d perp1(-tanP1.y, tanP1.x, tanP1.z);
    Base::Vector3d tanP2 = (arrow2Start - endTan).Normalize();
    Base::Vector3d perp2(-tanP2.y, tanP2.x, tanP2.z);
    double arrowLen = 5.0;                     // magic number
    double arrowWidth = arrowLen / 6.0 / 2.0;  // magic number calc!

    Base::Vector3d barb1 = arrow1Start + perp1 * arrowWidth - tanP1 * arrowLen;
    Base::Vector3d barb2 = arrow1Start - perp1 * arrowWidth - tanP1 * arrowLen;

    putArrow(arrow1Start, barb1, barb2, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);

    barb1 = arrow2Start + perp2 * arrowWidth - tanP2 * arrowLen;
    barb2 = arrow2Start - perp2 * arrowWidth - tanP2 * arrowLen;

    putArrow(arrow2Start, barb1, barb2, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);
}

//***************************
// writeRadialDimBlock
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeRadialDimBlock(const double* centerPoint,
                                    const double* textMidPoint,
                                    const double* arcPoint,
                                    const char* dimText)
{
    putLine(toVector3d(centerPoint),
            toVector3d(arcPoint),
            m_ssBlock,
            getBlockHandle(),
            m_saveBlkRecordHandle);

    putText(dimText,
            toVector3d(textMidPoint),
            toVector3d(textMidPoint),
            3.5,
            1,
            m_ssBlock,
            getBlockHandle(),
            m_saveBlkRecordHandle);

    Base::Vector3d center(MakeVector3d(centerPoint));
    Base::Vector3d a(MakeVector3d(arcPoint));
    Base::Vector3d para = a - center;
    double arrowLen = 5.0;                     // magic number
    double arrowWidth = arrowLen / 6.0 / 2.0;  // magic number calc!
    para.Normalize();
    Base::Vector3d perp(-para.y, para.x, para.z);
    Base::Vector3d arrowStart = a;
    Base::Vector3d barb1 = arrowStart + perp * arrowWidth - para * arrowLen;
    Base::Vector3d barb2 = arrowStart - perp * arrowWidth - para * arrowLen;

    putArrow(arrowStart, barb1, barb2, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);
}

//***************************
// writeDiametricDimBlock
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeDiametricDimBlock(const double* textMidPoint,
                                       const double* arcPoint1,
                                       const double* arcPoint2,
                                       const char* dimText)
{
    putLine(toVector3d(arcPoint1),
            toVector3d(arcPoint2),
            m_ssBlock,
            getBlockHandle(),
            m_saveBlkRecordHandle);

    putText(dimText,
            toVector3d(textMidPoint),
            toVector3d(textMidPoint),
            3.5,
            1,
            m_ssBlock,
            getBlockHandle(),
            m_saveBlkRecordHandle);

    Base::Vector3d a1(MakeVector3d(arcPoint1));
    Base::Vector3d a2(MakeVector3d(arcPoint2));
    Base::Vector3d para = a2 - a1;
    double arrowLen = 5.0;                     // magic number
    double arrowWidth = arrowLen / 6.0 / 2.0;  // magic number calc!
    para.Normalize();
    Base::Vector3d perp(-para.y, para.x, para.z);
    Base::Vector3d arrowStart = a1;
    Base::Vector3d barb1 = arrowStart + perp * arrowWidth + para * arrowLen;
    Base::Vector3d barb2 = arrowStart - perp * arrowWidth + para * arrowLen;

    putArrow(arrowStart, barb1, barb2, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);

    arrowStart = a2;
    barb1 = arrowStart + perp * arrowWidth - para * arrowLen;
    barb2 = arrowStart - perp * arrowWidth - para * arrowLen;
    putArrow(arrowStart, barb1, barb2, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);
}

//***************************
// writeBlocksSection
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeBlocksSection()
{
    if (m_version < 14) {
        std::stringstream ss;
        ss << "blocks1" << m_version << ".rub";
        std::string fileSpec = m_dataDir + ss.str();
        (*m_ofs) << getPlateFile(fileSpec);
    }

    // write blocks content
    (*m_ofs) << (*m_ssBlock).str();

    (*m_ofs) << "  0" << endl;
    (*m_ofs) << "ENDSEC" << endl;
}

//***************************
// writeEntitiesSection
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeEntitiesSection()
{
    std::stringstream ss;
    ss << "entities" << m_version << ".rub";
    std::string fileSpec = m_dataDir + ss.str();
    (*m_ofs) << getPlateFile(fileSpec);

    // write entities content
    (*m_ofs) << (*m_ssEntity).str();


    (*m_ofs) << "  0" << endl;
    (*m_ofs) << "ENDSEC" << endl;
}

//***************************
// writeObjectsSection
// added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
void CDxfWrite::writeObjectsSection()
{
    if (m_version < 14) {
        return;
    }
    std::stringstream ss;
    ss << "objects" << m_version << ".rub";
    std::string fileSpec = m_dataDir + ss.str();
    (*m_ofs) << getPlateFile(fileSpec);
}

const DxfUnits DxfUnits::Instance;

CDxfRead::CDxfRead(const std::string& filepath)
    : m_ifs(new ifstream(filepath))
{
    if (!(*m_ifs)) {
        m_fail = true;
        ImportError("DXF file didn't load\n");
        return;
    }
    m_ifs->imbue(std::locale("C"));
}

CDxfRead::~CDxfRead()
{
    delete m_ifs;
    // Delete the Layer objects which are referenced by pointer from the Layers table.
    for (auto& pair : Layers) {
        delete pair.second;
    }
}

// Static member initializers
const std::string CDxfRead::LineTypeByLayer("BYLAYER");     // NOLINT(runtime/string)
const std::string CDxfRead::LineTypeByBlock("BYBLOCK");     // NOLINT(runtime/string)
const std::string CDxfRead::DefaultLineType("CONTINUOUS");  // NOLINT(runtime/string)

//
//  Setup for ProcessCommonEntityAttribute
void CDxfRead::Setup3DVectorAttribute(eDXFGroupCode_t x_record_type, Base::Vector3d& destination)
{
    SetupScaledDoubleAttribute((eDXFGroupCode_t)(x_record_type + eXOffset), destination.x);
    SetupScaledDoubleAttribute((eDXFGroupCode_t)(x_record_type + eYOffset), destination.y);
    SetupScaledDoubleAttribute((eDXFGroupCode_t)(x_record_type + eZOffset), destination.z);
}
void CDxfRead::Setup3DCoordinatesIntoLists(eDXFGroupCode_t x_record_type,
                                           list<double>& x_destination,
                                           list<double>& y_destination,
                                           list<double>& z_destination)
{
    SetupScaledDoubleIntoList((eDXFGroupCode_t)(x_record_type + eXOffset), x_destination);
    SetupScaledDoubleIntoList((eDXFGroupCode_t)(x_record_type + eYOffset), y_destination);
    SetupScaledDoubleIntoList((eDXFGroupCode_t)(x_record_type + eZOffset), z_destination);
}
void CDxfRead::SetupScaledDoubleAttribute(eDXFGroupCode_t x_record_type, double& destination)
{
    m_coordinate_attributes.emplace(x_record_type, std::pair(&ProcessScaledDouble, &destination));
}
void CDxfRead::SetupScaledDoubleIntoList(eDXFGroupCode_t x_record_type, list<double>& destination)
{
    m_coordinate_attributes.emplace(x_record_type,
                                    std::pair(&ProcessScaledDoubleIntoList, &destination));
}
void CDxfRead::Setup3DDirectionAttribute(eDXFGroupCode_t x_record_type, Base::Vector3d& destination)
{
    SetupValueAttribute((eDXFGroupCode_t)(x_record_type + eXOffset), destination.x);
    SetupValueAttribute((eDXFGroupCode_t)(x_record_type + eYOffset), destination.y);
    SetupValueAttribute((eDXFGroupCode_t)(x_record_type + eZOffset), destination.z);
}
void CDxfRead::SetupStringAttribute(eDXFGroupCode_t x_record_type, std::string& destination)
{
    m_coordinate_attributes.emplace(x_record_type, std::pair(&ProcessStdString, &destination));
}
template<typename T>
void CDxfRead::SetupValueAttribute(eDXFGroupCode_t record_type, T& destination)
{
    m_coordinate_attributes.emplace(record_type, std::pair(&ProcessValue<T>, &destination));
}

//
// Static processing helpers for ProcessCommonEntityAttribute
void CDxfRead::ProcessScaledDouble(CDxfRead* object, void* target)
{
    std::istringstream ss;
    ss.imbue(std::locale("C"));

    ss.str(object->m_record_data);
    double value = 0;
    ss >> value;
    if (ss.fail()) {
        object->ImportError("Unable to parse value '%s', using zero as its value\n",
                            object->m_record_data);
    }
    *static_cast<double*>(target) = object->mm(value);
}
void CDxfRead::ProcessScaledDoubleIntoList(CDxfRead* object, void* target)
{
    std::istringstream ss;
    ss.imbue(std::locale("C"));

    ss.str(object->m_record_data);
    double value = 0;
    ss >> value;
    if (ss.fail()) {
        object->ImportError("Unable to parse value '%s', using zero as its value\n",
                            object->m_record_data);
    }
    static_cast<std::list<double>*>(target)->push_back(object->mm(value));
}
template<typename T>
bool CDxfRead::ParseValue(CDxfRead* object, void* target)
{
    std::istringstream ss;
    ss.imbue(std::locale("C"));

    ss.str(object->m_record_data);
    ss >> *static_cast<T*>(target);
    if (ss.fail()) {
        object->ImportError("Unable to parse value '%s', using zero as its value\n",
                            object->m_record_data);
        *static_cast<T*>(target) = 0;
        return false;
    }
    // TODO: Verify nothing it left but whitespace in ss.
    return true;
}
void CDxfRead::ProcessStdString(CDxfRead* object, void* target)
{
    *static_cast<std::string*>(target) = object->m_record_data;
}

void CDxfRead::InitializeAttributes()
{
    m_coordinate_attributes.clear();
}
bool CDxfRead::ProcessAttribute()
{
    auto found = m_coordinate_attributes.find(m_record_type);
    if (found != m_coordinate_attributes.end()) {
        (*found->second.first)(this, found->second.second);
        return true;
    }
    return false;
}
void CDxfRead::ProcessAllAttributes()
{
    while (get_next_record() && m_record_type != eObjectType) {
        ProcessAttribute();
    }
    repeat_last_record();
}
void CDxfRead::ProcessAllEntityAttributes()
{
    ProcessAllAttributes();
    ResolveEntityAttributes();
}
void CDxfRead::ResolveEntityAttributes()
{
    m_entityAttributes.ResolveBylayerAttributes(*this);
    // TODO: Look at the space and layer (hidden/frozen?) and options and return false if the entity
    // is not needed.
    // TODO: INSERT must not call this because an INSERT on a hidden layer should always be
    // honoured.

    // Calculate the net entity transformation.

    // This is to handle the Object Coordinate System used in many DXF records. Note that versions
    // before R13 used the term ECS (Entity Coordinate System) instead. Here's who uses OCS: Lines
    // and Points use WCS except they can be extruded (have nonzero Thickness (39)) which occurs in
    // the OCS Z direction all 3D objects use WCS entirely Dimensions use a mix of OCS and WCS,
    // Circle, Arc, Dolid, Trace, Text, Attib, Attdef, Shape, Insert, (lw)Polyline/Vertex, hatch,
    // image all use the OCS
    //
    // The transformed Z axis is in EntityNormalVector, but we rescale it in case the DXF contains
    // an unnormalized value
    if (EntityNormalVector.IsNull()) {
        ImportError("Entity has zero-length extrusion direction\n");
    }
    EntityNormalVector.Normalize();
    // Apply the Arbitrary Axis Algorithm to determine the X and Y directions
    // The purpose of this algorithm is to calculate a conventional 3d orientation based only on a Z
    // direction, while avoiding taking the cross product of two vectors that are nearly parallel,
    // which would be subject to a lot of numerical inaccuracy. In this case, "close to" the Z axis
    // means the X and Y components of EntityNormalVector are less than 1/64, a value chosen because
    // it is exactly representable in all binary floating-point systems.
    Base::Vector3d xDirection;
    if (EntityNormalVector.x < ArbitraryAxisAlgorithmThreshold
        && EntityNormalVector.y < ArbitraryAxisAlgorithmThreshold) {
        // The Z axis is close to the UCS Z axis, the X direction is UCSY × OCSZ
        static const Base::Vector3d UCSYAxis(0, 1, 0);
        xDirection = UCSYAxis % EntityNormalVector;
    }
    else {
        // otherwise, the X direction is UCSZ × OCSZ
        static const Base::Vector3d UCSZAxis(0, 0, 1);
        xDirection = UCSZAxis % EntityNormalVector;
    }
    OCSOrientationTransform.setCol(0, xDirection);
    // In all cases the Y direction is the Zdirection × XDirection which gives a right-hand
    // orthonormal coordinate system
    OCSOrientationTransform.setCol(1, EntityNormalVector % xDirection);
    // and EntityNormalVector is of course the direction of the Z axis in the UCS.
    OCSOrientationTransform.setCol(2, EntityNormalVector);
}

//
//  The individual Entity reader functions
//  These return false if they catch an exception and ignore it because of ignore_errors.
bool CDxfRead::ReadLine()
{
    Base::Vector3d start;
    Base::Vector3d end;
    Setup3DVectorAttribute(ePrimaryPoint, start);
    Setup3DVectorAttribute(ePoint2, end);
    ProcessAllEntityAttributes();

    OnReadLine(start, end, LineTypeIsHidden());
    return true;
}

bool CDxfRead::ReadPoint()
{
    Base::Vector3d location;
    Setup3DVectorAttribute(ePrimaryPoint, location);
    ProcessAllEntityAttributes();

    OnReadPoint(location);
    return true;
}

bool CDxfRead::ReadArc()
{
    double start_angle_degrees = 0;
    double end_angle_degrees = 0;
    double radius = 0;
    Base::Vector3d centre;
    Base::Vector3d extrusionDirection(0, 0, 1);

    Setup3DVectorAttribute(ePrimaryPoint, centre);
    SetupScaledDoubleAttribute(eFloat1, radius);
    SetupValueAttribute(eAngleDegrees1, start_angle_degrees);
    SetupValueAttribute(eAngleDegrees2, end_angle_degrees);
    Setup3DVectorAttribute(eExtrusionDirection, extrusionDirection);
    ProcessAllEntityAttributes();

    OnReadArc(start_angle_degrees,
              end_angle_degrees,
              radius,
              centre,
              extrusionDirection.z,
              LineTypeIsHidden());
    return true;
}

bool CDxfRead::ReadSpline()
{
    struct SplineData sd;
    sd.degree = 0;
    sd.knots = 0;
    sd.flag = 0;
    sd.control_points = 0;
    sd.fit_points = 0;

    Setup3DVectorAttribute(eExtrusionDirection, sd.norm);
    SetupValueAttribute(eInteger1, sd.flag);
    SetupValueAttribute(eInteger2, sd.degree);
    SetupValueAttribute(eInteger3, sd.knots);
    SetupValueAttribute(eInteger4, sd.control_points);
    SetupValueAttribute(eInteger5, sd.fit_points);
    SetupScaledDoubleIntoList(eFloat1, sd.knot);
    SetupScaledDoubleIntoList(eFloat2, sd.weight);
    Setup3DCoordinatesIntoLists(ePrimaryPoint, sd.controlx, sd.controly, sd.controlz);
    Setup3DCoordinatesIntoLists(ePoint2, sd.fitx, sd.fity, sd.fitz);
    Setup3DCoordinatesIntoLists(ePoint3, sd.starttanx, sd.starttany, sd.starttanz);
    Setup3DCoordinatesIntoLists(ePoint4, sd.endtanx, sd.endtany, sd.endtanz);
    ProcessAllEntityAttributes();

    OnReadSpline(sd);
    return true;
}

bool CDxfRead::ReadCircle()
{
    double radius = 0.0;
    Base::Vector3d centre;

    Setup3DVectorAttribute(ePrimaryPoint, centre);
    SetupScaledDoubleAttribute(eFloat1, radius);
    ProcessAllEntityAttributes();

    OnReadCircle(centre, radius, LineTypeIsHidden());
    return true;
}

bool CDxfRead::ReadText()
{
    Base::Vector3d insertionPoint;
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    double height = 0.03082;
    double rotation = 0;
    std::string textPrefix;

    Setup3DVectorAttribute(ePrimaryPoint, insertionPoint);
    SetupScaledDoubleAttribute(eFloat1, height);
    SetupValueAttribute(eAngleDegrees1, rotation);
    while (get_next_record() && m_record_type != eObjectType) {
        if (!ProcessAttribute()) {
            switch (m_record_type) {
                case eExtraText:
                    // Additional text that goes before the type 1 text
                    // Note that if breaking the text into type-3 records splits a UFT-8 encoding we
                    // do the decoding after splicing the lines together. I'm not sure if this
                    // actually occurs, but handling the text this way will treat this condition
                    // properly.
                case ePrimaryText:
                    // final text is treated the same.
                    // ORDER: We are asusming the type 1 record follows all the type 3's.
                    textPrefix.append(m_record_data);
                    break;
                default:
                    break;
            }
        }
    }
    ResolveEntityAttributes();

    if ((this->*stringToUTF8)(textPrefix)) {
        OnReadText(insertionPoint, height * 25.4 / 72.0, textPrefix, rotation);
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    }
    else {
        ImportError("Unable to process encoding for TEXT/MTEXT '%s'\n", textPrefix);
    }
    repeat_last_record();
    return true;
}

bool CDxfRead::ReadEllipse()
{
    Base::Vector3d centre;
    Base::Vector3d majorAxisEnd;  //  relative to centre
    double eccentricity = 0;
    double startAngleRadians = 0;
    double endAngleRadians = 2 * M_PI;

    Setup3DVectorAttribute(ePrimaryPoint, centre);
    Setup3DVectorAttribute(ePoint2, majorAxisEnd);
    SetupValueAttribute(eFloat1, eccentricity);
    SetupValueAttribute(eFloat2, startAngleRadians);
    SetupValueAttribute(eFloat3, endAngleRadians);
    ProcessAllEntityAttributes();

    OnReadEllipse(centre, majorAxisEnd, eccentricity, startAngleRadians, endAngleRadians);
    return true;
}

bool CDxfRead::ReadLwPolyLine()
{
    VertexInfo currentVertex;
    list<VertexInfo> vertices;
    int flags = 0;

    bool have_x = false;
    bool have_y = false;

    // The documentation for LZPOLYLINE does not specify how you know if you have a new vertex.
    // It also does not specify where the bulge, line-width, etc for a particular segment are placed
    // relative to the two end vertices.
    // We assume here that if we see an X or Y coordinate but we already have the same coordinate,
    // a new vertex is starting, and any previous vertex is completely specified. Furthermore, line
    // attributes like bulge are placed between the X/Y coordinates for the vertex that starts the
    // stroke and the X/Y coordinates for the vertex that ends the stroke or the end of the entity.
    // In the latter case the stroke attributes apply to the closure stroke (if any) which ends at
    // the first vertex.
    Setup3DVectorAttribute(ePrimaryPoint, currentVertex.location);
    SetupValueAttribute(eFloat3, currentVertex.bulge);
    SetupValueAttribute(eInteger1, flags);
    while (get_next_record() && m_record_type != eObjectType) {
        if ((m_record_type == ePrimaryPoint + eXOffset && have_x)
            || (m_record_type == ePrimaryPoint + eYOffset && have_y)) {
            // Starting a new vertex and there is a previous vertex. Save it and init a new one.
            vertices.push_back(currentVertex);
            currentVertex.location = Base::Vector3d();
            currentVertex.bulge = 0.0;
            have_x = m_record_type == ePrimaryPoint + eXOffset;
            have_y = m_record_type == ePrimaryPoint + eYOffset;
        }
        else if (m_record_type == ePrimaryPoint + eXOffset) {
            have_x = true;
        }
        else if (m_record_type == ePrimaryPoint + eYOffset) {
            have_y = true;
        }
        ProcessAttribute();
    }
    // At the end of the entity if we have vertex information use this as the final vertex.
    // (else it was a line with no vertices at all)
    if (have_x || have_y) {
        vertices.push_back(currentVertex);
    }

    ResolveEntityAttributes();

    OnReadPolyline(vertices, flags);
    repeat_last_record();
    return true;
}

bool CDxfRead::ReadPolyLine()
{
    VertexInfo currentVertex;
    list<VertexInfo> vertices;
    int flags = 0;

    SetupValueAttribute(eInteger1, flags);
    ProcessAllEntityAttributes();

    // We are now followed by a series of VERTEX entities followed by ENDSEQ.
    // To avoid eating and discarding the rest of the entieies if ENDSEQ is missing,
    // we quit on any unknown type-0 record.
    Setup3DVectorAttribute(ePrimaryPoint, currentVertex.location);
    SetupValueAttribute(eFloat3, currentVertex.bulge);
    while (get_next_record() && m_record_type == eObjectType && IsObjectName("VERTEX")) {
        // Set vertex defaults
        currentVertex.location = Base::Vector3d();
        currentVertex.bulge = 0.0;
        ProcessAllEntityAttributes();
        vertices.push_back(currentVertex);
    }
    if (!IsObjectName("SEQEND")) {
        ImportError("POLYLINE ends with '%s' record rather than 'SEQEND'\n", m_record_data);
        repeat_last_record();
    }

    OnReadPolyline(vertices, flags);
    return true;
}

bool CDxfRead::ReadInsert()
{
    Base::Vector3d center;
    Base::Vector3d scale(1, 1, 1);
    double rotationDegrees = 0.0;
    std::string blockName;

    Setup3DVectorAttribute(ePrimaryPoint, center);
    SetupValueAttribute(eFloat2, scale.x);
    SetupValueAttribute(eFloat3, scale.y);
    SetupValueAttribute(eFloat4, scale.z);
    SetupValueAttribute(eAngleDegrees1, rotationDegrees);
    SetupStringAttribute(eName, blockName);
    ProcessAllEntityAttributes();
    OnReadInsert(center, scale, blockName, Base::toRadians(rotationDegrees));
    return (true);
}

bool CDxfRead::ReadDimension()
{
    Base::Vector3d start;
    Base::Vector3d end;
    Base::Vector3d linePosition;
    Base::Vector3d textPosition;
    double rotation = 0;
    int dimensionType = 0;  //  This appears to default to zero

    // Per documentation:
    // 10 is the "dimension line location"
    // 11 is the midpoint of the dimension text
    // 13 is the start point of the 1st extension line
    // 14 is the start point of the 2nd extension line
    // 50 is the rotation of the dimension (the direction of the dimension line)
    // 52 (if present) is the angle relative to the dimension line for the extension lines; default
    // 90 degrees
    Setup3DVectorAttribute(ePoint4, start);               // WCS
    Setup3DVectorAttribute(ePoint5, end);                 // WCS
    Setup3DVectorAttribute(ePrimaryPoint, linePosition);  // WCS
    Setup3DVectorAttribute(ePoint2, textPosition);        // OCS
    SetupValueAttribute(eAngleDegrees1, rotation);
    SetupValueAttribute(eInteger1, dimensionType);
    ProcessAllEntityAttributes();

    dimensionType &= eTypeMask;  //  Remove flags
    switch ((eDimensionType_t)dimensionType) {
        case eLinear:
        case eAligned:
            OnReadDimension(start, end, linePosition, Base::toRadians(rotation));
            break;
        default:
            UnsupportedFeature("Dimension type '%d'", dimensionType);
            break;
    }
    return true;
}

bool CDxfRead::ReadUnknownEntity()
{
    UnsupportedFeature("Entity type '%s'", m_record_data);
    ProcessAllEntityAttributes();
    return true;
}

bool CDxfRead::ReadBlockInfo()
{
    int blockType = 0;
    std::string blockName;
    InitializeAttributes();
    // Both 2 and 3 are the block name.
    SetupStringAttribute(eName, blockName);
    SetupStringAttribute(eExtraText, blockName);
    SetupValueAttribute(eInteger1, blockType);
    ProcessAllAttributes();

    return OnReadBlock(blockName, blockType);
}
bool CDxfRead::ReadBlockContents()
{
    while (get_next_record() && m_record_type == eObjectType && !IsObjectName("ENDBLK")) {
        if (IgnoreErrors()) {
            try {
                if (!ReadEntity()) {
                    return false;
                }
            }
            catch (...) {
            }
        }
        else {
            if (!ReadEntity()) {
                return false;
            }
        }
    }
    return true;
}
bool CDxfRead::SkipBlockContents()
{
    while (get_next_record() && m_record_type == eObjectType && !IsObjectName("ENDBLK")) {
        if (IgnoreErrors()) {
            try {
                ProcessAllAttributes();
            }
            catch (...) {
            }
        }
        else {
            ProcessAllAttributes();
        }
    }
    return true;
}

template<typename... args>
void CDxfRead::UnsupportedFeature(const char* format, args&&... argValuess)
{
    // NOLINTNEXTLINE(runtime/printf)
    std::string formattedMessage = fmt::sprintf(format, std::forward<args>(argValuess)...);
    // We place these formatted messages in a map, count their occurrences and not their first
    // occurrence.
    if (m_unsupportedFeaturesNoted[formattedMessage].first++ == 0) {
        m_unsupportedFeaturesNoted[formattedMessage].second = m_line;
    }
}

bool CDxfRead::get_next_record()
{
    if (m_repeat_last_record) {
        m_repeat_last_record = false;
        return m_not_eof;
    }

    do {
        if ((*m_ifs).eof()) {
            m_not_eof = false;
            return false;
        }

        std::getline(*m_ifs, m_record_data);
        ++m_line;
        int temp = 0;
        if (!ParseValue<int>(this, &temp)) {
            ImportError("CDxfRead::get_next_record() Failed to get integer record type from '%s'\n",
                        m_record_data);
            return false;
        }
        m_record_type = (eDXFGroupCode_t)temp;
        if ((*m_ifs).eof()) {
            return false;
        }

        std::getline(*m_ifs, m_record_data);
        ++m_line;
    } while (m_record_type == eComment);

    // Remove any carriage return at the end of m_str which may occur because of inconsistent
    // handling of LF vs. CRLF line termination.
    auto last = m_record_data.rbegin();
    if (last != m_record_data.rend() && *last == '\r') {
        m_record_data.pop_back();
    }
    // The code that was here just blindly trimmed leading white space, but if you have, for
    // instance, a TEXT entity whose text starts with spaces, or, more plausibly, a long TEXT entity
    // where the text is broken into one or more type-3 records with a final type-1 and the break
    // happens to be just before a space, this would be wrong.
    return true;
}

void CDxfRead::repeat_last_record()
{
    m_repeat_last_record = true;
}

//
//  Intercepts for On... calls to derived class
//  (These have distinct signatures from the ones they call)
bool CDxfRead::ExplodePolyline(std::list<VertexInfo>& vertices, int flags)
{
    if (vertices.size() < 2) {
        // TODO: Warning
        return true;
    }

    bool closed = ((flags & 1) != 0);
    auto startVertex = vertices.end();
    if (closed) {
        // If the shape is closed, point at the last vertex. The first stroke drawn will be the
        // closure.
        --startVertex;
    }
    for (auto endVertex = vertices.begin(); endVertex != vertices.end(); endVertex++) {
        if (startVertex != vertices.end()) {
            if (startVertex->bulge != 0.0) {
                // Bulge is 1/4 tan(arc angle), positive for CCW arc.
                double cot = ((1.0 / startVertex->bulge) - startVertex->bulge) / 2;
                double cx = ((startVertex->location.x + endVertex->location.x)
                             - ((endVertex->location.y - startVertex->location.y) * cot))
                    / 2;
                double cy = ((startVertex->location.y + endVertex->location.y)
                             + ((endVertex->location.x - startVertex->location.x) * cot))
                    / 2;
                Base::Vector3d pc(cx, cy, (startVertex->location.z + endVertex->location.z) / 2);
                OnReadArc(startVertex->location,
                          endVertex->location,
                          pc,
                          startVertex->bulge >= 0,
                          false);
            }
            else {
                OnReadLine(startVertex->location, endVertex->location, false);
            }
        }
        // elsethis is the first loop iteration on an open shape, endVertex is the first point, and
        // there is no closure line to draw engin there.
        startVertex = endVertex;
    }
    return true;
}
void CDxfRead::OnReadArc(double start_angle,
                         double end_angle,
                         double radius,
                         const Base::Vector3d& center,
                         double z_extrusion_dir,
                         bool hidden)
{
    Base::Vector3d temp(center);
    // Calculate the start and end points of the arc
    Base::Vector3d start(center);
    start.x += radius * cos(Base::toRadians(start_angle));
    start.y += radius * sin(Base::toRadians(start_angle));
    Base::Vector3d end(center);
    end.x += radius * cos(Base::toRadians(end_angle));
    end.y += radius * sin(Base::toRadians(end_angle));
    if (z_extrusion_dir < 0) {
        // This is a dumbed-down handling of general OCS. This only works
        // for arcs drawn exactly upside down (i.e. with the extrusion vector
        // being (0, 0, <0).
        // We treat this as 180-degree mirroring through the YZ plane
        // TODO: I don't even think this is correct, but it is functionally what the
        // code did before.
        temp.x = -temp.x;
        start.x = -start.x;
        end.x = -end.x;
    }
    OnReadArc(start, end, temp, true, hidden);
}

void CDxfRead::OnReadCircle(const Base::Vector3d& center, double radius, bool hidden)
{
    // OnReadCircle wants a start point, so we pick an arbitrary point on the circumference
    Base::Vector3d start(center);
    start.x += radius;

    OnReadCircle(start,
                 center,
                 false,
                 hidden);  // false to change direction because otherwise the arc length is zero
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
void CDxfRead::OnReadEllipse(const Base::Vector3d& center,
                             const Base::Vector3d& majorAxisEnd,
                             double ratio,
                             double start_angle,
                             double end_angle)
// NOLINTEND(bugprone-easily-swappable-parameters)
{
    double major_radius = majorAxisEnd.Length();
    double minor_radius = major_radius * ratio;

    // Since we only support 2d stuff, we can calculate the rotation from the major axis x and y
    // value only, since z is zero, major_radius is the vector length

    double rotation = atan2(majorAxisEnd.y, majorAxisEnd.x);


    OnReadEllipse(center, major_radius, minor_radius, rotation, start_angle, end_angle, true);
}

bool CDxfRead::ReadVersion()
{
    static const std::vector<std::string> VersionNames = {
        // This table is indexed by eDXFVersion_t - (ROlder+1)
        "AC1006",
        "AC1009",
        "AC1012",
        "AC1014",
        "AC1015",
        "AC1018",
        "AC1021",
        "AC1024",
        "AC1027",
        "AC1032"};

    assert(VersionNames.size() == RNewer - ROlder - 1);
    get_next_record();  // Get the value for the variable
    auto first = VersionNames.cbegin();
    auto last = VersionNames.cend();
    auto found = std::lower_bound(first, last, m_record_data);
    if (found == last) {
        m_version = RNewer;
    }
    else if (*found == m_record_data) {
        m_version = (eDXFVersion_t)((int)std::distance(first, found) + (ROlder + 1));
    }
    else if (found == first) {
        m_version = ROlder;
    }
    else {
        m_version = RUnknown;
    }

    return ResolveEncoding();
}

bool CDxfRead::ReadDWGCodePage()
{
    get_next_record();           // Get the value for the variable
    assert(m_CodePage.empty());  // If not, we have found two DWGCODEPAGE variables or DoRead
                                 // was called twice on the same CDxfRead object.
    m_CodePage = m_record_data;

    return ResolveEncoding();
}

bool CDxfRead::ResolveEncoding()
{
    if (m_version >= R2007) {  // Note this does not include RUnknown, but does include RLater
        m_encoding = "utf_8";
        stringToUTF8 = &CDxfRead::UTF8ToUTF8;
    }
    else if (m_CodePage.empty()) {
        // cp1252
        m_encoding = "cp1252";
        stringToUTF8 = &CDxfRead::GeneralToUTF8;
    }
    else {
        // Codepage names may be of the form "ansi_1252" which we map to "cp1252" but we don't map
        // "ansi_x3xxxx" (which happens to mean "ascii")
        // Also some DXF files have the codepage name in uppercase so we lowercase it.
        m_encoding = m_CodePage;
        std::transform(m_encoding.begin(), m_encoding.end(), m_encoding.begin(), ::tolower);
        // NOLINTNEXTLINE(readability/nolint)
#define ANSI_ENCODING_PREFIX "ansi_"  // NOLINT(cppcoreguidelines-macro-usage)
        if (m_encoding.rfind(ANSI_ENCODING_PREFIX, 0) == 0 && m_encoding.rfind("ansi_x3", 0) != 0) {
            m_encoding.replace(0, (sizeof ANSI_ENCODING_PREFIX) - 1, "cp");
        }
        // At this point we want to recognize synonyms for "utf_8" and use the custom decoder
        // function. This is because this is one of the common cases and our decoder function is a
        // fast no-op. We don't actually use the decoder function we get from PyCodec_Decoder
        // because to call it we have to convert the (char *) text into a 'bytes' object first so we
        // can pass it to the function using PyObject_Callxxx(), getting the PYObject containing the
        // Python string, which we then decode back to UTF-8. It is simpler to call
        // PyUnicode_DecodeXxxx which takes a (const char *) and is just a direct c++ callable.
        Base::PyGILStateLocker lock;
        PyObject* pyDecoder = PyCodec_Decoder(m_encoding.c_str());
        if (pyDecoder == nullptr) {
            return false;  // A key error exception will have been placed.
        }
        PyObject* pyUTF8Decoder = PyCodec_Decoder("utf_8");
        assert(pyUTF8Decoder != nullptr);
        if (pyDecoder == pyUTF8Decoder) {
            stringToUTF8 = &CDxfRead::UTF8ToUTF8;
        }
        else {
            stringToUTF8 = &CDxfRead::GeneralToUTF8;
        }
        Py_DECREF(pyDecoder);
        Py_DECREF(pyUTF8Decoder);
    }
    return !m_encoding.empty();
}

// NOLINTNEXTLINE(readability/nolint)
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool CDxfRead::UTF8ToUTF8(std::string& /*encoded*/) const
{
    return true;
}

bool CDxfRead::GeneralToUTF8(std::string& encoded) const
{
    Base::PyGILStateLocker lock;
    PyObject* decoded = PyUnicode_Decode(encoded.c_str(),
                                         (Py_ssize_t)encoded.length(),
                                         m_encoding.c_str(),
                                         "strict");
    if (decoded == nullptr) {
        return false;
    }
    const char* converted = PyUnicode_AsUTF8(decoded);
    // converted has the same lifetime as decoded so we don't have to delete it.
    if (converted != nullptr) {
        encoded = converted;
    }
    Py_DECREF(decoded);
    return converted != nullptr;
}

void CDxfRead::DoRead(const bool ignore_errors /* = false */)
{
    m_ignore_errors = ignore_errors;
    if (m_fail) {
        return;
    }

    StartImport();
    // Loop reading the sections.
    while (get_next_record()) {
        if (m_record_type != eObjectType) {
            ImportError("Found type %d record when expecting start of a SECTION or EOF\n",
                        (int)m_record_type);
            continue;
        }
        if (IsObjectName("EOF")) {  // TODO: Check for drivel beyond EOF record
            break;
        }
        if (!IsObjectName("SECTION")) {
            ImportError("Found %s record when expecting start of a SECTION\n",
                        m_record_data.c_str());
            continue;
        }
        if (!ReadSection()) {
            return;
        }
    }
    FinishImport();

    // FLush out any unsupported features messages
    if (!m_unsupportedFeaturesNoted.empty()) {
        ImportError("Unsupported DXF features:\n");
        for (auto& featureInfo : m_unsupportedFeaturesNoted) {
            ImportError("%s: %d time(s) first at line %d\n",
                        featureInfo.first,
                        featureInfo.second.first,
                        featureInfo.second.second);
        }
    }
}

bool CDxfRead::ReadSection()
{
    if (!get_next_record()) {
        ImportError("Unclosed SECTION at end of file\n");
        return false;
    }
    if (m_record_type != eName) {
        ImportError("Ignored SECTION with no name record\n");
        return ReadIgnoredSection();
    }
    if (IsObjectName("HEADER")) {
        return ReadHeaderSection();
    }
    if (IsObjectName("TABLES")) {
        return ReadTablesSection();
    }
    if (IsObjectName("BLOCKS")) {
        return ReadBlocksSection();
    }
    if (IsObjectName("ENTITIES")) {
        return ReadEntitiesSection();
    }
    return ReadIgnoredSection();
}
void CDxfRead::ProcessLayerReference(CDxfRead* object, void* target)
{
    if (object->Layers.count(object->m_record_data) == 0) {
        object->ImportError("First reference to missing Layer '%s'", object->m_record_data);
        // Synthesize the Layer so we don't get the same error again.
        // We need to take copies of the string arguments because MakeLayer uses them as move
        // inputs.
        object->Layers[object->m_record_data] =
            object->MakeLayer(object->m_record_data, DefaultColor, std::string(DefaultLineType));
    }
    *static_cast<Layer**>(target) = object->Layers.at(object->m_record_data);
}
bool CDxfRead::ReadEntity()
{
    InitializeAttributes();
    m_entityAttributes.SetDefaults();
    EntityNormalVector.Set(0, 0, 1);
    Setup3DVectorAttribute(eExtrusionDirection, EntityNormalVector);
    SetupStringAttribute(eLinetypeName, m_entityAttributes.m_LineType);
    m_coordinate_attributes.emplace(eLayerName,
                                    std::pair(&ProcessLayerReference, &m_entityAttributes.m_Layer));
    SetupValueAttribute(
        eCoordinateSpace,
        m_entityAttributes.m_paperSpace);  // TODO: Ensure the stream is noboolalpha (for that
                                           // matter ensure the stream has the "C" locale
    SetupValueAttribute(eColor, m_entityAttributes.m_Color);
    // The entity record is already the current record and is already checked as a type 0 record
    if (IsObjectName("LINE")) {
        return ReadLine();
    }
    if (IsObjectName("ARC")) {
        return ReadArc();
    }
    if (IsObjectName("CIRCLE")) {
        return ReadCircle();
    }
    if (IsObjectName("MTEXT")) {
        return ReadText();
    }
    if (IsObjectName("TEXT")) {
        return ReadText();
    }
    if (IsObjectName("ELLIPSE")) {
        return ReadEllipse();
    }
    if (IsObjectName("SPLINE")) {
        return ReadSpline();
    }
    if (IsObjectName("LWPOLYLINE")) {
        return ReadLwPolyLine();
    }
    if (IsObjectName("POLYLINE")) {
        return ReadPolyLine();
    }
    if (IsObjectName("POINT")) {
        return ReadPoint();
    }
    if (IsObjectName("INSERT")) {
        return ReadInsert();
    }
    if (IsObjectName("DIMENSION")) {
        return ReadDimension();
    }
    return ReadUnknownEntity();
}

bool CDxfRead::ReadHeaderSection()
{
    // Read to the next ENDSEC record marking the end of the section.
    // This section contains variables, most of which we ignore. Each one is a type-9 record giving
    // the variable name, followed by a single record giving the value; the record type depends on
    // the variable's data type.
    while (get_next_record()) {
        if (m_record_type == eObjectType && IsObjectName("ENDSEC")) {
            if (m_unitScalingFactor == 0.0) {
                // Neither INSUNITS nor MEASUREMENT found, assume 1 DXF unit = 1mm
                // TODO: Perhaps this default should depend on the current measuring units of the
                // app.
                m_unitScalingFactor = m_additionalScaling;
                ImportObservation("No INSUNITS or MEASUREMENT; setting scaling to 1 DXF unit = "
                                  "%gmm based on DXF scaling option\n",
                                  m_unitScalingFactor);
            }
            return true;
        }
        if (m_record_type != eVariableName) {
            continue;  // Quietly ignore unknown record types
        }
        if (!ReadVariable()) {
            return false;
        }
    }
    return false;
}

bool CDxfRead::ReadVariable()
{
    if (IsVariableName("$INSUNITS")) {
        get_next_record();  // Get the value for the variable
        int varValue = 0;
        if (!ParseValue<int>(this, &varValue)) {
            ImportError("Failed to get integer from INSUNITS value '%s'\n", m_record_data);
        }
        else if (auto units = DxfUnits::eDxfUnits_t(varValue); !DxfUnits::IsValid(units)) {
            ImportError("Unknown value '%d' for INSUNITS\n", varValue);
        }
        else {
            m_unitScalingFactor = DxfUnits::Factor(units) * m_additionalScaling;
            ImportObservation("Setting scaling to 1 DXF unit = %gmm based on INSUNITS and "
                              "DXF scaling option\n",
                              m_unitScalingFactor);
        }
        return true;
    }
    if (IsVariableName("$MEASUREMENT")) {
        get_next_record();
        int varValue = 1;
        if (m_unitScalingFactor == 0.0 && ParseValue<int>(this, &varValue)) {
            m_unitScalingFactor =
                DxfUnits::Factor(varValue != 0 ? DxfUnits::eMillimeters : DxfUnits::eInches)
                * m_additionalScaling;
            ImportObservation("Setting scaling to 1 DXF unit = %gmm based on MEASUREMENT and "
                              "DXF scaling option\n",
                              m_unitScalingFactor);
        }
        return true;
    }
    if (IsVariableName("$ACADVER")) {
        return ReadVersion();
    }
    if (IsVariableName("$DWGCODEPAGE")) {
        return ReadDWGCodePage();
    }
    // any other variable, skip its value
    return get_next_record();
}

bool CDxfRead::ReadTablesSection()
{
    // Read to the next ENDSEC record marking the end of the section.
    // This section contains various tables, many of which we ignore. Each one is a type-0 TABLE
    // record followed by a type-2 (name) record giving the table name, followed by the table
    // contents. Each set of contents is terminates by the next type-0 TABLE or ENDSEC directive.
    while (get_next_record()) {
        if (m_record_type != eObjectType) {
            continue;  // Ignore any non-type-0 contents in the section.
        }
        if (IsObjectName("ENDSEC")) {
            return true;
        }
        if (!IsObjectName("TABLE")) {
            continue;  // Ignore any type-0 non-TABLE contents in the section
        }
        get_next_record();
        if (m_record_type != eName) {
            ImportError("Found unexpected type %d record instead of table name\n",
                        (int)m_record_type);
        }
        else if (IsObjectName("LAYER")) {
            if (!ReadLayerTable()) {
                return false;
            }
        }
        else {
            if (!ReadIgnoredTable()) {
                return false;
            }
        }
    }
    return false;
}

bool CDxfRead::ReadIgnoredSection()
{
    // Read to the next ENDSEC record marking the end of the section.
    while (get_next_record()) {
        if (m_record_type == eObjectType && IsObjectName("ENDSEC")) {
            return true;
        }
    }
    return false;
}

bool CDxfRead::ReadBlocksSection()
{
    // Read to the next ENDSEC record marking the end of the section.
    // Within this section we should find type-0 BLOCK groups
    while (get_next_record()) {
        if (m_record_type != eObjectType) {
            continue;  // quietly ignore non-type-0 records;
        }
        if (IsObjectName("ENDSEC")) {
            // End of section
            return true;
        }
        if (!IsObjectName("BLOCK")) {
            continue;  // quietly ignore non-BLOCK records
        }
        if (!ReadBlockInfo()) {
            ImportError("CDxfRead::DoRead() Failed to read block\n");
        }
    }

    return false;
}

bool CDxfRead::ReadEntitiesSection()
{
    // Read to the next ENDSEC record marking the end of the section.
    // Within this section we should find type-0 BLOCK groups
    while (get_next_record()) {
        if (m_record_type != eObjectType) {
            continue;  // quietly ignore non-type-0 records;
        }
        if (IsObjectName("ENDSEC")) {
            // End of section
            return true;
        }
        if (IgnoreErrors()) {
            try {
                if (!ReadEntity()) {
                    return false;
                }
            }
            catch (const Base::Exception& e) {
                e.ReportException();
            }
            catch (...) {
                ImportError("CDxfRead::ReadEntity raised unknown exception\n");
            }
        }
        else {
            if (!ReadEntity()) {
                return false;
            }
        }
    }

    return false;
}

bool CDxfRead::ReadLayer()
{
    std::string layername;
    ColorIndex_t layerColor = DefaultColor;
    int layerFlags = 0;
    std::string lineTypeName(DefaultLineType);
    InitializeAttributes();

    SetupStringAttribute(eName, layername);
    SetupValueAttribute(eColor, layerColor);
    SetupValueAttribute(eInteger1, layerFlags);
    SetupStringAttribute(eLinetypeName, lineTypeName);
    ProcessAllAttributes();
    if (layername.empty()) {
        ImportError("CDxfRead::ReadLayer() - no layer name\n");
        return false;
    }
    if ((layerFlags & 0x01) != 0) {
        // Frozen layers are implicitly hidden which we don't do yet.
        // TODO: Should have an import option to omit frozen layers.
        UnsupportedFeature("Frozen layers");
    }
    if (layerColor < 0) {
        UnsupportedFeature("Hidden layers");
    }
    Layers[layername] = MakeLayer(layername, layerColor, std::move(lineTypeName));
    return true;
}
CDxfRead::Layer*
CDxfRead::MakeLayer(const std::string& name, ColorIndex_t color, std::string&& lineType)
{
    return new Layer(name, color, std::move(lineType));
}

bool CDxfRead::ReadLayerTable()
{
    // Read to the next TABLE record indicating another table in the TABLES section, or to the
    // ENDSEC record marking the end of the TABLES section. This table contains a series of type-0
    // LAYER groups
    while (get_next_record()) {
        if (m_record_type != eObjectType) {
            continue;  // quietly ignore non-type-0 records; this table has some preamble
        }
        if (IsObjectName("TABLE") || IsObjectName("ENDSEC")) {
            // End of table
            repeat_last_record();
            return true;
        }
        if (!IsObjectName("LAYER")) {
            continue;  // quietly ignore non-LAYER records
        }
        if (!ReadLayer()) {
            ImportError("CDxfRead::DoRead() Failed to read layer\n");
        }
    }

    return false;
}

bool CDxfRead::ReadIgnoredTable()
{
    // Read to the next TABLE record indicating another table in the TABLES section, or to the
    // ENDSEC record marking the end of the TABLES section.
    while (get_next_record()) {
        if (m_record_type == eObjectType && (IsObjectName("TABLE") || IsObjectName("ENDSEC"))) {
            repeat_last_record();
            return true;
        }
    }
    return false;
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
inline static double level(int distance, double blackLevel)
{
    // Distance is the number of 24ths around the color wheel between the desired hue and
    // the primary hue in question. Coming in it is a ordinate difference and so can be negative
    // so the first thing we do is take its absolute value.
    if (distance < 0) {
        distance = -distance;
    }
    // If the distance is greater than 12, it is measuring the long way around the color wheel so we
    // reduce it to measuring along the short way instead
    if (distance > 12) {
        distance = 24 - distance;
    }
    if (distance <= 4) {
        // A distance 4 or less givs full intensity of the primary color
        return 1.0;
    }
    if (distance < 8) {
        // Between 4 and 8 gives a blend of the full primary and the black level
        return ((8 - distance) + blackLevel * (distance - 4)) / 4;
    }
    // 8 and beyond yield the black level
    return blackLevel;
}
inline static App::Color wheel(int hue, double blackLevel, double multiplier = 1.0)
{
    return App::Color((float)(level(hue - 0, blackLevel) * multiplier),
                      (float)(level(hue - 8, blackLevel) * multiplier),
                      (float)(level(hue - 16, blackLevel) * multiplier));
}
App::Color CDxfRead::ObjectColor(ColorIndex_t index)
{
    // TODO: If it is ColorByBlock we need to use the color of the INSERT entity.
    // This is tricky because a block can itself contain INSERT entities and we don't currently
    // record the required information. IIRC INSERT in a block will do something strange like
    // try to insert the block into the main drawing instead of into the block being defined.

    // The first 7 colors (1-7) have ad hoc names red, yellow, green, cyan, blue, magenta, and
    // black. 8, 9, 250-254 are lightening shades of gray. These are rendered by the app in a manner
    // to contrast with the background color.
    // For others, (color/10) determines the hue around the
    // color circle, with even numbers fading to black on the tens digit,
    // and odd numberd being blended with AA and again fading to black.
    // The fade is FF BD 81 68 4F (100%, 74%, 50%, 40%, 30%) indexed by (index/2)%5
    // The AA fades as AA 7E 56 45 35 which is almost the exact same percentages.
    // For hue, (index-10)/10 : 0 is ff0000, and each step linearly adds green until 4 is pure
    // yellow ffff00, then red starts to fade... until but not including 24 which is back to ff0000.
    App::Color result = App::Color();
    if (index == 0) {
        // Technically, 0 is BYBLOCK and not a real color, but all that means is that an object in a
        // block cannot specifically ask to be black. These colors are all contrasted to the
        // background so there is no objective black colour, through 255 is an objective white.
        result = App::Color();
    }
    else if (index < 7) {
        result = wheel((index - 1) * 4, 0x00);
    }
    else if (index == 7) {
        result = App::Color(1, 1, 1);
    }
    else if (index == 8) {
        result = App::Color(0.5, 0.5, 0.5);
    }
    else if (index == 9) {
        result = App::Color(0.75, 0.75, 0.75);
    }
    else if (index >= 250) {
        auto brightness = (float)((index - 250 + (255 - index) * 0.2) / 5);
        result = App::Color(brightness, brightness, brightness);
    }
    else {
        static const std::array<float, 5> fades = {1.00F, 0.74F, 0.50F, 0.40F, 0.30F};
        return wheel(index / 10 - 1, (index & 1) != 0 ? 0.69 : 0, fades[(index / 2) % 5]);
    }
    // TODO: These colors are modified to contrast with the background. In the original program
    // this is just a rendering feature, but FreeCAD does not support this so the user has the
    // option of modifying the colors to contrast with the background at time of import.
    return result;
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
