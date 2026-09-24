// SPDX-License-Identifier: LGPL-2.1-or-later
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <QSignalBlocker>
#include <QApplication>
#include <QAbstractTextDocumentLayout>
#include <QCoreApplication>
#include <QLabel>
#include <QPen>
#include <QPixmap>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontMetricsF>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPointer>
#include <QSpinBox>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QHeaderView>
#include <QImage>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTableWidget>
#include <QTextDocument>
#include <QTimer>
#include <QListWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <Precision.hxx>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoRenderManager.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoRotationXYZ.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Quantity.h>
#include <Base/Unit.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Parameter.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Sketcher/App/HatchPattern.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "DrawSketchHandler.h"
#include "GeometryCreationMode.h"
#include "SketchAnnotations.h"
#include "ViewProviderSketch.h"
#include "Utils.h"

using namespace SketcherGui;
using Sketcher::Annotation;

namespace
{
constexpr const char* annotationNode = "SketchAnnotation_";
// Boundary edits arrive once per mouse move while geometry is dragged. Re-clipping a
// hatch is far too expensive at that rate, so let the geometry settle first.
constexpr int boundarySettleDelay = 150;
}  // namespace

long SketcherGui::annotationIdFromSubName(std::string_view name)
{
    return Annotation::idFromSubName(name);
}

std::string SketcherGui::annotationSubName(long id)
{
    return Annotation::subName(id);
}

std::string SketcherGui::annotationNodeName(long id)
{
    // Coin names allow no '-': previews of annotations not yet created use ID -1.
    return id < 0 ? std::string(annotationNode) + "Preview" : annotationNode + std::to_string(id);
}

