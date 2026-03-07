// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "FreeCADStyle.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <span>
#include <string>
#include <vector>
#include <QApplication>
#include <QAbstractSpinBox>
#include <QFrame>
#include <QGroupBox>
#include <QImage>
#include <QLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>
#include <QStyleOption>

#include <Base/Color.h>
#include <Base/Converter.h>
#include <Base/Exception.h>

#include "Application.h"
#include "ThemeReloadEvent.h"
#include "StyleParameters/Corners.h"
#include "StyleParameters/Gradient.h"
#include "StyleParameters/InnerShadow.h"
#include "StyleParameters/Insets.h"
#include "StyleParameters/ParameterManager.h"

QT_BEGIN_NAMESPACE
extern Q_WIDGETS_EXPORT void qt_blurImage(
    QPainter* painter,
    QImage& blurImage,
    qreal radius,
    bool quality,
    bool alphaOnly,
    int transposed = 0
);
QT_END_NAMESPACE

using namespace Gui;

// ─── Base::convertTo specializations ──────────────────────────────────────
// These teach Base::convertTo how to convert StyleParameters domain types
// into Qt/FreeCADStyle types. Placed here (not in Base/) because the target
// types (QMarginsF, QBrush, FreeCADStyle structs) live in the Gui layer.

namespace Base
{

template<>
FreeCADStyle::CornerRadii convertTo<FreeCADStyle::CornerRadii, StyleParameters::Corners>(
    const StyleParameters::Corners& corners
)
{
    return {
        .topLeft = corners.topLeft().value,
        .topRight = corners.topRight().value,
        .bottomRight = corners.bottomRight().value,
        .bottomLeft = corners.bottomLeft().value,
    };
}

template<>
QMarginsF convertTo<QMarginsF, StyleParameters::Insets>(const StyleParameters::Insets& insets)
{
    return QMarginsF(insets.left().value, insets.top().value, insets.right().value, insets.bottom().value);
}

template<>
FreeCADStyle::InnerShadow convertTo<FreeCADStyle::InnerShadow, StyleParameters::InnerShadow>(
    const StyleParameters::InnerShadow& shadow
)
{
    return {
        .color = shadow.color().asValue<QColor>(),
        .x = shadow.x(),
        .y = shadow.y(),
        .blur = shadow.blur(),
    };
}

template<>
QBrush convertTo<QBrush, StyleParameters::Value>(const StyleParameters::Value& value)
{
    using namespace StyleParameters;

    if (value.holds<::Base::Color>()) {
        return QBrush(value.get<::Base::Color>().asValue<QColor>());
    }

    if (!value.holds<Tuple>()) {
        return Qt::NoBrush;
    }

    const Tuple& tuple = value.get<Tuple>();

    const auto applyStopsAndBuild = [](auto qGradient, const auto& gradient) -> QBrush {
        qGradient.setCoordinateMode(QGradient::ObjectMode);
        for (const auto& stop : gradient.colorStops()) {
            qGradient.setColorAt(stop.position.value, stop.color.template asValue<QColor>());
        }
        return QBrush(qGradient);
    };

    if (tuple.kind == TupleKind::LinearGradient) {
        try {
            const LinearGradient gradient(tuple);
            return applyStopsAndBuild(
                QLinearGradient(gradient.x1(), gradient.y1(), gradient.x2(), gradient.y2()),
                gradient
            );
        }
        catch (const ::Base::Exception&) {
            return Qt::NoBrush;
        }
    }

    if (tuple.kind == TupleKind::RadialGradient) {
        try {
            const RadialGradient gradient(tuple);
            return applyStopsAndBuild(
                QRadialGradient(
                    gradient.cx(),
                    gradient.cy(),
                    gradient.radius(),
                    gradient.fx(),
                    gradient.fy()
                ),
                gradient
            );
        }
        catch (const ::Base::Exception&) {
            return Qt::NoBrush;
        }
    }

    return Qt::NoBrush;
}

}  // namespace Base

namespace
{

// Arc start angles (in degrees) for each corner of a clockwise rounded rectangle.
constexpr qreal arcStartTopRight = 90;
constexpr qreal arcStartBottomRight = 0;
constexpr qreal arcStartBottomLeft = 270;
constexpr qreal arcStartTopLeft = 180;
constexpr qreal arcSweepClockwise = -90;

QPainterPath roundedRectPath(const QRectF& rect, const FreeCADStyle::CornerRadii& radii)
{
    const qreal topLeft = radii.topLeft;
    const qreal topRight = radii.topRight;
    const qreal bottomRight = radii.bottomRight;
    const qreal bottomLeft = radii.bottomLeft;

    QPainterPath path;
    path.moveTo(rect.left() + topLeft, rect.top());
    path.lineTo(rect.right() - topRight, rect.top());
    path.arcTo(
        rect.right() - (2 * topRight),
        rect.top(),
        2 * topRight,
        2 * topRight,
        arcStartTopRight,
        arcSweepClockwise
    );
    path.lineTo(rect.right(), rect.bottom() - bottomRight);
    path.arcTo(
        rect.right() - (2 * bottomRight),
        rect.bottom() - (2 * bottomRight),
        2 * bottomRight,
        2 * bottomRight,
        arcStartBottomRight,
        arcSweepClockwise
    );
    path.lineTo(rect.left() + bottomLeft, rect.bottom());
    path.arcTo(
        rect.left(),
        rect.bottom() - (2 * bottomLeft),
        2 * bottomLeft,
        2 * bottomLeft,
        arcStartBottomLeft,
        arcSweepClockwise
    );
    path.lineTo(rect.left(), rect.top() + topLeft);
    path.arcTo(rect.left(), rect.top(), 2 * topLeft, 2 * topLeft, arcStartTopLeft, arcSweepClockwise);
    path.closeSubpath();
    return path;
}

FreeCADStyle::CornerRadii innerRadii(const FreeCADStyle::CornerRadii& outer, const QMarginsF& thickness)
{
    auto shrink = [](qreal radius, qreal a, qreal b) -> qreal {
        return std::max(0.0, radius - std::max(a, b));
    };
    return {
        .topLeft = shrink(outer.topLeft, thickness.top(), thickness.left()),
        .topRight = shrink(outer.topRight, thickness.top(), thickness.right()),
        .bottomRight = shrink(outer.bottomRight, thickness.bottom(), thickness.right()),
        .bottomLeft = shrink(outer.bottomLeft, thickness.bottom(), thickness.left()),
    };
}

struct ShadowCacheKey
{
    int width, height;
    qreal x, y, blur;
    QRgb color;
    qreal radiusTopLeft, radiusTopRight, radiusBottomRight, radiusBottomLeft;

    auto operator<=>(const ShadowCacheKey&) const = default;
};

QImage buildShadowImage(
    const QRect& rect,
    const FreeCADStyle::CornerRadii& radii,
    const FreeCADStyle::InnerShadow& shadow
)
{
    const int padding = static_cast<int>(std::ceil(shadow.blur)) + 1;
    const QSize imageSize = rect.size() + QSize(2 * padding, 2 * padding);

    // Create a fully opaque black image and punch a transparent hole in the shape.
    // The opaque ring that remains around the hole produces the shadow after blurring.
    QImage mask(imageSize, QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::black);

    {
        QPainter maskPainter(&mask);
        maskPainter.setRenderHint(QPainter::Antialiasing);
        maskPainter.setCompositionMode(QPainter::CompositionMode_Clear);
        maskPainter.fillPath(
            roundedRectPath(QRectF(padding, padding, rect.width(), rect.height()), radii),
            Qt::transparent
        );
    }

    QImage blurred(imageSize, QImage::Format_ARGB32_Premultiplied);
    blurred.fill(Qt::transparent);
    {
        QPainter blurPainter(&blurred);
        qt_blurImage(&blurPainter, mask, shadow.blur, false, false);
    }

    // Tint the blurred image with the shadow color.
    {
        QPainter tintPainter(&blurred);
        tintPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        tintPainter.fillRect(blurred.rect(), shadow.color);
    }

    return blurred;
}

const QImage& getCachedShadowImage(
    const QRect& rect,
    const FreeCADStyle::CornerRadii& radii,
    const FreeCADStyle::InnerShadow& shadow
)
{
    constexpr int maxCacheEntries = 32;
    static std::map<ShadowCacheKey, QImage> cache;

    const ShadowCacheKey key {
        .width = rect.width(),
        .height = rect.height(),
        .x = shadow.x,
        .y = shadow.y,
        .blur = shadow.blur,
        .color = shadow.color.rgba(),
        .radiusTopLeft = radii.topLeft,
        .radiusTopRight = radii.topRight,
        .radiusBottomRight = radii.bottomRight,
        .radiusBottomLeft = radii.bottomLeft,
    };

    if (auto it = cache.find(key); it != cache.end()) {
        return it->second;
    }
    if (static_cast<int>(cache.size()) >= maxCacheEntries) {
        cache.erase(cache.begin());
    }
    return cache.emplace(key, buildShadowImage(rect, radii, shadow)).first->second;
}

// ─── StyleToken string tables ──────────────────────────────────────────────
// These functions convert enum values to the string fragments used in token
// names such as "ButtonPrimaryHoveredBackground".

// Returns the inheritance chain for a component, ordered from most-specific to
// most-abstract. Each entry is the token name prefix for that level.
// To add a new component: add a StyleComponent enum entry and a chain array here.
// To add a new abstract base: add an entry to the relevant chains — no enum change needed.
std::span<const std::string_view> componentChain(StyleComponent component)
{
    static constexpr auto pushButton = std::to_array<std::string_view>({"Button", "FormControl"});
    static constexpr auto toolButton = std::to_array<std::string_view>({
        "ToolButton",
        "Button",
        "FormControl",
    });
    static constexpr auto lineEdit = std::to_array<std::string_view>({"LineEdit", "FormControl"});
    static constexpr auto textEdit = std::to_array<std::string_view>(
        {"TextEdit", "LineEdit", "FormControl"}
    );
    static constexpr auto select = std::to_array<std::string_view>({"Select", "Button", "FormControl"});
    static constexpr auto comboBox = std::to_array<std::string_view>(
        {"ComboBox", "LineEdit", "FormControl"}
    );

    switch (component) {
        case StyleComponent::PushButton:
            return pushButton;
        case StyleComponent::ToolButton:
            return toolButton;
        case StyleComponent::LineEdit:
            return lineEdit;
        case StyleComponent::TextEdit:
            return textEdit;
        case StyleComponent::Select:
            return select;
        case StyleComponent::ComboBox:
            return comboBox;
        default:
            return {};
    }
}

// Returns the token string for a single VariantSlot value.
// Add a case here whenever a new VariantSlot is added.
std::string_view variantSlotString(VariantSlot slot, uint8_t value)
{
    switch (slot) {
        case VariantSlot::ButtonType:
            switch (static_cast<ButtonType>(value)) {
                case ButtonType::Primary:
                    return "Primary";
                case ButtonType::Link:
                    return "Link";
                default:
                    return "";
            }
        case VariantSlot::ControlSize:
            switch (static_cast<ControlSize>(value)) {
                case ControlSize::Small:
                    return "Small";
                case ControlSize::Big:
                    return "Big";
                default:
                    return "";
            }
        default:
            return "";
    }
}

// Concatenates the string fragments of all non-default variant slots.
// e.g. ButtonType=Primary, ControlSize=Default → "Primary"
std::string variantString(const VariantKey& variant)
{
    std::string result;
    for (size_t index = 0; index < variant.slots.size(); ++index) {
        result += variantSlotString(VariantSlot(index), variant.slots.at(index));
    }
    return result;
}

std::string_view stateString(StyleState state)
{
    switch (state) {
        case StyleState::Pressed:
            return "Pressed";
        case StyleState::Hovered:
            return "Hovered";
        case StyleState::Checked:
            return "Checked";
        case StyleState::Focused:
            return "Focused";
        default:
            return "";
    }
}

std::string_view propertyString(StyleProperty property)
{
    switch (property) {
        case StyleProperty::Width:
            return "Width";
        case StyleProperty::MinWidth:
            return "MinWidth";
        case StyleProperty::MaxWidth:
            return "MaxWidth";
        case StyleProperty::Height:
            return "Height";
        case StyleProperty::MinHeight:
            return "MinHeight";
        case StyleProperty::MaxHeight:
            return "MaxHeight";
        case StyleProperty::BorderThickness:
            return "BorderThickness";
        case StyleProperty::BorderRadius:
            return "BorderRadius";
        case StyleProperty::BorderColor:
            return "BorderColor";
        case StyleProperty::Padding:
            return "Padding";
        case StyleProperty::Margin:
            return "Margin";
        case StyleProperty::IconSize:
            return "IconSize";
        case StyleProperty::IconSpacing:
            return "IconSpacing";
        case StyleProperty::FontSize:
            return "FontSize";
        case StyleProperty::FontWeight:
            return "FontWeight";
        case StyleProperty::Background:
            return "Background";
        case StyleProperty::TextColor:
            return "TextColor";
        case StyleProperty::Overlay:
            return "Overlay";
        case StyleProperty::OverlayOpacity:
            return "OverlayOpacity";
        case StyleProperty::InnerShadow:
            return "InnerShadow";
        default:
            return "";
    }
}

// ─── Prefix list builder ────────────────────────────────────────────────────
//
// Produces the ordered fallback prefix list for a StyleContext.
//
// Given component="Button", variant="Primary", active states={Hovered, Focused}
// the result is:
//   "ButtonPrimaryHovered"   ← variant + highest-priority state
//   "ButtonPrimaryFocused"   ← variant + next state
//   "ButtonPrimary"          ← variant, no state
//   "ButtonHovered"          ← no variant, highest-priority state
//   "ButtonFocused"          ← no variant, next state
//   "Button"                 ← baseline

// Priority order — highest first. Mirrors the enum declaration order (Pressed > Hovered > …).
constexpr std::array<StyleState, 4> statePriorityOrder = {
    StyleState::Pressed,
    StyleState::Hovered,
    StyleState::Checked,
    StyleState::Focused,
};

std::vector<std::string> buildPrefixes(const StyleContext& context)
{
    const std::string variantSuffix = variantString(context.variant);

    std::vector<StyleState> activeStates;
    for (const StyleState stateFlag : statePriorityOrder) {
        if (context.state.testFlag(stateFlag)) {
            activeStates.push_back(stateFlag);
        }
    }

    std::vector<std::string> prefixes;

    for (const std::string_view componentPrefix : componentChain(context.component)) {
        if (!variantSuffix.empty()) {
            for (const StyleState stateFlag : activeStates) {
                prefixes.push_back(
                    std::string(componentPrefix) + variantSuffix + std::string(stateString(stateFlag))
                );
            }
            prefixes.push_back(std::string(componentPrefix) + variantSuffix);
        }

        for (const StyleState stateFlag : activeStates) {
            prefixes.push_back(std::string(componentPrefix) + std::string(stateString(stateFlag)));
        }

        prefixes.push_back(std::string(componentPrefix));
    }

    return prefixes;
}

// ─── Cache key packing ─────────────────────────────────────────────────────
//
// Packs a (StyleContext, StyleProperty) pair into a uint32_t for use as an
// unordered_map key. Bit layout:
//
//   bits  0– 4 : StyleComponent  (5 bits, up to 32 values)
//   bits  5– 8 : StyleState      (4-bit bitmask)
//   bits  9–14 : StyleProperty   (6 bits, up to 64 values)
//   bits 15–.. : VariantSlots    (4 bits each, starting at bit 15)
//
// Adding a new VariantSlot or enum value does not require changing this function.

uint32_t packVariant(const VariantKey& variant)
{
    uint32_t packed = 0;
    for (size_t index = 0; index < variant.slots.size(); ++index) {
        packed |= uint32_t(variant.slots.at(index)) << (index * 4);
    }
    return packed;
}

// clang-format off
// Bit offsets within the packed cache key.
constexpr uint32_t componentBitOffset = 0;
constexpr uint32_t stateBitOffset     = 5;   // component (5 bits) ends at bit 4
constexpr uint32_t propertyBitOffset  = 9;   // state (4-bit bitmask) ends at bit 8
constexpr uint32_t variantBitOffset   = 15;  // property (6 bits) ends at bit 14
// clang-format on

uint32_t packCacheKey(const StyleContext& context, StyleProperty property)
{
    // clang-format off
    return (static_cast<uint32_t>(context.component)                << componentBitOffset)
         | (static_cast<uint32_t>(context.state.toUnderlyingType()) << stateBitOffset)
         | (static_cast<uint32_t>(property)                         << propertyBitOffset)
         | (packVariant(context.variant)                            << variantBitOffset);
    // clang-format on
}

}  // namespace

