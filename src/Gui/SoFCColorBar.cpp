// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 **************************************************************************/

#include <cmath>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/SbBox2f.h>
#include <Inventor/fields/SoMFString.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTransparencyType.h>
#include <QApplication>
#include <QMenu>

#include "Dialogs/DlgSettingsColorGradientImp.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "SoFCColorBar.h"
#include "SoLabelNodes.h"


using namespace Gui;

namespace
{
constexpr float xMin = 5.0F;
constexpr float xMax = 5.5F;
constexpr float yMin = -4.0F;
constexpr float yMax = 4.0F;
constexpr float legendXMin = 4.0F;
constexpr float legendXMax = 4.5F;
constexpr float spaceX = 0.1F;
constexpr float spaceY = 0.05F;

std::vector<SbVec3f> getLegendLabelPositions(int count, const SbBox2f& bounds)
{
    std::vector<SbVec3f> positions;
    const float minimumY = bounds.getMin()[1];
    const float maximumX = bounds.getMax()[0];
    const float maximumY = bounds.getMax()[1] - 0.5F;

    if (count > 1) {
        const float step = (maximumY - minimumY) / static_cast<float>(count - 1);
        positions.emplace_back(maximumX + 0.1F, maximumY + 0.20F + step, 0.0F);
        for (int index = 0; index < count; ++index) {
            positions.emplace_back(0.0F, -step, 0.0F);
        }
    }

    return positions;
}

std::vector<SbVec3f> getLegendValuePositions(int count, const SbBox2f& bounds)
{
    std::vector<SbVec3f> positions;
    const float minimumY = bounds.getMin()[1];
    const float maximumX = bounds.getMax()[0];
    const float maximumY = bounds.getMax()[1] - 0.5F;

    if (count > 2) {
        const float step = (maximumY - minimumY) / static_cast<float>(count - 2);
        const float offset = step / 4.0F;

        positions.emplace_back(maximumX + 0.1F, maximumY + 0.25F + 1.5F * step, 0.0F);
        for (int index = 0; index < count; ++index) {
            positions.emplace_back(0.0F, -step, 0.0F);
        }

        positions[1][1] -= offset;
        positions[2][1] += offset;
        positions.back()[1] += offset;
    }

    return positions;
}
}  // namespace

namespace Gui
{
class ColorScaleCoinPresentation
{
public:
    explicit ColorScaleCoinPresentation(std::shared_ptr<ColorScaleOverlay> colorScale)
        : colorScale(std::move(colorScale))
        , gradientBounds(xMin, yMin, xMax, yMax)
        , legendBounds(legendXMin, yMin, legendXMax, yMax)
    {
        modeSwitch = new SoSwitch;
        gradientRoot = new SoSeparator;
        legendRoot = new SoSeparator;
        gradientLabels = new SoSeparator;
        legendLabels = new SoSeparator;
        legendValues = new SoSeparator;
        gradientLabels->ref();
        legendLabels->ref();
        legendValues->ref();

        modeSwitch->addChild(gradientRoot);
        modeSwitch->addChild(legendRoot);
        modeSwitch->whichChild = 0;
    }

    ~ColorScaleCoinPresentation()
    {
        gradientLabels->unref();
        legendLabels->unref();
        legendValues->unref();
    }

    SoSwitch* root() const
    {
        return modeSwitch;
    }

    void setMode(ColorScaleMode mode)
    {
        modeSwitch->whichChild = mode == ColorScaleMode::Gradient ? 0 : 1;
    }

    void refresh()
    {
        rebuildGradient();
        rebuildLegend();
    }

