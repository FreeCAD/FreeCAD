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
#include <Base/Vector3D.h>


using namespace std;

Base::Vector3d toVector3d(const double* a)
{
    Base::Vector3d result;
    result.x = a[0];
    result.y = a[1];
    result.z = a[2];
    return result;
}

CDxfWrite::CDxfWrite(const char* filepath)
    :  // TODO: these should probably be parameters in config file
       // handles:
       // boilerplate 0 - A00
       // used by dxf.cpp A01 - FFFE
       // ACAD HANDSEED  FFFF

    m_handle(0xA00)
    ,  // room for 2560 handles in boilerplate files
    // m_entityHandle(0x300),               //don't need special ranges for handles
    // m_layerHandle(0x30),
    // m_blockHandle(0x210),
    // m_blkRecordHandle(0x110),
    m_polyOverride(false)
    , m_layerName("none")
{
    // start the file
    m_fail = false;
    m_version = 12;
    Base::FileInfo fi(filepath);
    m_ofs = new Base::ofstream(fi, ios::out);
    m_ssBlock = new std::ostringstream();
    m_ssBlkRecord = new std::ostringstream();
    m_ssEntity = new std::ostringstream();
    m_ssLayer = new std::ostringstream();

    if (!(*m_ofs)) {
        m_fail = true;
        return;
    }
    m_ofs->imbue(std::locale("C"));
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
        ifstream inFile(fi.filePath().c_str());

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

void CDxfWrite::addBlockName(std::string b, std::string h)
{
    m_blockList.push_back(b);
    m_blkRecordList.push_back(h);
}

void CDxfWrite::setLayerName(std::string s)
{
    m_layerName = s;
    m_layerList.push_back(s);
}

void CDxfWrite::writeLine(const double* s, const double* e)
{
    putLine(toVector3d(s), toVector3d(e), m_ssEntity, getEntityHandle(), m_saveModelSpaceHandle);
}

void CDxfWrite::putLine(const Base::Vector3d s,
                        const Base::Vector3d e,
                        std::ostringstream* outStream,
                        const std::string handle,
                        const std::string ownerHandle)
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
    (*outStream) << " 10" << endl;  // Start point of line
    (*outStream) << s.x << endl;    // X in WCS coordinates
    (*outStream) << " 20" << endl;
    (*outStream) << s.y << endl;  // Y in WCS coordinates
    (*outStream) << " 30" << endl;
    (*outStream) << s.z << endl;    // Z in WCS coordinates
    (*outStream) << " 11" << endl;  // End point of line
    (*outStream) << e.x << endl;    // X in WCS coordinates
    (*outStream) << " 21" << endl;
    (*outStream) << e.y << endl;  // Y in WCS coordinates
    (*outStream) << " 31" << endl;
    (*outStream) << e.z << endl;  // Z in WCS coordinates
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
        (*m_ssEntity) << "0.0" << endl;
    }
    (*m_ssEntity) << "  0" << endl;
    (*m_ssEntity) << "SEQEND" << endl;
    (*m_ssEntity) << "  5" << endl;
    (*m_ssEntity) << getEntityHandle() << endl;
    (*m_ssEntity) << "  8" << endl;
    (*m_ssEntity) << getLayerName() << endl;
}

void CDxfWrite::writePoint(const double* s)
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
    (*m_ssEntity) << s[0] << endl;  // X in WCS coordinates
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << s[1] << endl;  // Y in WCS coordinates
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << s[2] << endl;  // Z in WCS coordinates
}

void CDxfWrite::writeArc(const double* s, const double* e, const double* c, bool dir)
{
    double ax = s[0] - c[0];
    double ay = s[1] - c[1];
    double bx = e[0] - c[0];
    double by = e[1] - c[1];

    double start_angle = atan2(ay, ax) * 180 / M_PI;
    double end_angle = atan2(by, bx) * 180 / M_PI;
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
    (*m_ssEntity) << " 10" << endl;  // Centre X
    (*m_ssEntity) << c[0] << endl;   // X in WCS coordinates
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << c[1] << endl;  // Y in WCS coordinates
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << c[2] << endl;    // Z in WCS coordinates
    (*m_ssEntity) << " 40" << endl;   //
    (*m_ssEntity) << radius << endl;  // Radius

    if (m_version > 12) {
        (*m_ssEntity) << "100" << endl;
        (*m_ssEntity) << "AcDbArc" << endl;
    }
    (*m_ssEntity) << " 50" << endl;
    (*m_ssEntity) << start_angle << endl;  // Start angle
    (*m_ssEntity) << " 51" << endl;
    (*m_ssEntity) << end_angle << endl;  // End angle
}

void CDxfWrite::writeCircle(const double* c, double radius)
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
    (*m_ssEntity) << " 10" << endl;  // Centre X
    (*m_ssEntity) << c[0] << endl;   // X in WCS coordinates
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << c[1] << endl;  // Y in WCS coordinates
                                    //    (*m_ssEntity) << " 30"       << endl;
    //    (*m_ssEntity) << c[2]        << endl;    // Z in WCS coordinates
    (*m_ssEntity) << " 40" << endl;   //
    (*m_ssEntity) << radius << endl;  // Radius
}

