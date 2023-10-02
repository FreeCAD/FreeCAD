/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMenu>
# include <QFont>
# include <QFontMetrics>
# include <QImage>
# include <QPainter>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoAsciiText.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoImage.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoPointSet.h>
# include <Inventor/nodes/SoRotationXYZ.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTranslation.h>
#endif
# include <Inventor/draggers/SoTranslate2Dragger.h>

#include <App/Annotation.h>
#include <App/Document.h>
#include <App/PropertyStandard.h>
#include <Base/Parameter.h>

#include "ViewProviderAnnotation.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "SoFCSelection.h"
#include "SoTextLabel.h"
#include "Tools.h"
#include "Window.h"


using namespace Gui;

const char* ViewProviderAnnotation::JustificationEnums[]= {"Left","Right","Center",nullptr};
const char* ViewProviderAnnotation::RotationAxisEnums[]= {"X","Y","Z",nullptr};

PROPERTY_SOURCE(Gui::ViewProviderAnnotation, Gui::ViewProviderDocumentObject)


ViewProviderAnnotation::ViewProviderAnnotation()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    unsigned long col = hGrp->GetUnsigned("AnnotationTextColor",4294967295UL); // light grey
    float r,g,b;
    r = ((col >> 24) & 0xff) / 255.0; g = ((col >> 16) & 0xff) / 255.0; b = ((col >> 8) & 0xff) / 255.0;
    ADD_PROPERTY(TextColor,(r,g,b));
    ADD_PROPERTY(Justification,((long)0));
    Justification.setEnums(JustificationEnums);
    ADD_PROPERTY(FontSize,(12));
    ADD_PROPERTY(FontName,("Arial"));
    ADD_PROPERTY(LineSpacing,(1.0));
    ADD_PROPERTY(Rotation,(0));
    ADD_PROPERTY(RotationAxis,((long)2));
    RotationAxis.setEnums(RotationAxisEnums);

    pFont = new SoFont();
    pFont->ref();
    pLabel = new SoText2();
    pLabel->ref();
    pLabel3d = new SoAsciiText();
    pLabel3d->ref();
    pColor = new SoBaseColor();
    pColor->ref();
    pTranslation = new SoTranslation();
    pTranslation->ref();
    pRotationXYZ = new SoRotationXYZ();
    pRotationXYZ->ref();

    RotationAxis.touch();
    TextColor.touch();
    FontSize.touch();
    FontName.touch();

    sPixmap = "Tree_Annotation";
}

ViewProviderAnnotation::~ViewProviderAnnotation()
{
    pFont->unref();
    pLabel->unref();
    pLabel3d->unref();
    pColor->unref();
    pTranslation->unref();
    pRotationXYZ->unref();
}

void ViewProviderAnnotation::onChanged(const App::Property* prop)
{
    if (prop == &TextColor) {
        const App::Color& c = TextColor.getValue();
        pColor->rgb.setValue(c.r,c.g,c.b);
    }
    else if (prop == &Justification) {
        if (Justification.getValue() == 0) {
            pLabel->justification = SoText2::LEFT;
            pLabel3d->justification = SoAsciiText::LEFT;
        }
        else if (Justification.getValue() == 1) {
            pLabel->justification = SoText2::RIGHT;
            pLabel3d->justification = SoAsciiText::RIGHT;
        }
        else if (Justification.getValue() == 2) {
            pLabel->justification = SoText2::CENTER;
            pLabel3d->justification = SoAsciiText::CENTER;
        }
    }
    else if (prop == &FontSize) {
        pFont->size = FontSize.getValue();
    }
    else if (prop == &FontName) {
        pFont->name = FontName.getValue();
    }
    else if (prop == &LineSpacing) {
        pLabel->spacing = LineSpacing.getValue();
        pLabel3d->spacing = LineSpacing.getValue();
    }
    else if (prop == &RotationAxis) {
        if (RotationAxis.getValue() == 0) {
            pRotationXYZ->axis = SoRotationXYZ::X;
        }
        else if (RotationAxis.getValue() == 1) {
            pRotationXYZ->axis = SoRotationXYZ::Y;
        }
        else if (RotationAxis.getValue() == 2) {
            pRotationXYZ->axis = SoRotationXYZ::Z;
        }
    }
    else if (prop == &Rotation) {
        pRotationXYZ->angle = (Rotation.getValue()/360)*(2*M_PI);
    }
    else {
        ViewProviderDocumentObject::onChanged(prop);
    }
}

