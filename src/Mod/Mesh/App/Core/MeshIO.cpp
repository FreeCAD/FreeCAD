// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
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


#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string_view>


#include <boost/algorithm/string.hpp>
#include <boost/convert.hpp>
#include <boost/convert/spirit.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include "IO/Reader3MF.h"
#include "IO/ReaderOBJ.h"
#include "IO/ReaderPLY.h"
#include "IO/Writer3MF.h"
#include "IO/WriterInventor.h"
#include "IO/WriterOBJ.h"
#include <Base/Builder3D.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Placement.h>
#include <Base/Reader.h>
#include <Base/Sequencer.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/Writer.h>
#include <zipios++/gzipoutputstream.h>
#include <zipios++/zipoutputstream.h>

#include "Builder.h"
#include "Definitions.h"
#include "Degeneration.h"
#include "Iterator.h"
#include "MeshIO.h"
#include "MeshKernel.h"


using namespace MeshCore;

namespace MeshCore
{

std::string& ltrim(std::string& str)
{
    std::string::size_type pos = 0;
    for (char it : str) {
        if (it != 0x20 && it != 0x09) {
            break;
        }
        pos++;
    }
    if (pos > 0) {
        str = str.substr(pos);
    }
    return str;
}

int numDigits(int number)
{
    number = std::abs(number);
    int digits = 1;
    int step = 10;
    while (step <= number) {
        digits++;
        step *= 10;
    }
    return digits;
}

/* Usage by CMeshNastran, CMeshCadmouldFE. Added by Sergey Sukhov (26.04.2002)*/
struct NODE
{
    float x, y, z;
};
struct TRIA
{
    int iV[3];
};
struct QUAD
{
    int iV[4];
};

// This is a workaround for the issue described at:
// https://github.com/Zipios/Zipios/issues/43#issue-1618151314
//
// The workaround creates a tmp. ZIP file and uses the Python API
// to open the file zipios++ isn't able to handle and to copy over
// the files.
class ZipFixer
{
public:
    ZipFixer(const char* filename)
        : tmp {Base::FileInfo::getTempFileName()}
    {
        Base::ZipTools::rewrite(filename, tmp.filePath().c_str());
        str.open(tmp, std::ios::in | std::ios::binary);
    }

    ~ZipFixer()
    {
        tmp.deleteFile();
    }

    Base::ifstream& getStream()
    {
        return str;
    }

private:
    Base::FileInfo tmp;
    Base::ifstream str;
};

}  // namespace MeshCore

// --------------------------------------------------------------

bool Material::operator==(const Material& mat) const
{
    if (binding != mat.binding) {
        return false;
    }
    if (ambientColor != mat.ambientColor) {
        return false;
    }
    if (diffuseColor != mat.diffuseColor) {
        return false;
    }
    if (specularColor != mat.specularColor) {
        return false;
    }
    if (emissiveColor != mat.emissiveColor) {
        return false;
    }
    if (shininess != mat.shininess) {
        return false;
    }
    if (transparency != mat.transparency) {
        return false;
    }
    return true;
}

bool Material::operator!=(const Material& mat) const
{
    return !operator==(mat);
}

// --------------------------------------------------------------

std::vector<std::string> MeshInput::supportedMeshFormats()
{
    std::vector<std::string> fmt;
    fmt.emplace_back("bms");
    fmt.emplace_back("ply");
    fmt.emplace_back("stl");
    fmt.emplace_back("ast");
    fmt.emplace_back("obj");
    fmt.emplace_back("nas");
    fmt.emplace_back("bdf");
    fmt.emplace_back("off");
    fmt.emplace_back("smf");
    return fmt;
}

MeshIO::Format MeshInput::getFormat(const char* FileName)
{
    Base::FileInfo fi(FileName);
    if (fi.hasExtension("bms")) {
        return MeshIO::Format::BMS;
    }
    if (fi.hasExtension("ply")) {
        return MeshIO::Format::PLY;
    }
    if (fi.hasExtension("stl")) {
        return MeshIO::Format::STL;
    }
    if (fi.hasExtension("ast")) {
        return MeshIO::Format::ASTL;
    }
    if (fi.hasExtension("obj")) {
        return MeshIO::Format::OBJ;
    }
    if (fi.hasExtension("off")) {
        return MeshIO::Format::OFF;
    }
    if (fi.hasExtension("smf")) {
        return MeshIO::Format::SMF;
    }

    throw Base::FileException("File extension not supported", FileName);
}

bool MeshInput::LoadAny(const char* FileName)
{
    // ask for read permission
    Base::FileInfo fi(FileName);
    if (!fi.exists() || !fi.isFile()) {
        throw Base::FileException("File does not exist", FileName);
    }
    if (!fi.isReadable()) {
        throw Base::FileException("No permission on the file", FileName);
    }

    Base::ifstream str(fi, std::ios::in | std::ios::binary);

    if (fi.hasExtension("bms")) {
        _rclMesh.Read(str);
        return true;
    }

    // read file
    bool ok = false;
    if (fi.hasExtension({"stl", "ast"})) {
        ok = LoadSTL(str);
    }
    else if (fi.hasExtension("iv")) {
        ok = LoadInventor(str);
        if (ok && _rclMesh.CountFacets() == 0) {
            Base::Console().warning("No usable mesh found in file '%s'", FileName);
        }
    }
    else if (fi.hasExtension({"nas", "bdf"})) {
        ok = LoadNastran(str);
    }
    else if (fi.hasExtension("obj")) {
        ok = LoadOBJ(str, FileName);
    }
    else if (fi.hasExtension("smf")) {
        ok = LoadSMF(str);
    }
    else if (fi.hasExtension("3mf")) {
        try {
            ok = Load3MF(str);
        }
        catch (const zipios::FCollException&) {
            ZipFixer zip(FileName);
            ok = Load3MF(zip.getStream());
        }
    }
    else if (fi.hasExtension("off")) {
        ok = LoadOFF(str);
    }
    else if (fi.hasExtension("ply")) {
        ok = LoadPLY(str);
    }
    else {
        throw Base::FileException("File extension not supported", FileName);
    }

    return ok;
}

bool MeshInput::LoadFormat(std::istream& input, MeshIO::Format fmt)
{
    switch (fmt) {
        case MeshIO::BMS:
            _rclMesh.Read(input);
            return true;
        case MeshIO::APLY:
        case MeshIO::PLY:
            return LoadPLY(input);
        case MeshIO::ASTL:
            return LoadAsciiSTL(input);
        case MeshIO::BSTL:
            return LoadBinarySTL(input);
        case MeshIO::STL:
            return LoadSTL(input);
        case MeshIO::OBJ:
            return LoadOBJ(input);
        case MeshIO::SMF:
            return LoadSMF(input);
        case MeshIO::ThreeMF:
            return Load3MF(input);
        case MeshIO::OFF:
            return LoadOFF(input);
        case MeshIO::IV:
            return LoadInventor(input);
        case MeshIO::NAS:
            return LoadNastran(input);
        default:
            throw Base::FileException("Unsupported file format");
    }
}

/** Loads an STL file either in binary or ASCII format.
 * Therefore the file header gets checked to decide if the file is binary or not.
 */
bool MeshInput::LoadSTL(std::istream& input)
{
    char szBuf[200];

    if (!input || input.bad()) {
        return false;
    }

    // Read in 50 characters from position 80 on and check for keywords like 'SOLID', 'FACET',
    // 'NORMAL', 'VERTEX', 'ENDFACET' or 'ENDLOOP'. As the file can be binary with one triangle only
    // we must not read in more than (max.) 54 bytes because the file size has only 134 bytes in
    // this case. On the other hand we must overread the first 80 bytes because it can happen that
    // the file is binary but contains one of these keywords.
    std::streambuf* buf = input.rdbuf();
    if (!buf) {
        return false;
    }
    buf->pubseekoff(80, std::ios::beg, std::ios::in);
    uint32_t ulCt {}, ulBytes = 50;
    input.read((char*)&ulCt, sizeof(ulCt));
    // if we have a binary STL with a single triangle we can only read-in 50 bytes
    if (ulCt > 1) {
        ulBytes = 100;
    }
    // Either it's really an invalid STL file or it's just empty. In this case the number of facets
    // must be 0.
    if (!input.read(szBuf, ulBytes)) {
        return (ulCt == 0);
    }
    szBuf[ulBytes] = 0;
    boost::algorithm::to_upper(szBuf);

    try {
        if (!strstr(szBuf, "SOLID") && !strstr(szBuf, "FACET") && !strstr(szBuf, "NORMAL")
            && !strstr(szBuf, "VERTEX") && !strstr(szBuf, "ENDFACET") && !strstr(szBuf, "ENDLOOP")) {
            // probably binary STL
            buf->pubseekoff(0, std::ios::beg, std::ios::in);
            return LoadBinarySTL(input);
        }

        // Ascii STL
        buf->pubseekoff(0, std::ios::beg, std::ios::in);
        return LoadAsciiSTL(input);
    }
    catch (const Base::MemoryException&) {
        _rclMesh.Clear();
        throw;  // Throw the same instance of Base::MemoryException
    }
    catch (const Base::AbortException&) {
        _rclMesh.Clear();
        return false;
    }
    catch (const Base::Exception&) {
        _rclMesh.Clear();
        throw;  // Throw the same instance of Base::Exception
    }
    catch (...) {
        _rclMesh.Clear();
        throw;
    }

    return true;
}

/** Loads an OBJ file. */
bool MeshInput::LoadOBJ(std::istream& input)
{
    ReaderOBJ reader(this->_rclMesh, this->_material);
    if (reader.Load(input)) {
        _groupNames = reader.GetGroupNames();
        return true;
    }

    return false;
}

bool MeshInput::LoadOBJ(std::istream& input, const char* filename)
{
    ReaderOBJ reader(this->_rclMesh, this->_material);
    if (reader.Load(input)) {
        _groupNames = reader.GetGroupNames();
        if (this->_material && this->_material->binding == MeshCore::MeshIO::PER_FACE) {
            Base::FileInfo fi(filename);
            std::string fn = fi.dirPath() + "/" + this->_material->library;
            fi.setFile(fn);
            Base::ifstream mtl(fi, std::ios::in | std::ios::binary);
            reader.LoadMaterial(mtl);
            mtl.close();
        }

        return true;
    }

    return false;
}

/** Loads an SMF file. */
bool MeshInput::LoadSMF(std::istream& input)
{
    boost::regex rx_p(
        "^v\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
        "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
        "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$"
    );
    boost::regex rx_f3(
        "^f\\s+([-+]?[0-9]+)"
        "\\s+([-+]?[0-9]+)"
        "\\s+([-+]?[0-9]+)\\s*$"
    );
    boost::cmatch what;

    unsigned long segment = 0;
    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;

    std::string line;
    float fX {}, fY {}, fZ {};
    int i1 = 1, i2 = 1, i3 = 1;
    MeshFacet item;

    if (!input || input.bad()) {
        return false;
    }

    std::streambuf* buf = input.rdbuf();
    if (!buf) {
        return false;
    }

    while (std::getline(input, line)) {
        if (boost::regex_match(line.c_str(), what, rx_p)) {
            fX = (float)std::atof(what[1].first);
            fY = (float)std::atof(what[4].first);
            fZ = (float)std::atof(what[7].first);
            meshPoints.push_back(MeshPoint(Base::Vector3f(fX, fY, fZ)));
        }
        else if (boost::regex_match(line.c_str(), what, rx_f3)) {
            // 3-vertex face
            i1 = std::atoi(what[1].first);
            i1 = i1 > 0 ? i1 - 1 : i1 + static_cast<int>(meshPoints.size());
            i2 = std::atoi(what[2].first);
            i2 = i2 > 0 ? i2 - 1 : i2 + static_cast<int>(meshPoints.size());
            i3 = std::atoi(what[3].first);
            i3 = i3 > 0 ? i3 - 1 : i3 + static_cast<int>(meshPoints.size());
            item.SetVertices(i1, i2, i3);
            item.SetProperty(segment);
            meshFacets.push_back(item);
        }
    }

    this->_rclMesh.Clear();  // remove all data before

    MeshCleanup meshCleanup(meshPoints, meshFacets);
    meshCleanup.RemoveInvalids();
    MeshPointFacetAdjacency meshAdj(meshPoints.size(), meshFacets);
    meshAdj.SetFacetNeighbourhood();
    this->_rclMesh.Adopt(meshPoints, meshFacets);

    return true;
}

