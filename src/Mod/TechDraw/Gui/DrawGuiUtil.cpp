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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>

#include <QBitmap>
#include <QColor>
#include <QComboBox>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QString>

# include <BRepAdaptor_Surface.hxx>
# include <BRepLProp_SLProps.hxx>
# include <gp_Dir.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyPythonObject.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Base/Type.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/PrefWidgets.h>
#include <Inventor/SbVec3f.h>

#include <Mod/TechDraw/App/ArrowPropEnum.h>
#include <Mod/TechDraw/App/BalloonPropEnum.h>
#include <Mod/TechDraw/App/LineNameEnum.h>
#include <Mod/TechDraw/App/MattingPropEnum.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/LineGenerator.h>
#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "DlgPageChooser.h"
#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "QGSPage.h"
#include "ViewProviderPage.h"
#include "Rez.h"

using namespace TechDrawGui;
using namespace TechDraw;
using DU = DrawUtil;

void DrawGuiUtil::loadArrowBox(QComboBox* qcb)
{
    qcb->clear();

    auto curStyleSheet =
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow")
            ->GetASCII("StyleSheet", "None");

    int i = 0;
    for (; i < ArrowPropEnum::ArrowCount; i++) {
        qcb->addItem(
            QCoreApplication::translate("ArrowPropEnum", ArrowPropEnum::ArrowTypeEnums[i]));
        QIcon itemIcon(QString::fromUtf8(ArrowPropEnum::ArrowTypeIcons[i].c_str()));
        if (isStyleSheetDark(curStyleSheet)) {
            QColor textColor = Preferences::lightTextColor().asValue<QColor>();
            QSize iconSize(48, 48);
            QIcon itemUpdatedIcon(maskBlackPixels(itemIcon, iconSize, textColor));
            qcb->setItemIcon(i, itemUpdatedIcon);
        }
        else {
            qcb->setItemIcon(i, itemIcon);
        }
    }
}

void DrawGuiUtil::loadBalloonShapeBox(QComboBox* qballooncb)
{
    qballooncb->clear();

    auto curStyleSheet =
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow")
            ->GetASCII("StyleSheet", "None");

    int i = 0;
    for (; i < BalloonPropEnum::BalloonCount; i++) {
        qballooncb->addItem(
            QCoreApplication::translate("BalloonPropEnum", BalloonPropEnum::BalloonTypeEnums[i]));
        QIcon itemIcon(QString::fromUtf8(BalloonPropEnum::BalloonTypeIcons[i].c_str()));
        if (isStyleSheetDark(curStyleSheet)) {
            QColor textColor = Preferences::lightTextColor().asValue<QColor>();
            QSize iconSize(48, 48);
            QIcon itemUpdatedIcon(maskBlackPixels(itemIcon, iconSize, textColor));
            qballooncb->setItemIcon(i, itemUpdatedIcon);
        }
        else {
            qballooncb->setItemIcon(i, itemIcon);
        }
    }
}

void DrawGuiUtil::loadMattingStyleBox(QComboBox* qmattingcb)
{
    qmattingcb->clear();
    auto curStyleSheet =
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow")
            ->GetASCII("StyleSheet", "None");

    int i = 0;
    for (; i < MattingPropEnum::MattingCount; i++) {
        qmattingcb->addItem(
            QCoreApplication::translate("MattingPropEnum", MattingPropEnum::MattingTypeEnums[i]));
        QIcon itemIcon(QString::fromUtf8(MattingPropEnum::MattingTypeIcons[i].c_str()));
        if (isStyleSheetDark(curStyleSheet)) {
            QColor textColor = Preferences::lightTextColor().asValue<QColor>();
            QSize iconSize(48, 48);
            QIcon itemUpdatedIcon(maskBlackPixels(itemIcon, iconSize, textColor));
            qmattingcb->setItemIcon(i, itemUpdatedIcon);
        }
        else {
            qmattingcb->setItemIcon(i, itemIcon);
        }
    }
}

void DrawGuiUtil::loadLineStandardsChoices(QComboBox* combo)
{
    combo->clear();
    std::vector<std::string> choices = LineGenerator::getAvailableLineStandards();
    for (auto& entry : choices) {
        QString qentry = QString::fromStdString(entry);
        combo->addItem(qentry);
    }
}

