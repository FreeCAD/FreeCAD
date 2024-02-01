/***************************************************************************
 *   Copyright (c) 2015 Yorik van Havre (yorik@uncreated.net)              *
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

#ifndef IMPEXPDXF_H
#define IMPEXPDXF_H

#include <gp_Pnt.hxx>

#include <App/Document.h>
#include <TopoDS_Shape.hxx>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PartFeature.h>

#include "dxf.h"


class BRepAdaptor_Curve;

namespace Import
{
class ImportExport ImpExpDxfRead: public CDxfRead
{
public:
    ImpExpDxfRead(const std::string& filepath, App::Document* pcDoc);
    ImpExpDxfRead(const ImpExpDxfRead&) = delete;
    ImpExpDxfRead(ImpExpDxfRead&&) = delete;
    void operator=(const ImpExpDxfRead&) = delete;
    void operator=(ImpExpDxfRead&&) = delete;
    ~ImpExpDxfRead() override
    {
        Py_XDECREF(DraftModule);
    }

    bool ReadEntitiesSection() override;

    // CDxfRead's virtual functions
    bool OnReadBlock(const std::string& name, int flags) override;
    void OnReadLine(const Base::Vector3d& start, const Base::Vector3d& end, bool hidden) override;
    void OnReadPoint(const Base::Vector3d& start) override;
    void OnReadText(const Base::Vector3d& point,
                    double height,
                    const std::string& text,
                    double rotation) override;
    void OnReadArc(const Base::Vector3d& start,
                   const Base::Vector3d& end,
                   const Base::Vector3d& center,
                   bool dir,
                   bool hidden) override;
    void OnReadCircle(const Base::Vector3d& start,
                      const Base::Vector3d& center,
                      bool dir,
                      bool hidden) override;
    void OnReadEllipse(const Base::Vector3d& center,
                       double major_radius,
                       double minor_radius,
                       double rotation,
                       double start_angle,
                       double end_angle,
                       bool dir) override;
    void OnReadSpline(struct SplineData& sd) override;
    void OnReadInsert(const Base::Vector3d& point,
                      const Base::Vector3d& scale,
                      const std::string& name,
                      double rotation) override;
    // Expand a block reference; this should only happen when the collector draws to the document
    // rather than saving things The transform should include the OCS Orientation transform for the
    // insertion.
    void ExpandInsert(const std::string& name,
                      const Base::Matrix4D& transform,
                      const Base::Vector3d& point,
                      double rotation,
                      const Base::Vector3d& scale);
    void OnReadDimension(const Base::Vector3d& start,
                         const Base::Vector3d& end,
                         const Base::Vector3d& point,
                         double rotation) override;
    void OnReadPolyline(std::list<VertexInfo>& /*vertices*/, int flags) override;

    std::string Deformat(const char* text);  // Removes DXF formatting from texts

    std::string getOptionSource()
    {
        return m_optionSource;
    }
    void setOptionSource(const std::string& sourceName)
    {
        m_optionSource = sourceName;
    }
    void setOptions();

private:
    static gp_Pnt makePoint(const Base::Vector3d& point3d)
    {
        return {point3d.x, point3d.y, point3d.z};
    }
    void MoveToLayer(App::DocumentObject* object) const;
    // Combine all the shapes in the given shapes collection into a single shape, and AddObject that
    // to the drawing. unref's all the shapes in the collection, possibly freeing them.
    void CombineShapes(std::list<TopoDS_Shape>& shapes, const char* nameBase) const;
    PyObject* DraftModule = nullptr;

protected:
    PyObject* getDraftModule()
    {
        if (DraftModule == nullptr) {
            DraftModule = PyImport_ImportModule("Draft");
        }
        return DraftModule;
    }
    CDxfRead::Layer*
    MakeLayer(const std::string& name, ColorIndex_t color, std::string&& lineType) override;

    // Overrides for layer management so we can record the layer objects in the FreeCAD drawing that
    // are associated with the layers in the DXF.
    class Layer: public CDxfRead::Layer
    {
    public:
        Layer(const std::string& name,
              ColorIndex_t color,
              std::string&& lineType,
              PyObject* drawingLayer);
        Layer(const Layer&) = delete;
        Layer(Layer&&) = delete;
        void operator=(const Layer&) = delete;
        void operator=(Layer&&) = delete;
        ~Layer() override;
        PyObject* const DraftLayer;
        PyObject* const DraftLayerView;
    };

    using FeaturePythonBuilder =
        std::function<App::FeaturePython*(const Base::Matrix4D& transform)>;
    // Block management
    class Block
    {
    public:
        struct Insert
        {
            const Base::Vector3d Point;
            const Base::Vector3d Scale;
            const std::string Name;
            const double Rotation;

            // NOLINTNEXTLINE(readability/nolint)
            // NOLINTNEXTLINE(modernize-pass-by-value) Pass by value adds unwarranted complexity
            Insert(const std::string& Name,
                   const Base::Vector3d& Point,
                   double Rotation,
                   const Base::Vector3d& Scale)
                : Point(Point)
                , Scale(Scale)
                , Name(Name)
                , Rotation(Rotation)
            {}
        };
        // NOLINTNEXTLINE(readability/nolint)
        // NOLINTNEXTLINE(modernize-pass-by-value)  Pass by value adds unwarranted complexity
        Block(const std::string& name, int flags)
            : Name(name)
            , Flags(flags)
        {}
        const std::string Name;
        const int Flags;
        std::map<CDxfRead::CommonEntityAttributes, std::list<TopoDS_Shape>> Shapes;
        std::map<CDxfRead::CommonEntityAttributes, std::list<FeaturePythonBuilder>>
            FeatureBuildersList;
        std::map<CDxfRead::CommonEntityAttributes, std::list<Insert>> Inserts;
    };

