// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

// Std. configurations

#include <sstream>
#include <vector>
#include <cstdint>
#include <Base/Tools3D.h>
#include <FCGlobal.h>

#include "Placement.h"

namespace Base
{
class Matrix4D;

class BaseExport ColorRGB
{
public:
    ColorRGB();
    ColorRGB(const ColorRGB&) = default;
    ColorRGB(ColorRGB&&) = default;
    explicit ColorRGB(float red, float green, float blue);
    ~ColorRGB() = default;
    ColorRGB& operator=(const ColorRGB&) = default;
    ColorRGB& operator=(ColorRGB&&) = default;

    float red() const
    {
        return Rgb.red;
    }

    float green() const
    {
        return Rgb.green;
    }

    float blue() const
    {
        return Rgb.blue;
    }

private:
    /*! Returns the clamped value in range [-1, +1] */
    static float valueInRange(float value);
    struct
    {
        float red;
        float green;
        float blue;
    } Rgb;
};

class BaseExport DrawStyle
{
public:
    enum class Style
    {
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

class BaseExport VertexOrdering
{
public:
    enum class Ordering
    {
        UnknownOrdering,
        Clockwise,
        CounterClockwise
    };

    const char* toString() const;

    Ordering ordering = Ordering::UnknownOrdering;
};

class BaseExport ShapeType
{
public:
    enum class Type
    {
        UnknownShapeType,
        Convex
    };

    const char* toString() const;

    Type type = Type::UnknownShapeType;
};

class BaseExport BindingElement
{
public:
    enum class Binding
    {
        Overall = 2,
        PerPart = 3,
        PerPartIndexed = 4,
        PerFace = 5,
        PerFaceIndexed = 6,
        PerVertex = 7,
        PerVertexIndexed = 8,
        Default = Overall,
        None = Overall
    };

    const char* bindingAsString() const;
    Binding value = Binding::Overall;
};

class BaseExport PolygonOffset
{
public:
    enum class Style
    {
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
    explicit Triangle(const Base::Vector3f& pt1, const Base::Vector3f& pt2, const Base::Vector3f& pt3)
        : pt1(pt1)
        , pt2(pt2)
        , pt3(pt3)
    {}

    const Base::Vector3f& getPoint1() const
    {
        return pt1;
    }

    const Base::Vector3f& getPoint2() const
    {
        return pt2;
    }