void CDxfWrite::writeEllipse(const double* c,
                             double major_radius,
                             double minor_radius,
                             double rotation,
                             double start_angle,
                             double end_angle,
                             bool endIsCW)
{
    double m[3] = {major_radius * sin(rotation), major_radius * cos(rotation), 0};
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
    (*m_ssEntity) << " 10" << endl;  // Centre X
    (*m_ssEntity) << c[0] << endl;   // X in WCS coordinates
    (*m_ssEntity) << " 20" << endl;
    (*m_ssEntity) << c[1] << endl;  // Y in WCS coordinates
    (*m_ssEntity) << " 30" << endl;
    (*m_ssEntity) << c[2] << endl;   // Z in WCS coordinates
    (*m_ssEntity) << " 11" << endl;  //
    (*m_ssEntity) << m[0] << endl;   // Major X
    (*m_ssEntity) << " 21" << endl;
    (*m_ssEntity) << m[1] << endl;  // Major Y
    (*m_ssEntity) << " 31" << endl;
    (*m_ssEntity) << m[2] << endl;   // Major Z
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
                        const Base::Vector3d location1,
                        const Base::Vector3d location2,
                        const double height,
                        const int horizJust,
                        std::ostringstream* outStream,
                        const std::string handle,
                        const std::string ownerHandle)
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