void DrawGuiUtil::loadLineStyleChoices(QComboBox* combo, LineGenerator* generator)
{
    combo->clear();
    std::vector<std::string> choices;
    if (generator) {
        choices = generator->getLoadedDescriptions();
    }
    else {
        choices = LineGenerator::getLineDescriptions();
    }

    auto translationContext = LineName::currentTranslationContext();
    int itemNumber {0};
    for (auto& entry : choices) {
        QString qentry = QCoreApplication::translate(translationContext.c_str(), entry.c_str());
        combo->addItem(qentry);
        if (generator) {
            combo->setItemIcon(itemNumber, iconForLine(itemNumber + 1, generator));
        }
        itemNumber++;
    }
}

void DrawGuiUtil::loadLineGroupChoices(QComboBox* combo)
{
    combo->clear();
    std::string lgFileName = Preferences::lineGroupFile();
    std::string lgRecord = LineGroup::getGroupNamesFromFile(lgFileName);
    // split collected groups
    std::stringstream ss(lgRecord);
    std::vector<QString> lgNames;
    while (std::getline(ss, lgRecord, ',')) {
        lgNames.push_back(QString::fromStdString(lgRecord));
    }
    // fill the combobox with the found names
    for (auto& name : lgNames) {
        combo->addItem(name);
    }
}


//! make an icon that shows a sample of lineNumber in the current line standard
QIcon DrawGuiUtil::iconForLine(size_t lineNumber,
                               TechDraw::LineGenerator* generator)
{
    //    Base::Console().Message("DGU::iconForLine(lineNumber: %d)\n", lineNumber);
    constexpr int iconSize {64};
    constexpr int borderSize {4};
    constexpr double iconLineWeight {1.0};
    size_t lineCount {4};
    double maxLineLength = iconSize - borderSize * 2.0;
    QBitmap bitmap {iconSize, iconSize};
    bitmap.clear();

    QPainter painter(&bitmap);
    QPen linePen = generator->getLinePen(lineNumber, iconLineWeight);
    linePen.setDashOffset(0.0);
    linePen.setCapStyle(Qt::FlatCap);
    linePen.setColor(Qt::color1);

    QSize lineIconSize(iconSize, iconSize);

    auto curStyleSheet =
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow")
            ->GetASCII("StyleSheet", "None");
    QColor textColor{Qt::black};
    if (isStyleSheetDark(curStyleSheet)) {
        textColor = Preferences::lightTextColor().asValue<QColor>();
    }

    // handle simple case of continuous line
    if (linePen.style() == Qt::SolidLine) {
        linePen.setWidthF(iconLineWeight * lineCount);
        painter.setPen(linePen);
        painter.drawLine(borderSize, iconSize / 2, iconSize - borderSize, iconSize / 2);
        if (isStyleSheetDark(curStyleSheet)) {
            QIcon lineItemIcon(bitmap);
            return QIcon(maskBlackPixels(lineItemIcon, lineIconSize, textColor));
        }
        else {
            return QIcon(bitmap);
        }
    }

    // dashed line
    linePen.setWidthF(iconLineWeight);
    painter.setPen(linePen);
    double yHeight = (iconSize / 2) - (lineCount * iconLineWeight);
    size_t iLine = 0;
    // draw multiple lines to stretch the line vertically without horizontal
    // distortion
    for (; iLine < lineCount; iLine++) {
        painter.drawLine(borderSize, yHeight, maxLineLength, yHeight);
        yHeight += iconLineWeight;
    }
    if (isStyleSheetDark(curStyleSheet)) {
        QIcon lineItemIcon(bitmap);
        return QIcon(maskBlackPixels(lineItemIcon, lineIconSize, textColor));
    }
    else {
        return QIcon(bitmap);
    }
}


//===========================================================================
// validate helper routines
//===========================================================================