void FreeCADStyle::drawBoxBackground(QPainter* painter, const QRect& rect, const BoxStyleDefinition& rule)
{
    if (rule.background.style() == Qt::NoBrush) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    // Clip to the outer rect so antialiased arc pixels cannot bleed outside.
    painter->setClipRect(rect, Qt::IntersectClip);

    const bool hasBorder = rule.borderColor.has_value() && rule.borderThickness.has_value();

    QRect backgroundRect = rect;
    CornerRadii backgroundRadii = rule.borderRadius;

    if (hasBorder) {
        const QMarginsF& thickness = *rule.borderThickness;

        // Snap each border side to the nearest integer pixel.
        const QMarginsF snappedThickness(
            qRound(thickness.left()),
            qRound(thickness.top()),
            qRound(thickness.right()),
            qRound(thickness.bottom())
        );
        backgroundRect = rect.adjusted(
            qRound(thickness.left()),
            qRound(thickness.top()),
            -qRound(thickness.right()),
            -qRound(thickness.bottom())
        );
        backgroundRadii = innerRadii(rule.borderRadius, snappedThickness);

        // Fill outer rect with border colour.
        painter->fillPath(roundedRectPath(QRectF(rect), rule.borderRadius), QBrush(*rule.borderColor));
    }

    painter->fillPath(roundedRectPath(QRectF(backgroundRect), backgroundRadii), rule.background);

    if (rule.overlay) {
        painter->fillPath(roundedRectPath(QRectF(backgroundRect), backgroundRadii), *rule.overlay);
    }

    painter->restore();

    if (rule.innerShadow) {
        const int padding = static_cast<int>(std::ceil(rule.innerShadow->blur)) + 1;
        const QImage& shadowImage = getCachedShadowImage(rect, rule.borderRadius, *rule.innerShadow);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setClipPath(roundedRectPath(QRectF(rect), rule.borderRadius), Qt::IntersectClip);
        painter->drawImage(
            QPointF(
                rect.left() - padding + rule.innerShadow->x,
                rect.top() - padding + rule.innerShadow->y
            ),
            shadowImage
        );
        painter->restore();
    }
}