    void setViewport(
        ColorScaleMode mode,
        float minimumX,
        float minimumY,
        float maximumX,
        float maximumY,
        float labelWidth
    )
    {
        if (mode == ColorScaleMode::Gradient) {
            layoutGradient(minimumX, minimumY, maximumX, maximumY, labelWidth);
        }
        else {
            layoutLegend(minimumX, minimumY, maximumX, maximumY, labelWidth);
        }
    }

private:
    void rebuildGradient()
    {
        const auto snapshot = colorScale->snapshot(ColorScaleMode::Gradient);
        setGradientLabels(snapshot);

        gradientCoords = new SoCoordinate3;
        const int colorCount = static_cast<int>(snapshot.gradientStops.size());
        gradientCoords->point.setNum(2 * colorCount);
        updateGradientPoints();

        auto faces = new SoIndexedFaceSet;
        faces->coordIndex.setNum(8 * (colorCount - 1));
        for (int index = 0; index < colorCount - 1; ++index) {
            faces->coordIndex.set1Value(8 * index, 2 * index);
            faces->coordIndex.set1Value(8 * index + 1, 2 * index + 3);
            faces->coordIndex.set1Value(8 * index + 2, 2 * index + 1);
            faces->coordIndex.set1Value(8 * index + 3, SO_END_FACE_INDEX);
            faces->coordIndex.set1Value(8 * index + 4, 2 * index);
            faces->coordIndex.set1Value(8 * index + 5, 2 * index + 2);
            faces->coordIndex.set1Value(8 * index + 6, 2 * index + 3);
            faces->coordIndex.set1Value(8 * index + 7, SO_END_FACE_INDEX);
        }

        auto transparencyType = new SoTransparencyType;
        transparencyType->value = SoGLRenderAction::DELAYED_BLEND;

        auto material = new SoMaterial;
        material->diffuseColor.setNum(2 * colorCount);
        for (int index = 0; index < colorCount; ++index) {
            const Base::Color& color = snapshot.gradientStops[colorCount - index - 1];
            material->diffuseColor.set1Value(2 * index, color.r, color.g, color.b);
            material->diffuseColor.set1Value(2 * index + 1, color.r, color.g, color.b);
        }

        auto materialBinding = new SoMaterialBinding;
        materialBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;

        gradientRoot->removeAllChildren();
        gradientRoot->addChild(transparencyType);
        gradientRoot->addChild(gradientLabels);
        gradientRoot->addChild(gradientCoords);
        gradientRoot->addChild(material);
        gradientRoot->addChild(materialBinding);
        gradientRoot->addChild(faces);
    }

    void setGradientLabels(const ColorScaleSnapshot& snapshot)
    {
        gradientLabels->removeAllChildren();

        const int count = static_cast<int>(snapshot.ticks.size());
        if (count <= 1) {
            return;
        }

        const SbVec2f maximum = gradientBounds.getMax();
        const SbVec2f minimum = gradientBounds.getMin();
        const float step = (maximum[1] - minimum[1]) / static_cast<float>(count - 1);
        const auto textColor = Base::Color(snapshot.textFormat.textColor);

        auto translation = new SoTransform;
        translation->translation.setValue(maximum[0] + spaceX, maximum[1] - spaceY + step, 0.0F);
        gradientLabels->addChild(translation);

        auto color = new SoBaseColor;
        color->rgb.setValue(textColor.r, textColor.g, textColor.b);
        gradientLabels->addChild(color);

        auto font = new SoFont;
        font->name.setValue("Helvetica,Arial,Times New Roman");
        font->size.setValue(static_cast<float>(snapshot.textFormat.textSize));
        gradientLabels->addChild(font);

        for (const auto& tick : snapshot.ticks) {
            auto offset = new SoTransform;
            offset->translation.setValue(0.0F, -step, 0.0F);
            gradientLabels->addChild(offset);

            auto label = new SoColorBarLabel;
            label->string.setValue(tick.text.c_str());
            gradientLabels->addChild(label);
        }
    }

    void updateGradientPoints()
    {
        const float minimumX = gradientBounds.getMin()[0];
        const float minimumY = gradientBounds.getMin()[1];
        const float maximumX = gradientBounds.getMax()[0];
        const float maximumY = gradientBounds.getMax()[1];
        const int count = gradientCoords->point.getNum() / 2;

        for (int index = 0; index < count; ++index) {
            const float factor = static_cast<float>(index) / static_cast<float>(count - 1);
            const float positionY = (1.0F - factor) * maximumY + factor * minimumY;
            gradientCoords->point.set1Value(2 * index, minimumX, positionY, 0.0F);
            gradientCoords->point.set1Value(2 * index + 1, maximumX, positionY, 0.0F);
        }
    }