/** Loads an OFF file. */
bool MeshInput::LoadOFF(std::istream& input)
{
    // http://edutechwiki.unige.ch/en/3D_file_format
    boost::regex rx_n(R"(^\s*([0-9]+)\s+([0-9]+)\s+([0-9]+)\s*$)");
    boost::cmatch what;

    bool colorPerVertex = false;
    std::vector<Base::Color> diffuseColor;
    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;

    std::string line;
    MeshFacet item;

    if (!input || input.bad()) {
        return false;
    }

    std::streambuf* buf = input.rdbuf();
    if (!buf) {
        return false;
    }

    std::getline(input, line);
    boost::algorithm::to_lower(line);
    if (line.find("coff") != std::string::npos) {
        // we expect colors to be there per vertex: x y z r g b a
        colorPerVertex = true;
    }
    else if (line.find("off") == std::string::npos) {
        return false;  // not an OFF file
    }

    // get number of vertices and faces
    int numPoints = 0, numFaces = 0;

    while (true) {
        std::getline(input, line);
        boost::algorithm::to_lower(line);
        if (boost::regex_match(line.c_str(), what, rx_n)) {
            numPoints = std::atoi(what[1].first);
            numFaces = std::atoi(what[2].first);
            break;
        }
    }

    if (numPoints == 0 || numFaces == 0) {
        return false;
    }

    meshPoints.reserve(numPoints);
    meshFacets.reserve(numFaces);
    if (colorPerVertex) {
        diffuseColor.reserve(numPoints);
    }
    else {
        diffuseColor.reserve(numFaces);
    }

    int cntPoints = 0;
    while (cntPoints < numPoints) {
        if (!std::getline(input, line)) {
            break;
        }
        std::istringstream str(line);
        str.unsetf(std::ios_base::skipws);
        str >> std::ws;
        if (str.eof()) {
            continue;  // empty line
        }

        float fX {}, fY {}, fZ {};
        str >> fX >> std::ws >> fY >> std::ws >> fZ;
        if (str) {
            meshPoints.push_back(MeshPoint(Base::Vector3f(fX, fY, fZ)));
            cntPoints++;

            if (colorPerVertex) {
                std::size_t pos = std::size_t(str.tellg());
                if (line.size() > pos) {
                    float r {}, g {}, b {}, a {};
                    str >> std::ws >> r >> std::ws >> g >> std::ws >> b;
                    if (str) {
                        str >> std::ws >> a;
                        // no transparency
                        if (!str) {
                            a = 1.0F;
                        }

                        if (r > 1.0F || g > 1.0F || b > 1.0F || a > 1.0F) {
                            r = static_cast<float>(r) / 255.0F;
                            g = static_cast<float>(g) / 255.0F;
                            b = static_cast<float>(b) / 255.0F;
                            a = static_cast<float>(a) / 255.0F;
                        }
                        diffuseColor.emplace_back(r, g, b, a);
                    }
                }
            }
        }
    }

    int cntFaces = 0;
    while (cntFaces < numFaces) {
        if (!std::getline(input, line)) {
            break;
        }
        std::istringstream str(line);
        str.unsetf(std::ios_base::skipws);
        str >> std::ws;
        if (str.eof()) {
            continue;  // empty line
        }
        int count {}, index {};
        str >> count;
        if (count >= 3) {
            std::vector<int> faces;
            faces.reserve(count);

            for (int i = 0; i < count; i++) {
                str >> std::ws;
                str >> index;
                faces.push_back(index);
            }

            for (int i = 0; i < count - 2; i++) {
                item.SetVertices(faces[0], faces[i + 1], faces[i + 2]);
                meshFacets.push_back(item);
            }
            cntFaces++;

            std::size_t pos = std::size_t(str.tellg());
            if (line.size() > pos) {
                float r {}, g {}, b {}, a {};
                str >> std::ws >> r >> std::ws >> g >> std::ws >> b;
                if (str) {
                    str >> std::ws >> a;
                    // no transparency
                    if (!str) {
                        a = 1.0F;
                    }

                    if (r > 1.0F || g > 1.0F || b > 1.0F || a > 1.0F) {
                        r = static_cast<float>(r) / 255.0F;
                        g = static_cast<float>(g) / 255.0F;
                        b = static_cast<float>(b) / 255.0F;
                        a = static_cast<float>(a) / 255.0F;
                    }
                    for (int i = 0; i < count - 2; i++) {
                        diffuseColor.emplace_back(r, g, b, a);
                    }
                }
            }
        }
    }

    if (_material) {
        if (colorPerVertex) {
            if (meshPoints.size() == diffuseColor.size()) {
                _material->binding = MeshIO::PER_VERTEX;
                _material->diffuseColor.swap(diffuseColor);
            }
        }
        else {
            if (meshFacets.size() == diffuseColor.size()) {
                _material->binding = MeshIO::PER_FACE;
                _material->diffuseColor.swap(diffuseColor);
            }
        }
    }
    this->_rclMesh.Clear();  // remove all data before

    MeshCleanup meshCleanup(meshPoints, meshFacets);
    if (_material) {
        meshCleanup.SetMaterial(_material);
    }
    meshCleanup.RemoveInvalids();
    MeshPointFacetAdjacency meshAdj(meshPoints.size(), meshFacets);
    meshAdj.SetFacetNeighbourhood();
    this->_rclMesh.Adopt(meshPoints, meshFacets);

    return true;
}

bool MeshInput::LoadPLY(std::istream& input)
{
    ReaderPLY reader(this->_rclMesh, this->_material);
    return reader.Load(input);
}

bool MeshInput::LoadMeshNode(std::istream& input)
{
    boost::regex rx_p(
        "^v\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
        "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
        "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$"
    );
    boost::regex rx_f(R"(^f\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s*$)");
    boost::regex rx_e("\\s*]\\s*");
    boost::cmatch what;

    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;

    std::string line;
    float fX {}, fY {}, fZ {};
    unsigned int i1 = 1, i2 = 1, i3 = 1;
    MeshGeomFacet clFacet;

    if (!input || input.bad()) {
        return false;
    }

    std::streambuf* buf = input.rdbuf();
    if (!buf) {
        return false;
    }

    while (std::getline(input, line)) {
        boost::algorithm::to_lower(line);
        if (boost::regex_match(line.c_str(), what, rx_p)) {
            fX = (float)std::atof(what[1].first);
            fY = (float)std::atof(what[4].first);
            fZ = (float)std::atof(what[7].first);
            meshPoints.push_back(MeshPoint(Base::Vector3f(fX, fY, fZ)));
        }
        else if (boost::regex_match(line.c_str(), what, rx_f)) {
            i1 = std::atoi(what[1].first);
            i2 = std::atoi(what[2].first);
            i3 = std::atoi(what[3].first);
            meshFacets.push_back(MeshFacet(i1 - 1, i2 - 1, i3 - 1));
        }
        else if (boost::regex_match(line.c_str(), what, rx_e)) {
            break;
        }
    }

    this->_rclMesh.Clear();  // remove all data before

    MeshCleanup meshCleanup(meshPoints, meshFacets);
    meshCleanup.RemoveInvalids();
    MeshPointFacetAdjacency meshAdj(meshPoints.size(), meshFacets);
    meshAdj.SetFacetNeighbourhood();
    this->_rclMesh.Adopt(meshPoints, meshFacets);

    return true;
}

/** Loads an ASCII STL file. */
bool MeshInput::LoadAsciiSTL(std::istream& input)
{
    boost::regex rx_p(
        "^\\s*VERTEX\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
        "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
        "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$"
    );
    boost::regex rx_f(
        "^\\s*FACET\\s+NORMAL\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
        "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
        "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$"
    );
    boost::cmatch what;

    std::string line;
    float fX {}, fY {}, fZ {};
    unsigned long ulVertexCt {}, ulFacetCt {};
    MeshGeomFacet clFacet;

    if (!input || input.bad()) {
        return false;
    }

    std::streamoff ulSize = 0;
    std::streambuf* buf = input.rdbuf();
    ulSize = buf->pubseekoff(0, std::ios::end, std::ios::in);
    buf->pubseekoff(0, std::ios::beg, std::ios::in);
    ulSize -= 20;

    // count facets
    while (std::getline(input, line)) {
        boost::algorithm::to_upper(line);
        if (line.find("ENDFACET") != std::string::npos) {
            ulFacetCt++;
        }
        // prevent from reading EOF (as I don't know how to reread the file then)
        if (input.tellg() > ulSize) {
            break;
        }
        if (line.find("ENDSOLID") != std::string::npos) {
            break;
        }
    }

    // restart from the beginning
    buf->pubseekoff(0, std::ios::beg, std::ios::in);

#if 0
    MeshBuilder builder(this->_rclMesh);
#else
    MeshFastBuilder builder(this->_rclMesh);
#endif
    builder.Initialize(ulFacetCt);

    ulVertexCt = 0;
    while (std::getline(input, line)) {
        boost::algorithm::to_upper(line);
        if (boost::regex_match(line.c_str(), what, rx_f)) {
            fX = (float)std::atof(what[1].first);
            fY = (float)std::atof(what[4].first);
            fZ = (float)std::atof(what[7].first);
            clFacet.SetNormal(Base::Vector3f(fX, fY, fZ));
        }
        else if (boost::regex_match(line.c_str(), what, rx_p)) {
            fX = (float)std::atof(what[1].first);
            fY = (float)std::atof(what[4].first);
            fZ = (float)std::atof(what[7].first);
            clFacet._aclPoints[ulVertexCt++].Set(fX, fY, fZ);
            if (ulVertexCt == 3) {
                ulVertexCt = 0;
                builder.AddFacet(clFacet);
            }
        }
    }

    builder.Finish();

    return true;
}

/** Loads a binary STL file. */
bool MeshInput::LoadBinarySTL(std::istream& input)
{
    char szInfo[80];
    Base::Vector3f clVects[4];
    uint16_t usAtt = 0;
    uint32_t ulCt = 0;

    if (!input || input.bad()) {
        return false;
    }

    // Header-Info ueberlesen
    input.read(szInfo, sizeof(szInfo));

    // Anzahl Facets
    input.read((char*)&ulCt, sizeof(ulCt));
    if (input.bad()) {
        return false;
    }

    // get file size and calculate the number of facets
    std::streamoff ulSize = 0;
    std::streambuf* buf = input.rdbuf();
    if (buf) {
        std::streamoff ulCurr {};
        ulCurr = buf->pubseekoff(0, std::ios::cur, std::ios::in);
        ulSize = buf->pubseekoff(0, std::ios::end, std::ios::in);
        buf->pubseekoff(ulCurr, std::ios::beg, std::ios::in);
    }

    uint32_t ulFac = (ulSize - (80 + sizeof(uint32_t))) / 50;

    // compare the calculated with the read value
    if (ulCt > ulFac) {
        return false;  // not a valid STL file
    }

#if 0
    MeshBuilder builder(this->_rclMesh);
#else
    MeshFastBuilder builder(this->_rclMesh);
#endif
    builder.Initialize(ulCt);

    for (uint32_t i = 0; i < ulCt; i++) {
        // read normal, points
        input.read((char*)&clVects, sizeof(clVects));

        std::swap(clVects[0], clVects[3]);
        builder.AddFacet(clVects);

        // overread 2 bytes attribute
        input.read((char*)&usAtt, sizeof(usAtt));
    }

    builder.Finish();

    return true;
}

