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


#include "PreCompiled.h"
#include "Definitions.h"
#include "Iterator.h"

#include "MeshKernel.h"
#include "MeshIO.h"
#include "Builder.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/FileInfo.h>
#include <Base/Sequencer.h>
#include <Base/Stream.h>
#include <Base/Placement.h>
#include <zipios++/gzipoutputstream.h>

#include <math.h>
#include <sstream>
#include <iomanip>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>


using namespace MeshCore;

char *upper(char * string)
{
    int i;
    int l;

    if (string != NULL) {
        l = std::strlen(string);
        for (i=0; i<l; i++)
            string[i] = toupper(string[i]);
    }

    return string;
}

char *ltrim (char *psz)
{
    int i, sl;

    if (psz) {
        for (i = 0; (psz[i] == 0x20) || (psz[i] == 0x09); i++);
        sl = std::strlen (psz + i);
        memmove (psz, psz + i, sl);
        psz[sl] = 0;
    }
    return psz;
}

std::string& upper(std::string& str)
{
    for (std::string::iterator it = str.begin(); it != str.end(); ++it)
        *it = toupper(*it);
    return str;
}

std::string& ltrim(std::string& str)
{
    std::string::size_type pos=0;
    for (std::string::iterator it = str.begin(); it != str.end(); ++it) {
        if (*it != 0x20 && *it != 0x09)
            break;
        pos++;
    }
    if (pos > 0)
        str = str.substr(pos);
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

/* Define a vector.  */
typedef struct _Vector
	{
		double  x;
		double  y;
		double  z;
	} Vector, *VectorPtr;

/* Usage by CMeshNastran, CMeshCadmouldFE. Added by Sergey Sukhov (26.04.2002)*/
struct NODE {float x, y, z;};
struct TRIA {int iV[3];};
struct QUAD {int iV[4];};

/* Takes the modulus of v */
#define VMod(v)		(sqrt((v).x*(v).x + (v).y*(v).y + (v).z*(v).z))

/* Returns the dot product of v1 & v2 */
#define VDot(v1, v2)	((v1).x*(v2).x + (v1).y*(v2).y + (v1).z*(v2).z)

/* Fills the fields of a vector.	*/
#define VNew(a, b, c, r)	((r).x = (a), (r).y = (b), (r).z = (c))

#define VAdd(v1, v2, r)		((r).x = (v1).x + (v2).x , \
							 (r).y = (v1).y + (v2).y , \
							 (r).z = (v1).z + (v2).z )

#define VSub(v1, v2, r)		((r).x = (v1).x - (v2).x , \
							 (r).y = (v1).y - (v2).y , \
							 (r).z = (v1).z - (v2).z )

#define VCross(v1, v2, r)	((r).x = (v1).y * (v2).z - (v1).z * (v2).y , \
				 			 (r).y = (v1).z * (v2).x - (v1).x * (v2).z , \
				 			 (r).z = (v1).x * (v2).y - (v1).y * (v2).x )

#define VScalarMul(v, d, r)	((r).x = (v).x * (d) , \
				 			 (r).y = (v).y * (d) , \
				 			 (r).z = (v).z * (d) )

#define VUnit(v, t, r)		((t) = 1 / VMod(v) , \
				 			 VScalarMul(v, t, r) )

typedef struct _Quaternion {
	Vector	vect_part;
	double	real_part;
	} Quaternion;

Quaternion
Build_Rotate_Quaternion(Vector axis, double cos_angle)
{
	Quaternion	quat;
	double	sin_half_angle;
	double	cos_half_angle;
	double	angle;

	/* The quaternion requires half angles. */
	if ( cos_angle > 1.0 ) cos_angle = 1.0;
	if ( cos_angle < -1.0 ) cos_angle = -1.0;
	angle = acos(cos_angle);
	sin_half_angle = sin(angle / 2);
	cos_half_angle = cos(angle / 2);

	VScalarMul(axis, sin_half_angle, quat.vect_part);
	quat.real_part = cos_half_angle;

	return quat;
}

static Quaternion
QQMul(Quaternion *q1, Quaternion *q2)
{
	Quaternion	res;
	Vector		temp_v;

	res.real_part = q1->real_part * q2->real_part -
					VDot(q1->vect_part, q2->vect_part);
	VCross(q1->vect_part, q2->vect_part, res.vect_part);
	VScalarMul(q1->vect_part, q2->real_part, temp_v);
	VAdd(temp_v, res.vect_part, res.vect_part);
	VScalarMul(q2->vect_part, q1->real_part, temp_v);
	VAdd(temp_v, res.vect_part, res.vect_part);

	return res;
}

static void
Quaternion_To_Axis_Angle(Quaternion *q, Vector *axis, double *angle)
{
	double	half_angle;
	double	sin_half_angle;

	half_angle = acos(q->real_part);
	sin_half_angle = sin(half_angle);
	*angle = half_angle * 2;
	if ( sin_half_angle < 1e-8 && sin_half_angle > -1e-8 )
		VNew(1, 0, 0, *axis);
	else
	{
		sin_half_angle = 1 / sin_half_angle;
		VScalarMul(q->vect_part, sin_half_angle, *axis);
	}
}

static void
Convert_Camera_Model(Vector *pos, Vector *at, Vector *up, Vector *res_axis,
                     double *res_angle)
{
	Vector		n, v;

	Quaternion	rot_quat;
	Vector		norm_axis;
	Quaternion	norm_quat;
	Quaternion	inv_norm_quat;
	Quaternion	y_quat, new_y_quat, rot_y_quat;
	Vector		new_y;

	double		temp_d;
	Vector		temp_v;

	/* n = (norm)(pos - at) */
	VSub(*at, *pos, n);
	VUnit(n, temp_d, n);

	/* v = (norm)(view_up - (view_up.n)n) */
	VUnit(*up, temp_d, *up);
	temp_d = VDot(*up, n);
	VScalarMul(n, temp_d, temp_v);
	VSub(*up, temp_v, v);
	VUnit(v, temp_d, v);

	VNew(n.y, -n.x, 0, norm_axis);
	if ( VDot(norm_axis, norm_axis) < 1e-8 )
	{
		/* Already aligned, or maybe inverted. */
		if ( n.z > 0.0 )
		{
			norm_quat.real_part = 0.0;
			VNew(0, 1, 0, norm_quat.vect_part);
		}
		else
		{
			norm_quat.real_part = 1.0;
			VNew(0, 0, 0, norm_quat.vect_part);
		}
	}
	else
	{
		VUnit(norm_axis, temp_d, norm_axis);
		norm_quat = Build_Rotate_Quaternion(norm_axis, -n.z);
	}

	/* norm_quat now holds the rotation needed to line up the view directions.
	** We need to find the rotation to align the up vectors also.
	*/

	/* Need to rotate the world y vector to see where it ends up. */
	/* Find the inverse rotation. */
	inv_norm_quat.real_part = norm_quat.real_part;
	VScalarMul(norm_quat.vect_part, -1, inv_norm_quat.vect_part);

	/* Rotate the y. */
	y_quat.real_part = 0.0;
	VNew(0, 1, 0, y_quat.vect_part);
	new_y_quat = QQMul(&norm_quat, &y_quat);
	new_y_quat = QQMul(&new_y_quat, &inv_norm_quat);
	new_y = new_y_quat.vect_part;

	/* Now need to find out how much to rotate about n to line up y. */
	VCross(new_y, v, temp_v);
	if ( VDot(temp_v, temp_v) < 1.e-8 )
	{
		/* The old and new may be pointing in the same or opposite. Need
		** to generate a vector perpendicular to the old or new y.
		*/
		VNew(0, -v.z, v.y, temp_v);
		if ( VDot(temp_v, temp_v) < 1.e-8 )
			VNew(v.z, 0, -v.x, temp_v);
	}
	VUnit(temp_v, temp_d, temp_v);
	rot_y_quat = Build_Rotate_Quaternion(temp_v, VDot(new_y, v));

	/* rot_y_quat holds the rotation about the initial camera direction needed
	** to align the up vectors in the final position.
	*/

	/* Put the 2 rotations together. */
	rot_quat = QQMul(&rot_y_quat, &norm_quat);

	/* Extract the axis and angle from the quaternion. */
	Quaternion_To_Axis_Angle(&rot_quat, res_axis, res_angle);
}

// --------------------------------------------------------------

bool MeshInput::LoadAny(const char* FileName)
{
    // ask for read permission
    Base::FileInfo fi(FileName);
    if (!fi.exists() || !fi.isFile())
        throw Base::FileException("File does not exist",FileName);
    if (!fi.isReadable())
        throw Base::FileException("No permission on the file",FileName);

    Base::ifstream str(fi, std::ios::in | std::ios::binary);

    if (fi.hasExtension("bms")) {
        _rclMesh.Read(str);
        return true;
    }
    else {
        // read file
        bool ok = false;
        if (fi.hasExtension("stl") || fi.hasExtension("ast")) {
            ok = LoadSTL(str);
        }
        else if (fi.hasExtension("iv")) {
            ok = LoadInventor( str );
            if (ok && _rclMesh.CountFacets() == 0)
                Base::Console().Warning("No usable mesh found in file '%s'", FileName);
        }
        else if (fi.hasExtension("nas") || fi.hasExtension("bdf")) {
            ok = LoadNastran( str );
        }
        else if (fi.hasExtension("obj")) {
            ok = LoadOBJ( str );
        }
        else if (fi.hasExtension("off")) {
            ok = LoadOFF( str );
        }
        else if (fi.hasExtension("ply")) {
            ok = LoadPLY( str );
        }
        else {
            throw Base::FileException("File extension not supported",FileName);
        }

        return ok;
    }
}

/** Loads an STL file either in binary or ASCII format. 
 * Therefore the file header gets checked to decide if the file is binary or not.
 */
