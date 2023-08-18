/***************************************************************************
 *   Copyright (c) 2022 edi                                                *
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

#ifndef TECHDRAWGUI_TASKSURFACEFINISHSYMBOLS_H
#define TECHDRAWGUI_TASKSURFACEFINISHSYMBOLS_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


class QComboBox;
class QLineEdit;
class QGraphicsScene;

namespace App {
class DocumentObject;
}

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawViewPart;
class CosmeticEdge;
class LineFormat;
}

namespace TechDraw
{
class Face;
}

namespace TechDrawGui
{
class QGSPage;
class QGIView;
class QGIPrimPath;
class ViewProviderViewPart;
class Ui_TaskSurfaceFinishSymbols;

class SvgString
// Class to create an SVG as a string
{
    std::stringstream svgStream;

public:

    SvgString(int width, int height);
    void addLine(int xStart, int yStart, int xEnd, int yEnd);
    void addCircle(int xCenter, int yCenter, int radius);
    void addText(int xText, int yText, std::string text);
    std::string finish();

}; // SvgString

class TaskSurfaceFinishSymbols : public QWidget
{
    Q_OBJECT

public:
    explicit TaskSurfaceFinishSymbols(TechDraw::DrawViewPart* view);
    ~TaskSurfaceFinishSymbols() override = default;

    virtual bool accept();
    virtual bool reject();
    void updateTask();

protected:
    void changeEvent(QEvent *event) override;
    void setUiEdit();

private:
    enum symbolType {anyMethod=0, removeProhibit, removeRequired,
                     anyMethodAll, removeProhibitAll, removeRequiredAll};
    QPixmap baseSymbol(symbolType type);
    std::string completeSymbol();
    TechDraw::DrawViewPart* selectedView;
    QGraphicsScene* symbolScene;     //note this is not QGSPage, but another scene only used to
                                     //display symbols in this task's ui
    std::vector<std::string> raValues, laySymbols, roughGrades;
    QGraphicsProxyWidget *proxyRA, *proxySamLength, *proxyMinRough, *proxyMaxRough;
    QLineEdit *leMethod, *leSamLength, *leAddition;
    QComboBox *cbRA, *cbMinRought, *cbMaxRought, *cbLay;
    symbolType activeIcon;
    bool isISO;
    std::unique_ptr<Ui_TaskSurfaceFinishSymbols> ui;

private Q_SLOTS:
    void onIconChanged();
    void onISO();
    void onASME();

}; // class TaskSurfaceFinishSymbols

class TaskDlgSurfaceFinishSymbols : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgSurfaceFinishSymbols(TechDraw::DrawViewPart* view);
    ~TaskDlgSurfaceFinishSymbols() override;

    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user presses the help button
    bool isAllowedAlterDocument() const override
                        { return false; }
    void update();

private:
    TaskSurfaceFinishSymbols* widget;
    Gui::TaskView::TaskBox* taskbox;
}; // class TaskDlgSurfaceFinishSymbols

} // namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKSURFACEFINISHSYMBOLS_H
