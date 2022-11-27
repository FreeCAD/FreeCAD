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


#ifndef BASE_BUILDER3D_H
#define BASE_BUILDER3D_H

// Std. configurations

#include <sstream>
#include <vector>
#include <Base/Tools3D.h>
#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif

namespace Base
{
class Matrix4D;

class BaseExport ColorRGB
{
public:
    ColorRGB();
    explicit ColorRGB(float red, float green, float blue);
    ~ColorRGB() = default;

    float red() const {
        return Rgb.red;
    }

    float green() const {
        return Rgb.green;
    }

    float blue() const {
        return Rgb.blue;
    }

protected:
    /*! Returns the clamped value in range [-1, +1] */
    static float valueInRange(float value);
    struct {
        float red;
        float green;
        float blue;
    } Rgb;
};

class BaseExport DrawStyle
{
public:
    enum class Style {
        Filled,
        Lines,
        Points,
        Invisible
    };

    const char* styleAsString() const;
    std::string patternAsString() const;
    Style style = Style::Filled;
    unsigned short pointSize = 2;
    unsigned short lineWidth = 2;
    unsigned short linePattern = 0xffff;
};

class BaseExport PolygonOffset
{
public:
    enum class Style {
        Filled,
        Lines,
        Points
    };

    const char* styleAsString() const;
    float factor = 1.0F;
    float units = 1.0F;
    Style style = Style::Filled;
    bool on = true;
};

class BaseExport Triangle
{
public:
    explicit Triangle(const Base::Vector3f& pt1,
                      const Base::Vector3f& pt2,
                      const Base::Vector3f& pt3)
        : pt1(pt1), pt2(pt2), pt3(pt3)
    {
    }

    const Base::Vector3f& getPoint1() const {
        return pt1;
    }

    const Base::Vector3f& getPoint2() const {
        return pt2;
    }

    const Base::Vector3f& getPoint3() const {
        return pt3;
    }

private:
    Base::Vector3f pt1;
    Base::Vector3f pt2;
    Base::Vector3f pt3;
};

/**
 * This class does basically the same as Builder3D except that it writes the data
 * directly into a given stream without buffering the output data in a string stream.
 * Compared to file streams, string streams are quite slow when writing data with more
 * than a few hundred lines. Due to performance reasons the user should use a file
 * stream in this case.
 * @author Werner Mayer
 */
class BaseExport InventorBuilder
{
public:
    /*!
     * \brief Construction of an InventorBuilder instance.
     * This automatically opens a separator node.
     * \param str - stream to write the content into
     */
    explicit InventorBuilder(std::ostream& str);
    /*!
     * \brief Destruction of an InventorBuilder instance
     */
    virtual ~InventorBuilder();
    /*!
     * \brief If needed closes the first opened separator node.
     * This method must not be used more than one time for an instance.
     */
    void close();

    /*!
     * \brief Sets a separator node.
     */
    void beginSeparator();
    /*!
     * \brief Closes the last added separator node.
     */
    void endSeparator();
    /*!
     * \brief Sets an info node.
     * \param str - text set to the info node
     */
    void addInfo(const char* str);
    /*!
     * \brief Sets a label node.
     * \param str - text set to the label node
     */
    void addLabel(const char* str);
    /** @name Appearance handling */
    //@{
    /*!
     * \brief Sets a base color node. The colors are in the range [0, 1].
     */
    void addBaseColor(const ColorRGB& rgb);
    /*!
     * \brief Sets a material node. The colors are in the range [0, 1].
     */
    void addMaterial(const ColorRGB& rgb, float transparency=0);
    /*!
     * \brief Starts a material node. The node must be closed with \ref endMaterial
     * and the colors must be added with \ref addColor().
     */
    void beginMaterial();
    /*!
     * \brief Closes a material node.
     */
    void endMaterial();
    /*!
     * \brief Adds a color to a material node. The colors are in the range [0, 1].
     */
    void addColor(const ColorRGB& rgb);
    /*!
     * \brief Sets a material binding node.
     * \param binding - binding of the material. Allowed values are:
     * OVERALL, PER_PART, PER_PART_INDEXED, PER_FACE, PER_FACE_INDEXED, PER_VERTEX,
     * PER_VERTEX_INDEXED and DEFAULT.
     */
    void addMaterialBinding(const char* binding = "OVERALL");
    /*!
     * \brief Sets a draw style node.
     */
    void addDrawStyle(DrawStyle drawStyle);
    /*!
     * \brief Sets a shape hints node.
     * \param crease - the crease angle in radians
     */
    void addShapeHints(float creaseAngle=0.0f);
    /*!
     * \brief Sets a polygon offset node.
     */
    void addPolygonOffset(PolygonOffset polygonOffset);
    //@}