namespace
{
/// Explains a refusal without stealing focus. A modal box on a Delete keypress would
/// be both intrusive and untestable; this reaches the report view and the status bar.
void notify(const QString& message)
{
    Base::Console().warning("%s\n", message.toUtf8().constData());
    if (auto* window = Gui::getMainWindow()) {
        window->showMessage(message, 4000);
    }
}
/// Selection in edit mode is recorded against the edited object path (a Body, for a
/// sketch inside one), so ask the view provider rather than the sketch object.
bool isAnnotationSelected(const ViewProviderSketch& view, long id)
{
    const auto subname = annotationSubName(id);
    if (view.isInEditMode()) {
        return view.isSelected(subname);
    }
    return Gui::Selection().isSelected(view.getObject(), subname.c_str());
}
void deselectAnnotation(ViewProviderSketch& view, long id)
{
    const auto subname = annotationSubName(id);
    if (view.isInEditMode()) {
        view.rmvSelection(subname);
        return;
    }
    Gui::Selection().rmvSelection(
        view.getObject()->getDocument()->getName(),
        view.getObject()->getNameInDocument(),
        subname.c_str()
    );
}
QString typeName(Annotation::Kind kind)
{
    if (kind == Annotation::Kind::Text) {
        return QObject::tr("Text");
    }
    if (kind == Annotation::Kind::Hatch) {
        return QObject::tr("Hatch");
    }
    return QObject::tr("Leader Line");
}
/// Whether two versions of an annotation draw the same strokes. Renaming a hatch must not
/// redo its boolean clipping.
bool sameStrokes(const Annotation& a, const Annotation& b)
{
    if (a.kind != b.kind) {
        return false;
    }
    if (a.kind == Annotation::Kind::Hatch) {
        return a.boundary == b.boundary && a.pattern == b.pattern && a.position == b.position
            && a.rotation == b.rotation && a.spacing == b.spacing;
    }
    return a.points == b.points && a.arrowSize == b.arrowSize && a.arrowStyle == b.arrowStyle;
}
const char* typeIcon(Annotation::Kind kind)
{
    switch (kind) {
        case Annotation::Kind::Text:
            return "Sketcher_CosmeticText";
        case Annotation::Kind::Hatch:
            return "Sketcher_CosmeticHatch";
        case Annotation::Kind::Leader:
            return "Sketcher_CosmeticLeader";
    }
    return "Sketcher_CosmeticLeader";
}
/// Selected sketch edges (not external geometry), in selection order.
std::vector<int> selectedEdges(ViewProviderSketch& view)
{
    std::vector<int> edges;
    auto* sketch = view.getSketchObject();
    for (const auto& selection : Gui::Selection().getSelectionEx(sketch->getDocument()->getName())) {
        if (selection.getObject() != sketch) {
            continue;
        }
        for (const auto& name : selection.getSubNames()) {
            int geo;
            Sketcher::PointPos pos;
            if (sketch->geoIdFromShapeType(name.c_str(), geo, pos) && geo >= 0
                && pos == Sketcher::PointPos::none
                && std::find(edges.begin(), edges.end(), geo) == edges.end()) {
                edges.push_back(geo);
            }
        }
    }
    return edges;
}
std::string pythonData(const Annotation& a)
{
    // Dictionaries use plain coordinate tuples for a reproducible recorded command.
    Base::PyGILStateLocker lock;
    Py::Dict dict(Py::Object(a.toPython(), true));
    auto vector = [](const Base::Vector3d& p) {
        Py::Tuple t(3);
        t[0] = Py::Float(p.x);
        t[1] = Py::Float(p.y);
        t[2] = Py::Float(p.z);
        return t;
    };
    dict.setItem("Position", vector(a.position));
    Py::List points;
    for (const auto& p : a.points) {
        points.append(vector(p));
    }
    dict.setItem("Points", points);
    Py::String repr(Py::Object(PyObject_Repr(dict.ptr()), true));
    return repr.as_string();
}
// Unit-aware throughout: the user's schema, decimal separator and expressions all work,
// and internal values stay in millimetres and degrees.
Gui::QuantitySpinBox* quantity(
    QFormLayout* form,
    const QString& label,
    double value,
    const Base::Unit& unit,
    double min,
    double max
)
{
    auto* box = new Gui::QuantitySpinBox;
    box->setUnit(unit);
    box->setMinimum(min);
    box->setMaximum(max);
    box->setValue(value);
    form->addRow(label, box);
    return box;
}
/// Accepts a plain number in the user's locale or an explicit length such as "0.5 in".
bool parseLength(const QString& text, double& out)
{
    try {
        const auto parsed = Base::Quantity::parse(text.trimmed().toStdString());
        if (!(parsed.getUnit() == Base::Unit::Length || parsed.getUnit() == Base::Unit::One)) {
            return false;
        }
        out = parsed.getValue();
        return std::isfinite(out);
    }
    catch (const Base::Exception&) {
        return false;
    }
}
QString patternLabel(std::string_view name)
{
    // Section-lining symbols of ANSI Y14.2 (acad.pat ANSI31–ANSI38). ISO 128-3 prescribes
    // the ANSI31 lines for general use and leaves material symbols to the drawing.
    static const std::map<std::string_view, const char*> labels {
        {"ANSI31", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "General use, cast iron (ANSI31, ISO 128)")},
        {"ANSI32", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Steel (ANSI32)")},
        {"ANSI33", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Bronze, brass, copper (ANSI33)")},
        {"ANSI34", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Plastic, rubber (ANSI34)")},
        {"ANSI35", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Refractory, fire brick (ANSI35)")},
        {"ANSI36", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Marble, slate, glass (ANSI36)")},
        {"ANSI37", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Lead, zinc, magnesium, insulation (ANSI37)")},
        {"ANSI38", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Aluminum (ANSI38)")},
        {"NET", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Square grid")},
        {"BRICK", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Brick")},
        {"EARTH", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Earth")},
        {"HBONE", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Herringbone")},
        {"CROSS", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Crosses")},
        {"HONEY", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Honeycomb")},
        {"INSUL", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Insulation")},
        {"DOTS", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Dots")},
        {"AR-CONC", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Concrete")},
        {"AR-SAND", QT_TRANSLATE_NOOP("SketcherGui::HatchPattern", "Sand")},
    };
    const auto it = labels.find(name);
    return it == labels.end() ? QString::fromUtf8(name.data(), static_cast<int>(name.size()))
                              : QCoreApplication::translate("SketcherGui::HatchPattern", it->second);
}
/// A swatch of the pattern for the combobox, drawn from the same PAT data as the sketch.
QIcon patternIcon(std::string_view name, const QColor& color)
{
    constexpr int width = 48;
    constexpr int height = 24;
    const qreal ratio = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pixmap(QSize(width, height) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 1.0));
    painter.setClipRect(0, 0, width, height);
    // Sketch Y points up, as in the viewport.
    painter.translate(0, height);
    painter.scale(1, -1);
    const double radius = std::hypot(width, height);
    const Base::Vector3d center(width / 2.0, height / 2.0, 0);
    const auto families = Sketcher::placeHatchPattern(name, Base::Vector3d(), 0, 3.0);
    int budget = 4000;
    for (const auto& lines : families) {
        const Base::Vector3d normal(-lines.direction.y, lines.direction.x, 0);
        const double pitch = lines.offset.Dot(normal);
        if (std::abs(pitch) < 0.2) {
            continue;
        }
        const double c = (center - lines.origin).Dot(normal) / pitch;
        const double span = radius / std::abs(pitch);
        double period = 0;
        for (double dash : lines.dashes) {
            period += std::abs(dash);
        }
        for (int k = int(std::floor(c - span)); k <= int(std::ceil(c + span)) && budget > 0; ++k) {
            const auto through = lines.origin + lines.offset * k;
            const double along = (center - through).Dot(lines.direction);
            auto draw = [&](double t0, double t1) {
                const auto p = through + lines.direction * t0;
                const auto q = through + lines.direction * t1;
                painter.drawLine(QPointF(p.x, p.y), QPointF(q.x, q.y));
                --budget;
            };
            if (period < 1e-9) {
                draw(along - radius, along + radius);
                continue;
            }
            for (double cycle = std::floor((along - radius) / period) * period;
                 cycle < along + radius && budget > 0;
                 cycle += period) {
                double at = cycle;
                for (double dash : lines.dashes) {
                    if (dash > 0) {
                        draw(at, at + dash);
                    }
                    else if (dash == 0) {
                        draw(at, at + 0.4);
                    }
                    at += std::abs(dash);
                }
            }
        }
    }
    painter.end();
    return QIcon(pixmap);
}
void fillPatternCombo(QComboBox* combo, const std::string& current)
{
    const auto color = combo->palette().color(QPalette::Text);
    combo->setIconSize(QSize(48, 24));
    for (const auto& pattern : Sketcher::hatchPatterns()) {
        const std::string_view name = pattern.name;
        if (name == "NET") {
            combo->insertSeparator(combo->count());
        }
        combo->addItem(patternIcon(name, color), patternLabel(name), QString::fromLatin1(pattern.name));
    }
    combo->setCurrentIndex(std::max(0, combo->findData(QString::fromStdString(current))));
}
QString spacingToolTip()
{
    return QCoreApplication::translate(
        "SketcherGui::HatchHandler",
        "Sets the distance between the general-use hatch lines. The other patterns scale by "
        "the same factor, so they keep their standard proportions."
    );
}
QString patternToolTip()
{
    return QCoreApplication::translate(
        "SketcherGui::HatchHandler",
        "Sets the hatch pattern. ANSI31 to ANSI38 are the standard section symbols for "
        "materials; the others are common drafting patterns."
    );
}
QString angleToolTip()
{
    return QCoreApplication::translate(
        "SketcherGui::HatchHandler",
        "Sets the rotation of the pattern. At 0° the patterns are drawn as defined, with the "
        "general-use lines at 45°."
    );
}
QString arrowStyleToolTip()
{
    return QCoreApplication::translate(
        "SketcherGui::LeaderHandler",
        "Sets the symbol drawn at the first point of the leader line"
    );
}
QString arrowSizeToolTip()
{
    return QCoreApplication::translate(
        "SketcherGui::LeaderHandler",
        "Sets the length of the arrowhead along the leader line"
    );
}
QString arrowStyleLabel(const std::string& style)
{
    // The names are TechDraw's ArrowPropEnum values, stored as is.
    static const std::map<std::string, const char*> labels {
        {"Filled arrow", QT_TRANSLATE_NOOP("SketcherGui::ArrowStyle", "Filled arrow")},
        {"Open arrow", QT_TRANSLATE_NOOP("SketcherGui::ArrowStyle", "Open arrow")},
        {"Tick", QT_TRANSLATE_NOOP("SketcherGui::ArrowStyle", "Tick")},
        {"Dot", QT_TRANSLATE_NOOP("SketcherGui::ArrowStyle", "Dot")},
        {"Open circle", QT_TRANSLATE_NOOP("SketcherGui::ArrowStyle", "Open circle")},
        {"Fork", QT_TRANSLATE_NOOP("SketcherGui::ArrowStyle", "Fork")},
        {"Filled triangle", QT_TRANSLATE_NOOP("SketcherGui::ArrowStyle", "Filled triangle")},
        {"None", QT_TRANSLATE_NOOP("SketcherGui::ArrowStyle", "None")},
    };
    const auto it = labels.find(style);
    return it == labels.end() ? QString::fromStdString(style)
                              : QCoreApplication::translate("SketcherGui::ArrowStyle", it->second);
}
/// A swatch of the arrowhead, drawn from the same geometry as the sketch.
QIcon arrowStyleIcon(const Sketcher::SketchObject& sketch, const std::string& style, const QColor& color)
{
    constexpr int width = 48;
    constexpr int height = 24;
    const qreal ratio = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pixmap(QSize(width, height) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    Annotation sample;
    sample.kind = Annotation::Kind::Leader;
    sample.arrowStyle = style;
    sample.arrowSize = 16;
    sample.points = {Base::Vector3d(6, height / 2.0, 0), Base::Vector3d(width - 4, height / 2.0, 0)};
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 1.2));
    painter.setBrush(color);
    const auto lines = sketch.annotationStrokes(sample);
    for (size_t i = 0; i + 1 < lines.size(); i += 2) {
        painter.drawLine(QPointF(lines[i].x, lines[i].y), QPointF(lines[i + 1].x, lines[i + 1].y));
    }
    const auto fills = sketch.annotationFills(sample);
    painter.setPen(Qt::NoPen);
    for (size_t i = 0; i + 2 < fills.size(); i += 3) {
        const QPointF triangle[] = {
            {fills[i].x, fills[i].y},
            {fills[i + 1].x, fills[i + 1].y},
            {fills[i + 2].x, fills[i + 2].y},
        };
        painter.drawPolygon(triangle, 3);
    }
    painter.end();
    return QIcon(pixmap);
}
void fillArrowStyleCombo(QComboBox* combo, const Sketcher::SketchObject& sketch, const std::string& current)
{
    const auto color = combo->palette().color(QPalette::Text);
    combo->setIconSize(QSize(48, 24));
    for (const auto& style : Annotation::arrowStyles()) {
        combo->addItem(
            arrowStyleIcon(sketch, style, color),
            arrowStyleLabel(style),
            QString::fromStdString(style)
        );
    }
    combo->setCurrentIndex(std::max(0, combo->findData(QString::fromStdString(current))));
}
/// The color annotation strokes and text use for this annotation, as the scene draws it.
QColor annotationInk(const ViewProviderSketch& view, const Annotation& a)
{
    Base::Color color;
    if (a.construction) {
        color.setPackedValue(
            App::GetApplication()
                .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                ->GetUnsigned("ConstructionColor", 0x0000dcff)
        );
    }
    else {
        color = view.LineColor.getValue();
    }
    return QColor::fromRgbF(color.r, color.g, color.b);
}

/// The command and tool icon of a cosmetic type, in construction form when asked.
QPixmap toolIcon(Annotation::Kind kind, bool construction)
{
    return Gui::BitmapFactory().pixmap(
        (std::string(typeIcon(kind)) + (construction ? "_Constr" : "")).c_str()
    );
}
/// Tool widgets are built once per tool; only their task box header shows the mode.
void setToolHeaderIcon(QWidget* toolwidget, const QPixmap& icon)
{
    for (auto* widget = toolwidget; widget; widget = widget->parentWidget()) {
        if (auto* box = qobject_cast<Gui::TaskView::TaskBox*>(widget)) {
            box->setHeaderIcon(icon);
            return;
        }
    }
}

/// Edits an annotation's rich text in the 3D view itself, like TechDraw's rich annotation:
/// a text item on the viewer's graphics scene, laid over the sketch exactly where the
/// text is drawn, with a formatting toolbar above it. Keys go to the text, not to Coin or
/// to application shortcuts, while it is being edited.
class TextEditOverlay: public QObject
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::TextEditOverlay)

public:
    TextEditOverlay(ViewProviderSketch& view, Gui::View3DInventorViewer* viewer, const Annotation& a)
        : view(view)
        , viewer(viewer)
        , annotation(a)
    {
        auto* scene = viewer->scene();
        text = new QGraphicsTextItem;
        text->setObjectName(QStringLiteral("sketchTextEditor"));
        text->document()->setDefaultFont(textFont());
        text->document()->setDocumentMargin(0);
        text->setDefaultTextColor(annotationInk(view, a));
        if (!a.html.empty()) {
            text->setHtml(QString::fromStdString(a.html));
        }
        text->setTextInteractionFlags(Qt::TextEditorInteraction);
        text->setZValue(1000);
        scene->addItem(text);
        buildToolbar();
        proxy = scene->addWidget(toolbar);
        proxy->setZValue(1001);
        connect(text->document(), &QTextDocument::contentsChanged, this, [this] { place(); });
        connect(text->document(), &QTextDocument::cursorPositionChanged, this, [this] {
            syncToolbar();
        });
        cameraSensor.setFunction([](void* data, SoSensor*) {
            static_cast<TextEditOverlay*>(data)->place();
        });
        cameraSensor.setData(this);
        if (auto* camera = viewer->getCamera()) {
            cameraSensor.attach(camera);
        }
        viewer->installEventFilter(this);
        viewer->viewport()->installEventFilter(this);
        setGeometry(a.textSize, a.textWidth, a.rotation);
        text->document()->setModified(false);
        viewer->setFocus();
        scene->setFocus();
        text->setFocus();
        auto cursor = text->textCursor();
        cursor.movePosition(QTextCursor::End);
        text->setTextCursor(cursor);
        syncToolbar();
    }
    ~TextEditOverlay() override
    {
        close();
    }
    /// Removes the editor from the view at once; the object itself may be deleted later.
    void close()
    {
        finished = nullptr;
        cameraSensor.detach();
        if (!text) {
            return;
        }
        // Without its viewer, the scene has already deleted the items (and the pointers
        // to them are null).
        if (viewer) {
            viewer->removeEventFilter(this);
            viewer->viewport()->removeEventFilter(this);
            viewer->viewport()->update();
        }
        delete text;
        delete proxy;  // Also deletes the toolbar.
    }
    /// False once the editor or its view is gone, e.g. the view was closed while editing.
    bool isAlive() const
    {
        return text && viewer;
    }

    /// Called (queued) when the user finishes with Escape or Ctrl+Enter. It may delete
    /// this overlay's owner, so it is moved out before it runs.
    std::function<void()> finished;

    void setGeometry(double size, double width, double rotation)
    {
        if (!text) {
            return;
        }
        annotation.textSize = size;
        annotation.textWidth = width;
        annotation.rotation = rotation;
        text->setTextWidth(width > 0 ? width / unitsPerPixel() : -1);
        place();
    }
    void setInk(const QColor& color)
    {
        if (text) {
            text->setDefaultTextColor(color);
        }
    }
    QString html() const
    {
        return text ? text->toHtml() : QString();
    }
    /// Whether the content was edited; formatting-only HTML differences do not count.
    bool isModified() const
    {
        return text && text->document()->isModified();
    }
    bool isEmpty() const
    {
        return !text || text->document()->toPlainText().trimmed().isEmpty();
    }

private:
    static QFont textFont()
    {
        // The scene rasters text with this font; the same layout keeps the editor aligned.
        return QFont(QStringLiteral("Sans Serif"), 12);
    }
    double unitsPerPixel() const
    {
        return annotation.textSize / QFontMetricsF(textFont()).height();
    }
    QPointF project(const Base::Vector3d& local) const
    {
        const auto world = view.getDocument()->getEditingTransform() * local;
        auto* manager = viewer->getSoRenderManager();
        const auto& region = manager->getViewportRegion();
        const SbViewVolume volume
            = manager->getCamera()->getViewVolume(region.getViewportAspectRatio());
        SbVec3f screen;
        volume.projectToScreen(SbVec3f(float(world.x), float(world.y), float(world.z)), screen);
        const auto size = region.getViewportSizePixels();
        const double ratio = viewer->devicePixelRatio();
        return {screen[0] * size[0] / ratio, (1.0 - screen[1]) * size[1] / ratio};
    }
    /// Maps document pixels onto the view: the anchor is the text's top left corner, the
    /// text grows along the rotated sketch X axis and down the rotated sketch -Y axis.
    void place()
    {
        if (!text || !viewer || !viewer->getSoRenderManager()->getCamera()) {
            return;
        }
        constexpr double step = 100;  // Project a long basis; single pixels lose precision.
        const double units = unitsPerPixel() * step;
        const double angle = annotation.rotation * std::acos(-1.0) / 180.0;
        const Base::Vector3d along(std::cos(angle), std::sin(angle), 0);
        const Base::Vector3d down(std::sin(angle), -std::cos(angle), 0);
        const auto origin = project(annotation.position);
        const auto x = (project(annotation.position + along * units) - origin) / step;
        const auto y = (project(annotation.position + down * units) - origin) / step;
        text->setTransform(QTransform(x.x(), x.y(), y.x(), y.y(), origin.x(), origin.y()));
        const auto box = text->sceneBoundingRect();
        const auto area = QRectF(QPointF(0, 0), QSizeF(viewer->viewport()->size()));
        const QSizeF bar = toolbar->sizeHint();
        QPointF at(box.left(), box.top() - bar.height() - 6);
        at.setX(std::clamp(at.x(), area.left(), std::max(area.left(), area.right() - bar.width())));
        at.setY(std::clamp(at.y(), area.top(), std::max(area.top(), area.bottom() - bar.height())));
        proxy->setPos(at);
        viewer->viewport()->update();
    }
    QToolButton* button(const QString& label, const QString& tip, bool checkable)
    {
        auto* b = new QToolButton;
        b->setText(label);
        b->setToolTip(tip);
        b->setCheckable(checkable);
        b->setFocusPolicy(Qt::NoFocus);
        b->setAutoRaise(true);
        return b;
    }
    void merge(const QTextCharFormat& format)
    {
        auto cursor = text->textCursor();
        cursor.mergeCharFormat(format);
        text->setTextCursor(cursor);
        text->setFocus();
        syncToolbar();
    }
    void buildToolbar()
    {
        toolbar = new QWidget;
        toolbar->setObjectName(QStringLiteral("sketchTextToolbar"));
        toolbar->setAutoFillBackground(true);
        auto* layout = new QHBoxLayout(toolbar);
        layout->setContentsMargins(3, 3, 3, 3);
        layout->setSpacing(2);
        family = new QFontComboBox;
        family->setObjectName(QStringLiteral("sketchTextFont"));
        family->setMaximumWidth(160);
        family->setToolTip(tr("Sets the font of the selected text"));
        layout->addWidget(family);
        connect(family, &QFontComboBox::currentFontChanged, this, [this](const QFont& font) {
            if (syncing) {
                return;
            }
            QTextCharFormat format;
            format.setFontFamilies({font.family()});
            merge(format);
        });
        points = new QSpinBox;
        points->setObjectName(QStringLiteral("sketchTextSize"));
        points->setRange(1, 400);
        points->setSuffix(QStringLiteral(" pt"));
        points->setToolTip(
            tr("Sets the font size of the selected text. 12 pt is the text height set in the "
               "tool; other sizes scale from it.")
        );
        layout->addWidget(points);
        connect(points, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            if (syncing) {
                return;
            }
            QTextCharFormat format;
            format.setFontPointSize(value);
            merge(format);
        });
        bold = button(QStringLiteral("B"), tr("Toggles bold for the selected text"), true);
        auto boldFont = bold->font();
        boldFont.setBold(true);
        bold->setFont(boldFont);
        connect(bold, &QToolButton::clicked, this, [this](bool on) {
            QTextCharFormat format;
            format.setFontWeight(on ? QFont::Bold : QFont::Normal);
            merge(format);
        });
        italic = button(QStringLiteral("I"), tr("Toggles italic for the selected text"), true);
        auto italicFont = italic->font();
        italicFont.setItalic(true);
        italic->setFont(italicFont);
        connect(italic, &QToolButton::clicked, this, [this](bool on) {
            QTextCharFormat format;
            format.setFontItalic(on);
            merge(format);
        });
        underline = button(QStringLiteral("U"), tr("Toggles underline for the selected text"), true);
        auto underlineFont = underline->font();
        underlineFont.setUnderline(true);
        underline->setFont(underlineFont);
        connect(underline, &QToolButton::clicked, this, [this](bool on) {
            QTextCharFormat format;
            format.setFontUnderline(on);
            merge(format);
        });
        strike = button(QStringLiteral("S"), tr("Toggles strikethrough for the selected text"), true);
        auto strikeFont = strike->font();
        strikeFont.setStrikeOut(true);
        strike->setFont(strikeFont);
        connect(strike, &QToolButton::clicked, this, [this](bool on) {
            QTextCharFormat format;
            format.setFontStrikeOut(on);
            merge(format);
        });
        for (auto* b : {bold, italic, underline, strike}) {
            layout->addWidget(b);
        }
        auto* color = button(QStringLiteral("A"), tr("Sets the color of the selected text"), false);
        color->setObjectName(QStringLiteral("sketchTextColor"));
        connect(color, &QToolButton::clicked, this, [this] {
            const auto chosen = QColorDialog::getColor(
                text->textCursor().charFormat().foreground().color(),
                Gui::getMainWindow()
            );
            if (chosen.isValid()) {
                QTextCharFormat format;
                format.setForeground(chosen);
                merge(format);
            }
        });
        layout->addWidget(color);
        const std::pair<Qt::Alignment, const char*> alignments[] = {
            {Qt::AlignLeft,
             QT_TRANSLATE_NOOP("SketcherGui::TextEditOverlay", "Aligns the paragraph to the left")},
            {Qt::AlignHCenter,
             QT_TRANSLATE_NOOP("SketcherGui::TextEditOverlay", "Centers the paragraph")},
            {Qt::AlignRight,
             QT_TRANSLATE_NOOP("SketcherGui::TextEditOverlay", "Aligns the paragraph to the right")},
        };
        const QString glyphs[] = {QString(QChar(0x21E4)), QString(QChar(0x2194)), QString(QChar(0x21E5))};
        for (int i = 0; i < 3; ++i) {
            auto* b = button(glyphs[i], tr(alignments[i].second), false);
            const auto alignment = alignments[i].first;
            connect(b, &QToolButton::clicked, this, [this, alignment] {
                QTextBlockFormat format;
                format.setAlignment(alignment);
                auto cursor = text->textCursor();
                cursor.mergeBlockFormat(format);
                text->setTextCursor(cursor);
                text->setFocus();
            });
            layout->addWidget(b);
        }
        toolbar->adjustSize();
    }
    void syncToolbar()
    {
        if (!toolbar || !text) {
            return;
        }
        syncing = true;
        const auto format = text->textCursor().charFormat();
        const auto font = format.font();
        family->setCurrentFont(font);
        points->setValue(format.fontPointSize() > 0 ? int(std::lround(format.fontPointSize())) : 12);
        bold->setChecked(format.fontWeight() >= QFont::Bold);
        italic->setChecked(format.fontItalic());
        underline->setChecked(format.fontUnderline());
        strike->setChecked(format.fontStrikeOut());
        syncing = false;
    }
    bool eventFilter(QObject* object, QEvent* event) override
    {
        if (!viewer || !text) {
            return false;
        }
        auto* scene = viewer->scene();
        // While the editor is open every key belongs to it. A click on the toolbar's
        // background or on empty scene clears the scene's focus item; give it back to the
        // text rather than letting the key reach Sketcher or application shortcuts.
        auto focusEditor = [this, scene] {
            if (!scene->focusItem()) {
                scene->setFocus();
                text->setFocus();
            }
        };
        switch (event->type()) {
            case QEvent::ShortcutOverride:
                event->accept();
                return true;
            case QEvent::KeyPress: {
                focusEditor();
                const auto* key = static_cast<QKeyEvent*>(event);
                const bool enter = key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter;
                if (key->key() == Qt::Key_Escape
                    || (enter && (key->modifiers() & Qt::ControlModifier))) {
                    // The owner deletes this overlay; never do that inside its filter.
                    QTimer::singleShot(0, this, [this] {
                        if (auto callback = std::move(finished)) {
                            callback();
                        }
                    });
                    return true;
                }
                QCoreApplication::sendEvent(scene, event);
                return true;
            }
            case QEvent::KeyRelease:
            case QEvent::InputMethod:
                focusEditor();
                QCoreApplication::sendEvent(scene, event);
                return true;
            case QEvent::Resize:
                if (object == viewer) {
                    QTimer::singleShot(0, this, [this] { place(); });
                }
                break;
            default:
                break;
        }
        return false;
    }

    ViewProviderSketch& view;
    QPointer<Gui::View3DInventorViewer> viewer;
    Annotation annotation;
    // The viewer's scene owns these items and deletes them if the view closes first.
    QPointer<QGraphicsTextItem> text;
    QPointer<QGraphicsProxyWidget> proxy;
    QPointer<QWidget> toolbar;
    QFontComboBox* family = nullptr;
    QSpinBox* points = nullptr;
    QToolButton* bold = nullptr;
    QToolButton* italic = nullptr;
    QToolButton* underline = nullptr;
    QToolButton* strike = nullptr;
    bool syncing = false;
    SoNodeSensor cameraSensor;
};

/// Places and edits cosmetic text in the view. A click places a new text and opens the
/// in-view editor; clicking elsewhere, Escape or Ctrl+Enter finishes it. Text height,
/// wrapping width and rotation are in the tool widget, so there is no edit dialog.
class TextHandler: public DrawSketchHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::TextHandler)

public:
    explicit TextHandler(std::optional<Annotation> existing)
        : existing(std::move(existing))
    {
        if (this->existing) {
            settings = *this->existing;
            return;
        }
        auto parameters = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/Annotations"
        );
        settings.kind = Annotation::Kind::Text;
        settings.textSize = std::max(0.01, parameters->GetFloat("TextHeight", 3.5));
        settings.textWidth = std::max(0.0, parameters->GetFloat("TextWidth", 0.0));
        settings.rotation = parameters->GetFloat("TextRotation", 0.0);
    }
    ~TextHandler() override
    {
        disconnectWidget();
        dropEditor();
    }

    std::string getToolName() const override
    {
        return "DSH_CosmeticText";
    }
    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_CosmeticText.svg");
    }
    std::unique_ptr<QWidget> createWidget() const override
    {
        auto widget = std::make_unique<QWidget>();
        widget->setObjectName("sketchTextToolWidget");
        auto* form = new QFormLayout(widget.get());
        auto quantity = [form](const char* name, const QString& label, const Base::Unit& unit,
                               double min, double max, double value) {
            auto* box = new Gui::QuantitySpinBox;
            box->setObjectName(QString::fromLatin1(name));
            box->setUnit(unit);
            box->setMinimum(min);
            box->setMaximum(max);
            box->setValue(value);
            form->addRow(label, box);
            return box;
        };
        quantity("textHeight", tr("Text height"), Base::Unit::Length, 0.01, 10000, settings.textSize)
            ->setToolTip(tr("Sets the height of a line of 12 pt text; other font sizes scale from it"));
        quantity("textWidth", tr("Wrapping width"), Base::Unit::Length, 0, 100000, settings.textWidth)
            ->setToolTip(tr("Sets the width at which lines wrap. 0 disables wrapping."));
        quantity("textRotation", tr("Rotation"), Base::Unit::Angle, -360, 360, settings.rotation)
            ->setToolTip(tr("Sets the rotation of the text around its top-left corner"));
        return widget;
    }
    bool isWidgetVisible() const override
    {
        return true;
    }
    QPixmap getToolIcon() const override
    {
        return toolIcon(Annotation::Kind::Text, isConstructionMode());
    }
    void onConstructionModeChanged() override
    {
        setToolHeaderIcon(toolwidget, getToolIcon());
        // A new text follows the mode; an edited one keeps its own construction flag.
        if (editor && !existing) {
            settings.construction = isConstructionMode();
            editor->setInk(annotationInk(*sketchgui, settings));
        }
    }
    QString getToolWidgetText() const override
    {
        return tr("Cosmetic Text Parameters");
    }

    void mouseMove(SnapManager::SnapHandle snap) override
    {
        if (!editor) {
            setPositionText(snap.compute());
        }
    }
    bool pressButton(Base::Vector2d) override
    {
        return true;
    }
    bool releaseButton(Base::Vector2d p) override
    {
        if (editor) {
            finishEditing();  // A click outside the text finishes it.
            return true;
        }
        settings.position = Base::Vector3d(p.x, p.y, 0);
        settings.construction = isConstructionMode();
        settings.html.clear();
        startEditing();
        return true;
    }
    void pressRightButton(Base::Vector2d) override
    {
        if (editor) {
            finishEditing();
        }
        else {
            quit();
        }
    }
    void cancelCurrentAction() override
    {
        if (editor) {
            finishEditing();
        }
        else {
            quit();
        }
    }
    void deactivate() override
    {
        disconnectWidget();
        // Leaving the tool keeps what was typed, as clicking away does. Cancelling the sketch
        // edit restores the sketch anyway, so it must not add an undo step first.
        if (editor && !sketchgui->editingCancelled) {
            commit();
        }
        dropEditor();
        DrawSketchHandler::deactivate();
    }
    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;
        if (!editor) {
            return {{tr("%1 place text"), {MouseLeft}}};
        }
        // Clicking anywhere outside the text, or Escape, keeps what was typed.
        return {{tr("%1/%2 finish text"), {MouseLeft, KeyEscape}}};
    }

private:
    void activated() override
    {
        continuousMode = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher")
                             ->GetBool("ContinuousCreationMode", true);
        if (existing) {
            startEditing();
        }
    }
    void onWidgetChanged() override
    {
        disconnectWidget();
        if (!toolwidget) {
            return;
        }
        auto connectQuantity = [this](const char* name, double Annotation::* field) {
            if (auto* box = toolwidget->findChild<Gui::QuantitySpinBox*>(name)) {
                connections.push_back(QObject::connect(
                    box,
                    qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
                    [this, field](double value) {
                        settings.*field = value;
                        if (editor) {
                            editor->setGeometry(settings.textSize, settings.textWidth, settings.rotation);
                        }
                    }
                ));
            }
        };
        connectQuantity("textHeight", &Annotation::textSize);
        connectQuantity("textWidth", &Annotation::textWidth);
        connectQuantity("textRotation", &Annotation::rotation);
    }
    void disconnectWidget()
    {
        for (const auto& connection : connections) {
            QObject::disconnect(connection);
        }
        connections.clear();
    }
    Gui::View3DInventorViewer* viewer() const
    {
        auto* document = sketchgui->getDocument();
        auto* active = document ? qobject_cast<Gui::View3DInventor*>(document->getActiveView())
                                : nullptr;
        return active ? active->getViewer() : nullptr;
    }
    void startEditing()
    {
        auto* target = viewer();
        if (!target) {
            return;
        }
        editor = new TextEditOverlay(*sketchgui, target, settings);
        editor->finished = [this] { finishEditing(); };
        // The scene would draw the stored text under the editor; hide it meanwhile.
        sketchgui->annotationManager().setTextEditing(existing ? existing->id : 0);
        updateHint();
    }
    void dropEditor()
    {
        if (!editor) {
            return;
        }
        editor->close();
        editor->deleteLater();
        editor = nullptr;
        if (sketchgui) {
            sketchgui->annotationManager().setTextEditing(0);
        }
    }
    /// Stores the edited text. Returns false when that was refused, so the editor stays.
    bool commit()
    {
        if (!editor->isAlive()) {
            return true;  // The view closed under the editor; its content is gone.
        }
        auto& manager = sketchgui->annotationManager();
        const bool empty = editor->isEmpty();
        auto a = settings;
        a.html = editor->html().toStdString();
        if (existing) {
            if (empty) {
                manager.remove({existing->id});  // Clearing a text deletes it.
                return true;
            }
            if (editor->isModified() || a.textSize != existing->textSize
                || a.textWidth != existing->textWidth || a.rotation != existing->rotation) {
                if (!editor->isModified()) {
                    a.html = existing->html;  // Keep the stored HTML when only settings changed.
                }
                return manager.save(a, false);
            }
            return true;
        }
        if (empty) {
            return true;
        }
        a.id = 0;
        auto parameters = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/Annotations"
        );
        parameters->SetFloat("TextHeight", settings.textSize);
        parameters->SetFloat("TextWidth", settings.textWidth);
        parameters->SetFloat("TextRotation", settings.rotation);
        return manager.save(a, true);
    }
    void finishEditing()
    {
        if (!editor) {
            return;
        }
        if (!commit()) {
            // Keep editing; Escape was consumed by this call, so arm it again.
            editor->finished = [this] { finishEditing(); };
            return;
        }
        dropEditor();
        if (existing || !continuousMode) {
            quit();  // Deletes this handler.
            return;
        }
        updateHint();
    }

    std::optional<Annotation> existing;
    Annotation settings;
    TextEditOverlay* editor = nullptr;
    bool continuousMode = true;
    std::vector<QMetaObject::Connection> connections;
};

// getPreselectCurve(): -1 is nothing, other negative values are external geometry.
constexpr int noCurve = -1;

std::string edgeName(int geoId)
{
    return "Edge" + std::to_string(geoId + 1);
}

/// The current index of the geometry with this stable ID, or -1.
int geoIdOf(const Sketcher::SketchObject& sketch, long stableId)
{
    const auto& geometry = sketch.Geometry.getValues();
    for (size_t i = 0; i < geometry.size(); ++i) {
        if (Sketcher::GeometryFacade::getId(geometry[i]) == stableId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/// The edges of the closed loop through geoId, or nothing when geoId is on no loop.
/// Edges dangling from the loop (a construction line leaving a corner, for instance)
/// are pruned, so they neither prevent nor join the hatch boundary.
std::vector<int> closedLoop(const ViewProviderSketch& view, int geoId)
{
    const auto* sketch = view.getSketchObject();
    const auto* geo = sketch->getGeometry(geoId);
    if (geoId < 0 || !geo || Sketcher::isPoint(*geo)) {
        return {};
    }
    if (Sketcher::isCircle(*geo) || Sketcher::isEllipse(*geo)
        || Sketcher::isPeriodicBSplineCurve(*geo)) {
        return {geoId};
    }
    std::vector<Base::Vector3d> nodes;
    auto node = [&nodes](const Base::Vector3d& p) {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if ((nodes[i] - p).Length() < Precision::Confusion()) {
                return i;
            }
        }
        nodes.push_back(p);
        return nodes.size() - 1;
    };
    struct Edge
    {
        int geoId;
        size_t start;
        size_t end;
    };
    std::vector<Edge> edges;
    for (int id : view.getConnectedEdges(geoId, false)) {
        const auto start = node(sketch->getPoint(id, Sketcher::PointPos::start));
        edges.push_back({id, start, node(sketch->getPoint(id, Sketcher::PointPos::end))});
    }
    for (bool pruned = true; pruned;) {
        std::vector<int> degree(nodes.size(), 0);
        for (const auto& edge : edges) {
            ++degree[edge.start];
            ++degree[edge.end];
        }
        const auto before = edges.size();
        std::erase_if(edges, [&degree](const Edge& edge) {
            return degree[edge.start] < 2 || degree[edge.end] < 2;
        });
        pruned = edges.size() != before;
    }
    std::vector<int> loop;
    for (const auto& edge : edges) {
        loop.push_back(edge.geoId);
    }
    if (std::find(loop.begin(), loop.end(), geoId) == loop.end()) {
        return {};
    }
    return loop;
}

/// Remembered settings of the cosmetic tools (text, hatch and leader line).
ParameterGrp::handle annotationParameters()
{
    return App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/Annotations"
    );
}

/// Picks closed loops one click at a time and shows the resulting hatch as it grows.
/// Nested loops become holes, so an outer and an inner rectangle hatch the frame.
class HatchHandler: public DrawSketchHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::HatchHandler)

public:
    explicit HatchHandler(std::vector<int> preselected)
        : preselected(std::move(preselected))
    {
        auto parameters = annotationParameters();
        settings.kind = Annotation::Kind::Hatch;
        settings.pattern = parameters->GetASCII("HatchPattern", Sketcher::defaultHatchPattern);
        if (!Sketcher::findHatchPattern(settings.pattern)) {
            settings.pattern = Sketcher::defaultHatchPattern;
        }
        settings.spacing = std::max(0.01, parameters->GetFloat("HatchSpacing", 2.0));
        settings.rotation = parameters->GetFloat("HatchAngle", 0.0);
    }
    ~HatchHandler() override
    {
        disconnectWidget();
    }

    std::string getToolName() const override
    {
        return "DSH_Hatch";
    }
    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_CosmeticHatch.svg");
    }
    std::unique_ptr<QWidget> createWidget() const override
    {
        auto widget = std::make_unique<QWidget>();
        widget->setObjectName("sketchHatchToolWidget");
        auto* form = new QFormLayout(widget.get());
        auto* pattern = new QComboBox;
        pattern->setObjectName("hatchPattern");
        fillPatternCombo(pattern, settings.pattern);
        pattern->setToolTip(patternToolTip());
        form->addRow(tr("Pattern"), pattern);
        auto* spacing = new Gui::QuantitySpinBox;
        spacing->setObjectName("hatchSpacing");
        spacing->setUnit(Base::Unit::Length);
        spacing->setMinimum(0.01);
        spacing->setMaximum(100000);
        spacing->setValue(settings.spacing);
        spacing->setToolTip(spacingToolTip());
        form->addRow(tr("Spacing"), spacing);
        auto* angle = new Gui::QuantitySpinBox;
        angle->setObjectName("hatchAngle");
        angle->setUnit(Base::Unit::Angle);
        angle->setMinimum(-360);
        angle->setMaximum(360);
        angle->setValue(settings.rotation);
        angle->setToolTip(angleToolTip());
        form->addRow(tr("Angle"), angle);
        auto* status = new QLabel;
        status->setObjectName("hatchStatus");
        status->setWordWrap(true);
        form->addRow(status);
        return widget;
    }
    bool isWidgetVisible() const override
    {
        return true;
    }
    QPixmap getToolIcon() const override
    {
        return toolIcon(Annotation::Kind::Hatch, isConstructionMode());
    }
    void onConstructionModeChanged() override
    {
        setToolHeaderIcon(toolwidget, getToolIcon());
        refresh();
    }
    QString getToolWidgetText() const override
    {
        return tr("Hatch Parameters");
    }

    void mouseMove(SnapManager::SnapHandle) override
    {}
    bool pressButton(Base::Vector2d) override
    {
        return true;
    }
    bool releaseButton(Base::Vector2d) override
    {
        const int geoId = getPreselectCurve();
        if (geoId >= 0) {
            toggleLoop(geoId, true);
        }
        else if (geoId != noCurve) {
            notify(tr("External geometry cannot bound a hatch"));
        }
        return true;
    }
    void registerPressedKey(bool pressed, int key) override
    {
        if (pressed && (key == SoKeyboardEvent::RETURN || key == SoKeyboardEvent::PAD_ENTER)) {
            commit();
        }
        else {
            DrawSketchHandler::registerPressedKey(pressed, key);
        }
    }
    void pressRightButton(Base::Vector2d) override
    {
        if (loops.empty()) {
            quit();
        }
        else {
            commit();
        }
    }
    void cancelCurrentAction() override
    {
        quit();
    }
    void deactivate() override
    {
        disconnectWidget();
        clearLoopSelection();
        loops.clear();
        sketchgui->annotationManager().setPreview(std::nullopt);
        DrawSketchHandler::deactivate();
    }
    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;
        if (loops.empty()) {
            return {{tr("%1 pick closed loop"), {MouseLeft}}};
        }
        return {
            {tr("%1 add or remove closed loop"), {MouseLeft}},
            {tr("%1/%2 create hatch"), {MouseRight, KeyEnter}},
        };
    }

private:
    void activated() override
    {
        continuousMode = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher")
                             ->GetBool("ContinuousCreationMode", true);
        // Preselected edges start the hatch; each one contributes its whole loop.
        Gui::Selection().clearSelection();
        for (int geoId : preselected) {
            toggleLoop(geoId, false);
        }
        refresh();
    }
    void onWidgetChanged() override
    {
        disconnectWidget();
        if (!toolwidget) {
            return;
        }
        if (auto* pattern = toolwidget->findChild<QComboBox*>("hatchPattern")) {
            connections.push_back(QObject::connect(
                pattern,
                qOverload<int>(&QComboBox::currentIndexChanged),
                [this, pattern] {
                    settings.pattern = pattern->currentData().toString().toStdString();
                    refresh();
                }
            ));
        }
        auto connectQuantity = [this](const char* name, double Annotation::* field) {
            if (auto* box = toolwidget->findChild<Gui::QuantitySpinBox*>(name)) {
                connections.push_back(QObject::connect(
                    box,
                    qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
                    [this, field](double value) {
                        settings.*field = value;
                        refresh();
                    }
                ));
            }
        };
        connectQuantity("hatchSpacing", &Annotation::spacing);
        connectQuantity("hatchAngle", &Annotation::rotation);
        refresh();
    }
    void disconnectWidget()
    {
        for (const auto& connection : connections) {
            QObject::disconnect(connection);
        }
        connections.clear();
    }

    std::vector<long> boundary() const
    {
        std::vector<long> ids;
        for (const auto& loop : loops) {
            ids.insert(ids.end(), loop.begin(), loop.end());
        }
        return ids;
    }
    Annotation annotation() const
    {
        auto a = settings;
        a.id = -1;  // Preview only: the document allocates the real ID.
        a.boundary = boundary();
        a.construction = isConstructionMode();
        return a;
    }
    void setEdgeSelected(int geoId, bool selected)
    {
        const auto name = edgeName(geoId);
        if (selected && !sketchgui->isSelected(name)) {
            sketchgui->addSelection2(name);
        }
        else if (!selected && sketchgui->isSelected(name)) {
            sketchgui->rmvSelection(name);
        }
    }
    void setLoopSelected(const std::vector<long>& loop, bool selected)
    {
        const auto& sketch = *sketchgui->getSketchObject();
        for (long id : loop) {
            if (const int geoId = geoIdOf(sketch, id); geoId >= 0) {
                setEdgeSelected(geoId, selected);
            }
        }
    }
    void clearLoopSelection()
    {
        for (const auto& loop : loops) {
            setLoopSelected(loop, false);
        }
    }
    /// A click on a hatched loop removes it; a click on any other edge adds its loop.
    void toggleLoop(int geoId, bool allowRemoval)
    {
        auto* sketch = sketchgui->getSketchObject();
        const auto* geo = sketch->getGeometry(geoId);
        if (!geo) {
            return;
        }
        const long stable = Sketcher::GeometryFacade::getId(geo);
        for (auto it = loops.begin(); it != loops.end(); ++it) {
            if (std::find(it->begin(), it->end(), stable) != it->end()) {
                if (allowRemoval) {
                    setLoopSelected(*it, false);
                    loops.erase(it);
                    refresh();
                }
                return;
            }
        }
        const auto edges = closedLoop(*sketchgui, geoId);
        if (edges.empty()) {
            setEdgeSelected(geoId, false);
            notify(tr("This edge is not part of a closed loop"));
            return;
        }
        std::vector<long> loop;
        for (int id : edges) {
            loop.push_back(Sketcher::GeometryFacade::getId(sketch->getGeometry(id)));
        }
        auto candidate = annotation();
        candidate.boundary.insert(candidate.boundary.end(), loop.begin(), loop.end());
        try {
            sketch->annotationFace(candidate);
        }
        catch (const Base::Exception& error) {
            setEdgeSelected(geoId, false);
            notify(QString::fromStdString(error.getTranslatedMessage()));
            return;
        }
        loops.push_back(std::move(loop));
        setLoopSelected(loops.back(), true);
        refresh();
    }
    void refresh()
    {
        auto& manager = sketchgui->annotationManager();
        manager.setPreview(loops.empty() ? std::nullopt : std::optional(annotation()));
        if (toolwidget) {
            if (auto* status = toolwidget->findChild<QLabel*>("hatchStatus")) {
                const auto error = loops.empty() ? std::string() : manager.error(-1);
                if (!error.empty()) {
                    status->setText(QString::fromStdString(error));
                }
                else if (loops.empty()) {
                    status->setText(tr("No closed loop selected"));
                }
                else {
                    status->setText(tr("Selected closed loops: %1").arg(loops.size()));
                }
            }
        }
        updateHint();
    }
    void commit()
    {
        if (loops.empty()) {
            notify(tr("A hatch needs at least one closed loop"));
            return;
        }
        auto a = annotation();
        a.id = 0;
        try {
            a.validate();
            // Spacing and angle can still make the pattern invalid (too dense, say).
            sketchgui->getSketchObject()->annotationStrokes(a);
        }
        catch (const Base::Exception& error) {
            notify(QString::fromStdString(error.getTranslatedMessage()));
            return;
        }
        auto& manager = sketchgui->annotationManager();
        if (!manager.save(a, true)) {
            return;  // Refused and explained; the loops stay picked.
        }
        auto parameters = annotationParameters();
        parameters->SetASCII("HatchPattern", settings.pattern);
        parameters->SetFloat("HatchSpacing", settings.spacing);
        parameters->SetFloat("HatchAngle", settings.rotation);
        clearLoopSelection();
        loops.clear();
        manager.setPreview(std::nullopt);
        if (!continuousMode) {
            quit();  // Deletes this handler.
            return;
        }
        refresh();
    }

    std::vector<int> preselected;
    std::vector<std::vector<long>> loops;
    Annotation settings;
    bool continuousMode = true;
    std::vector<QMetaObject::Connection> connections;
};

/// Places a leader: the arrowhead first, then any number of bends. The preview shows the
/// arrowhead as it will be drawn, and Ctrl snaps each segment's angle like the line tool.
class LeaderHandler: public DrawSketchHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::LeaderHandler)

public:
    LeaderHandler()
    {
        auto parameters = annotationParameters();
        settings.kind = Annotation::Kind::Leader;
        settings.arrowStyle = parameters->GetASCII("LeaderArrowStyle", "Open arrow");
        const auto& styles = Annotation::arrowStyles();
        if (std::find(styles.begin(), styles.end(), settings.arrowStyle) == styles.end()) {
            settings.arrowStyle = "Open arrow";
        }
        settings.arrowSize = std::max(0.01, parameters->GetFloat("LeaderArrowSize", 2.0));
    }
    ~LeaderHandler() override
    {
        disconnectWidget();
    }

