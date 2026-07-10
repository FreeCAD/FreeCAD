// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2009 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <FCConfig.h>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QStringList>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/misc/SoState.h>

#include <Inventor/SbVec2f.h>
#include <Inventor/C/basic.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>

#include <algorithm>

#include "SoLabelNodes.h"
#include "SoFCInteractiveElement.h"
#include "Tools.h"


using namespace Gui;


SO_NODE_SOURCE(SoColorBarLabel)

void SoColorBarLabel::initClass()
{
    SO_NODE_INIT_CLASS(SoColorBarLabel, SoText2, "Text2");
}

SoColorBarLabel::SoColorBarLabel()
{
    SO_NODE_CONSTRUCTOR(SoColorBarLabel);
}

void SoColorBarLabel::computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center)
{
    inherited::computeBBox(action, box, center);
    if (!box.hasVolume()) {
        SbViewVolume vv = SoViewVolumeElement::get(action->getState());
        // workaround for https://github.com/coin3d/coin/issues/417:
        // extend by 2 percent
        vv.scaleWidth(1.02f);
        SoViewVolumeElement::set(action->getState(), this, vv);
        inherited::computeBBox(action, box, center);
    }
}

// ------------------------------------------------------

SO_NODE_SOURCE(SoStringLabel)

void SoStringLabel::initClass()
{
    SO_NODE_INIT_CLASS(SoStringLabel, SoNode, "Node");
}

SoStringLabel::SoStringLabel()
{
    SO_NODE_CONSTRUCTOR(SoStringLabel);
    SO_NODE_ADD_FIELD(string, (""));
    SO_NODE_ADD_FIELD(textColor, (SbVec3f(1.0f, 1.0f, 1.0f)));
    SO_NODE_ADD_FIELD(name, ("Helvetica"));
    SO_NODE_ADD_FIELD(size, (12));

    textSwitch = new SoSwitch;
    textSwitch->ref();

    textSeparator = new SoSeparator;

    textTexture = new SoTexture2;
    textTexture->wrapS = SoTexture2::CLAMP;
    textTexture->wrapT = SoTexture2::CLAMP;
    textTexture->model = SoTexture2::MODULATE;
    textSeparator->addChild(textTexture);

    textVertexProperty = new SoVertexProperty;
    textFaceSet = new SoFaceSet;
    textFaceSet->vertexProperty.setValue(textVertexProperty);
    textFaceSet->numVertices.set1Value(0, 4);
    textSeparator->addChild(textFaceSet);

    textSwitch->addChild(textSeparator);
    textSwitch->whichChild.setValue(SO_SWITCH_NONE);
}

/**
 * Renders the open edges only.
 */
void SoStringLabel::GLRender(SoGLRenderAction* action)
{
    if (!action || !textSwitch) {
        return;
    }

    SoState* state = action->getState();
    if (!state) {
        return;
    }

    state->push();
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);

    ensureTextGeometry(state);

    if (textSwitch->whichChild.getValue() == SO_SWITCH_NONE) {
        state->pop();
        return;
    }

    const SbViewportRegion& vp = SoViewportRegionElement::get(state);
    SbVec2s vpsize = vp.getViewportSizePixels();

    SbViewVolume ortho;
    ortho.ortho(0.0f, static_cast<float>(vpsize[0]), 0.0f, static_cast<float>(vpsize[1]), -1.0f, 1.0f);
    SbMatrix affine;
    SbMatrix projection;
    ortho.getMatrices(affine, projection);

    SoModelMatrixElement::set(state, this, SbMatrix::identity());
    SoViewingMatrixElement::set(state, this, SbMatrix::identity());
    SoProjectionMatrixElement::set(state, this, projection);
    SoViewVolumeElement::set(state, this, ortho);

    SoDepthBufferElement::set(state, FALSE, FALSE, SoDepthBufferElement::ALWAYS, SbVec2f(0.0f, 1.0f));

    SbColor white(1.0f, 1.0f, 1.0f);
    SoLazyElement::setDiffuse(state, this, 1, &white, 0);
    SoLazyElement::setTransparencyType(state, static_cast<int32_t>(SoGLRenderAction::BLEND));
    SoGLTextureEnabledElement::set(state, this, TRUE);
    SoMultiTextureEnabledElement::set(state, this, FALSE);

    textSwitch->whichChild.setValue(0);
    textSwitch->GLRender(action);

    state->pop();
}