    void layoutGradient(float minimumX, float minimumY, float maximumX, float maximumY, float labelWidth)
    {
        int transformCount = 0;
        for (int index = 0; index < gradientLabels->getNumChildren(); ++index) {
            if (gradientLabels->getChild(index)->getTypeId() == SoTransform::getClassTypeId()) {
                ++transformCount;
            }
        }

        if (transformCount > 2) {
            const float step = (maximumY - minimumY) / static_cast<float>(transformCount - 2);
            bool first = true;
            for (int index = 0; index < gradientLabels->getNumChildren(); ++index) {
                auto* child = gradientLabels->getChild(index);
                if (child->getTypeId() != SoTransform::getClassTypeId()) {
                    continue;
                }

                auto* transform = static_cast<SoTransform*>(child);
                if (first) {
                    first = false;
                    transform->translation.setValue(
                        minimumX + (maximumX - minimumX) + spaceX - labelWidth,
                        maximumY - spaceY + step,
                        0.0F
                    );
                }
                else {
                    transform->translation.setValue(0.0F, -step, 0.0F);
                }
            }
        }

        gradientBounds.setBounds(minimumX - labelWidth, minimumY, maximumX - labelWidth, maximumY);
        updateGradientPoints();
    }

    void rebuildLegend()
    {
        const auto snapshot = colorScale->snapshot(ColorScaleMode::Legend);
        const int intervalCount = static_cast<int>(snapshot.intervals.size());

        legendCoords = new SoCoordinate3;
        legendCoords->point.setNum(4 * intervalCount);
        updateLegendPoints();
        setLegendLabels(snapshot);

        auto faces = new SoIndexedFaceSet;
        faces->coordIndex.setNum(5 * intervalCount);
        for (int index = 0; index < intervalCount; ++index) {
            faces->coordIndex.set1Value(5 * index, 4 * index);
            faces->coordIndex.set1Value(5 * index + 1, 4 * index + 1);
            faces->coordIndex.set1Value(5 * index + 2, 4 * index + 2);
            faces->coordIndex.set1Value(5 * index + 3, 4 * index + 3);
            faces->coordIndex.set1Value(5 * index + 4, SO_END_FACE_INDEX);
        }

        auto material = new SoMaterial;
        material->diffuseColor.setNum(intervalCount);
        for (int index = 0; index < intervalCount; ++index) {
            const Base::Color& color = snapshot.intervals[intervalCount - index - 1].color;
            material->diffuseColor.set1Value(index, color.r, color.g, color.b);
        }

        auto materialBinding = new SoMaterialBinding;
        materialBinding->value = SoMaterialBinding::PER_FACE;

        legendRoot->removeAllChildren();
        legendRoot->addChild(legendLabels);
        legendRoot->addChild(legendValues);
        legendRoot->addChild(legendCoords);
        legendRoot->addChild(material);
        legendRoot->addChild(materialBinding);
        legendRoot->addChild(faces);
    }

    void setLegendLabels(const ColorScaleSnapshot& snapshot)
    {
        legendLabels->removeAllChildren();
        legendValues->removeAllChildren();

        const int labelCount = static_cast<int>(snapshot.intervals.size());
        if (labelCount > 1) {
            const auto positions = getLegendLabelPositions(labelCount, legendBounds);
            auto translation = new SoTransform;
            translation->translation.setValue(positions.front());
            legendLabels->addChild(translation);

            for (int index = 0; index < labelCount; ++index) {
                auto offset = new SoTransform;
                offset->translation.setValue(positions[index + 1]);
                legendLabels->addChild(offset);

                auto color = new SoBaseColor;
                color->rgb.setValue(0.0F, 0.0F, 0.0F);
                legendLabels->addChild(color);

                auto label = new SoText2;
                label->string.setValue(snapshot.intervals[index].label.c_str());
                legendLabels->addChild(label);
            }
        }

        const int valueCount = static_cast<int>(snapshot.ticks.size());
        if (valueCount > 1) {
            const auto positions = getLegendValuePositions(valueCount, legendBounds);
            auto translation = new SoTransform;
            translation->translation.setValue(positions.front());
            legendValues->addChild(translation);

            for (int index = 0; index < valueCount; ++index) {
                auto offset = new SoTransform;
                offset->translation.setValue(positions[index + 1]);
                legendValues->addChild(offset);

                auto color = new SoBaseColor;
                color->rgb.setValue(0.0F, 0.0F, 0.0F);
                legendValues->addChild(color);

                auto value = new SoText2;
                value->string.setValue(snapshot.ticks[index].text.c_str());
                legendValues->addChild(value);
            }
        }
    }