    std::string getToolName() const override
    {
        return "DSH_Leader";
    }
    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_CosmeticLeader.svg");
    }
    std::unique_ptr<QWidget> createWidget() const override
    {
        auto widget = std::make_unique<QWidget>();
        widget->setObjectName("sketchLeaderToolWidget");
        auto* form = new QFormLayout(widget.get());
        auto* style = new QComboBox;
        style->setObjectName("leaderArrowStyle");
        fillArrowStyleCombo(style, *sketchgui->getSketchObject(), settings.arrowStyle);
        style->setToolTip(arrowStyleToolTip());
        form->addRow(tr("Arrowhead"), style);
        auto* size = new Gui::QuantitySpinBox;
        size->setObjectName("leaderArrowSize");
        size->setUnit(Base::Unit::Length);
        size->setMinimum(0.01);
        size->setMaximum(10000);
        size->setValue(settings.arrowSize);
        size->setToolTip(arrowSizeToolTip());
        form->addRow(tr("Arrowhead size"), size);
        return widget;
    }
    bool isWidgetVisible() const override
    {
        return true;
    }
    QPixmap getToolIcon() const override
    {
        return toolIcon(Annotation::Kind::Leader, isConstructionMode());
    }
    void onConstructionModeChanged() override
    {
        setToolHeaderIcon(toolwidget, getToolIcon());
        refresh();
    }
    QString getToolWidgetText() const override
    {
        return tr("Leader Line Parameters");
    }

    void mouseMove(SnapManager::SnapHandle snap) override
    {
        cursor = snap.compute();
        hasCursor = true;
        setPositionText(cursor);
        refresh();
    }
    bool pressButton(Base::Vector2d) override
    {
        return true;
    }
    bool releaseButton(Base::Vector2d p) override
    {
        if (points.empty() || (points.back() - p).Length() > 1e-9) {
            points.push_back(p);
            // Ctrl now snaps the next segment's angle around this point.
            setAngleSnapping(true, p);
            refresh();
            updateHint();
        }
        return true;
    }
    void registerPressedKey(bool pressed, int key) override
    {
        if (pressed && (key == SoKeyboardEvent::RETURN || key == SoKeyboardEvent::PAD_ENTER)) {
            finish();
        }
        else {
            DrawSketchHandler::registerPressedKey(pressed, key);
        }
    }
    void pressRightButton(Base::Vector2d) override
    {
        if (points.size() >= 2) {
            finish();
        }
        else {
            quit();
        }
    }
    void cancelCurrentAction() override
    {
        quit();
    }
    void deactivate() override
    {
        disconnectWidget();
        sketchgui->annotationManager().setPreview(std::nullopt);
        DrawSketchHandler::deactivate();
    }
    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;
        if (points.empty()) {
            return {{tr("%1 place arrowhead"), {MouseLeft}}};
        }
        if (points.size() == 1) {
            return {
                {tr("%1 snap angle"), {ModifierCtrl}},
                {tr("%1 pick next point"), {MouseLeft}},
            };
        }
        return {
            {tr("%1 snap angle"), {ModifierCtrl}},
            {tr("%1 pick next point"), {MouseLeft}},
            {tr("%1/%2 finish"), {MouseRight, KeyEnter}},
        };
    }