std::vector<std::string> ViewProviderAnnotation::getDisplayModes() const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.emplace_back("Screen");
    StrList.emplace_back("World");
    return StrList;
}

void ViewProviderAnnotation::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Screen") == 0)
        setDisplayMaskMode("Screen");
    else if (strcmp(ModeName, "World")==0)
        setDisplayMaskMode("World");

    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

void ViewProviderAnnotation::attach(App::DocumentObject* f)
{
    ViewProviderDocumentObject::attach(f);

    auto anno = new SoAnnotation();
    auto anno3d = new SoAnnotation();

    auto textsep = new SoFCSelection();

    // set selection/highlight colors
    float transparency;
    ParameterGrp::handle hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("View");
    SbColor highlightColor = textsep->colorHighlight.getValue();
    auto highlight = (unsigned long)(highlightColor.getPackedValue());
    highlight = hGrp->GetUnsigned("HighlightColor", highlight);
    highlightColor.setPackedValue((uint32_t)highlight, transparency);
    textsep->colorHighlight.setValue(highlightColor);
    // Do the same with the selection color
    SbColor selectionColor = textsep->colorSelection.getValue();
    auto selection = (unsigned long)(selectionColor.getPackedValue());
    selection = hGrp->GetUnsigned("SelectionColor", selection);
    selectionColor.setPackedValue((uint32_t)selection, transparency);
    textsep->colorSelection.setValue(selectionColor);

    textsep->objectName = pcObject->getNameInDocument();
    textsep->documentName = pcObject->getDocument()->getName();
    textsep->subElementName = "Main";
    textsep->addChild(pTranslation);
    textsep->addChild(pRotationXYZ);
    textsep->addChild(pColor);
    textsep->addChild(pFont); // causes problems
    textsep->addChild(pLabel);

    auto textsep3d = new SoFCSelection();

    // set sel/highlight color here too
    textsep3d->colorHighlight.setValue(highlightColor);
    textsep3d->colorSelection.setValue(selectionColor);

    textsep3d->objectName = pcObject->getNameInDocument();
    textsep3d->documentName = pcObject->getDocument()->getName();
    textsep3d->subElementName = "Main";
    textsep3d->addChild(pTranslation);
    textsep3d->addChild(pRotationXYZ);
    textsep3d->addChild(pColor);
    textsep3d->addChild(pFont);
    textsep3d->addChild(pLabel3d);

    anno->addChild(textsep);
    anno3d->addChild(textsep3d);

    addDisplayMaskMode(anno, "Screen");
    addDisplayMaskMode(anno3d, "World");
}

void ViewProviderAnnotation::updateData(const App::Property* prop)
{
    if (prop->getTypeId() == App::PropertyStringList::getClassTypeId() &&
        strcmp(prop->getName(),"LabelText") == 0) {
        const std::vector<std::string> lines = static_cast<const App::PropertyStringList*>(prop)->getValues();
        int index=0;
        pLabel->string.setNum((int)lines.size());
        pLabel3d->string.setNum((int)lines.size());
        for (const auto & line : lines) {
            const char* cs = line.c_str();
            if (line.empty())
                cs = " "; // empty lines make coin crash, we use a space instead
#if (COIN_MAJOR_VERSION <= 3)
            QByteArray latin1str;
            latin1str = (QString::fromUtf8(cs)).toLatin1();
            pLabel->string.set1Value(index, SbString(latin1str.constData()));
            pLabel3d->string.set1Value(index, SbString(latin1str.constData()));
#else
            pLabel->string.set1Value(index, SbString(cs));
            pLabel3d->string.set1Value(index, SbString(cs));
#endif
            index++;
        }
    }
    else if (prop->getTypeId() == App::PropertyVector::getClassTypeId() &&
        strcmp(prop->getName(),"Position") == 0) {
        Base::Vector3d v = static_cast<const App::PropertyVector*>(prop)->getValue();
        pTranslation->translation.setValue(v.x,v.y,v.z);
    }

    ViewProviderDocumentObject::updateData(prop);
}

// ----------------------------------------------------------------------------

const char* ViewProviderAnnotationLabel::JustificationEnums[]= {"Left","Right","Center",nullptr};

PROPERTY_SOURCE(Gui::ViewProviderAnnotationLabel, Gui::ViewProviderDocumentObject)