// find a page in Selection, Document or CurrentWindow.
TechDraw::DrawPage* DrawGuiUtil::findPage(Gui::Command* cmd, bool findAny)
{
    //    Base::Console().Message("DGU::findPage()\n");
    std::vector<std::string> names;
    std::vector<std::string> labels;
    auto docs = App::GetApplication().getDocuments();

    if (findAny) {
        // find a page in any open document
        std::vector<App::DocumentObject*> foundPageObjects;
        // no page found in the usual places, but we have been asked to search all
        // open documents for a page.
        auto docsAll = App::GetApplication().getDocuments();
        for (auto& doc : docsAll) {
            auto docPages = doc->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
            if (docPages.empty()) {
                // this open document has no TD pages
                continue;
            }
            foundPageObjects.insert(foundPageObjects.end(), docPages.begin(), docPages.end());
        }
        if (foundPageObjects.empty()) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("No page found"),
                                 QObject::tr("No Drawing Pages available."));
            return nullptr;
        }
        else if (foundPageObjects.size() > 1) {
            // multiple pages available, ask for help
            for (auto obj : foundPageObjects) {
                std::string name = obj->getNameInDocument();
                names.push_back(name);
                std::string label = obj->Label.getValue();
                labels.push_back(label);
            }
            DlgPageChooser dlg(labels, names, Gui::getMainWindow());
            if (dlg.exec() == QDialog::Accepted) {
                std::string selName = dlg.getSelection();
                App::Document* doc = cmd->getDocument();
                return static_cast<TechDraw::DrawPage*>(doc->getObject(selName.c_str()));
            }
        }
        else {
            // only 1 page found
            return static_cast<TechDraw::DrawPage*>(foundPageObjects.front());
        }
    }

    // check Selection for a page
    std::vector<App::DocumentObject*> selPages =
        cmd->getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (selPages.empty()) {
        // no page in selection, try this document
        auto docPages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
        if (docPages.empty()) {
            // we are only to look in this document, and there is no page in this document
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("No page found"),
                                 QObject::tr("No Drawing Pages in document."));
            return nullptr;
        }
        else if (docPages.size() > 1) {
            // multiple pages in document, use active page if there is one
            Gui::MainWindow* w = Gui::getMainWindow();
            Gui::MDIView* mv = w->activeWindow();
            MDIViewPage* mvp = dynamic_cast<MDIViewPage*>(mv);
            if (mvp) {
                QGSPage* qp = mvp->getViewProviderPage()->getQGSPage();
                return qp->getDrawPage();
            }
            else {
                // none of pages in document is active, ask for help
                for (auto obj : docPages) {
                    std::string name = obj->getNameInDocument();
                    names.push_back(name);
                    std::string label = obj->Label.getValue();
                    labels.push_back(label);
                }
                DlgPageChooser dlg(labels, names, Gui::getMainWindow());
                if (dlg.exec() == QDialog::Accepted) {
                    std::string selName = dlg.getSelection();
                    App::Document* doc = cmd->getDocument();
                    return static_cast<TechDraw::DrawPage*>(doc->getObject(selName.c_str()));
                }
                return nullptr;
            }
        }
        else {
            // only 1 page in document - use it
            return static_cast<TechDraw::DrawPage*>(docPages.front());
        }
    }
    else if (selPages.size() > 1) {
        // multiple pages in selection
        for (auto obj : selPages) {
            std::string name = obj->getNameInDocument();
            names.push_back(name);
            std::string label = obj->Label.getValue();
            labels.push_back(label);
        }
        DlgPageChooser dlg(labels, names, Gui::getMainWindow());
        if (dlg.exec() == QDialog::Accepted) {
            std::string selName = dlg.getSelection();
            App::Document* doc = cmd->getDocument();
            return static_cast<TechDraw::DrawPage*>(doc->getObject(selName.c_str()));
        }
    }
    else {
        // exactly 1 page in selection, use it
        return static_cast<TechDraw::DrawPage*>(selPages.front());
    }

    // we can not actually reach this point.
    return nullptr;
}