private:
    void activated() override
    {
        continuousMode = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher")
                             ->GetBool("ContinuousCreationMode", true);
    }
    void onWidgetChanged() override
    {
        disconnectWidget();
        if (!toolwidget) {
            return;
        }
        if (auto* style = toolwidget->findChild<QComboBox*>("leaderArrowStyle")) {
            connections.push_back(QObject::connect(
                style,
                qOverload<int>(&QComboBox::currentIndexChanged),
                [this, style] {
                    settings.arrowStyle = style->currentData().toString().toStdString();
                    refresh();
                }
            ));
        }
        if (auto* size = toolwidget->findChild<Gui::QuantitySpinBox*>("leaderArrowSize")) {
            connections.push_back(QObject::connect(
                size,
                qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
                [this](double value) {
                    settings.arrowSize = value;
                    refresh();
                }
            ));
        }
    }
    void disconnectWidget()
    {
        for (const auto& connection : connections) {
            QObject::disconnect(connection);
        }
        connections.clear();
    }
    Annotation annotation(bool withCursor) const
    {
        auto a = settings;
        a.id = -1;  // Preview only: the document allocates the real ID.
        a.construction = isConstructionMode();
        for (const auto& p : points) {
            a.points.emplace_back(p.x, p.y, 0);
        }
        if (withCursor && hasCursor && (points.empty() || (points.back() - cursor).Length() > 1e-9)) {
            a.points.emplace_back(cursor.x, cursor.y, 0);
        }
        return a;
    }
    void refresh()
    {
        auto preview = annotation(true);
        sketchgui->annotationManager().setPreview(
            preview.points.size() >= 2 ? std::optional(preview) : std::nullopt
        );
    }
    void finish()
    {
        if (points.size() < 2) {
            notify(tr("A leader line needs at least two points"));
            return;
        }
        auto a = annotation(false);
        a.id = 0;
        try {
            a.validate();
        }
        catch (const Base::Exception& error) {
            notify(QString::fromStdString(error.getTranslatedMessage()));
            return;
        }
        auto& manager = sketchgui->annotationManager();
        if (!manager.save(a, true)) {
            return;  // Refused and explained; the placed points stay.
        }
        auto parameters = annotationParameters();
        parameters->SetASCII("LeaderArrowStyle", settings.arrowStyle);
        parameters->SetFloat("LeaderArrowSize", settings.arrowSize);
        points.clear();
        setAngleSnapping(false);
        manager.setPreview(std::nullopt);
        if (!continuousMode) {
            quit();  // Deletes this handler.
            return;
        }
        updateHint();
    }

    std::vector<Base::Vector2d> points;
    Base::Vector2d cursor;
    bool hasCursor = false;
    Annotation settings;
    bool continuousMode = true;
    std::vector<QMetaObject::Connection> connections;
};
}  // namespace