bool MeshInput::LoadSTL (std::istream &rstrIn)
{
    char szBuf[200];

    if (!rstrIn || rstrIn.bad() == true)
        return false;

    // Read in 50 characters from position 80 on and check for keywords like 'SOLID', 'FACET', 'NORMAL',
    // 'VERTEX', 'ENDFACET' or 'ENDLOOP'.
    // As the file can be binary with one triangle only we must not read in more than (max.) 54 bytes because
    // the file size has only 134 bytes in this case. On the other hand we must overread the first 80 bytes
    // because it can happen that the file is binary but contains one of these keywords.
    std::streambuf* buf = rstrIn.rdbuf();
    if (!buf) return false;
    buf->pubseekoff(80, std::ios::beg, std::ios::in);
    uint32_t ulCt, ulBytes=50;
    rstrIn.read((char*)&ulCt, sizeof(ulCt));
    // if we have a binary STL with a single triangle we can only read-in 50 bytes
    if (ulCt > 1)
        ulBytes = 100;
    // Either it's really an invalid STL file or it's just empty. In this case the number of facets must be 0.
    if (!rstrIn.read(szBuf, ulBytes))
        return (ulCt==0);
    szBuf[ulBytes] = 0;
    upper(szBuf);

    try {
        if ((strstr(szBuf, "SOLID") == NULL)  && (strstr(szBuf, "FACET") == NULL)    && (strstr(szBuf, "NORMAL") == NULL) &&
            (strstr(szBuf, "VERTEX") == NULL) && (strstr(szBuf, "ENDFACET") == NULL) && (strstr(szBuf, "ENDLOOP") == NULL)) {
            // probably binary STL
            buf->pubseekoff(0, std::ios::beg, std::ios::in);
            return LoadBinarySTL(rstrIn);
        }
        else {
            // Ascii STL
            buf->pubseekoff(0, std::ios::beg, std::ios::in);
            return LoadAsciiSTL(rstrIn);
        }
    }
    catch (const Base::MemoryException&) {
        _rclMesh.Clear();
        throw; // Throw the same instance of Base::MemoryException
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
bool MeshInput::LoadOBJ (std::istream &rstrIn)
{
    boost::regex rx_p("^v\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                        "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                        "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$");
    boost::regex rx_f3("^f\\s+([0-9]+)/?[0-9]*/?[0-9]*"
                         "\\s+([0-9]+)/?[0-9]*/?[0-9]*"
                         "\\s+([0-9]+)/?[0-9]*/?[0-9]*\\s*$");
    boost::regex rx_f4("^f\\s+([0-9]+)/?[0-9]*/?[0-9]*"
                         "\\s+([0-9]+)/?[0-9]*/?[0-9]*"
                         "\\s+([0-9]+)/?[0-9]*/?[0-9]*"
                         "\\s+([0-9]+)/?[0-9]*/?[0-9]*\\s*$");
    boost::cmatch what;

    unsigned long segment=0;
    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;

    std::string line;
    float fX, fY, fZ;
    unsigned int  i1=1,i2=1,i3=1,i4=1;
    MeshGeomFacet clFacet;
    MeshFacet item;

    if (!rstrIn || rstrIn.bad() == true)
        return false;

    std::streambuf* buf = rstrIn.rdbuf();
    if (!buf)
        return false;

    bool readvertices=false;
    while (std::getline(rstrIn, line)) {
        for (std::string::iterator it = line.begin(); it != line.end(); ++it)
            *it = tolower(*it);
        if (boost::regex_match(line.c_str(), what, rx_p)) {
            readvertices = true;
            fX = (float)std::atof(what[1].first);
            fY = (float)std::atof(what[4].first);
            fZ = (float)std::atof(what[7].first);
            meshPoints.push_back(MeshPoint(Base::Vector3f(fX, fY, fZ)));
        }
        else if (boost::regex_match(line.c_str(), what, rx_f3)) {
            // starts a new segment
            if (readvertices) {
                readvertices = false;
                segment++;
            }

            // 3-vertex face
            i1 = std::atoi(what[1].first);
            i2 = std::atoi(what[2].first);
            i3 = std::atoi(what[3].first);
            item.SetVertices(i1-1,i2-1,i3-1);
            item.SetProperty(segment);
            meshFacets.push_back(item);
        }
        else if (boost::regex_match(line.c_str(), what, rx_f4)) {
            // starts a new segment
            if (readvertices) {
                readvertices = false;
                segment++;
            }

            // 4-vertex face
            i1 = std::atoi(what[1].first);
            i2 = std::atoi(what[2].first);
            i3 = std::atoi(what[3].first);
            i4 = std::atoi(what[4].first);

            item.SetVertices(i1-1,i2-1,i3-1);
            item.SetProperty(segment);
            meshFacets.push_back(item);

            item.SetVertices(i3-1,i4-1,i1-1);
            item.SetProperty(segment);
            meshFacets.push_back(item);
        }
    }

    this->_rclMesh.Clear(); // remove all data before
    // Don't use Assign() because Merge() checks which points are really needed.
    // This method sets already the correct neighbourhood
    unsigned long ct = meshPoints.size();
    std::list<unsigned long> removeFaces;
    for (MeshFacetArray::_TConstIterator it = meshFacets.begin(); it != meshFacets.end(); ++it) {
        bool ok = true;
        for (int i=0;i<3;i++) {
            if (it->_aulPoints[i] >= ct) {
                Base::Console().Warning("Face index %ld out of range\n", it->_aulPoints[i]);
                ok = false;
            }
        }

        if (!ok)
            removeFaces.push_front(it-meshFacets.begin());
    }

    for (std::list<unsigned long>::iterator it = removeFaces.begin(); it != removeFaces.end(); ++it)
        meshFacets.erase(meshFacets.begin() + *it);

    MeshKernel tmp;
    tmp.Adopt(meshPoints,meshFacets);
    this->_rclMesh.Merge(tmp);

    return true;
}

/** Loads an OFF file. */
bool MeshInput::LoadOFF (std::istream &rstrIn)
{
    boost::regex rx_n("^\\s*([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s*$");
    boost::regex rx_p("^\\s*([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                       "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                       "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$");
    boost::regex rx_f3("^\\s*([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s*$");
    boost::regex rx_f4("^\\s*([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s*$");

    boost::cmatch what;

    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;

    std::string line;
    float fX, fY, fZ;
    unsigned int  i1=1,i2=1,i3=1,i4=1;
    MeshGeomFacet clFacet;
    MeshFacet item;

    if (!rstrIn || rstrIn.bad() == true)
        return false;

    std::streambuf* buf = rstrIn.rdbuf();
    if (!buf)
        return false;

    std::getline(rstrIn, line);
    boost::algorithm::to_lower(line);
    if (line.find("off") == std::string::npos)
        return false; // not an OFF file

    // get number of vertices and faces
    int numPoints=0, numFaces=0;
    std::getline(rstrIn, line);
    boost::algorithm::to_lower(line);
    if (boost::regex_match(line.c_str(), what, rx_n)) {
        numPoints = std::atoi(what[1].first);
        numFaces = std::atoi(what[2].first);
    }
    else {
        // Cannot read number of elements
        return false;
    }

    meshPoints.reserve(numPoints);
    meshFacets.reserve(numFaces);

    int cntPoints = 0;
    while (cntPoints < numPoints) {
        if (!std::getline(rstrIn, line))
            break;
        if (boost::regex_match(line.c_str(), what, rx_p)) {
            fX = (float)std::atof(what[1].first);
            fY = (float)std::atof(what[4].first);
            fZ = (float)std::atof(what[7].first);
            meshPoints.push_back(MeshPoint(Base::Vector3f(fX, fY, fZ)));
            cntPoints++;
        }
    }

    int cntFaces = 0;
    while (cntFaces < numFaces) {
        if (!std::getline(rstrIn, line))
            break;
        if (boost::regex_match(line.c_str(), what, rx_f3)) {
            // 3-vertex face
            if (std::atoi(what[1].first) == 3) {
                i1 = std::atoi(what[2].first);
                i2 = std::atoi(what[3].first);
                i3 = std::atoi(what[4].first);
                item.SetVertices(i1,i2,i3);
                meshFacets.push_back(item);
                cntFaces++;
            }
        }
        else if (boost::regex_match(line.c_str(), what, rx_f4)) {
            // 4-vertex face
            if (std::atoi(what[1].first) == 4) {
                i1 = std::atoi(what[2].first);
                i2 = std::atoi(what[3].first);
                i3 = std::atoi(what[4].first);
                i4 = std::atoi(what[5].first);

                item.SetVertices(i1,i2,i3);
                meshFacets.push_back(item);

                item.SetVertices(i3,i4,i1);
                meshFacets.push_back(item);
                cntFaces++;
            }
        }
    }

    this->_rclMesh.Clear(); // remove all data before
    // Don't use Assign() because Merge() checks which points are really needed.
    // This method sets already the correct neighbourhood
    unsigned long ct = meshPoints.size();
    std::list<unsigned long> removeFaces;
    for (MeshFacetArray::_TConstIterator it = meshFacets.begin(); it != meshFacets.end(); ++it) {
        bool ok = true;
        for (int i=0;i<3;i++) {
            if (it->_aulPoints[i] >= ct) {
                Base::Console().Warning("Face index %ld out of range\n", it->_aulPoints[i]);
                ok = false;
            }
        }

        if (!ok)
            removeFaces.push_front(it-meshFacets.begin());
    }

    for (std::list<unsigned long>::iterator it = removeFaces.begin(); it != removeFaces.end(); ++it)
        meshFacets.erase(meshFacets.begin() + *it);

    MeshKernel tmp;
    tmp.Adopt(meshPoints,meshFacets);
    this->_rclMesh.Merge(tmp);

    return true;
}

bool MeshInput::LoadPLY (std::istream &inp)
{
    // http://local.wasp.uwa.edu.au/~pbourke/dataformats/ply/
    std::size_t v_count=0, f_count=0;
    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;

    enum {
        ascii, binary_little_endian, binary_big_endian
    } format;
    if (!inp || inp.bad() == true)
        return false;

    std::streambuf* buf = inp.rdbuf();
    if (!buf)
        return false;

    // read in the first three characters
    char ply[3];
    inp.read(ply, 3);
    inp.ignore(1);
    if (!inp)
        return false;
    if ((ply[0] != 'p') || (ply[1] != 'l') || (ply[2] != 'y'))
        return false; // wrong header

    std::string line, element;
    bool xyz_float=false,xyz_double=false;
    MeshIO::Binding rgb_value = MeshIO::OVERALL;
    while (std::getline(inp, line)) {
        std::istringstream str(line);
        str.unsetf(std::ios_base::skipws);
        str >> std::ws;
        if (str.eof())
            continue; // empty line
        std::string kw;
        str >> kw;
        if (kw == "format") {
            std::string format_string, version;
            char space_format_string, space_format_version;
            str >> space_format_string >> std::ws
                >> format_string >> space_format_version
                >> std::ws >> version >> std::ws;
            if (!str || !str.eof() ||
                !std::isspace(space_format_string) ||
                !std::isspace(space_format_version)) {
                return false;
            }
            if (format_string == "ascii") {
                format = ascii;
            }
            else if (format_string == "binary_big_endian") {
                format = binary_big_endian;
            }
            else if (format_string == "binary_little_endian") {
                format = binary_little_endian;
            }
            else {
                // wrong format version
                return false;
            }
            if (version != "1.0") {
                // wrong version
                return false;
            }
        }
        else if (kw == "element") {
            std::string name;
            std::size_t count;
            char space_element_name, space_name_count;
            str >> space_element_name >> std::ws
                >> name >> space_name_count >> std::ws
                >> count >> std::ws;
            if (!str || !str.eof() ||
                !std::isspace(space_element_name) ||
                !std::isspace(space_name_count)) {
                return false;
            }
            else if (name == "vertex") {
                element = name;
                v_count = count;
                meshPoints.reserve(count);
            }
            else if (name == "face") {
                element = name;
                f_count = count;
                meshFacets.reserve(count);
            }
            else {
                element.clear();
            }
        }
        else if (kw == "property") {
            std::string type, name;
            char space;
            if (element == "vertex") {
                str >> space >> std::ws
                    >> type >> space >> std::ws >> name >> std::ws;
                if (name == "x") {
                    if (type == "float" || type == "float32")
                        xyz_float = true;
                    else if (type == "double" || type == "float64")
                        xyz_double = true;
                }
                else if (name == "red") {
                    rgb_value = MeshIO::PER_VERTEX;
                    if (_material) {
                        _material->binding = MeshIO::PER_VERTEX;
                        _material->diffuseColor.reserve(v_count);
                    }
                }
            }
            else if (element == "face") {
            }
        }
        else if (kw == "end_header") {
            break; // end of the header, now read the data
        }
    }

    if (format == ascii) {
        boost::regex rx_p("^([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                          "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                          "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$");
        boost::regex rx_c("^([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                          "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                          "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                          "\\s+([0-9]{1,3})\\s+([0-9]{1,3})\\s+([0-9]{1,3})\\s*$");
        boost::regex rx_f("^\\s*3\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s*$");
        boost::cmatch what;
        Base::Vector3f pt;

        if (rgb_value == MeshIO::PER_VERTEX) {
            int r,g,b;
            for (std::size_t i = 0; i < v_count && std::getline(inp, line); i++) {
                if (boost::regex_match(line.c_str(), what, rx_c)) {
                    pt.x = (float)std::atof(what[1].first);
                    pt.y = (float)std::atof(what[4].first);
                    pt.z = (float)std::atof(what[7].first);
                    meshPoints.push_back(pt);
                    if (_material) {
                        r = std::min<int>(std::atoi(what[10].first),255);
                        g = std::min<int>(std::atoi(what[11].first),255);
                        b = std::min<int>(std::atoi(what[12].first),255);
                        float fr = (float)r/255.0f;
                        float fg = (float)g/255.0f;
                        float fb = (float)b/255.0f;
                        _material->diffuseColor.push_back(App::Color(fr, fg, fb));
                    }
                }
                else {
                    return false;
                }
            }
        }
        else {
            for (std::size_t i = 0; i < v_count && std::getline(inp, line); i++) {
                if (boost::regex_match(line.c_str(), what, rx_p)) {
                    pt.x = (float)std::atof(what[1].first);
                    pt.y = (float)std::atof(what[4].first);
                    pt.z = (float)std::atof(what[7].first);
                    meshPoints.push_back(pt);
                }
                else {
                    return false;
                }
            }
        }
        int f1, f2, f3;
        for (std::size_t i = 0; i < f_count && std::getline(inp, line); i++) {
            if (boost::regex_match(line.c_str(), what, rx_f)) {
                f1 = std::atoi(what[1].first);
                f2 = std::atoi(what[2].first);
                f3 = std::atoi(what[3].first);
                meshFacets.push_back(MeshFacet(f1,f2,f3));
            }
        }
    }
    // binary
    else {
        Base::InputStream is(inp);
        if (format == binary_little_endian)
            is.setByteOrder(Base::Stream::LittleEndian);
        else
            is.setByteOrder(Base::Stream::BigEndian);
        int r,g,b;
        if (xyz_float) {
            Base::Vector3f pt;
            for (std::size_t i = 0; i < v_count; i++) {
                is >> pt.x >> pt.y >> pt.z;
                meshPoints.push_back(pt);
                if (rgb_value == MeshIO::PER_VERTEX) {
                    is >> r >> g >> b;
                    if (_material) {
                        float fr = (float)r/255.0f;
                        float fg = (float)g/255.0f;
                        float fb = (float)b/255.0f;
                        _material->diffuseColor.push_back(App::Color(fr, fg, fb));
                    }
                }
            }
        }
        else if (xyz_double) {
            Base::Vector3d pt;
            for (std::size_t i = 0; i < v_count; i++) {
                is >> pt.x >> pt.y >> pt.z;
                meshPoints.push_back(Base::Vector3f((float)pt.x,(float)pt.y,(float)pt.z));
                if (rgb_value == MeshIO::PER_VERTEX) {
                    is >> r >> g >> b;
                    if (_material) {
                        float fr = (float)r/255.0f;
                        float fg = (float)g/255.0f;
                        float fb = (float)b/255.0f;
                        _material->diffuseColor.push_back(App::Color(fr, fg, fb));
                    }
                }
            }
        }
        unsigned char n;
        int f1, f2, f3;
        for (std::size_t i = 0; i < f_count; i++) {
            is >> n;
            if (n==3) {
                is >> f1 >> f2 >> f3;
                meshFacets.push_back(MeshFacet(f1,f2,f3));
            }
        }
    }

    this->_rclMesh.Clear(); // remove all data before
    // Don't use Assign() because Merge() checks which points are really needed.
    // This method sets already the correct neighbourhood
    MeshKernel tmp;
    tmp.Adopt(meshPoints,meshFacets);
    this->_rclMesh.Merge(tmp);

    return true;
}

bool MeshInput::LoadMeshNode (std::istream &rstrIn)
{
    boost::regex rx_p("^v\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$");
    boost::regex rx_f("^f\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s*$");
    boost::regex rx_e("\\s*]\\s*");
    boost::cmatch what;

    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;

    std::string line;
    float fX, fY, fZ;
    unsigned int  i1=1,i2=1,i3=1;
    MeshGeomFacet clFacet;

    if (!rstrIn || rstrIn.bad() == true)
        return false;

    std::streambuf* buf = rstrIn.rdbuf();
    if (!buf)
        return false;

    while (std::getline(rstrIn, line)) {
        for (std::string::iterator it = line.begin(); it != line.end(); ++it)
            *it = tolower(*it);
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
            meshFacets.push_back(MeshFacet(i1-1,i2-1,i3-1));
        }
        else if (boost::regex_match(line.c_str(), what, rx_e)) {
            break;
        }
    }

    this->_rclMesh.Clear(); // remove all data before
    // Don't use Assign() because Merge() checks which points are really needed.
    // This method sets already the correct neighbourhood
    MeshKernel tmp;
    tmp.Adopt(meshPoints,meshFacets);
    this->_rclMesh.Merge(tmp);

    return true;
}

/** Loads an ASCII STL file. */
bool MeshInput::LoadAsciiSTL (std::istream &rstrIn)
{
    boost::regex rx_p("^\\s*VERTEX\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$");
    boost::regex rx_f("^\\s*FACET\\s+NORMAL\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$");
    boost::cmatch what;

    std::string line;
    float fX, fY, fZ;
    unsigned long ulVertexCt, ulFacetCt=0;
    MeshGeomFacet clFacet;

    if (!rstrIn || rstrIn.bad() == true)
        return false;

    std::streamoff ulSize = 0;
    std::streambuf* buf = rstrIn.rdbuf();
    ulSize = buf->pubseekoff(0, std::ios::end, std::ios::in);
    buf->pubseekoff(0, std::ios::beg, std::ios::in);
    ulSize -= 20;

    // count facets
    while (std::getline(rstrIn, line)) {
        for (std::string::iterator it = line.begin(); it != line.end(); ++it)
            *it = toupper(*it);
        if (line.find("ENDFACET") != std::string::npos)
            ulFacetCt++;
        // prevent from reading EOF (as I don't know how to reread the file then)
        if (rstrIn.tellg() > ulSize)
            break;
        else if (line.find("ENDSOLID") != std::string::npos)
            break;
    }

    // restart from the beginning
    buf->pubseekoff(0, std::ios::beg, std::ios::in);

    MeshBuilder builder(this->_rclMesh);
    builder.Initialize(ulFacetCt);

    ulVertexCt = 0;
    while (std::getline(rstrIn, line)) {
        for (std::string::iterator it = line.begin(); it != line.end(); ++it)
            *it = toupper(*it);
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
bool MeshInput::LoadBinarySTL (std::istream &rstrIn)
{
    char szInfo[80];
    Base::Vector3f clVects[4];
    uint16_t usAtt; 
    uint32_t ulCt;

    if (!rstrIn || rstrIn.bad() == true)
        return false;

    // Header-Info ueberlesen
    rstrIn.read(szInfo, sizeof(szInfo));

    // Anzahl Facets
    rstrIn.read((char*)&ulCt, sizeof(ulCt));
    if (rstrIn.bad() == true)
        return false;

    // get file size and calculate the number of facets
    std::streamoff ulSize = 0; 
    std::streambuf* buf = rstrIn.rdbuf();
    if (buf) {
        std::streamoff ulCurr;
        ulCurr = buf->pubseekoff(0, std::ios::cur, std::ios::in);
        ulSize = buf->pubseekoff(0, std::ios::end, std::ios::in);
        buf->pubseekoff(ulCurr, std::ios::beg, std::ios::in);
    }

    uint32_t ulFac = (ulSize - (80 + sizeof(uint32_t))) / 50;

    // compare the calculated with the read value
    if (ulCt > ulFac)
        return false;// not a valid STL file
 
    MeshBuilder builder(this->_rclMesh);
    builder.Initialize(ulCt);

    for (uint32_t i = 0; i < ulCt; i++) {
        // read normal, points
        rstrIn.read((char*)&clVects, sizeof(clVects));

        std::swap(clVects[0], clVects[3]);
        builder.AddFacet(clVects);

        // overread 2 bytes attribute
        rstrIn.read((char*)&usAtt, sizeof(usAtt));
    }

    builder.Finish();

    return true;
}

/** Loads the mesh object from an XML file. */
void MeshInput::LoadXML (Base::XMLReader &reader)
{
    MeshPointArray cPoints;
    MeshFacetArray cFacets;
 
//  reader.readElement("Mesh");

    reader.readElement("Points");
    int Cnt = reader.getAttributeAsInteger("Count");

    cPoints.resize(Cnt);
    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("P");
        cPoints[i].x = (float)reader.getAttributeAsFloat("x");
        cPoints[i].y = (float)reader.getAttributeAsFloat("y");
        cPoints[i].z = (float)reader.getAttributeAsFloat("z");
    }
    reader.readEndElement("Points");

    reader.readElement("Faces");
    Cnt = reader.getAttributeAsInteger("Count");

    cFacets.resize(Cnt);
    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("F");
        cFacets[i]._aulPoints[0] = reader.getAttributeAsInteger("p0");
        cFacets[i]._aulPoints[1] = reader.getAttributeAsInteger("p1");
        cFacets[i]._aulPoints[2] = reader.getAttributeAsInteger("p2");
        cFacets[i]._aulNeighbours[0] = reader.getAttributeAsInteger("n0");
        cFacets[i]._aulNeighbours[1] = reader.getAttributeAsInteger("n1");
        cFacets[i]._aulNeighbours[2] = reader.getAttributeAsInteger("n2");
    }

    reader.readEndElement("Faces");
    reader.readEndElement("Mesh");

    _rclMesh.Adopt(cPoints, cFacets);
}

/** Loads an OpenInventor file. */
bool MeshInput::LoadInventor (std::istream &rstrIn)
{
    if (!rstrIn || rstrIn.bad() == true)
        return false;

    boost::regex rx_p("\\s*([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s*[\\,\\]]\\s*");
    boost::regex rx_f("\\s*([0-9]+)\\s*\\,\\s*"
                      "\\s+([0-9]+)\\s*\\,\\s*"
                      "\\s+([0-9]+)\\s*\\,\\s*");
    boost::cmatch what;

    // get file size and estimate the number of lines
    std::streamoff ulSize = 0; 
    std::streambuf* buf = rstrIn.rdbuf();
    if (!buf)
        return false;
    if (buf) {
        std::streamoff ulCurr;
        ulCurr = buf->pubseekoff(0, std::ios::cur, std::ios::in);
        ulSize = buf->pubseekoff(0, std::ios::end, std::ios::in);
        buf->pubseekoff(ulCurr, std::ios::beg, std::ios::in);
    }

    std::string line;
    MeshGeomFacet clFacet;
    std::vector<MeshGeomFacet> clFacetAry;
    std::vector<Base::Vector3f> aclPoints;

    // We have approx. 30 characters per line
    Base::SequencerLauncher seq("Loading...", ulSize/30);
    bool flag = false;
    bool normals = false;
    bool points = false;
    bool facets = false;
    while (std::getline(rstrIn, line) && !facets) {
        for (std::string::iterator it = line.begin(); it != line.end(); ++it)
            *it = toupper(*it);

        // read the normals if they are defined
        if (!normals && line.find("NORMAL {") != std::string::npos) {
            float fX, fY, fZ;
            normals = true; // okay, the normals are set by an SoNormal node
            flag = true;
            // Get the next line and check for the normal field which might begin
            // with the first vector already i.e. it's of the form 'vector [ 0.0 0.0 1.0,'
            // This is a special case to support also file formats directly written by
            // Inventor 2.1 classes.
            std::getline(rstrIn, line);
            for (std::string::iterator it = line.begin(); it != line.end(); ++it)
                *it = toupper(*it);
            std::string::size_type pos = line.find("VECTOR [");
            if (pos != std::string::npos)
                line = line.substr(pos+8); // 8 = length of 'VECTOR ['
            do {
                if (boost::regex_match(line.c_str(), what, rx_p)) {
                    fX = (float)std::atof(what[1].first);
                    fY = (float)std::atof(what[4].first);
                    fZ = (float)std::atof(what[7].first);
                    clFacet.SetNormal(Base::Vector3f(fX, fY, fZ));
                    clFacetAry.push_back(clFacet);
                    seq.next(true); // allow to cancel
                } else 
                    flag = false;
            } while (std::getline(rstrIn, line) && flag);
        }
        // read the coordinates
        else if (!points && line.find("COORDINATE3 {") != std::string::npos) {
            Base::Vector3f clPoint;
            points = true; // the SoCoordinate3 node
            flag = true;
            // Get the next line and check for the points field which might begin
            // with the first point already i.e. it's of the form 'point [ 0.0 0.0 0.0,'
            // This is a special case to support also file formats directly written by
            // Inventor 2.1 classes.
            std::getline(rstrIn, line);
            for (std::string::iterator it = line.begin(); it != line.end(); ++it)
                *it = toupper(*it);
            std::string::size_type pos = line.find("POINT [");
            if (pos != std::string::npos)
                line = line.substr(pos+7); // 7 = length of 'POINT ['
            do {
                if (boost::regex_match(line.c_str(), what, rx_p)) {
                    clPoint.x = (float)std::atof(what[1].first);
                    clPoint.y = (float)std::atof(what[4].first);
                    clPoint.z = (float)std::atof(what[7].first);
                    aclPoints.push_back(clPoint);
                    seq.next(true); // allow to cancel
                } else 
                    flag = false;
            } while (std::getline(rstrIn, line) && flag);
        }
        // read the point indices of the facets
        else if (points && line.find("INDEXEDFACESET {") != std::string::npos) {
            unsigned long ulPoints[3];
            facets = true;
            flag = true;
            unsigned long ulCt = 0;
            // Get the next line and check for the index field which might begin
            // with the first index already.
            // This is a special case to support also file formats directly written by
            // Inventor 2.1 classes.
            // Furthermore we must check whether more than one triple is given per line, which
            // is handled in the while-loop.
            std::getline(rstrIn, line);
            for (std::string::iterator it = line.begin(); it != line.end(); ++it)
                *it = toupper(*it);
            std::string::size_type pos = line.find("COORDINDEX [");
            if (pos != std::string::npos)
                line = line.substr(pos+12); // 12 = length of 'COORDINDEX ['
            do {
                flag = false;
                std::string::size_type pos = line.find("-1");
                while (pos != std::string::npos) {
                    std::string part = line.substr(0, pos);
                    pos = line.find_first_of(",]", pos);
                    line = line.substr(pos+1);
                    pos = line.find("-1");
                    if (boost::regex_match(part.c_str(), what, rx_f)) {
                        flag = true;
                        ulPoints[0] = std::atol(what[1].first);
                        ulPoints[1] = std::atol(what[2].first);
                        ulPoints[2] = std::atol(what[3].first);
                        if (normals) {
                            // get a reference to the facet with defined normal
                            MeshGeomFacet& rclFacet = clFacetAry[ulCt++];
                            for (int i = 0; i < 3; i++)
                                rclFacet._aclPoints[i] = aclPoints[ulPoints[i]];
                        }
                        else {
                            for (int i = 0; i < 3; i++)
                                clFacet._aclPoints[i] = aclPoints[ulPoints[i]];
                            clFacetAry.push_back(clFacet);
                        }
                        seq.next(true); // allow to cancel
                    }
                }
            } while (std::getline(rstrIn, line) && flag);
        }
    }

    _rclMesh = clFacetAry;
    return (rstrIn?true:false);
}

/** Loads a Nastran file. */
bool MeshInput::LoadNastran (std::istream &rstrIn)
{
    if ((!rstrIn) || (rstrIn.bad() == true))
        return false;

    boost::regex rx_p("\\s*GRID\\s+([0-9]+)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*");
    boost::regex rx_t("\\s*CTRIA3\\s+([0-9]+)\\s+([0-9]+)"
                      "\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s*");
    boost::regex rx_q("\\s*CTRIA3\\s+([0-9]+)\\s+([0-9]+)"
                      "\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s*");
    boost::cmatch what;

    std::string line;
    MeshFacet clMeshFacet;
    MeshPointArray vVertices;
    MeshFacetArray vTriangle;

    int index;
    std::map <int, NODE> mNode;
    std::map <int, TRIA> mTria;
    std::map <int, QUAD> mQuad;

    while (std::getline(rstrIn, line)) {
        upper(ltrim(line));
        if (line.find("GRID*") == 0) {
            assert(0);
        }
        else if (line.find("*") == 0) {
            assert(0);
        }
        // insert the read-in vertex into a map to preserve the order
        else if (line.find("GRID") == 0) {
            if (boost::regex_match(line.c_str(), what, rx_p)) {
                index = std::atol(what[1].first)-1;
                mNode[index].x = (float)std::atof(what[2].first);
                mNode[index].y = (float)std::atof(what[5].first);
                mNode[index].z = (float)std::atof(what[8].first);
            }
        }
        // insert the read-in triangle into a map to preserve the order
        else if (line.find("CTRIA3 ") == 0) {
            if (boost::regex_match(line.c_str(), what, rx_t)) {
                index = std::atol(what[1].first)-1;
                mTria[index].iV[0] = std::atol(what[3].first)-1;
                mTria[index].iV[1] = std::atol(what[4].first)-1;
                mTria[index].iV[2] = std::atol(what[5].first)-1;
            }
        }
        // insert the read-in quadrangle into a map to preserve the order
        else if (line.find("CQUAD4") == 0) {
            if (boost::regex_match(line.c_str(), what, rx_q)) {
                index = std::atol(what[1].first)-1;
                mQuad[index].iV[0] = std::atol(what[3].first)-1;
                mQuad[index].iV[1] = std::atol(what[4].first)-1;
                mQuad[index].iV[2] = std::atol(what[5].first)-1;
                mQuad[index].iV[3] = std::atol(what[6].first)-1;
            }
        }
    }

    float fLength[2];
    if (mTria.empty())
        index = 0;
    else
        index = mTria.rbegin()->first + 1;
    for (std::map <int, QUAD>::iterator QI=mQuad.begin(); QI!=mQuad.end(); QI++) {
        for (int i = 0; i < 2; i++) {
            float fDx = mNode[(*QI).second.iV[i+2]].x - mNode[(*QI).second.iV[i]].x;
            float fDy = mNode[(*QI).second.iV[i+2]].y - mNode[(*QI).second.iV[i]].y;
            float fDz = mNode[(*QI).second.iV[i+2]].z - mNode[(*QI).second.iV[i]].z;
            fLength[i] = fDx*fDx + fDy*fDy + fDz*fDz;
        }
        if (fLength[0] < fLength[1]) {
            mTria[index].iV[0] = (*QI).second.iV[0];
            mTria[index].iV[1] = (*QI).second.iV[1];
            mTria[index].iV[2] = (*QI).second.iV[2];

            mTria[index+1].iV[0] = (*QI).second.iV[0];
            mTria[index+1].iV[1] = (*QI).second.iV[2];
            mTria[index+1].iV[2] = (*QI).second.iV[3];
        }
        else {
            mTria[index].iV[0] = (*QI).second.iV[0];
            mTria[index].iV[1] = (*QI).second.iV[1];
            mTria[index].iV[2] = (*QI).second.iV[3];

            mTria[index+1].iV[0] = (*QI).second.iV[1];
            mTria[index+1].iV[1] = (*QI).second.iV[2];
            mTria[index+1].iV[2] = (*QI).second.iV[3];
        }

        index += 2;
    }

    // Applying the nodes
    vVertices.reserve(mNode.size());
    for (std::map<int, NODE>::iterator MI=mNode.begin(); MI!=mNode.end(); MI++) {
        vVertices.push_back(Base::Vector3f(MI->second.x, MI->second.y, MI->second.z));
    }

    // Converting data to Mesh. Negative conversion for right orientation of normal-vectors.
    vTriangle.reserve(mTria.size());
    for (std::map<int, TRIA>::iterator MI=mTria.begin(); MI!=mTria.end(); MI++) {
        clMeshFacet._aulPoints[0] = (*MI).second.iV[1];
        clMeshFacet._aulPoints[1] = (*MI).second.iV[0];
        clMeshFacet._aulPoints[2] = (*MI).second.iV[2];
        vTriangle.push_back (clMeshFacet);
    }

    // make sure to add only vertices which are referenced by the triangles
    _rclMesh.Merge(vVertices, vTriangle);

    return true;
}

/** Loads a Cadmould FE file. */
bool MeshInput::LoadCadmouldFE (std::ifstream &rstrIn)
{
    if ((!rstrIn) || (rstrIn.bad() == true))
        return false;
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

void MeshOutput::Transform(const Base::Matrix4D& mat)
{
    _transform = mat;
    if (mat != Base::Matrix4D())
        apply_transform = true;
}

/// Save in a file, format is decided by the extension if not explicitly given
bool MeshOutput::SaveAny(const char* FileName, MeshIO::Format format) const
{
    // ask for write permission
    Base::FileInfo fi(FileName);
    Base::FileInfo di(fi.dirPath().c_str());
    if ((fi.exists() && !fi.isWritable()) || !di.exists() || !di.isWritable())
        throw Base::FileException("No write permission for file",FileName);

    MeshIO::Format fileformat = format;
    if (fileformat == MeshIO::Undefined) {
        if (fi.hasExtension("bms")) {
            fileformat = MeshIO::BMS;
        }
        else if (fi.hasExtension("stl")) {
            fileformat = MeshIO::BSTL;
        }
        else if (fi.hasExtension("ast")) {
            fileformat = MeshIO::ASTL;
        }
        else if (fi.hasExtension("obj")) {
            fileformat = MeshIO::OBJ;
        }
        else if (fi.hasExtension("off")) {
            fileformat = MeshIO::OFF;
        }
        else if (fi.hasExtension("ply")) {
            fileformat = MeshIO::PLY;
        }
        else if (fi.hasExtension("iv")) {
            fileformat = MeshIO::IV;
        }
        else if (fi.hasExtension("x3d")) {
            fileformat = MeshIO::X3D;
        }
        else if (fi.hasExtension("py")) {
            fileformat = MeshIO::PY;
        }
        else if (fi.hasExtension("wrl") || fi.hasExtension("vrml")) {
            fileformat = MeshIO::VRML;
        }
        else if (fi.hasExtension("wrz")) {
            fileformat = MeshIO::WRZ;
        }
        else if (fi.hasExtension("nas") || fi.hasExtension("bdf")) {
            fileformat = MeshIO::NAS;
        }
    }

    Base::ofstream str(fi, std::ios::out | std::ios::binary);

    if (fileformat == MeshIO::BMS) {
        _rclMesh.Write(str);
    }
    else if (fileformat == MeshIO::BSTL) {
        MeshOutput aWriter(_rclMesh);
        aWriter.Transform(this->_transform);

        // write file
        bool ok = false;
        ok = aWriter.SaveBinarySTL( str );
        if (!ok)
            throw Base::FileException("Export of STL mesh failed",FileName);
          
    }
    else if (fileformat == MeshIO::ASTL) {
        MeshOutput aWriter(_rclMesh);
        aWriter.Transform(this->_transform);

        // write file
        bool ok = false;
        ok = aWriter.SaveAsciiSTL( str );
        if (!ok)
            throw Base::FileException("Export of STL mesh failed",FileName);
          
    }
    else if (fileformat == MeshIO::OBJ) {
        // write file
        if (!SaveOBJ(str)) 
            throw Base::FileException("Export of OBJ mesh failed",FileName);
    }
    else if (fileformat == MeshIO::OFF) {
        // write file
        if (!SaveOFF(str)) 
            throw Base::FileException("Export of OFF mesh failed",FileName);
    }
    else if (fileformat == MeshIO::PLY) {
        // write file
        if (!SaveBinaryPLY(str)) 
            throw Base::FileException("Export of PLY mesh failed",FileName);
    }
    else if (fileformat == MeshIO::APLY) {
        // write file
        if (!SaveAsciiPLY(str)) 
            throw Base::FileException("Export of PLY mesh failed",FileName);
    }
    else if (fileformat == MeshIO::IV) {
        // write file
        if (!SaveInventor(str))
            throw Base::FileException("Export of Inventor mesh failed",FileName);
    }
    else if (fileformat == MeshIO::X3D) {
        // write file
        if (!SaveX3D(str))
            throw Base::FileException("Export of X3D failed",FileName);
    }
    else if (fileformat == MeshIO::PY) {
        // write file
        if (!SavePython(str))
            throw Base::FileException("Export of Python mesh failed",FileName);
    }
    else if (fileformat == MeshIO::VRML) {
        // write file
        App::Material clMat;
        if (!SaveVRML(str, clMat))
            throw Base::FileException("Export of VRML mesh failed",FileName);
    }
    else if (fileformat == MeshIO::WRZ) {
        // Compressed VRML is nothing else than a GZIP'ped VRML ascii file
        // str.close();
        //Base::ogzstream gzip(FileName, std::ios::out | std::ios::binary);
        //FIXME: The compression level seems to be higher than with ogzstream
        //which leads to problems to load the wrz file in debug mode, the
        //application simply crashes.
        zipios::GZIPOutputStream gzip(str);
        // write file
        App::Material clMat;
        if (!SaveVRML(gzip, clMat))
            throw Base::FileException("Export of compressed VRML mesh failed",FileName);
    }
    else if (fileformat == MeshIO::NAS) {
        // write file
        if (!SaveNastran(str))
            throw Base::FileException("Export of NASTRAN mesh failed",FileName);
    }
    else {
        throw Base::FileException("File format not supported", FileName);
    }

    return true;
}

/** Saves the mesh object into an ASCII file. */
bool MeshOutput::SaveAsciiSTL (std::ostream &rstrOut) const
{
    MeshFacetIterator clIter(_rclMesh), clEnd(_rclMesh);
    clIter.Transform(this->_transform);
    const MeshGeomFacet *pclFacet;
    unsigned long i;

    if (!rstrOut || rstrOut.bad() == true || _rclMesh.CountFacets() == 0)
        return false;

    rstrOut.precision(6);
    rstrOut.setf(std::ios::fixed | std::ios::showpoint);
    Base::SequencerLauncher seq("saving...", _rclMesh.CountFacets() + 1);

    rstrOut << "solid Mesh" << std::endl;

    clIter.Begin();
    clEnd.End();
    while (clIter < clEnd) {
        pclFacet = &(*clIter);
      
        // normal
        rstrOut << "  facet normal " << pclFacet->GetNormal().x << " " 
                                     << pclFacet->GetNormal().y << " "
                                     << pclFacet->GetNormal().z << std::endl;
        rstrOut << "    outer loop" << std::endl;

        // vertices
        for (i = 0; i < 3; i++) {
            rstrOut << "      vertex "  << pclFacet->_aclPoints[i].x << " "
                                        << pclFacet->_aclPoints[i].y << " "
                                        << pclFacet->_aclPoints[i].z << std::endl;
        }

        rstrOut << "    endloop" << std::endl;
        rstrOut << "  endfacet" << std::endl;

        ++clIter; 
        seq.next(true);// allow to cancel
    }

    rstrOut << "endsolid Mesh" << std::endl;
 
    return true;
}

/** Saves the mesh object into a binary file. */
bool MeshOutput::SaveBinarySTL (std::ostream &rstrOut) const
{
    MeshFacetIterator clIter(_rclMesh), clEnd(_rclMesh);
    clIter.Transform(this->_transform);
    const MeshGeomFacet *pclFacet;
    uint32_t i;
    uint16_t usAtt;
    char szInfo[81];

    if (!rstrOut || rstrOut.bad() == true /*|| _rclMesh.CountFacets() == 0*/)
        return false;

    Base::SequencerLauncher seq("saving...", _rclMesh.CountFacets() + 1);  
 
    strcpy(szInfo, stl_header.c_str());
    rstrOut.write(szInfo, std::strlen(szInfo));

    uint32_t uCtFts = (uint32_t)_rclMesh.CountFacets();
    rstrOut.write((const char*)&uCtFts, sizeof(uCtFts));

    usAtt = 0;
    clIter.Begin();
    clEnd.End();
    while (clIter < clEnd) {
        pclFacet = &(*clIter);
        // normal
        Base::Vector3f normal = pclFacet->GetNormal();
        rstrOut.write((const char*)&(normal.x), sizeof(float));
        rstrOut.write((const char*)&(normal.y), sizeof(float));
        rstrOut.write((const char*)&(normal.z), sizeof(float));

        // vertices
        for (i = 0; i < 3; i++) {
            rstrOut.write((const char*)&(pclFacet->_aclPoints[i].x), sizeof(float));
            rstrOut.write((const char*)&(pclFacet->_aclPoints[i].y), sizeof(float));
            rstrOut.write((const char*)&(pclFacet->_aclPoints[i].z), sizeof(float));
        }

        // attribute 
        rstrOut.write((const char*)&usAtt, sizeof(usAtt));

        ++clIter;
        seq.next(true); // allow to cancel
    }

    return true;
}

/** Saves an OBJ file. */
bool MeshOutput::SaveOBJ (std::ostream &rstrOut) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();

    if (!rstrOut || rstrOut.bad() == true)
        return false;

    Base::SequencerLauncher seq("saving...", _rclMesh.CountPoints() + _rclMesh.CountFacets());  

    // vertices
    if (this->apply_transform) {
        Base::Vector3f pt;
        for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
            pt = this->_transform * *it;
            rstrOut << "v " << pt.x << " " << pt.y << " " << pt.z << std::endl;
            seq.next(true); // allow to cancel
        }
    }
    else {
        for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
            rstrOut << "v " << it->x << " " << it->y << " " << it->z << std::endl;
            seq.next(true); // allow to cancel
        }
    }
    // facet indices (no texture and normal indices)
    for (MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it) {
        rstrOut << "f " << it->_aulPoints[0]+1 << " "
                        << it->_aulPoints[1]+1 << " "
                        << it->_aulPoints[2]+1 << std::endl;
        seq.next(true); // allow to cancel
    }

    return true;
}

