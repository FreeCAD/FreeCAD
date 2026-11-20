// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <optional>
#include <string>
#include <string_view>
#include <QCoreApplication>
#include <QObject>
#include <QElapsedTimer>
#include <QImage>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Gui/ViewProvider.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Sketcher/App/Annotation.h>
#include <fastsignals/signal.h>

class SoSeparator;
class SoSwitch;
class SoPickedPoint;
class QListWidget;
class QCheckBox;
class QToolButton;
class QMenu;
class QTimer;
namespace SketcherGui
{
class ViewProviderSketch;
using AnnotationId = Sketcher::Annotation::Id;
/// The ID a preview of an annotation not yet created is drawn with.
constexpr AnnotationId PreviewAnnotationId = -1;
/// Parses an "Annotation<id>" subelement name. Returns Annotation::NoId for anything else,
/// including names that merely start with the prefix, so stale or foreign names are ignored.
AnnotationId annotationIdFromSubName(std::string_view name);
/// The stable subelement name for an annotation ID.
std::string annotationSubName(AnnotationId id);
/// The Coin node name the manager gives an annotation's scene subtree.
std::string annotationNodeName(AnnotationId id);
class AnnotationManager: public QObject
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::AnnotationManager)

public:
    explicit AnnotationManager(ViewProviderSketch& view);
    ~AnnotationManager() override;
    SoSeparator* root() const;
    /// Annotation data or visibility changed: refresh scene and task box.
    void scheduleUpdate(bool geometryChanged = false);
    /// The camera moved: only re-raster text whose resolution no longer fits.
    void cameraChanged();
    void update();
    AnnotationId picked(const SoPickedPoint* point) const;
    bool mouseButton(int button, bool pressed, Base::Vector3d position, AnnotationId pickedId);
    bool mouseMove(Base::Vector3d position, AnnotationId pickedId);
    bool cancelDrag();
    void create(Sketcher::Annotation::Kind kind);
    /// Text opens the in-view text tool; hatches and leaders open the editor dialog.
    void edit(AnnotationId id);
    void remove(const std::vector<AnnotationId>& ids);
    void appendContextMenu(QMenu* menu);
    /// Adds or updates the annotation in one undo step. Returns false, having told the
    /// user why, when it was refused (invalid data).
    bool save(const Sketcher::Annotation& annotation, bool create);
    /// Draws a not yet created annotation (PreviewAnnotationId for a new one) until reset.
    /// Scene only: the document is untouched; error(PreviewAnnotationId) reports why a new
    /// preview cannot be drawn.
    void setPreview(std::optional<Sketcher::Annotation> preview);
    /// Hides a text while it is edited in the view (Annotation::NoId: none).
    void setTextEditing(AnnotationId id);
    std::string error(AnnotationId id) const;
    bool isVisible(AnnotationId id) const;
    void setVisible(const std::vector<AnnotationId>& ids, bool visible);
    fastsignals::signal<void()> changed;

private:
    void editInDialog(Sketcher::Annotation annotation);
    void requestUpdate(bool notifyObservers);
    /// Pixels per sketch millimetre in the active view; also (re)binds the camera sensor.
    double viewportDensity();
    bool textRasterOutOfDate();
    void markBoundaryDependentsStale();

    ViewProviderSketch& view;
    Gui::CoinPtr<SoSeparator> scene;
    struct Cache
    {
        std::vector<Base::Vector3d> strokes;
        /// Filled triangles, such as a filled arrowhead.
        std::vector<Base::Vector3d> fills;
        // Cheap value identity beats serialising through Python on every update.
        std::optional<Sketcher::Annotation> source;
        std::string error;
        // Boundary geometry moved; keep drawing these strokes until it settles.
        bool stale = false;
    };
    struct TextCache
    {
        QImage image;
        QSizeF size;
        // Only the fields the raster actually depends on, so moving text never re-renders.
        std::string html;
        double textSize = 0;
        double textWidth = 0;
        std::uint32_t color = 0;
        bool valid = false;
        double density = 0;
        // Reused across scene rebuilds, so an unchanged raster is not copied to Coin again.
        Gui::CoinPtr<SoTexture2> texture;
    };
    std::map<AnnotationId, TextCache> textCache;
    SoNodeSensor cameraSensor;
    fastsignals::connection selectionConnection;
    std::map<AnnotationId, Cache> cache;
    bool updatePending = false;
    bool notifyPending = false;
    bool rebuildStaleStrokes = false;
    QTimer* boundarySettleTimer = nullptr;
    std::optional<Sketcher::Annotation> editorPreview;
    AnnotationId textEditing = Sketcher::Annotation::NoId;
    AnnotationId dragging = Sketcher::Annotation::NoId;
    int dragVertex = -1;
    Base::Vector3d dragStart;
    Sketcher::Annotation dragOriginal;
    Sketcher::Annotation dragPreview;
    QElapsedTimer clickTimer;
    AnnotationId previousClick = Sketcher::Annotation::NoId;
};

class TaskSketcherAnnotations: public Gui::TaskView::TaskBox
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::TaskSketcherAnnotations)

public:
    explicit TaskSketcherAnnotations(ViewProviderSketch* view);
    ~TaskSketcherAnnotations() override;

private:
    void refresh();
    bool eventFilter(QObject*, QEvent*) override;
    ViewProviderSketch* view;
    QListWidget* list;
    QCheckBox* filterEnabled;
    QToolButton* filterButton;
    std::set<Sketcher::Annotation::Kind> excludedTypes;
    void updateFilters();
    void populateFilters();
    fastsignals::connection connection;
};
void CreateSketcherAnnotationCommands();
}  // namespace SketcherGui