AnnotationManager::AnnotationManager(ViewProviderSketch& view)
    : view(view)
    , scene(new SoSeparator)
{
    scene->setName("SketchAnnotations");
    cameraSensor.setFunction([](void* data, SoSensor*) {
        static_cast<AnnotationManager*>(data)->cameraChanged();
    });
    cameraSensor.setData(this);
    selectionConnection = Gui::Selection().signalSelectionChanged.connect(
        [this](const Gui::SelectionChanges& message) {
            if (message.Type == Gui::SelectionChanges::MovePreselect
                || this->view.getSketchObject()->Annotations.getValues().empty()) {
                return;
            }
            // Selection in another document cannot change how these annotations draw.
            const char* document = this->view.getSketchObject()->getDocument()->getName();
            if (message.pDocName && *message.pDocName
                && std::strcmp(message.pDocName, document) != 0) {
                return;
            }
            scheduleUpdate();
        }
    );
}
AnnotationManager::~AnnotationManager()
{
    selectionConnection.disconnect();
}
SoSeparator* AnnotationManager::root() const
{
    return scene;
}
double AnnotationManager::viewportDensity()
{
    constexpr double fallbackDensity = 20;
    // The camera sensor can still fire while the document is being torn down.
    auto* document = view.getDocument();
    if (!document) {
        return fallbackDensity;
    }
    auto* active = qobject_cast<Gui::View3DInventor*>(document->getActiveView());
    if (!active) {
        return fallbackDensity;
    }
    auto* viewer = active->getViewer();
    if (cameraSensor.getAttachedNode() != viewer->getCamera()) {
        cameraSensor.attach(viewer->getCamera());
    }
    if (auto* camera = dynamic_cast<SoOrthographicCamera*>(viewer->getCamera())) {
        return viewer->getViewportRegion().getViewportSizePixels()[1]
            / std::max(1e-9F, camera->height.getValue());
    }
    return fallbackDensity;
}

bool AnnotationManager::textRasterOutOfDate()
{
    const double density = viewportDensity();
    return std::any_of(textCache.begin(), textCache.end(), [density](const auto& entry) {
        const auto& raster = entry.second;
        return raster.valid && (density > raster.density * 1.5 || density < raster.density / 2.0);
    });
}

void AnnotationManager::cameraChanged()
{
    // Text rasters are the only camera-dependent part of the scene. Rebuilding
    // everything (and repopulating the task box) on every pan and zoom is wasted work.
    if (textRasterOutOfDate()) {
        requestUpdate(false);
    }
}

void AnnotationManager::markBoundaryDependentsStale()
{
    auto* sketch = view.getSketchObject();
    bool waiting = false;
    for (auto& [id, entry] : cache) {
        const auto* annotation = sketch->findAnnotation(id);
        if (annotation && annotation->kind == Annotation::Kind::Hatch) {
            entry.stale = true;
            waiting = true;
        }
    }
    if (!waiting) {
        return;
    }
    if (!boundarySettleTimer) {
        boundarySettleTimer = new QTimer(this);
        boundarySettleTimer->setSingleShot(true);
        connect(boundarySettleTimer, &QTimer::timeout, this, [this] {
            rebuildStaleStrokes = true;
            requestUpdate(true);
        });
    }
    // Leading edge: a single edit re-clips at once, so the result never feels delayed.
    // A burst (dragging geometry) then rebuilds only once more, when it settles.
    if (!boundarySettleTimer->isActive()) {
        rebuildStaleStrokes = true;
    }
    boundarySettleTimer->start(boundarySettleDelay);
}

void AnnotationManager::scheduleUpdate(bool geometryChanged)
{
    if (geometryChanged) {
        markBoundaryDependentsStale();
    }
    if (dragging) {
        auto* sketch = view.getSketchObject();
        if (!sketch->findAnnotation(dragging)) {
            dragging = 0;
        }
    }
    requestUpdate(true);
}