void FreeCADStyle::polish(QPalette& palette)
{
    QProxyStyle::polish(palette);

    // Sets role in both Active and Inactive groups; leaves Disabled unchanged.
    const auto set = [&](QPalette::ColorRole role, std::string_view token) {
        if (const auto color = resolve<Base::Color>(token)) {
            palette.setColor(QPalette::Active, role, color->asValue<QColor>());
            palette.setColor(QPalette::Inactive, role, color->asValue<QColor>());
        }
    };

    // Sets role only in the Disabled group.
    const auto setDisabled = [&](QPalette::ColorRole role, std::string_view token) {
        if (const auto color = resolve<Base::Color>(token)) {
            palette.setColor(QPalette::Disabled, role, color->asValue<QColor>());
        }
    };

    // ── Active / Inactive ────────────────────────────────────────────────────

    // Window surfaces
    set(QPalette::Window, "BaseWindowBackground");
    set(QPalette::WindowText, "BaseTextColor");

    // Input / item-view surfaces
    set(QPalette::Base, "BaseInputBackground");
    set(QPalette::AlternateBase, "BaseAlternateBackground");
    set(QPalette::Text, "BaseTextColor");
    set(QPalette::PlaceholderText, "BasePlaceholderTextColor");

    // Buttons
    set(QPalette::Button, "BaseButtonBackground");
    set(QPalette::ButtonText, "ButtonTextColor");

    // Selection
    set(QPalette::Highlight, "BaseHighlightBackground");
    set(QPalette::HighlightedText, "BaseHighlightTextColor");
    set(QPalette::BrightText, "BaseHighlightTextColor");

    // Links
    set(QPalette::Link, "BaseLinkColor");
    set(QPalette::LinkVisited, "BaseLinkColor");  // themes can override if needed

    // Tooltips
    set(QPalette::ToolTipBase, "BaseTooltipBackground");
    set(QPalette::ToolTipText, "BaseTooltipTextColor");

    // 3D shading (used by non-custom widgets for borders and shadows)
    set(QPalette::Light, "BaseShadingLight");
    set(QPalette::Midlight, "BaseShadingMidlight");
    set(QPalette::Mid, "BaseShadingMid");
    set(QPalette::Dark, "BaseShadingDark");
    set(QPalette::Shadow, "BaseShadingShadow");

    // ── Disabled ─────────────────────────────────────────────────────────────

    setDisabled(QPalette::WindowText, "BaseDisabledTextColor");
    setDisabled(QPalette::Text, "BaseDisabledTextColor");
    setDisabled(QPalette::ButtonText, "BaseDisabledTextColor");
    setDisabled(QPalette::PlaceholderText, "BaseDisabledTextColor");
    setDisabled(QPalette::Base, "BaseWindowBackground");
    setDisabled(QPalette::Button, "BaseDisabledBackground");
    setDisabled(QPalette::Highlight, "BaseShadingMid");
    setDisabled(QPalette::HighlightedText, "BaseDisabledTextColor");
}