/** Saves an OFF file. */
bool MeshOutput::SaveOFF (std::ostream &out) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();

    if (!out || out.bad() == true)
        return false;

    Base::SequencerLauncher seq("saving...", _rclMesh.CountPoints() + _rclMesh.CountFacets());  

    out << "OFF" << std::endl;
    out << rPoints.size() << " " << rFacets.size() << " 0" << std::endl;

    // vertices
    if (this->apply_transform) {
        Base::Vector3f pt;
        for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
            pt = this->_transform * *it;
            out << pt.x << " " << pt.y << " " << pt.z << std::endl;
            seq.next(true); // allow to cancel
        }
    }
    else {
        for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
            out << it->x << " " << it->y << " " << it->z << std::endl;
            seq.next(true); // allow to cancel
        }
    }

    // facet indices (no texture and normal indices)
    for (MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it) {
        out << "3 " << it->_aulPoints[0] 
            << " " << it->_aulPoints[1]
            << " " << it->_aulPoints[2] << std::endl;
        seq.next(true); // allow to cancel
    }

    return true;
}

bool MeshOutput::SaveBinaryPLY (std::ostream &out) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    std::size_t v_count = rPoints.size();
    std::size_t f_count = rFacets.size();
    if (!out || out.bad() == true)
        return false;
    bool saveVertexColor = (_material && _material->binding == MeshIO::PER_VERTEX
        && _material->diffuseColor.size() == rPoints.size());
    out << "ply" << std::endl
        << "format binary_little_endian 1.0" << std::endl
        << "comment Created by FreeCAD <http://www.freecadweb.org>" << std::endl
        << "element vertex " << v_count << std::endl
        << "property float32 x" << std::endl
        << "property float32 y" << std::endl
        << "property float32 z" << std::endl;
    if (saveVertexColor) {
        out << "property uchar red" << std::endl
            << "property uchar green" << std::endl
            << "property uchar blue" << std::endl;
    }
    out << "element face " << f_count << std::endl
        << "property list uchar int vertex_index" << std::endl
        << "end_header" << std::endl;

    Base::OutputStream os(out);
    os.setByteOrder(Base::Stream::LittleEndian);
    Base::Vector3f pt;
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
            const App::Color& c = _material->diffuseColor[i];
            int r = (int)(255.0f * c.r);
            int g = (int)(255.0f * c.g);
            int b = (int)(255.0f * c.b);
            os << r << g << b;
        }
    }
    unsigned char n = 3;
    int f1, f2, f3;
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