void AnnotationManager::requestUpdate(bool notifyObservers)
{
    notifyPending = notifyPending || notifyObservers;
    if (updatePending) {
        return;
    }
    updatePending = true;
    QTimer::singleShot(0, this, [this] {
        updatePending = false;
        const bool notify = notifyPending;
        notifyPending = false;
        update();
        if (notify) {
            changed();
        }
    });
}
bool AnnotationManager::isVisible(long id) const
{
    const auto& hidden = view.HiddenAnnotations.getValues();
    return std::find(hidden.begin(), hidden.end(), id) == hidden.end();
}
void AnnotationManager::setVisible(const std::vector<long>& ids, bool visible)
{
    if (ids.empty()) {
        return;
    }
    auto values = view.HiddenAnnotations.getValues();
    std::set<long> hidden(values.begin(), values.end());
    for (long id : ids) {
        if (visible) {
            hidden.erase(id);
        }
        else {
            hidden.insert(id);
        }
    }
    if (std::vector<long>(hidden.begin(), hidden.end()) == values) {
        return;
    }
    std::string data;
    for (long id : hidden) {
        if (!data.empty()) {
            data += ",";
        }
        data += std::to_string(id);
    }
    auto* document = view.getObject()->getDocument();
    document->openTransaction(QT_TRANSLATE_NOOP("Command", "Change cosmetic visibility"));
    try {
        Gui::cmdAppObjectArgs(view.getObject(), "ViewObject.HiddenAnnotations = [%s]", data.c_str());
        document->commitTransaction();
    }
    // Called from list and menu slots: report, never let the error escape into Qt.
    catch (const Base::Exception& e) {
        document->abortTransaction();
        notify(QString::fromStdString(e.getTranslatedMessage()));
        return;
    }
    if (!visible) {
        for (long id : ids) {
            deselectAnnotation(view, id);
        }
        Gui::Selection().rmvPreselect();
    }
}
void AnnotationManager::setTextEditing(long id)
{
    if (textEditing == id) {
        return;
    }
    textEditing = id;
    update();
}
void AnnotationManager::setPreview(std::optional<Annotation> preview)
{
    if (editorPreview == preview) {
        return;
    }
    editorPreview = std::move(preview);
    update();
}
std::string AnnotationManager::error(long id) const
{
    auto it = cache.find(id);
    return it == cache.end() ? std::string() : it->second.error;
}
void AnnotationManager::update()
{
    scene->removeAllChildren();
    if (!view.getObject() || (!view.Visibility.getValue() && !view.isInEditMode())) {
        return;
    }
    const auto* sketch = view.getSketchObject();
    const double pixelsPerMM = viewportDensity();
    // Read the palette once rather than per annotation.
    auto viewParameters = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    Base::Color constructionColor;
    constructionColor.setPackedValue(viewParameters->GetUnsigned("ConstructionColor", 0x0000dcff));
    Base::Color selectionColor;
    selectionColor.setPackedValue(viewParameters->GetUnsigned("SelectionColor", 0x00ff00ff));
    Base::Color preselectionColor;
    preselectionColor.setPackedValue(viewParameters->GetUnsigned("HighlightColor", 0xffff00ff));
    std::set<long> live;
    auto displayed = sketch->Annotations.getValues();
    if (editorPreview) {
        std::erase_if(displayed, [this](const auto& a) { return a.id == editorPreview->id; });
        displayed.push_back(*editorPreview);
    }
    for (const auto& stored : displayed) {
        live.insert(stored.id);
        const auto& a = dragging == stored.id ? dragPreview : stored;
        if (a.id == textEditing || !isVisible(a.id) || (a.construction && !view.isInEditMode())) {
            // A hidden hatch misses the rebuild after its boundary moved; forget its strokes
            // so that showing it again cannot draw the old ones.
            if (const auto it = cache.find(a.id); it != cache.end() && it->second.stale) {
                cache.erase(it);
            }
            continue;
        }
        auto* item = new SoSeparator;
        item->setName(annotationNodeName(a.id).c_str());
        scene->addChild(item);
        const auto subname = annotationSubName(a.id);
        const bool selected = isAnnotationSelected(view, a.id);
        const auto& preselection = Gui::Selection().getPreselection();
        const bool hovered = preselection.Object.getObject() == view.getObject()
            && preselection.Object.getSubName() == subname;
        const auto color = a.construction ? constructionColor : view.LineColor.getValue();
        auto* pickStyle = new SoPickStyle;
        pickStyle->style = a.kind == Annotation::Kind::Hatch ? SoPickStyle::UNPICKABLE
                                                             : SoPickStyle::SHAPE;
        item->addChild(pickStyle);
        auto* light = new SoLightModel;
        light->model = SoLightModel::BASE_COLOR;
        item->addChild(light);
        const auto& highlight = selected ? selectionColor : preselectionColor;
        auto* material = new SoMaterial;
        material->diffuseColor.setValue(color.r, color.g, color.b);
        item->addChild(material);
        auto* depth = new SoTranslation;
        depth->translation.setValue(0, 0, a.kind == Annotation::Kind::Hatch ? 0.003F : 0.005F);
        item->addChild(depth);
        if (a.kind == Annotation::Kind::Text) {
            auto& raster = textCache[a.id];
            // Selected or hovered text is drawn in the highlight color, like geometry;
            // colors set in the rich text itself still win.
            const auto& ink = selected || hovered ? highlight : color;
            const auto packedColor = ink.getPackedValue();
            const bool contentChanged = !raster.valid || raster.html != a.html
                || raster.textSize != a.textSize || raster.textWidth != a.textWidth
                || raster.color != packedColor;
            // Moving text must not cost a re-raster; only content and
            // resolution do. The hysteresis keeps small zoom steps free.
            if (contentChanged || pixelsPerMM > raster.density * 1.5
                || pixelsPerMM < raster.density / 2.0) {
                raster.html = a.html;
                raster.textSize = a.textSize;
                raster.textWidth = a.textWidth;
                raster.color = packedColor;
                raster.valid = true;
                raster.density = pixelsPerMM;
                QTextDocument document;
                const QFont font(QStringLiteral("Sans Serif"), 12);
                document.setDefaultFont(font);
                document.setDocumentMargin(0);
                document.setDefaultStyleSheet(
                    QStringLiteral("body { color: %1; }")
                        .arg(QColor::fromRgbF(ink.r, ink.g, ink.b).name())
                );
                document.setHtml(QString::fromStdString(a.html));
                const double units = a.textSize / QFontMetricsF(font).height();
                if (a.textWidth > 0) {
                    document.setTextWidth(a.textWidth / units);
                }
                else {
                    document.adjustSize();
                }
                const QSizeF layout = document.size();
                const double scale = std::clamp(
                    pixelsPerMM * units * 1.5,
                    1e-12,
                    std::max(1e-12, 4096.0 / std::max({layout.width(), layout.height(), 1.0}))
                );
                const QSize size(
                    std::clamp(int(std::ceil(layout.width() * scale)), 1, 4096),
                    std::clamp(int(std::ceil(layout.height() * scale)), 1, 4096)
                );
                QImage image(size, QImage::Format_RGBA8888);
                image.fill(Qt::transparent);
                QPainter painter(&image);
                painter.scale(scale, scale);
                QAbstractTextDocumentLayout::PaintContext context;
                context.palette.setColor(QPalette::Text, QColor::fromRgbF(ink.r, ink.g, ink.b));
                document.documentLayout()->draw(&painter, context);
                painter.end();
                raster.image = std::move(image);
                raster.size = layout * units;
                raster.texture = nullptr;
            }
            const auto& image = raster.image;
            const auto size = image.size();
            material->diffuseColor.setValue(1, 1, 1);
            auto* translation = new SoTranslation;
            translation->translation.setValue(a.position.x, a.position.y, 0);
            item->addChild(translation);
            auto* rotation = new SoRotationXYZ;
            rotation->axis = SoRotationXYZ::Z;
            rotation->angle = a.rotation * std::acos(-1.) / 180.;
            item->addChild(rotation);
            // Coin copies the image; do that only when the raster changed, not per rebuild.
            if (!raster.texture) {
                raster.texture = new SoTexture2;
                raster.texture->image.setValue(
                    SbVec2s(size.width(), size.height()),
                    4,
                    image.constBits()
                );
            }
            item->addChild(raster.texture);
            auto* texCoords = new SoTextureCoordinate2;
            const SbVec2f uv[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
            texCoords->point.setValues(0, 4, uv);
            item->addChild(texCoords);
            auto* coords = new SoCoordinate3;
            const float w = raster.size.width(), h = raster.size.height();
            const SbVec3f vertices[] = {{0, 0, 0}, {w, 0, 0}, {w, -h, 0}, {0, -h, 0}};
            coords->point.setValues(0, 4, vertices);
            item->addChild(coords);
            auto* quad = new SoFaceSet;
            quad->numVertices = 4;
            item->addChild(quad);
            if (selected || hovered) {
                auto* noTexture = new SoTexture2;
                item->addChild(noTexture);
                auto* borderColor = new SoMaterial;
                borderColor->diffuseColor.setValue(highlight.r, highlight.g, highlight.b);
                item->addChild(borderColor);
                auto* borderStyle = new SoDrawStyle;
                borderStyle->lineWidth = 2;
                item->addChild(borderStyle);
                auto* borderCoordinates = new SoCoordinate3;
                const SbVec3f border[]
                    = {{0, 0, 0.001F}, {w, 0, 0.001F}, {w, -h, 0.001F}, {0, -h, 0.001F}, {0, 0, 0.001F}
                    };
                borderCoordinates->point.setValues(0, 5, border);
                item->addChild(borderCoordinates);
                auto* borderLine = new SoLineSet;
                borderLine->numVertices = 5;
                item->addChild(borderLine);
            }
        }
        else {
            if (selected || hovered) {
                material->diffuseColor.setValue(highlight.r, highlight.g, highlight.b);
            }
            auto& entry = cache[a.id];
            const bool sourceChanged = !entry.source || !sameStrokes(*entry.source, a);
            // A stale entry keeps drawing its previous strokes until the boundary
            // geometry settles, so dragging geometry never triggers a boolean per frame.
            if (sourceChanged || dragging == a.id || (entry.stale && rebuildStaleStrokes)) {
                entry.source = a;
                entry.stale = false;
                entry.error.clear();
                entry.fills.clear();
                try {
                    entry.strokes = sketch->annotationStrokes(a);
                    entry.fills = sketch->annotationFills(a);
                }
                catch (const Base::Exception& e) {
                    entry.strokes.clear();
                    entry.error = e.getTranslatedMessage();
                }
                catch (const std::exception& e) {
                    entry.strokes.clear();
                    entry.error = e.what();
                }
            }
            if (entry.strokes.empty()) {
                continue;
            }
            auto* style = new SoDrawStyle;
            style->lineWidth = view.LineWidth.getValue();
            item->addChild(style);
            auto* coords = new SoCoordinate3;
            std::vector<SbVec3f> vertices;
            for (const auto& p : entry.strokes) {
                vertices.emplace_back(p.x, p.y, p.z);
            }
            coords->point.setValues(0, vertices.size(), vertices.data());
            item->addChild(coords);
            auto* lines = new SoLineSet;
            std::vector<int32_t> counts(vertices.size() / 2, 2);
            lines->numVertices.setValues(0, counts.size(), counts.data());
            item->addChild(lines);
            if (entry.fills.size() >= 3) {
                auto* fillCoords = new SoCoordinate3;
                std::vector<SbVec3f> corners;
                for (const auto& p : entry.fills) {
                    corners.emplace_back(p.x, p.y, p.z);
                }
                fillCoords->point.setValues(0, corners.size(), corners.data());
                item->addChild(fillCoords);
                auto* faces = new SoFaceSet;
                std::vector<int32_t> triangles(corners.size() / 3, 3);
                faces->numVertices.setValues(0, triangles.size(), triangles.data());
                item->addChild(faces);
            }
        }
    }
    rebuildStaleStrokes = false;
    std::erase_if(textCache, [&](const auto& entry) { return !live.contains(entry.first); });
    std::erase_if(cache, [&](const auto& entry) { return !live.contains(entry.first); });
}
long AnnotationManager::picked(const SoPickedPoint* point) const
{
    if (!point) {
        return 0;
    }
    const auto* path = point->getPath();
    for (int i = 0; i < path->getLength(); ++i) {
        const std::string name = path->getNode(i)->getName().getString();
        if (name.starts_with(annotationNode)) {
            return std::strtol(name.c_str() + std::strlen(annotationNode), nullptr, 10);
        }
    }
    return 0;
}
bool AnnotationManager::mouseButton(int button, bool pressed, Base::Vector3d position, long id)
{
    if (button != 1) {
        if (dragging) {
            return cancelDrag();
        }
        return false;
    }
    if (pressed && id) {
        const auto* found = view.getSketchObject()->findAnnotation(id);
        if (!found) {
            return false;  // A pick left over from an annotation that has since gone.
        }
        const auto a = *found;
        const std::string subname = annotationSubName(id);
        const bool extend = QApplication::keyboardModifiers() & Qt::ControlModifier;
        if (extend && isAnnotationSelected(view, id)) {
            deselectAnnotation(view, id);
            return true;
        }
        if (!extend) {
            Gui::Selection().clearSelection();
        }
        view.addSelection(subname);
        if (extend) {
            return true;
        }
        if (previousClick == id && clickTimer.isValid()
            && clickTimer.elapsed() < QApplication::doubleClickInterval()) {
            previousClick = 0;
            QTimer::singleShot(0, this, [this, id] { edit(id); });
            return true;
        }
        previousClick = id;
        clickTimer.restart();
        dragging = id;
        dragOriginal = a;
        dragPreview = a;
        dragStart = position;
        dragVertex = -1;
        if (a.kind == Annotation::Kind::Leader) {
            for (size_t i = 0; i < a.points.size(); ++i) {
                if ((position - a.points[i]).Length() < a.arrowSize * 0.5) {
                    dragVertex = i;
                    break;
                }
            }
        }
        return true;
    }
    if (!pressed && dragging) {
        mouseMove(position, 0);
        const auto a = dragPreview;
        // Sub-pixel wobble while clicking is not a drag and must not create an undo step.
        const bool moved = (position - dragStart).Length() * viewportDensity() > 1.0;
        dragging = 0;
        if (moved) {
            save(a, false);
        }
        scheduleUpdate();
        return true;
    }
    return false;
}
bool AnnotationManager::mouseMove(Base::Vector3d position, long id)
{
    if (dragging) {
        dragPreview = dragOriginal;
        const auto delta = position - dragStart;
        if (dragPreview.kind == Annotation::Kind::Leader) {
            if (dragVertex >= 0) {
                dragPreview.points[dragVertex] += delta;
            }
            else {
                for (auto& p : dragPreview.points) {
                    p += delta;
                }
            }
        }
        else {
            dragPreview.position += delta;
        }
        update();
        return true;
    }
    if (id) {
        const auto world = view.getDocument()->getEditingTransform() * position;
        Gui::Selection().setPreselect(
            view.getSketchObject()->getDocument()->getName(),
            view.getObject()->getNameInDocument(),
            annotationSubName(id).c_str(),
            world.x,
            world.y,
            world.z
        );
        return true;
    }
    const auto& preselection = Gui::Selection().getPreselection();
    if (preselection.Object.getObject() == view.getObject()
        && annotationIdFromSubName(preselection.Object.getSubName())) {
        Gui::Selection().rmvPreselect();
    }
    return false;
}
bool AnnotationManager::cancelDrag()
{
    if (!dragging) {
        return false;
    }
    dragging = 0;
    scheduleUpdate();
    return true;
}
bool AnnotationManager::save(const Annotation& a, bool create)
{
    auto* doc = view.getSketchObject()->getDocument();
    doc->openTransaction(
        create ? QT_TRANSLATE_NOOP("Command", "Create cosmetic")
               : QT_TRANSLATE_NOOP("Command", "Edit cosmetic")
    );
    try {
        auto data = pythonData(a);
        if (create) {
            Gui::cmdAppObjectArgs(view.getObject(), "addAnnotation(%s)", data.c_str());
        }
        else {
            Gui::cmdAppObjectArgs(view.getObject(), "updateAnnotation(%ld, %s)", a.id, data.c_str());
        }
        doc->commitTransaction();
        return true;
    }
    catch (const Base::Exception& e) {
        doc->abortTransaction();
        notify(QString::fromStdString(e.getTranslatedMessage()));
    }
    catch (const std::exception& e) {
        doc->abortTransaction();
        notify(QString::fromUtf8(e.what()));
    }
    return false;
}
void AnnotationManager::remove(const std::vector<long>& ids)
{
    if (ids.empty()) {
        return;
    }
    auto* sketch = view.getSketchObject();
    std::vector<long> removable;
    for (long id : ids) {
        const auto* a = sketch->findAnnotation(id);
        if (!a) {
            continue;
        }
        removable.push_back(id);
    }
    if (removable.empty()) {
        return;
    }
    std::string text;
    for (long id : removable) {
        if (!text.empty()) {
            text += ",";
        }
        text += std::to_string(id);
    }
    auto* doc = view.getSketchObject()->getDocument();
    doc->openTransaction(QT_TRANSLATE_NOOP("Command", "Delete cosmetics"));
    try {
        Gui::cmdAppObjectArgs(view.getObject(), "delAnnotations([%s])", text.c_str());
        doc->commitTransaction();
    }
    // Called from key, list and menu handlers: report, never let the error escape into Qt.
    catch (const Base::Exception& e) {
        doc->abortTransaction();
        notify(QString::fromStdString(e.getTranslatedMessage()));
        return;
    }
    Gui::Selection().clearSelection();
}
void AnnotationManager::create(Annotation::Kind kind)
{
    if (kind == Annotation::Kind::Hatch) {
        view.activateHandler(std::make_unique<HatchHandler>(selectedEdges(view)));
    }
    else if (kind == Annotation::Kind::Leader) {
        view.activateHandler(std::make_unique<LeaderHandler>());
    }
    else {
        view.activateHandler(std::make_unique<TextHandler>(std::nullopt));
    }
}
void AnnotationManager::edit(long id)
{
    const auto* found = view.getSketchObject()->findAnnotation(id);
    if (!found) {
        return;
    }
    if (found->kind != Annotation::Kind::Text) {
        editInDialog(*found);
        return;
    }
    // Text is edited in the view, with its settings in the tool widget. Purging a running
    // tool can store a text being edited (possibly this one), so read it afterwards.
    if (view.getSketchMode() == ViewProviderSketch::STATUS_SKETCH_UseHandler) {
        view.purgeHandler();
    }
    if (const auto* text = view.getSketchObject()->findAnnotation(id)) {
        view.activateHandler(std::make_unique<TextHandler>(*text));
    }
}
void AnnotationManager::editInDialog(Annotation a)
{
    const Annotation original = a;
    QDialog dialog(Gui::getMainWindow());
    dialog.setObjectName("sketchAnnotationEditor");
    dialog.setWindowTitle(
        a.kind == Annotation::Kind::Hatch ? tr("Edit Hatch") : tr("Edit Leader Line")
    );
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout;
    layout->addLayout(form);
    auto* label = new QLineEdit(QString::fromStdString(a.label));
    label->setObjectName("annotationLabel");
    label->setToolTip(tr("Sets the name shown in the Cosmetics list"));
    form->addRow(tr("Name"), label);
    auto* construction = new QCheckBox(tr("Construction (edit mode only)"));
    construction->setObjectName("annotationConstruction");
    construction->setToolTip(
        tr("Shows the cosmetic only while the sketch is edited, in the construction color")
    );
    construction->setChecked(a.construction);
    form->addRow(construction);
    auto* x = quantity(form, tr("X"), a.position.x, Base::Unit::Length, -1e9, 1e9);
    auto* y = quantity(form, tr("Y"), a.position.y, Base::Unit::Length, -1e9, 1e9);
    auto* rotation = quantity(form, tr("Rotation"), a.rotation, Base::Unit::Angle, -360, 360);
    x->setToolTip(tr("Sets the X coordinate of the pattern origin"));
    y->setToolTip(tr("Sets the Y coordinate of the pattern origin"));
    Gui::QuantitySpinBox* spacing = nullptr;
    QComboBox* pattern = nullptr;
    QComboBox* arrowStyle = nullptr;
    QTableWidget* points = nullptr;
    Gui::QuantitySpinBox* arrow = nullptr;
    if (a.kind == Annotation::Kind::Hatch) {
        spacing = quantity(form, tr("Spacing"), a.spacing, Base::Unit::Length, 0.01, 100000);
        spacing->setToolTip(spacingToolTip());
        pattern = new QComboBox;
        pattern->setObjectName("annotationHatchPattern");
        fillPatternCombo(pattern, a.pattern);
        pattern->setToolTip(patternToolTip());
        rotation->setToolTip(angleToolTip());
        int row = 0;
        QFormLayout::ItemRole role {};
        form->getWidgetPosition(spacing, &row, &role);
        form->insertRow(row, tr("Pattern"), pattern);
        if (auto* angleLabel = qobject_cast<QLabel*>(form->labelForField(rotation))) {
            angleLabel->setText(tr("Angle"));
        }
    }
    else {
        x->hide();
        y->hide();
        rotation->hide();
        form->labelForField(x)->hide();
        form->labelForField(y)->hide();
        form->labelForField(rotation)->hide();
        arrowStyle = new QComboBox;
        arrowStyle->setObjectName("annotationArrowStyle");
        fillArrowStyleCombo(arrowStyle, *view.getSketchObject(), a.arrowStyle);
        arrowStyle->setToolTip(arrowStyleToolTip());
        form->addRow(tr("Arrowhead"), arrowStyle);
        arrow = quantity(form, tr("Arrowhead size"), a.arrowSize, Base::Unit::Length, 0.01, 10000);
        arrow->setToolTip(arrowSizeToolTip());
        points = new QTableWidget(a.points.size(), 2);
        points->setObjectName("annotationLeaderPoints");
        points->setToolTip(
            tr("Lists the points of the leader line; the arrowhead is at the first point. "
               "Coordinates accept lengths with units, such as 0.5 in.")
        );
        points->setHorizontalHeaderLabels({tr("X"), tr("Y")});
        points->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        for (int i = 0; i < points->rowCount(); ++i) {
            points->setItem(i, 0, new QTableWidgetItem(QString::number(a.points[i].x, 'g', 15)));
            points->setItem(i, 1, new QTableWidgetItem(QString::number(a.points[i].y, 'g', 15)));
        }
        layout->addWidget(points);
        auto* add = new QPushButton(tr("Add Point"));
        add->setToolTip(tr("Adds a point at the end of the leader line"));
        layout->addWidget(add);
        connect(add, &QPushButton::clicked, &dialog, [points] {
            const int row = points->rowCount();
            points->insertRow(row);
            for (int col = 0; col < 2; ++col) {
                points->setItem(row, col, new QTableWidgetItem("0"));
            }
        });
        auto* remove = new QPushButton(tr("Remove Point"));
        remove->setToolTip(
            tr("Removes the selected point. A leader line keeps at least two points.")
        );
        layout->addWidget(remove);
        connect(remove, &QPushButton::clicked, &dialog, [points] {
            if (points->rowCount() > 2 && points->currentRow() >= 0) {
                points->removeRow(points->currentRow());
            }
        });
    }
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        // An empty name would leave a blank row in the Cosmetics list.
        if (const auto name = label->text().trimmed(); !name.isEmpty()) {
            a.label = name.toStdString();
        }
        a.construction = construction->isChecked();
        a.position = Base::Vector3d(x->rawValue(), y->rawValue(), 0);
        a.rotation = rotation->rawValue();
        if (spacing) {
            a.spacing = spacing->rawValue();
            a.pattern = pattern->currentData().toString().toStdString();
        }
        try {
            if (points) {
                a.arrowSize = arrow->rawValue();
                a.arrowStyle = arrowStyle->currentData().toString().toStdString();
                a.points.clear();
                for (int i = 0; i < points->rowCount(); ++i) {
                    const auto* itemX = points->item(i, 0);
                    const auto* itemY = points->item(i, 1);
                    double px = 0;
                    double py = 0;
                    if (!itemX || !itemY || !parseLength(itemX->text(), px)
                        || !parseLength(itemY->text(), py)) {
                        QMessageBox::warning(
                            &dialog,
                            tr("Edit Leader Line"),
                            tr("The point coordinates must be lengths")
                        );
                        return;
                    }
                    a.points.emplace_back(px, py, 0);
                }
            }
            a.validate();
            if (a.kind == Annotation::Kind::Hatch) {
                // Refuse a spacing too dense to draw here, not after the dialog closed.
                view.getSketchObject()->annotationStrokes(a);
            }
            dialog.accept();
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(
                &dialog,
                dialog.windowTitle(),
                QString::fromStdString(e.getTranslatedMessage())
            );
        }
        catch (const std::exception& e) {
            QMessageBox::warning(&dialog, dialog.windowTitle(), QString::fromUtf8(e.what()));
        }
    });
    // Preview is scene-only: cancel never mutates the document or consumes an ID.
    auto preview = [&] {
        auto candidate = a;
        candidate.construction = construction->isChecked();
        candidate.position = Base::Vector3d(x->rawValue(), y->rawValue(), 0);
        candidate.rotation = rotation->rawValue();
        if (spacing) {
            candidate.spacing = spacing->rawValue();
            candidate.pattern = pattern->currentData().toString().toStdString();
        }
        if (points) {
            candidate.arrowSize = arrow->rawValue();
            candidate.arrowStyle = arrowStyle->currentData().toString().toStdString();
            candidate.points.clear();
            for (int i = 0; i < points->rowCount(); ++i) {
                const auto* itemX = points->item(i, 0);
                const auto* itemY = points->item(i, 1);
                double px = 0;
                double py = 0;
                if (!itemX || !itemY || !parseLength(itemX->text(), px)
                    || !parseLength(itemY->text(), py)) {
                    return;
                }
                candidate.points.emplace_back(px, py, 0);
            }
        }
        try {
            candidate.validate();
        }
        catch (const Base::Exception&) {
            return;
        }
        catch (const std::exception&) {
            return;
        }
        editorPreview = candidate;
        update();
    };
    for (auto* control : {x, y, rotation, spacing, arrow}) {
        if (control) {
            connect(
                control,
                qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
                &dialog,
                preview
            );
        }
    }
    connect(construction, &QCheckBox::toggled, &dialog, preview);
    for (auto* combo : {pattern, arrowStyle}) {
        if (combo) {
            connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, preview);
        }
    }
    if (points) {
        connect(points, &QTableWidget::itemChanged, &dialog, preview);
        connect(points->model(), &QAbstractItemModel::rowsRemoved, &dialog, preview);
    }
    preview();
    dialog.resize(480, 480);
    const bool accepted = dialog.exec() == QDialog::Accepted;
    editorPreview.reset();
    scheduleUpdate();
    // OK without a change must not add an undo step.
    if (accepted && !(a == original)) {
        save(a, false);
    }
}