bool DrawGuiUtil::isDraftObject(App::DocumentObject* obj)
{
    bool result = false;
    App::PropertyPythonObject* proxy =
        dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));

    if (proxy) {
        // if no proxy, can not be Draft obj
        // if has proxy, might be Draft obj
        std::stringstream ss;
        Py::Object proxyObj = proxy->getValue();
        Base::PyGILStateLocker lock;
        try {
            if (proxyObj.hasAttr("__module__")) {
                Py::String mod(proxyObj.getAttr("__module__"));
                ss << (std::string)mod;
                if (ss.str().find("Draft") != std::string::npos) {
                    result = true;
                }
                else if (ss.str().find("draft") != std::string::npos) {
                    result = true;
                }
            }
        }
        catch (Py::Exception&) {
            Base::PyException e;  // extract the Python error text
            e.ReportException();
            result = false;
        }
    }
    return result;
}

bool DrawGuiUtil::isArchObject(App::DocumentObject* obj)
{
    bool result = false;
    App::PropertyPythonObject* proxy =
        dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));

    if (proxy) {
        // if no proxy, can not be Arch obj
        // if has proxy, might be Arch obj
        Py::Object proxyObj = proxy->getValue();
        std::stringstream ss;
        Base::PyGILStateLocker lock;
        try {
            if (proxyObj.hasAttr("__module__")) {
                Py::String mod(proxyObj.getAttr("__module__"));
                ss << (std::string)mod;
                // does this have to be an ArchSection, or can it be any Arch object?
                if (ss.str().find("Arch") != std::string::npos) {
                    result = true;
                }
            }
        }
        catch (Py::Exception&) {
            Base::PyException e;  // extract the Python error text
            e.ReportException();
            result = false;
        }
    }
    return result;
}

bool DrawGuiUtil::isArchSection(App::DocumentObject* obj)
{
    bool result = false;
    App::PropertyPythonObject* proxy =
        dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));

    if (proxy) {
        // if no proxy, can not be Arch obj
        // if has proxy, might be Arch obj
        Py::Object proxyObj = proxy->getValue();
        std::stringstream ss;
        Base::PyGILStateLocker lock;
        try {
            if (proxyObj.hasAttr("__module__")) {
                Py::String mod(proxyObj.getAttr("__module__"));
                ss << (std::string)mod;
                // does this have to be an ArchSection, or can it be other Arch objects?
                if (ss.str().find("ArchSectionPlane") != std::string::npos) {
                    result = true;
                }
            }
        }
        catch (Py::Exception&) {
            Base::PyException e;  // extract the Python error text
            e.ReportException();
            result = false;
        }
    }
    return result;
}

bool DrawGuiUtil::needPage(Gui::Command* cmd, bool findAny)
{
    if (findAny) {
        // look for any page in any open document
        auto docsAll = App::GetApplication().getDocuments();
        for (auto& doc : docsAll) {
            auto docPages = doc->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
            if (docPages.empty()) {
                // this open document has no TD pages
                continue;
            }
            else {
                // found at least 1 page
                return true;
            }
        }
        // did not find any pages
        return false;
    }

    // need a Document and a Page
    if (cmd->hasActiveDocument()) {
        auto drawPageType(TechDraw::DrawPage::getClassTypeId());
        auto selPages = cmd->getDocument()->getObjectsOfType(drawPageType);
        return !selPages.empty();
    }
    return false;
}

bool DrawGuiUtil::needView(Gui::Command* cmd, bool partOnly)
{
    bool haveView = false;
    if (cmd->hasActiveDocument()) {
        if (partOnly) {
            auto drawPartType(TechDraw::DrawViewPart::getClassTypeId());
            auto selParts = cmd->getDocument()->getObjectsOfType(drawPartType);
            if (!selParts.empty()) {
                haveView = true;
            }
        }
        else {
            auto drawViewType(TechDraw::DrawView::getClassTypeId());
            auto selParts = cmd->getDocument()->getObjectsOfType(drawViewType);
            if (!selParts.empty()) {
                haveView = true;
            }
        }
    }
    return haveView;
}

void DrawGuiUtil::dumpRectF(const char* text, const QRectF& r)
{
    Base::Console().Message("DUMP - dumpRectF - %s\n", text);
    double left = r.left();
    double right = r.right();
    double top = r.top();
    double bottom = r.bottom();
    Base::Console().Message("Extents: L: %.3f, R: %.3f, T: %.3f, B: %.3f\n",
                            left,
                            right,
                            top,
                            bottom);
    Base::Console().Message("Size: W: %.3f H: %.3f\n", r.width(), r.height());
    Base::Console().Message("Centre: (%.3f, %.3f)\n", r.center().x(), r.center().y());
}