/** Loads the mesh object from an XML file. */
void MeshInput::LoadXML(Base::XMLReader& reader)
{
    MeshPointArray cPoints;
    MeshFacetArray cFacets;

    //  reader.readElement("Mesh");

    reader.readElement("Points");
    int Cnt = reader.getAttribute<long>("Count");

    cPoints.resize(Cnt);
    for (int i = 0; i < Cnt; i++) {
        reader.readElement("P");
        cPoints[i].x = (float)reader.getAttribute<double>("x");
        cPoints[i].y = (float)reader.getAttribute<double>("y");
        cPoints[i].z = (float)reader.getAttribute<double>("z");
    }
    reader.readEndElement("Points");

    reader.readElement("Faces");
    Cnt = reader.getAttribute<long>("Count");

    cFacets.resize(Cnt);
    for (int i = 0; i < Cnt; i++) {
        reader.readElement("F");
        cFacets[i]._aulPoints[0] = reader.getAttribute<long>("p0");
        cFacets[i]._aulPoints[1] = reader.getAttribute<long>("p1");
        cFacets[i]._aulPoints[2] = reader.getAttribute<long>("p2");
        cFacets[i]._aulNeighbours[0] = reader.getAttribute<long>("n0");
        cFacets[i]._aulNeighbours[1] = reader.getAttribute<long>("n1");
        cFacets[i]._aulNeighbours[2] = reader.getAttribute<long>("n2");
    }

    reader.readEndElement("Faces");
    reader.readEndElement("Mesh");

    _rclMesh.Adopt(cPoints, cFacets);
}

/** Loads a 3MF file. */
bool MeshInput::Load3MF(std::istream& input)
{
    Reader3MF reader(input);
    reader.Load();
    std::vector<int> ids = reader.GetMeshIds();
    if (!ids.empty()) {
        MeshKernel compound = reader.GetMesh(ids[0]);
        compound.Transform(reader.GetTransform(ids[0]));

        for (std::size_t index = 1; index < ids.size(); index++) {
            MeshKernel mesh = reader.GetMesh(ids[index]);
            mesh.Transform(reader.GetTransform(ids[index]));
            compound.Merge(mesh);
        }

        _rclMesh = compound;
        return true;
    }

    return false;
}

/** Loads an OpenInventor file. */
bool MeshInput::LoadInventor(std::istream& input)
{
    Base::InventorLoader loader(input);
    if (!loader.read()) {
        return false;
    }

    if (!loader.isValid()) {
        return false;
    }

    const auto& points = loader.getPoints();
    const auto& faces = loader.getFaces();

    MeshPointArray meshPoints;
    meshPoints.reserve(points.size());
    std::transform(
        points.begin(),
        points.end(),
        std::back_inserter(meshPoints),
        [](const Base::Vector3f& v) { return MeshPoint(v); }
    );

    MeshFacetArray meshFacets;
    meshFacets.reserve(faces.size());
    std::transform(
        faces.begin(),
        faces.end(),
        std::back_inserter(meshFacets),
        [](const Base::InventorLoader::Face& f) { return MeshFacet(f.p1, f.p2, f.p3); }
    );

    MeshCleanup meshCleanup(meshPoints, meshFacets);
    meshCleanup.RemoveInvalids();
    MeshPointFacetAdjacency meshAdj(meshPoints.size(), meshFacets);
    meshAdj.SetFacetNeighbourhood();
    this->_rclMesh.Adopt(meshPoints, meshFacets);

    if (loader.isNonIndexed()) {
        if (!MeshEvalDuplicatePoints(this->_rclMesh).Evaluate()) {
            MeshFixDuplicatePoints(this->_rclMesh).Fixup();
        }
    }

    return true;
}

/** Loads a Nastran file. */
bool MeshInput::LoadNastran(std::istream& input)
{
    if (!input || input.bad()) {
        return false;
    }

    boost::regex rx_t(
        "\\s*CTRIA3\\s+([0-9]+)\\s+([0-9]+)"
        "\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s*"
    );
    boost::regex rx_q(
        "\\s*CQUAD4\\s+([0-9]+)\\s+([0-9]+)"
        "\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s*"
    );
    boost::cmatch what;

    std::string line;
    MeshFacet clMeshFacet;
    MeshPointArray vVertices;
    MeshFacetArray vTriangle;

    int index {};
    std::map<int, NODE> mNode;
    std::map<int, TRIA> mTria;
    std::map<int, QUAD> mQuad;

    int badElementCounter = 0;

    while (std::getline(input, line)) {
        boost::algorithm::to_upper(ltrim(line));
        if (line.empty()) {
            // Skip all the following tests
        }
        else if (line.rfind("GRID*", 0) == 0) {
            // This element is the 16-digit-precision GRID element, which occupies two lines of the
            // card. Note that FreeCAD discards the extra precision, downcasting to an four-byte
            // float.
            //
            // The two lines are:
            // 1      8               24             40             56
            // GRID*  Index(16)       Blank(16)      x(16)          y(at least one)
            // *      z(at least one)
            //
            // The first character is typically the sign, and may be omitted for positive numbers,
            // so it is possible for a field to begin with a blank. Trailing zeros may be omitted,
            // so a field may also end with blanks. No space or other delimiter is required between
            // the numbers. The following is a valid NASTRAN GRID* element:
            //
            // GRID*  1                               0.1234567890120.
            // *      1.
            //
            // Element type(8), index(16), empty(16), x(16), y(>=1)
            if (line.length() < 8 + 16 + 16 + 16 + 1) {
                badElementCounter++;
                continue;
            }
            auto indexView = std::string_view(&line[8], 16);
            // auto blankView = std::string_view(&line[8+16], 16); // No data is needed here
            auto xView = std::string_view(&line[8 + 16 + 16], 16);
            auto yView = std::string_view(&line[8 + 16 + 16 + 16]);

            std::string line2;
            std::getline(input, line2);
            if ((!line2.empty() && line2[0] != '*') || line2.length() < 9) {
                badElementCounter++;
                continue;  // File format error: second line is not a continuation line
            }
            auto zView = std::string_view(&line2[8]);

            // We have to strip off any whitespace (technically really just any *trailing*
            // whitespace):
            auto indexString = boost::trim_copy(std::string(indexView));
            auto xString = boost::trim_copy(std::string(xView));
            auto yString = boost::trim_copy(std::string(yView));
            auto zString = boost::trim_copy(std::string(zView));

            auto converter = boost::cnv::spirit();
            auto indexCheck = boost::convert<int>(indexString, converter);
            if (!indexCheck.is_initialized()) {
                // File format error: index couldn't be converted to an integer
                badElementCounter++;
                continue;
            }
            index = indexCheck.get() - 1;  // Minus one so we are zero-indexed to match existing code

            // Get the high-precision versions first
            auto x = boost::convert<double>(xString, converter);
            auto y = boost::convert<double>(yString, converter);
            auto z = boost::convert<double>(zString, converter);

            if (!x.is_initialized() || !y.is_initialized() || !z.is_initialized()) {
                // File format error: x, y or z could not be converted
                badElementCounter++;
                continue;
            }

            // Now drop precision:
            mNode[index].x = (float)x.get();
            mNode[index].y = (float)y.get();
            mNode[index].z = (float)z.get();
        }
        else if (line.rfind("GRID", 0) == 0) {

            boost::regex rx_spaceDelimited(
                "\\s*GRID\\s+([0-9]+)"
                "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*"
            );

            if (boost::regex_match(line.c_str(), what, rx_spaceDelimited)) {
                // insert the read-in vertex into a map to preserve the order
                index = std::atol(what[1].first) - 1;
                mNode[index].x = (float)std::atof(what[2].first);
                mNode[index].y = (float)std::atof(what[5].first);
                mNode[index].z = (float)std::atof(what[8].first);
            }
            else {
                // Classic NASTRAN uses a fixed 8 character field width:
                // 1       8       16      24      32      40
                // $-------ID------CP------X1------X2------X3------CD------PS------9-------+-------
                // GRID    1               1.2345671.2345671.234567
                // GRID    112             6.0000000.5000000.00E+00

                // Element type(8), id(8), cp(8), x(8), y(8), z(at least 1)
                if (line.length() < 41) {
                    badElementCounter++;
                    continue;
                }
                auto indexView = std::string_view(&line[8], 8);
                auto xView = std::string_view(&line[24], 8);
                auto yView = std::string_view(&line[32], 8);
                auto zView = std::string_view(&line[40], 8);

                auto indexString = boost::trim_copy(std::string(indexView));
                auto xString = boost::trim_copy(std::string(xView));
                auto yString = boost::trim_copy(std::string(yView));
                auto zString = boost::trim_copy(std::string(zView));

                auto converter = boost::cnv::spirit();
                auto indexCheck = boost::convert<int>(indexString, converter);
                if (!indexCheck.is_initialized()) {
                    // File format error: index couldn't be converted to an integer
                    badElementCounter++;
                    continue;
                }
                // Minus one so we are zero-indexed to match existing code
                index = indexCheck.get() - 1;

                auto x = boost::convert<float>(xString, converter);
                auto y = boost::convert<float>(yString, converter);
                auto z = boost::convert<float>(zString, converter);

                if (!x.is_initialized() || !y.is_initialized() || !z.is_initialized()) {
                    // File format error: x, y or z could not be converted
                    badElementCounter++;
                    continue;
                }

                mNode[index].x = x.get();
                mNode[index].y = y.get();
                mNode[index].z = z.get();
            }
        }
        else if (line.rfind("CTRIA3 ", 0) == 0) {
            if (boost::regex_match(line.c_str(), what, rx_t)) {
                // insert the read-in triangle into a map to preserve the order
                index = std::atol(what[1].first) - 1;
                mTria[index].iV[0] = std::atol(what[3].first) - 1;
                mTria[index].iV[1] = std::atol(what[4].first) - 1;
                mTria[index].iV[2] = std::atol(what[5].first) - 1;
            }
        }
        else if (line.rfind("CQUAD4", 0) == 0) {
            if (boost::regex_match(line.c_str(), what, rx_q)) {
                // insert the read-in quadrangle into a map to preserve the order
                index = std::atol(what[1].first) - 1;
                mQuad[index].iV[0] = std::atol(what[3].first) - 1;
                mQuad[index].iV[1] = std::atol(what[4].first) - 1;
                mQuad[index].iV[2] = std::atol(what[5].first) - 1;
                mQuad[index].iV[3] = std::atol(what[6].first) - 1;
            }
        }
    }

    if (badElementCounter > 0) {
        Base::Console().warning("Found bad elements while reading NASTRAN file.\n");
    }

    // Check the triangles to make sure the vertices they refer to actually exist:
    for (const auto& tri : mTria) {
        for (int i : tri.second.iV) {
            if (mNode.find(i) == mNode.end()) {
                Base::Console().error(
                    "CTRIA3 element refers to a node that does not exist, or could not be read.\n"
                );
                return false;
            }
        }
    }

    // Check the quads to make sure the vertices they refer to actually exist:
    for (const auto& quad : mQuad) {
        for (int i : quad.second.iV) {
            if (mNode.find(i) == mNode.end()) {
                Base::Console().error(
                    "CQUAD4 element refers to a node that does not exist, or could not be read.\n"
                );
                return false;
            }
        }
    }

    float fLength[2];
    if (mTria.empty()) {
        index = 0;
    }
    else {
        index = mTria.rbegin()->first + 1;
    }
    for (const auto& QI : mQuad) {
        for (int i = 0; i < 2; i++) {
            float fDx = mNode[QI.second.iV[i + 2]].x - mNode[QI.second.iV[i]].x;
            float fDy = mNode[QI.second.iV[i + 2]].y - mNode[QI.second.iV[i]].y;
            float fDz = mNode[QI.second.iV[i + 2]].z - mNode[QI.second.iV[i]].z;
            fLength[i] = fDx * fDx + fDy * fDy + fDz * fDz;
        }
        if (fLength[0] < fLength[1]) {
            mTria[index].iV[0] = QI.second.iV[0];
            mTria[index].iV[1] = QI.second.iV[1];
            mTria[index].iV[2] = QI.second.iV[2];

            mTria[index + 1].iV[0] = QI.second.iV[0];
            mTria[index + 1].iV[1] = QI.second.iV[2];
            mTria[index + 1].iV[2] = QI.second.iV[3];
        }
        else {
            mTria[index].iV[0] = QI.second.iV[0];
            mTria[index].iV[1] = QI.second.iV[1];
            mTria[index].iV[2] = QI.second.iV[3];

            mTria[index + 1].iV[0] = QI.second.iV[1];
            mTria[index + 1].iV[1] = QI.second.iV[2];
            mTria[index + 1].iV[2] = QI.second.iV[3];
        }

        index += 2;
    }

    // Applying the nodes
    vVertices.reserve(mNode.size());
    for (const auto& MI : mNode) {
        vVertices.push_back(Base::Vector3f(MI.second.x, MI.second.y, MI.second.z));
    }

    // Converting data to Mesh. Negative conversion for right orientation of normal-vectors.
    vTriangle.reserve(mTria.size());
    for (const auto& MI : mTria) {
        clMeshFacet._aulPoints[0] = MI.second.iV[1];
        clMeshFacet._aulPoints[1] = MI.second.iV[0];
        clMeshFacet._aulPoints[2] = MI.second.iV[2];
        vTriangle.push_back(clMeshFacet);
    }

    // make sure to add only vertices which are referenced by the triangles
    _rclMesh.Merge(vVertices, vTriangle);

    return true;
}

