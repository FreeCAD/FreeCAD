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
#include <QGroupBox>
#include <QImage>
#include <QLayout>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>
#include <QStyleOption>

#include <Base/Color.h>
#include <Base/Exception.h>

#include "Application.h"
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

}  // namespace

void FreeCADStyle::drawBoxBackground(QPainter* painter, const QRect& rect, const BoxBackground& rule)
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

    if (hasBorder) {
        const QMarginsF& thickness = *rule.borderThickness;

        // Snap each border side to the nearest integer pixel.
        const QRect innerRect = rect.adjusted(
            qRound(thickness.left()),
            qRound(thickness.top()),
            -qRound(thickness.right()),
            -qRound(thickness.bottom())
        );
        // Rounded margins used for inner corner radius computation.
        const QMarginsF snappedThickness(
            qRound(thickness.left()),
            qRound(thickness.top()),
            qRound(thickness.right()),
            qRound(thickness.bottom())
        );
        const CornerRadii innerRadiusValues = innerRadii(rule.borderRadius, snappedThickness);

        // 1. Fill outer rect with border colour.
        painter->fillPath(roundedRectPath(QRectF(rect), rule.borderRadius), QBrush(*rule.borderColor));

        // 2. Fill inner rect with background.
        painter->fillPath(roundedRectPath(QRectF(innerRect), innerRadiusValues), rule.background);

        // 3. Fill inner rect with overlay (background only, not border).
        if (rule.overlay) {
            painter->fillPath(roundedRectPath(QRectF(innerRect), innerRadiusValues), *rule.overlay);
        }
    }
    else {
        painter->fillPath(roundedRectPath(QRectF(rect), rule.borderRadius), rule.background);

        if (rule.overlay) {
            painter->fillPath(roundedRectPath(QRectF(rect), rule.borderRadius), *rule.overlay);
        }
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

void FreeCADStyle::drawPrimitive(
    PrimitiveElement element,
    const QStyleOption* option,
    QPainter* painter,
    const QWidget* widget
) const
{
    if (element == PE_PanelButtonCommand) {

        const std::string state = [&option]() -> std::string {
            if (option->state & State_Sunken) {
                return "Pressed";
            }

            if (option->state & State_On) {
                return "Checked";
            }

            if (option->state & State_MouseOver) {
                return "Hover";
            }

            return "";
        }();

        const std::string type = [&option]() -> std::string {
            if (auto* styleOptionButton = qstyleoption_cast<const QStyleOptionButton*>(option);
                styleOptionButton && styleOptionButton->features & QStyleOptionButton::DefaultButton) {
                return "Primary";
            }

            return "";
        }();

        drawBoxBackground(
            painter,
            option->rect,
            resolveBoxBackground(
                {fmt::format("Button{}{}", type, state),
                 fmt::format("Button{}", type),
                 fmt::format("Button{}", state),
                 "Button"}
            )
        );
        return;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

std::string FreeCADStyle::controlSizeSuffix(const QWidget* widget)
{
    if (!widget) {
        return {};
    }
    const QString sizeName = widget->property("controlSize").toString();
    if (sizeName.isEmpty()) {
        return {};
    }
    return (sizeName.left(1).toUpper() + sizeName.mid(1)).toStdString();
}

QSize FreeCADStyle::sizeFromContents(
    ContentsType type,
    const QStyleOption* option,
    const QSize& size,
    const QWidget* widget
) const
{
    if (type == CT_ToolButton) {
        const std::string sizedPrefix = fmt::format("ToolButton{}", controlSizeSuffix(widget));

        const auto* tbOption = qstyleoption_cast<const QStyleOptionToolButton*>(option);
        const bool hasIconOrArrow = tbOption
            && (!tbOption->icon.isNull() || tbOption->arrowType != Qt::NoArrow);
        const bool needsCustomLayout = hasIconOrArrow && tbOption && !tbOption->text.isEmpty()
            && (tbOption->toolButtonStyle == Qt::ToolButtonTextBesideIcon
                || tbOption->toolButtonStyle == Qt::ToolButtonTextUnderIcon);

        QMarginsF paddingF;
        if (const auto paddingValue = resolve({sizedPrefix, "ToolButton"}, "Padding")) {
            try {
                paddingF = toMarginsF(StyleParameters::Padding(*paddingValue));
            }
            catch (const Base::Exception&) {
            }
        }

        int width = size.width() + static_cast<int>(paddingF.left() + paddingF.right());
        int height = size.height() + static_cast<int>(paddingF.top() + paddingF.bottom());

        if (needsCustomLayout) {
            // ToolButton{Size}IconSpacing → ToolButtonIconSpacing → ButtonIconSpacing
            int iconSpacing = 4;  // Qt's built-in default (see QToolButton::sizeHint)
            if (const auto spacingValue
                = resolve({sizedPrefix, "ToolButton", "Button"}, "IconSpacing")) {
                if (spacingValue->holds<StyleParameters::Numeric>()) {
                    iconSpacing = static_cast<int>(spacingValue->get<StyleParameters::Numeric>().value);
                }
            }

            // Qt hardcodes +4 as the icon-text gap in QToolButton::sizeHint's content size.
            // Replace that with our spacing so the widget is wide enough.
            if (tbOption->toolButtonStyle == Qt::ToolButtonTextBesideIcon) {
                constexpr int qtBuiltInIconGap = 4;
                width += iconSpacing - qtBuiltInIconGap;
            }
        }

        if (const auto heightValue = resolve({sizedPrefix, "ToolButton"}, "Height")) {
            if (heightValue->holds<StyleParameters::Numeric>()) {
                height = static_cast<int>(heightValue->get<StyleParameters::Numeric>().value);
            }
        }

        return QSize(width, height);
    }

    return QProxyStyle::sizeFromContents(type, option, size, widget);
}

QRect FreeCADStyle::subControlRect(
    ComplexControl complexControl,
    const QStyleOptionComplex* option,
    SubControl subControl,
    const QWidget* widget
) const
{
    return QProxyStyle::subControlRect(complexControl, option, subControl, widget);
}

void FreeCADStyle::drawControl(
    ControlElement element,
    const QStyleOption* option,
    QPainter* painter,
    const QWidget* widget
) const
{
    if (element == CE_ToolButtonLabel) {
        if (const auto* toolButtonOption = qstyleoption_cast<const QStyleOptionToolButton*>(option)) {
            const std::string sizedPrefix = fmt::format("ToolButton{}", controlSizeSuffix(widget));

            QMarginsF paddingF;
            if (const auto paddingValue = resolve({sizedPrefix, "ToolButton"}, "Padding")) {
                try {
                    paddingF = toMarginsF(StyleParameters::Padding(*paddingValue));
                }
                catch (const Base::Exception&) {
                }
            }

            const QRect contentRect = toolButtonOption->rect.adjusted(
                static_cast<int>(paddingF.left()),
                static_cast<int>(paddingF.top()),
                -static_cast<int>(paddingF.right()),
                -static_cast<int>(paddingF.bottom())
            );

            const Qt::ToolButtonStyle tbStyle = toolButtonOption->toolButtonStyle;
            const bool hasIconOrArrow = !toolButtonOption->icon.isNull()
                || toolButtonOption->arrowType != Qt::NoArrow;
            const bool needsCustomLayout = hasIconOrArrow && !toolButtonOption->text.isEmpty()
                && (tbStyle == Qt::ToolButtonTextBesideIcon
                    || tbStyle == Qt::ToolButtonTextUnderIcon);

            if (!needsCustomLayout) {
                // Icon-only, text-only, FollowStyle, etc.: delegate to parent unchanged.
                // The parent handles its own internal spacing; we must not inset the rect
                // because the button may be parent-sized (not padded) and inset would go negative.
                QProxyStyle::drawControl(element, option, painter, widget);
                return;
            }

            // ToolButton{Size}IconSpacing → ToolButtonIconSpacing → ButtonIconSpacing
            int iconSpacing = 4;  // matches Qt's built-in default
            if (const auto spacingValue
                = resolve({sizedPrefix, "ToolButton", "Button"}, "IconSpacing")) {
                if (spacingValue->holds<StyleParameters::Numeric>()) {
                    iconSpacing = static_cast<int>(spacingValue->get<StyleParameters::Numeric>().value);
                }
            }

            // Apply pressed/checked shift — we manage layout so we do this ourselves.
            QRect shiftedContentRect = contentRect;
            if (toolButtonOption->state & (State_Sunken | State_On)) {
                shiftedContentRect.translate(
                    proxy()->pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                    proxy()->pixelMetric(PM_ButtonShiftVertical, option, widget)
                );
            }

            const bool hasArrow = toolButtonOption->arrowType != Qt::NoArrow;

            QPixmap pixmap;
            QSize pixmapSize = toolButtonOption->iconSize;
            if (!hasArrow && !toolButtonOption->icon.isNull()) {
                const QIcon::State iconState = (toolButtonOption->state & State_On) ? QIcon::On
                                                                                    : QIcon::Off;
                QIcon::Mode iconMode = QIcon::Normal;
                if (!(toolButtonOption->state & State_Enabled)) {
                    iconMode = QIcon::Disabled;
                }
                else if ((toolButtonOption->state & State_MouseOver)
                         && (toolButtonOption->state & State_AutoRaise)) {
                    iconMode = QIcon::Active;
                }
                pixmap = toolButtonOption->icon.pixmap(
                    shiftedContentRect.size().boundedTo(toolButtonOption->iconSize),
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
                switch (toolButtonOption->arrowType) {
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
            painter->setFont(toolButtonOption->font);

            if (tbStyle == Qt::ToolButtonTextBesideIcon) {
                const QRect iconRect(
                    shiftedContentRect.left(),
                    shiftedContentRect.top() + (shiftedContentRect.height() - pixmapSize.height()) / 2,
                    pixmapSize.width(),
                    pixmapSize.height()
                );
                const QRect textRect
                    = shiftedContentRect.adjusted(pixmapSize.width() + iconSpacing, 0, 0, 0);

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
                    toolButtonOption->palette,
                    toolButtonOption->state & State_Enabled,
                    toolButtonOption->text,
                    QPalette::ButtonText
                );
            }
            else {
                // Qt::ToolButtonTextUnderIcon
                const int fontHeight = toolButtonOption->fontMetrics.height();
                const QRect iconRect
                    = shiftedContentRect.adjusted(0, 0, 0, -(fontHeight + iconSpacing));
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
                    toolButtonOption->palette,
                    toolButtonOption->state & State_Enabled,
                    toolButtonOption->text,
                    QPalette::ButtonText
                );
            }

            painter->restore();
            return;
        }
    }

    QProxyStyle::drawControl(element, option, painter, widget);
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

QColor FreeCADStyle::toQColor(const Base::Color& color)
{
    return QColor::fromRgbF(color.r, color.g, color.b, color.a);
}

FreeCADStyle::CornerRadii FreeCADStyle::toCornerRadii(const StyleParameters::Corners& corners)
{
    return {
        .topLeft = corners.topLeft().value,
        .topRight = corners.topRight().value,
        .bottomRight = corners.bottomRight().value,
        .bottomLeft = corners.bottomLeft().value,
    };
}

QMarginsF FreeCADStyle::toMarginsF(const StyleParameters::Insets& insets)
{
    return QMarginsF(insets.left().value, insets.top().value, insets.right().value, insets.bottom().value);
}

FreeCADStyle::InnerShadow FreeCADStyle::toInnerShadow(const StyleParameters::InnerShadow& shadow)
{
    return {
        .color = shadow.color().asValue<QColor>(),
        .x = shadow.x(),
        .y = shadow.y(),
        .blur = shadow.blur(),
    };
}

QBrush FreeCADStyle::toBackgroundBrush(const StyleParameters::Value& value)
{
    if (value.holds<Base::Color>()) {
        return QBrush(value.get<Base::Color>().asValue<QColor>());
    }

    if (!value.holds<StyleParameters::Tuple>()) {
        return Qt::NoBrush;
    }

    const auto& tuple = value.get<StyleParameters::Tuple>();

    if (tuple.kind == StyleParameters::TupleKind::LinearGradient) {
        try {
            const StyleParameters::LinearGradient gradient(tuple);
            QLinearGradient qGradient(gradient.x1(), gradient.y1(), gradient.x2(), gradient.y2());
            qGradient.setCoordinateMode(QGradient::ObjectMode);
            for (const auto& stop : gradient.colorStops()) {
                qGradient.setColorAt(stop.position.value, stop.color.asValue<QColor>());
            }
            return QBrush(qGradient);
        }
        catch (const Base::Exception&) {
            return Qt::NoBrush;
        }
    }

    if (tuple.kind == StyleParameters::TupleKind::RadialGradient) {
        try {
            const StyleParameters::RadialGradient gradient(tuple);
            QRadialGradient qGradient(
                gradient.cx(),
                gradient.cy(),
                gradient.radius(),
                gradient.fx(),
                gradient.fy()
            );
            qGradient.setCoordinateMode(QGradient::ObjectMode);
            for (const auto& stop : gradient.colorStops()) {
                qGradient.setColorAt(stop.position.value, stop.color.asValue<QColor>());
            }
            return QBrush(qGradient);
        }
        catch (const Base::Exception&) {
            return Qt::NoBrush;
        }
    }

    return Qt::NoBrush;
}

FreeCADStyle::BoxBackground FreeCADStyle::resolveBoxBackground(
    std::initializer_list<std::string_view> prefixes
) const
{
    BoxBackground result;

    if (auto backgroundValue = resolve(prefixes, "Background")) {
        result.background = toBackgroundBrush(*backgroundValue);
    }

    if (auto overlayValue = resolve(prefixes, "Overlay")) {
        if (overlayValue->holds<Base::Color>()) {
            result.overlay = overlayValue->get<Base::Color>().asValue<QColor>();
        }
    }

    if (auto borderColorValue = resolve(prefixes, "BorderColor")) {
        if (borderColorValue->holds<Base::Color>()) {
            result.borderColor = borderColorValue->get<Base::Color>().asValue<QColor>();
        }
    }

    if (auto borderThicknessValue = resolve(prefixes, "BorderThickness")) {
        try {
            result.borderThickness = toMarginsF(
                StyleParameters::BorderThickness(*borderThicknessValue)
            );
        }
        catch (const Base::Exception&) {
        }
    }

    if (auto borderRadiusValue = resolve(prefixes, "BorderRadius")) {
        try {
            result.borderRadius = toCornerRadii(StyleParameters::Corners(*borderRadiusValue));
        }
        catch (const Base::Exception&) {
        }
    }

    if (auto innerShadowValue = resolve(prefixes, "InnerShadow")) {
        try {
            result.innerShadow = toInnerShadow(StyleParameters::InnerShadow(*innerShadowValue));
        }
        catch (const Base::Exception&) {
        }
    }

    return result;
}

bool FreeCADStyle::eventFilter(QObject* obj, QEvent* event)
{
    // This is a hacky fix for https://github.com/FreeCAD/FreeCAD/issues/23607
    // Basically after widget is shown or polished we enforce it's minimum size to at least cover
    // the minimum size hint - something that QSS ignores if min-width is specified
    if (event->type() == QEvent::Polish || event->type() == QEvent::Show) {
        if (auto* btn = qobject_cast<QPushButton*>(obj)) {
            btn->setMinimumWidth(std::max(btn->minimumSizeHint().width(), btn->minimumWidth()));
        }
    }

    if (event->type() == QEvent::Polish) {
        if (auto* groupBox = qobject_cast<QGroupBox*>(obj)) {
            if (auto* layout = groupBox->layout()) {
                layout->setContentsMargins(0, 0, 0, 0);
            }
        }

        if (auto* toolButton = qobject_cast<QToolButton*>(obj)) {
            const std::string sizedPrefix = fmt::format("ToolButton{}", controlSizeSuffix(toolButton));
            if (const auto heightValue = resolve({sizedPrefix, "ToolButton"}, "Height")) {
                if (heightValue->holds<StyleParameters::Numeric>()) {
                    toolButton->setFixedHeight(
                        static_cast<int>(heightValue->get<StyleParameters::Numeric>().value)
                    );
                }
            }
        }
    }

    return QObject::eventFilter(obj, event);
}