    /** @name Add coordinates */
    //@{
    /// add a single point
    void addPoint(const Vector3f& pnt);
    /// add a list of points
    void addPoints(const std::vector<Vector3f>& points);
    //@}

    /** @name Point set handling */
    //@{
    /// starts a point set
    void beginPoints();
    /// ends the points set operation
    void endPoints();
    /// add an SoPointSet node
    void addPointSet();
    /// add a single point (without startPoints() & endPoints() )
    void addSinglePoint(const Base::Vector3f& point, DrawStyle drawStyle, const ColorRGB& color = ColorRGB{1.0F, 1.0F, 1.0F});
    //@}

    /** @name Normal handling */
    //@{
    /// starts a point set
    void beginNormal();
    /// ends the points set operation
    void endNormal();
    /// add normal binding node
    void addNormalBinding(const char*);
    //@}

    /** @name Line/Direction handling */
    //@{
    /// add a line
    void addSingleLine(const Base::Line3f& line, DrawStyle drawStyle, const ColorRGB& rgb = ColorRGB{1.0F, 1.0F, 1.0F});
    /// add a arrow
    void addSingleArrow(const Base::Line3f& line, DrawStyle drawStyle, const ColorRGB& rgb = ColorRGB{1.0F, 1.0F, 1.0F});
    /// add a line defined by a list of points whereat always a pair (i.e. a point and the following point) builds a line.
    void addLineSet(const std::vector<Vector3f>& points, DrawStyle drawStyle, const ColorRGB& rgb = ColorRGB{1.0F, 1.0F, 1.0F});
    /// add an SoLineSet node
    void addLineSet();
    //@}

    /** @name Triangle handling */
    //@{
    /// add a (filled) triangle defined by 3 vectors
    void addSingleTriangle(const Triangle& triangle, DrawStyle drawStyle, const ColorRGB& rgb = ColorRGB{1.0F, 1.0F, 1.0F});
    void addSinglePlane(const Vector3f& base, const Vector3f& eX, const Vector3f& eY, float length, float width, DrawStyle drawStyle,
                        const ColorRGB& rgb = ColorRGB{1.0F, 1.0F, 1.0F});
    void addIndexedFaceSet(const std::vector<int>& indices);
    void addFaceSet(const std::vector<int>& vertices);
    //@}

    /** @name Surface handling */
    //@{
    void addNurbsSurface(const std::vector<Base::Vector3f>& controlPoints,
        int numUControlPoints, int numVControlPoints,
        const std::vector<float>& uKnots, const std::vector<float>& vKnots);
    void addCylinder(float radius, float height);
    void addSphere(float radius);
    //@}

    /** @name Bounding Box handling */
    //@{
    void addBoundingBox(const Vector3f& pt1, const Vector3f& pt2, DrawStyle drawStyle,
                        const ColorRGB& rgb = ColorRGB{1.0F, 1.0F, 1.0F});
    //@}

    /** @name Transformation */
    //@{
    /// adds a transformation
    void addTransformation(const Matrix4D&);
    void addTransformation(const Base::Placement&);
    //@}

