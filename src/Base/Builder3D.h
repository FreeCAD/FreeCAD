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

class BaseExport ColorRGBA : public ColorRGB
{
public:
    ColorRGBA() : _alpha {1.0F} {
    }
    explicit ColorRGBA(float red, float green, float blue, float alpha)
        : ColorRGB(red, green, blue)
        , _alpha {valueInRange(alpha)} {
    }
    ~ColorRGBA() = default;
    float alpha() const {
        return _alpha;
    }

private:
    float _alpha;
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

    Style style = Style::Filled;
    unsigned short pointSize = 2;
    unsigned short lineWidth = 2;
    unsigned short linePattern = 0xffff;
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
class BaseExport Builder3D
{
public:
    Builder3D();
    virtual ~Builder3D();

    /** @name point set handling */
    //@{
    /// starts a point set
    void startPoints(short pointSize=2, const ColorRGB& color = ColorRGB{1.0F, 0.0F, 0.0F});
    /// add a point to a point set
    void addPoint(const Vector3f& point);
    /// ends the points set operation
    void endPoints();
    /// add a single point (without startPoints() & endPoints() )
    void addSinglePoint(const Base::Vector3f& point, DrawStyle drawStyle, const ColorRGB& color = ColorRGB{1.0F, 1.0F, 1.0F});
    //@}

    /** @name line/direction handling */
    //@{
    /// add a line
    void addSingleLine(const Base::Line3f& line, DrawStyle drawStyle, const ColorRGB& color = ColorRGB{1.0F, 1.0F, 1.0F});
    /// add an arrow.
    void addSingleArrow(const Base::Line3f& line, DrawStyle drawStyle, const ColorRGB& color = ColorRGB{1.0F, 1.0F, 1.0F});
    //@}

    /** @name triangle handling */
    //@{
    /// add a triangle
    void addSingleTriangle(const Triangle& triangle, DrawStyle drawStyle, const ColorRGB& color = ColorRGB{1.0F, 1.0F, 1.0F});
    //@}

    /** @name Transformation */
    //@{
    /// adds a transformation
    void addTransformation(const Base::Matrix4D&);
    void addTransformation(const Base::Placement&);
    //@}

    /** @name text handling */
    //@{
    /// add a text
    void addText(const Base::Vector3f& point, const char * text, const ColorRGB& color = ColorRGB{1.0F, 1.0F, 1.0F});
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

    bool bStartEndOpen;
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
    InventorBuilder(std::ostream& str);
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
     * \param color_r - red color
     * \param color_g - green color
     * \param color_b - blue color
     */
    void addBaseColor(float color_r,float color_g,float color_b);
    /*!
     * \brief Sets a material node. The colors are in the range [0, 1].
     * \param color_r - red color
     * \param color_g - green color
     * \param color_b - blue color
     * \param color_a - transparency
     */
    void addMaterial(float color_r,float color_g,float color_b,float color_a=0);
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
     * \param color_r - red color
     * \param color_g - green color
     * \param color_b - blue color
     */
    void addColor(float color_r,float color_g,float color_b);
    /*!
     * \brief Sets a material binding node.
     * \param binding - binding of the material. Allowed values are:
     * OVERALL, PER_PART, PER_PART_INDEXED, PER_FACE, PER_FACE_INDEXED, PER_VERTEX,
     * PER_VERTEX_INDEXED and DEFAULT.
     */
    void addMaterialBinding(const char* binding = "OVERALL");
    /*!
     * \brief Sets a draw style node.
     * \param pointSize - the point size
     * \param lineWidth - the line width
     * \param linePattern - the line pattern
     * \param style - the draw style
     */
    void addDrawStyle(short pointSize, short lineWidth,
        unsigned short linePattern = 0xffff, const char* style="FILLED");
    /*!
     * \brief Sets a shape hints node.
     * \param crease - the crease angle in radians
     */
    void addShapeHints(float crease=0.0f);
    /*!
     * \brief Sets a polygon offset node.
     * \param factor - Offset multiplication factor.
     * \param units - Offset translation multiplication factor.
     * \param styles - Can be FILLED, LINES or POINTS.
     * \param on - Whether the offset is on or off.
     */
    void addPolygonOffset(float factor=1.0f, float units=1.0f, const char* styles="FILLED", bool on=true);
    //@}

    /** @name Add coordinates */
    //@{
    /// add a single point
    void addPoint(float x, float y, float z);
    /// add a single point
    void addPoint(const Vector3f &vec);
    /// add a list of points
    void addPoints(const std::vector<Vector3f> &vec);
    //@}

    /** @name Point set handling */
    //@{
    /// starts a point set
    void beginPoints();
    /// ends the points set operation
    void endPoints();
    /// add an SoPointSet node
    void addPointSet();
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
    /// add a line defined by 2 Vector3D
    void addSingleLine(const Vector3f& pt1, const Vector3f& pt2, short lineSize=2,
                       float color_r=1.0,float color_g=1.0,float color_b=1.0, unsigned short linePattern = 0xffff);
    /// add a arrow (directed line) by 2 Vector3D. The arrow shows in direction of point 2.
    void addSingleArrow(const Vector3f& pt1, const Vector3f& pt2, short lineSize=2,
                        float color_r=1.0,float color_g=1.0,float color_b=1.0, unsigned short linePattern = 0xffff);
    /// add a line defined by a list of points whereat always a pair (i.e. a point and the following point) builds a line.
    void addLineSet(const std::vector<Vector3f>& points, short lineSize=2,
                    float color_r=1.0,float color_g=1.0,float color_b=1.0, unsigned short linePattern = 0xffff);
    /// add an SoLineSet node
    void addLineSet();
    //@}

    /** @name Triangle handling */
    //@{
    /// add a (filled) triangle defined by 3 vectors
    void addSingleTriangle(const Vector3f& pt0, const Vector3f& pt1, const Vector3f& pt2, bool filled = true, short lineSize=2,
                           float color_r=1.0,float color_g=1.0,float color_b=1.0);
    void addSinglePlane(const Vector3f& base, const Vector3f& eX, const Vector3f& eY, float length, float width, bool filled = true,
                        short lineSize=2, float color_r=1.0,float color_g=1.0,float color_b=1.0);
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
    void addBoundingBox(const Vector3f& pt1, const Vector3f& pt2, short lineWidth=2,
                        float color_r=1.0,float color_g=1.0,float color_b=1.0);
    //@}

    /** @name Transformation */
    //@{
    /// adds a transformation
    void addTransformation(const Matrix4D&);
    void addTransformation(const Vector3f& translation, const Vector3f& rotationaxis, float fAngle);
    //@}

    /** @name Text handling */
    //@{
    /// add a text
    void addText(float pos_x, float pos_y , float pos_z,const char * text,
                 float color_r=1.0,float color_g=1.0,float color_b=1.0);
    /// add a text
    void addText(const Vector3f &vec,const char * text, float color_r=1.0,float color_g=1.0,float color_b=1.0);
    //@}

private:
    InventorBuilder (const InventorBuilder&);
    void operator = (const InventorBuilder&);

private:
    std::ostream& result;
    int indent;
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
