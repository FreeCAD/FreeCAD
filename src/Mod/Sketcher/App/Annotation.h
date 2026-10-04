// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <App/Property.h>
#include <Base/Vector3D.h>
#include <Mod/Sketcher/SketcherGlobal.h>

namespace Sketcher
{
// Drawing data only: these records never enter Geometry, Shape, or the solver.
struct SketcherExport Annotation
{
    enum class Kind
    {
        Text,
        Hatch,
        Leader
    };
    long id = 0;
    Kind kind = Kind::Text;
    std::string label;
    bool construction = false;
    Base::Vector3d position;
    double rotation = 0;
    std::string html;
    double textSize = 3.5;
    double textWidth = 0;
    std::vector<long> boundary;
    double spacing = 2;
    /// A HatchPattern name; the pattern is scaled by spacing and turned by rotation.
    std::string pattern = "ANSI31";
    std::vector<Base::Vector3d> points;
    double arrowSize = 2;
    /// Leader start symbol, named as TechDraw's ArrowPropEnum so linked views match.
    std::string arrowStyle = "Open arrow";

    /// The arrowhead styles in presentation order.
    static const std::vector<std::string>& arrowStyles();

    // Cheap value identity: the Gui caches derived geometry and rasters against it.
    bool operator==(const Annotation& other) const = default;

    /// "Annotation<id>" is the stable subelement name. One definition for App, Gui and
    /// selection, so a foreign name such as "AnnotationGroup" is never mistaken for one.
    static long idFromSubName(std::string_view name);
    static std::string subName(long id);

    PyObject* toPython() const;
    static Annotation fromPython(PyObject* values, Annotation initial);
    void validate() const;
};

class SketcherExport PropertyAnnotationList: public App::Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    const std::vector<Annotation>& getValues() const
    {
        return values;
    }
    long highestAllocatedId() const
    {
        return highestId;
    }
    void setValue()
    {
        setValues({});
    }
    void setValues(std::vector<Annotation> annotations);
    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;
    App::Property* Copy() const override;
    void Paste(const App::Property& from) override;
    unsigned int getMemSize() const override;

private:
    std::vector<Annotation> values;
    // Survives Paste/undo so a new annotation cannot reuse a linked identity.
    long highestId = 0;
};
}  // namespace Sketcher