TaskSketcherAnnotations::TaskSketcherAnnotations(ViewProviderSketch* view)
    : TaskBox(Gui::BitmapFactory().pixmap("Sketcher_CosmeticLeader"), tr("Cosmetics"), true, nullptr)
    , view(view)
{
    setObjectName("sketchCosmeticsTaskBox");
    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    auto* controls = new QHBoxLayout;
    filterEnabled = new QCheckBox;
    filterEnabled->setObjectName("cosmeticFilterEnabled");
    filterEnabled->setToolTip(tr("Enables the type filter of the list"));
    controls->addWidget(filterEnabled);
    filterButton = new QToolButton;
    filterButton->setObjectName("cosmeticFilterButton");
    filterButton->setText(tr("Filter"));
    filterButton->setToolTip(
        tr("Chooses the types the list shows. The filter does not change what the view shows.")
    );
    filterButton->setPopupMode(QToolButton::InstantPopup);
    filterButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    filterButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    filterButton->setEnabled(false);
    auto* filters = new QMenu(filterButton);
    filterButton->setMenu(filters);
    connect(filters, &QMenu::aboutToShow, this, &TaskSketcherAnnotations::populateFilters);
    controls->addWidget(filterButton);
    layout->addLayout(controls);
    list = new QListWidget;
    list->setObjectName("sketchCosmetics");
    list->setIconSize(QSize(24, 24));
    list->setMinimumHeight(70);
    list->setMaximumHeight(200);
    list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(list);
    groupLayout()->addWidget(container);
    list->installEventFilter(this);
    list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(filterEnabled, &QCheckBox::toggled, this, [this](bool checked) {
        filterButton->setEnabled(checked);
        updateFilters();
    });
    connect(list, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        const long id = item->data(Qt::UserRole).toLongLong();
        const bool visible = item->checkState() == Qt::Checked;
        if (visible != this->view->annotationManager().isVisible(id)) {
            this->view->annotationManager().setVisible({id}, visible);
        }
    });
    connect(list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        this->view->annotationManager().edit(item->data(Qt::UserRole).toLongLong());
    });
    connect(list, &QListWidget::itemSelectionChanged, this, [this] {
        if (!(QApplication::keyboardModifiers() & Qt::ControlModifier)) {
            Gui::Selection().clearSelection();
        }
        else {
            for (const auto& a : this->view->getSketchObject()->Annotations.getValues()) {
                deselectAnnotation(*this->view, a.id);
            }
        }
        for (auto* item : list->selectedItems()) {
            this->view->addSelection(annotationSubName(item->data(Qt::UserRole).toLongLong()));
        }
    });
    connect(list, &QListWidget::customContextMenuRequested, this, [this](QPoint p) {
        QMenu menu;
        if (auto* item = list->itemAt(p)) {
            if (!item->isSelected()) {
                list->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
            }
            auto* sketch = this->view->getSketchObject();
            const long id = item->data(Qt::UserRole).toLongLong();
            const auto* current = sketch->findAnnotation(id);
            auto* edit = menu.addAction(tr("Edit"), this, [this, id] {
                this->view->annotationManager().edit(id);
            });
            edit->setEnabled(current != nullptr);
            std::vector<long> selected;
            for (auto* row : list->selectedItems()) {
                const long selectedId = row->data(Qt::UserRole).toLongLong();
                const auto* a = sketch->findAnnotation(selectedId);
                if (!a) {
                    continue;
                }
                selected.push_back(selectedId);
            }
            menu.addAction(
                    tr("Delete"),
                    this,
                    [this, selected] { this->view->annotationManager().remove(selected); }
            );
            menu.addSeparator();
        }
        std::vector<long> all;
        for (const auto& annotation : this->view->getSketchObject()->Annotations.getValues()) {
            all.push_back(annotation.id);
        }
        menu.addAction(
                tr("Show All"),
                this,
                [this, all] { this->view->annotationManager().setVisible(all, true); }
        )->setEnabled(!all.empty());
        menu.addAction(
                tr("Hide All"),
                this,
                [this, all] { this->view->annotationManager().setVisible(all, false); }
        )->setEnabled(!all.empty());
        menu.exec(list->viewport()->mapToGlobal(p));
    });
    connection = view->annotationManager().changed.connect([this] { refresh(); });
    refresh();
}
TaskSketcherAnnotations::~TaskSketcherAnnotations()
{
    connection.disconnect();
}
void TaskSketcherAnnotations::populateFilters()
{
    auto* menu = filterButton->menu();
    menu->clear();
    menu->addSection(tr("Type"));
    for (auto kind : {Annotation::Kind::Text, Annotation::Kind::Hatch, Annotation::Kind::Leader}) {
        auto* action
            = menu->addAction(Gui::BitmapFactory().iconFromTheme(typeIcon(kind)), typeName(kind));
        action->setCheckable(true);
        action->setChecked(!excludedTypes.contains(kind));
        action->setObjectName("cosmeticTypeFilter" + QString::number(static_cast<int>(kind)));
        connect(action, &QAction::toggled, this, [this, kind](bool checked) {
            if (checked) {
                excludedTypes.erase(kind);
            }
            else {
                excludedTypes.insert(kind);
            }
            updateFilters();
        });
    }
}
void TaskSketcherAnnotations::updateFilters()
{
    for (int i = 0; i < list->count(); ++i) {
        auto* row = list->item(i);
        const auto* a
            = view->getSketchObject()->findAnnotation(row->data(Qt::UserRole).toLongLong());
        if (!a) {
            continue;  // The list is one refresh behind the model.
        }
        const bool filtered = filterEnabled->isChecked() && excludedTypes.contains(a->kind);
        row->setHidden(filtered);
    }
}