void DrawGuiUtil::dumpPointF(const char* text, const QPointF& p)
{
    Base::Console().Message("DUMP - dumpPointF - %s\n", text);
    Base::Console().Message("Point: (%.3f, %.3f)\n", p.x(), p.y());
}

std::pair<Base::Vector3d, Base::Vector3d> DrawGuiUtil::get3DDirAndRot()
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    Base::Vector3d viewDir(0.0, -1.0, 0.0);   // default to front
    Base::Vector3d viewUp(0.0, 0.0, 1.0);     // default to top
    Base::Vector3d viewRight(1.0, 0.0, 0.0);  // default to right
    std::list<Gui::MDIView*> mdis = Gui::Application::Instance->activeDocument()->getMDIViews();
    Gui::View3DInventor* view;
    Gui::View3DInventorViewer* viewer = nullptr;
    for (auto& m : mdis) {  // find the 3D viewer
        view = dynamic_cast<Gui::View3DInventor*>(m);
        if (view) {
            viewer = view->getViewer();
            break;
        }
    }
    if (!viewer) {
        return std::make_pair(viewDir, viewRight);
    }

    // Coin is giving us a values like 0.000000134439 instead of 0.000000000000.
    // This small difference caused circles to be projected as ellipses among other
    // problems.
    // Since SbVec3f is single precision floating point, it is only good to 6-9
    // significant decimal digits, and the rest of TechDraw works with doubles
    // that are good to 15-18 significant decimal digits.
    // But. When a float is promoted to double the value is supposed to be unchanged!
    // So where do the garbage digits come from???
    // In any case, if we restrict directions to 6 digits, we avoid the problem.
    int digits(6);
    SbVec3f dvec = viewer->getViewDirection();
    double dvecX = roundToDigits(dvec[0], digits);
    double dvecY = roundToDigits(dvec[1], digits);
    double dvecZ = roundToDigits(dvec[2], digits);
    viewDir = Base::Vector3d(dvecX, dvecY, dvecZ);
    viewDir = viewDir * (-1.0);  // Inventor dir is opposite TD projection dir

    SbVec3f upvec = viewer->getUpDirection();
    double upvecX = roundToDigits(upvec[0], digits);
    double upvecY = roundToDigits(upvec[1], digits);
    double upvecZ = roundToDigits(upvec[2], digits);
    viewUp = Base::Vector3d(upvecX, upvecY, upvecZ);

    Base::Vector3d right = viewUp.Cross(viewDir);

    result = std::make_pair(viewDir, right);
    return result;
}

std::pair<Base::Vector3d, Base::Vector3d> DrawGuiUtil::getProjDirFromFace(App::DocumentObject* obj,
                                                                          std::string faceName)
{
    std::pair<Base::Vector3d, Base::Vector3d> d3Dirs = get3DDirAndRot();
    std::pair<Base::Vector3d, Base::Vector3d> dirs;
    dirs.first = Base::Vector3d(0.0, 0.0, 1.0);  // set a default
    dirs.second = Base::Vector3d(1.0, 0.0, 0.0);
    Base::Vector3d projDir, rotVec;
    projDir = d3Dirs.first;
    rotVec = d3Dirs.second;

    auto ts = Part::Feature::getShape(obj, faceName.c_str(), true);
    if (ts.IsNull() || ts.ShapeType() != TopAbs_FACE) {
        Base::Console().Warning("getProjDirFromFace(%s) is not a Face\n", faceName.c_str());
        return dirs;
    }

    const TopoDS_Face& face = TopoDS::Face(ts);
    TopAbs_Orientation orient = face.Orientation();
    BRepAdaptor_Surface adapt(face);

    double u1 = adapt.FirstUParameter();
    double u2 = adapt.LastUParameter();
    double v1 = adapt.FirstVParameter();
    double v2 = adapt.LastVParameter();
    double uMid = (u1 + u2) / 2.0;
    double vMid = (v1 + v2) / 2.0;

    BRepLProp_SLProps props(adapt, uMid, vMid, 2, Precision::Confusion());
    if (props.IsNormalDefined()) {
        gp_Dir vec = props.Normal();
        projDir = Base::Vector3d(vec.X(), vec.Y(), vec.Z());
        if (orient != TopAbs_FORWARD) {
            projDir = projDir * (-1.0);
        }
    }

    return std::make_pair(projDir, rotVec);
}