bool MeshOutput::SaveAsciiPLY (std::ostream &out) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    std::size_t v_count = rPoints.size();
    std::size_t f_count = rFacets.size();
    if (!out || out.bad() == true)
        return false;

    bool saveVertexColor = (_material && _material->binding == MeshIO::PER_VERTEX
        && _material->diffuseColor.size() == rPoints.size());
    out << "ply" << std::endl
        << "format ascii 1.0" << std::endl
        << "comment Created by FreeCAD <http://www.freecadweb.org>" << std::endl
        << "element vertex " << v_count << std::endl
        << "property float32 x" << std::endl
        << "property float32 y" << std::endl
        << "property float32 z" << std::endl;
    if (saveVertexColor) {
        out << "property uchar red" << std::endl
            << "property uchar green" << std::endl
            << "property uchar blue" << std::endl;
    }
    out << "element face " << f_count << std::endl
        << "property list uchar int vertex_index" << std::endl
        << "end_header" << std::endl;

    Base::Vector3f pt;

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

            const App::Color& c = _material->diffuseColor[i];
            int r = (int)(255.0f * c.r);
            int g = (int)(255.0f * c.g);
            int b = (int)(255.0f * c.b);
            out << " " << r << " " << g << " " << b << std::endl;
        }
    }
    else {
        for (std::size_t i = 0; i < v_count; i++) {
            const MeshPoint& p = rPoints[i];
            if (this->apply_transform) {
                Base::Vector3f pt = this->_transform * p;
                out << pt.x << " " << pt.y << " " << pt.z << std::endl;
            }
            else {
                out << p.x << " " << p.y << " " << p.z << std::endl;
            }
        }
    }

    unsigned int n = 3;
    int f1, f2, f3;
    for (std::size_t i = 0; i < f_count; i++) {
        const MeshFacet& f = rFacets[i];
        f1 = (int)f._aulPoints[0];
        f2 = (int)f._aulPoints[1];
        f3 = (int)f._aulPoints[2];
        out << n << " " << f1 << " " << f2 << " " << f3 << std::endl;
    }

    return true;
}