ViewProviderAnnotationLabel::ViewProviderAnnotationLabel()
{
    ADD_PROPERTY(TextColor,(1.0f,1.0f,1.0f));
    ADD_PROPERTY(BackgroundColor,(0.0f,0.333f,1.0f));
    ADD_PROPERTY(Justification,((long)0));
    Justification.setEnums(JustificationEnums);
    QFont fn;
    ADD_PROPERTY(FontSize,(fn.pointSize()));
    ADD_PROPERTY(FontName,((const char*)fn.family().toLatin1()));
    ADD_PROPERTY(Frame,(true));

    pColor = new SoBaseColor();
    pColor->ref();
    pBaseTranslation = new SoTranslation();
    pBaseTranslation->ref();
    pTextTranslation = new SoTransform();
    pTextTranslation->ref();
    pCoords = new SoCoordinate3();
    pCoords->ref();
    pImage = new SoImage();
    pImage->ref();

    BackgroundColor.touch();

    sPixmap = "Tree_Annotation";
}

ViewProviderAnnotationLabel::~ViewProviderAnnotationLabel()
{
    pColor->unref();
    pBaseTranslation->unref();
    pTextTranslation->unref();
    pCoords->unref();
    pImage->unref();
}

void ViewProviderAnnotationLabel::onChanged(const App::Property* prop)
{
    if (prop == &BackgroundColor) {
        const App::Color& c = BackgroundColor.getValue();
        pColor->rgb.setValue(c.r,c.g,c.b);
    }
    if (prop == &TextColor || prop == &BackgroundColor ||
        prop == &Justification || prop == &FontSize ||
        prop == &FontName || prop == &Frame) {
        if (getObject()) {
            App::Property* label = getObject()->getPropertyByName("LabelText");
            if (label && label->getTypeId() == App::PropertyStringList::getClassTypeId())
                drawImage(static_cast<App::PropertyStringList*>(label)->getValues());
        }
    }
    else {
        ViewProviderDocumentObject::onChanged(prop);
    }
}

std::vector<std::string> ViewProviderAnnotationLabel::getDisplayModes() const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.emplace_back("Line");
    StrList.emplace_back("Object");
    return StrList;
}

void ViewProviderAnnotationLabel::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Line") == 0)
        setDisplayMaskMode("Line");
    else if (strcmp(ModeName, "Object")==0)
        setDisplayMaskMode("Object");

    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

void ViewProviderAnnotationLabel::attach(App::DocumentObject* f)
{
    ViewProviderDocumentObject::attach(f);

    // plain image
    SoSeparator* textsep = new SoAnnotation();
    textsep->addChild(pBaseTranslation);
    textsep->addChild(pImage);

    // image with line
    SoSeparator* linesep = new SoAnnotation();
    linesep->addChild(pBaseTranslation);
    linesep->addChild(pColor);
    linesep->addChild(pCoords);
    linesep->addChild(new SoLineSet());
    auto ds = new SoDrawStyle();
    ds->pointSize.setValue(3.0f);
    linesep->addChild(ds);
    linesep->addChild(new SoPointSet());
    linesep->addChild(pTextTranslation);
    linesep->addChild(pImage);

    addDisplayMaskMode(linesep, "Line");
    addDisplayMaskMode(textsep, "Object");
}

void ViewProviderAnnotationLabel::updateData(const App::Property* prop)
{
    if (prop->getTypeId() == App::PropertyStringList::getClassTypeId() &&
        strcmp(prop->getName(),"LabelText") == 0) {
        drawImage(static_cast<const App::PropertyStringList*>(prop)->getValues());
    }
    else if (prop->getTypeId() == App::PropertyVector::getClassTypeId() &&
        strcmp(prop->getName(),"BasePosition") == 0) {
        Base::Vector3d v = static_cast<const App::PropertyVector*>(prop)->getValue();
        pBaseTranslation->translation.setValue(v.x,v.y,v.z);
    }
    else if (prop->getTypeId() == App::PropertyVector::getClassTypeId() &&
        strcmp(prop->getName(),"TextPosition") == 0) {
        Base::Vector3d v = static_cast<const App::PropertyVector*>(prop)->getValue();
        pCoords->point.set1Value(1, SbVec3f(v.x,v.y,v.z));
        pTextTranslation->translation.setValue(v.x,v.y,v.z);
    }

    ViewProviderDocumentObject::updateData(prop);
}