private:
    std::map<std::string, Block> Blocks;
    App::Document* document;
    std::string m_optionSource;

protected:
    virtual void ApplyGuiStyles(Part::Feature* /*object*/) const
    {}
    virtual void ApplyGuiStyles(App::FeaturePython* /*object*/) const
    {}

    // Gathering of created entities
    class EntityCollector
    {
    public:
        explicit EntityCollector(ImpExpDxfRead& reader)
            : Reader(reader)
            , previousCollector(reader.Collector)
        {
            Reader.Collector = this;
        }
        EntityCollector(const EntityCollector&) = delete;
        EntityCollector(EntityCollector&&) = delete;
        void operator=(const EntityCollector&) = delete;
        void operator=(EntityCollector&&) = delete;

        virtual ~EntityCollector()
        {
            if (Reader.Collector == this) {
                Reader.Collector = previousCollector;
            }
        }

        // Called by OnReadXxxx functions to add Part objects
        virtual void AddObject(const TopoDS_Shape& shape, const char* nameBase) = 0;
        // Called by OnReadXxxx functions to add FeaturePython (draft) objects.
        // Because we can't readily copy Draft objects, this method instead takes a builder which,
        // when called, creates and returns the object.
        virtual void AddObject(FeaturePythonBuilder shapeBuilder) = 0;
        // Called by OnReadInsert to either remember in a nested block or expand the block into the
        // drawing
        virtual void AddInsert(const Base::Vector3d& point,
                               const Base::Vector3d& scale,
                               const std::string& name,
                               double rotation) = 0;

    protected:
        ImpExpDxfRead& Reader;

    private:
        EntityCollector* const previousCollector;
    };
    class DrawingEntityCollector: public EntityCollector
    {
        // This collector places all objects into the drawing
    public:
        explicit DrawingEntityCollector(ImpExpDxfRead& reader)
            : EntityCollector(reader)
        {}

        void AddObject(const TopoDS_Shape& shape, const char* nameBase) override;
        void AddObject(FeaturePythonBuilder shapeBuilder) override;
        void AddInsert(const Base::Vector3d& point,
                       const Base::Vector3d& scale,
                       const std::string& name,
                       double rotation) override
        {
            Reader.ExpandInsert(name, Reader.OCSOrientationTransform, point, rotation, scale);
        }
    };
    class ShapeSavingEntityCollector: public DrawingEntityCollector
    {
        // This places draft objects into the drawing but stashes away Shapes.
    public:
        ShapeSavingEntityCollector(
            ImpExpDxfRead& reader,
            std::map<CDxfRead::CommonEntityAttributes, std::list<TopoDS_Shape>>& shapesList)
            : DrawingEntityCollector(reader)
            , ShapesList(shapesList)
        {}

        void AddObject(const TopoDS_Shape& shape, const char* /*nameBase*/) override
        {
            ShapesList[Reader.m_entityAttributes].push_back(shape);
        }

    private:
        std::map<CDxfRead::CommonEntityAttributes, std::list<TopoDS_Shape>>& ShapesList;
    };
#ifdef LATER
    class PolylineEntityCollector: public CombiningDrawingEntityCollector
    {
        // This places draft objects into the drawing but stashes away Shapes.
    public:
        explicit PolylineEntityCollector(CDxfRead& reader)
            : CombiningDrawingEntityCollector(reader)
            , previousMmergeOption(reader.m_mergeOption)
        {
            // TODO: We also have to temporarily shift from DraftObjects to some other mode so the
            // pieces of the polyline some through as shapes and not Draft objects, or maybe the
            // polyline builder should not call OnArcRead and OnLineRead to do their dirty work.
        }
        ~PolylineEntityCollector();

        void AddObject(Part::TopoShape* shape) override;
        void AddObject(App::FeaturePython* shape) override;

    private:
        const EntityCollector* previousEntityCollector;
        const eEntityMergeType_t previousMmergeOption;
    };
