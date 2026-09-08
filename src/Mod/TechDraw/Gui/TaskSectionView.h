/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>
#include <QMetaObject>
#include <QPointer>
#include <QPointF>
#include <QVariant>
#include <memory>
#include <utility>
#include <vector>

class Ui_TaskSectionView;
class QGraphicsEllipseItem;
class QGraphicsItem;
class QGraphicsScene;

namespace TechDraw {
    class DrawPage;
    class DrawViewPart;
    class DrawViewSection;
}

namespace App {
    class DocumentObject;
}

namespace TechDrawGui
{

class CompassWidget;
class QGISectionLine;
class QGIViewPart;
class QGVPage;
class VectorEditWidget;

// QGraphicsScene owns every item added to it. This deleter lets task-local
// handles remove their items early, while becoming a no-op if the scene has
// already destroyed them.
class SceneItemDeleter
{
public:
    SceneItemDeleter() = default;
    explicit SceneItemDeleter(QGraphicsScene* scene);
    void operator()(QGraphicsItem* item) const;

private:
    QPointer<QGraphicsScene> m_scene;
};

template<typename T>
using SceneItemPtr = std::unique_ptr<T, SceneItemDeleter>;

class TaskSectionView : public QWidget
{
    Q_OBJECT

public:
    enum class PositionMode
    {
        Horizontal,
        Vertical,
        CenterAndViewDirection,
        CenterAndPoint,
        CenterAndTwoPoints,
        TwoPoints,
        TwoPointsPartial,
        SketchBased
    };

    TaskSectionView(TechDraw::DrawPage* page, QGVPage* graphicsView);
    explicit TaskSectionView(TechDraw::DrawViewPart* base);
    explicit TaskSectionView(TechDraw::DrawViewSection* section);
    ~TaskSectionView() override;

    virtual bool accept();
    virtual bool reject();
    bool apply(bool forceUpdate = false);
    void setInteractiveBase(TechDraw::DrawViewPart* base);
    void setInteractiveParameters(const Base::Vector3d& origin,
                                  const Base::Vector3d& direction);
    void setDirectionControlsVisible(bool visible);
    void setSectionControlsEnabled(bool enabled);
    void adoptTemporarySectionLine(QGISectionLine* line,
                                   QGIViewPart* baseItem);
    void adoptSketchBasedBase(QGISectionLine* line,
                              QGIViewPart* baseItem);
    void resetInteractivePreview();
    void updateTemporarySectionLine(const QPointF& center,
                                    const Base::Vector3d& displayedDirection);
    void updateTemporarySectionLineFromEndpoints(
        const QPointF& first,
        const QPointF& second,
        const Base::Vector3d& displayedDirection);
    void updateTemporarySectionLineEndpoint(
        const QPointF& center,
        const QPointF& endpoint,
        const Base::Vector3d& displayedDirection,
        bool startEndpoint);
    void updateTemporaryHalfSection(const QPointF& center,
                                    const QPointF& endpoint,
                                    const Base::Vector3d& displayedDirection);
    void updateTemporaryRotatedSectionEndpoint(
        const QPointF& endpoint,
        bool startEndpoint);
    void setTemporaryHalfRotations(double startRotation,
                                   double endRotation);
    std::pair<double, double> projectOffsetStart(const QPointF& point) const;
    void startSingleOffset(double along,
                           double minimumAlong,
                           double maximumAlong,
                           bool towardStart);
    void startNotchOffset(double along,
                          double minimumAlong,
                          double maximumAlong,
                          bool towardStart);
    void startArcOffset(double along,
                        double minimumAlong,
                        double maximumAlong,
                        bool towardStart);
    void startArcNotchOffset(double along,
                             double minimumAlong,
                             double maximumAlong,
                             bool towardStart);
    void beginSingleOffset(double along,
                           double currentOffset,
                           bool towardStart);
    void beginArcOffset(double along, bool towardStart);
    void beginArcNotch(double along, bool towardStart);
    void updatePendingSingleOffset(double offset);
    void updatePendingNotchOuterAlong(double along);
    void updatePendingArcAngle(double angle);
    void commitPendingArc();
    void cancelPendingArc();
    void updatePendingArcNotch(double angle, double secondAlong);
    void commitPendingArcNotch();
    void cancelPendingArcNotch();
    void commitPendingSingleOffset();
    void cancelPendingSingleOffset();
    QPointF snapViewPoint(const QPointF& point, bool disableSnapping) const;
    QPointF mapFromSectionHalf(const QPointF& point, bool startHalf) const;
    QPointF mapToSectionHalf(const QPointF& point, bool startHalf) const;
    QPointF sectionHalfEndpoint(bool startHalf) const;
    void showSectionHandles();
    void hideSectionHandles();
    QPointF temporaryCenter() const { return m_temporaryCenter; }
    Base::Vector3d displayedDirection() const { return m_displayedDirection; }
    double temporaryStartAlong() const { return m_temporaryStartAlong; }
    double temporaryEndAlong() const { return m_temporaryEndAlong; }
    QGIViewPart* baseItem() const { return m_baseItem; }
    bool hasBase() const { return m_base != nullptr; }
    PositionMode positionMode() const;
    void setPositioningActive(bool active);
    void initializeEditPreview(QGVPage* graphicsView);

Q_SIGNALS:
    void baseViewChanged(bool valid);
    void restartPositioningRequested();

protected:
    void changeEvent(QEvent *event) override;