void TaskSketcherAnnotations::refresh()
{
    // Do not show an unparented taskbox as a separate window during construction.
    if (parentWidget() || !view->areCosmeticsEnabled()) {
        setVisible(view->areCosmeticsEnabled());
    }
    QSignalBlocker block(list);
    const auto& values = view->getSketchObject()->Annotations.getValues();
    bool rebuild = list->count() != static_cast<int>(values.size());
    if (!rebuild) {
        for (int i = 0; i < list->count(); ++i) {
            rebuild |= list->item(i)->data(Qt::UserRole).toLongLong() != values[i].id;
        }
    }
    if (rebuild) {
        list->clear();
    }
    for (size_t i = 0; i < values.size(); ++i) {
        const auto& a = values[i];
        auto* row = rebuild ? new QListWidgetItem(list) : list->item(i);
        row->setText(QString::fromStdString(a.label));
        if (rebuild) {
            row->setIcon(Gui::BitmapFactory().iconFromTheme(typeIcon(a.kind)));
        }
        row->setData(Qt::UserRole, qlonglong(a.id));
        row->setFlags(row->flags() | Qt::ItemIsUserCheckable);
        row->setCheckState(view->annotationManager().isVisible(a.id) ? Qt::Checked : Qt::Unchecked);
        row->setSelected(
            isAnnotationSelected(*view, a.id)
        );
        const auto error = view->annotationManager().error(a.id);
        QString tooltip = typeName(a.kind);
        if (a.construction) {
            tooltip += QStringLiteral("\n") + tr("Construction: shown only while the sketch is edited");
        }
        tooltip += QStringLiteral("\n") + tr("The checkbox shows or hides the cosmetic");
        if (!error.empty()) {
            tooltip += "\n" + QString::fromStdString(error);
        }
        row->setToolTip(tooltip);
        row->setForeground(error.empty() ? QBrush() : QBrush(Qt::red));
    }
    updateFilters();
    list->doItemsLayout();
}
bool TaskSketcherAnnotations::eventFilter(QObject* object, QEvent* event)
{
    if (object == list
        && (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)) {
        const auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Delete) {
            event->accept();
            if (event->type() == QEvent::KeyPress) {
                std::vector<long> ids;
                for (auto* item : list->selectedItems()) {
                    ids.push_back(item->data(Qt::UserRole).toLongLong());
                }
                view->annotationManager().remove(ids);
            }
            return true;
        }
    }
    return TaskBox::eventFilter(object, event);
}

namespace
{
class AnnotationCommand: public Gui::Command
{
public:
    AnnotationCommand(const char* name, Annotation::Kind kind, const char* label, const char* tip)
        : Command(name)
        , kind(kind)
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = label;
        sToolTipText = tip;
        sWhatsThis = name;
        sStatusTip = tip;
        sPixmap = typeIcon(kind);
        eType = ForEdit;
    }
    const char* className() const override
    {
        return "SketcherAnnotationCommand";
    }
    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
    void activated(int) override
    {
        auto* vp = dynamic_cast<ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
        if (vp) {
            vp->annotationManager().create(kind);
        }
    }
    /// Follows the construction toggle, like the geometry creation commands.
    void updateAction(int mode) override
    {
        auto* action = getAction();
        if (!action) {
            return;
        }
        const bool construction
            = static_cast<GeometryCreationMode>(mode) == GeometryCreationMode::Construction;
        action->setIcon(Gui::BitmapFactory().iconFromTheme(
            (std::string(typeIcon(kind)) + (construction ? "_Constr" : "")).c_str()
        ));
    }

private:
    Annotation::Kind kind;
};
}  // namespace
void SketcherGui::CreateSketcherAnnotationCommands()
{
    auto& manager = Gui::Application::Instance->commandManager();
    // Command texts are translated in the context of className().
    manager.addCommand(new AnnotationCommand(
        "Sketcher_AnnotationText",
        Annotation::Kind::Text,
        QT_TRANSLATE_NOOP("SketcherAnnotationCommand", "Cosmetic Text"),
        QT_TRANSLATE_NOOP(
            "SketcherAnnotationCommand",
            "Creates a rich text note, typed and formatted directly in the 3D view. Cosmetics "
            "annotate the sketch without adding geometry or constraints."
        )
    ));
    manager.addCommand(new AnnotationCommand(
        "Sketcher_AnnotationHatch",
        Annotation::Kind::Hatch,
        QT_TRANSLATE_NOOP("SketcherAnnotationCommand", "Hatch"),
        QT_TRANSLATE_NOOP(
            "SketcherAnnotationCommand",
            "Creates a hatch inside closed loops of the sketch. Nested loops become holes, and the "
            "hatch follows the geometry when it changes."
        )
    ));
    manager.addCommand(new AnnotationCommand(
        "Sketcher_AnnotationLeader",
        Annotation::Kind::Leader,
        QT_TRANSLATE_NOOP("SketcherAnnotationCommand", "Leader Line"),
        QT_TRANSLATE_NOOP(
            "SketcherAnnotationCommand",
            "Creates a leader line from an arrowhead through any number of points"
        )
    ));
    for (const char* name :
         {"Sketcher_AnnotationText", "Sketcher_AnnotationHatch", "Sketcher_AnnotationLeader"}) {
        manager.addCommandMode("ToggleConstruction", name);
    }
}

void AnnotationManager::appendContextMenu(QMenu* menu)
{
    std::vector<long> selected;
    for (const auto& selection :
         Gui::Selection().getSelectionEx(view.getObject()->getDocument()->getName())) {
        if (selection.getObject() != view.getObject()) {
            continue;
        }
        for (const auto& name : selection.getSubNames()) {
            const long id = annotationIdFromSubName(name);
            if (id && view.getSketchObject()->findAnnotation(id)) {
                selected.push_back(id);
            }
        }
    }
    if (selected.empty()) {
        return;
    }
    menu->addSeparator();
    if (selected.size() == 1) {
        menu->addAction(tr("Edit Cosmetic"), this, [this, id = selected.front()] { edit(id); });
    }
    menu->addAction(tr("Delete Cosmetics"), this, [this, selected] { remove(selected); });
}