    const Base::Vector3f& getPoint3() const
    {
        return pt3;
    }

private:
    Base::Vector3f pt1;
    Base::Vector3f pt2;
    Base::Vector3f pt3;
};

class Indentation
{
    int spaces = 0;

public:
    void increaseIndent()
    {
        spaces += 2;
    }
    void decreaseIndent()
    {
        spaces -= 2;
    }
    int count() const
    {
        return spaces;
    }
    friend std::ostream& operator<<(std::ostream& os, Indentation ind)
    {
        for (int i = 0; i < ind.count(); i++) {
            os << " ";
        }
        return os;
    }
};

class BaseExport InventorOutput
{
public:
    explicit InventorOutput(std::ostream& result, Indentation& indent);
    std::ostream& stream();
    std::ostream& write();
    std::ostream& write(const char*);
    std::ostream& write(const std::string&);
    std::ostream& writeLine();
    std::ostream& writeLine(const char*);
    std::ostream& writeLine(const std::string&);
    void increaseIndent();
    void decreaseIndent();

private:
    std::ostream& result;
    Indentation& indent;
};

class BaseExport NodeItem
{
public:
    virtual ~NodeItem() = default;
    virtual void write(InventorOutput& out) const = 0;

protected:
    NodeItem() = default;
    NodeItem(const NodeItem&) = default;
    NodeItem(NodeItem&&) = default;
    NodeItem& operator=(const NodeItem&) = default;
    NodeItem& operator=(NodeItem&&) = default;
};

/*!
 * \brief The LabelItem class supports the SoLabel node.
 */
class BaseExport LabelItem: public NodeItem
{
public:
    explicit LabelItem(std::string text);
    void write(InventorOutput& out) const override;

private:
    std::string text;
};

/*!
 * \brief The InfoItem class supports the SoInfo node.
 */
class BaseExport InfoItem: public NodeItem
{
public:
    explicit InfoItem(std::string text);
    void write(InventorOutput& out) const override;

private:
    std::string text;
};

/*!
 * \brief The BaseColorItem class supports the SoBaseColor node.
 */
class BaseExport BaseColorItem: public NodeItem
{
public:
    explicit BaseColorItem(const ColorRGB& rgb);
    void write(InventorOutput& out) const override;

private:
    ColorRGB rgb;
};

class BaseExport PointItem: public NodeItem
{
public:
    explicit PointItem(
        const Base::Vector3f& point,
        DrawStyle drawStyle,
        const ColorRGB& rgb = ColorRGB {1.0F, 1.0F, 1.0F}
    );
    void write(InventorOutput& out) const override;

private:
    Base::Vector3f point;
    DrawStyle drawStyle;
    ColorRGB rgb;
};

class BaseExport LineItem: public NodeItem
{
public:
    explicit LineItem(
        const Base::Line3f& line,
        DrawStyle drawStyle,
        const ColorRGB& rgb = ColorRGB {1.0F, 1.0F, 1.0F}
    );
    void write(InventorOutput& out) const override;

private:
    Base::Line3f line;
    DrawStyle drawStyle;
    ColorRGB rgb;
};

class BaseExport MultiLineItem: public NodeItem
{
public:
    /// add a line defined by a list of points whereat always a pair (i.e. a point and the following
    /// point) builds a line.
    explicit MultiLineItem(
        std::vector<Vector3f> points,
        DrawStyle drawStyle,
        const ColorRGB& rgb = ColorRGB {1.0F, 1.0F, 1.0F}
    );
    void write(InventorOutput& out) const override;

private:
    std::vector<Vector3f> points;
    DrawStyle drawStyle;
    ColorRGB rgb;
};

class BaseExport ArrowItem: public NodeItem
{
public:
    explicit ArrowItem(
        const Base::Line3f& line,
        DrawStyle drawStyle,
        const ColorRGB& rgb = ColorRGB {1.0F, 1.0F, 1.0F}
    );
    void write(InventorOutput& out) const override;

private:
    Base::Line3f line;
    DrawStyle drawStyle;
    ColorRGB rgb;
};

class BaseExport BoundingBoxItem: public NodeItem
{
public:
    explicit BoundingBoxItem(
        const Vector3f& pt1,
        const Vector3f& pt2,
        DrawStyle drawStyle,
        const ColorRGB& rgb = ColorRGB {1.0F, 1.0F, 1.0F}
    );
    void write(InventorOutput& out) const override;

private:
    Vector3f pt1;
    Vector3f pt2;
    DrawStyle drawStyle;
    ColorRGB rgb;
};

/*!
 * \brief The MaterialItem class supports the SoMaterial node.
 */
class BaseExport MaterialItem: public NodeItem
{
public:
    void setAmbientColor(const std::vector<ColorRGB>& rgb);
    void setDiffuseColor(const std::vector<ColorRGB>& rgb);
    void setSpecularColor(const std::vector<ColorRGB>& rgb);
    void setEmissiveColor(const std::vector<ColorRGB>& rgb);
    void setShininess(const std::vector<float>& value);
    void setTransparency(const std::vector<float>& value);
    void write(InventorOutput& out) const override;

private:
    void beginMaterial(InventorOutput& out) const;
    void endMaterial(InventorOutput& out) const;
    void writeAmbientColor(InventorOutput& out) const;
    void writeDiffuseColor(InventorOutput& out) const;
    void writeSpecularColor(InventorOutput& out) const;
    void writeEmissiveColor(InventorOutput& out) const;
    void writeShininess(InventorOutput& out) const;
    void writeTransparency(InventorOutput& out) const;

private:
    std::vector<ColorRGB> ambientColor;
    std::vector<ColorRGB> diffuseColor;
    std::vector<ColorRGB> specularColor;
    std::vector<ColorRGB> emissiveColor;
    std::vector<float> shininess;
    std::vector<float> transparency;
};

/*!
 * \brief The MaterialBindingItem class supports the SoMaterialBinding node.
 */
class BaseExport MaterialBindingItem: public NodeItem
{
public:
    MaterialBindingItem() = default;
    explicit MaterialBindingItem(BindingElement::Binding);
    void setValue(BindingElement::Binding bind);
    void write(InventorOutput& out) const override;

private:
    BindingElement value;
};

/*!
 * \brief The DrawStyleItem class supports the SoDrawStyle node.
 */
class BaseExport DrawStyleItem: public NodeItem
{
public:
    DrawStyleItem() = default;
    explicit DrawStyleItem(DrawStyle);
    void setValue(DrawStyle value);
    void write(InventorOutput& out) const override;

private:
    DrawStyle style;
};

/*!
 * \brief The ShapeHintsItem class supports the SoShapeHints node.
 */
class BaseExport ShapeHintsItem: public NodeItem
{
public:
    explicit ShapeHintsItem(float creaseAngle = 0.0F);
    void setVertexOrdering(VertexOrdering::Ordering);
    void setShapeType(ShapeType::Type);
    void write(InventorOutput& out) const override;

private:
    float creaseAngle = 0.0F;
    VertexOrdering vertexOrdering;
    ShapeType shapeType;
};

/*!
 * \brief The PolygonOffsetItem class supports the SoPolygonOffset node.
 */
class BaseExport PolygonOffsetItem: public NodeItem
{
public:
    void setValue(PolygonOffset value);
    void write(InventorOutput& out) const override;

private:
    PolygonOffset offset;
};

/*!
 * \brief The Coordinate3Item class supports the SoCoordinate3 node.
 */
class BaseExport Coordinate3Item: public NodeItem
{
public:
    explicit Coordinate3Item(std::vector<Vector3f> points);
    void write(InventorOutput& out) const override;

private:
    void beginPoint(InventorOutput& out) const;
    void endPoint(InventorOutput& out) const;
    std::vector<Vector3f> points;
};

/*!
 * \brief The PointSetItem class supports the SoPointSet node.
 */
class BaseExport PointSetItem: public NodeItem
{
public:
    void write(InventorOutput& out) const override;
};

/*!
 * \brief The LineSetItem class supports the SoLineSet node.
 */
class BaseExport LineSetItem: public NodeItem
{
public:
    void write(InventorOutput& out) const override;
};

/*!
 * \brief The FaceSetItem class supports the SoFaceSet node.
 */
class BaseExport FaceSetItem: public NodeItem
{
public:
    explicit FaceSetItem(std::vector<int>);
    void write(InventorOutput& out) const override;

private:
    std::vector<int> indices;
};

/*!
 * \brief The IndexedLineSetItem class supports the SoIndexedLineSet node.
 */
class BaseExport IndexedLineSetItem: public NodeItem
{
public:
    explicit IndexedLineSetItem(std::vector<int>);
    void write(InventorOutput& out) const override;

private:
    std::vector<int> indices;
};

/*!
 * \brief The IndexedFaceSetItem class supports the SoIndexedFaceSet node.
 */
class BaseExport IndexedFaceSetItem: public NodeItem
{
public:
    explicit IndexedFaceSetItem(std::vector<int>);
    void write(InventorOutput& out) const override;

private:
    std::vector<int> indices;
};

/*!
 * \brief The NormalItem class supports the SoNormal node.
 */
class BaseExport NormalItem: public NodeItem
{
public:
    explicit NormalItem(std::vector<Base::Vector3f> vec);
    void write(InventorOutput& out) const override;

private:
    void beginNormal(InventorOutput& out) const;
    void endNormal(InventorOutput& out) const;
    std::vector<Base::Vector3f> vector;
};

/*!
 * \brief The MaterialBindingItem class supports the SoMaterialBinding node.
 */
class BaseExport NormalBindingItem: public NodeItem
{
public:
    NormalBindingItem() = default;
    void setValue(BindingElement::Binding bind);
    void write(InventorOutput& out) const override;

private:
    BindingElement value;
};

/*!
 * \brief The CylinderItem class supports the SoCylinder node.
 */
class BaseExport CylinderItem: public NodeItem
{
public:
    void setRadius(float);
    void setHeight(float);
    void write(InventorOutput& out) const override;

private:
    float radius = 2.0F;
    float height = 10.0F;
};

/*!
 * \brief The ConeItem class supports the SoCone node.
 */
class BaseExport ConeItem: public NodeItem
{
public:
    void setBottomRadius(float);
    void setHeight(float);
    void write(InventorOutput& out) const override;

private:
    float bottomRadius = 2.0F;
    float height = 10.0F;
};

/*!
 * \brief The SphereItem class supports the SoSphere node.
 */
class BaseExport SphereItem: public NodeItem
{
public:
    void setRadius(float);
    void write(InventorOutput& out) const override;

private:
    float radius = 2.0F;
};

/*!
 * \brief The NurbsSurfaceItem class supports the SoNurbsSurface node.
 */
class BaseExport NurbsSurfaceItem: public NodeItem
{
public:
    void setControlPoints(int numU, int numV);
    void setKnotVector(const std::vector<float>&, const std::vector<float>&);
    void write(InventorOutput& out) const override;

private:
    int numUControlPoints = 0;
    int numVControlPoints = 0;
    std::vector<float> uKnotVector;
    std::vector<float> vKnotVector;
};

/*!
 * \brief The Text2Item class supports the SoText2 node.
 */
class BaseExport Text2Item: public NodeItem
{
public:
    explicit Text2Item(std::string);
    void write(InventorOutput& out) const override;

private:
    std::string string;
};

/*!
 * \brief The TransformItem class supports the SoTransform node.
 */
class BaseExport TransformItem: public NodeItem
{
public:
    explicit TransformItem(const Base::Placement&);
    explicit TransformItem(const Base::Matrix4D&);
    void write(InventorOutput& out) const override;

private:
    Base::Placement placement;
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
     * \param str - stream to write the content into
     */
    explicit InventorBuilder(std::ostream& str);
    /*!
     * \brief Adds the OpenInventor header.
     */
    void addHeader();
    /*!
     * \brief Destruction of an InventorBuilder instance
     */
    virtual ~InventorBuilder();
    /*!
     * \brief addNode
     * Writes the content of the added node to the output stream.
     */
    void addNode(const NodeItem&);
    /*!
     * \brief Sets a separator node.
     */
    void beginSeparator();
    /*!
     * \brief Closes the last added separator node.
     */
    void endSeparator();

private:
    void increaseIndent();
    void decreaseIndent();

public:
    InventorBuilder(const InventorBuilder&) = delete;
    InventorBuilder(InventorBuilder&&) = delete;
    void operator=(const InventorBuilder&) = delete;
    void operator=(InventorBuilder&&) = delete;

private:
    std::ostream& result;
    Indentation indent;
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
    ~Builder3D();