void CDxfWrite::putArrow(Base::Vector3d arrowPos,
                         Base::Vector3d barb1Pos,
                         Base::Vector3d barb2Pos,
                         std::ostringstream* outStream,
                         const std::string handle,
                         const std::string ownerHandle)
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
                                    const double* dimLine,
                                    const double* e1Start,
                                    const double* e2Start,
                                    const char* dimText,
                                    int type)
{
    Base::Vector3d e1S(e1Start[0], e1Start[1], e1Start[2]);
    Base::Vector3d e2S(e2Start[0], e2Start[1], e2Start[2]);
    Base::Vector3d dl(dimLine[0], dimLine[1], dimLine[2]);  // point on DimLine (somewhere!)
    Base::Vector3d perp = dl.DistanceToLineSegment(e2S, e1S);
    Base::Vector3d e1E = e1S - perp;
    Base::Vector3d e2E = e2S - perp;
    Base::Vector3d para = e1E - e2E;
    Base::Vector3d X(1.0, 0.0, 0.0);
    double angle = para.GetAngle(X);
    angle = angle * 180.0 / M_PI;
    if (type == ALIGNED) {
        // NOP
    }
    else if (type == HORIZONTAL) {
        double x = e1Start[0];
        double y = dimLine[1];
        e1E = Base::Vector3d(x, y, 0.0);
        x = e2Start[0];
        e2E = Base::Vector3d(x, y, 0.0);
        perp = Base::Vector3d(0, -1, 0);  // down
        para = Base::Vector3d(1, 0, 0);   // right
        if (dimLine[1] > e1Start[1]) {
            perp = Base::Vector3d(0, 1, 0);  // up
        }
        if (e1Start[0] > e2Start[0]) {
            para = Base::Vector3d(-1, 0, 0);  // left
        }
        angle = 0;
    }
    else if (type == VERTICAL) {
        double x = dimLine[0];
        double y = e1Start[1];
        e1E = Base::Vector3d(x, y, 0.0);
        y = e2Start[1];
        e2E = Base::Vector3d(x, y, 0.0);
        perp = Base::Vector3d(1, 0, 0);
        para = Base::Vector3d(0, 1, 0);
        if (dimLine[0] < e1Start[0]) {
            perp = Base::Vector3d(-1, 0, 0);
        }
        if (e1Start[1] > e2Start[1]) {
            para = Base::Vector3d(0, -1, 0);
        }
        angle = 90;
    }

    double arrowLen = 5.0;                     // magic number
    double arrowWidth = arrowLen / 6.0 / 2.0;  // magic number calc!

    putLine(e2S, e2E, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);

    putLine(e1S, e1E, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);

    putLine(e1E, e2E, m_ssBlock, getBlockHandle(), m_saveBlkRecordHandle);

    putText(dimText,
            toVector3d(textMidPoint),
            toVector3d(dimLine),
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
    Base::Vector3d e1S(startExt1[0], startExt1[1], startExt1[2]);  // apex
    Base::Vector3d e2S(startExt2[0], startExt2[1], startExt2[2]);
    Base::Vector3d e1E(endExt1[0], endExt1[1], endExt1[2]);
    Base::Vector3d e2E(endExt2[0], endExt2[1], endExt2[2]);
    Base::Vector3d e1 = e1E - e1S;
    Base::Vector3d e2 = e2E - e2S;

    double startAngle = atan2(e2.y, e2.x);
    double endAngle = atan2(e1.y, e1.x);
    double span = fabs(endAngle - startAngle);
    double offset = span * 0.10;
    if (startAngle < 0) {
        startAngle += 2.0 * M_PI;
    }
    if (endAngle < 0) {
        endAngle += 2.0 * M_PI;
    }
    Base::Vector3d startOff(cos(startAngle + offset), sin(startAngle + offset), 0.0);
    Base::Vector3d endOff(cos(endAngle - offset), sin(endAngle - offset), 0.0);
    startAngle = startAngle * 180.0 / M_PI;
    endAngle = endAngle * 180.0 / M_PI;

    Base::Vector3d linePt(lineDefPoint[0], lineDefPoint[1], lineDefPoint[2]);
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

    Base::Vector3d center(centerPoint[0], centerPoint[1], centerPoint[2]);
    Base::Vector3d a(arcPoint[0], arcPoint[1], arcPoint[2]);
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

    Base::Vector3d a1(arcPoint1[0], arcPoint1[1], arcPoint1[2]);
    Base::Vector3d a2(arcPoint2[0], arcPoint2[1], arcPoint2[2]);
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

CDxfRead::CDxfRead(const char* filepath)
{
    // start the file
    m_ifs = new ifstream(filepath);
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
    delete m_CodePage;
    delete m_encoding;
}

double CDxfRead::mm(double value) const
{
    // re #6461
    // this if handles situation of malformed DXF file where
    // MEASUREMENT specifies English units, but
    // INSUNITS specifies millimeters or is not specified
    //(millimeters is our default)
    if (m_measurement_inch && (m_eUnits == eMillimeters)) {
        value *= 25.4;
    }

    switch (m_eUnits) {
        case eUnspecified:
            return (value * 1.0);  // We don't know any better.
        case eInches:
            return (value * 25.4);
        case eFeet:
            return (value * 25.4 * 12);
        case eMiles:
            return (value * 1609344.0);
        case eMillimeters:
            return (value * 1.0);
        case eCentimeters:
            return (value * 10.0);
        case eMeters:
            return (value * 1000.0);
        case eKilometers:
            return (value * 1000000.0);
        case eMicroinches:
            return (value * 25.4 / 1000.0);
        case eMils:
            return (value * 25.4 / 1000.0);
        case eYards:
            return (value * 3 * 12 * 25.4);
        case eAngstroms:
            return (value * 0.0000001);
        case eNanometers:
            return (value * 0.000001);
        case eMicrons:
            return (value * 0.001);
        case eDecimeters:
            return (value * 100.0);
        case eDekameters:
            return (value * 10000.0);
        case eHectometers:
            return (value * 100000.0);
        case eGigameters:
            return (value * 1000000000000.0);
        case eAstronomicalUnits:
            return (value * 149597870690000.0);
        case eLightYears:
            return (value * 9454254955500000000.0);
        case eParsecs:
            return (value * 30856774879000000000.0);
        default:
            return (value * 1.0);  // We don't know any better.
    }                              // End switch
}  // End mm() method

//
//  Setup for ProcessCommonEntityAttribute
void CDxfRead::Setup3DVectorAttribute(int x_record_type, double destination[3])
{
    SetupScaledDoubleAttribute(x_record_type, destination[0]);
    SetupScaledDoubleAttribute(x_record_type + 10, destination[1]);
    SetupScaledDoubleAttribute(x_record_type + 20, destination[2]);
    destination[0] = destination[1] = destination[2] = 0.0;
}
void CDxfRead::Setup3DCoordinatesIntoLists(int x_record_type,
                                           list<double>& x_destination,
                                           list<double>& y_destination,
                                           list<double>& z_destination)
{
    SetupScaledDoubleIntoList(x_record_type, x_destination);
    SetupScaledDoubleIntoList(x_record_type + 10, y_destination);
    SetupScaledDoubleIntoList(x_record_type + 20, z_destination);
}
void CDxfRead::SetupScaledDoubleAttribute(int x_record_type, double& destination)
{
    m_coordinate_attributes.emplace(x_record_type, std::pair(&ProcessScaledDouble, &destination));
}
void CDxfRead::SetupScaledDoubleIntoList(int x_record_type, list<double>& destination)
{
    m_coordinate_attributes.emplace(x_record_type,
                                    std::pair(&ProcessScaledDoubleIntoList, &destination));
}
void CDxfRead::SetupStringAttribute(int x_record_type, char* destination)
{
    m_coordinate_attributes.emplace(x_record_type, std::pair(&ProcessString, destination));
}
void CDxfRead::SetupStringAttribute(int x_record_type, std::string& destination)
{
    m_coordinate_attributes.emplace(x_record_type, std::pair(&ProcessStdString, &destination));
}
template<typename T>
void CDxfRead::SetupValueAttribute(int record_type, T& destination)
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
    double value;
    ss >> value;
    if (ss.fail()) {
        object->ImportError("Unable to parse value '%s', using zero as its value\n",
                            object->m_record_data);
        value = 0;
    }
    *static_cast<double*>(target) = object->mm(value);
}
void CDxfRead::ProcessScaledDoubleIntoList(CDxfRead* object, void* target)
{
    std::istringstream ss;
    ss.imbue(std::locale("C"));

    ss.str(object->m_record_data);
    double value;
    ss >> value;
    if (ss.fail()) {
        object->ImportError("Unable to parse value '%s', using zero as its value\n",
                            object->m_record_data);
        value = 0;
    }
    static_cast<std::list<double>*>(target)->push_back(object->mm(value));
}
template<typename T>
void CDxfRead::ProcessValue(CDxfRead* object, void* target)
{
    std::istringstream ss;
    ss.imbue(std::locale("C"));

    ss.str(object->m_record_data);
    ss >> *static_cast<T*>(target);
    if (ss.fail()) {
        object->ImportError("Unable to parse value '%s', using zero as its value\n",
                            object->m_record_data);
        *static_cast<T*>(target) = 0;
    }
}
void CDxfRead::ProcessStdString(CDxfRead* object, void* target)
{
    *static_cast<std::string*>(target) = object->m_record_data;
}
void CDxfRead::ProcessString(CDxfRead* object, void* target)
{
    strcpy(static_cast<char*>(target), object->m_record_data);
}

void CDxfRead::InitializeAttributes()
{
    m_coordinate_attributes.clear();
}
void CDxfRead::InitializeCommonEntityAttributes()
{
    InitializeAttributes();
    strcpy(m_layer_name, "0");
    m_LineType[0] = '\0';
    m_ColorIndex = ColorBylayer;
    SetupStringAttribute(6, m_LineType);
    SetupStringAttribute(8, m_layer_name);
    SetupValueAttribute(62, m_ColorIndex);
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
    while (get_next_record() && m_record_type != 0) {
        ProcessAttribute();
    }
    repeat_last_record();
}

//
//  The individual Entity reader functions
//  These return false if they catch an exception and ignore it because of ignore_errors.
bool CDxfRead::ReadLine()
{
    double start[3], end[3];
    Setup3DVectorAttribute(10, start);
    Setup3DVectorAttribute(11, end);
    ProcessAllAttributes();

    OnReadLine(start, end, LineTypeIsHidden());
    return true;
}

bool CDxfRead::ReadPoint()
{
    double location[3];
    Setup3DVectorAttribute(10, location);
    ProcessAllAttributes();

    OnReadPoint(location);
    return true;
}

bool CDxfRead::ReadArc()
{
    double start_angle_degrees = 0;
    double end_angle_degrees = 0;
    double radius = 0;
    double centre[3];
    double z_extrusion_dir = 1.0;

    Setup3DVectorAttribute(10, centre);
    SetupScaledDoubleAttribute(40, radius);
    SetupValueAttribute(50, start_angle_degrees);
    SetupValueAttribute(51, end_angle_degrees);
    SetupValueAttribute(230, z_extrusion_dir);
    ProcessAllAttributes();

    OnReadArc(start_angle_degrees,
              end_angle_degrees,
              radius,
              centre,
              z_extrusion_dir,
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

    Setup3DVectorAttribute(210, sd.norm);
    SetupValueAttribute(70, sd.flag);
    SetupValueAttribute(71, sd.degree);
    SetupValueAttribute(72, sd.knots);
    SetupValueAttribute(73, sd.control_points);
    SetupValueAttribute(74, sd.fit_points);
    SetupScaledDoubleIntoList(40, sd.knot);
    SetupScaledDoubleIntoList(41, sd.weight);
    Setup3DCoordinatesIntoLists(10, sd.controlx, sd.controly, sd.controlz);
    Setup3DCoordinatesIntoLists(11, sd.fitx, sd.fity, sd.fitz);
    Setup3DCoordinatesIntoLists(12, sd.starttanx, sd.starttany, sd.starttanz);
    Setup3DCoordinatesIntoLists(12, sd.endtanx, sd.endtany, sd.endtanz);
    ProcessAllAttributes();

    OnReadSpline(sd);
    return true;
}

bool CDxfRead::ReadCircle()
{
    double radius = 0.0;
    double centre[3];

    Setup3DVectorAttribute(10, centre);
    SetupScaledDoubleAttribute(40, radius);
    ProcessAllAttributes();

    OnReadCircle(centre, radius, LineTypeIsHidden());
    return true;
}

bool CDxfRead::ReadText()
{
    double insertionPoint[3];
    double height = 0.03082;
    double rotation = 0;
    std::string textPrefix;

    Setup3DVectorAttribute(10, insertionPoint);
    SetupScaledDoubleAttribute(40, height);
    SetupValueAttribute(50, rotation);
    while (get_next_record() && m_record_type != 0) {
        if (!ProcessAttribute()) {
            switch (m_record_type) {
                case 3:
                    // Additional text that goes before the type 1 text
                    // Note that if breaking the text into type-3 records splits a UFT-8 encoding we
                    // do the decoding after splicing the lines together. I'm not sure if this
                    // actually occurs, but handling the text this way will treat this condition
                    // properly.
                    textPrefix.append(m_record_data);
                    break;
                case 1:
                    // final text
                    textPrefix.append(m_record_data);
                    break;
            }
        }
    }

    const char* utfStr = (this->*stringToUTF8)(textPrefix.c_str());
    if (utfStr != nullptr) {
        OnReadText(insertionPoint, height * 25.4 / 72.0, utfStr, rotation);
        if (utfStr != textPrefix.c_str()) {
            delete utfStr;
        }
    }
    else {
        ImportError("Unable to process encoding for TEXT/MTEXT '%s'", textPrefix.c_str());
    }
    repeat_last_record();
    return true;
}

bool CDxfRead::ReadEllipse()
{
    double centre[3];
    double majorAxisEnd[3];  //  relative to centre
    double eccentricity = 0;
    double startAngleRadians = 0;
    double endAngleRadians = 2 * M_PI;

    Setup3DVectorAttribute(10, centre);
    Setup3DVectorAttribute(11, majorAxisEnd);
    SetupValueAttribute(40, eccentricity);
    SetupValueAttribute(41, startAngleRadians);
    SetupValueAttribute(42, endAngleRadians);
    ProcessAllAttributes();

    OnReadEllipse(centre, majorAxisEnd, eccentricity, startAngleRadians, endAngleRadians);
    return true;
}

bool CDxfRead::ReadLwPolyLine()
{
    VertexInfo currentVertex = {{0, 0, 0}, 0};
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
    Setup3DVectorAttribute(10, currentVertex.location);
    SetupScaledDoubleAttribute(42, currentVertex.bulge);
    SetupValueAttribute(70, flags);
    while (get_next_record() && m_record_type != 0) {
        if ((m_record_type == 10 && have_x) || (m_record_type == 21 && have_y)) {
            // Starting a new vertex and there is a previous vertex. Save it and init a new one.
            vertices.push_back(currentVertex);
            currentVertex.location[0] = 0.0;
            currentVertex.location[1] = 0.0;
            currentVertex.location[2] = 0.0;
            currentVertex.bulge = 0.0;
            have_x = m_record_type == 10;
            have_y = m_record_type == 20;
        }
        else if (m_record_type == 10) {
            have_x = true;
        }
        else if (m_record_type == 10) {
            have_y = true;
        }
        ProcessAttribute();
    }
    // At the end of the entity if we have vertex information use this as the final vertex.
    // (else it was a line with no vertices at all)
    if (have_x || have_y) {
        vertices.push_back(currentVertex);
    }

    OnReadPolyline(vertices, flags);
    repeat_last_record();
    return true;
}

bool CDxfRead::ReadPolyLine()
{
    VertexInfo currentVertex = {{0, 0, 0}, 0};
    list<VertexInfo> vertices;
    int flags = 0;

    SetupValueAttribute(70, flags);
    ProcessAllAttributes();

    // We are now followed by a series of VERTEX entities followed by ENDSEQ.
    // To avoid eating and discarding the rest of the entieies if ENDSEQ is missing,
    // we quit on any unknown type-0 record.
    Setup3DVectorAttribute(10, currentVertex.location);
    SetupScaledDoubleAttribute(42, currentVertex.bulge);
    while (get_next_record() && m_record_type == 0 && !strcmp(m_record_data, "VERTEX")) {
        // Set vertex defaults
        currentVertex.location[0] = 0.0;
        currentVertex.location[1] = 0.0;
        currentVertex.location[2] = 0.0;
        currentVertex.bulge = 0.0;
        ProcessAllAttributes();
        vertices.push_back(currentVertex);
    }
    if (strcmp(m_record_data, "SEQEND")) {
        ImportError("POLYLINE ends with '%s' record rather than 'SEQEND'\n", m_record_data);
        repeat_last_record();
    }

    OnReadPolyline(vertices, flags);
    return true;
}

bool CDxfRead::ReadInsert()
{
    double center[3];
    double scale[3] = {1, 1, 1};
    double rotationDegrees = 0.0;
    char blockName[1024] = {0};

    Setup3DVectorAttribute(10, center);
    SetupValueAttribute(41, scale[0]);
    SetupValueAttribute(42, scale[1]);
    SetupValueAttribute(43, scale[2]);
    SetupValueAttribute(50, rotationDegrees);
    SetupStringAttribute(2, blockName);
    ProcessAllAttributes();
    OnReadInsert(center, scale, blockName, rotationDegrees * M_PI / 180);
    return (true);
}

bool CDxfRead::ReadDimension()
{
    double start[3];
    double end[3];
    double linePosition[3];
    double textPosition[3];
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
    Setup3DVectorAttribute(13, start);         // WCS
    Setup3DVectorAttribute(14, end);           // WCS
    Setup3DVectorAttribute(10, linePosition);  // WCS
    Setup3DVectorAttribute(11, textPosition);  // OCS
    SetupValueAttribute(50, rotation);
    SetupValueAttribute(70, dimensionType);
    ProcessAllAttributes();

    dimensionType &= ~0xE0;  //  Remove flags
    switch (dimensionType) {
        case 0:
        case 1:
            OnReadDimension(start, end, linePosition, rotation * M_PI / 180);
            break;
        default:
            UnsupportedFeature("Dimension type '%d'", dimensionType);
            break;
    }
    return (true);
}

bool CDxfRead::ReadUnknownEntity()
{
    UnsupportedFeature("Entity type '%s'", m_record_data);
    ProcessAllAttributes();
    return true;
}

bool CDxfRead::ReadBlockInfo()
{
    int blockType = 0;
    InitializeAttributes();
    // Both 2 and 3 are the block name.
    SetupStringAttribute(2, m_block_name);
    SetupStringAttribute(3, m_block_name);
    SetupValueAttribute(70, blockType);
    ProcessAllAttributes();

    // Read the entities in the block definition.
    // These are processed just like in-drawing entities but the code can
    // recognize that we are in the BLOCKS section, and which block we are defining,
    // by looking at the result of LayerName()
    // Note that the Layer Name in the block entities is ignored even though it
    // should be used to resolve BYLAYER attributes and also for placing the entity
    // when the block is inserted.
    if (blockType & 0x04) {
        // Note that this doesn't mean there are not entities in the block. I don't
        // know if the external reference can be cached because there are two other bits
        // here, 0x10 and 0x20, that seem to handle "resolved" external references.
        UnsupportedFeature("External (xref) BLOCK");
    }
    while (get_next_record() && m_record_type != 0 && strcmp(m_record_data, "ENDBLK")) {
        if (blockType & 0x01) {
            // It is an anonymous block used to build dimensions, hatches, etc so we don't need it
            // and don't want to complaining about unhandled entity types.
            while (get_next_record() && m_record_type != 0) {}
            repeat_last_record();
        }
        else if (IgnoreErrors()) {
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

void CDxfRead::UnsupportedFeature(const char* format, ...)
{
    // Upcoming C++20 std::format might simplify some of this code
    va_list args;
    std::string formattedMessage;

    va_start(args, format);
    // Make room in formattedMessage for the formatted text and its terminating null byte
    formattedMessage.resize((size_t)vsnprintf(nullptr, 0, format, args) + 1);
    va_end(args);
    va_start(args, format);
    vsprintf(formattedMessage.data(), format, args);
    va_end(args);
    // Remove the null byte
    formattedMessage.pop_back();

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

    if ((*m_ifs).eof()) {
        m_not_eof = false;
        return false;
    }

    m_ifs->getline(m_record_data, 1024);
    ++m_line;
    if (sscanf(m_record_data, "%d", &m_record_type) != 1) {
        ImportError("CDxfRead::get_next_record() Failed to get integer record type from '%s'\n",
                    m_record_data);
        return (false);
    }
    if ((*m_ifs).eof()) {
        return false;
    }

    m_ifs->getline(m_record_data, 1024);
    ++m_line;
    // Remove any carriage return at the end of m_str which may occur because of inconsistent
    // handling of LF vs. CRLF line termination.
    size_t len = strlen(m_record_data);
    if (len > 0 && m_record_data[len - 1] == '\r') {
        m_record_data[len - 1] = '\0';
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

bool CDxfRead::ReadUnits()
{
    get_next_record();  // Get the value for the variable
    int n = 0;
    if (sscanf(m_record_data, "%d", &n) == 1) {
        m_eUnits = eDxfUnits_t(n);
        if (m_eUnits != eUnspecified) {
            m_measurement_inch = false; // prioritize INSUNITS over MEASUREMENT variable
        }
        return (true);
    }  // End if - then
    else {
        ImportError("CDxfRead::ReadUnits() Failed to get integer from '%s'\n", m_record_data);
        return (false);
    }
}

//
//  Intercepts for On... calls to derived class
//  (These have distinct signatures from the ones they call)
bool CDxfRead::OnReadPolyline(std::list<VertexInfo>& vertices, int flags)
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
                double cot = 0.5 * ((1.0 / startVertex->bulge) - startVertex->bulge);
                double cx = ((startVertex->location[0] + endVertex->location[0])
                             - ((endVertex->location[1] - startVertex->location[1]) * cot))
                    / 2.0;
                double cy = ((startVertex->location[1] + endVertex->location[1])
                             + ((endVertex->location[0] - startVertex->location[0]) * cot))
                    / 2.0;
                double pc[3] = {cx, cy, (startVertex->location[2] + endVertex->location[2]) / 2.0};
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
        startVertex = endVertex;
    }
    return true;
}
void CDxfRead::OnReadArc(double start_angle,
                         double end_angle,
                         double radius,
                         const double* center,
                         double z_extrusion_dir,
                         bool hidden)
{
    double s[3] = {0, 0, 0}, e[3] = {0, 0, 0}, temp[3] = {0, 0, 0};
    if (z_extrusion_dir == 1.0) {
        temp[0] = center[0];
        temp[1] = center[1];
        temp[2] = center[2];
        s[0] = center[0] + radius * cos(start_angle * M_PI / 180);
        s[1] = center[1] + radius * sin(start_angle * M_PI / 180);
        s[2] = center[2];
        e[0] = center[0] + radius * cos(end_angle * M_PI / 180);
        e[1] = center[1] + radius * sin(end_angle * M_PI / 180);
        e[2] = center[2];
    }
    else {
        temp[0] = -center[0];
        temp[1] = center[1];
        temp[2] = center[2];

        e[0] = -(center[0] + radius * cos(start_angle * M_PI / 180));
        e[1] = (center[1] + radius * sin(start_angle * M_PI / 180));
        e[2] = center[2];
        s[0] = -(center[0] + radius * cos(end_angle * M_PI / 180));
        s[1] = (center[1] + radius * sin(end_angle * M_PI / 180));
        s[2] = center[2];
    }
    OnReadArc(s, e, temp, true, hidden);
}

void CDxfRead::OnReadCircle(const double* center, double radius, bool hidden)
{
    // OnReadCircle wants a start point, so we pick an arbitrary point on the circunference
    double s[3] = {center[0] + radius, center[1], center[2]};

    OnReadCircle(s,
                 center,
                 false,
                 hidden);  // false to change direction because otherwise the arc length is zero
}

void CDxfRead::OnReadEllipse(const double* center,
                             const double* m,
                             double ratio,
                             double start_angle,
                             double end_angle)
{
    double major_radius = sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
    double minor_radius = major_radius * ratio;

    // Since we only support 2d stuff, we can calculate the rotation from the major axis x and y
    // value only, since z is zero, major_radius is the vector length

    double rotation = atan2(m[1] / major_radius, m[0] / major_radius);


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
    std::vector<std::string>::const_iterator first = VersionNames.cbegin();
    std::vector<std::string>::const_iterator last = VersionNames.cend();
    std::vector<std::string>::const_iterator found = std::lower_bound(first, last, m_record_data);
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
    get_next_record();              // Get the value for the variable
    assert(m_CodePage == nullptr);  // If not, we have found two DWGCODEPAGE variables or DoRead was
                                    // called twice on the same CDxfRead object.
    m_CodePage = new std::string(m_record_data);

    return ResolveEncoding();
}

bool CDxfRead::ResolveEncoding()
{
    if (m_encoding != nullptr) {
        delete m_encoding;
        m_encoding = nullptr;
    }
    if (m_version >= R2007) {  // Note this does not include RUnknown, but does include RLater
        m_encoding = new std::string("utf_8");
        stringToUTF8 = &CDxfRead::UTF8ToUTF8;
    }
    else if (m_CodePage == nullptr) {
        // cp1252
        m_encoding = new std::string("cp1252");
        stringToUTF8 = &CDxfRead::GeneralToUTF8;
    }
    else {
        // Codepage names may be of the form "ansi_1252" which we map to "cp1252" but we don't map
        // "ansi_x3xxxx" (which happens to mean "ascii")
        // Also some DXF files have the codepage name in uppercase so we lowercase it.
        std::string* p = new std::string(*m_CodePage);
        std::transform(p->begin(), p->end(), p->begin(), ::tolower);
        if (strncmp(p->c_str(), "ansi_", 5) == 0 && strncmp(p->c_str(), "ansi_x3", 7) != 0) {
            p->replace(0, 5, "cp");
        }
        m_encoding = p;
        // At this point we want to recognize synonyms for "utf_8" and use the custom decoder
        // function. This is because this is one of the common cases and our decoder function is a
        // fast no-op. We don't actually use the decoder function we get from PyCodec_Decoder
        // because to call it we have to convert the (char *) text into a 'bytes' object first so we
        // can pass it to the function using PyObject_Callxxx(), getting the PYObject containing the
        // Python string, which we then decode back to UTF-8. It is simpler to call
        // PyUnicode_DecodeXxxx which takes a (const char *) and is just a direct c++ callable.
        Base::PyGILStateLocker lock;
        PyObject* pyDecoder = PyCodec_Decoder(m_encoding->c_str());
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
    return m_encoding != nullptr;
}

const char* CDxfRead::UTF8ToUTF8(const char* encoded) const
{
    return encoded;
}

const char* CDxfRead::GeneralToUTF8(const char* encoded) const
{
    Base::PyGILStateLocker lock;
    PyObject* decoded = PyUnicode_Decode(encoded, strlen(encoded), m_encoding->c_str(), "strict");
    if (decoded == nullptr) {
        return nullptr;
    }
    Py_ssize_t len;
    const char* converted = PyUnicode_AsUTF8AndSize(decoded, &len);
    char* result = nullptr;
    if (converted != nullptr) {
        // converted only has lifetime of decoded so we must save a copy.
        result = (char*)malloc(len + 1);
        if (result == nullptr) {
            PyErr_SetString(PyExc_MemoryError, "Out of memory");
        }
        else {
            memcpy(result, converted, len + 1);
        }
    }
    Py_DECREF(decoded);
    return result;
}

void CDxfRead::DoRead(const bool ignore_errors /* = false */)
{
    m_ignore_errors = ignore_errors;
    if (m_fail) {
        return;
    }

    // Loop reading and identifying the sections.
    while (get_next_record()) {
        if (m_record_type != 0) {
            ImportError("Found type %d record when expecting start of a SECTION or EOF\n",
                        m_record_type);
            continue;
        }
        if (!strcmp(m_record_data, "EOF")) {  // TODO: Check for drivel beyond EOF record
            break;
        }
        if (strcmp(m_record_data, "SECTION")) {
            ImportError("Found %s record when expecting start of a SECTION\n", m_record_data);
            continue;
        }

        if (!get_next_record()) {
            ImportError("Unclosed SECTION at end of file\n");
            return;
        }
        if (m_record_type != 2) {
            ImportError("Ignored SECTION with no name record\n");
            if (!ReadIgnoredSection()) {
                return;
            }
        }
        else if (!strcmp(m_record_data, "HEADER")) {
            if (!ReadHeaderSection()) {
                return;
            }
        }
        else if (!strcmp(m_record_data, "TABLES")) {
            if (!ReadTablesSection()) {
                return;
            }
        }
        else if (!strcmp(m_record_data, "BLOCKS")) {
            if (!ReadBlocksSection()) {
                return;
            }
        }
        else if (!strcmp(m_record_data, "ENTITIES")) {
            if (!ReadEntitiesSection()) {
                return;
            }
        }
        else {
            if (!ReadIgnoredSection()) {
                return;
            }
        }
    }
    AddGraphics();

    // FLush out any unsupported features messages
    auto i = m_unsupportedFeaturesNoted.begin();
    if (i != m_unsupportedFeaturesNoted.end()) {
        ImportError("Unsupported DXF features:\n");
        do {
            ImportError("%s: %d time(s) first at line %d\n",
                        i->first,
                        i->second.first,
                        i->second.second);
        } while (++i != m_unsupportedFeaturesNoted.end());
    }
}

bool CDxfRead::ReadEntity()
{
    InitializeCommonEntityAttributes();
    // The entity record is already the current record and is already checked as a type 0 record
    if (!strcmp(m_record_data, "LINE")) {
        return ReadLine();
    }
    else if (!strcmp(m_record_data, "ARC")) {
        return ReadArc();
    }
    else if (!strcmp(m_record_data, "CIRCLE")) {
        return ReadCircle();
    }
    else if (!strcmp(m_record_data, "MTEXT")) {
        return ReadText();
    }
    else if (!strcmp(m_record_data, "TEXT")) {
        return ReadText();
    }
    else if (!strcmp(m_record_data, "ELLIPSE")) {
        return ReadEllipse();
    }
    else if (!strcmp(m_record_data, "SPLINE")) {
        return ReadSpline();
    }
    else if (!strcmp(m_record_data, "LWPOLYLINE")) {
        return ReadLwPolyLine();
    }
    else if (!strcmp(m_record_data, "POLYLINE")) {
        return ReadPolyLine();
    }
    else if (!strcmp(m_record_data, "POINT")) {
        return ReadPoint();
    }
    else if (!strcmp(m_record_data, "INSERT")) {
        return ReadInsert();
    }
    else if (!strcmp(m_record_data, "DIMENSION")) {
        return ReadDimension();
    }
    else {
        return ReadUnknownEntity();
    }
}

bool CDxfRead::ReadHeaderSection()
{
    // Read to the next ENDSEC record marking the end of the section.
    // This section contains variables, most of which we ignore. Each one is a type-9 record giving
    // the variable name, followed by a single record giving the value; the record type depends on
    // the variable's data type.
    while (get_next_record()) {
        if (m_record_type == 0 && !strcmp(m_record_data, "ENDSEC")) {
            return true;
        }
        else if (m_record_type != 9) {
            continue;  // Quietly ignore unknown record types
        }
        if (!strcmp(m_record_data, "$INSUNITS")) {
            if (!ReadUnits()) {
                return false;
            }
        }
        else if (!strcmp(m_record_data, "$MEASUREMENT")) {
            get_next_record();
            int n = 1;
            if (sscanf(m_record_data, "%d", &n) == 1) {
                if (n == 0) {
                    m_measurement_inch = true;
                }
            }
        }
        else if (!strcmp(m_record_data, "$ACADVER")) {
            if (!ReadVersion()) {
                return false;
            }
        }
        else if (!strcmp(m_record_data, "$DWGCODEPAGE")) {
            if (!ReadDWGCodePage()) {
                return false;
            }
        }
        else {
            // any other variable, skip its value
            if (!get_next_record()) {
                return false;
            }
        }
    }
    return false;
}

bool CDxfRead::ReadTablesSection()
{
    // Read to the next ENDSEC record marking the end of the section.
    // This section contains various tables, many of which we ignore. Each one is a type-0 TABLE
    // record followed by a type-2 (name) record giving the table name, followed by the table
    // contents. Each set of contents is terminates by the next type-0 TABLE or ENDSEC directive.
    while (get_next_record()) {
        if (m_record_type != 0) {
            continue;  // Ignore any non-type-0 contents in the section.
        }
        if (!strcmp(m_record_data, "ENDSEC")) {
            return true;
        }
        if (strcmp(m_record_data, "TABLE")) {
            continue;  // Ignore any type-0 non-TABLE contents in the section
        }
        get_next_record();
        if (m_record_type != 2) {
            ImportError("Found unexpected type %d record instead of table name\n", m_record_type);
        }
        else if (!strcmp(m_record_data, "LAYER")) {
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
        if (m_record_type == 0 && !strcmp(m_record_data, "ENDSEC")) {
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
        if (m_record_type != 0) {
            continue;  // quietly ignore non-type-0 records;
        }
        if (!strcmp(m_record_data, "ENDSEC")) {
            // End of section
            return true;
        }
        if (strcmp(m_record_data, "BLOCK")) {
            continue;  // quietly ignore non-BLOCK records
        }
        if (!ReadBlockInfo()) {
            ImportError("CDxfRead::DoRead() Failed to read block\n");
        }
        m_block_name[0] = '\0';
    }

    return false;
}

bool CDxfRead::ReadEntitiesSection()
{
    // Read to the next ENDSEC record marking the end of the section.
    // Within this section we should find type-0 BLOCK groups
    while (get_next_record()) {
        if (m_record_type != 0) {
            continue;  // quietly ignore non-type-0 records;
        }
        if (!strcmp(m_record_data, "ENDSEC")) {
            // End of section
            return true;
        }
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

    return false;
}

bool CDxfRead::ReadLayer()
{
    std::string layername;
    ColorIndex_t layerColor = 0;
    int layerFlags = 0;
    InitializeAttributes();

    SetupStringAttribute(2, layername);
    SetupValueAttribute(62, layerColor);
    SetupValueAttribute(70, layerFlags);
    ProcessAllAttributes();
    if (layername.empty()) {
        ImportError("CDxfRead::ReadLayer() - no layer name\n");
        return false;
    }
    if (layerFlags & 0x01) {
        // Frozen layers are implicitly hidden which we don't do yet.
        // TODO: Should have an import option to omit frozen layers.
        UnsupportedFeature("Frozen layers");
    }
    if (layerColor < 0) {
        UnsupportedFeature("Hidden layers");
        layerColor = -layerColor;
    }
    m_layer_ColorIndex_map[layername] = layerColor;
    return true;
}
bool CDxfRead::ReadLayerTable()
{
    // Read to the next TABLE record indicating another table in the TABLES section, or to the
    // ENDSEC record marking the end of the TABLES section. This table contains a series of type-0
    // LAYER groups
    while (get_next_record()) {
        if (m_record_type != 0) {
            continue;  // quietly ignore non-type-0 records; this table has some preamble
        }
        if (!strcmp(m_record_data, "TABLE") || !strcmp(m_record_data, "ENDSEC")) {
            // End of table
            repeat_last_record();
            return true;
        }
        if (strcmp(m_record_data, "LAYER")) {
            continue;  // quietly ignore non-LAYER records
        }
        if (!ReadLayer()) {
            ImportError("CDxfRead::DoRead() Failed to read layer\n");
        }
        continue;
    }

    return false;
}

bool CDxfRead::ReadIgnoredTable()
{
    // Read to the next TABLE record indicating another table in the TABLES section, or to the
    // ENDSEC record marking the end of the TABLES section.
    while (get_next_record()) {
        if (m_record_type == 0
            && (!strcmp(m_record_data, "TABLE") || !strcmp(m_record_data, "ENDSEC"))) {
            repeat_last_record();
            return true;
        }
    }
    return false;
}

std::string CDxfRead::LayerName() const
{
    std::string result;

    if (strlen(m_block_name) > 0) {
        result.append("BLOCKS ");
        result.append(m_block_name);
        result.append(" ");
    }

    else if (strlen(m_layer_name) > 0) {
        result.append("ENTITIES ");
        result.append(m_layer_name);
    }

    return (result);
}

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
    else if (distance < 8) {
        // Between 4 and 8 gives a blend of the full primary and the black level
        return ((8 - distance) + blackLevel * (distance - 4)) / 4;
    }
    else {
        // 8 and beyond yield the black level
        return blackLevel;
    }
}
inline static App::Color wheel(int hue, double blackLevel, double multiplier = 1.0)
{
    return App::Color(level(hue - 0, blackLevel) * multiplier,
                      level(hue - 8, blackLevel) * multiplier,
                      level(hue - 16, blackLevel) * multiplier);
}
App::Color CDxfRead::ObjectColor() const
{
    int index = m_ColorIndex;
    if (index == ColorBylayer)  // if color = layer color, replace by color from layer
    {
        auto key = std::string(m_layer_name);
        index = m_layer_ColorIndex_map.count(key) > 0 ? m_layer_ColorIndex_map.at(key) : 0;
    }
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
        int brightness = (index - 250 + (255 - index) * 0.2) / 5;
        result = App::Color(brightness, brightness, brightness);
    }
    else {
        int fade = (index / 2) % 5;
        static double fades[5] = {1.00, 0.74, 0.50, 0.40, 0.30};
        return wheel(index / 10 - 1, (index & 1) ? 0.69 : 0, fades[fade]);
    }
    // TODO: These colors are modified to contrast with the background. In the original program
    // this is just a rendering feature, but FreeCAD does not support this so the user has the
    // option of modifying the colors to contrast with the background at time of import.
    return result;
}