    void applyQuick(std::string dir);
    void applyAligned();

    TechDraw::DrawViewSection* createSectionView();
    void updateSectionView();
    void createOffsetProfile();
    void updateOffsetProfile();
    std::string serializeModernProfileState() const;
    bool restoreModernProfileState();
    void convertSectionToComplex();

    void setUiPrimary();
    void setUiEdit();
    void setUiCommon(const QPointF& center);

    void enableAll(bool enable);

    void failNoObject();
    bool isBaseValid();
    bool isSectionValid();

protected Q_SLOTS:
    void onIdentifierChanged();
    void onScaleChanged();
    void onCenterXChanged();
    void onCenterYChanged();
    void scaleTypeChanged(int index);
    void liveUpdateClicked();
    void slotChangeAngle(double newAngle);
    void reverseSectionDirection(double newAngle);
    void slotViewDirectionChanged(Base::Vector3d newDirection);
    void onSectionObjectsUseSelectionClicked();
    void onProfileObjectUseSelectionClicked();
    void onProjectionStrategyChanged(int index);
    void onParallelToChanged(int index);
    void onSectionLinePositionChanged(int index);
    void onResetSectionLineClicked();
    void onShowManualControlsToggled(bool checked);

private:
    double requiredRotation(double inputAngle) const;
    double sectionViewRotation() const;
    std::string makeSectionLabel(QString symbol);
    QString makeSectionCaption(const QString& symbol) const;
    bool directionChanged() const { return m_directionChanged; }
    void directionChanged(bool newState) { m_directionChanged = newState; }
    bool hasComplexPath() const;
    bool hasAlignedPath() const;
    bool hasBentAxis() const;
    void updateParallelToVisibility();
    void updatePartialDisplayVisibility();
    bool isGeneratedProfile(const App::DocumentObject* profile) const;
    void setCustomComplexSection(bool enabled);
    void setProfileObject(App::DocumentObject* profile);
    QString sourcesToString() const;
    void updateCustomProfilePreview();
    void configureTemporarySectionLine();
    void setBaseGraphicsItem(QGIViewPart* baseItem);
    void updateInteractiveOverlayPosition();
    void updateManualControlsVisibility();
    void updateTemporarySectionLineFromUi();
    void setGeneratedSectionVisible(bool visible);
    void showGeneratedSectionWhenReady(int request);
    void showUnappliedPreview(bool updateFromUi = true);
    void drawTemporarySingleOffset();
    std::vector<std::pair<QPointF, double>>
        sectionPathPoints(bool includePending) const;
    std::pair<bool, bool> partialEndpointStatus(
        const std::vector<std::pair<QPointF, double>>& pathPoints) const;
    QPointF rotateSectionPoint(const QPointF& point, double along) const;
    void ensureSectionHandles();
    void updateSectionHandles();
    void rebuildSelectableOffsetEdges();
    QPointF halfSectionLeaderEnd() const;
    void makeHalfSection(bool retainStartHalf);
    void clearHalfSection();
    void removeOffsetStep(size_t index);
    void removeArcBend(size_t index);
    void ensureSectionControls();
    void updateSectionControls();
    void clearSectionControls();
    void moveSectionCenter(const QPointF& scenePoint,
                           Qt::KeyboardModifiers modifiers);
    void moveSingleOffsetP1(size_t index,
                            const QPointF& scenePoint,
                            Qt::KeyboardModifiers modifiers);
    void moveSingleOffsetP2(size_t index,
                            const QPointF& scenePoint,
                            Qt::KeyboardModifiers modifiers);
    void moveNotchP1(size_t index,
                     const QPointF& scenePoint,
                     Qt::KeyboardModifiers modifiers);
    void moveNotchP2(size_t index,
                     const QPointF& scenePoint,
                     Qt::KeyboardModifiers modifiers);
    void moveArcBendPoint(size_t index,
                          bool endpoint,
                          const QPointF& scenePoint,
                          Qt::KeyboardModifiers modifiers);
    void rotateSectionHalf(bool startHalf,
                           const QPointF& scenePoint,
                           Qt::KeyboardModifiers modifiers,
                           bool commit);
    void moveSectionEndpoint(bool startEndpoint,
                             const QPointF& scenePoint,
                             Qt::KeyboardModifiers modifiers);
    Base::Vector3d viewPointToSectionOrigin(const QPointF& point) const;
    QPointF sectionOriginToView(const Base::Vector3d& origin) const;
    QPointF centerFromUi() const;
    void setCenterUi(const QPointF& center);
    Base::Vector3d baseToDisplayedDirection(const Base::Vector3d& direction) const;

    std::unique_ptr<Ui_TaskSectionView> ui;
    TechDraw::DrawViewPart* m_base;
    TechDraw::DrawViewSection* m_section;