bool MeshOutput::SaveMeshNode (std::ostream &rstrOut)
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();

    if (!rstrOut || rstrOut.bad() == true)
        return false;

    // vertices
    rstrOut << "[" << std::endl;
    if (this->apply_transform) {
        Base::Vector3f pt;
        for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
            pt = this->_transform * *it;
            rstrOut << "v " << pt.x << " " << pt.y << " " << pt.z << std::endl;
        }
    }
    else {
        for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
            rstrOut << "v " << it->x << " " << it->y << " " << it->z << std::endl;
        }
    }
    // facet indices (no texture and normal indices)
    for (MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it) {
        rstrOut << "f " << it->_aulPoints[0]+1 << " "
                        << it->_aulPoints[1]+1 << " "
                        << it->_aulPoints[2]+1 << std::endl;
    }
    rstrOut << "]" << std::endl;

    return true;
}

/** Saves the mesh object into an XML file. */
void MeshOutput::SaveXML (Base::Writer &writer) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();

    //  writer << writer.ind() << "<Mesh>" << std::endl;

    writer.incInd();
    writer.Stream() << writer.ind() << "<Points Count=\"" << _rclMesh.CountPoints() << "\">" << std::endl;

    writer.incInd();
    if (this->apply_transform) {
        Base::Vector3f pt;
        for (MeshPointArray::_TConstIterator itp = rPoints.begin(); itp != rPoints.end(); itp++) {
            pt = this->_transform * *itp;
            writer.Stream() <<  writer.ind() << "<P "
                            << "x=\"" <<  pt.x << "\" "
                            << "y=\"" <<  pt.y << "\" "
                            << "z=\"" <<  pt.z << "\"/>"
                            << std::endl;
        }
    }
    else {
        for (MeshPointArray::_TConstIterator itp = rPoints.begin(); itp != rPoints.end(); itp++) {
            writer.Stream() <<  writer.ind() << "<P "
                            << "x=\"" <<  itp->x << "\" "
                            << "y=\"" <<  itp->y << "\" "
                            << "z=\"" <<  itp->z << "\"/>"
                            << std::endl;
        }
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Points>" << std::endl;

    // write the faces
    writer.Stream() << writer.ind() << "<Faces Count=\"" << _rclMesh.CountFacets() << "\">" << std::endl;

    writer.incInd();
    for (MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); it++) {
        writer.Stream() << writer.ind() << "<F "
                        << "p0=\"" <<  it->_aulPoints[0] << "\" "
                        << "p1=\"" <<  it->_aulPoints[1] << "\" "
                        << "p2=\"" <<  it->_aulPoints[2] << "\" " 
                        << "n0=\"" <<  it->_aulNeighbours[0] << "\" "
                        << "n1=\"" <<  it->_aulNeighbours[1] << "\" "
                        << "n2=\"" <<  it->_aulNeighbours[2] << "\"/>"
                        << std::endl;
    } 
    writer.decInd();
    writer.Stream() << writer.ind() << "</Faces>" << std::endl;

    writer.Stream() << writer.ind() << "</Mesh>" << std::endl;
    writer.decInd();
}