void FreeCADStyle::drawPrimitive(
    PrimitiveElement element,
    const QStyleOption* option,
    QPainter* painter,
    const QWidget* widget
) const
{
    if (element == PE_PanelButtonCommand) {
        drawComponent(painter, option->rect, widget, option);
        return;
    }

    if (element == PE_PanelLineEdit) {
        // Qt sets lineWidth = 0 on the inner QLineEdit embedded inside QAbstractSpinBox
        // (via setFrame(false)). Respect that: the spinbox outer frame is drawn separately
        // via CC_SpinBox → PE_PanelLineEdit with the spinbox widget, so the inner edit
        // panel must not draw a second border.
        const auto* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (frameOption && frameOption->lineWidth == 0) {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }
        drawComponent(painter, option->rect, widget, option);
        return;
    }

    if (element == PE_Frame) {
        if (qobject_cast<const QTextEdit*>(widget) || qobject_cast<const QPlainTextEdit*>(widget)) {
            const auto* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
            if (frameOption && frameOption->lineWidth > 0) {
                drawComponent(painter, option->rect, widget, option);
                return;
            }
        }
    }

    if (element == PE_IndicatorToolBarSeparator) {
        // In a horizontal toolbar the buttons are side-by-side, so the separator
        // is a vertical line; in a vertical toolbar it is a horizontal line.
        const bool toolbarIsHorizontal = option->state & QStyle::State_Horizontal;
        drawSeparatorLine(painter, option->rect, !toolbarIsHorizontal);
        return;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

QSize FreeCADStyle::sizeFromContents(
    ContentsType type,
    const QStyleOption* option,
    const QSize& size,
    const QWidget* widget
) const
{
    if (type == CT_PushButton) {
        const StyleContext context = contextOf(widget, option);
        const auto* btnOption = qstyleoption_cast<const QStyleOptionButton*>(option);
        const BoxGeometryDefinition geometry = resolveBoxGeometry(context);

        int width = size.width() + geometry.paddingH();
        int height = size.height() + geometry.paddingV();

        // Fix icon-text spacing contribution (Qt hardcodes 4 px).
        if (btnOption && !btnOption->icon.isNull() && !btnOption->text.isEmpty()) {
            constexpr int qtBuiltInIconGap = 4;
            width += geometry.iconSpacing - qtBuiltInIconGap;
        }

        if (geometry.height) {
            height = *geometry.height;
        }
        if (geometry.minWidth) {
            width = std::max(width, *geometry.minWidth);
        }

        return {width, height};
    }

    if (type == CT_ComboBox || type == CT_LineEdit || type == CT_SpinBox) {
        const BoxGeometryDefinition geometry = resolveBoxGeometry(contextOf(widget, option));
        QSize result = QProxyStyle::sizeFromContents(type, option, size, widget);
        if (geometry.height) {
            result.setHeight(*geometry.height);
        }
        return result;
    }

    if (type == CT_ToolButton) {
        const StyleContext context = contextOf(widget, option);
        const auto* tbOption = qstyleoption_cast<const QStyleOptionToolButton*>(option);
        const BoxGeometryDefinition geometry = resolveBoxGeometry(context);

        const bool hasIconOrArrow = tbOption
            && (!tbOption->icon.isNull() || tbOption->arrowType != Qt::NoArrow);
        const bool needsCustomLayout = hasIconOrArrow && tbOption && !tbOption->text.isEmpty()
            && (tbOption->toolButtonStyle == Qt::ToolButtonTextBesideIcon
                || tbOption->toolButtonStyle == Qt::ToolButtonTextUnderIcon);

        int width = size.width() + geometry.paddingH();
        int height = size.height() + geometry.paddingV();

        if (needsCustomLayout && tbOption->toolButtonStyle == Qt::ToolButtonTextBesideIcon) {
            // Qt hardcodes +4 as the icon-text gap in QToolButton::sizeHint's content size.
            // Replace that with our spacing so the widget is wide enough.
            constexpr int qtBuiltInIconGap = 4;
            width += geometry.iconSpacing - qtBuiltInIconGap;
        }

        if (geometry.height) {
            height = *geometry.height;
        }

        return {width, height};
    }

    return QProxyStyle::sizeFromContents(type, option, size, widget);
}

QRect FreeCADStyle::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
{
    if (element == SE_LineEditContents) {
        // Qt sets lineWidth = 0 on the inner QLineEdit of QAbstractSpinBox (setFrame(false)).
        // In that case, the spinbox itself manages the edit field rect — do not apply our
        // padding on top of it.
        const auto* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (frameOption && frameOption->lineWidth == 0) {
            return QProxyStyle::subElementRect(element, option, widget);
        }
        const StyleContext context = contextOf(widget, option);
        const BoxGeometryDefinition geometry = resolveBoxGeometry(context);
        return geometry.contentRect(option->rect);
    }

    return QProxyStyle::subElementRect(element, option, widget);
}

QRect FreeCADStyle::subControlRect(
    ComplexControl complexControl,
    const QStyleOptionComplex* option,
    SubControl subControl,
    const QWidget* widget
) const
{
    if (complexControl == CC_ComboBox) {
        const auto* comboOption = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        if (comboOption) {
            const BoxGeometryDefinition geometry = resolveBoxGeometry(contextOf(widget, option));
            const QRect outerRect = option->rect;
            const QRect contentRect = geometry.contentRect(outerRect);
            const int arrowWidth = proxy()->pixelMetric(PM_MenuButtonIndicator, option, widget);

            const int arrowLeft = contentRect.right() - arrowWidth + 1;
            const int editRight = arrowLeft - 1;

            switch (subControl) {
                case SC_ComboBoxFrame:
                    return outerRect;
                case SC_ComboBoxEditField:
                    return QRect(
                        contentRect.left(),
                        contentRect.top(),
                        editRight - contentRect.left() + 1,
                        contentRect.height()
                    );
                case SC_ComboBoxArrow:
                    return QRect(arrowLeft, contentRect.top(), arrowWidth, contentRect.height());
                default:
                    break;
            }
        }
    }

    if (complexControl == CC_SpinBox) {
        const auto* spinOption = qstyleoption_cast<const QStyleOptionSpinBox*>(option);
        if (spinOption) {
            const BoxGeometryDefinition geometry = resolveBoxGeometry(contextOf(widget, option));
            const QRect outerRect = option->rect;
            const QRect contentRect = geometry.contentRect(outerRect);

            // Borrow the button width from the base style; only the position changes.
            const bool hasButtons = spinOption->buttonSymbols != QAbstractSpinBox::NoButtons;
            const int buttonWidth = hasButtons
                ? QProxyStyle::subControlRect(complexControl, option, SC_SpinBoxUp, widget).width()
                : 0;

            const int buttonLeft = contentRect.right() - buttonWidth + 1;
            const int editRight = hasButtons ? buttonLeft - 1 : contentRect.right();

            switch (subControl) {
                case SC_SpinBoxFrame:
                    return outerRect;
                case SC_SpinBoxEditField:
                    return QRect(
                        contentRect.left(),
                        contentRect.top(),
                        editRight - contentRect.left() + 1,
                        contentRect.height()
                    );
                case SC_SpinBoxUp: {
                    if (!hasButtons) {
                        return {};
                    }
                    const int halfHeight = contentRect.height() / 2;
                    return QRect(buttonLeft, contentRect.top(), buttonWidth, halfHeight);
                }
                case SC_SpinBoxDown: {
                    if (!hasButtons) {
                        return {};
                    }
                    const int halfHeight = contentRect.height() / 2;
                    return QRect(
                        buttonLeft,
                        contentRect.top() + halfHeight,
                        buttonWidth,
                        contentRect.height() - halfHeight
                    );
                }
                default:
                    break;
            }
        }
    }

    return QProxyStyle::subControlRect(complexControl, option, subControl, widget);
}

void FreeCADStyle::drawComplexControl(
    ComplexControl control,
    const QStyleOptionComplex* option,
    QPainter* painter,
    const QWidget* widget
) const
{
    if (control == CC_SpinBox) {
        if (const auto* spinOption = qstyleoption_cast<const QStyleOptionSpinBox*>(option)) {
            // Draw our styled background + border for the full frame.
            if (spinOption->frame && (spinOption->subControls & SC_SpinBoxFrame)) {
                const QRect frameRect
                    = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxFrame, widget);
                drawComponent(painter, frameRect, widget, option);
            }

            // Draw spin button arrows on a transparent background (Breeze-style: no
            // separate button fill). We do not delegate to the base style at all — it
            // would re-draw its own frame and button backgrounds on top of ours.
            if (spinOption->buttonSymbols != QAbstractSpinBox::NoButtons) {
                const bool isPlusMinus = spinOption->buttonSymbols == QAbstractSpinBox::PlusMinus;

                const auto drawSpinButton = [&](SubControl subControl,
                                                PrimitiveElement arrowIndicator,
                                                PrimitiveElement plusMinusIndicator) {
                    if (!(spinOption->subControls & subControl)) {
                        return;
                    }
                    QStyleOptionSpinBox buttonOption = *spinOption;
                    buttonOption.rect = proxy()->subControlRect(CC_SpinBox, option, subControl, widget);
                    // Clear the sunken flag unless this specific button is active.
                    if (!(spinOption->activeSubControls & subControl)) {
                        buttonOption.state &= ~State_Sunken;
                    }
                    proxy()->drawPrimitive(
                        isPlusMinus ? plusMinusIndicator : arrowIndicator,
                        &buttonOption,
                        painter,
                        widget
                    );
                };

                drawSpinButton(SC_SpinBoxUp, PE_IndicatorArrowUp, PE_IndicatorSpinPlus);
                drawSpinButton(SC_SpinBoxDown, PE_IndicatorArrowDown, PE_IndicatorSpinMinus);
            }

            return;
        }
    }

    if (control == CC_ComboBox) {
        if (const auto* comboOption = qstyleoption_cast<const QStyleOptionComboBox*>(option)) {
            // Draw our styled background + border for the full frame.
            drawComponent(painter, option->rect, widget, option);

            // Draw the dropdown arrow indicator over the transparent button area.
            // QComboBox::paintEvent draws CE_ComboBoxLabel separately; it uses our
            // subControlRect(SC_ComboBoxEditField) for the text area.
            if (comboOption->subControls & SC_ComboBoxArrow) {
                QStyleOptionComboBox arrowOption = *comboOption;
                arrowOption.rect
                    = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget);
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &arrowOption, painter, widget);
            }

            return;
        }
    }

    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