/** Loads a Cadmould FE file. */
bool MeshInput::LoadCadmouldFE(std::ifstream& input)
{
    if (!input || input.bad()) {
        return false;
    }
    assert(0);
    return false;
}

// --------------------------------------------------------------

std::string MeshOutput::stl_header = "MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-"
                                     "MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH\n";

void MeshOutput::SetSTLHeaderData(const std::string& header)
{
    if (header.size() > 80) {
        stl_header = header.substr(0, 80);
    }
    else if (header.size() < 80) {
        std::fill(stl_header.begin(), stl_header.end(), ' ');
        std::copy(header.begin(), header.end(), stl_header.begin());
    }
    else {
        stl_header = header;
    }
}

std::string MeshOutput::asyWidth = "500";
std::string MeshOutput::asyHeight = "500";

void MeshOutput::SetAsymptoteSize(const std::string& w, const std::string& h)
{
    asyWidth = w;
    asyHeight = h;
}

void MeshOutput::Transform(const Base::Matrix4D& mat)
{
    _transform = mat;
    if (mat != Base::Matrix4D()) {
        apply_transform = true;
    }
}

std::vector<std::string> MeshOutput::supportedMeshFormats()
{
    std::vector<std::string> fmt;
    fmt.emplace_back("bms");
    fmt.emplace_back("ply");
    fmt.emplace_back("stl");
    fmt.emplace_back("obj");
    fmt.emplace_back("off");
    fmt.emplace_back("smf");
    fmt.emplace_back("x3d");
    fmt.emplace_back("x3dz");
    fmt.emplace_back("xhtml");
    fmt.emplace_back("wrl");
    fmt.emplace_back("wrz");
    fmt.emplace_back("amf");
    fmt.emplace_back("asy");
    fmt.emplace_back("3mf");
    return fmt;
}

MeshIO::Format MeshOutput::GetFormat(const char* FileName)
{
    Base::FileInfo file(FileName);
    if (file.hasExtension("bms")) {
        return MeshIO::BMS;
    }
    if (file.hasExtension("stl")) {
        return MeshIO::BSTL;
    }
    if (file.hasExtension("ast")) {
        return MeshIO::ASTL;
    }
    if (file.hasExtension("obj")) {
        return MeshIO::OBJ;
    }
    if (file.hasExtension("off")) {
        return MeshIO::OFF;
    }
    if (file.hasExtension("ply")) {
        return MeshIO::PLY;
    }
    if (file.hasExtension("idtf")) {
        return MeshIO::IDTF;
    }
    if (file.hasExtension("mgl")) {
        return MeshIO::MGL;
    }
    if (file.hasExtension("iv")) {
        return MeshIO::IV;
    }
    if (file.hasExtension("x3d")) {
        return MeshIO::X3D;
    }
    if (file.hasExtension("x3dz")) {
        return MeshIO::X3DZ;
    }
    if (file.hasExtension("xhtml")) {
        return MeshIO::X3DOM;
    }
    if (file.hasExtension("py")) {
        return MeshIO::PY;
    }
    if (file.hasExtension({"wrl", "vrml"})) {
        return MeshIO::VRML;
    }
    if (file.hasExtension("wrz")) {
        return MeshIO::WRZ;
    }
    if (file.hasExtension({"nas", "bdf"})) {
        return MeshIO::NAS;
    }
    if (file.hasExtension("amf")) {
        return MeshIO::AMF;
    }
    if (file.hasExtension("3mf")) {
        return MeshIO::ThreeMF;
    }
    if (file.hasExtension("smf")) {
        return MeshIO::SMF;
    }
    if (file.hasExtension("asy")) {
        return MeshIO::ASY;
    }

    return MeshIO::Undefined;
}