    std::string m_dirName;
    std::string m_sectionName;
    std::string m_baseName;
    App::Document* m_doc;
    TechDraw::DrawPage* m_page;
    QGVPage* m_graphicsView;
    int m_transactionId{0};

    bool m_createMode;

    std::string m_saveBaseName;
    std::string m_savePageName;

    CompassWidget* m_compass;
    VectorEditWidget* m_viewDirectionWidget;

    bool m_scaleEdited;
    bool m_directionChanged{false};
    SceneItemPtr<QGISectionLine> m_temporarySectionLine;
    QGIViewPart* m_baseItem{nullptr};
    QMetaObject::Connection m_baseItemPositionConnection;
    double m_temporaryStartAlong{0.0};
    double m_temporaryEndAlong{0.0};
    bool m_endpointsEdited{false};
    bool m_startEndpointEdited{false};
    bool m_endEndpointEdited{false};
    bool m_halfSection{false};
    bool m_halfSectionTowardStart{false};
    int m_generationRequest{0};
    QPointF m_temporaryCenter;
    Base::Vector3d m_displayedDirection{1.0, 0.0, 0.0};
    struct OffsetStep
    {
        enum class Transition
        {
            Sharp,
            Arc
        };
        enum class Role
        {
            Generic,
            NotchAnchor,
            NotchOuter
        };

        double along{0.0};
        double offset{0.0};
        Transition transition{Transition::Sharp};
        double startAlong{0.0};
        Role role{Role::Generic};
        int modifierId{0};
        bool towardStart{false};
    };
    enum class PendingOffsetKind
    {
        Single,
        Notch,
        Arc
    };
    std::vector<OffsetStep> m_offsetSteps;
    struct ArcBend
    {
        double along{0.0};
        double angle{0.0};
        bool towardStart{false};
    };
    std::vector<ArcBend> m_arcBends;
    bool m_hasPendingArc{false};
    ArcBend m_pendingArc;
    bool m_hasPendingArcNotch{false};
    ArcBend m_pendingArcNotchFirst;
    ArcBend m_pendingArcNotchSecond;
    double m_startOffset{0.0};
    bool m_hasPendingOffset{false};
    OffsetStep m_pendingOffset;
    double m_pendingBaseOffset{0.0};
    bool m_pendingTowardStart{false};
    PendingOffsetKind m_pendingOffsetKind{PendingOffsetKind::Single};
    double m_pendingMinimumAlong{0.0};
    double m_pendingMaximumAlong{0.0};
    double m_pendingNotchOuterAlong{0.0};
    int m_nextModifierId{1};
    double m_startRotation{0.0};
    double m_endRotation{0.0};
    bool m_sectionControlsEnabled{false};
    std::string m_profileName;
    std::string m_captionForConversion;
    App::DocumentObject* m_profileObject{nullptr};
    App::DocumentObject* m_profileBeforeSketchMode{nullptr};
    std::string m_profileNameBeforeSketchMode;
    bool m_hasProfileBeforeSketchMode{false};
    bool m_hasModernProfileState{false};
    Base::Vector3d m_modernCenterOrigin;
    std::vector<App::DocumentObject*> m_shapes;
    std::vector<App::DocumentObject*> m_xShapes;
    bool m_customComplexSection{false};
    SceneItemPtr<QGraphicsEllipseItem> m_centerHandle;
    using EllipseItemPtr = SceneItemPtr<QGraphicsEllipseItem>;
    std::vector<std::pair<EllipseItemPtr, EllipseItemPtr>>
        m_offsetHandles;
    std::vector<std::pair<EllipseItemPtr, EllipseItemPtr>>
        m_arcBendHandles;
    std::vector<SceneItemPtr<QGraphicsItem>> m_selectableOffsetEdges;
    std::vector<SceneItemPtr<QGraphicsItem>> m_sectionControls;
    SceneItemPtr<QGraphicsItem> m_startRotationHandle;
    SceneItemPtr<QGraphicsItem> m_endRotationHandle;
    SceneItemPtr<QGraphicsItem> m_startEndpointHandle;
    SceneItemPtr<QGraphicsItem> m_endEndpointHandle;
};

class TaskDlgSectionView : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgSectionView(TechDraw::DrawPage* page, QGVPage* graphicsView);
    explicit TaskDlgSectionView(TechDraw::DrawViewPart* base);
    explicit TaskDlgSectionView(TechDraw::DrawViewSection* section);
    ~TaskDlgSectionView() override;

    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int id) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel;
    }
    void modifyStandardButtons(QDialogButtonBox* buttonBox) override;

    bool isAllowedAlterSelection() const override
    { return true; }
    bool isAllowedAlterDocument() const override
    { return false; }

private:
    void restartPositioningHandler();
    void suppressFaceSelection(bool suppress);
    TaskSectionView * widget;
    Gui::TaskView::TaskBox* taskbox;
    QGVPage* m_graphicsView{nullptr};
    bool m_startInteractiveHandler{false};
    QVariant m_previousFaceSelectionSuppression;
    bool m_faceSelectionSuppressionSet{false};
};

} //namespace TechDrawGui