void FreeCADStyle::drawControl(
    ControlElement element,
    const QStyleOption* option,
    QPainter* painter,
    const QWidget* widget
) const
{
    if (element == CE_PushButtonLabel) {
        if (const auto* btnOption = qstyleoption_cast<const QStyleOptionButton*>(option)) {
            drawPushButtonLabel(painter, btnOption, widget);
            return;
        }
    }

    if (element == CE_ToolButtonLabel) {
        if (const auto* tbOption = qstyleoption_cast<const QStyleOptionToolButton*>(option)) {
            drawToolButtonLabel(painter, tbOption, widget);
            return;
        }
    }

    if (element == CE_ShapedFrame) {
        if (const auto* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option)) {
            const QFrame::Shape shape = frameOption->frameShape;
            if (shape == QFrame::HLine || shape == QFrame::VLine) {
                drawSeparatorLine(painter, option->rect, shape == QFrame::HLine);
                return;
            }
        }
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}

void FreeCADStyle::drawToolButtonLabel(
    QPainter* painter,
    const QStyleOptionToolButton* option,
    const QWidget* widget
) const
{
    const StyleContext context = contextOf(widget, option);
    const BoxGeometryDefinition geometry = resolveBoxGeometry(context);
    const QRect contentRect = geometry.contentRect(option->rect);

    const Qt::ToolButtonStyle tbStyle = option->toolButtonStyle;
    const bool hasIconOrArrow = !option->icon.isNull() || option->arrowType != Qt::NoArrow;
    const bool needsCustomLayout = hasIconOrArrow && !option->text.isEmpty()
        && (tbStyle == Qt::ToolButtonTextBesideIcon || tbStyle == Qt::ToolButtonTextUnderIcon);

    if (!needsCustomLayout) {
        // Icon-only, text-only, FollowStyle, etc.: delegate to parent unchanged.
        // The parent handles its own internal spacing; we must not inset the rect
        // because the button may be parent-sized (not padded) and inset would go negative.
        QProxyStyle::drawControl(CE_ToolButtonLabel, option, painter, widget);
        return;
    }

    const int iconSpacing = geometry.iconSpacing;

    // Apply pressed/checked shift — we manage layout so we do this ourselves.
    QRect shiftedContentRect = contentRect;
    if (option->state & (State_Sunken | State_On)) {
        shiftedContentRect.translate(
            proxy()->pixelMetric(PM_ButtonShiftHorizontal, option, widget),
            proxy()->pixelMetric(PM_ButtonShiftVertical, option, widget)
        );
    }

    const bool hasArrow = option->arrowType != Qt::NoArrow;

    QPixmap pixmap;
    QSize pixmapSize = option->iconSize;
    if (!hasArrow && !option->icon.isNull()) {
        const QIcon::State iconState = (option->state & State_On) ? QIcon::On : QIcon::Off;
        QIcon::Mode iconMode = QIcon::Normal;
        if (!(option->state & State_Enabled)) {
            iconMode = QIcon::Disabled;
        }
        else if ((option->state & State_MouseOver) && (option->state & State_AutoRaise)) {
            iconMode = QIcon::Active;
        }
        pixmap = option->icon.pixmap(
            shiftedContentRect.size().boundedTo(option->iconSize),
            painter->device()->devicePixelRatio(),
            iconMode,
            iconState
        );
        pixmapSize = pixmap.size() / painter->device()->devicePixelRatio();
    }

    const auto drawArrowInRect = [&](const QRect& arrowRect) {
        QStyleOption arrowOpt(*option);
        arrowOpt.rect = arrowRect;
        PrimitiveElement primitive = PE_IndicatorArrowDown;
        switch (option->arrowType) {
            case Qt::LeftArrow:
                primitive = PE_IndicatorArrowLeft;
                break;
            case Qt::RightArrow:
                primitive = PE_IndicatorArrowRight;
                break;
            case Qt::UpArrow:
                primitive = PE_IndicatorArrowUp;
                break;
            default:
                break;
        }
        proxy()->drawPrimitive(primitive, &arrowOpt, painter, widget);
    };

    int textFlags = Qt::TextShowMnemonic;
    if (!proxy()->styleHint(SH_UnderlineShortcut, option, widget)) {
        textFlags |= Qt::TextHideMnemonic;
    }

    painter->save();
    painter->setFont(option->font);

    if (tbStyle == Qt::ToolButtonTextBesideIcon) {
        const QRect iconRect(
            shiftedContentRect.left(),
            shiftedContentRect.top() + (shiftedContentRect.height() - pixmapSize.height()) / 2,
            pixmapSize.width(),
            pixmapSize.height()
        );
        const QRect textRect = shiftedContentRect.adjusted(pixmapSize.width() + iconSpacing, 0, 0, 0);

        if (hasArrow) {
            drawArrowInRect(iconRect);
        }
        else {
            proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);
        }
        proxy()->drawItemText(
            painter,
            QStyle::visualRect(option->direction, shiftedContentRect, textRect),
            textFlags | Qt::AlignLeft | Qt::AlignVCenter,
            option->palette,
            option->state & State_Enabled,
            option->text,
            QPalette::ButtonText
        );
    }
    else {
        // Qt::ToolButtonTextUnderIcon
        const int fontHeight = option->fontMetrics.height();
        const QRect iconRect = shiftedContentRect.adjusted(0, 0, 0, -(fontHeight + iconSpacing));
        const QRect textRect(
            shiftedContentRect.left(),
            iconRect.bottom() + 1 + iconSpacing,
            shiftedContentRect.width(),
            fontHeight
        );

        if (hasArrow) {
            drawArrowInRect(iconRect);
        }
        else {
            proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);
        }
        proxy()->drawItemText(
            painter,
            QStyle::visualRect(option->direction, shiftedContentRect, textRect),
            textFlags | Qt::AlignHCenter | Qt::AlignTop,
            option->palette,
            option->state & State_Enabled,
            option->text,
            QPalette::ButtonText
        );
    }

    painter->restore();
}