/** Writes an OpenInventor file. */
bool MeshOutput::SaveInventor (std::ostream &rstrOut) const
{
    if ((!rstrOut) || (rstrOut.bad() == true) || (_rclMesh.CountFacets() == 0))
        return false;

    MeshFacetIterator clIter(_rclMesh), clEnd(_rclMesh);
    clIter.Transform(this->_transform);
    MeshPointIterator clPtIter(_rclMesh), clPtEnd(_rclMesh);
    clPtIter.Transform(this->_transform);
    const MeshGeomFacet* pclFacet;
    unsigned long ulAllFacets = _rclMesh.CountFacets();

    Base::SequencerLauncher seq("Saving...", _rclMesh.CountFacets() + 1);
    rstrOut.precision(6);
    rstrOut.setf(std::ios::fixed | std::ios::showpoint);

    // Header info
    rstrOut << "#Inventor V2.1 ascii\n" << std::endl;
    rstrOut << "# Created by FreeCAD <http://www.freecadweb.org>" << std::endl;
    rstrOut << "# Triangle mesh contains " << _rclMesh.CountPoints() << " vertices"
            << " and " << _rclMesh.CountFacets() << " faces" << std::endl;
    rstrOut << "Separator {\n" << std::endl;
    rstrOut << "  Label {" << std::endl;
    rstrOut << "    label \"Triangle mesh\"\n  }" << std::endl;

    // write out the normals of the facets
    rstrOut << "  Normal { " << std::endl;
    rstrOut << "    vector [ ";

    clIter.Begin();
    clEnd.End();

    pclFacet = &(*clIter);
    rstrOut << pclFacet->GetNormal().x << "  "
            << pclFacet->GetNormal().y << "  "
            << pclFacet->GetNormal().z;
    ++clIter;

    while (clIter < clEnd) {
        pclFacet = &(*clIter);
        rstrOut << ",\n        "
                << pclFacet->GetNormal().x << "  "
                << pclFacet->GetNormal().y << "  "
                << pclFacet->GetNormal().z;
        ++clIter;

        seq.next(true); // allow to cancel
    }

    rstrOut << " ]\n\n  }" << std::endl;

    // coordinates of the vertices
    rstrOut << "  NormalBinding {\n    value PER_FACE\n  }" << std::endl;
    rstrOut << "  Coordinate3 {\n    point [ ";

    clPtIter.Begin();
    clPtEnd.End();

    rstrOut << clPtIter->x << "  "
            << clPtIter->y << "  "
            << clPtIter->z;
    ++clPtIter;

    while (clPtIter < clPtEnd) {
        rstrOut << ",\n        " 
                << clPtIter->x << "  "
                << clPtIter->y << "  "
                << clPtIter->z;
        ++clPtIter;
        seq.next(true); // allow to cancel
    }

    rstrOut << " ]\n\n  }" << std::endl;

    // and finally the facets with their point indices
    rstrOut << "  IndexedFaceSet {\n    coordIndex [ ";

    const MeshFacet clFacet = _rclMesh.GetFacets()[0];
    rstrOut << clFacet._aulPoints[0] << ", "
            << clFacet._aulPoints[1] << ", "
            << clFacet._aulPoints[2] << ", -1";

    unsigned long i = 1;
    while (i < ulAllFacets) {
        // write two triples per line
        const MeshFacet clFacet = _rclMesh.GetFacets()[i];
        if ( i%2==0 ) {
            rstrOut << ",\n        "
                    << clFacet._aulPoints[0] << ", "
                    << clFacet._aulPoints[1] << ", "
                    << clFacet._aulPoints[2] << ", -1";
        }
        else {
            rstrOut << ", "
                    << clFacet._aulPoints[0] << ", "
                    << clFacet._aulPoints[1] << ", "
                    << clFacet._aulPoints[2] << ", -1";
        }
        ++i;
    }

    rstrOut << " ]\n\n  }" << std::endl;
    rstrOut << "#End of triangle mesh \n}\n" << std::endl;

    return true;
}

/** Writes an X3D file. */
bool MeshOutput::SaveX3D (std::ostream &out) const
{
    if ((!out) || (out.bad() == true) || (_rclMesh.CountFacets() == 0))
        return false;

    const MeshPointArray& pts = _rclMesh.GetPoints();
    const MeshFacetArray& fts = _rclMesh.GetFacets();

    Base::SequencerLauncher seq("Saving...", _rclMesh.CountFacets() + 1);
    out.precision(6);
    out.setf(std::ios::fixed | std::ios::showpoint);

    // Header info
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    out << "<X3D profile=\"Immersive\" version=\"3.2\" xmlns:xsd="
        << "\"http://www.w3.org/2001/XMLSchema-instance\" xsd:noNamespaceSchemaLocation="
        << "\"http://www.web3d.org/specifications/x3d-3.2.xsd\">" << std::endl;
    out << "  <head>" << std::endl
        << "    <meta name=\"generator\" content=\"FreeCAD\"/>" << std::endl
        << "    <meta name=\"author\" content=\"\"/> " << std::endl
        << "    <meta name=\"company\" content=\"\"/>" << std::endl
        << "  </head>" << std::endl;

    // Beginning
    out << "  <Scene>" << std::endl;
    if (apply_transform) {
        Base::Placement p(_transform);
        const Base::Vector3d& v = p.getPosition();
        const Base::Rotation& r = p.getRotation();
        Base::Vector3d axis; double angle;
        r.getValue(axis, angle);
        out << "    <Transform "
            << "translation='"
            << v.x << " "
            << v.y << " "
            << v.z << "' "
            << "rotation='"
            << axis.x << " "
            << axis.y << " "
            << axis.z << " "
            << angle << "'>" << std::endl;
    }
    else {
        out << "    <Transform>" << std::endl;
    }
    out << "      <Shape>" << std::endl;

    out << "        <IndexedFaceSet solid=\"false\" coordIndex=\"";
    for (MeshFacetArray::_TConstIterator it = fts.begin(); it != fts.end(); ++it) {
        out << it->_aulPoints[0] << " " << it->_aulPoints[1] << " " << it->_aulPoints[2] << " -1 ";
    }
    out << "\">" << std::endl;

    out << "          <Coordinate point=\"";
    for (MeshPointArray::_TConstIterator it = pts.begin(); it != pts.end(); ++it) {
        out << it->x << " " << it->y << " " << it->z << ", ";
    }
    out << "\"/>" << std::endl;

    // End
    out << "        </IndexedFaceSet>" << std::endl
        << "      </Shape>" << std::endl
        << "    </Transform>" << std::endl
        << "  </Scene>" << std::endl
        << "</X3D>" << std::endl;

    return true;
}

/** Writes a Nastran file. */
bool MeshOutput::SaveNastran (std::ostream &rstrOut) const
{
    if ((!rstrOut) || (rstrOut.bad() == true) || (_rclMesh.CountFacets() == 0))
        return false;

    MeshPointIterator clPIter(_rclMesh);
    clPIter.Transform(this->_transform);
    MeshFacetIterator clTIter(_rclMesh);
    int iIndx = 1;

    Base::SequencerLauncher seq("Saving...", _rclMesh.CountFacets() + 1);

    rstrOut.precision(3);
    rstrOut.setf(std::ios::fixed | std::ios::showpoint);
    for (clPIter.Init(); clPIter.More(); clPIter.Next()) {
        float x = clPIter->x;
        float y = clPIter->y;
        float z = clPIter->z;

        rstrOut << "GRID";

        rstrOut << std::setfill(' ') << std::setw(12) << iIndx;
        rstrOut << std::setfill(' ') << std::setw(16) << x;
        rstrOut << std::setfill(' ') << std::setw(8)  << y;
        rstrOut << std::setfill(' ') << std::setw(8)  << z;
        rstrOut << std::endl;

        iIndx++;
        seq.next();
    }

    iIndx = 1;
    for (clTIter.Init(); clTIter.More(); clTIter.Next()) {
        rstrOut << "CTRIA3";

        rstrOut << std::setfill(' ') << std::setw(10) << iIndx;
        rstrOut << std::setfill(' ') << std::setw(8) << (int)0;
        rstrOut << std::setfill(' ') << std::setw(8) << clTIter.GetIndices()._aulPoints[1]+1;
        rstrOut << std::setfill(' ') << std::setw(8) << clTIter.GetIndices()._aulPoints[0]+1;
        rstrOut << std::setfill(' ') << std::setw(8) << clTIter.GetIndices()._aulPoints[2]+1;
        rstrOut <<std::endl;

        iIndx++;
        seq.next();
    }

    rstrOut << "ENDDATA";

    return true;
}

/** Writes a Cadmould FE file. */
bool MeshOutput::SaveCadmouldFE (std::ostream &rstrOut) const
{
    return false;
}

/** Writes a Python module */
bool MeshOutput::SavePython (std::ostream &str) const
{
    if ((!str) || (str.bad() == true) || (_rclMesh.CountFacets() == 0))
        return false;

    MeshFacetIterator clIter(_rclMesh);
    clIter.Transform(this->_transform);
    str.precision(4);
    str.setf(std::ios::fixed | std::ios::showpoint);

    str << "faces = [" << std::endl;
    for (clIter.Init(); clIter.More(); clIter.Next()) {
        const MeshGeomFacet& rFacet = *clIter;
        for (int i = 0; i < 3; i++) {
            str << "[" << rFacet._aclPoints[i].x
                << "," << rFacet._aclPoints[i].y
                << "," << rFacet._aclPoints[i].z
                << "],";
        }
        str << std::endl;
    }

    str << "]" << std::endl;

    return true;
}

// --------------------------------------------------------------

class MeshVRML
{
public:
    MeshVRML (const MeshKernel &rclM, const Base::Matrix4D&);
    MeshVRML (const MeshKernel &rclM, const Base::Matrix4D&,VRMLInfo* pclVRMLInfo);
    ~MeshVRML (void){}

    bool Save (std::ostream &rstrOut, const App::Material &rclMat) const;
    bool Save (std::ostream &rstrOut, const std::vector<App::Color> &raclColor, 
               const App::Material &rclMat, bool bColorPerVertex = true) const;

protected:
    void WriteVRMLHeaderInfo(std::ostream &rstrOut) const;
    void WriteVRMLAnnotations(std::ostream &rstrOut) const;
    void WriteVRMLViewpoints(std::ostream &rstrOut) const;

    const MeshKernel &_rclMesh;   // reference to mesh data structure
    Base::Matrix4D _transform;
    VRMLInfo* _pclVRMLInfo;
};

/** Writes a VRML file. */
bool MeshOutput::SaveVRML (std::ostream &rstrOut, const App::Material &rclMat) const
{
    return MeshVRML(_rclMesh, this->_transform).Save(rstrOut, rclMat);
}

MeshVRML::MeshVRML (const MeshKernel &rclM, const Base::Matrix4D& trf)
  : _rclMesh(rclM), _transform(trf), _pclVRMLInfo(0)
{
}

MeshVRML::MeshVRML (const MeshKernel &rclM, const Base::Matrix4D& trf, VRMLInfo* pclVRMLInfo)
  : _rclMesh(rclM), _transform(trf), _pclVRMLInfo(pclVRMLInfo)
{
}

bool MeshVRML::Save (std::ostream &rstrOut, const std::vector<App::Color> &raclColor, 
                     const App::Material &rclMat, bool bColorPerVertex) const
{
    if ((!rstrOut) || (rstrOut.bad() == true) || (_rclMesh.CountFacets() == 0))
        return false;

    Base::BoundBox3f clBB = _rclMesh.GetBoundBox();
    Base::Vector3f   clCenter = clBB.CalcCenter();

    Base::SequencerLauncher seq("Saving VRML file...", 
        _rclMesh.CountPoints() + raclColor.size() + _rclMesh.CountFacets());

    rstrOut << "#VRML V2.0 utf8" << std::endl;

    if (_pclVRMLInfo)
        WriteVRMLHeaderInfo(rstrOut);
    else
        rstrOut << "WorldInfo {\n"
                << "  title \"Exported tringle mesh to VRML97\"\n"
                << "  info [\"Created by FreeCAD\"\n"
                << "        \"<http://www.freecadweb.org>\"]\n"
                << "}\n";

    // Write background
    // Note: We must write the background info straight after the header and before any geometry.
    // To see the background color in the Inventor viewer we must either skip the skyAngle property
    // or set it to 1.57, otherwise we'll see a strange beam in the scene.
    // FIXME: Test this also with external VRML viewer plugins.
    if (_pclVRMLInfo) {
        float r = float(_pclVRMLInfo->_clColor.r) / 255.0f;
        float g = float(_pclVRMLInfo->_clColor.g) / 255.0f;
        float b = float(_pclVRMLInfo->_clColor.b) / 255.0f;
        rstrOut.precision(1);
        rstrOut.setf(std::ios::fixed | std::ios::showpoint);
        rstrOut << "Background\n{\n  skyAngle    1.57\n  skyColor    "
                << r << " " << g << " " << b << "\n}" << std::endl;
    }
    else {
        rstrOut << "Background\n{\n  skyAngle    1.57\n  skyColor    "
                << "0.1 0.2 0.2\n}" << std::endl;
    }
  
    // Transform
    rstrOut.precision(3);
    rstrOut.setf(std::ios::fixed | std::ios::showpoint);
    rstrOut << "Transform\n{\n"
            << "scale             1 1 1\n"
            << "rotation          0 0 1 0\n"
            << "scaleOrientation  0 0 1 0\n"
            << "bboxCenter        0 0 0\n"
            << "bboxSize      "
            << clBB.LengthX() << " "
            << clBB.LengthY() << " "
            << clBB.LengthZ() << std::endl;
    rstrOut << "center        "
            << 0.0f << "  "
            << 0.0f << "  "
            << 0.0f << std::endl;
    rstrOut << "translation  "
            << 0.0f << "  "
            << 0.0f << "  "
            << 0.0f << std::endl;

    rstrOut << "children\n[\n";
    rstrOut << "Shape\n{" << std::endl;

    // write appearance
    rstrOut << "  appearance Appearance\n  {\n    material Material\n    {\n";
    rstrOut << "      ambientIntensity 0.2" << std::endl;

    rstrOut.precision(2);
    rstrOut.setf(std::ios::fixed | std::ios::showpoint);
    if (raclColor.size() > 0) { // black
        rstrOut << "      diffuseColor     0.2 0.2 0.2\n";
        rstrOut << "      emissiveColor    0.2 0.2 0.2\n";
        rstrOut << "      specularColor    0.2 0.2 0.2\n";
    }
    else {
        App::Color clCol;
        clCol = rclMat.diffuseColor;
        rstrOut << "      diffuseColor      "
                << clCol.r << " "
                << clCol.g << " "
                << clCol.b << std::endl;
        clCol = rclMat.emissiveColor;
        rstrOut << "      emissiveColor     "
                << clCol.r << " "
                << clCol.g << " "
                << clCol.b << std::endl;
        clCol = rclMat.specularColor;
        rstrOut << "      specularColor     "
                << clCol.r << " "
                << clCol.g << " "
                << clCol.b << std::endl;
    }

    rstrOut << "      shininess        " << rclMat.shininess << std::endl;
    rstrOut << "      transparency     " << rclMat.transparency << std::endl;
    rstrOut << "    }\n  }" << std::endl; // end write appearance


    // write IndexedFaceSet
    rstrOut << "  geometry IndexedFaceSet\n  {\n";
    rstrOut << "    solid   FALSE\n";

    // write coords
    rstrOut << "    coord Coordinate\n    {\n      point\n      [\n";
    MeshPointIterator pPIter(_rclMesh);
    pPIter.Transform(this->_transform);
    unsigned long i = 0, k = _rclMesh.CountPoints();
    rstrOut.precision(3);
    rstrOut.setf(std::ios::fixed | std::ios::showpoint);
    for (pPIter.Init(); pPIter.More(); pPIter.Next()) {
        rstrOut << "        "
                << pPIter->x << " "
                << pPIter->y << " "
                << pPIter->z;
        if (i++ < (k-1))
            rstrOut << ",\n";
        else
            rstrOut << "\n";

        seq.next();
    }

    rstrOut << "      ]\n    }" << std::endl;  // end write coord

    rstrOut << "    colorPerVertex  " 
            << (bColorPerVertex ? "TRUE" : "FALSE") << std::endl;

    if (raclColor.size() > 0) {
        // write colors for each vertex
        rstrOut << "    color Color\n    {\n       color\n       [\n";
        rstrOut.precision(3);
        rstrOut.setf(std::ios::fixed | std::ios::showpoint);
        for (std::vector<App::Color>::const_iterator pCIter = raclColor.begin();
            pCIter != raclColor.end(); pCIter++) {
            rstrOut << "         "
                    << float(pCIter->r) / 255.0f << " "
                    << float(pCIter->g) / 255.0f << " "
                    << float(pCIter->b) / 255.0f;
            if (pCIter < (raclColor.end() - 1))
                rstrOut << ",\n";
            else
                rstrOut << "\n";
            seq.next();
        }

        rstrOut << "      ]\n    }" << std::endl;
    }

    // write face index
    rstrOut << "    coordIndex\n    [\n";
    MeshFacetIterator pFIter(_rclMesh);
    pFIter.Transform(this->_transform);
    i = 0, k = _rclMesh.CountFacets();

    for (pFIter.Init(); pFIter.More(); pFIter.Next()) {
        MeshFacet clFacet = pFIter.GetIndices();
        rstrOut << "      "
                << clFacet._aulPoints[0] << ", "
                << clFacet._aulPoints[1] << ", "
                << clFacet._aulPoints[2] << ", -1";
        if (i++ < (k-1))
            rstrOut << ",\n";
        else
            rstrOut << "\n";

        seq.next();
    }

    rstrOut << "    ]\n  }" << std::endl;  // End IndexedFaceSet
    rstrOut << "}" << std::endl;  // End Shape

    // close children and Transform
    rstrOut << "]\n}" << std::endl;

    // write viewpoints
    rstrOut.precision(3);
    rstrOut.setf(std::ios::fixed | std::ios::showpoint);
    for (i = 0; i < 6; i++) {
        rstrOut << "Viewpoint\n{\n"
                << "  jump         TRUE\n";

        Base::Vector3f clPos = clCenter;
        float fLen  = clBB.CalcDiagonalLength();
        switch (i)
        {
        case 0:
            {
                Vector at, pos, up, rot_axis;
                double rot_angle;
                at.x  = clPos.x;        at.y  = clPos.y;        at.z  = clPos.z;
                pos.x = clPos.x;        pos.y = clPos.y;        pos.z = clPos.z + fLen;
                up.x  = 0.0;            up.y  = 1.0;            up.z  = 0.0;
                Convert_Camera_Model(&pos, &at, &up, &rot_axis, &rot_angle);
                rstrOut << "  orientation   "
                        << rot_axis.x << " "
                        << rot_axis.y << " "
                        << rot_axis.z << " "
                        << rot_angle << "\n";
                rstrOut << "  description  \"top\"\n";
                clPos.z += fLen;
                break;
            }
        case 1:
            {
                Vector at, pos, up, rot_axis;
                double rot_angle;
                at.x  = clPos.x;        at.y  = clPos.y;        at.z  = clPos.z;
                pos.x = clPos.x;        pos.y = clPos.y;        pos.z = clPos.z - fLen;
                up.x  = 0.0;            up.y  = -1.0;           up.z  = 0.0;
                Convert_Camera_Model(&pos, &at, &up, &rot_axis, &rot_angle);
                rstrOut << "  orientation   "
                        << rot_axis.x << " "
                        << rot_axis.y << " "
                        << rot_axis.z << " "
                        << rot_angle << "\n";
                rstrOut << "  description  \"bottom\"\n";
                clPos.z -= fLen;
                break;
            }
        case 2:
            {
                Vector at, pos, up, rot_axis;
                double rot_angle;
                at.x  = clPos.x;        at.y  = clPos.y;        at.z  = clPos.z;
                pos.x = clPos.x;        pos.y = clPos.y - fLen; pos.z = clPos.z;
                up.x  = 0.0;            up.y  = 0.0;            up.z  = 1.0;
                Convert_Camera_Model(&pos, &at, &up, &rot_axis, &rot_angle);
                rstrOut << "  orientation   "
                        << rot_axis.x << " "
                        << rot_axis.y << " "
                        << rot_axis.z << " "
                        << rot_angle << "\n";
                rstrOut << "  description  \"front\"\n";
                clPos.y -= fLen;
                break;
            }
        case 3:
            {
                Vector at, pos, up, rot_axis;
                double rot_angle;
                at.x  = clPos.x;        at.y  = clPos.y;        at.z  = clPos.z;
                pos.x = clPos.x;        pos.y = clPos.y + fLen; pos.z = clPos.z;
                up.x  = 0.0;            up.y  = 0.0;            up.z  = 1.0;
                Convert_Camera_Model(&pos, &at, &up, &rot_axis, &rot_angle);
                rstrOut << "  orientation   "
                        << rot_axis.x << " "
                        << rot_axis.y << " "
                        << rot_axis.z << " "
                        << rot_angle << "\n";
                rstrOut << "  description  \"back\"\n";
                clPos.y += fLen;
                break;
            }
        case 4:
            {
                Vector at, pos, up, rot_axis;
                double rot_angle;
                at.x  = clPos.x;        at.y  = clPos.y;        at.z  = clPos.z;
                pos.x = clPos.x - fLen; pos.y = clPos.y;        pos.z = clPos.z;
                up.x  = 0.0;            up.y  = 0.0;            up.z  = 1.0;
                Convert_Camera_Model(&pos, &at, &up, &rot_axis, &rot_angle);
                rstrOut << "  orientation   "
                        << rot_axis.x << " "
                        << rot_axis.y << " "
                        << rot_axis.z << " "
                        << rot_angle << "\n";
                rstrOut << "  description  \"right\"\n";
                clPos.x -= fLen;
                break;
            }
        case 5:
            {
                Vector at, pos, up, rot_axis;
                double rot_angle;
                at.x  = clPos.x;        at.y  = clPos.y;        at.z  = clPos.z;
                pos.x = clPos.x + fLen; pos.y = clPos.y;        pos.z = clPos.z;
                up.x  = 0.0;            up.y  = 0.0;            up.z  = 1.0;
                Convert_Camera_Model(&pos, &at, &up, &rot_axis, &rot_angle);
                rstrOut << "  orientation   "
                        << rot_axis.x << " "
                        << rot_axis.y << " "
                        << rot_axis.z << " "
                        << rot_angle << "\n";
                rstrOut << "  description  \"left\"\n";
                clPos.x += fLen;
                break;
            }
        }

        rstrOut << "  position     "
                << clPos.x << " "
                << clPos.y << " "
                << clPos.z << "\n}" << std::endl;
    }

    // write navigation info
    rstrOut << "NavigationInfo\n{\n"
            << "  avatarSize       [0.25, 1.6, 0.75]\n"
            << "  headlight        TRUE\n"
            << "  speed            1.0\n"
            << "  type             \"EXAMINE\"\n"
            << "  visibilityLimit  0.0\n"
            << "}" << std::endl;

    // write custom viewpoints
    if (_pclVRMLInfo)
        WriteVRMLViewpoints(rstrOut);

    if (_pclVRMLInfo)
        WriteVRMLAnnotations(rstrOut);

    return true;
}