    /// clear the string buffer
    void clear();

    /** @name write the result */
    //@{
    /// sends the result to the log and gui
    void saveToLog();
    /// save the result to a file (*.iv)
    void saveToFile(const char* FileName);
    //@}

    /*!
     * \brief addNode
     * Writes the content of the added node to the output stream.
     */
    void addNode(const NodeItem&);
    /*!
     * \brief Sets a separator node.
     */
    void beginSeparator();
    /*!
     * \brief Closes the last added separator node.
     */
    void endSeparator();

    Builder3D(const Builder3D&) = delete;
    Builder3D(Builder3D&&) = delete;
    Builder3D& operator=(const Builder3D&) = delete;
    Builder3D& operator=(Builder3D&&) = delete;

private:
    /// the result string
    std::stringstream result;
    InventorBuilder builder;
};

/**
 * Loads an OpenInventor file.
 * @author Werner Mayer
 */
class BaseExport InventorLoader
{
public:
    struct Face
    {
        Face(int32_t p1, int32_t p2, int32_t p3)
            : p1(p1)
            , p2(p2)
            , p3(p3)
        {}
        int32_t p1, p2, p3;
    };

    explicit InventorLoader(std::istream& inp)
        : inp(inp)
    {}

    /// Start the read process. Returns true if successful and false otherwise.
    /// The obtained data can be accessed with the appropriate getter functions.
    bool read();

    /// Checks if the loaded data are valid
    bool isValid() const;

    /// Returns true if the data come from a non-indexed node as SoFaceSet.
    /// This means that the read points contain duplicates.
    bool isNonIndexed() const
    {
        return isnonindexed;
    }

    /// Return the vectors of an SoNormal node
    const std::vector<Vector3f>& getVector() const
    {
        return vector;
    }

    /// Return the points of an SoCoordinate3 node
    const std::vector<Vector3f>& getPoints() const
    {
        return points;
    }

    /// Return the faces of an SoIndexedFaceSet node
    const std::vector<Face>& getFaces() const
    {
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
    std::istream& inp;
};

/*!
 * Expects a string of the form "(x,y,z)" and creates a vector from it.
 * If it fails then a std::exception is thrown.
 * Supported type names are float or double
 */
BaseExport Base::Vector3f stringToVector(std::string);

/*!
 * Expects a string of the form "(x,y,z)" and creates a vector from it.
 * If it fails then a std::exception is thrown.
 * Supported type names are float or double
 */
BaseExport std::string vectorToString(Vector3f);

}  // namespace Base