void FreeCADStyle::drawPushButtonLabel(
    QPainter* painter,
    const QStyleOptionButton* option,
    const QWidget* widget
) const
{
    const StyleContext context = contextOf(widget, option);
    const BoxGeometryDefinition geometry = resolveBoxGeometry(context);

    // option->rect at this point is SE_PushButtonContents from Fusion (inset by its own frame
    // width), which doesn't reflect our token-based padding. Use widget->rect() — the true
    // button rect — as the base, then apply token padding to derive the content area.
    // This is consistent with CT_PushButton in sizeFromContents, which also computes the total
    // size as content + token padding (not Fusion's frame).
    const QRect buttonRect = widget ? widget->rect() : option->rect;
    const QRect contentRect = geometry.contentRect(buttonRect);

    // For icon-only or text-only, delegate to parent with the token-padded content rect.
    // The parent centers the content within this rect; press-state shift is left to the parent.
    if (option->icon.isNull() || option->text.isEmpty()) {
        QStyleOptionButton adjustedOption = *option;
        adjustedOption.rect = contentRect;
        QProxyStyle::drawControl(CE_PushButtonLabel, &adjustedOption, painter, widget);
        return;
    }

    // Icon + text: custom layout with token icon spacing.
    QRect shiftedContentRect = contentRect;
    if (option->state & (State_Sunken | State_On)) {
        shiftedContentRect.translate(
            proxy()->pixelMetric(PM_ButtonShiftHorizontal, option, widget),
            proxy()->pixelMetric(PM_ButtonShiftVertical, option, widget)
        );
    }

    const int iconSpacing = geometry.iconSpacing;

    const QIcon::State iconState = (option->state & State_On) ? QIcon::On : QIcon::Off;
    const QIcon::Mode iconMode = (option->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled;
    const QPixmap pixmap = option->icon.pixmap(
        shiftedContentRect.size().boundedTo(option->iconSize),
        painter->device()->devicePixelRatio(),
        iconMode,
        iconState
    );
    const QSize pixmapSize = pixmap.size() / painter->device()->devicePixelRatio();

    // Center the icon+text group horizontally in the content rect.
    const int textWidth = option->fontMetrics.horizontalAdvance(option->text);
    const int groupWidth = pixmapSize.width() + iconSpacing + textWidth;
    const int groupLeft = shiftedContentRect.left() + (shiftedContentRect.width() - groupWidth) / 2;

    const QRect iconRect(
        groupLeft,
        shiftedContentRect.top() + (shiftedContentRect.height() - pixmapSize.height()) / 2,
        pixmapSize.width(),
        pixmapSize.height()
    );
    const QRect textRect(
        groupLeft + pixmapSize.width() + iconSpacing,
        shiftedContentRect.top(),
        shiftedContentRect.right() - (groupLeft + pixmapSize.width() + iconSpacing),
        shiftedContentRect.height()
    );

    int textFlags = Qt::TextShowMnemonic | Qt::AlignVCenter | Qt::AlignLeft;
    if (!proxy()->styleHint(SH_UnderlineShortcut, option, widget)) {
        textFlags |= Qt::TextHideMnemonic;
    }

    painter->save();
    proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);
    proxy()->drawItemText(
        painter,
        QStyle::visualRect(option->direction, shiftedContentRect, textRect),
        textFlags,
        option->palette,
        option->state & State_Enabled,
        option->text,
        QPalette::ButtonText
    );
    painter->restore();
}

std::optional<StyleParameters::Value> FreeCADStyle::resolve(std::string_view name) const
{
    return Application::Instance->styleParameterManager()->resolve(std::string(name));
}

std::optional<StyleParameters::Value> FreeCADStyle::resolve(
    std::initializer_list<std::string_view> names
) const
{
    for (const std::string_view name : names) {
        if (auto value = resolve(name)) {
            return value;
        }
    }
    return std::nullopt;
}

std::optional<StyleParameters::Value> FreeCADStyle::resolve(
    std::initializer_list<std::string_view> prefixes,
    std::string_view suffix
) const
{
    for (const std::string_view prefix : prefixes) {
        if (auto value = resolve(std::string(prefix) + std::string(suffix))) {
            return value;
        }
    }
    return std::nullopt;
}

StyleContext FreeCADStyle::contextOf(const QWidget* widget, const QStyleOption* option)
{
    StyleContext context;

    if (qobject_cast<const QToolButton*>(widget)) {
        context.component = StyleComponent::ToolButton;
    }
    else if (qobject_cast<const QPushButton*>(widget)) {
        context.component = StyleComponent::PushButton;
    }
    else if (qobject_cast<const QLineEdit*>(widget) || qobject_cast<const QAbstractSpinBox*>(widget)) {
        context.component = StyleComponent::LineEdit;
    }
    else if (qobject_cast<const QTextEdit*>(widget) || qobject_cast<const QPlainTextEdit*>(widget)) {
        context.component = StyleComponent::TextEdit;
    }
    else if (const auto* comboBox = qobject_cast<const QComboBox*>(widget)) {
        context.component = comboBox->isEditable() ? StyleComponent::ComboBox
                                                   : StyleComponent::Select;
    }

    // ButtonType — derived from style option features first, then widget properties.
    const auto* buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
    if (buttonOption && (buttonOption->features & QStyleOptionButton::DefaultButton)) {
        context.variant.set(VariantSlot::ButtonType, ButtonType::Primary);
    }
    else if (buttonOption && (buttonOption->features & QStyleOptionButton::Flat)) {
        context.variant.set(VariantSlot::ButtonType, ButtonType::Link);
    }
    else if (const auto* toolButton = qobject_cast<const QToolButton*>(widget);
             toolButton && toolButton->autoRaise()) {
        context.variant.set(VariantSlot::ButtonType, ButtonType::Link);
    }
    else if (widget && widget->property("flat").toBool()) {
        context.variant.set(VariantSlot::ButtonType, ButtonType::Link);
    }

    // ControlSize — derived from the "controlSize" widget property.
    if (widget) {
        const QString sizeName = widget->property("controlSize").toString();
        if (sizeName == u"small") {
            context.variant.set(VariantSlot::ControlSize, ControlSize::Small);
        }
        else if (sizeName == u"big") {
            context.variant.set(VariantSlot::ControlSize, ControlSize::Big);
        }
    }

    // State — all active flags captured as a bitmask.
    if (option) {
        // State_Sunken means "button is being pressed" for buttons, but "has a sunken
        // frame appearance" for input widgets (QLineEdit always sets it). Only map it
        // to Pressed for button components to avoid masking the Focused state on inputs.
        const bool isButton = context.component == StyleComponent::PushButton
            || context.component == StyleComponent::ToolButton
            || context.component == StyleComponent::Select;
        if (isButton && (option->state & QStyle::State_Sunken)) {
            context.state |= StyleState::Pressed;
        }
        if (option->state & QStyle::State_MouseOver) {
            context.state |= StyleState::Hovered;
        }
        if (option->state & QStyle::State_On) {
            context.state |= StyleState::Checked;
        }
        if (option->state & QStyle::State_HasFocus) {
            context.state |= StyleState::Focused;
        }
    }

    // QAbstractSpinBox delegates keyboard focus to an inner QLineEdit child, so
    // the spinbox widget's hasFocus() returns false and State_HasFocus is absent
    // from its style option. Supplement the state by checking the inner edit directly.
    if (qobject_cast<const QAbstractSpinBox*>(widget)) {
        if (const QLineEdit* innerEdit = widget->findChild<QLineEdit*>()) {
            if (innerEdit->hasFocus()) {
                context.state |= StyleState::Focused;
            }
        }
    }

    // An editable QComboBox also delegates keyboard focus to its inner QLineEdit.
    // Same pattern as QAbstractSpinBox: supplement state from the inner edit.
    if (const auto* comboBox = qobject_cast<const QComboBox*>(widget);
        comboBox && comboBox->isEditable()) {
        if (const QLineEdit* lineEdit = comboBox->lineEdit()) {
            if (lineEdit->hasFocus()) {
                context.state |= StyleState::Focused;
            }
        }
    }

    return context;
}