void MeshVRML::WriteVRMLHeaderInfo(std::ostream &rstrOut) const
{
    // save information about file
    //
    rstrOut << "#=================================================#\n#\n"
            << "# F I L E   I N F O R M A T I O N\n#\n"
            << "# This file was created by " << _pclVRMLInfo->_clAuthor << "\n"
            << "# Creation Date:    " << _pclVRMLInfo->_clDate << "\n"
            << "# Company:          " << _pclVRMLInfo->_clCompany << "\n";
    std::vector<std::string>::const_iterator sIt = _pclVRMLInfo->_clComments.begin();
    rstrOut << "# Further comments: " << *sIt << "\n";
    for (sIt++ ;sIt != _pclVRMLInfo->_clComments.end(); ++sIt) {
        rstrOut << "#                   " << *sIt << "\n";
    }
    rstrOut << "#=================================================#\n" << std::endl;
}

void MeshVRML::WriteVRMLAnnotations(std::ostream &rstrOut) const
{
    float r = float(_pclVRMLInfo->_clColor.r) / 255.0f;
    float g = float(_pclVRMLInfo->_clColor.g) / 255.0f;
    float b = float(_pclVRMLInfo->_clColor.b) / 255.0f;
    float textr = 1.0f - r;
    float textg = 1.0f - g;
    float textb = 1.0f - b;

    if (fabs(textr-r) < 0.05)
        textr = 1.0f;
    if (fabs(textg-g) < 0.05)
        textg = 1.0f;
    if (fabs(textb-b) < 0.05)
        textb = 1.0f;

    // annotationen
    rstrOut << "DEF User ProximitySensor {\n"
            << " size        1000000 1000000 1000000\n"
            << "}\n"
            << "\n"
            << "    Group { \n"
            << "      children [\n"
            << " DEF UserPos Transform {\n"
            << "   children [\n"
            << "     # Text position\n"
            << "     Transform {\n"
            << "       translation  -1.0 -0.75 -2\n"
            << "       children [\n"
            << "          Transform {\n"
            << "            translation 1.95 0.75 0\n"
            << "            children [\n"
            << "              Shape {\n";
    if (_pclVRMLInfo->_bSavePicture) {
        rstrOut << "                appearance Appearance {\n" 
                << "              texture ImageTexture { \n"
                << "                url \""
                << _pclVRMLInfo->_clPicFileName
                << "\"\n"
                << "                repeatS FALSE\n"
                << "                repeatT FALSE\n"
                << "              }\n"
                << "                }\n"
                << "                geometry IndexedFaceSet {\n"
                << "              coord Coordinate { point [ -0.08 -0.8 0,\n"
                << "                             0.08 -0.8 0,\n"
                << "                             0.08  0.8 0,\n"
                << "                             -0.08  0.8 0\n"
                << "                           ]\n"
                << "                       }\n"
                << "              coordIndex [0,1,2,3, -1]\n"
                << "              texCoord TextureCoordinate {\n"
                << "               point   [ 0 0,\n"
                << "                    1 0,\n"
                << "                    1 1,\n"
                << "                   0 1 ]\n"
                << "              }\n"
                << "              texCoordIndex	[ 0, 1, 2, 3, -1 ]\n\n"
                << "             solid FALSE\n"
                << "                }" << std::endl;
    }

    rstrOut << "              }\n"
            << "            ]\n"
            << "          }\n"
            << "  Shape {\n"
            << "    appearance DEF COAP Appearance {\n"
            << "      material Material {diffuseColor "
            << textr << " "
            << textg << " "
            << textb << "}} # text color\n"
            << "    geometry   DEF MyText Text {\n"
            << "      string \""
            << _pclVRMLInfo->_clAnnotation
            << "\"\n"
            << "      fontStyle DEF COFS FontStyle {\n"
            << "        family [ \"Verdana\", \"Arial\", \"Helvetica\" ]\n"
            << "        size         0.08                     # text size\n"
            << "      }\n"
            << "    }\n"
            << "  }\n"
            << "       ]\n"
            << "     }\n"
            << "   ]\n"
            << " }\n"
            << "      ]\n"
            << "    }\n"
            << ""
            << "ROUTE User.position_changed TO UserPos.set_translation\n"
            << "ROUTE User.orientation_changed TO UserPos.set_rotation" << std::endl;
}

void MeshVRML::WriteVRMLViewpoints(std::ostream &rstrOut) const
{
    // write viewpoints
    //
    rstrOut.precision(3);
    rstrOut.setf(std::ios::fixed | std::ios::showpoint);
    Base::BoundBox3f clBB = _rclMesh.GetBoundBox();
    Base::Vector3f   clCenter = clBB.CalcCenter();
    for (std::vector<VRMLViewpointData>::iterator it = _pclVRMLInfo->_clViewpoints.begin(); it != _pclVRMLInfo->_clViewpoints.end(); ++it)
    {
        // build up the observer's coordinate system
        Base::Vector3f u,v,w;
        v = it->clVRefUp;
        w = it->clVRefPln;
        u = v % w;
        u.Normalize();
        v.Normalize();
        w.Normalize();

        // calc the view axis in world coordinates
        //
        Base::Vector3f p_uvw, p_xyz;
        p_uvw   = it->clPRefPt;
        p_xyz.x = u.x*p_uvw.x+v.x*p_uvw.y+w.x*p_uvw.z;
        p_xyz.y = u.y*p_uvw.x+v.y*p_uvw.y+w.y*p_uvw.z;
        p_xyz.z = u.z*p_uvw.x+v.z*p_uvw.y+w.z*p_uvw.z;
        p_xyz   = p_xyz + it->clVRefPt;

        float t = (clCenter - p_xyz)*w;
        Base::Vector3f a = p_xyz + t*w;

        Vector at, pos, up, rot_axis;
        double rot_angle;

        float fDistance = (float)(it->dVmax - it->dVmin);
        Base::Vector3f p = a + fDistance * w;
        pos.x = p.x;   pos.y = p.y;   pos.z = p.z;
        at.x  = a.x;   at.y  = a.y;   at.z  = a.z;
        up.x  = v.x;   up.y  = v.y;   up.z  = v.z;

        Convert_Camera_Model(&pos, &at, &up, &rot_axis, &rot_angle);

        rstrOut << "Viewpoint\n{\n"
                << "  jump         TRUE\n"
                << "  orientation   "
                << rot_axis.x << " "
                << rot_axis.y << " "
                << rot_axis.z << " "
                << rot_angle << "\n";
        rstrOut << "  description  \"" << it->clName << "\"\n";
        rstrOut << "  position     "
                << pos.x << " "
                << pos.y << " "
                << pos.z << "\n}" << std::endl;
    }
}

bool MeshVRML::Save (std::ostream &rstrOut, const App::Material &rclMat) const
{
    std::vector<App::Color> aclDummy;
/*
for (int i = 0; i < _rclMesh.CountPoints(); i++)
{
unsigned short r =  (rand() * 255) / float(RAND_MAX);
unsigned short g =  (rand() * 255) / float(RAND_MAX);
unsigned short b =  (rand() * 255) / float(RAND_MAX);

  aclDummy.push_back(App::Color(r, g, b));
}
*/
    return Save(rstrOut, aclDummy, rclMat, false);
}