    void updateLegendPoints()
    {
        const float minimumX = legendBounds.getMin()[0];
        const float minimumY = legendBounds.getMin()[1];
        const float maximumX = legendBounds.getMax()[0];
        const float maximumY = legendBounds.getMax()[1] - 0.5F;
        const int count = legendCoords->point.getNum() / 4;

        for (int index = 0; index < count; ++index) {
            const float factor = static_cast<float>(index) / static_cast<float>(count - 1);
            const float positionY1 = factor * maximumY + (1.0F - factor) * minimumY;
            const float positionY2 = positionY1 + 0.5F;
            legendCoords->point.set1Value(4 * index, minimumX, positionY1, 0.0F);
            legendCoords->point.set1Value(4 * index + 1, maximumX, positionY1, 0.0F);
            legendCoords->point.set1Value(4 * index + 2, maximumX, positionY2, 0.0F);
            legendCoords->point.set1Value(4 * index + 3, minimumX, positionY2, 0.0F);
        }
    }

    void layoutLegend(float minimumX, float minimumY, float maximumX, float maximumY, float labelWidth)
    {
        legendBounds.setBounds(minimumX - labelWidth, minimumY, maximumX - labelWidth, maximumY);
        arrangeLegendLabels(legendLabels, true);
        arrangeLegendLabels(legendValues, false);
        updateLegendPoints();
    }

    void arrangeLegendLabels(SoSeparator* group, bool labels)
    {
        int transformCount = 0;
        for (int index = 0; index < group->getNumChildren(); ++index) {
            if (group->getChild(index)->getTypeId() == SoTransform::getClassTypeId()) {
                ++transformCount;
            }
        }

        const int minimumTransforms = labels ? 2 : 3;
        if (transformCount <= minimumTransforms) {
            return;
        }

        const auto positions = labels ? getLegendLabelPositions(transformCount - 1, legendBounds)
                                      : getLegendValuePositions(transformCount - 1, legendBounds);
        int position = 0;
        for (int index = 0; index < group->getNumChildren(); ++index) {
            auto* child = group->getChild(index);
            if (child->getTypeId() == SoTransform::getClassTypeId()) {
                static_cast<SoTransform*>(child)->translation.setValue(positions[position++]);
            }
        }
    }

    std::shared_ptr<ColorScaleOverlay> colorScale;
    SoSwitch* modeSwitch {};
    SoSeparator* gradientRoot {};
    SoSeparator* legendRoot {};
    SoSeparator* gradientLabels {};
    SoSeparator* legendLabels {};
    SoSeparator* legendValues {};
    SoCoordinate3* gradientCoords {};
    SoCoordinate3* legendCoords {};
    SbBox2f gradientBounds;
    SbBox2f legendBounds;
};

class ColorScaleOptionsEvent: public QObject
{
public:
    explicit ColorScaleOptionsEvent(SoFCColorBar* bar)
        : bar(bar)
    {}

    void customEvent(QEvent*) override
    {
        bar->customize();
        deleteLater();
    }

private:
    SoFCColorBar* bar;
};
}  // namespace Gui

SO_NODE_SOURCE(SoFCColorBar)

SoFCColorBar::SoFCColorBar()
    : colorScale(std::make_shared<ColorScaleOverlay>())
    , presentation(std::make_unique<ColorScaleCoinPresentation>(colorScale))
    , windowSize(0, 0)
{
    SO_NODE_CONSTRUCTOR(SoFCColorBar);

    auto lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    addChild(lightModel);
    addChild(presentation->root());

    colorScale->setRange(-0.5F, 0.5F, 1);
    updatePresentation();
}

SoFCColorBar::~SoFCColorBar() = default;

void SoFCColorBar::initClass()
{
    SO_NODE_INIT_CLASS(SoFCColorBar, SoSeparator, "Separator");
}

void SoFCColorBar::finish()
{
    atexit_cleanup();
}

const ColorScaleOverlay& SoFCColorBar::getColorScale() const
{
    return *colorScale;
}

