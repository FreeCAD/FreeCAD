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

class QComboBox;

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
class QGVPage;
class QGIView;
class QGIPrimPath;
class MDIViewPage;
class ViewProviderViewPart;
class Ui_TaskSurfaceFinishSymbols;

class SvgString
// Class to create an SVG as a string
{
    std::stringstream svgStream;

public:

    SvgString(int width, int height);
    void addLine(int x1, int y1, int x2, int y2);
    void addCircle(int xCenter, int yCenter, int radius);
    void addText(int xText, int yText, std::string text);
    std::string finish(void);

}; // SvgString

class TaskSurfaceFinishSymbols : public QWidget
{
    Q_OBJECT

public:
    TaskSurfaceFinishSymbols(TechDraw::DrawViewPart* view);
    ~TaskSurfaceFinishSymbols();

public Q_SLOTS:

public:
    virtual bool accept();
    virtual bool reject();
    void updateTask();
    
private Q_SLOTS:
    void onIconChanged();
    void onISO();
    void onASME();

protected Q_SLOTS:

protected:
    void changeEvent(QEvent *e);

    void setUiEdit(void);

private:
    enum symbolType {anyMethod=0, removeProhibit, removeRequired,
                     anyMethodAll, removeProhibitAll, removeRequiredAll};
    QPixmap baseSymbol(symbolType type);
    std::string completeSymbol();
    TechDraw::DrawViewPart* selectedView;
    QGraphicsScene* symbolScene;
    std::vector<std::string> raValues, laySymbols, roughGrades;
    QGraphicsProxyWidget *proxyRA, *proxySamLength, *proxyMinRough, *proxyMaxRough;
    QLineEdit *leMethod, *leSamLength, *leAddition;
    QComboBox *cbRA, *cbMinRought, *cbMaxRought, *cbLay;
    symbolType activeIcon;
    bool isISO;
    std::unique_ptr<Ui_TaskSurfaceFinishSymbols> ui;
}; // class TaskSurfaceFinishSymbols

class TaskDlgSurfaceFinishSymbols : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgSurfaceFinishSymbols(TechDraw::DrawViewPart* view);
    ~TaskDlgSurfaceFinishSymbols();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual void helpRequested() { return;}
    virtual bool isAllowedAlterDocument(void) const
                        { return false; }
    void update();

protected:

private:
    TaskSurfaceFinishSymbols* widget;
    Gui::TaskView::TaskBox* taskbox;
}; // class TaskDlgSurfaceFinishSymbols

} // namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKSURFACEFINISHSYMBOLS_H
