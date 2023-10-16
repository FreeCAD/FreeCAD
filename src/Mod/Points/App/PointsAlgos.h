/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef _PointsAlgos_h_
#define _PointsAlgos_h_

#include <Eigen/Core>

#include "Points.h"
#include "Properties.h"


namespace Points
{

/** The Points algorithms container class
 */
class PointsExport PointsAlgos
{
public:
    /** Load a point cloud
     */
    static void Load(PointKernel&, const char* FileName);
    /** Load a point cloud
     */
    static void LoadAscii(PointKernel&, const char* FileName);
};

class Reader
{
public:
    Reader();
    virtual ~Reader();
    virtual void read(const std::string& filename) = 0;

    void clear();
    const PointKernel& getPoints() const;
    bool hasProperties() const;
    const std::vector<float>& getIntensities() const;
    bool hasIntensities() const;
    const std::vector<App::Color>& getColors() const;
    bool hasColors() const;
    const std::vector<Base::Vector3f>& getNormals() const;
    bool hasNormals() const;
    bool isStructured() const;
    int getWidth() const;
    int getHeight() const;

protected:
    PointKernel points;
    std::vector<float> intensity;
    std::vector<App::Color> colors;
    std::vector<Base::Vector3f> normals;
    int width, height;
};

class AscReader: public Reader
{
public:
    AscReader();
    void read(const std::string& filename) override;
};

class PlyReader: public Reader
{
public:
    PlyReader();
    void read(const std::string& filename) override;

private:
    std::size_t readHeader(std::istream&,
                           std::string& format,
                           std::size_t& offset,
                           std::vector<std::string>& fields,
                           std::vector<std::string>& types,
                           std::vector<int>& sizes);
    void readAscii(std::istream&, std::size_t offset, Eigen::MatrixXd& data);
    void readBinary(bool swapByteOrder,
                    std::istream&,
                    std::size_t offset,
                    const std::vector<std::string>& types,
                    const std::vector<int>& sizes,
                    Eigen::MatrixXd& data);
};

class PcdReader: public Reader
{
public:
    PcdReader();
    void read(const std::string& filename) override;

private:
    std::size_t readHeader(std::istream&,
                           std::string& format,
                           std::vector<std::string>& fields,
                           std::vector<std::string>& types,
                           std::vector<int>& sizes);
    void readAscii(std::istream&, Eigen::MatrixXd& data);
    void readBinary(bool transpose,
                    std::istream&,
                    const std::vector<std::string>& types,
                    const std::vector<int>& sizes,
                    Eigen::MatrixXd& data);
};

class E57Reader: public Reader
{
public:
    E57Reader(bool Color, bool State, double Distance);
    void read(const std::string& filename) override;

protected:
    bool useColor, checkState;
    double minDistance;
};

class Writer
{
public:
    explicit Writer(const PointKernel&);
    virtual ~Writer();
    virtual void write(const std::string& filename) = 0;

    void setIntensities(const std::vector<float>&);
    void setColors(const std::vector<App::Color>&);
    void setNormals(const std::vector<Base::Vector3f>&);
    void setWidth(int);
    void setHeight(int);
    void setPlacement(const Base::Placement&);

protected:
    const PointKernel& points;
    std::vector<float> intensity;
    std::vector<App::Color> colors;
    std::vector<Base::Vector3f> normals;
    int width, height;
    Base::Placement placement;
};

class AscWriter: public Writer
{
public:
    explicit AscWriter(const PointKernel&);
    void write(const std::string& filename) override;
};

class PlyWriter: public Writer
{
public:
    explicit PlyWriter(const PointKernel&);
    void write(const std::string& filename) override;
};

class PcdWriter: public Writer
{
public:
    explicit PcdWriter(const PointKernel&);
    void write(const std::string& filename) override;
};

}  // namespace Points


#endif