/// Save in a file, format is decided by the extension if not explicitly given
bool MeshOutput::SaveAny(const char* FileName, MeshIO::Format format) const
{
    // ask for write permission
    Base::FileInfo file(FileName);
    Base::FileInfo directory(file.dirPath());
    if (!directory.exists()) {
        throw Base::FileException("Directory does not exist", FileName);
    }
    if ((file.exists() && !file.isWritable()) || !directory.isWritable()) {
        throw Base::FileException("No write permission for file", FileName);
    }

    MeshIO::Format fileformat = format;
    if (fileformat == MeshIO::Undefined) {
        fileformat = GetFormat(FileName);
    }

    Base::ofstream str(file, std::ios::out | std::ios::binary);

    if (fileformat == MeshIO::BMS) {
        _rclMesh.Write(str);
    }
    else if (fileformat == MeshIO::BSTL) {
        MeshOutput aWriter(_rclMesh);
        aWriter.Transform(this->_transform);

        // write file
        bool ok = false;
        ok = aWriter.SaveBinarySTL(str);
        if (!ok) {
            throw Base::FileException("Export of STL mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::ASTL) {
        MeshOutput aWriter(_rclMesh);
        aWriter.SetObjectName(objectName);
        aWriter.Transform(this->_transform);

        // write file
        bool ok = false;
        ok = aWriter.SaveAsciiSTL(str);
        if (!ok) {
            throw Base::FileException("Export of STL mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::OBJ) {
        // write file
        if (!SaveOBJ(str, FileName)) {
            throw Base::FileException("Export of OBJ mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::SMF) {
        // write file
        if (!SaveSMF(str)) {
            throw Base::FileException("Export of SMF mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::OFF) {
        // write file
        if (!SaveOFF(str)) {
            throw Base::FileException("Export of OFF mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::PLY) {
        // write file
        if (!SaveBinaryPLY(str)) {
            throw Base::FileException("Export of PLY mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::APLY) {
        // write file
        if (!SaveAsciiPLY(str)) {
            throw Base::FileException("Export of PLY mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::IDTF) {
        // write file
        if (!SaveIDTF(str)) {
            throw Base::FileException("Export of IDTF mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::MGL) {
        // write file
        if (!SaveMGL(str)) {
            throw Base::FileException("Export of MGL mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::IV) {
        // write file
        if (!SaveInventor(str)) {
            throw Base::FileException("Export of Inventor mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::X3D) {
        // write file
        if (!SaveX3D(str)) {
            throw Base::FileException("Export of X3D failed", FileName);
        }
    }
    else if (fileformat == MeshIO::X3DZ) {
        // Compressed X3D is nothing else than a GZIP'ped X3D ascii file
        zipios::GZIPOutputStream gzip(str);
        // write file
        if (!SaveX3D(gzip)) {
            throw Base::FileException("Export of compressed X3D mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::X3DOM) {
        // write file
        if (!SaveX3DOM(str)) {
            throw Base::FileException("Export of X3DOM failed", FileName);
        }
    }
    else if (fileformat == MeshIO::ThreeMF) {
        // write file
        if (!Save3MF(str)) {
            throw Base::FileException("Export of 3MF failed", FileName);
        }
    }
    else if (fileformat == MeshIO::PY) {
        // write file
        if (!SavePython(str)) {
            throw Base::FileException("Export of Python mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::VRML) {
        // write file
        if (!SaveVRML(str)) {
            throw Base::FileException("Export of VRML mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::WRZ) {
        // Compressed VRML is nothing else than a GZIP'ped VRML ascii file
        // str.close();
        // Base::ogzstream gzip(FileName, std::ios::out | std::ios::binary);
        // Hint: The compression level seems to be higher than with ogzstream
        // which leads to problems to load the wrz file in debug mode, the
        // application simply crashes.
        zipios::GZIPOutputStream gzip(str);
        // write file
        if (!SaveVRML(gzip)) {
            throw Base::FileException("Export of compressed VRML mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::NAS) {
        // write file
        if (!SaveNastran(str)) {
            throw Base::FileException("Export of NASTRAN mesh failed", FileName);
        }
    }
    else if (fileformat == MeshIO::ASY) {
        // write file
        if (!SaveAsymptote(str)) {
            throw Base::FileException("Export of ASY mesh failed", FileName);
        }
    }
    else {
        throw Base::FileException("File format not supported", FileName);
    }

    return true;
}

bool MeshOutput::SaveFormat(std::ostream& str, MeshIO::Format fmt) const
{
    switch (fmt) {
        case MeshIO::BMS:
            _rclMesh.Write(str);
            return true;
        case MeshIO::ASTL:
            return SaveAsciiSTL(str);
        case MeshIO::BSTL:
            return SaveBinarySTL(str);
        case MeshIO::OBJ:
            return SaveOBJ(str);
        case MeshIO::SMF:
            return SaveSMF(str);
        case MeshIO::OFF:
            return SaveOFF(str);
        case MeshIO::IDTF:
            return SaveIDTF(str);
        case MeshIO::MGL:
            return SaveMGL(str);
        case MeshIO::IV:
            return SaveInventor(str);
        case MeshIO::X3D:
            return SaveX3D(str);
        case MeshIO::X3DOM:
            return SaveX3DOM(str);
        case MeshIO::VRML:
            return SaveVRML(str);
        case MeshIO::WRZ:
            // it's up to the client to create the needed stream
            return SaveVRML(str);
        case MeshIO::ThreeMF:
            return Save3MF(str);
        case MeshIO::NAS:
            return SaveNastran(str);
        case MeshIO::PLY:
            return SaveBinaryPLY(str);
        case MeshIO::APLY:
            return SaveAsciiPLY(str);
        case MeshIO::PY:
            return SavePython(str);
        case MeshIO::ASY:
            return SaveAsymptote(str);
        default:
            throw Base::FileException("Unsupported file format");
    }
}

/** Saves the mesh object into an ASCII file. */
bool MeshOutput::SaveAsciiSTL(std::ostream& output) const
{
    MeshFacetIterator clIter(_rclMesh), clEnd(_rclMesh);
    clIter.Transform(this->_transform);
    const MeshGeomFacet* pclFacet {};

    if (!output || output.bad() || _rclMesh.CountFacets() == 0) {
        return false;
    }

    output.precision(6);
    output.setf(std::ios::fixed | std::ios::showpoint);
    Base::SequencerLauncher seq("saving...", _rclMesh.CountFacets() + 1);

    if (this->objectName.empty()) {
        output << "solid Mesh\n";
    }
    else {
        output << "solid " << this->objectName << '\n';
    }

    clIter.Begin();
    clEnd.End();
    while (clIter < clEnd) {
        pclFacet = &(*clIter);

        // normal
        output << "  facet normal " << pclFacet->GetNormal().x << " " << pclFacet->GetNormal().y
               << " " << pclFacet->GetNormal().z << '\n';
        output << "    outer loop\n";

        // vertices
        for (const auto& pnt : pclFacet->_aclPoints) {
            output << "      vertex " << pnt.x << " " << pnt.y << " " << pnt.z << '\n';
        }

        output << "    endloop\n";
        output << "  endfacet\n";

        ++clIter;
        seq.next(true);  // allow to cancel
    }

    output << "endsolid Mesh\n";

    return true;
}

/** Saves the mesh object into a binary file. */
bool MeshOutput::SaveBinarySTL(std::ostream& output) const
{
    MeshFacetIterator clIter(_rclMesh), clEnd(_rclMesh);
    clIter.Transform(this->_transform);
    const MeshGeomFacet* pclFacet {};
    uint16_t usAtt {};
    char szInfo[81];

    if (!output || output.bad() /*|| _rclMesh.CountFacets() == 0*/) {
        return false;
    }

    Base::SequencerLauncher seq("saving...", _rclMesh.CountFacets() + 1);

    // stl_header has a length of 80
    strcpy(szInfo, stl_header.c_str());
    output.write(szInfo, std::strlen(szInfo));

    uint32_t uCtFts = (uint32_t)_rclMesh.CountFacets();
    output.write((const char*)&uCtFts, sizeof(uCtFts));

    usAtt = 0;
    clIter.Begin();
    clEnd.End();
    while (clIter < clEnd) {
        pclFacet = &(*clIter);
        // normal
        Base::Vector3f normal = pclFacet->GetNormal();
        output.write((const char*)&(normal.x), sizeof(float));
        output.write((const char*)&(normal.y), sizeof(float));
        output.write((const char*)&(normal.z), sizeof(float));

        // vertices
        for (uint32_t i = 0; i < 3; i++) {
            output.write((const char*)&(pclFacet->_aclPoints[i].x), sizeof(float));
            output.write((const char*)&(pclFacet->_aclPoints[i].y), sizeof(float));
            output.write((const char*)&(pclFacet->_aclPoints[i].z), sizeof(float));
        }

        // attribute
        output.write((const char*)&usAtt, sizeof(usAtt));

        ++clIter;
        seq.next(true);  // allow one to cancel
    }

    return true;
}

/** Saves an OBJ file. */
bool MeshOutput::SaveOBJ(std::ostream& out) const
{
    WriterOBJ writer(this->_rclMesh, this->_material);
    writer.SetTransform(this->_transform);
    writer.SetGroups(this->_groups);
    return writer.Save(out);
}

bool MeshOutput::SaveOBJ(std::ostream& out, const char* filename) const
{
    WriterOBJ writer(this->_rclMesh, this->_material);
    writer.SetTransform(this->_transform);
    writer.SetGroups(this->_groups);
    if (writer.Save(out)) {
        if (this->_material && this->_material->binding == MeshCore::MeshIO::PER_FACE) {
            Base::FileInfo fi(filename);
            std::string fn = fi.dirPath() + "/" + this->_material->library;
            fi.setFile(fn);
            Base::ofstream mtl(fi, std::ios::out | std::ios::binary);
            writer.SaveMaterial(mtl);
            mtl.close();
        }

        return true;
    }

    return false;
}

/** Saves an SMF file. */
bool MeshOutput::SaveSMF(std::ostream& out) const
{
    // http://people.sc.fsu.edu/~jburkardt/data/smf/smf.txt
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();

    if (!out || out.bad()) {
        return false;
    }

    Base::SequencerLauncher seq("saving...", _rclMesh.CountPoints() + _rclMesh.CountFacets());

    // Header
    out << "#$SMF 1.0\n";
    out << "#$vertices " << rPoints.size() << '\n';
    out << "#$faces " << rFacets.size() << '\n';
    out << "#\n";
    out << "# Created by FreeCAD <https://www.freecad.org>\n";

    out.precision(6);
    out.setf(std::ios::fixed | std::ios::showpoint);

    // vertices
    Base::Vector3f pt;
    std::size_t index = 0;
    for (auto it = rPoints.begin(); it != rPoints.end(); ++it, ++index) {
        if (this->apply_transform) {
            pt = this->_transform * *it;
        }
        else {
            pt.Set(it->x, it->y, it->z);
        }

        out << "v " << pt.x << " " << pt.y << " " << pt.z << '\n';
        seq.next(true);  // allow one to cancel
    }

    // facet indices
    for (const auto& it : rFacets) {
        out << "f " << it._aulPoints[0] + 1 << " " << it._aulPoints[1] + 1 << " "
            << it._aulPoints[2] + 1 << '\n';
        seq.next(true);  // allow one to cancel
    }

    return true;
}

/** Saves an Asymptote file. */
bool MeshOutput::SaveAsymptote(std::ostream& out) const
{
    out << "/*\n"
           " * Created by FreeCAD <https://www.freecad.org>\n"
           " */\n\n";

    out << "import three;\n\n";

    if (!asyWidth.empty()) {
        out << "size(" << asyWidth;
        if (!asyHeight.empty()) {
            out << ", " << asyHeight;
        }
        out << ");\n\n";
    }

    Base::BoundBox3f bbox = _rclMesh.GetBoundBox();
    Base::Vector3f center = bbox.GetCenter();
    this->_transform.multVec(center, center);
    Base::Vector3f camera(center);
    camera.x += std::max<float>(std::max<float>(bbox.LengthX(), bbox.LengthY()), bbox.LengthZ());
    Base::Vector3f target(center);
    Base::Vector3f upvec(0.0F, 0.0F, 1.0F);

    out << "// CA:Camera, OB:Camera\n"
        << "currentprojection = orthographic(camera = (" << camera.x << ", " << camera.y << ", "
        << camera.z << "),\n"
        << "                                 target = (" << target.x << ", " << target.y << ", "
        << target.z
        << "),\n"
           "                                 showtarget = false,\n"
           "                                 up = ("
        << upvec.x << ", " << upvec.y << ", " << upvec.z << "));\n\n";

    // out << "// LA:Spot, OB:Lamp\n"
    //     << "// WO:World\n"
    //     << "currentlight = light(diffuse = rgb(1, 1, 1),\n"
    //        "                     specular = rgb(1, 1, 1),\n"
    //        "                     background = rgb(0.078281, 0.16041, 0.25),\n"
    //        "                     0.56639, 0.21839, 0.79467);\n\n";

    out << "// ME:Mesh, OB:Mesh\n";

    MeshFacetIterator clIter(_rclMesh), clEnd(_rclMesh);
    clIter.Transform(this->_transform);
    clIter.Begin();
    clEnd.End();

    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    bool saveVertexColor
        = (_material && _material->binding == MeshIO::PER_VERTEX
           && _material->diffuseColor.size() == rPoints.size());
    bool saveFaceColor
        = (_material && _material->binding == MeshIO::PER_FACE
           && _material->diffuseColor.size() == rFacets.size());
    // global mesh color
    Base::Color mc(0.8F, 0.8F, 0.8F);
    if (_material && _material->binding == MeshIO::OVERALL && _material->diffuseColor.size() == 1) {
        mc = _material->diffuseColor[0];
    }

    std::size_t index = 0;
    const MeshGeomFacet* pclFacet {};
    while (clIter < clEnd) {
        pclFacet = &(*clIter);

        out << "draw(surface(";

        // vertices
        for (const auto& pnt : pclFacet->_aclPoints) {
            out << '(' << pnt.x << ", " << pnt.y << ", " << pnt.z << ")--";
        }

        out << "cycle";

        if (saveVertexColor) {
            const MeshFacet& face = rFacets[index];
            out << ",\n             new pen[] {";
            for (int i = 0; i < 3; i++) {
                const Base::Color& c = _material->diffuseColor[face._aulPoints[i]];
                out << "rgb(" << c.r << ", " << c.g << ", " << c.b << ")";
                if (i < 2) {
                    out << ", ";
                }
            }
            out << "}));\n";
        }
        else if (saveFaceColor) {
            const Base::Color& c = _material->diffuseColor[index];
            out << "),\n     rgb(" << c.r << ", " << c.g << ", " << c.b << "));\n";
        }
        else {
            out << "),\n     rgb(" << mc.r << ", " << mc.g << ", " << mc.b << "));\n";
        }

        ++clIter;
        ++index;
    }

    return true;
}

/** Saves an OFF file. */
bool MeshOutput::SaveOFF(std::ostream& out) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();

    if (!out || out.bad()) {
        return false;
    }

    Base::SequencerLauncher seq("saving...", _rclMesh.CountPoints() + _rclMesh.CountFacets());

    bool exportColor = false;
    if (_material) {
        if (_material->binding == MeshIO::PER_FACE) {
            Base::Console().warning("Cannot export color information because it's defined per face");
        }
        else if (_material->binding == MeshIO::PER_VERTEX) {
            if (_material->diffuseColor.size() != rPoints.size()) {
                Base::Console().warning(
                    "Cannot export color information because there is a "
                    "different number of points and colors"
                );
            }
            else {
                exportColor = true;
            }
        }
        else if (_material->binding == MeshIO::OVERALL) {
            if (_material->diffuseColor.empty()) {
                Base::Console().warning(
                    "Cannot export color information because there is no color defined"
                );
            }
            else {
                exportColor = true;
            }
        }
    }

    if (exportColor) {
        out << "COFF\n";
    }
    else {
        out << "OFF\n";
    }
    out << rPoints.size() << " " << rFacets.size() << " 0\n";

    // vertices
    Base::Vector3f pt;
    std::size_t index = 0;
    for (auto it = rPoints.begin(); it != rPoints.end(); ++it, ++index) {
        if (this->apply_transform) {
            pt = this->_transform * *it;
        }
        else {
            pt.Set(it->x, it->y, it->z);
        }

        if (exportColor) {
            Base::Color c;
            if (_material->binding == MeshIO::PER_VERTEX) {
                c = _material->diffuseColor[index];
            }
            else {
                c = _material->diffuseColor.front();
            }

            int r = static_cast<int>(c.r * 255.0F);
            int g = static_cast<int>(c.g * 255.0F);
            int b = static_cast<int>(c.b * 255.0F);
            int a = static_cast<int>(c.a * 255.0F);

            out << pt.x << " " << pt.y << " " << pt.z << " " << r << " " << g << " " << b << " "
                << a << '\n';
        }
        else {
            out << pt.x << " " << pt.y << " " << pt.z << '\n';
        }
        seq.next(true);  // allow one to cancel
    }

    // facet indices (no texture and normal indices)
    for (const auto& it : rFacets) {
        out << "3 " << it._aulPoints[0] << " " << it._aulPoints[1] << " " << it._aulPoints[2] << '\n';
        seq.next(true);  // allow one to cancel
    }

    return true;
}

bool MeshOutput::SaveBinaryPLY(std::ostream& out) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    std::size_t v_count = rPoints.size();
    std::size_t f_count = rFacets.size();
    if (!out || out.bad()) {
        return false;
    }
    bool saveVertexColor
        = (_material && _material->binding == MeshIO::PER_VERTEX
           && _material->diffuseColor.size() == rPoints.size());
    out << "ply\n"
        << "format binary_little_endian 1.0\n"
        << "comment Created by FreeCAD <https://www.freecad.org>\n"
        << "element vertex " << v_count << '\n'
        << "property float32 x\n"
        << "property float32 y\n"
        << "property float32 z\n";
    if (saveVertexColor) {
        out << "property uchar red\n"
            << "property uchar green\n"
            << "property uchar blue\n";
    }
    out << "element face " << f_count << '\n'
        << "property list uchar int vertex_index\n"
        << "end_header\n";

    Base::OutputStream os(out);
    os.setByteOrder(Base::Stream::LittleEndian);

    for (std::size_t i = 0; i < v_count; i++) {
        const MeshPoint& p = rPoints[i];
        if (this->apply_transform) {
            Base::Vector3f pt = this->_transform * p;
            os << pt.x << pt.y << pt.z;
        }
        else {
            os << p.x << p.y << p.z;
        }
        if (saveVertexColor) {
            const Base::Color& c = _material->diffuseColor[i];
            uint8_t r = uint8_t(255.0F * c.r);
            uint8_t g = uint8_t(255.0F * c.g);
            uint8_t b = uint8_t(255.0F * c.b);
            os << r << g << b;
        }
    }
    unsigned char n = 3;
    int f1 {}, f2 {}, f3 {};
    for (std::size_t i = 0; i < f_count; i++) {
        const MeshFacet& f = rFacets[i];
        f1 = (int)f._aulPoints[0];
        f2 = (int)f._aulPoints[1];
        f3 = (int)f._aulPoints[2];
        os << n;
        os << f1 << f2 << f3;
    }

    return true;
}

bool MeshOutput::SaveAsciiPLY(std::ostream& out) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    std::size_t v_count = rPoints.size();
    std::size_t f_count = rFacets.size();
    if (!out || out.bad()) {
        return false;
    }

    bool saveVertexColor
        = (_material && _material->binding == MeshIO::PER_VERTEX
           && _material->diffuseColor.size() == rPoints.size());
    out << "ply\n"
        << "format ascii 1.0\n"
        << "comment Created by FreeCAD <https://www.freecad.org>\n"
        << "element vertex " << v_count << '\n'
        << "property float32 x\n"
        << "property float32 y\n"
        << "property float32 z\n";
    if (saveVertexColor) {
        out << "property uchar red\n"
            << "property uchar green\n"
            << "property uchar blue\n";
    }
    out << "element face " << f_count << '\n'
        << "property list uchar int vertex_index\n"
        << "end_header\n";

    out.precision(6);
    out.setf(std::ios::fixed | std::ios::showpoint);
    if (saveVertexColor) {
        for (std::size_t i = 0; i < v_count; i++) {
            const MeshPoint& p = rPoints[i];
            if (this->apply_transform) {
                Base::Vector3f pt = this->_transform * p;
                out << pt.x << " " << pt.y << " " << pt.z;
            }
            else {
                out << p.x << " " << p.y << " " << p.z;
            }

            const Base::Color& c = _material->diffuseColor[i];
            int r = (int)(255.0F * c.r);
            int g = (int)(255.0F * c.g);
            int b = (int)(255.0F * c.b);
            out << " " << r << " " << g << " " << b << '\n';
        }
    }
    else {
        for (std::size_t i = 0; i < v_count; i++) {
            const MeshPoint& p = rPoints[i];
            if (this->apply_transform) {
                Base::Vector3f pt = this->_transform * p;
                out << pt.x << " " << pt.y << " " << pt.z << '\n';
            }
            else {
                out << p.x << " " << p.y << " " << p.z << '\n';
            }
        }
    }

    unsigned int n = 3;
    int f1 {}, f2 {}, f3 {};
    for (std::size_t i = 0; i < f_count; i++) {
        const MeshFacet& f = rFacets[i];
        f1 = (int)f._aulPoints[0];
        f2 = (int)f._aulPoints[1];
        f3 = (int)f._aulPoints[2];
        out << n << " " << f1 << " " << f2 << " " << f3 << '\n';
    }

    return true;
}

bool MeshOutput::SaveMeshNode(std::ostream& output)
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();

    if (!output || output.bad()) {
        return false;
    }

    // vertices
    output << "[" << '\n';
    if (this->apply_transform) {
        Base::Vector3f pt;
        for (const auto& it : rPoints) {
            pt = this->_transform * it;
            output << "v " << pt.x << " " << pt.y << " " << pt.z << '\n';
        }
    }
    else {
        for (const auto& it : rPoints) {
            output << "v " << it.x << " " << it.y << " " << it.z << '\n';
        }
    }
    // facet indices (no texture and normal indices)
    for (const auto& it : rFacets) {
        output << "f " << it._aulPoints[0] + 1 << " " << it._aulPoints[1] + 1 << " "
               << it._aulPoints[2] + 1 << '\n';
    }
    output << "]" << '\n';

    return true;
}

/** Saves the mesh object into an XML file. */
void MeshOutput::SaveXML(Base::Writer& writer) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();

    //  writer << writer.ind() << "<Mesh>" << '\n';

    writer.incInd();
    writer.Stream() << writer.ind() << "<Points Count=\"" << _rclMesh.CountPoints() << "\">" << '\n';

    writer.incInd();
    if (this->apply_transform) {
        Base::Vector3f pt;
        for (const auto& it : rPoints) {
            pt = this->_transform * it;
            writer.Stream() << writer.ind() << "<P "
                            << "x=\"" << pt.x << "\" "
                            << "y=\"" << pt.y << "\" "
                            << "z=\"" << pt.z << "\"/>" << '\n';
        }
    }
    else {
        for (const auto& it : rPoints) {
            writer.Stream() << writer.ind() << "<P "
                            << "x=\"" << it.x << "\" "
                            << "y=\"" << it.y << "\" "
                            << "z=\"" << it.z << "\"/>" << '\n';
        }
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Points>" << '\n';

    // write the faces
    writer.Stream() << writer.ind() << "<Faces Count=\"" << _rclMesh.CountFacets() << "\">" << '\n';

    writer.incInd();
    for (const auto& it : rFacets) {
        writer.Stream() << writer.ind() << "<F "
                        << "p0=\"" << it._aulPoints[0] << "\" "
                        << "p1=\"" << it._aulPoints[1] << "\" "
                        << "p2=\"" << it._aulPoints[2] << "\" "
                        << "n0=\"" << it._aulNeighbours[0] << "\" "
                        << "n1=\"" << it._aulNeighbours[1] << "\" "
                        << "n2=\"" << it._aulNeighbours[2] << "\"/>" << '\n';
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Faces>" << '\n';

    writer.Stream() << writer.ind() << "</Mesh>" << '\n';
    writer.decInd();
}

/** Saves the mesh object into a 3MF file. */
bool MeshOutput::Save3MF(std::ostream& output) const
{
    Writer3MF writer(output);
    writer.AddMesh(_rclMesh, _transform);
    return writer.Save();
}

/** Writes an IDTF file. */
bool MeshOutput::SaveIDTF(std::ostream& str) const
{
    if (!str || str.bad() || (_rclMesh.CountFacets() == 0)) {
        return false;
    }

    const MeshPointArray& pts = _rclMesh.GetPoints();
    const MeshFacetArray& fts = _rclMesh.GetFacets();
    std::string resource = objectName;
    if (resource.empty()) {
        resource = "Resource";
    }

    str.precision(6);
    str.setf(std::ios::fixed | std::ios::showpoint);

    str << "FILE_FORMAT \"IDTF\"\n"
        << "FORMAT_VERSION 100\n\n";

    str << Base::tabs(0) << "NODE \"MODEL\" {\n";
    str << Base::tabs(1) << "NODE_NAME \"FreeCAD\"\n";
    str << Base::tabs(1) << "PARENT_LIST {\n";
    str << Base::tabs(2) << "PARENT_COUNT 1\n";
    str << Base::tabs(2) << "PARENT 0 {\n";
    str << Base::tabs(3) << "PARENT_NAME \"<NULL>\"\n";
    str << Base::tabs(3) << "PARENT_TM {\n";
    str << Base::tabs(4) << "1.000000 0.000000 0.000000 0.000000\n";
    str << Base::tabs(4) << "0.000000 1.000000 0.000000 0.000000\n";
    str << Base::tabs(4) << "0.000000 0.000000 1.000000 0.000000\n";
    str << Base::tabs(4) << "0.000000 0.000000 0.000000 1.000000\n";
    str << Base::tabs(3) << "}\n";
    str << Base::tabs(2) << "}\n";
    str << Base::tabs(1) << "}\n";
    str << Base::tabs(1) << "RESOURCE_NAME \"" << resource << "\"\n";
    str << Base::tabs(0) << "}\n\n";

    str << Base::tabs(0) << "RESOURCE_LIST \"MODEL\" {\n";
    str << Base::tabs(1) << "RESOURCE_COUNT 1\n";
    str << Base::tabs(1) << "RESOURCE 0 {\n";
    str << Base::tabs(2) << "RESOURCE_NAME \"" << resource << "\"\n";
    str << Base::tabs(2) << "MODEL_TYPE \"MESH\"\n";
    str << Base::tabs(2) << "MESH {\n";
    str << Base::tabs(3) << "FACE_COUNT " << fts.size() << '\n';
    str << Base::tabs(3) << "MODEL_POSITION_COUNT " << pts.size() << '\n';
    str << Base::tabs(3) << "MODEL_NORMAL_COUNT " << 3 * fts.size() << '\n';
    str << Base::tabs(3) << "MODEL_DIFFUSE_COLOR_COUNT 0\n";
    str << Base::tabs(3) << "MODEL_SPECULAR_COLOR_COUNT 0\n";
    str << Base::tabs(3) << "MODEL_TEXTURE_COORD_COUNT 0\n";
    str << Base::tabs(3) << "MODEL_BONE_COUNT 0\n";
    str << Base::tabs(3) << "MODEL_SHADING_COUNT 1\n";
    str << Base::tabs(3) << "MODEL_SHADING_DESCRIPTION_LIST {\n";
    str << Base::tabs(4) << "SHADING_DESCRIPTION 0 {\n";
    str << Base::tabs(5) << "TEXTURE_LAYER_COUNT 0\n";
    str << Base::tabs(5) << "SHADER_ID 0\n";
    str << Base::tabs(4) << "}\n";
    str << Base::tabs(3) << "}\n";
    str << Base::tabs(3) << "MESH_FACE_POSITION_LIST {\n";
    for (const auto& ft : fts) {
        str << Base::tabs(4) << ft._aulPoints[0] << " " << ft._aulPoints[1] << " "
            << ft._aulPoints[2] << '\n';
    }
    str << Base::tabs(3) << "}\n";
    str << Base::tabs(3) << "MESH_FACE_NORMAL_LIST {\n";
    int index = 0;
    for (auto it = fts.begin(); it != fts.end(); ++it) {
        str << Base::tabs(4) << index << " " << index + 1 << " " << index + 2 << '\n';
        index += 3;
    }
    str << Base::tabs(3) << "}\n";
    str << Base::tabs(3) << "MESH_FACE_SHADING_LIST {\n";
    for (auto it = fts.begin(); it != fts.end(); ++it) {
        str << Base::tabs(4) << "0\n";
    }
    str << Base::tabs(3) << "}\n";
    str << Base::tabs(3) << "MODEL_POSITION_LIST {\n";
    for (const auto& pt : pts) {
        str << Base::tabs(4) << pt.x << " " << pt.y << " " << pt.z << '\n';
    }
    str << Base::tabs(3) << "}\n";
    str << Base::tabs(3) << "MODEL_NORMAL_LIST {\n";
    for (const auto& ft : fts) {
        MeshGeomFacet face = _rclMesh.GetFacet(ft);
        Base::Vector3f normal = face.GetNormal();
        str << Base::tabs(4) << normal.x << " " << normal.y << " " << normal.z << '\n';
        str << Base::tabs(4) << normal.x << " " << normal.y << " " << normal.z << '\n';
        str << Base::tabs(4) << normal.x << " " << normal.y << " " << normal.z << '\n';
    }

    str << Base::tabs(3) << "}\n";
    str << Base::tabs(2) << "}\n";
    str << Base::tabs(1) << "}\n";
    str << Base::tabs(0) << "}\n";

    return true;
}

/** Writes an MGL file. */
bool MeshOutput::SaveMGL(std::ostream& str) const
{
    /*
    light on
    list t 0 1 2 | 0 1 3 | 0 2 3 | 1 2 3
    list xt 1 1 0 0
    list yt -1 -1 1 0
    list zt -1 -1 -1 1
    triplot t xt yt zt 'b'
    #triplot t xt yt zt '#k'
    */
    if (!str || str.bad() || (_rclMesh.CountFacets() == 0)) {
        return false;
    }

    const MeshPointArray& pts = _rclMesh.GetPoints();
    const MeshFacetArray& fts = _rclMesh.GetFacets();

    str.precision(2);
    str.setf(std::ios::fixed | std::ios::showpoint);

    str << "light on\n";
    str << "list t ";
    for (const auto& ft : fts) {
        str << ft._aulPoints[0] << " " << ft._aulPoints[1] << " " << ft._aulPoints[2] << " | ";
    }
    str << std::endl;

    str << "list xt ";
    for (const auto& pt : pts) {
        str << pt.x << " ";
    }
    str << std::endl;

    str << "list yt ";
    for (const auto& pt : pts) {
        str << pt.y << " ";
    }
    str << std::endl;

    str << "list zt ";
    for (const auto& pt : pts) {
        str << pt.z << " ";
    }
    str << std::endl;

    str << "triplot t xt yt zt 'b'" << std::endl;
    str << "#triplot t xt yt zt '#k'" << std::endl;

    return true;
}

/** Writes an OpenInventor file. */
bool MeshOutput::SaveInventor(std::ostream& output) const
{
    WriterInventor writer(_rclMesh, _material);
    writer.SetTransform(_transform);
    return writer.Save(output);
}

/** Writes an X3D file. */
bool MeshOutput::SaveX3D(std::ostream& out) const
{
    if (!out || out.bad() || (_rclMesh.CountFacets() == 0)) {
        return false;
    }

    // XML header info
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    return SaveX3DContent(out, false);
}

/** Writes an X3D file. */
bool MeshOutput::SaveX3DContent(std::ostream& out, bool exportViewpoints) const
{
    if (!out || out.bad() || (_rclMesh.CountFacets() == 0)) {
        return false;
    }

    const MeshPointArray& pts = _rclMesh.GetPoints();
    const MeshFacetArray& fts = _rclMesh.GetFacets();
    Base::BoundBox3f bbox = _rclMesh.GetBoundBox();
    if (apply_transform) {
        bbox = bbox.Transformed(_transform);
    }

    Base::Color mat(0.65F, 0.65F, 0.65F);
    if (_material && _material->binding == MeshIO::Binding::OVERALL) {
        if (!_material->diffuseColor.empty()) {
            mat = _material->diffuseColor.front();
        }
    }
    bool saveVertexColor
        = (_material && _material->binding == MeshIO::PER_VERTEX
           && _material->diffuseColor.size() == pts.size());
    bool saveFaceColor
        = (_material && _material->binding == MeshIO::PER_FACE
           && _material->diffuseColor.size() == fts.size());

    Base::SequencerLauncher seq("Saving...", _rclMesh.CountFacets() + 1);
    out.precision(6);
    out.setf(std::ios::fixed | std::ios::showpoint);

    // Header info
    out << R"(<X3D profile="Immersive" version="3.2" xmlns:xsd=)"
        << "\"http://www.w3.org/2001/XMLSchema-instance\" xsd:noNamespaceSchemaLocation="
        << "\"http://www.web3d.org/specifications/x3d-3.2.xsd\" width=\"1280px\"  "
           "height=\"1024px\">\n";
    out << "  <head>\n"
        << "    <meta name=\"generator\" content=\"FreeCAD\"/>\n"
        << "    <meta name=\"author\" content=\"\"/> \n"
        << "    <meta name=\"company\" content=\"\"/>\n"
        << "  </head>\n";

    // Beginning
    out << "  <Scene>\n";

    if (exportViewpoints) {
        auto viewpoint = [&out](
                             const char* text,
                             const Base::Vector3f& cnt,
                             const Base::Vector3f& pos,
                             const Base::Vector3f& axis,
                             float angle
                         ) {
            out << "    <Viewpoint id=\"" << text << "\" centerOfRotation=\"" << cnt.x << " "
                << cnt.y << " " << cnt.z << "\" position=\"" << pos.x << " " << pos.y << " "
                << pos.z << "\" orientation=\"" << axis.x << " " << axis.y << " " << axis.z << " "
                << angle << R"(" description="camera" fieldOfView="0.9">)"
                << "</Viewpoint>\n";
        };

        Base::Vector3f cnt = bbox.GetCenter();
        float dist = 1.2F * bbox.CalcDiagonalLength();
        float dist3 = 0.577350F * dist;  // sqrt(1/3) * dist

        viewpoint(
            "Iso",
            cnt,
            Base::Vector3f(cnt.x + dist3, cnt.y - dist3, cnt.z + dist3),
            Base::Vector3f(0.742906F, 0.307722F, 0.594473F),
            1.21712F
        );
        viewpoint(
            "Front",
            cnt,
            Base::Vector3f(cnt.x, cnt.y - dist, cnt.z),
            Base::Vector3f(1.0F, 0.0F, 0.0F),
            1.5707964F
        );
        viewpoint(
            "Back",
            cnt,
            Base::Vector3f(cnt.x, cnt.y + dist, cnt.z),
            Base::Vector3f(0.0F, 0.707106F, 0.707106F),
            3.141592F
        );
        viewpoint(
            "Right",
            cnt,
            Base::Vector3f(cnt.x + dist, cnt.y, cnt.z),
            Base::Vector3f(0.577350F, 0.577350F, 0.577350F),
            2.094395F
        );
        viewpoint(
            "Left",
            cnt,
            Base::Vector3f(cnt.x - dist, cnt.y, cnt.z),
            Base::Vector3f(-0.577350F, 0.577350F, 0.577350F),
            4.188790F
        );
        viewpoint(
            "Top",
            cnt,
            Base::Vector3f(cnt.x, cnt.y, cnt.z + dist),
            Base::Vector3f(0.0F, 0.0F, 1.0F),
            0.0F
        );
        viewpoint(
            "Bottom",
            cnt,
            Base::Vector3f(cnt.x, cnt.y, cnt.z - dist),
            Base::Vector3f(1.0F, 0.0F, 0.0F),
            3.141592F
        );
    }

    if (apply_transform) {
        Base::Placement p(_transform);
        const Base::Vector3d& v = p.getPosition();
        const Base::Rotation& r = p.getRotation();
        Base::Vector3d axis;
        double angle {};
        r.getValue(axis, angle);
        out << "    <Transform "
            << "translation='" << v.x << " " << v.y << " " << v.z << "' "
            << "rotation='" << axis.x << " " << axis.y << " " << axis.z << " " << angle << "'>\n";
    }
    else {
        out << "    <Transform>\n";
    }
    out << "      <Shape>\n";
    out << "        <Appearance>\n"
           "          <Material diffuseColor='"
        << mat.r << " " << mat.g << " " << mat.b
        << "' shininess='0.9' specularColor='1 1 1'></Material>\n"
           "        </Appearance>\n";

    out << "        <IndexedFaceSet solid=\"false\" ";
    if (saveVertexColor) {
        out << "colorPerVertex=\"true\" ";
    }
    else if (saveFaceColor) {
        out << "colorPerVertex=\"false\" ";
    }

    out << "coordIndex=\"";
    for (const auto& ft : fts) {
        out << ft._aulPoints[0] << " " << ft._aulPoints[1] << " " << ft._aulPoints[2] << " -1 ";
    }
    out << "\">\n";

    out << "          <Coordinate point=\"";
    for (const auto& pt : pts) {
        out << pt.x << " " << pt.y << " " << pt.z << ", ";
    }
    out << "\"/>\n";

    // write colors per vertex or face
    if (saveVertexColor || saveFaceColor) {
        out << "          <Color color=\"";
        for (const auto& c : _material->diffuseColor) {
            out << c.r << " " << c.g << " " << c.b << ", ";
        }
        out << "\"/>\n";
    }

    // End
    out << "        </IndexedFaceSet>\n"
        << "      </Shape>\n"
        << "    </Transform>\n"
        << "    <Background groundColor=\"0.7 0.7 0.7\" skyColor=\"0.7 0.7 0.7\" />\n"
        << "    <NavigationInfo/>\n"
        << "  </Scene>\n"
        << "</X3D>\n";

    return true;
}

/** Writes an X3DOM file. */
bool MeshOutput::SaveX3DOM(std::ostream& out) const
{
    if (!out || out.bad() || (_rclMesh.CountFacets() == 0)) {
        return false;
    }

    // See:
    // https://stackoverflow.com/questions/31976056/unable-to-color-faces-using-indexedfaceset-in-x3dom
    //
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
           "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n";
    out << "<html xmlns='http://www.w3.org/1999/xhtml'>\n"
        << "  <head>\n"
        << "    <script type='text/javascript' src='http://www.x3dom.org/download/x3dom.js'> "
           "</script>\n"
        << "    <link rel='stylesheet' type='text/css' "
           "href='http://www.x3dom.org/download/x3dom.css'></link>\n"
        << "  </head>\n";

    auto onclick = [&out](const char* text) {
        out << "  <button onclick=\"document.getElementById('" << text
            << "').setAttribute('set_bind','true');\">" << text << "</button>\n";
    };

    onclick("Iso");
    onclick("Front");
    onclick("Back");
    onclick("Right");
    onclick("Left");
    onclick("Top");
    onclick("Bottom");

#if 0  // https://stackoverflow.com/questions/32305678/x3dom-how-to-make-zoom-buttons
    function zoom (delta) {
        var x3d = document.getElementById("right");
        var vpt = x3d.getElementsByTagName("Viewpoint")[0];
        vpt.fieldOfView = parseFloat(vpt.fieldOfView) + delta;
    }

    <button onclick="zoom(0.15);">Zoom out</button>
#endif

    SaveX3DContent(out, true);

    out << "</html>\n";

    return true;
}

/** Writes a Nastran file. */
bool MeshOutput::SaveNastran(std::ostream& output) const
{
    if (!output || output.bad() || (_rclMesh.CountFacets() == 0)) {
        return false;
    }

    MeshPointIterator clPIter(_rclMesh);
    clPIter.Transform(this->_transform);
    MeshFacetIterator clTIter(_rclMesh);
    int iIndx = 1;

    Base::SequencerLauncher seq("Saving...", _rclMesh.CountFacets() + 1);

    output.precision(3);
    output.setf(std::ios::fixed | std::ios::showpoint);
    for (clPIter.Init(); clPIter.More(); clPIter.Next()) {
        float x = clPIter->x;
        float y = clPIter->y;
        float z = clPIter->z;

        output << "GRID";

        output << std::setfill(' ') << std::setw(12) << iIndx;
        output << std::setfill(' ') << std::setw(16) << x;
        output << std::setfill(' ') << std::setw(8) << y;
        output << std::setfill(' ') << std::setw(8) << z;
        output << '\n';

        iIndx++;
        seq.next();
    }

    iIndx = 1;
    for (clTIter.Init(); clTIter.More(); clTIter.Next()) {
        output << "CTRIA3";

        output << std::setfill(' ') << std::setw(10) << iIndx;
        output << std::setfill(' ') << std::setw(8) << (int)0;
        output << std::setfill(' ') << std::setw(8) << clTIter.GetIndices()._aulPoints[1] + 1;
        output << std::setfill(' ') << std::setw(8) << clTIter.GetIndices()._aulPoints[0] + 1;
        output << std::setfill(' ') << std::setw(8) << clTIter.GetIndices()._aulPoints[2] + 1;
        output << '\n';

        iIndx++;
        seq.next();
    }

    output << "ENDDATA";

    return true;
}

/** Writes a Cadmould FE file. */
bool MeshOutput::SaveCadmouldFE(std::ostream& /*output*/) const
{
    return false;
}

/** Writes a Python module */
bool MeshOutput::SavePython(std::ostream& str) const
{
    if (!str || str.bad() || (_rclMesh.CountFacets() == 0)) {
        return false;
    }

    MeshFacetIterator clIter(_rclMesh);
    clIter.Transform(this->_transform);
    str.precision(4);
    str.setf(std::ios::fixed | std::ios::showpoint);

    str << "faces = [\n";
    for (clIter.Init(); clIter.More(); clIter.Next()) {
        const MeshGeomFacet& rFacet = *clIter;
        for (const auto& pnt : rFacet._aclPoints) {
            str << "[" << pnt.x << "," << pnt.y << "," << pnt.z << "],";
        }
        str << '\n';
    }

    str << "]\n";

    return true;
}

/** Writes a VRML file. */
bool MeshOutput::SaveVRML(std::ostream& output) const
{
    if (!output || output.bad() || (_rclMesh.CountFacets() == 0)) {
        return false;
    }

    Base::BoundBox3f clBB = _rclMesh.GetBoundBox();

    Base::SequencerLauncher seq("Saving VRML file...", _rclMesh.CountPoints() + _rclMesh.CountFacets());

    output << "#VRML V2.0 utf8\n";
    output << "WorldInfo {\n"
           << "  title \"Exported triangle mesh to VRML97\"\n"
           << "  info [\"Created by FreeCAD\"\n"
           << "        \"<https://www.freecad.org>\"]\n"
           << "}\n\n";

    // Transform
    output.precision(3);
    output.setf(std::ios::fixed | std::ios::showpoint);
    output << "Transform {\n"
           << "  scale 1 1 1\n"
           << "  rotation 0 0 1 0\n"
           << "  scaleOrientation 0 0 1 0\n"
           << "  center " << 0.0F << " " << 0.0F << " " << 0.0F << "\n"
           << "  translation " << 0.0F << " " << 0.0F << " " << 0.0F << "\n";

    output << "  children\n";
    output << "    Shape { \n";

    // write appearance
    output << "      appearance\n"
           << "      Appearance {\n"
           << "        material\n"
           << "        Material {\n";
    if (_material && _material->binding == MeshIO::OVERALL) {
        if (!_material->diffuseColor.empty()) {
            Base::Color c = _material->diffuseColor.front();
            output << "          diffuseColor " << c.r << " " << c.g << " " << c.b << "\n";
        }
        else {
            output << "          diffuseColor 0.8 0.8 0.8\n";
        }
    }
    else {
        output << "          diffuseColor 0.8 0.8 0.8\n";
    }
    output << "        }\n      }\n";  // end write appearance


    // write IndexedFaceSet
    output << "      geometry\n"
           << "      IndexedFaceSet {\n";

    output.precision(2);
    output.setf(std::ios::fixed | std::ios::showpoint);

    // write coords
    output << "        coord\n        Coordinate {\n          point [\n";
    MeshPointIterator pPIter(_rclMesh);
    pPIter.Transform(this->_transform);
    unsigned long i = 0, k = _rclMesh.CountPoints();
    output.precision(3);
    output.setf(std::ios::fixed | std::ios::showpoint);
    for (pPIter.Init(); pPIter.More(); pPIter.Next()) {
        output << "            " << pPIter->x << " " << pPIter->y << " " << pPIter->z;
        if (i++ < (k - 1)) {
            output << ",\n";
        }
        else {
            output << "\n";
        }

        seq.next();
    }

    output << "          ]\n        }\n";  // end write coord

    if (_material && _material->binding != MeshIO::OVERALL) {
        // write colors for each vertex
        output << "        color\n        Color {\n          color [\n";
        output.precision(3);
        output.setf(std::ios::fixed | std::ios::showpoint);
        for (auto pCIter = _material->diffuseColor.begin(); pCIter != _material->diffuseColor.end();
             ++pCIter) {
            output << "          " << float(pCIter->r) << " " << float(pCIter->g) << " "
                   << float(pCIter->b);
            if (pCIter < (_material->diffuseColor.end() - 1)) {
                output << ",\n";
            }
            else {
                output << "\n";
            }
        }

        output << "      ]\n    }\n";
        if (_material->binding == MeshIO::PER_VERTEX) {
            output << "    colorPerVertex TRUE\n";
        }
        else {
            output << "    colorPerVertex FALSE\n";
        }
    }

    // write face index
    output << "        coordIndex [\n";
    MeshFacetIterator pFIter(_rclMesh);
    pFIter.Transform(this->_transform);
    i = 0, k = _rclMesh.CountFacets();

    for (pFIter.Init(); pFIter.More(); pFIter.Next()) {
        MeshFacet clFacet = pFIter.GetIndices();
        output << "          " << clFacet._aulPoints[0] << ", " << clFacet._aulPoints[1] << ", "
               << clFacet._aulPoints[2] << ", -1";
        if (i++ < (k - 1)) {
            output << ",\n";
        }
        else {
            output << "\n";
        }

        seq.next();
    }

    output << "        ]\n      }\n";  // End IndexedFaceSet
    output << "    }\n";               // End Shape
    output << "}\n";                   // close children and Transform

    return true;
}

// ----------------------------------------------------------------------------

MeshCleanup::MeshCleanup(MeshPointArray& p, MeshFacetArray& f)
    : pointArray(p)
    , facetArray(f)
{}

void MeshCleanup::SetMaterial(Material* mat)
{
    materialArray = mat;
}

void MeshCleanup::RemoveInvalids()
{
    // first mark all points as invalid
    pointArray.SetFlag(MeshPoint::INVALID);
    std::size_t numPoints = pointArray.size();

    // Now go through the facets and invalidate facets with wrong indices
    // If a facet is valid all its referenced points are validated again
    // Points that are not referenced are still invalid and thus can be deleted
    for (auto& it : facetArray) {
        for (PointIndex point : it._aulPoints) {
            // vertex index out of range
            if (point >= numPoints) {
                it.SetInvalid();
                break;
            }
        }

        // validate referenced points
        if (it.IsValid()) {
            pointArray[it._aulPoints[0]].ResetInvalid();
            pointArray[it._aulPoints[1]].ResetInvalid();
            pointArray[it._aulPoints[2]].ResetInvalid();
        }
    }

    // Remove the invalid items
    RemoveInvalidFacets();
    RemoveInvalidPoints();
}

void MeshCleanup::RemoveInvalidFacets()
{
    MeshIsFlag<MeshFacet> flag;
    std::size_t countInvalidFacets
        = std::count_if(facetArray.begin(), facetArray.end(), [flag](const MeshFacet& f) {
              return flag(f, MeshFacet::INVALID);
          });
    if (countInvalidFacets > 0) {

        // adjust the material array if needed
        if (materialArray && materialArray->binding == MeshIO::PER_FACE
            && materialArray->diffuseColor.size() == facetArray.size()) {
            std::vector<Base::Color> colors;
            colors.reserve(facetArray.size() - countInvalidFacets);
            for (std::size_t index = 0; index < facetArray.size(); index++) {
                if (facetArray[index].IsValid()) {
                    colors.push_back(materialArray->diffuseColor[index]);
                }
            }

            materialArray->diffuseColor.swap(colors);
        }

        MeshFacetArray copy_facets(facetArray.size() - countInvalidFacets);
        // copy all valid facets to the new array
        std::remove_copy_if(
            facetArray.begin(),
            facetArray.end(),
            copy_facets.begin(),
            [flag](const MeshFacet& f) { return flag(f, MeshFacet::INVALID); }
        );
        facetArray.swap(copy_facets);
    }
}

void MeshCleanup::RemoveInvalidPoints()
{
    MeshIsFlag<MeshPoint> flag;
    std::size_t countInvalidPoints
        = std::count_if(pointArray.begin(), pointArray.end(), [flag](const MeshPoint& p) {
              return flag(p, MeshPoint::INVALID);
          });
    if (countInvalidPoints > 0) {
        // generate array of decrements
        std::vector<PointIndex> decrements;
        decrements.resize(pointArray.size());
        PointIndex decr = 0;

        MeshPointArray::_TIterator p_end = pointArray.end();
        std::vector<PointIndex>::iterator decr_it = decrements.begin();
        for (auto p_it = pointArray.begin(); p_it != p_end; ++p_it, ++decr_it) {
            *decr_it = decr;
            if (!p_it->IsValid()) {
                decr++;
            }
        }

        // correct point indices of the facets
        MeshFacetArray::_TIterator f_end = facetArray.end();
        for (auto f_it = facetArray.begin(); f_it != f_end; ++f_it) {
            f_it->_aulPoints[0] -= decrements[f_it->_aulPoints[0]];
            f_it->_aulPoints[1] -= decrements[f_it->_aulPoints[1]];
            f_it->_aulPoints[2] -= decrements[f_it->_aulPoints[2]];
        }

        // delete point, number of valid points
        std::size_t validPoints = pointArray.size() - countInvalidPoints;

        // adjust the material array if needed
        if (materialArray && materialArray->binding == MeshIO::PER_VERTEX
            && materialArray->diffuseColor.size() == pointArray.size()) {
            std::vector<Base::Color> colors;
            colors.reserve(validPoints);
            for (std::size_t index = 0; index < pointArray.size(); index++) {
                if (pointArray[index].IsValid()) {
                    colors.push_back(materialArray->diffuseColor[index]);
                }
            }

            materialArray->diffuseColor.swap(colors);
        }

        MeshPointArray copy_points(validPoints);
        // copy all valid facets to the new array
        std::remove_copy_if(
            pointArray.begin(),
            pointArray.end(),
            copy_points.begin(),
            [flag](const MeshPoint& p) { return flag(p, MeshPoint::INVALID); }
        );
        pointArray.swap(copy_points);
    }
}

// ----------------------------------------------------------------------------

MeshPointFacetAdjacency::MeshPointFacetAdjacency(std::size_t p, MeshFacetArray& f)
    : numPoints(p)
    , facets(f)
{
    Build();
}

void MeshPointFacetAdjacency::Build()
{
    std::vector<std::size_t> numFacetAdjacency(numPoints);
    for (const auto& it : facets) {
        numFacetAdjacency[it._aulPoints[0]]++;
        numFacetAdjacency[it._aulPoints[1]]++;
        numFacetAdjacency[it._aulPoints[2]]++;
    }

    pointFacetAdjacency.resize(numPoints);
    for (std::size_t i = 0; i < numPoints; i++) {
        pointFacetAdjacency[i].reserve(numFacetAdjacency[i]);
    }

    std::size_t numFacets = facets.size();
    for (std::size_t i = 0; i < numFacets; i++) {
        for (PointIndex ptIndex : facets[i]._aulPoints) {
            pointFacetAdjacency[ptIndex].push_back(i);
        }
    }
}

void MeshPointFacetAdjacency::SetFacetNeighbourhood()
{
    std::size_t numFacets = facets.size();
    for (std::size_t index = 0; index < numFacets; index++) {
        MeshFacet& facet1 = facets[index];
        for (int i = 0; i < 3; i++) {
            std::size_t n1 = facet1._aulPoints[i];
            std::size_t n2 = facet1._aulPoints[(i + 1) % 3];

            bool success = false;
            const std::vector<std::size_t>& refFacets = pointFacetAdjacency[n1];
            for (std::size_t it : refFacets) {
                if (it != index) {
                    MeshFacet& facet2 = facets[it];
                    if (facet2.HasPoint(n2)) {
                        facet1._aulNeighbours[i] = it;
                        success = true;
                        break;
                    }
                }
            }

            if (!success) {
                facet1._aulNeighbours[i] = FACET_INDEX_MAX;
            }
        }
    }
}