bool ViewProviderAnnotationLabel::doubleClicked()
{
    Gui::Application::Instance->activeDocument()->setEdit(this);
    return true;
}

void ViewProviderAnnotationLabel::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    menu->addAction(QObject::tr("Move annotation"), receiver, member);
}

void ViewProviderAnnotationLabel::dragStartCallback(void *, SoDragger *)
{
    // This is called when a manipulator is about to manipulating
    Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Transform"));
}

void ViewProviderAnnotationLabel::dragFinishCallback(void *, SoDragger *)
{
    // This is called when a manipulator has done manipulating
    Gui::Application::Instance->activeDocument()->commitCommand();
}

void ViewProviderAnnotationLabel::dragMotionCallback(void *data, SoDragger *drag)
{
    auto that = static_cast<ViewProviderAnnotationLabel*>(data);
    const SbMatrix& mat = drag->getMotionMatrix();
    App::DocumentObject* obj = that->getObject();
    if (obj && obj->getTypeId() == App::AnnotationLabel::getClassTypeId()) {
        static_cast<App::AnnotationLabel*>(obj)->TextPosition.setValue(mat[3][0],mat[3][1],mat[3][2]);
    }
}

bool ViewProviderAnnotationLabel::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::FIRST);
    sa.setSearchingAll(false);
    sa.setNode(this->pTextTranslation);
    sa.apply(pcRoot);
    SoPath * path = sa.getPath();
    if (path) {
        auto manip = new TranslateManip;
        SoDragger* dragger = manip->getDragger();
        dragger->addStartCallback(dragStartCallback, this);
        dragger->addFinishCallback(dragFinishCallback, this);
        dragger->addMotionCallback(dragMotionCallback, this);
        return manip->replaceNode(path);
    }

    return false;
}

void ViewProviderAnnotationLabel::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    SoSearchAction sa;
    sa.setType(TranslateManip::getClassTypeId());
    sa.setInterest(SoSearchAction::FIRST);
    sa.apply(pcRoot);
    SoPath * path = sa.getPath();

    // No transform manipulator found.
    if (!path)
        return;

    auto manip = static_cast<TranslateManip*>(path->getTail());
    SoTransform* transform = this->pTextTranslation;
    manip->replaceManip(path, transform);
}

void ViewProviderAnnotationLabel::drawImage(const std::vector<std::string>& s)
{
    if (s.empty()) {
        pImage->image = SoSFImage();
        this->hide();
        return;
    }

    QFont font(QString::fromLatin1(this->FontName.getValue()), (int)this->FontSize.getValue());
    QFontMetrics fm(font);
    int w = 0;
    int h = fm.height() * s.size();
    const App::Color& b = this->BackgroundColor.getValue();
    QColor brush;
    brush.setRgbF(b.r,b.g,b.b);
    const App::Color& t = this->TextColor.getValue();
    QColor front;
    front.setRgbF(t.r,t.g,t.b);

    QStringList lines;
    for (const auto & it : s) {
        QString line = QString::fromUtf8(it.c_str());
        w = std::max<int>(w, QtTools::horizontalAdvance(fm, line));
        lines << line;
    }

    QImage image(w+10,h+10,QImage::Format_ARGB32_Premultiplied);
    image.fill(0x00000000);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    bool drawFrame = this->Frame.getValue();
    if (drawFrame) {
        painter.setPen(QPen(QColor(0,0,127), 2, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
        painter.setBrush(QBrush(brush, Qt::SolidPattern));
        QRectF rectangle(0.0, 0.0, w+10, h+10);
        painter.drawRoundedRect(rectangle, 5, 5);
    }

    painter.setPen(front);

    Qt::Alignment align = Qt::AlignVCenter;
    if (Justification.getValue() == 0)
        align = Qt::AlignVCenter | Qt::AlignLeft;
    else if (Justification.getValue() == 1)
        align = Qt::AlignVCenter | Qt::AlignRight;
    else
        align = Qt::AlignVCenter | Qt::AlignHCenter;
    QString text = lines.join(QLatin1String("\n"));
    painter.setFont(font);
    painter.drawText(5,5,w,h,align,text);
    painter.end();

    SoSFImage sfimage;
    Gui::BitmapFactory().convert(image, sfimage);
    pImage->image = sfimage;
}