void SoFCColorBar::GLRenderBelowPath(SoGLRenderAction* action)
{
    prepareViewport(action->getViewportRegion().getWindowSize());
    SoSeparator::GLRenderBelowPath(action);
}

void SoFCColorBar::setRange(float minimum, float maximum, int precision)
{
    colorScale->setRange(minimum, maximum, precision);
    updatePresentation();
}

Base::Color SoFCColorBar::getColor(float value) const
{
    return colorScale->getColor(value);
}

void SoFCColorBar::setOutsideGrayed(bool value)
{
    colorScale->setOutsideGrayed(value);
}

bool SoFCColorBar::isVisible(float value) const
{
    return colorScale->isVisible(value);
}

float SoFCColorBar::getMinValue() const
{
    return colorScale->minimum();
}

float SoFCColorBar::getMaxValue() const
{
    return colorScale->maximum();
}

void SoFCColorBar::setFormat(const SoLabelTextFormat& format)
{
    colorScale->setTextFormat(format);
    updatePresentation();
}

void SoFCColorBar::prepareViewport(const SbVec2s& size, bool force)
{
    if (!force && windowSize == size) {
        return;
    }

    windowSize = size;
    float minimumX {};
    float minimumY {};
    float maximumX {};
    float maximumY {};
    float labelWidth = getBounds(size, minimumX, minimumY, maximumX, maximumY);

    if (colorScale->mode() == ColorScaleMode::Gradient) {
        minimumX = snapToPixelGrid(minimumX, size);
        minimumY = snapToPixelGrid(minimumY, size);
        maximumX = snapToPixelGrid(maximumX, size);
        maximumY = snapToPixelGrid(maximumY, size);
        labelWidth = snapExtentToPixelGrid(labelWidth, size);
    }

    presentation->setViewport(colorScale->mode(), minimumX, minimumY, maximumX, maximumY, labelWidth);
}

void SoFCColorBar::updatePresentation()
{
    presentation->refresh();
    boxWidth = -1.0F;
    if (windowSize[0] > 0 && windowSize[1] > 0) {
        prepareViewport(windowSize, true);
    }
}

void SoFCColorBar::setMode(ColorScaleMode mode)
{
    colorScale->setMode(mode);
    presentation->setMode(mode);
    if (windowSize[0] > 0 && windowSize[1] > 0) {
        boxWidth = -1.0F;
        prepareViewport(windowSize, true);
    }
}

float SoFCColorBar::getBoundingWidth(const SbVec2s& size)
{
    const float ratio = static_cast<float>(size[0]) / static_cast<float>(size[1]);
    if (ratio >= 1.0F && boxWidth >= 0.0F) {
        return boxWidth;
    }

    auto camera = new SoOrthographicCamera;
    camera->position = SbVec3f(0.0F, 0.0F, 5.0F);
    camera->height = 10.0F;
    camera->nearDistance = 0.0F;
    camera->farDistance = 10.0F;

    auto group = new SoGroup;
    group->ref();
    group->addChild(camera);
    group->addChild(this);

    SoGetBoundingBoxAction action {SbViewportRegion(size)};
    action.apply(group);
    SbVec3f minimum;
    SbVec3f maximum;
    action.getBoundingBox().getBounds(minimum, maximum);
    group->unref();

    boxWidth = maximum[0] - minimum[0];
    return boxWidth;
}

float SoFCColorBar::snapToPixelGrid(float coordinate, const SbVec2s& size) const
{
    if (size[1] <= 0) {
        return coordinate;
    }

    const float logicalUnitsPerPixel = 10.0F / static_cast<float>(size[1]);
    return static_cast<float>(
        std::round(static_cast<double>(coordinate) / logicalUnitsPerPixel)
        * static_cast<double>(logicalUnitsPerPixel)
    );
}

float SoFCColorBar::snapExtentToPixelGrid(float extent, const SbVec2s& size) const
{
    if (size[1] <= 0) {
        return extent;
    }

    const float logicalUnitsPerPixel = 10.0F / static_cast<float>(size[1]);
    return std::ceil(extent / logicalUnitsPerPixel) * logicalUnitsPerPixel;
}