#endif
    class BlockDefinitionCollector: public EntityCollector
    {
        // Collect all the entities plus their entityAttrubutes into given collections.
    public:
        BlockDefinitionCollector(
            ImpExpDxfRead& reader,
            std::map<CDxfRead::CommonEntityAttributes, std::list<TopoDS_Shape>>& shapesList,
            std::map<CDxfRead::CommonEntityAttributes, std::list<FeaturePythonBuilder>>&
                featureBuildersList,
            std::map<CDxfRead::CommonEntityAttributes, std::list<Block::Insert>>& insertsList)
            : EntityCollector(reader)
            , ShapesList(shapesList)
            , FeatureBuildersList(featureBuildersList)
            , InsertsList(insertsList)
        {}

        // TODO: We will want AddAttributeDefinition as well.
        void AddObject(const TopoDS_Shape& shape, const char* /*nameBase*/) override
        {
            ShapesList[Reader.m_entityAttributes].push_back(shape);
        }
        void AddObject(FeaturePythonBuilder shapeBuilder) override
        {
            FeatureBuildersList[Reader.m_entityAttributes].push_back(shapeBuilder);
        }
        void AddInsert(const Base::Vector3d& point,
                       const Base::Vector3d& scale,
                       const std::string& name,
                       double rotation) override
        {
            InsertsList[Reader.m_entityAttributes].push_back(
                Block::Insert(name, point, rotation, scale));
        }

    private:
        std::map<CDxfRead::CommonEntityAttributes, std::list<TopoDS_Shape>>& ShapesList;
        std::map<CDxfRead::CommonEntityAttributes, std::list<FeaturePythonBuilder>>&
            FeatureBuildersList;
        std::map<CDxfRead::CommonEntityAttributes, std::list<Block::Insert>>& InsertsList;
    };

private:
    EntityCollector* Collector = nullptr;
};

class ImportExport ImpExpDxfWrite: public CDxfWrite
{
public:
    explicit ImpExpDxfWrite(std::string filepath);
    ImpExpDxfWrite(const ImpExpDxfWrite&) = delete;
    ImpExpDxfWrite(const ImpExpDxfWrite&&) = delete;
    ImpExpDxfWrite& operator=(const ImpExpDxfWrite&) = delete;
    ImpExpDxfWrite& operator=(const ImpExpDxfWrite&&) = delete;
    ~ImpExpDxfWrite();

    void exportShape(TopoDS_Shape input);
    std::string getOptionSource()
    {
        return m_optionSource;
    }
    void setOptionSource(const std::string& s)
    {
        m_optionSource = s;
    }
    void setOptions();

    void exportText(const char* text,
                    Base::Vector3d position1,
                    Base::Vector3d position2,
                    double size,
                    int just);
    void exportLinearDim(Base::Vector3d textLocn,
                         Base::Vector3d lineLocn,
                         Base::Vector3d extLine1Start,
                         Base::Vector3d extLine2Start,
                         char* dimText,
                         int type);
    void exportAngularDim(Base::Vector3d textLocn,
                          Base::Vector3d lineLocn,
                          Base::Vector3d extLine1End,
                          Base::Vector3d extLine2End,
                          Base::Vector3d apexPoint,
                          char* dimText);
    void exportRadialDim(Base::Vector3d centerPoint,
                         Base::Vector3d textLocn,
                         Base::Vector3d arcPoint,
                         char* dimText);
    void exportDiametricDim(Base::Vector3d textLocn,
                            Base::Vector3d arcPoint1,
                            Base::Vector3d arcPoint2,
                            char* dimText);


    static bool gp_PntEqual(gp_Pnt p1, gp_Pnt p2);
    static bool gp_PntCompare(gp_Pnt p1, gp_Pnt p2);

protected:
    void exportCircle(BRepAdaptor_Curve& c);
    void exportEllipse(BRepAdaptor_Curve& c);
    void exportArc(BRepAdaptor_Curve& c);
    void exportEllipseArc(BRepAdaptor_Curve& c);
    void exportBSpline(BRepAdaptor_Curve& c);
    void exportBCurve(BRepAdaptor_Curve& c);
    void exportLine(BRepAdaptor_Curve& c);
    void exportLWPoly(BRepAdaptor_Curve& c);  // LWPolyline not supported in R12?
    void exportPolyline(BRepAdaptor_Curve& c);

    //        std::string m_optionSource;
    double optionMaxLength;
    bool optionPolyLine;
    bool optionExpPoints;
};

}  // namespace Import

#endif  // IMPEXPDXF_H