std::optional<StyleParameters::Value> FreeCADStyle::resolve(
    const StyleContext& context,
    StyleProperty property
) const
{
    const uint32_t key = packCacheKey(context, property);

    if (const auto found = tokenCache.find(key); found != tokenCache.end()) {
        return found->second;
    }

    const std::vector<std::string> prefixes = buildPrefixes(context);
    const std::string_view propertySuffix = propertyString(property);

    std::optional<StyleParameters::Value> result;
    for (const std::string& prefix : prefixes) {
        result = resolve(prefix + std::string(propertySuffix));
        if (result) {
            break;
        }
    }

    tokenCache.emplace(key, result);
    return result;
}

FreeCADStyle::BoxStyleDefinition FreeCADStyle::resolveBoxStyle(const StyleContext& context) const
{
    BoxStyleDefinition result;

    if (const auto backgroundValue = resolve(context, StyleProperty::Background)) {
        result.background = Base::convertTo<QBrush>(*backgroundValue);
    }

    if (const auto overlay = resolve<Base::Color>(context, StyleProperty::Overlay)) {
        result.overlay = overlay->asValue<QColor>();
    }

    if (const auto borderColor = resolve<Base::Color>(context, StyleProperty::BorderColor)) {
        result.borderColor = borderColor->asValue<QColor>();
    }

    if (const auto borderThickness
        = resolve<StyleParameters::Insets>(context, StyleProperty::BorderThickness)) {
        result.borderThickness = Base::convertTo<QMarginsF>(*borderThickness);
    }

    if (const auto borderRadius
        = resolve<StyleParameters::Corners>(context, StyleProperty::BorderRadius)) {
        result.borderRadius = Base::convertTo<CornerRadii>(*borderRadius);
    }

    if (const auto innerShadow
        = resolve<StyleParameters::InnerShadow>(context, StyleProperty::InnerShadow)) {
        result.innerShadow = Base::convertTo<InnerShadow>(*innerShadow);
    }

    return result;
}

FreeCADStyle::BoxGeometryDefinition FreeCADStyle::resolveBoxGeometry(const StyleContext& context) const
{
    BoxGeometryDefinition result;

    if (const auto padding = resolve<StyleParameters::Insets>(context, StyleProperty::Padding)) {
        result.padding = Base::convertTo<QMarginsF>(*padding);
    }

    if (const auto height = resolve<StyleParameters::Numeric>(context, StyleProperty::Height)) {
        result.height = static_cast<int>(height->value);
    }

    if (const auto minWidth = resolve<StyleParameters::Numeric>(context, StyleProperty::MinWidth)) {
        result.minWidth = static_cast<int>(minWidth->value);
    }

    if (const auto spacing = resolve<StyleParameters::Numeric>(context, StyleProperty::IconSpacing)) {
        result.iconSpacing = static_cast<int>(spacing->value);
    }

    return result;
}

void FreeCADStyle::drawComponent(QPainter* painter, const QRect& rect, const StyleContext& context) const
{
    drawBoxBackground(painter, rect, resolveBoxStyle(context));
}

void FreeCADStyle::drawComponent(
    QPainter* painter,
    const QRect& rect,
    const QWidget* widget,
    const QStyleOption* option
) const
{
    drawComponent(painter, rect, contextOf(widget, option));
}

void FreeCADStyle::drawSeparatorLine(QPainter* painter, const QRect& rect, bool isHorizontal) const
{
    int thickness = 1;
    if (const auto numeric = resolve<StyleParameters::Numeric>("SeparatorThickness")) {
        thickness = static_cast<int>(numeric->value);
    }
    if (const auto color = resolve<Base::Color>("SeparatorColor")) {
        const QRect lineRect = isHorizontal
            ? QRect(rect.left(), rect.center().y() - (thickness / 2), rect.width(), thickness)
            : QRect(rect.center().x() - (thickness / 2), rect.top(), thickness, rect.height());
        painter->fillRect(lineRect, color->asValue<QColor>());
    }
}

void FreeCADStyle::clearTokenCache()
{
    tokenCache.clear();
}

bool FreeCADStyle::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == ThemeReloadEvent::registeredType()) {
        clearTokenCache();
        for (QWidget* widget : QApplication::allWidgets()) {
            widget->update();
        }
        return false;  // Let ThemeReloadHandler in Application also process the event
    }

    if (event->type() == QEvent::Polish) {
        if (auto* groupBox = qobject_cast<QGroupBox*>(obj)) {
            if (auto* layout = groupBox->layout()) {
                layout->setContentsMargins(0, 0, 0, 0);
            }
        }

        // Apply token padding to QTextEdit / QPlainTextEdit via the document margin.
        // This pads the text content relative to the viewport while leaving scrollbars
        // flush with the frame edge (unlike viewport-margin approaches).
        const auto applyTextEditDocumentMargin = [this](QWidget* widget, QTextDocument* document) {
            const StyleContext context = contextOf(widget);
            const BoxGeometryDefinition geometry = resolveBoxGeometry(context);
            document->setDocumentMargin(geometry.padding.left());
        };

        if (auto* textEdit = qobject_cast<QTextEdit*>(obj)) {
            applyTextEditDocumentMargin(textEdit, textEdit->document());
        }
        else if (auto* plainTextEdit = qobject_cast<QPlainTextEdit*>(obj)) {
            applyTextEditDocumentMargin(plainTextEdit, plainTextEdit->document());
        }
    }

    return QObject::eventFilter(obj, event);
}