// converts original value to one with only digits significant figures
double DrawGuiUtil::roundToDigits(double original, int digits)
{
    double factor = std::pow(10.0, digits);
    double temp = original * factor;
    double rounded = std::round(temp);
    temp = rounded / factor;
    return temp;
}

// Returns true if the item or any of its descendants is selected
bool DrawGuiUtil::isSelectedInTree(QGraphicsItem* item)
{
    if (item) {
        if (item->isSelected()) {
            return true;
        }

        for (QGraphicsItem* child : item->childItems()) {
            if (isSelectedInTree(child)) {
                return true;
            }
        }
    }

    return false;
}

// Selects or deselects the item and all its descendants
void DrawGuiUtil::setSelectedTree(QGraphicsItem* item, bool selected)
{
    if (item) {
        item->setSelected(selected);

        for (QGraphicsItem* child : item->childItems()) {
            setSelectedTree(child, selected);
        }
    }
}


//! convert point from scene coords to mm and conventional Y axis (page coords).
Base::Vector3d DrawGuiUtil::fromSceneCoords(const Base::Vector3d& sceneCoord, bool invert)
{
    Base::Vector3d result;
    if (invert) {
        result = Rez::appX(DU::invertY(sceneCoord));
    } else {
        result = Rez::appX(sceneCoord);
    }
    return Rez::appX(DU::invertY(sceneCoord));
}

//! convert point from printed page coords to scene units (Rez(mm) and inverted Y axis (scene coords)
Base::Vector3d DrawGuiUtil::toSceneCoords(const Base::Vector3d& pageCoord, bool invert)
{
    Base::Vector3d result;
    if (invert) {
        result = Rez::guiX(DU::invertY(pageCoord));
    } else {
        result = Rez::guiX(pageCoord);
    }
    return result;
}

//! convert unscaled, unrotated point to scaled, rotated view coordinates
Base::Vector3d DrawGuiUtil::toGuiPoint(DrawView* obj, const Base::Vector3d& toConvert)
{
    Base::Vector3d result{toConvert};
    auto rotDegrees = obj->Rotation.getValue();
    if (rotDegrees != 0.0) {
        result.RotateZ(Base::toRadians(rotDegrees));
    }
    result *= obj->getScale();
    result = DU::invertY(result);
    result = Rez::guiX(result);

    return result;
}


//! true if targetObj is in the selection list
bool DrawGuiUtil::findObjectInSelection(const std::vector<Gui::SelectionObject>& selection,
                                        const App::DocumentObject& targetObject)
{
    for (auto& selObj : selection) {
        if (&targetObject == selObj.getObject()) {
            return true;
        }
    }
    return false;
}

std::vector<std::string>  DrawGuiUtil::getSubsForSelectedObject(const std::vector<Gui::SelectionObject>& selection,
                                                                App::DocumentObject* selectedObj)
{
    for (auto& selObj : selection) {
        if (selectedObj == selObj.getObject()) {
            return selObj.getSubNames();
        }
    }
    return {};
}

bool DrawGuiUtil::isStyleSheetDark(std::string curStyleSheet)
{
    if (curStyleSheet.find("dark") != std::string::npos ||
        curStyleSheet.find("Dark") != std::string::npos) {
        return true;
    }
    return false;
}


QIcon DrawGuiUtil::maskBlackPixels(QIcon itemIcon, QSize iconSize, QColor textColor)
{
    QPixmap originalPix = itemIcon.pixmap(iconSize, QIcon::Mode::Normal, QIcon::State::On);
    QPixmap filler(iconSize);
    filler.fill(QColor(textColor));
    filler.setMask(originalPix.createMaskFromColor(Qt::black, Qt::MaskOutColor));
    return filler;
}