float SoFCColorBar::getBounds(
    const SbVec2s& size,
    float& minimumX,
    float& minimumY,
    float& maximumX,
    float& maximumY
)
{
    const float ratio = static_cast<float>(size[0]) / static_cast<float>(size[1]);
    const float baseY = ratio > 3.0F ? 2.5F : 3.0F;
    constexpr float barWidth = 0.5F;

    minimumX = 4.95F * ratio;
    maximumX = minimumX + barWidth;
    minimumY = -baseY - 0.6F;
    maximumY = baseY;

    if (ratio < 1.0F) {
        minimumX /= ratio;
        maximumX /= ratio;
        minimumY = -baseY / ratio;
        maximumY = baseY / ratio;
    }

    return getBoundingWidth(size);
}

void SoFCColorBar::customize()
{
    if (colorScale->mode() != ColorScaleMode::Gradient) {
        return;
    }

    QWidget* parent = Gui::getMainWindow()->activeWindow();
    App::ColorGradient editableGradient;
    editableGradient.setProfile(colorScale->gradientProfile());
    Gui::Dialog::DlgSettingsColorGradientImp dialog(editableGradient, parent);
    const App::ColorGradientProfile originalProfile = editableGradient.getProfile();
    const int originalPrecision = colorScale->precision();
    dialog.setNumberOfDecimals(originalPrecision, originalProfile.fMin, originalProfile.fMax);

    QPoint position(QCursor::pos());
    position += QPoint(int(-1.1 * dialog.width()), int(-0.1 * dialog.height()));  // NOLINT
    dialog.move(position);

    auto applyProfile = [&](const App::ColorGradientProfile& profile, int precision) {
        colorScale->setGradientProfile(profile, precision);
        updatePresentation();
        Notify(0);
    };
    QObject::connect(&dialog, &Gui::Dialog::DlgSettingsColorGradientImp::colorModelChanged, [&] {
        try {
            applyProfile(dialog.getProfile(), dialog.numberOfDecimals());
        }
        catch (const Base::Exception& exception) {
            exception.reportException();
        }
    });

    if (dialog.exec() != QDialog::Accepted) {
        if (!originalProfile.isEqual(dialog.getProfile())
            || dialog.numberOfDecimals() != originalPrecision) {
            applyProfile(originalProfile, originalPrecision);
        }
    }
    else if (
        !originalProfile.isEqual(dialog.getProfile())
        || dialog.numberOfDecimals() != colorScale->precision()
    ) {
        applyProfile(dialog.getProfile(), dialog.numberOfDecimals());
    }
}

void SoFCColorBar::handleEvent(SoHandleEventAction* action)
{
    const SoEvent* event = action->getEvent();
    if (!event->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        return;
    }

    if (!action->getPickedPoint()) {
        return;
    }

    const auto* mouseEvent = static_cast<const SoMouseButtonEvent*>(event);
    action->setHandled();
    if (mouseEvent->getButton() == SoMouseButtonEvent::BUTTON1
        && mouseEvent->getState() == SoButtonEvent::DOWN) {
        if (!timer.isValid()) {
            timer.start();
        }
        else if (timer.restart() < QApplication::doubleClickInterval()) {
            QApplication::postEvent(new ColorScaleOptionsEvent(this), new QEvent(QEvent::User));
        }
        return;
    }

    if (mouseEvent->getButton() != SoMouseButtonEvent::BUTTON2
        || mouseEvent->getState() != SoButtonEvent::UP) {
        return;
    }

    QMenu menu;
    QAction* gradient = menu.addAction(QObject::tr("Color Gradient"));
    gradient->setCheckable(true);
    gradient->setChecked(colorScale->mode() == ColorScaleMode::Gradient);
    QAction* legend = menu.addAction(QObject::tr("Color Legend"));
    legend->setCheckable(true);
    legend->setChecked(colorScale->mode() == ColorScaleMode::Legend);
    menu.addSeparator();
    QAction* options = menu.addAction(QObject::tr("Options"));

    QAction* selected = menu.exec(QCursor::pos());
    if (selected == gradient) {
        setMode(ColorScaleMode::Gradient);
    }
    else if (selected == legend) {
        setMode(ColorScaleMode::Legend);
    }
    else if (selected == options && colorScale->mode() == ColorScaleMode::Gradient) {
        QApplication::postEvent(new ColorScaleOptionsEvent(this), new QEvent(QEvent::User));
    }
}