SoStringLabel::~SoStringLabel()
{
    if (textSwitch) {
        textSwitch->unref();
        textSwitch = nullptr;
    }
    textSeparator = nullptr;
    textTexture = nullptr;
    textFaceSet = nullptr;
    textVertexProperty = nullptr;
}

void SoStringLabel::notify(SoNotList* list)
{
    if (list) {
        SoField* f = list->getLastField();
        if (f == &this->string || f == &this->textColor || f == &this->name || f == &this->size) {
            textGeometryDirty = true;
        }
    }

    inherited::notify(list);
}

void SoStringLabel::ensureTextGeometry(SoState* state)
{
    if (!state || !textTexture || !textVertexProperty || !textFaceSet || !textSwitch) {
        return;
    }

    const SbString* values = string.getValues(0);
    const int count = string.getNum();
    if (count <= 0) {
        textSwitch->whichChild.setValue(SO_SWITCH_NONE);
        textVertexProperty->vertex.setNum(0);
        textVertexProperty->texCoord.setNum(0);
        textFaceSet->numVertices.setNum(0);
        textTexture->image.setValue(SbVec2s(0, 0), 0, nullptr);
        cachedImageWidth = 0;
        cachedImageHeight = 0;
        textGeometryDirty = false;
        return;
    }

    const SbMatrix& model = SoModelMatrixElement::get(state);
    const SbMatrix& viewing = SoViewingMatrixElement::get(state);
    const SbMatrix& projection = SoProjectionMatrixElement::get(state);
    const SbViewportRegion& viewport = SoViewportRegionElement::get(state);
    SbVec2s viewportSize = viewport.getViewportSizePixels();

    bool dirty = textGeometryDirty || !model.equals(cachedModelMatrix, 0.0f)
        || !viewing.equals(cachedViewingMatrix, 0.0f)
        || !projection.equals(cachedProjectionMatrix, 0.0f) || cachedViewportSize != viewportSize;

    if (!dirty) {
        return;
    }

    cachedModelMatrix = model;
    cachedViewingMatrix = viewing;
    cachedProjectionMatrix = projection;
    cachedViewportSize = viewportSize;
    textGeometryDirty = false;

    SbMatrix combined = model * viewing * projection;
    SbVec3f anchor3(0.0f, 0.0f, 0.0f);
    combined.multVecMatrix(anchor3, anchor3);
    anchor3[0] = (anchor3[0] + 1.0f) * 0.5f * viewportSize[0];
    anchor3[1] = (anchor3[1] + 1.0f) * 0.5f * viewportSize[1];
    cachedAnchor.setValue(anchor3[0], anchor3[1]);

    QStringList lines;
    lines.reserve(count);
    QString fontFamily = QString::fromLatin1(name.getValue().getString());
    QFont font(fontFamily);
    const int fontSize = std::max(1, size.getValue());
    font.setPixelSize(fontSize);
    font.setStyleStrategy(QFont::NoAntialias);

    QFontMetrics metrics(font);
    int maxWidth = 0;
    for (int i = 0; i < count; ++i) {
        QString line = QString::fromUtf8(values[i].getString());
        maxWidth = std::max<int>(maxWidth, QtTools::horizontalAdvance(metrics, line));
        lines << line;
    }

    if (maxWidth <= 0) {
        maxWidth = 1;
    }

    const int lineHeight = std::max(1, metrics.height());
    const int ascent = metrics.ascent();
    const int descent = metrics.descent();
    const int textHeight = std::max(1, ascent + descent + (count - 1) * lineHeight);

    QImage image(maxWidth, textHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(font);

    const SbColor sbColor = this->textColor.getValue();
    QColor colorQt;
    colorQt.setRgbF(sbColor[0], sbColor[1], sbColor[2]);
    painter.setPen(colorQt);

    int baseline = ascent;
    for (const QString& line : lines) {
        painter.drawText(0, baseline, line);
        baseline += lineHeight;
    }
    painter.end();

    SoSFImage sfImage;
    Gui::BitmapFactory().convert(image, sfImage);
    textTexture->image = sfImage;

    cachedImageWidth = maxWidth;
    cachedImageHeight = textHeight;

    const float anchorX = cachedAnchor[0];
    const float anchorY = cachedAnchor[1];
    // Anchor the text block at its projected top-center point.
    const float left = anchorX - 0.5f * static_cast<float>(maxWidth);
    const float right = left + static_cast<float>(maxWidth);
    const float top = anchorY;
    const float bottom = top - static_cast<float>(textHeight);

    textVertexProperty->vertex.setNum(4);
    textVertexProperty->vertex.set1Value(0, SbVec3f(left, bottom, 0.0f));
    textVertexProperty->vertex.set1Value(1, SbVec3f(right, bottom, 0.0f));
    textVertexProperty->vertex.set1Value(2, SbVec3f(right, top, 0.0f));
    textVertexProperty->vertex.set1Value(3, SbVec3f(left, top, 0.0f));

    textVertexProperty->texCoord.setNum(4);
    textVertexProperty->texCoord.set1Value(0, SbVec2f(0.0f, 0.0f));
    textVertexProperty->texCoord.set1Value(1, SbVec2f(1.0f, 0.0f));
    textVertexProperty->texCoord.set1Value(2, SbVec2f(1.0f, 1.0f));
    textVertexProperty->texCoord.set1Value(3, SbVec2f(0.0f, 1.0f));

    textFaceSet->numVertices.set1Value(0, 4);
    textSwitch->whichChild.setValue(0);
}

// ------------------------------------------------------

SO_NODE_SOURCE(SoFrameLabel)

void SoFrameLabel::initClass()
{
    SO_NODE_INIT_CLASS(SoFrameLabel, SoImage, "Image");
}

SoFrameLabel::SoFrameLabel()
{
    SO_NODE_CONSTRUCTOR(SoFrameLabel);
    SO_NODE_ADD_FIELD(string, (""));
    SO_NODE_ADD_FIELD(textColor, (SbVec3f(1.0f, 1.0f, 1.0f)));
    SO_NODE_ADD_FIELD(backgroundColor, (SbVec3f(0.0f, 0.333f, 1.0f)));
    SO_NODE_ADD_FIELD(justification, (LEFT));
    SO_NODE_ADD_FIELD(name, ("Helvetica"));
    SO_NODE_ADD_FIELD(size, (12));
    SO_NODE_ADD_FIELD(frame, (true));
    SO_NODE_ADD_FIELD(border, (true));
    SO_NODE_ADD_FIELD(backgroundUseBaseColor, (false));
    SO_NODE_ADD_FIELD(textUseBaseColor, (false));
    // SO_NODE_ADD_FIELD(image, (SbVec2s(0,0), 0, NULL));
}

void SoFrameLabel::setIcon(const QPixmap& pixMap)
{
    iconPixmap = pixMap;
    imageDirty = true;
    touch();
}

void SoFrameLabel::notify(SoNotList* list)
{
    SoField* f = list->getLastField();
    if (f == &this->string || f == &this->textColor || f == &this->backgroundColor
        || f == &this->justification || f == &this->name || f == &this->size || f == &this->frame
        || f == &this->border || f == &this->backgroundUseBaseColor || f == &this->textUseBaseColor) {
        imageDirty = true;
    }

    inherited::notify(list);
}

void SoFrameLabel::drawImage(const SbColor& effectiveBackground, const SbColor& effectiveText)
{
    const SbString* s = string.getValues(0);
    int num = string.getNum();
    if (num == 0) {
        this->image = SoSFImage();
        return;
    }

    QFont font(QString::fromLatin1(QByteArray(name.getValue())), size.getValue());
    QFontMetrics fm(font);
    int w = 0;
    int h = fm.height() * num;
    QColor backgroundBrush;
    backgroundBrush.setRgbF(effectiveBackground[0], effectiveBackground[1], effectiveBackground[2]);
    QColor front;
    front.setRgbF(effectiveText[0], effectiveText[1], effectiveText[2]);
    const QPen borderPen(QColor(0, 0, 127), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    QStringList lines;
    for (int i = 0; i < num; i++) {
        QString line = QString::fromUtf8(s[i].getString());
        w = std::max<int>(w, QtTools::horizontalAdvance(fm, line));
        lines << line;
    }

    int padding = 5;

    bool drawIcon = false;
    QImage iconImg;
    int widthIcon = 0;
    int heightIcon = 0;
    if (!iconPixmap.isNull()) {
        drawIcon = true;
        iconImg = iconPixmap.toImage();
        widthIcon = iconImg.width() + 2 * padding;
        heightIcon = iconImg.height() + 2 * padding;
    }

    int widthText = w + 2 * padding;
    int heightText = h + 2 * padding;
    int widthTotal = widthText + widthIcon;
    int heightTotal = heightText > heightIcon ? heightText : heightIcon;
    int paddingTextV = (heightTotal - h) / 2;
    int paddingIconV = (heightTotal - iconImg.height()) / 2;

    QImage image(widthTotal, heightTotal, QImage::Format_ARGB32_Premultiplied);
    image.fill(0x00000000);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    SbBool drawFrame = frame.getValue();
    SbBool drawBorder = border.getValue();
    if (drawFrame || drawBorder) {
        painter.setPen(drawBorder ? borderPen : QPen(Qt::transparent));
        painter.setBrush(QBrush(drawFrame ? backgroundBrush : Qt::transparent));

        QRectF rectangle(0.0, 0.0, widthTotal, heightTotal);
        painter.drawRoundedRect(rectangle, 5, 5);
    }

    if (drawIcon) {
        painter.drawImage(QPoint(padding, paddingIconV), iconImg);
    }

    painter.setPen(front);

    Qt::Alignment align = Qt::AlignVCenter;
    if (justification.getValue() == 0) {
        align = Qt::AlignVCenter | Qt::AlignLeft;
    }
    else if (justification.getValue() == 1) {
        align = Qt::AlignVCenter | Qt::AlignRight;
    }
    else {
        align = Qt::AlignVCenter | Qt::AlignHCenter;
    }
    QString text = lines.join(QLatin1String("\n"));
    painter.setFont(font);
    painter.drawText(widthIcon + padding, paddingTextV, w, h, align, text);
    painter.end();

    SoSFImage sfimage;
    Gui::BitmapFactory().convert(image, sfimage);
    this->image = sfimage;
}

void SoFrameLabel::prepareImage(SoState* state)
{
    if (!state) {
        return;
    }

    const SbColor& diffuse = SoLazyElement::getDiffuse(state, 0);
    const SbColor effectiveBackground = backgroundUseBaseColor.getValue()
        ? diffuse
        : backgroundColor.getValue();
    const SbColor effectiveText = textUseBaseColor.getValue() ? diffuse : textColor.getValue();

    if (imageDirty || !effectiveColorsValid || effectiveBackground != cachedEffectiveBackground
        || effectiveText != cachedEffectiveText) {
        drawImage(effectiveBackground, effectiveText);
        cachedEffectiveBackground = effectiveBackground;
        cachedEffectiveText = effectiveText;
        effectiveColorsValid = true;
        imageDirty = false;
    }
}

/**
 * Renders the open edges only.
 */
void SoFrameLabel::GLRender(SoGLRenderAction* action)
{
    prepareImage(action ? action->getState() : nullptr);
    inherited::GLRender(action);
}