    /** @name Text handling */
    //@{
    /// add a text
    void addText(const Vector3f &vec,const char * text, const ColorRGB& rgb = ColorRGB{1.0F, 1.0F, 1.0F});
    //@}

private:
    InventorBuilder (const InventorBuilder&);
    void operator = (const InventorBuilder&);

private:
    std::ostream& result;
    int indent;
};

/** A Builder class for 3D representations on App level
 * On the application level nothing is known of the visual representation of data.
 * Nevertheless it's often needed to see some 3D information, e.g. points, directions,
 * when you program or debug an algorithm. Builder3D was made for this specific purpose.
 * This class allows you to easily build up a 3D representation of some mathematical and
 * algorithm internals. You can save this representation to a file and view it in an
 * Inventor viewer, or send it to the log. In the case of using the log and a debug
 * FreeCAD the representation will be loaded into the active viewer.
 *  \par
 * The workflow goes as follows: Create the a Builder3D object and call the methods
 * to insert the graphical elements. After that call either saveToLog() or saveToFile().
 *  \par
 * Usage:
 *  \code
  Base::Builder3D log3D;
  for ( unsigned long i=0; i<pMesh->CountPoints(); i++ )
  {
    log3D.addSinglePoint(pMesh->GetPoint(i));
    log3D.addText(pMesh->GetPoint(i),"Point");
    ...
  }
  log3D.saveToLog();
 *  \endcode
 * \see Base::ConsoleSingleton
 */
class BaseExport Builder3D : public InventorBuilder
{
public:
    Builder3D();
    virtual ~Builder3D();

    /** @name point set handling */
    //@{
    //@}

    /// clear the string buffer
    void clear();

    /** @name write the result */
    //@{
    /// sends the result to the log and gui
    void saveToLog();
    /// save the result to a file (*.iv)
    void saveToFile(const char* FileName);
    //@}

private:
    /// the result string
    std::stringstream result;
};

/**
 * Loads an OpenInventor file.
 * @author Werner Mayer
 */
class BaseExport InventorLoader {
public:
    struct Face {
        Face(int32_t p1, int32_t p2, int32_t p3)
            : p1(p1), p2(p2), p3(p3) {}
        int32_t p1, p2, p3;
    };

    explicit InventorLoader(std::istream &inp) : inp(inp) {
    }

    /// Start the read process. Returns true if successful and false otherwise.
    /// The obtained data can be accessed with the appropriate getter functions.
    bool read();

    /// Checks if the loaded data are valid
    bool isValid() const;

    /// Returns true if the data come from a non-indexed node as SoFaceSet.
    /// This means that the read points contain duplicates.
    bool isNonIndexed() const {
        return isnonindexed;
    }

    /// Return the vectors of an SoNormal node
    const std::vector<Vector3f>& getVector() const {
        return vector;
    }

    /// Return the points of an SoCoordinate3 node
    const std::vector<Vector3f>& getPoints() const {
        return points;
    }

    /// Return the faces of an SoIndexedFaceSet node
    const std::vector<Face>& getFaces() const {
        return faces;
    }

private:
    void readNormals();
    void readCoords();
    void readIndexedFaceSet();
    void readFaceSet();
    template<typename T>
    std::vector<T> readData(const char*) const;
    std::vector<Vector3f> convert(const std::vector<float>&) const;
    std::vector<Face> convert(const std::vector<int32_t>&) const;
    std::vector<Face> convert(const std::vector<std::vector<int32_t>>&) const;
    static std::vector<std::vector<int32_t>> split(const std::vector<int32_t>&);

private:
    bool isnonindexed = false;
    std::vector<Vector3f> vector;
    std::vector<Vector3f> points;
    std::vector<Face> faces;
    std::istream &inp;
};

/*!
 * Expects a string of the form "(x,y,z)" and creates a vector from it.
 * If it fails then a std::exception is thrown.
 * Supported type names are float or double
 */
BaseExport Base::Vector3f to_vector(std::string);

} //namespace Base

#endif // BASE_BUILDER3D_H
